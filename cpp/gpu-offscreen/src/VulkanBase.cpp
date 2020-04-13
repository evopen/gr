#include "VulkanBase.h"

#include "Filesystem.h"
#include "Input.h"
#include "Shader.h"
#include "VulkanInitializer.h"

#include <VulkanTools.h>


#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <set>

void VulkanBase::Init()
{
    InitWindow();
    InitVulkan();
}

void VulkanBase::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(window_width, window_height, window_title.c_str(), nullptr, nullptr);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, window_width / 2, window_height / 2);
    dhh::input::camera = &camera;
    glfwSetCursorPosCallback(window, dhh::input::CursorPosCallback);
    glfwSetScrollCallback(window, dhh::input::ScrollCallback);
    glfwSetWindowPos(window, 500, 200);
}

void VulkanBase::InitVulkan()
{
    CreateInstance();
    if (enable_validation)
    {
        SetupDebugMessenger();
    }
    PickPhysicalDevice();
    FindQueueFamilyIndex();
    CreateLogicalDevice();
    CreateMemoryAllocator();
    CreateSwapchain();
    CreateSwapchainImageViews();
    CreateRenderPass();
    CreateDepthResources();
    CreateFramebuffers();
    CreateCommandPool();
    CreateSyncObjects();
    CreateDescriptorPool();
    AllocateCommandbuffers();
}

void VulkanBase::CreateInstance()
{
    VkApplicationInfo app_create_info;
    app_create_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_create_info.apiVersion         = VK_API_VERSION_1_1;
    app_create_info.applicationVersion = kAppVersion;
    app_create_info.engineVersion      = kEngineVersion;
    app_create_info.pApplicationName   = app_name.c_str();
    app_create_info.pEngineName        = "NO ENGINE";
    app_create_info.pNext              = nullptr;

    const std::vector<const char*> kRequiredExtensions = GetRequiredExtensions();
    const std::vector<const char*> kRequiredLayers     = GetRequiredLayers();

    VkInstanceCreateInfo instance_create_info;
    instance_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.enabledExtensionCount   = static_cast<uint32_t>(kRequiredExtensions.size());
    instance_create_info.ppEnabledExtensionNames = kRequiredExtensions.data();
    instance_create_info.enabledLayerCount       = static_cast<uint32_t>(kRequiredLayers.size());
    instance_create_info.ppEnabledLayerNames     = kRequiredLayers.data();
    instance_create_info.pApplicationInfo        = &app_create_info;
    instance_create_info.flags                   = VK_NULL_HANDLE;
    if (enable_validation)
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info;
        debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_utils_messenger_create_info.pNext = nullptr;
        debug_utils_messenger_create_info.flags = VK_NULL_HANDLE;
        debug_utils_messenger_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                                        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                                                        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debug_utils_messenger_create_info.pUserData       = nullptr;
        debug_utils_messenger_create_info.pfnUserCallback = &DebugCallback;
        instance_create_info.pNext                        = &debug_utils_messenger_create_info;
    }
    else
    {
        instance_create_info.pNext = nullptr;
    }
    vkCreateInstance(&instance_create_info, nullptr, &instance);
}

void VulkanBase::SetupDebugMessenger()
{
    auto create_debug_utils_messenger =
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = {};
    debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_utils_messenger_create_info.pfnUserCallback = &DebugCallback;

    if (create_debug_utils_messenger(instance, &debug_utils_messenger_create_info, nullptr, &debugMessenger_)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to setup debug messenger");
    }
}

void VulkanBase::PickPhysicalDevice()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    std::string device_name;
    std::string device_type;
    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        physical_device = device;
        device_name     = properties.deviceName;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            break;
        }
    }
    std::cout << "GPU Picked: " << device_name << "\n";
}

void VulkanBase::CreateLogicalDevice()
{
    float queue_priority                   = 1.f;
    std::set<uint32_t> unique_queue_family = {
        queue_family_index.graphics_family.value(),
        queue_family_index.present_family.value(),
        queue_family_index.transfer_family.value(),
    };

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    for (uint32_t queue_index : unique_queue_family)
    {
        VkDeviceQueueCreateInfo queue_create_info;
        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.flags            = VK_NULL_HANDLE;
        queue_create_info.pNext            = nullptr;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_info.queueCount       = 1;
        queue_create_info.queueFamilyIndex = queue_index;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures features = {};
    features.shaderFloat64            = VK_TRUE;
    features.fillModeNonSolid         = VK_FALSE;
    VkDeviceCreateInfo device_create_info;
    device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.enabledExtensionCount   = static_cast<uint32_t>(kDeviceExtensions.size());
    device_create_info.ppEnabledExtensionNames = kDeviceExtensions.data();
    device_create_info.enabledLayerCount       = 0;
    device_create_info.ppEnabledLayerNames     = nullptr;
    device_create_info.flags                   = VK_NULL_HANDLE;
    device_create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos       = queue_create_infos.data();
    device_create_info.pEnabledFeatures        = &features;
    device_create_info.pNext                   = nullptr;

    vkCreateDevice(physical_device, &device_create_info, nullptr, &device);

    vkGetDeviceQueue(device, queue_family_index.graphics_family.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_index.present_family.value(), 0, &present_queue);
    vkGetDeviceQueue(device, queue_family_index.transfer_family.value(), 0, &transfer_queue);
}

void VulkanBase::CreateMemoryAllocator()
{
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice         = physical_device;
    allocator_info.device                 = device;

    vmaCreateAllocator(&allocator_info, &allocator);
}

void VulkanBase::CreateSwapchain()
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    surface_format = ChooseSurfaceFormat();

    VkSwapchainCreateInfoKHR swapchain_create_info;
    swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext                 = nullptr;
    swapchain_create_info.flags                 = VK_NULL_HANDLE;
    swapchain_create_info.surface               = surface;
    swapchain_create_info.minImageCount         = 3;
    swapchain_create_info.imageFormat           = surface_format.format;
    swapchain_create_info.imageColorSpace       = surface_format.colorSpace;
    swapchain_create_info.imageExtent           = surface_capabilities.currentExtent;
    swapchain_create_info.imageArrayLayers      = 1;
    swapchain_create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = VK_NULL_HANDLE;
    swapchain_create_info.pQueueFamilyIndices   = nullptr;
    swapchain_create_info.preTransform          = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode           = ChoosePresentMode();
    swapchain_create_info.clipped               = VK_TRUE;
    swapchain_create_info.oldSwapchain          = nullptr;

    vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain);

    uint32_t image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());
}

void VulkanBase::CreateSwapchainImageViews()
{
    swapchain_image_views.resize(swapchain_images.size());
    for (size_t i = 0; i < swapchain_images.size(); i++)
    {
        swapchain_image_views[i] =
            CreateImageView(swapchain_images[i], surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void VulkanBase::CreateRenderPass()
{
    VkAttachmentDescription color_attachment;
    color_attachment.format         = surface_format.format;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.flags          = VK_NULL_HANDLE;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentDescription depth_attachment;
    depth_attachment.format         = depth_image_format;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.flags          = VK_NULL_HANDLE;
    depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    std::vector<VkAttachmentDescription> attachments = {color_attachment, depth_attachment};

    VkAttachmentReference color_attachment_ref;
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref;
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description;
    subpass_description.flags                   = VK_NULL_HANDLE;
    subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount    = 0;
    subpass_description.pInputAttachments       = nullptr;
    subpass_description.colorAttachmentCount    = 1;
    subpass_description.pColorAttachments       = &color_attachment_ref;
    subpass_description.pResolveAttachments     = nullptr;
    subpass_description.pDepthStencilAttachment = &depth_attachment_ref;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments    = nullptr;

    VkRenderPassCreateInfo render_pass_create_info;
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments    = attachments.data();
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_description;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies   = nullptr;
    render_pass_create_info.flags           = VK_NULL_HANDLE;
    render_pass_create_info.pNext           = nullptr;

    vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass);
}

void VulkanBase::CreateDepthResources()
{
    CreateImage(window_width, window_height, 1, sample_count, depth_image_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, depth_image, depth_image_allocation);
    depth_image_view = CreateImageView(depth_image, depth_image_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void VulkanBase::CreateFramebuffers()
{
    framebuffers.resize(swapchain_image_views.size());

    for (size_t i = 0; i < swapchain_image_views.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {
            swapchain_image_views[i],
            depth_image_view,
        };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass              = render_pass;
        framebuffer_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments            = attachments.data();
        framebuffer_info.width                   = window_width;
        framebuffer_info.height                  = window_height;
        framebuffer_info.layers                  = 1;

        if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VulkanBase::CreateCommandPool()
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex        = queue_family_index.graphics_family.value();

    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

void VulkanBase::CreateSyncObjects()
{
    image_available_semaphores.resize(kMaxFramesInFlight);
    render_finished_semaphores.resize(kMaxFramesInFlight);
    in_flight_fences.resize(kMaxFramesInFlight);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < kMaxFramesInFlight; i++)
    {
        if (vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS
            || vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}


void VulkanBase::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    };

    VkDescriptorPoolCreateInfo pool_create_info;
    pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.pNext         = nullptr;
    pool_create_info.flags         = VK_NULL_HANDLE;
    pool_create_info.maxSets       = static_cast<uint32_t>(swapchain_images.size());
    pool_create_info.poolSizeCount = pool_sizes.size();
    pool_create_info.pPoolSizes    = pool_sizes.data();

    vkCreateDescriptorPool(device, &pool_create_info, nullptr, &descriptor_pool);
}

void VulkanBase::AllocateCommandbuffers()
{
    command_buffers.resize(3);

    VkCommandBufferAllocateInfo info =
        dhh::initializer::CommandBufferAllocateInfo(command_pool, 3, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    vkAllocateCommandBuffers(device, &info, command_buffers.data());
}

VkPresentModeKHR VulkanBase::ChoosePresentMode()
{
    uint32_t supported_present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &supported_present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> available_modes(supported_present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &supported_present_mode_count, available_modes.data());
    if (std::find(available_modes.begin(), available_modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != available_modes.end())
    {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    else if (std::find(available_modes.begin(), available_modes.end(), VK_PRESENT_MODE_FIFO_KHR)
             != available_modes.end())
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    else
    {
        throw std::runtime_error("No supported present mode");
    }
}

void VulkanBase::CreateUniformBuffer(VkDeviceSize bufferSize)
{
    uniform_buffers.resize(swapchain_images.size());
    uniform_buffer_allocation.resize(swapchain_images.size());
    for (size_t i = 0; i < swapchain_images.size(); i++)
    {
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, uniform_buffers[i],
            uniform_buffer_allocation[i]);
    }
}

void VulkanBase::DrawFrame()
{
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fences[current_frame]);

    uint32_t image_index;
    vkAcquireNextImageKHR(
        device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

    VkSubmitInfo submit_info = {};
    submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[]      = {image_available_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount     = 1;
    submit_info.pWaitSemaphores        = wait_semaphores;
    submit_info.pWaitDstStageMask      = wait_stages;
    submit_info.commandBufferCount     = 1;
    submit_info.pCommandBuffers        = &command_buffers[image_index];

    VkSemaphore signal_semaphores[]  = {render_finished_semaphores[current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR present_info   = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain;
    present_info.pImageIndices      = &image_index;

    vkQueuePresentKHR(present_queue, &present_info);

    current_frame = (current_frame + 1) % kMaxFramesInFlight;
}

VkBool32 VulkanBase::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl << std::endl;
    return VK_FALSE;
}

std::vector<const char*> VulkanBase::GetRequiredExtensions()
{
    uint32_t extension_count              = 0;
    const char** glfw_required_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    std::vector<const char*> required_extensions(glfw_required_extensions, glfw_required_extensions + extension_count);

    if (enable_validation)
    {
        required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return required_extensions;
}

std::vector<const char*> VulkanBase::GetRequiredLayers()
{
    std::vector<const char*> required_layers(kExtraLayers);
    if (enable_validation)
    {
        required_layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    return required_layers;
}

void VulkanBase::FindQueueFamilyIndex()
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties.data());

    /// Find Graphics queue family index
    for (size_t i = 0; i < queue_family_properties.size(); i++)
    {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_family_index.graphics_family = i;
            break;
        }
    }

    // Get window surface from GLFW
    glfwCreateWindowSurface(instance, window, nullptr, &surface);

    /// Find queue family that support presentation
    VkBool32 present_support;
    for (size_t i = 0; i < queue_family_properties.size(); i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, static_cast<uint32_t>(i), surface, &present_support);
        if (present_support)
        {
            queue_family_index.present_family = i;
            break;
        }
    }

    /// Find Transfer queue family index
    for (size_t i = 0; i < queue_family_properties.size(); i++)
    {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            queue_family_index.transfer_family = i;
            break;
        }
    }

    if (!queue_family_index.IsComplete())
    {
        throw std::runtime_error("queue family incomplete");
    }
}

VkImageView VulkanBase::CreateImageView(
    VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, uint32_t mipLevels, VkImageViewType view_type)
{
    VkImageViewCreateInfo view_create_info;
    view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.image                           = image;
    view_create_info.flags                           = VK_NULL_HANDLE;
    view_create_info.format                          = format;
    view_create_info.viewType                        = view_type;
    view_create_info.subresourceRange.aspectMask     = aspectFlags;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.baseMipLevel   = 0;
    view_create_info.subresourceRange.layerCount     = 1;
    view_create_info.subresourceRange.levelCount     = mipLevels;
    view_create_info.pNext                           = nullptr;

    VkImageView image_view;
    vkCreateImageView(device, &view_create_info, nullptr, &image_view);

    return image_view;
}

VkSurfaceFormatKHR VulkanBase::ChooseSurfaceFormat()
{
    VkSurfaceFormatKHR surface_format;
    surface_format.format     = VK_FORMAT_B8G8R8A8_UNORM;
    surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    return surface_format;
}

void VulkanBase::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer,
    VmaAllocation& allocation)
{
    VkBufferCreateInfo buffer_create_info;
    buffer_create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext                 = nullptr;
    buffer_create_info.flags                 = VK_NULL_HANDLE;
    buffer_create_info.queueFamilyIndexCount = VK_NULL_HANDLE;
    buffer_create_info.pQueueFamilyIndices   = nullptr;
    buffer_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.size                  = size;
    buffer_create_info.usage                 = usage;

    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.usage                   = memoryUsage;

    vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &buffer, &allocation, nullptr);
}

void VulkanBase::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevelCount, VkSampleCountFlagBits sampleCount,
    VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage& image,
    VmaAllocation& allocation, uint32_t layers, VkImageCreateFlags flags)
{
    VkImageCreateInfo image_create_info;
    image_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext                 = nullptr;
    image_create_info.arrayLayers           = layers;
    image_create_info.extent.width          = width;
    image_create_info.extent.height         = height;
    image_create_info.extent.depth          = 1;
    image_create_info.mipLevels             = mipLevelCount;
    image_create_info.samples               = sampleCount;
    image_create_info.flags                 = flags;
    image_create_info.format                = format;
    image_create_info.tiling                = tiling;
    image_create_info.usage                 = usage;
    image_create_info.imageType             = VK_IMAGE_TYPE_2D;
    image_create_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.queueFamilyIndexCount = VK_NULL_HANDLE;
    image_create_info.pQueueFamilyIndices   = nullptr;
    image_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.usage                   = memoryUsage;

    vmaCreateImage(allocator, &image_create_info, &allocation_create_info, &image, &allocation, nullptr);
}

VkShaderModule VulkanBase::CreateShaderModule(const std::string& filename)
{
    const std::vector<char>& code        = dhh::filesystem::LoadFile(filename, true);
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize                 = code.size();
    create_info.pCode                    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shader_module;
}

VkCommandBuffer VulkanBase::CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
{
    VkCommandBuffer cmd_buf;
    VkCommandBufferAllocateInfo info = dhh::initializer::CommandBufferAllocateInfo(command_pool, 1, level);
    vkAllocateCommandBuffers(device, &info, &cmd_buf);
    VkCommandBufferBeginInfo beginInfo = dhh::initializer::CommandBufferBeginInfo();
    if (begin)
        vkBeginCommandBuffer(cmd_buf, &beginInfo);
    return cmd_buf;
}

void VulkanBase::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    if (commandBuffer == VK_NULL_HANDLE)
    {
        return;
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK_RESULT(vkQueueWaitIdle(queue));

    if (free)
    {
        vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
    }
}