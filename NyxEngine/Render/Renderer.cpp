#include "Renderer.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>

static std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
        exit(1);
    }
    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

void Renderer::Initialize(VulkanContext& ctx) {
    CreateBallMesh(ctx);
    CreateCrosshairMesh(ctx);
    CreateGroundMesh(ctx);
    CreateShadowMesh(ctx);
    CreateSkyMesh(ctx);          // 新增
    CreateGraphicsPipeline(ctx);
    CreateSkyPipeline(ctx);      // 新增

    VkSemaphoreCreateInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fi = {};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateSemaphore(ctx.device, &si, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(ctx.device, &si, nullptr, &renderFinishedSemaphore);
    vkCreateFence(ctx.device, &fi, nullptr, &inFlightFence);
}

void Renderer::CreateSkyMesh(VulkanContext& ctx) {
    // 全屏三角形：两个三角形组成四边形，覆�?NDC
    struct Vertex2D {
        float x, y;
    };

    std::vector<Vertex2D> verts = {
        {-1.0f, -1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},

        {-1.0f, -1.0f},
        { 1.0f,  1.0f},
        {-1.0f,  1.0f}
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

    std::cout << "Sky mesh created" << std::endl;
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
    binding.stride = 8; // vec2
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

    // 天空不写深度、不测试深度
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
    pushRange.size = 32; // 两个 vec4

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

    std::cout << "Sky pipeline created" << std::endl;
}

void Renderer::CreateBallMesh(VulkanContext& ctx) {
    const int latSegments = 32;
    const int lonSegments = 32;
    const float radius = 0.5f;

    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        float r, g, b;
    };

    std::vector<Vertex> vertices;
    for (int lat = 0; lat <= latSegments; lat++) {
        float theta = lat * 3.14159f / latSegments;
        for (int lon = 0; lon <= lonSegments; lon++) {
            float phi = lon * 2.0f * 3.14159f / lonSegments;
            float x = cos(phi) * sin(theta);
            float y = cos(theta);
            float z = sin(phi) * sin(theta);
            Vertex v;
            v.x = x * radius; v.y = y * radius; v.z = z * radius;
            v.nx = x; v.ny = y; v.nz = z;
            v.r = 0.36f; v.g = 0.36f; v.b = 0.84f;
            vertices.push_back(v);
        }
    }

    std::vector<Vertex> triVerts;
    for (int lat = 0; lat < latSegments; lat++) {
        for (int lon = 0; lon < lonSegments; lon++) {
            uint32_t first = lat * (lonSegments + 1) + lon;
            uint32_t second = first + lonSegments + 1;
            triVerts.push_back(vertices[first]);
            triVerts.push_back(vertices[second]);
            triVerts.push_back(vertices[first + 1]);
            triVerts.push_back(vertices[second]);
            triVerts.push_back(vertices[second + 1]);
            triVerts.push_back(vertices[first + 1]);
        }
    }

    vertexCount = (uint32_t)triVerts.size();
    VkDeviceSize bufferSize = sizeof(Vertex) * triVerts.size();

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = bufferSize;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(ctx.device, &bi, nullptr, &vertexBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, vertexBuffer, &memReqs);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = ctx.FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx.device, &ai, nullptr, &vertexBufferMemory);
    vkBindBufferMemory(ctx.device, vertexBuffer, vertexBufferMemory, 0);

    void* data;
    vkMapMemory(ctx.device, vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, triVerts.data(), (size_t)bufferSize);
    vkUnmapMemory(ctx.device, vertexBufferMemory);

    std::cout << "Ball mesh: " << vertexCount << " vertices" << std::endl;
}

void Renderer::CreateCrosshairMesh(VulkanContext& ctx) {
    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        float r, g, b;
    };

    float aspect = (float)ctx.swapchainExtent.width / (float)ctx.swapchainExtent.height;
    float lineLengthPixels = 10.0f;
    float lineWidthPixels = 2.0f;

    float halfLengthY = lineLengthPixels / (float)ctx.swapchainExtent.height;
    float halfWidthY = lineWidthPixels / (float)ctx.swapchainExtent.height;
    float halfLengthX = halfLengthY / aspect;
    float halfWidthX = halfWidthY / aspect;

    std::vector<Vertex> verts;
    auto addRect = [&](float x1, float y1, float x2, float y2) {
        verts.push_back({x1, y1, 0.0f, 0,0,1, 0.85f,0.56f,0.66f});
        verts.push_back({x2, y1, 0.0f, 0,0,1, 0.85f,0.56f,0.66f});
        verts.push_back({x2, y2, 0.0f, 0,0,1, 0.85f,0.56f,0.66f});
        verts.push_back({x1, y1, 0.0f, 0,0,1, 0.85f,0.56f,0.66f});
        verts.push_back({x2, y2, 0.0f, 0,0,1, 0.85f,0.56f,0.66f});
        verts.push_back({x1, y2, 0.0f, 0,0,1, 0.85f,0.56f,0.66f});
    };

    addRect(-halfLengthX, -halfWidthY, halfLengthX, halfWidthY);
    addRect(-halfWidthX, -halfLengthY, halfWidthX, halfLengthY);

    crosshairVertexCount = (uint32_t)verts.size();
    VkDeviceSize bufferSize = sizeof(Vertex) * verts.size();

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = bufferSize;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(ctx.device, &bi, nullptr, &crosshairVertexBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, crosshairVertexBuffer, &memReqs);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = ctx.FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx.device, &ai, nullptr, &crosshairVertexBufferMemory);
    vkBindBufferMemory(ctx.device, crosshairVertexBuffer, crosshairVertexBufferMemory, 0);

    void* data;
    vkMapMemory(ctx.device, crosshairVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, verts.data(), (size_t)bufferSize);
    vkUnmapMemory(ctx.device, crosshairVertexBufferMemory);

    std::cout << "Crosshair mesh: " << crosshairVertexCount << " vertices" << std::endl;
}

void Renderer::CreateGroundMesh(VulkanContext& ctx) {
    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        float r, g, b;
    };

    float size = 30.0f;
    float y = -2.0f;

    std::vector<Vertex> verts;
    auto addQuad = [&](float x1, float z1, float x2, float z2) {
        Vertex v;
        v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f;
        v.r = 0.7f; v.g = 0.7f; v.b = 0.72f;

        v.x = x1; v.y = y; v.z = z1; verts.push_back(v);
        v.x = x2; v.y = y; v.z = z1; verts.push_back(v);
        v.x = x2; v.y = y; v.z = z2; verts.push_back(v);

        v.x = x1; v.y = y; v.z = z1; verts.push_back(v);
        v.x = x2; v.y = y; v.z = z2; verts.push_back(v);
        v.x = x1; v.y = y; v.z = z2; verts.push_back(v);
    };

    addQuad(-size, -size, size, size);

    groundVertexCount = (uint32_t)verts.size();
    VkDeviceSize bufferSize = sizeof(Vertex) * verts.size();

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = bufferSize;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(ctx.device, &bi, nullptr, &groundVertexBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, groundVertexBuffer, &memReqs);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = ctx.FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx.device, &ai, nullptr, &groundVertexBufferMemory);
    vkBindBufferMemory(ctx.device, groundVertexBuffer, groundVertexBufferMemory, 0);

    void* data;
    vkMapMemory(ctx.device, groundVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, verts.data(), (size_t)bufferSize);
    vkUnmapMemory(ctx.device, groundVertexBufferMemory);

    std::cout << "Ground mesh: " << groundVertexCount << " vertices" << std::endl;
}

void Renderer::CreateShadowMesh(VulkanContext& ctx) {
    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        float r, g, b;
    };

    const int segments = 32;
    float radius = 0.5f;
    std::vector<Vertex> verts;
    Vertex center;
    center.x = 0; center.y = 0; center.z = 0;
    center.nx = 0; center.ny = 1; center.nz = 0;
    center.r = 0; center.g = 0; center.b = 0;
    verts.push_back(center);

    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.14159f / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        Vertex v;
        v.x = x; v.y = 0; v.z = z;
        v.nx = 0; v.ny = 1; v.nz = 0;
        v.r = 0; v.g = 0; v.b = 0;
        verts.push_back(v);
    }

    std::vector<Vertex> triVerts;
    for (int i = 1; i <= segments; i++) {
        triVerts.push_back(verts[0]);
        triVerts.push_back(verts[i]);
        triVerts.push_back(verts[i+1]);
    }

    shadowVertexCount = (uint32_t)triVerts.size();
    VkDeviceSize bufferSize = sizeof(Vertex) * triVerts.size();

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = bufferSize;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(ctx.device, &bi, nullptr, &shadowVertexBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, shadowVertexBuffer, &memReqs);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = ctx.FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx.device, &ai, nullptr, &shadowVertexBufferMemory);
    vkBindBufferMemory(ctx.device, shadowVertexBuffer, shadowVertexBufferMemory, 0);

    void* data;
    vkMapMemory(ctx.device, shadowVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, triVerts.data(), (size_t)bufferSize);
    vkUnmapMemory(ctx.device, shadowVertexBufferMemory);

    std::cout << "Shadow mesh: " << shadowVertexCount << " vertices" << std::endl;
}

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
    pushRange.size = 128;

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
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

    std::cout << "Pipeline created with MSAA x" << ctx.settings.msaaSamples << std::endl;
}

void Renderer::RecreatePipeline(VulkanContext& ctx) {
    vkDeviceWaitIdle(ctx.device);

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

    CreateGraphicsPipeline(ctx);
    CreateSkyPipeline(ctx);
}

void Renderer::RecordCommandBuffers(VulkanContext& ctx, glm::mat4 view, glm::mat4 proj,
                                    const std::vector<glm::vec3>& targetPositions,
                                    const std::vector<float>& targetScales,
                                    const std::vector<Material>& targetMaterials,
                                    const glm::vec3& lightDir,
                                    const glm::vec3& lightColor,
                                    float lightIntensity,
                                    const glm::vec4& skyTopColor,
                                    const glm::vec4& skyBottomColor,
                                    ImGuiManager* imgui) {
    if (ctx.commandBuffers.empty()) {
        ctx.commandBuffers.resize(ctx.framebuffers.size());
        VkCommandBufferAllocateInfo ai = {};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = ctx.commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = (uint32_t)ctx.commandBuffers.size();
        vkAllocateCommandBuffers(ctx.device, &ai, ctx.commandBuffers.data());
    } else {
        for (auto cmd : ctx.commandBuffers) {
            vkResetCommandBuffer(cmd, 0);
        }
    }

    struct ObjectPushData {
        glm::mat4 viewProj;
        glm::vec4 color;
        float metallic;
        float roughness;
        float emissive_strength;
        float pad;
        glm::vec4 lightDirAndIntensity;
        glm::vec4 lightColorAndPad;
    };

    struct SkyPushData {
        glm::vec4 topColor;
        glm::vec4 bottomColor;
    };

    for (size_t i = 0; i < ctx.commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo bi = {};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(ctx.commandBuffers[i], &bi);

        VkRenderPassBeginInfo rpi = {};
        rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpi.renderPass = ctx.renderPass;
        rpi.framebuffer = ctx.framebuffers[i];
        rpi.renderArea.extent = ctx.swapchainExtent;

        VkClearValue clearValues[3] = {};
        clearValues[0].color = {{0.968f, 0.961f, 0.949f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        clearValues[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        rpi.clearValueCount = 3;
        rpi.pClearValues = clearValues;

        vkCmdBeginRenderPass(ctx.commandBuffers[i], &rpi, VK_SUBPASS_CONTENTS_INLINE);

        // === 绘制天空 ===
        {
            VkDeviceSize skyOffset = 0;
            vkCmdBindPipeline(ctx.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline);
            vkCmdBindVertexBuffers(ctx.commandBuffers[i], 0, 1, &skyVertexBuffer, &skyOffset);
            SkyPushData skyPush;
            skyPush.topColor = skyTopColor;
            skyPush.bottomColor = skyBottomColor;
            vkCmdPushConstants(ctx.commandBuffers[i], skyPipelineLayout,
                               VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyPushData), &skyPush);
            vkCmdDraw(ctx.commandBuffers[i], 6, 1, 0, 0);
        }

        // === 绘制地面 ===
        vkCmdBindPipeline(ctx.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(ctx.commandBuffers[i], 0, 1, &groundVertexBuffer, offsets);
        ObjectPushData groundPush;
        groundPush.viewProj = proj * view;
        groundPush.color = glm::vec4(0.9f, 0.89f, 0.85f, 1.0f);
        groundPush.metallic = 0.0f;
        groundPush.roughness = 0.9f;
        groundPush.emissive_strength = 0.0f;
        groundPush.pad = 0.0f;
        groundPush.lightDirAndIntensity = glm::vec4(lightDir, lightIntensity);
        groundPush.lightColorAndPad = glm::vec4(lightColor, 0.0f);
        vkCmdPushConstants(ctx.commandBuffers[i], pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(ObjectPushData), &groundPush);
        vkCmdDraw(ctx.commandBuffers[i], groundVertexCount, 1, 0, 0);

        // === 绘制阴影 ===
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
            glm::mat4 viewProjShadow = proj * view * model;

            vkCmdBindVertexBuffers(ctx.commandBuffers[i], 0, 1, &shadowVertexBuffer, offsets);
            ObjectPushData shadowPush;
            shadowPush.viewProj = viewProjShadow;
            shadowPush.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            shadowPush.metallic = 0.0f;
            shadowPush.roughness = 1.0f;
            shadowPush.emissive_strength = 0.0f;
            shadowPush.pad = 0.0f;
            shadowPush.lightDirAndIntensity = glm::vec4(lightDir, lightIntensity);
            shadowPush.lightColorAndPad = glm::vec4(lightColor, 0.0f);
            vkCmdPushConstants(ctx.commandBuffers[i], pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(ObjectPushData), &shadowPush);
            vkCmdDraw(ctx.commandBuffers[i], shadowVertexCount, 1, 0, 0);
        }

        // === 绘制球体 ===
        vkCmdBindVertexBuffers(ctx.commandBuffers[i], 0, 1, &vertexBuffer, offsets);
        for (size_t t = 0; t < targetPositions.size(); t++) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), targetPositions[t]);
            model = glm::scale(model, glm::vec3(targetScales[t]));
            glm::mat4 viewProj = proj * view * model;

            ObjectPushData push;
            push.viewProj = viewProj;
            push.color = targetMaterials[t].color;
            push.metallic = targetMaterials[t].metallic;
            push.roughness = targetMaterials[t].roughness;
            push.emissive_strength = targetMaterials[t].emissive_strength;
            push.pad = 0.0f;
            push.lightDirAndIntensity = glm::vec4(lightDir, lightIntensity);
            push.lightColorAndPad = glm::vec4(lightColor, 0.0f);

            vkCmdPushConstants(ctx.commandBuffers[i], pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(ObjectPushData), &push);
            vkCmdDraw(ctx.commandBuffers[i], vertexCount, 1, 0, 0);
        }

        // === 绘制准星 ===
        VkDeviceSize crosshairOffset = 0;
        vkCmdBindVertexBuffers(ctx.commandBuffers[i], 0, 1, &crosshairVertexBuffer, &crosshairOffset);
        ObjectPushData crosshairPush;
        crosshairPush.viewProj = glm::mat4(1.0f);
        crosshairPush.color = glm::vec4(0.85f, 0.56f, 0.66f, 1.0f);
        crosshairPush.metallic = 0.0f;
        crosshairPush.roughness = 0.5f;
        crosshairPush.emissive_strength = 0.0f;
        crosshairPush.pad = 0.0f;
        crosshairPush.lightDirAndIntensity = glm::vec4(lightDir, lightIntensity);
        crosshairPush.lightColorAndPad = glm::vec4(lightColor, 0.0f);
        vkCmdPushConstants(ctx.commandBuffers[i], pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(ObjectPushData), &crosshairPush);
        vkCmdDraw(ctx.commandBuffers[i], crosshairVertexCount, 1, 0, 0);

        if (imgui) {
            imgui->Render(ctx.commandBuffers[i]);
        }

        vkCmdEndRenderPass(ctx.commandBuffers[i]);
        vkEndCommandBuffer(ctx.commandBuffers[i]);
    }
}

void Renderer::DrawFrame(VulkanContext& ctx, glm::mat4 view, glm::mat4 proj,
                         const std::vector<glm::vec3>& targetPositions,
                         const std::vector<float>& targetScales,
                         const std::vector<Material>& targetMaterials,
                         const glm::vec3& lightDir,
                         const glm::vec3& lightColor,
                         float lightIntensity,
                         const glm::vec4& skyTopColor,
                         const glm::vec4& skyBottomColor,
                         ImGuiManager* imgui) {
    vkWaitForFences(ctx.device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(ctx.device, 1, &inFlightFence);

    vkDeviceWaitIdle(ctx.device);
    RecordCommandBuffers(ctx, view, proj, targetPositions, targetScales, targetMaterials,
                         lightDir, lightColor, lightIntensity,
                         skyTopColor, skyBottomColor, imgui);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSem[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = waitSem;
    si.pWaitDstStageMask = waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &ctx.commandBuffers[imageIndex];
    VkSemaphore sigSem[] = {renderFinishedSemaphore};
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = sigSem;

    vkQueueSubmit(ctx.graphicsQueue, 1, &si, inFlightFence);

    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = sigSem;
    pi.swapchainCount = 1;
    pi.pSwapchains = &ctx.swapchain;
    pi.pImageIndices = &imageIndex;

    vkQueuePresentKHR(ctx.presentQueue, &pi);
}

void Renderer::Cleanup(VulkanContext& ctx) {
    vkDestroyFence(ctx.device, inFlightFence, nullptr);
    vkDestroySemaphore(ctx.device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(ctx.device, imageAvailableSemaphore, nullptr);

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
}
