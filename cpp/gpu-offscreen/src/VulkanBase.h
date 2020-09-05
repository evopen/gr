#pragma once

#include "Camera.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <optional>
#include <string>
#include <vector>

struct QueueFamilyIndex
{
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    std::optional<uint32_t> transfer_family;

    bool IsComplete()
    {
        return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
    }
};

const std::vector<const char*> kDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const std::vector<const char*> kExtraLayers = {"VK_LAYER_LUNARG_monitor"};

const int kMaxFramesInFlight = 2;

class VulkanBase
{
public:
    uint32_t window_width  = 800;
    uint32_t window_height = 600;
    GLFWwindow* window;
    std::string app_name          = "numerous";
    std::string window_title      = app_name;
    const uint32_t kAppVersion    = VK_MAKE_VERSION(0, 0, 1);
    const uint32_t kEngineVersion = VK_MAKE_VERSION(0, 0, 1);
    bool enable_validation        = true;
    QueueFamilyIndex queue_family_index;

private:
    VkDebugUtilsMessengerEXT debugMessenger_;

public:
    VmaAllocator allocator;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    VkRenderPass render_pass;
    VkSurfaceFormatKHR surface_format;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkImage color_image;
    VmaAllocation color_image_allocation;
    VkImageView color_image_view;
    VkImage depth_image;
    VmaAllocation depth_image_allocation;
    VkImageView depth_image_view;
    VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
    VkFormat depth_image_format        = VK_FORMAT_D32_SFLOAT;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool command_pool;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    std::vector<VkCommandBuffer> command_buffers;
    VkQueue graphics_queue;
    VkQueue transfer_queue;
    VkQueue present_queue;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;
    std::vector<VkBuffer> uniform_buffers;
    std::vector<VmaAllocation> uniform_buffer_allocation;
    size_t current_frame = 0;
    dhh::camera::Camera camera;



public:
    explicit VulkanBase(bool enableValidation) : enable_validation(enableValidation) {}

    void Init();

private:
    void InitWindow();
    void InitVulkan();

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void PickPhysicalDevice();
    void FindQueueFamilyIndex();
    void CreateLogicalDevice();
    void CreateMemoryAllocator();
    void CreateSwapchain();
    void CreateSwapchainImageViews();
    void CreateRenderPass();
    void CreateDepthResources();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateSyncObjects();
    void CreateDescriptorPool();
    void AllocateCommandbuffers();
    VkPresentModeKHR ChoosePresentMode();

public:
    void DrawFrame();
    void CreateUniformBuffer(VkDeviceSize bufferSize);

protected:
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    std::vector<const char*> GetRequiredExtensions();
    std::vector<const char*> GetRequiredLayers();
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, uint32_t mipLevels,
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D);
    VkSurfaceFormatKHR ChooseSurfaceFormat();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer,
        VmaAllocation& allocation);
    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevelCount, VkSampleCountFlagBits sampleCount,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage& image,
        VmaAllocation& allocation, uint32_t layers = 1, VkImageCreateFlags flags = VK_NULL_HANDLE);
    VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin);
    void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);
 public:
    VkShaderModule CreateShaderModule(const std::string& filename);
};
