#include "Filesystem.h"
#include "Shader.h"
#include "VulkanBase.h"
#include "VulkanInitializer.h"
#include "pch.h"

#include <VulkanTexture.hpp>
#include <VulkanTools.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <ktx.h>
#include <ktxvulkan.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <chrono>

const int32_t kWidth  = 128;
const int32_t kHeight = 128;

class RayTracer : public VulkanBase
{
public:
    explicit RayTracer(bool enableValidation) : VulkanBase(enableValidation) {}

    VkPipeline compute_pipeline;
    VkPipelineLayout compute_pipeline_layout;
    VkDescriptorSetLayout compute_descriptor_set_layout;
    VkDescriptorSet compute_descritor_set;
    vks::Texture cube_map;

    VkImageView skybox_image_view;
    VmaAllocation skybox_allocation;
    VkImage result_image;
    VkImageView result_image_view;
    VmaAllocation result_image_allocation;

    void CreateSkyboxSampler()
    {
        VkSamplerCreateInfo sampler_info = {};
        sampler_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter           = VK_FILTER_LINEAR;
        sampler_info.minFilter           = VK_FILTER_LINEAR;
        sampler_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV        = sampler_info.addressModeU;
        sampler_info.addressModeW        = sampler_info.addressModeU;
        sampler_info.mipLodBias          = 0.0f;
        sampler_info.compareOp           = VK_COMPARE_OP_NEVER;
        sampler_info.minLod              = 0.0f;
        sampler_info.maxLod              = 1.f;
        sampler_info.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_info.maxAnisotropy       = 1.0f;
        vkCreateSampler(device, &sampler_info, nullptr, &cube_map.sampler);
    }

    void CreateSkybox()
    {
        skybox_image_view = CreateImageView(cube_map.image, VK_FORMAT_BC3_UNORM_BLOCK, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_CUBE);
    }

    void CreateResultImage()
    {
        CreateImage(kWidth, kHeight, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_GPU_ONLY, result_image,
            result_image_allocation, 1);

        result_image_view = CreateImageView(result_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }


    void CreateComputePipeline()
    {
        VkDescriptorSetLayoutBinding skybox_binding = {};
        skybox_binding.binding                      = 0;
        skybox_binding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skybox_binding.descriptorCount              = 1;
        skybox_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding object_info_binding = {};
        object_info_binding.binding                      = 1;
        object_info_binding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        object_info_binding.descriptorCount              = 1;
        object_info_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding tex_coord_binding = {};
        tex_coord_binding.binding                      = 2;
        tex_coord_binding.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        tex_coord_binding.descriptorCount              = 1;
        tex_coord_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding result_image_binding = {};
        result_image_binding.binding                      = 3;
        result_image_binding.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        result_image_binding.descriptorCount              = 1;
        result_image_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            skybox_binding, object_info_binding, tex_coord_binding, result_image_binding};


        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
        descriptor_set_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_info.bindingCount = bindings.size();
        descriptor_set_layout_info.pBindings    = bindings.data();
        vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &compute_descriptor_set_layout);

        auto pipeline_layout_info = dhh::initializer::PipelineLayoutCreateInfo(compute_descriptor_set_layout);
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &compute_pipeline_layout));

        auto shader_path                     = dhh::shader::FindShaderDirectory();
        dhh::shader::Shader compute_shader   = (shader_path / "trace.comp");
        VkShaderModule compute_shader_module = compute_shader.CreateVulkanShaderModule(device);

        VkSpecializationMapEntry const_height = {};
        const_height.constantID               = 0;
        const_height.offset                   = 0;
        const_height.size                     = sizeof(int32_t);

        VkSpecializationMapEntry const_width = {};
        const_width.constantID               = 1;
        const_width.offset                   = 0;
        const_width.size                     = sizeof(int32_t);

        std::vector<VkSpecializationMapEntry> entries = {const_height, const_width};

        VkSpecializationInfo constant_info = {};
        constant_info.pMapEntries          = entries.data();
        constant_info.mapEntryCount        = entries.size();
        constant_info.dataSize             = sizeof(int32_t);
        constant_info.pData                = &kWidth;

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_info = {};

        pipeline_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_info.module              = compute_shader_module;
        pipeline_shader_stage_info.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
        pipeline_shader_stage_info.pName               = "main";
        pipeline_shader_stage_info.pSpecializationInfo = &constant_info;

        VkComputePipelineCreateInfo compute_pipeline_info = {};

        compute_pipeline_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_pipeline_info.stage  = pipeline_shader_stage_info;
        compute_pipeline_info.layout = compute_pipeline_layout;

        VK_CHECK_RESULT(
            vkCreateComputePipelines(device, nullptr, 1, &compute_pipeline_info, nullptr, &compute_pipeline));
    }

    VkCommandBuffer compute_cmd_buf;

    void writeComputeDescriptorSet()
    {
        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.pSetLayouts                 = &compute_descriptor_set_layout;
        allocate_info.descriptorSetCount          = 1;
        allocate_info.descriptorPool              = descriptor_pool;
        vkAllocateDescriptorSets(device, &allocate_info, &compute_descritor_set);

        VkDescriptorImageInfo skybox_image_info = dhh::initializer::DescriptorImageInfo(
            skybox_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cube_map.sampler);

        VkDescriptorImageInfo result_image_info =
            dhh::initializer::DescriptorImageInfo(result_image_view, VK_IMAGE_LAYOUT_GENERAL, cube_map.sampler);

        VkWriteDescriptorSet skybox_write = {};
        skybox_write.descriptorCount      = 1;
        skybox_write.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skybox_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skybox_write.pImageInfo           = &skybox_image_info;
        skybox_write.dstSet               = compute_descritor_set;

        auto result_image_write = dhh::initializer::WriteDescriptorSet(
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, 3, compute_descritor_set, &result_image_info);

        std::vector<VkWriteDescriptorSet> writes = {skybox_write, result_image_write};

        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
    }

    void BuildComputeCommandBuffers()
    {
        VkCommandBufferAllocateInfo info =
            dhh::initializer::CommandBufferAllocateInfo(command_pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        vkAllocateCommandBuffers(device, &info, &compute_cmd_buf);
        VkCommandBufferBeginInfo beginInfo = dhh::initializer::CommandBufferBeginInfo();
        vkBeginCommandBuffer(compute_cmd_buf, &beginInfo);


        vkCmdBindPipeline(compute_cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
        vkCmdBindDescriptorSets(compute_cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 0, 1,
            &compute_descritor_set, 0, nullptr);

        vks::tools::setImageLayout(compute_cmd_buf, result_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL);
        vkCmdDispatch(compute_cmd_buf, 256, 256, 1);

        vkEndCommandBuffer(compute_cmd_buf);
    }

    void Compute()
    {
        VkFence completeFence;

        VkFenceCreateInfo fenceCreateInfo = dhh::initializer::FenceCreateInfo();
        vkCreateFence(device, &fenceCreateInfo, nullptr, &completeFence);

        VkSubmitInfo submitInfo       = {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &compute_cmd_buf;

        VK_CHECK_RESULT(vkQueueSubmit(graphics_queue, 1, &submitInfo, completeFence));
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &completeFence, true, DEFAULT_FENCE_TIMEOUT));
        vkDestroyFence(device, completeFence, nullptr);
    }

    void SaveImage()
    {
        VkImage save_image;
        VmaAllocation save_image_allocation;
        CreateImage(kHeight, kHeight, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY, save_image, save_image_allocation);
        if (!save_image)
            throw std::runtime_error("failed to create image");

        VkCommandBuffer copy_cmd_buf;
        VkCommandBufferAllocateInfo info =
            dhh::initializer::CommandBufferAllocateInfo(command_pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        vkAllocateCommandBuffers(device, &info, &copy_cmd_buf);
        VkCommandBufferBeginInfo beginInfo = dhh::initializer::CommandBufferBeginInfo();
        vkBeginCommandBuffer(copy_cmd_buf, &beginInfo);

        vks::tools::setImageLayout(copy_cmd_buf, save_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vks::tools::setImageLayout(copy_cmd_buf, result_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);


        // Otherwise use image copy (requires us to manually flip components)
        VkImageCopy imageCopyRegion{};
        imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageCopyRegion.srcSubresource.layerCount = 1;
        imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageCopyRegion.dstSubresource.layerCount = 1;
        imageCopyRegion.extent.width              = kWidth;
        imageCopyRegion.extent.height             = kHeight;
        imageCopyRegion.extent.depth              = 1;

        // Issue the copy command
        vkCmdCopyImage(copy_cmd_buf, result_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, save_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

        vks::tools::setImageLayout(copy_cmd_buf, save_image, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

        VK_CHECK_RESULT(vkEndCommandBuffer(copy_cmd_buf));

        VkSubmitInfo submitInfo       = vks::initializers::submitInfo();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &copy_cmd_buf;
        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
        VkFence fence;
        VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
        // Submit to the queue
        VK_CHECK_RESULT(vkQueueSubmit(graphics_queue, 1, &submitInfo, fence));
        // Wait for the fence to signal that command buffer has finished executing
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
        vkDestroyFence(device, fence, nullptr);
        if (free)
        {
            vkFreeCommandBuffers(device, command_pool, 1, &copy_cmd_buf);
        }

        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(device, save_image, &subResource, &subResourceLayout);

        // Map image memory so we can start copying from it
        const uint8_t* data;
        vmaMapMemory(allocator, save_image_allocation, (void**) &data);
        data += subResourceLayout.offset;

        uint8_t* img = new uint8_t[kHeight * kWidth * 3];

        for (int row = 0; row < kHeight; ++row)
        {
            for (int col = 0; col < kWidth; ++col)
            {
                img[row * kWidth * 3 + col * 3 + 0] = data[col * 4 + 0];
                img[row * kWidth * 3 + col * 3 + 1] = data[col * 4 + 1];
                img[row * kWidth * 3 + col * 3 + 2] = data[col * 4 + 2];
            }
            data += subResourceLayout.rowPitch;
        }

        stbi_write_png("raytraced.png", kHeight, kWidth, STBI_rgb, img, kWidth * 3);

        std::cout << "saved to disk" << std::endl;

        // Clean up resources
        vmaUnmapMemory(allocator, save_image_allocation);
        vmaFreeMemory(allocator, save_image_allocation);
        vkDestroyImage(device, save_image, nullptr);
    }


    void LoadCubemap()
    {
        std::filesystem::path filename = "resource/cubemap_yokohama_bc3_unorm.ktx";
        ktxResult result;
        ktxTexture* ktxTexture;
        if (!std::filesystem::exists(filename))
        {
            throw std::runtime_error("cannot find texture");
        }

        result = ktxTexture_CreateFromNamedFile(
            std::filesystem::absolute(filename).string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

        assert(result == KTX_SUCCESS);

        // Get properties required for using and upload texture data from the ktx texture object
        cube_map.width               = ktxTexture->baseWidth;
        cube_map.height              = ktxTexture->baseHeight;
        cube_map.mipLevels           = ktxTexture->numLevels;
        ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
        ktx_size_t ktxTextureSize   = ktxTexture_GetSize(ktxTexture);

        VkBuffer staging_buffer;
        VmaAllocation staging_buffer_allocation;
        CreateBuffer(ktxTextureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, staging_buffer,
            staging_buffer_allocation);

        uint8_t* data;
        vmaMapMemory(allocator, staging_buffer_allocation, (void**) &data);
        memcpy(data, ktxTextureData, ktxTextureSize);
        CreateImage(cube_map.width, cube_map.height, cube_map.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_BC3_UNORM_BLOCK,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, cube_map.image, cube_map.allocation, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

        VkCommandBuffer cmd_buf = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        // Setup buffer copy regions for each face including all of its miplevels
        std::vector<VkBufferImageCopy> bufferCopyRegions;
        uint32_t offset = 0;

        for (uint32_t face = 0; face < 6; face++)
        {
            for (uint32_t level = 0; level < cube_map.mipLevels; level++)
            {
                // Calculate offset into staging buffer for the current mip level and face
                ktx_size_t offset;
                KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
                assert(ret == KTX_SUCCESS);
                VkBufferImageCopy bufferCopyRegion               = {};
                bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel       = level;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount     = 1;
                bufferCopyRegion.imageExtent.width               = ktxTexture->baseWidth >> level;
                bufferCopyRegion.imageExtent.height              = ktxTexture->baseHeight >> level;
                bufferCopyRegion.imageExtent.depth               = 1;
                bufferCopyRegion.bufferOffset                    = offset;
                bufferCopyRegions.push_back(bufferCopyRegion);
            }
        }

        // Image barrier for optimal image (target)
        // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = cube_map.mipLevels;
        subresourceRange.layerCount              = 6;

        vks::tools::setImageLayout(
            cmd_buf, cube_map.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

        // Copy the cube map faces from the staging buffer to the optimal tiled image
        vkCmdCopyBufferToImage(cmd_buf, staging_buffer, cube_map.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

        // Change texture image layout to shader read after all faces have been copied
        cube_map.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vks::tools::setImageLayout(
            cmd_buf, cube_map.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cube_map.imageLayout, subresourceRange);

        FlushCommandBuffer(cmd_buf, graphics_queue, true);

        vmaUnmapMemory(allocator, staging_buffer_allocation);
        		// Clean up staging resources
        vmaFreeMemory(allocator, staging_buffer_allocation);
        vkDestroyBuffer(device, staging_buffer, nullptr);
        ktxTexture_Destroy(ktxTexture);
    }
};


int main()
{
    try
    {
        RayTracer app(true);
        app.Init();
        app.CreateComputePipeline();
        app.CreateSkyboxSampler();
        app.LoadCubemap();
        app.CreateSkybox();
        app.CreateResultImage();
        app.writeComputeDescriptorSet();
        app.BuildComputeCommandBuffers();
        auto start = std::chrono::high_resolution_clock::now();
        app.Compute();
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        app.SaveImage();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }
    return 0;
}