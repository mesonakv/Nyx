#include "VulkanContext.h"
#include "Logger.h"
#include <algorithm>

static void SetWindowFullscreen(SDL_Window* window, WindowMode mode) {
    switch (mode) {
    case WindowMode::Windowed:
        SDL_SetWindowFullscreen(window, 0);
        break;
    case WindowMode::Borderless:
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        break;
    case WindowMode::ExclusiveFullscreen:
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        break;
    }
}

VkSampleCountFlagBits VulkanContext::GetMSAASamples() const {
    switch (settings.msaaSamples) {
    case 1: return VK_SAMPLE_COUNT_1_BIT;
    case 2: return VK_SAMPLE_COUNT_2_BIT;
    case 4: return VK_SAMPLE_COUNT_4_BIT;
    case 8: return VK_SAMPLE_COUNT_8_BIT;
    default: return VK_SAMPLE_COUNT_4_BIT;
    }
}

void VulkanContext::Initialize(SDL_Window* window, const DisplaySettings& initialSettings) {
    settings = initialSettings;
    SetWindowFullscreen(window, settings.windowMode);

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Nyx";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Nyx Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions.data();
    vkCreateInstance(&createInfo, nullptr, &instance);

    SDL_Vulkan_CreateSurface(window, instance, &surface);

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physicalDevice = devices[0];

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    NYX_LOG_INFO("GPU: %s", props.deviceName);

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());

    for (uint32_t i = 0; i < count; i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsQueueFamily == -1) graphicsQueueFamily = i;
        if (presentSupport && presentQueueFamily == -1) presentQueueFamily = i;
    }

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::vector<int> uniqueFamilies = {graphicsQueueFamily};
    if (presentQueueFamily != graphicsQueueFamily) uniqueFamilies.push_back(presentQueueFamily);

    float priority = 1.0f;
    for (int family : uniqueFamilies) {
        VkDeviceQueueCreateInfo qi = {};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount = 1;
        qi.pQueuePriorities = &priority;
        queueInfos.push_back(qi);
    }

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = (uint32_t)queueInfos.size();
    dci.pQueueCreateInfos = queueInfos.data();
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExtensions;
    vkCreateDevice(physicalDevice, &dci, nullptr, &device);
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    VkCommandPoolCreateInfo pi = {};
    pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pi.queueFamilyIndex = graphicsQueueFamily;
    pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &pi, nullptr, &commandPool);

    RecreateSwapchain(window);
}

void VulkanContext::UpdateWindowMode(SDL_Window* window, WindowMode mode) {
    if (settings.windowMode == mode) return;
    settings.windowMode = mode;
    SetWindowFullscreen(window, mode);
}

void VulkanContext::DestroySwapchainResources() {
    vkDeviceWaitIdle(device);

    // 命令缓冲依赖 swapchain image 数量，必须在这里释放
    if (!commandBuffers.empty()) {
        vkFreeCommandBuffers(device, commandPool,
            (uint32_t)commandBuffers.size(), commandBuffers.data());
        commandBuffers.clear();
    }

    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    framebuffers.clear();
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    if (depthImageView != VK_NULL_HANDLE) vkDestroyImageView(device, depthImageView, nullptr);
    if (depthImage != VK_NULL_HANDLE) vkDestroyImage(device, depthImage, nullptr);
    if (depthImageMemory != VK_NULL_HANDLE) vkFreeMemory(device, depthImageMemory, nullptr);
    depthImageView = VK_NULL_HANDLE;
    depthImage = VK_NULL_HANDLE;
    depthImageMemory = VK_NULL_HANDLE;

    if (msaaColorImageView != VK_NULL_HANDLE) vkDestroyImageView(device, msaaColorImageView, nullptr);
    if (msaaColorImage != VK_NULL_HANDLE) vkDestroyImage(device, msaaColorImage, nullptr);
    if (msaaColorImageMemory != VK_NULL_HANDLE) vkFreeMemory(device, msaaColorImageMemory, nullptr);
    msaaColorImageView = VK_NULL_HANDLE;
    msaaColorImage = VK_NULL_HANDLE;
    msaaColorImageMemory = VK_NULL_HANDLE;

    for (auto view : swapchainImageViews) vkDestroyImageView(device, view, nullptr);
    swapchainImageViews.clear();
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
}

void VulkanContext::RecreateSwapchain(SDL_Window* window) {
    if (swapchain != VK_NULL_HANDLE) {
        DestroySwapchainResources();
    }

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    VkSurfaceFormatKHR chosen = formats[0];
    for (auto f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }
    swapchainFormat = chosen.format;

    VkExtent2D extent;
    if (caps.currentExtent.width != UINT32_MAX) {
        extent = caps.currentExtent;
    } else {
        extent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, (uint32_t)settings.width));
        extent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, (uint32_t)settings.height));
    }
    swapchainExtent = extent;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = surface;
    sci.minImageCount = imageCount;
    sci.imageFormat = swapchainFormat;
    sci.imageColorSpace = chosen.colorSpace;
    sci.imageExtent = swapchainExtent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (graphicsQueueFamily != presentQueueFamily) {
        uint32_t indices[] = {(uint32_t)graphicsQueueFamily, (uint32_t)presentQueueFamily};
        sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices = indices;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (settings.vsync) {
        sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    } else {
        sci.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    sci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain) != VK_SUCCESS) {
        NYX_LOG_FATAL("Failed to create swapchain");
    }

    uint32_t scImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &scImageCount, nullptr);
    swapchainImages.resize(scImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &scImageCount, swapchainImages.data());

    swapchainImageViews.resize(scImageCount);
    for (uint32_t i = 0; i < scImageCount; i++) {
        VkImageViewCreateInfo vi = {};
        vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image = swapchainImages[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = swapchainFormat;
        vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.baseMipLevel = 0;
        vi.subresourceRange.levelCount = 1;
        vi.subresourceRange.baseArrayLayer = 0;
        vi.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &vi, nullptr, &swapchainImageViews[i]);
    }

    VkSampleCountFlagBits msaaSamples = GetMSAASamples();

    VkImageCreateInfo msaaInfo = {};
    msaaInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    msaaInfo.imageType = VK_IMAGE_TYPE_2D;
    msaaInfo.extent.width = swapchainExtent.width;
    msaaInfo.extent.height = swapchainExtent.height;
    msaaInfo.extent.depth = 1;
    msaaInfo.mipLevels = 1;
    msaaInfo.arrayLayers = 1;
    msaaInfo.format = swapchainFormat;
    msaaInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    msaaInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    msaaInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    msaaInfo.samples = msaaSamples;
    msaaInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(device, &msaaInfo, nullptr, &msaaColorImage);

    VkMemoryRequirements msaaMemReqs;
    vkGetImageMemoryRequirements(device, msaaColorImage, &msaaMemReqs);
    VkMemoryAllocateInfo msaaAlloc = {};
    msaaAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    msaaAlloc.allocationSize = msaaMemReqs.size;
    msaaAlloc.memoryTypeIndex = FindMemoryType(msaaMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &msaaAlloc, nullptr, &msaaColorImageMemory);
    vkBindImageMemory(device, msaaColorImage, msaaColorImageMemory, 0);

    VkImageViewCreateInfo msaaViewInfo = {};
    msaaViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    msaaViewInfo.image = msaaColorImage;
    msaaViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    msaaViewInfo.format = swapchainFormat;
    msaaViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    msaaViewInfo.subresourceRange.baseMipLevel = 0;
    msaaViewInfo.subresourceRange.levelCount = 1;
    msaaViewInfo.subresourceRange.baseArrayLayer = 0;
    msaaViewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &msaaViewInfo, nullptr, &msaaColorImageView);

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkImageCreateInfo depthInfo = {};
    depthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthInfo.imageType = VK_IMAGE_TYPE_2D;
    depthInfo.extent.width = swapchainExtent.width;
    depthInfo.extent.height = swapchainExtent.height;
    depthInfo.extent.depth = 1;
    depthInfo.mipLevels = 1;
    depthInfo.arrayLayers = 1;
    depthInfo.format = depthFormat;
    depthInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthInfo.samples = msaaSamples;
    depthInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(device, &depthInfo, nullptr, &depthImage);

    VkMemoryRequirements depthMemReqs;
    vkGetImageMemoryRequirements(device, depthImage, &depthMemReqs);
    VkMemoryAllocateInfo depthAlloc = {};
    depthAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depthAlloc.allocationSize = depthMemReqs.size;
    depthAlloc.memoryTypeIndex = FindMemoryType(depthMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &depthAlloc, nullptr, &depthImageMemory);
    vkBindImageMemory(device, depthImage, depthImageMemory, 0);

    VkImageViewCreateInfo depthViewInfo = {};
    depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthViewInfo.image = depthImage;
    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format = depthFormat;
    depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewInfo.subresourceRange.baseMipLevel = 0;
    depthViewInfo.subresourceRange.levelCount = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &depthViewInfo, nullptr, &depthImageView);

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription resolveAttachment = {};
    resolveAttachment.format = swapchainFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef = {};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveRef = {};
    resolveRef.attachment = 2;
    resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pResolveAttachments = &resolveRef;
    subpass.pDepthStencilAttachment = &depthRef;

    std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment, resolveAttachment};

    VkRenderPassCreateInfo rpi = {};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount = (uint32_t)attachments.size();
    rpi.pAttachments = attachments.data();
    rpi.subpassCount = 1;
    rpi.pSubpasses = &subpass;
    vkCreateRenderPass(device, &rpi, nullptr, &renderPass);

    framebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkImageView fbAttachments[] = {msaaColorImageView, depthImageView, swapchainImageViews[i]};
        VkFramebufferCreateInfo fi = {};
        fi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fi.renderPass = renderPass;
        fi.attachmentCount = 3;
        fi.pAttachments = fbAttachments;
        fi.width = swapchainExtent.width;
        fi.height = swapchainExtent.height;
        fi.layers = 1;
        vkCreateFramebuffer(device, &fi, nullptr, &framebuffers[i]);
    }

    SDL_SetWindowSize(window, (int)swapchainExtent.width, (int)swapchainExtent.height);

    NYX_LOG_INFO("Swapchain recreated with MSAA x%d", settings.msaaSamples);
}

void VulkanContext::Cleanup() {
    vkDeviceWaitIdle(device);
    DestroySwapchainResources();
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

uint32_t VulkanContext::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    NYX_LOG_FATAL("Failed to find memory type");
    return 0;  // 不会到这里，但满足编译器
}