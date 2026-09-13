#include "Renderer.h"
#include "Logger.h"
#include "../Resource/MeshData.h"
#include <fstream>
#include <cstring>
#include <cmath>

// ============ 每帧 uniform 数据 ============
struct FrameUniforms {
    glm::mat4 viewProj;
    glm::vec4 cameraPos;
    glm::vec4 lightDirAndIntensity;
    glm::vec4 lightColor;
    glm::vec4 ambientColor;
};

// ============ 每物体 push constant（96 字节）============
struct ObjectPushData {
    glm::mat4 model;
    glm::vec4 color;
    glm::vec4 material;
};

static std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        NYX_LOG_FATAL("Failed to open: %s", filename.c_str());
    }
    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

void Renderer::Initialize(VulkanContext& ctx, SDL_Window* w) {
    window = w;

    CreateBallMesh(ctx);
    CreateCrosshairMesh(ctx);
    CreateGroundMesh(ctx);
    CreateShadowMesh(ctx);
    CreateSkyMesh(ctx);

    CreateDescriptorSetLayout(ctx);
    CreateUniformBuffers(ctx);
    CreateDescriptorPool(ctx);
    CreateDescriptorSets(ctx);

    CreateGraphicsPipeline(ctx);
    CreateSkyPipeline(ctx);
    CreateDebugLinePipeline(ctx);

    debugDraw_.Initialize(ctx);

    CreateSyncObjects(ctx);
}

// ============ Descriptor / Uniform ============

void Renderer::CreateDescriptorSetLayout(VulkanContext& ctx) {
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;

    if (vkCreateDescriptorSetLayout(ctx.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        NYX_LOG_FATAL("Failed to create descriptor set layout");
    }
}

void Renderer::CreateUniformBuffers(VulkanContext& ctx) {
    VkDeviceSize bufferSize = sizeof(FrameUniforms);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBufferMemories.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo bi = {};
        bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size = bufferSize;
        bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(ctx.device, &bi, nullptr, &uniformBuffers[i]);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(ctx.device, uniformBuffers[i], &memReqs);

        VkMemoryAllocateInfo ai = {};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = memReqs.size;
        ai.memoryTypeIndex = ctx.FindMemoryType(memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(ctx.device, &ai, nullptr, &uniformBufferMemories[i]);
        vkBindBufferMemory(ctx.device, uniformBuffers[i], uniformBufferMemories[i], 0);

        vkMapMemory(ctx.device, uniformBufferMemories[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void Renderer::CreateDescriptorPool(VulkanContext& ctx) {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo pi = {};
    pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.poolSizeCount = 1;
    pi.pPoolSizes = &poolSize;
    pi.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(ctx.device, &pi, nullptr, &descriptorPool) != VK_SUCCESS) {
        NYX_LOG_FATAL("Failed to create descriptor pool");
    }
}

void Renderer::CreateDescriptorSets(VulkanContext& ctx) {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    VkDescriptorSetAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = descriptorPool;
    ai.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    ai.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(ctx.device, &ai, descriptorSets.data()) != VK_SUCCESS) {
        NYX_LOG_FATAL("Failed to allocate descriptor sets");
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(FrameUniforms);

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(ctx.device, 1, &write, 0, nullptr);
    }
}

void Renderer::DestroyUniformResources(VulkanContext& ctx) {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && i < uniformBuffers.size(); i++) {
        if (uniformBuffersMapped.size() > i && uniformBuffersMapped[i]) {
            vkUnmapMemory(ctx.device, uniformBufferMemories[i]);
            uniformBuffersMapped[i] = nullptr;
        }
        if (uniformBuffers[i] != VK_NULL_HANDLE) vkDestroyBuffer(ctx.device, uniformBuffers[i], nullptr);
        if (uniformBufferMemories[i] != VK_NULL_HANDLE) vkFreeMemory(ctx.device, uniformBufferMemories[i], nullptr);
    }
    uniformBuffers.clear();
    uniformBufferMemories.clear();
    uniformBuffersMapped.clear();

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    descriptorSets.clear();

    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}

// ============ Sync ============

void Renderer::CreateSyncObjects(VulkanContext& ctx) {
    size_t imageCount = ctx.swapchainImages.size();

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(imageCount);
    imagesInFlight.assign(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fi = {};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(ctx.device, &si, nullptr, &imageAvailableSemaphores[i]);
        vkCreateFence(ctx.device, &fi, nullptr, &inFlightFences[i]);
    }

    for (size_t i = 0; i < imageCount; i++) {
        vkCreateSemaphore(ctx.device, &si, nullptr, &renderFinishedSemaphores[i]);
    }

    currentFrame = 0;
}

void Renderer::DestroySyncObjects(VulkanContext& ctx) {
    for (auto s : imageAvailableSemaphores) vkDestroySemaphore(ctx.device, s, nullptr);
    imageAvailableSemaphores.clear();

    for (auto s : renderFinishedSemaphores) vkDestroySemaphore(ctx.device, s, nullptr);
    renderFinishedSemaphores.clear();

    for (auto f : inFlightFences) vkDestroyFence(ctx.device, f, nullptr);
    inFlightFences.clear();

    imagesInFlight.clear();
    currentFrame = 0;
}

// ============ 通用 mesh 上传 ============

void Renderer::UploadMeshData(VulkanContext& ctx, const void* vertexData, size_t vertexBytes,
                               VkBuffer& outBuffer, VkDeviceMemory& outMemory) {
    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = vertexBytes;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(ctx.device, &bi, nullptr, &outBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, outBuffer, &memReqs);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = ctx.FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx.device, &ai, nullptr, &outMemory);
    vkBindBufferMemory(ctx.device, outBuffer, outMemory, 0);

    void* data;
    vkMapMemory(ctx.device, outMemory, 0, vertexBytes, 0, &data);
    memcpy(data, vertexData, vertexBytes);
    vkUnmapMemory(ctx.device, outMemory);
}

// ============ Mesh ============

void Renderer::CreateSkyMesh(VulkanContext& ctx) {
    struct Vertex2D { float x, y; };

    std::vector<Vertex2D> verts = {
        {-1.0f, -1.0f}, { 1.0f, -1.0f}, { 1.0f,  1.0f},
        {-1.0f, -1.0f}, { 1.0f,  1.0f}, {-1.0f,  1.0f}
    };

    VkDeviceSize bufferSize = sizeof(Vertex2D) * verts.size();

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = bufferSize;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(ctx.device, &bi, nullptr, &skyVertexBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, skyVertexBuffer, &memReqs);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = ctx.FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx.device, &ai, nullptr, &skyVertexBufferMemory);
    vkBindBufferMemory(ctx.device, skyVertexBuffer, skyVertexBufferMemory, 0);

    void* data;
    vkMapMemory(ctx.device, skyVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, verts.data(), (size_t)bufferSize);
    vkUnmapMemory(ctx.device, skyVertexBufferMemory);

    NYX_LOG_INFO("Sky mesh created");
}

void Renderer::CreateBallMesh(VulkanContext& ctx) {
    const int latSegments = 32;
    const int lonSegments = 32;
    const float radius = 0.5f;

    MeshData mesh;

    std::vector<MeshVertex> grid;
    grid.reserve((latSegments + 1) * (lonSegments + 1));

    for (int lat = 0; lat <= latSegments; lat++) {
        float theta = lat * 3.14159f / latSegments;
        for (int lon = 0; lon <= lonSegments; lon++) {
            float phi = lon * 2.0f * 3.14159f / lonSegments;
            float x = cos(phi) * sin(theta);
            float y = cos(theta);
            float z = sin(phi) * sin(theta);

            MeshVertex v;
            v.position = glm::vec3(x * radius, y * radius, z * radius);
            v.normal = glm::vec3(x, y, z);
            v.color = glm::vec3(0.36f, 0.36f, 0.84f);
            grid.push_back(v);
        }
    }

    mesh.vertices.reserve(latSegments * lonSegments * 6);
    for (int lat = 0; lat < latSegments; lat++) {
        for (int lon = 0; lon < lonSegments; lon++) {
            uint32_t first = lat * (lonSegments + 1) + lon;
            uint32_t second = first + lonSegments + 1;
            mesh.vertices.push_back(grid[first]);
            mesh.vertices.push_back(grid[second]);
            mesh.vertices.push_back(grid[first + 1]);
            mesh.vertices.push_back(grid[second]);
            mesh.vertices.push_back(grid[second + 1]);
            mesh.vertices.push_back(grid[first + 1]);
        }
    }

    vertexCount = (uint32_t)mesh.vertices.size();
    UploadMeshData(ctx, mesh.vertices.data(), mesh.vertices.size() * sizeof(MeshVertex),
                   vertexBuffer, vertexBufferMemory);

    NYX_LOG_INFO("Ball mesh: %u vertices", vertexCount);
}

void Renderer::CreateCrosshairMesh(VulkanContext& ctx) {
    float aspect = (float)ctx.swapchainExtent.width / (float)ctx.swapchainExtent.height;
    float lineLengthPixels = 10.0f;
    float lineWidthPixels = 2.0f;

    float halfLengthY = lineLengthPixels / (float)ctx.swapchainExtent.height;
    float halfWidthY = lineWidthPixels / (float)ctx.swapchainExtent.height;
    float halfLengthX = halfLengthY / aspect;
    float halfWidthX = halfWidthY / aspect;

    MeshData mesh;

    auto addRect = [&](float x1, float y1, float x2, float y2) {
        glm::vec3 color(0.85f, 0.56f, 0.66f);
        glm::vec3 normal(0.0f, 0.0f, 1.0f);
        mesh.vertices.push_back({glm::vec3(x1, y1, 0.0f), normal, color});
        mesh.vertices.push_back({glm::vec3(x2, y1, 0.0f), normal, color});
        mesh.vertices.push_back({glm::vec3(x2, y2, 0.0f), normal, color});
        mesh.vertices.push_back({glm::vec3(x1, y1, 0.0f), normal, color});
        mesh.vertices.push_back({glm::vec3(x2, y2, 0.0f), normal, color});
        mesh.vertices.push_back({glm::vec3(x1, y2, 0.0f), normal, color});
    };

    addRect(-halfLengthX, -halfWidthY, halfLengthX, halfWidthY);
    addRect(-halfWidthX, -halfLengthY, halfWidthX, halfLengthY);

    crosshairVertexCount = (uint32_t)mesh.vertices.size();
    UploadMeshData(ctx, mesh.vertices.data(), mesh.vertices.size() * sizeof(MeshVertex),
                   crosshairVertexBuffer, crosshairVertexBufferMemory);

    NYX_LOG_INFO("Crosshair mesh: %u vertices", crosshairVertexCount);
}

void Renderer::CreateGroundMesh(VulkanContext& ctx) {
    float size = 30.0f;
    float y = -2.0f;

    MeshData mesh;
    mesh.vertices.reserve(6);

    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    glm::vec3 color(0.7f, 0.7f, 0.72f);

    auto addVertex = [&](float px, float pz) {
        mesh.vertices.push_back({glm::vec3(px, y, pz), normal, color});
    };

    addVertex(-size, -size);
    addVertex( size, -size);
    addVertex( size,  size);

    addVertex(-size, -size);
    addVertex( size,  size);
    addVertex(-size,  size);

    groundVertexCount = (uint32_t)mesh.vertices.size();
    UploadMeshData(ctx, mesh.vertices.data(), mesh.vertices.size() * sizeof(MeshVertex),
                   groundVertexBuffer, groundVertexBufferMemory);

    NYX_LOG_INFO("Ground mesh: %u vertices", groundVertexCount);
}

void Renderer::CreateShadowMesh(VulkanContext& ctx) {
    const int segments = 32;
    float radius = 0.5f;

    MeshData mesh;
    mesh.vertices.reserve(segments * 3);

    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    glm::vec3 color(0.0f, 0.0f, 0.0f);

    std::vector<MeshVertex> ring;
    ring.reserve(segments + 2);
    ring.push_back({glm::vec3(0.0f, 0.0f, 0.0f), normal, color});

    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.14159f / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        ring.push_back({glm::vec3(x, 0.0f, z), normal, color});
    }

    for (int i = 1; i <= segments; i++) {
        mesh.vertices.push_back(ring[0]);
        mesh.vertices.push_back(ring[i]);
        mesh.vertices.push_back(ring[i + 1]);
    }

    shadowVertexCount = (uint32_t)mesh.vertices.size();
    UploadMeshData(ctx, mesh.vertices.data(), mesh.vertices.size() * sizeof(MeshVertex),
                   shadowVertexBuffer, shadowVertexBufferMemory);

    NYX_LOG_INFO("Shadow mesh: %u vertices", shadowVertexCount);
}

// ============ Pipeline ============

void Renderer::CreateGraphicsPipeline(VulkanContext& ctx) {
    auto vertCode = ReadFile("Shaders/ball.vert.spv");
    auto fragCode = ReadFile("Shaders/ball.frag.spv");

    VkShaderModuleCreateInfo smi = {};
    smi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smi.codeSize = vertCode.size();
    smi.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
    VkShaderModule vertModule;
    vkCreateShaderModule(ctx.device, &smi, nullptr, &vertModule);

    smi.codeSize = fragCode.size();
    smi.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
    VkShaderModule fragModule;
    vkCreateShaderModule(ctx.device, &smi, nullptr, &fragModule);

    VkPipelineShaderStageCreateInfo vertStage = {};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage = {};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    VkVertexInputBindingDescription binding = {};
    binding.binding = 0;
    binding.stride = 36;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[3] = {};
    attrs[0].binding = 0; attrs[0].location = 0; attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[0].offset = 0;
    attrs[1].binding = 0; attrs[1].location = 1; attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[1].offset = 12;
    attrs[2].binding = 0; attrs[2].location = 2; attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[2].offset = 24;

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 3;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {};
    viewport.width = (float)ctx.swapchainExtent.width;
    viewport.height = (float)ctx.swapchainExtent.height;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = ctx.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = ctx.GetMSAASamples();
    multisample.sampleShadingEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAtt = {};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAtt.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend = {};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAtt;

    VkPushConstantRange pushRange = {};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(ObjectPushData);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    vkCreatePipelineLayout(ctx.device, &layoutInfo, nullptr, &pipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &blend;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = ctx.renderPass;
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);

    vkDestroyShaderModule(ctx.device, vertModule, nullptr);
    vkDestroyShaderModule(ctx.device, fragModule, nullptr);

    NYX_LOG_INFO("Pipeline created with MSAA x%d", ctx.settings.msaaSamples);
}

void Renderer::CreateSkyPipeline(VulkanContext& ctx) {
    auto vertCode = ReadFile("Shaders/sky.vert.spv");
    auto fragCode = ReadFile("Shaders/sky.frag.spv");

    VkShaderModuleCreateInfo smi = {};
    smi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smi.codeSize = vertCode.size();
    smi.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
    VkShaderModule vertModule;
    vkCreateShaderModule(ctx.device, &smi, nullptr, &vertModule);

    smi.codeSize = fragCode.size();
    smi.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
    VkShaderModule fragModule;
    vkCreateShaderModule(ctx.device, &smi, nullptr, &fragModule);

    VkPipelineShaderStageCreateInfo vertStage = {};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage = {};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    VkVertexInputBindingDescription binding = {};
    binding.binding = 0;
    binding.stride = 8;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attr = {};
    attr.binding = 0;
    attr.location = 0;
    attr.format = VK_FORMAT_R32G32_SFLOAT;
    attr.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 1;
    vertexInput.pVertexAttributeDescriptions = &attr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {};
    viewport.width = (float)ctx.swapchainExtent.width;
    viewport.height = (float)ctx.swapchainExtent.height;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = ctx.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = ctx.GetMSAASamples();
    multisample.sampleShadingEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAtt = {};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAtt.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend = {};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAtt;

    VkPushConstantRange pushRange = {};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = 32;

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    vkCreatePipelineLayout(ctx.device, &layoutInfo, nullptr, &skyPipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &blend;
    pipelineInfo.layout = skyPipelineLayout;
    pipelineInfo.renderPass = ctx.renderPass;
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &skyPipeline);

    vkDestroyShaderModule(ctx.device, vertModule, nullptr);
    vkDestroyShaderModule(ctx.device, fragModule, nullptr);

    NYX_LOG_INFO("Sky pipeline created");
}

void Renderer::CreateDebugLinePipeline(VulkanContext& ctx) {
    auto vertCode = ReadFile("Shaders/debug_line.vert.spv");
    auto fragCode = ReadFile("Shaders/debug_line.frag.spv");

    VkShaderModuleCreateInfo smi = {};
    smi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smi.codeSize = vertCode.size();
    smi.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
    VkShaderModule vertModule;
    vkCreateShaderModule(ctx.device, &smi, nullptr, &vertModule);

    smi.codeSize = fragCode.size();
    smi.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
    VkShaderModule fragModule;
    vkCreateShaderModule(ctx.device, &smi, nullptr, &fragModule);

    VkPipelineShaderStageCreateInfo vertStage = {};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage = {};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // 顶点布局：position(12) + color(12) = 24 字节
    VkVertexInputBindingDescription binding = {};
    binding.binding = 0;
    binding.stride = 24;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2] = {};
    attrs[0].binding = 0; attrs[0].location = 0; attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[0].offset = 0;
    attrs[1].binding = 0; attrs[1].location = 1; attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[1].offset = 12;

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    VkViewport viewport = {};
    viewport.width = (float)ctx.swapchainExtent.width;
    viewport.height = (float)ctx.swapchainExtent.height;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = ctx.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;
    // 深度偏移，避免线和几何体共面时 z-fighting
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = -1.0f;
    rasterizer.depthBiasSlopeFactor = -1.0f;

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = ctx.GetMSAASamples();
    multisample.sampleShadingEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState blendAtt = {};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAtt.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend = {};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAtt;

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;

    vkCreatePipelineLayout(ctx.device, &layoutInfo, nullptr, &debugLinePipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &blend;
    pipelineInfo.layout = debugLinePipelineLayout;
    pipelineInfo.renderPass = ctx.renderPass;
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &debugLinePipeline);

    vkDestroyShaderModule(ctx.device, vertModule, nullptr);
    vkDestroyShaderModule(ctx.device, fragModule, nullptr);

    NYX_LOG_INFO("Debug line pipeline created");
}

void Renderer::RecreatePipeline(VulkanContext& ctx) {
    vkDeviceWaitIdle(ctx.device);

    DestroySyncObjects(ctx);

    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ctx.device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (skyPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ctx.device, skyPipeline, nullptr);
        skyPipeline = VK_NULL_HANDLE;
    }
    if (skyPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(ctx.device, skyPipelineLayout, nullptr);
        skyPipelineLayout = VK_NULL_HANDLE;
    }
    if (debugLinePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ctx.device, debugLinePipeline, nullptr);
        debugLinePipeline = VK_NULL_HANDLE;
    }
    if (debugLinePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(ctx.device, debugLinePipelineLayout, nullptr);
        debugLinePipelineLayout = VK_NULL_HANDLE;
    }

    CreateGraphicsPipeline(ctx);
    CreateSkyPipeline(ctx);
    CreateDebugLinePipeline(ctx);

    CreateSyncObjects(ctx);
}

// ============ 录制命令缓冲 ============

void Renderer::RecordCommandBuffer(VulkanContext& ctx, uint32_t imageIndex, const FrameData& frame) {
    if (ctx.commandBuffers.size() != ctx.framebuffers.size()) {
        if (!ctx.commandBuffers.empty()) {
            vkFreeCommandBuffers(ctx.device, ctx.commandPool,
                (uint32_t)ctx.commandBuffers.size(), ctx.commandBuffers.data());
            ctx.commandBuffers.clear();
        }
        ctx.commandBuffers.resize(ctx.framebuffers.size());
        VkCommandBufferAllocateInfo ai = {};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = ctx.commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = (uint32_t)ctx.commandBuffers.size();
        vkAllocateCommandBuffers(ctx.device, &ai, ctx.commandBuffers.data());
    }

    struct SkyPushData {
        glm::vec4 topColor;
        glm::vec4 bottomColor;
    };

    const auto& targetPositions = *frame.targetPositions;
    const auto& targetScales = *frame.targetScales;
    const auto& targetMaterialIndices = *frame.targetMaterialIndices;
    const MaterialLibrary& materialLib = *frame.materialLibrary;

    const glm::vec3& lightDir = frame.lighting.lightDir;
    const glm::vec4& skyTopColor = frame.lighting.skyTopColor;
    const glm::vec4& skyBottomColor = frame.lighting.skyBottomColor;

    VkCommandBuffer cmd = ctx.commandBuffers[imageIndex];

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &bi);

    VkRenderPassBeginInfo rpi = {};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpi.renderPass = ctx.renderPass;
    rpi.framebuffer = ctx.framebuffers[imageIndex];
    rpi.renderArea.extent = ctx.swapchainExtent;

    VkClearValue clearValues[3] = {};
    clearValues[0].color = {{0.968f, 0.961f, 0.949f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    clearValues[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    rpi.clearValueCount = 3;
    rpi.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpi, VK_SUBPASS_CONTENTS_INLINE);

    // 天空
    {
        VkDeviceSize skyOffset = 0;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline);
        vkCmdBindVertexBuffers(cmd, 0, 1, &skyVertexBuffer, &skyOffset);
        SkyPushData skyPush;
        skyPush.topColor = skyTopColor;
        skyPush.bottomColor = skyBottomColor;
        vkCmdPushConstants(cmd, skyPipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyPushData), &skyPush);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }

    // 主 pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                            0, 1, &descriptorSets[currentFrame], 0, nullptr);

    VkDeviceSize offsets[] = {0};

    // 地面
    {
        vkCmdBindVertexBuffers(cmd, 0, 1, &groundVertexBuffer, offsets);
        ObjectPushData push;
        push.model = glm::mat4(1.0f);
        push.color = glm::vec4(0.9f, 0.89f, 0.85f, 1.0f);
        push.material = glm::vec4(0.0f, 0.9f, 0.0f, 0.0f);
        vkCmdPushConstants(cmd, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(ObjectPushData), &push);
        vkCmdDraw(cmd, groundVertexCount, 1, 0, 0);
    }

    // 阴影
    if (!targetPositions.empty()) {
        vkCmdBindVertexBuffers(cmd, 0, 1, &shadowVertexBuffer, offsets);

        for (size_t t = 0; t < targetPositions.size(); t++) {
            glm::vec3 pos = targetPositions[t];
            float scale = targetScales[t];
            float heightAboveGround = pos.y - (-2.0f) - 0.5f * scale;
            glm::vec3 shadowOffset(0.0f);
            if (fabs(lightDir.y) > 0.001f) {
                float shadowScale = 0.25f;
                shadowOffset = -glm::vec3(lightDir.x, 0.0f, lightDir.z) * (heightAboveGround / lightDir.y) * shadowScale;
            }
            glm::vec3 shadowPos = glm::vec3(pos.x, -1.98f, pos.z) + shadowOffset;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), shadowPos);
            model = glm::scale(model, glm::vec3(scale, 1.0f, scale));

            ObjectPushData push;
            push.model = model;
            push.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            push.material = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            vkCmdPushConstants(cmd, pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(ObjectPushData), &push);
            vkCmdDraw(cmd, shadowVertexCount, 1, 0, 0);
        }
    }

    // 球体
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);
    for (size_t t = 0; t < targetPositions.size(); t++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), targetPositions[t]);
        model = glm::scale(model, glm::vec3(targetScales[t]));

        int matIdx = targetMaterialIndices[t];
        if (matIdx < 0 || matIdx >= (int)materialLib.materials.size()) matIdx = 0;
        const Material& mat = materialLib.materials[matIdx];

        ObjectPushData push;
        push.model = model;
        push.color = mat.color;
        push.material = glm::vec4(
            mat.metallic,
            mat.roughness,
            mat.emissive_strength,
            0.0f
        );

        vkCmdPushConstants(cmd, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(ObjectPushData), &push);
        vkCmdDraw(cmd, vertexCount, 1, 0, 0);
    }

    // 准星（屏幕空间）
    {
        VkDeviceSize crosshairOffset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &crosshairVertexBuffer, &crosshairOffset);
        ObjectPushData push;
        push.model = glm::mat4(1.0f);
        push.color = glm::vec4(0.85f, 0.56f, 0.66f, 1.0f);
        push.material = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);
        vkCmdPushConstants(cmd, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(ObjectPushData), &push);
        vkCmdDraw(cmd, crosshairVertexCount, 1, 0, 0);
    }

    // Debug 线段
    if (!debugDraw_.IsEmpty()) {
        VkDeviceSize debugOffset = 0;
        VkBuffer dbgBuf = debugDraw_.GetVertexBuffer();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugLinePipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugLinePipelineLayout,
                                0, 1, &descriptorSets[currentFrame], 0, nullptr);
        vkCmdBindVertexBuffers(cmd, 0, 1, &dbgBuf, &debugOffset);
        vkCmdDraw(cmd, debugDraw_.GetVertexCount(), 1, 0, 0);
    }

    if (frame.imgui) {
        frame.imgui->Render(cmd);
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

// ============ DrawFrame ============

void Renderer::DrawFrame(VulkanContext& ctx, const FrameData& frame) {
    vkWaitForFences(ctx.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(
        ctx.device, ctx.swapchain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        ctx.RecreateSwapchain(window);
        RecreatePipeline(ctx);
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        NYX_LOG_ERROR("vkAcquireNextImageKHR failed: %d", (int)acquireResult);
        return;
    }

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(ctx.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    vkResetFences(ctx.device, 1, &inFlightFences[currentFrame]);

    FrameUniforms ubo;
    ubo.viewProj = frame.proj * frame.view;
    ubo.cameraPos = glm::vec4(frame.cameraPos, 0.0f);
    ubo.lightDirAndIntensity = glm::vec4(frame.lighting.lightDir, frame.lighting.lightIntensity);
    ubo.lightColor = glm::vec4(frame.lighting.lightColor, 0.0f);
    ubo.ambientColor = glm::vec4(frame.lighting.ambientColor, 0.0f);
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));

    // 上传 Debug 线段数据到 GPU
    debugDraw_.Upload(ctx);

    RecordCommandBuffer(ctx, imageIndex, frame);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSem[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = waitSem;
    si.pWaitDstStageMask = waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &ctx.commandBuffers[imageIndex];
    VkSemaphore signalSem[] = {renderFinishedSemaphores[imageIndex]};
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = signalSem;

    vkQueueSubmit(ctx.graphicsQueue, 1, &si, inFlightFences[currentFrame]);

    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = signalSem;
    pi.swapchainCount = 1;
    pi.pSwapchains = &ctx.swapchain;
    pi.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(ctx.presentQueue, &pi);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        vkDeviceWaitIdle(ctx.device);
        ctx.RecreateSwapchain(window);
        RecreatePipeline(ctx);
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::Cleanup(VulkanContext& ctx) {
    DestroySyncObjects(ctx);
    DestroyUniformResources(ctx);

    // Debug Draw
    debugDraw_.Shutdown(ctx);

    vkDestroyBuffer(ctx.device, crosshairVertexBuffer, nullptr);
    vkFreeMemory(ctx.device, crosshairVertexBufferMemory, nullptr);

    vkDestroyBuffer(ctx.device, vertexBuffer, nullptr);
    vkFreeMemory(ctx.device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(ctx.device, groundVertexBuffer, nullptr);
    vkFreeMemory(ctx.device, groundVertexBufferMemory, nullptr);

    vkDestroyBuffer(ctx.device, shadowVertexBuffer, nullptr);
    vkFreeMemory(ctx.device, shadowVertexBufferMemory, nullptr);

    vkDestroyBuffer(ctx.device, skyVertexBuffer, nullptr);
    vkFreeMemory(ctx.device, skyVertexBufferMemory, nullptr);

    vkDestroyPipeline(ctx.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);

    vkDestroyPipeline(ctx.device, skyPipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, skyPipelineLayout, nullptr);

    if (debugLinePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ctx.device, debugLinePipeline, nullptr);
    }
    if (debugLinePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(ctx.device, debugLinePipelineLayout, nullptr);
    }
}