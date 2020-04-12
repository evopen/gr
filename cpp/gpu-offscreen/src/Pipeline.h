#pragma once

#include "Shader.h"
#include "VulkanInitializer.h"
#include "VulkanTools.h"

#include <vector>


namespace dhh::shader
{
    class Pipeline
    {
    public:
        std::vector<Shader*> shaders;

        explicit Pipeline(VkDevice device, Shader* shader, VkDescriptorPool pool)
            : device_(device), shaders({shader}), descriptorPool_(pool)
        {
            is_compute_pipeline = true;
            CreateShaderModules();
            CreateShaderStageCreateInfos();
            GatherDescriptorInfo();
            CreateDescriptorSetLayouts();
            CreatePipelineLayout();
            CreateComputePipeline();
            AllocateDescriptorSets();
        }


        explicit Pipeline(VkDevice device, const std::vector<Shader*>& shaders, VkDescriptorPool pool,
            VkRenderPass renderPass, VkPipelineMultisampleStateCreateInfo multisampleState,
            std::vector<VkDynamicState> dynamicStates, VkPipelineRasterizationStateCreateInfo rasterizationState,
            VkPipelineDepthStencilStateCreateInfo depthStencilState, VkPipelineViewportStateCreateInfo viewportState,
            VkPipelineColorBlendAttachmentState colorBlendAttachmentState,
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState)
            : device_(device), shaders(shaders), descriptorPool_(pool), renderPass_(renderPass),
              multisample_state(multisampleState), dynamic_states(dynamicStates), rasterization_state(rasterizationState),
              depth_stencil_state(depthStencilState), viewport_state(viewportState),
              color_blend_attachment_state(colorBlendAttachmentState), input_assembly_state(inputAssemblyState)

        {
            is_compute_pipeline = false;
            ValidateGraphicsPipelineShaders();
            GetVertexInputInfos();
            CreateShaderModules();
            CreateShaderStageCreateInfos();
            GatherDescriptorInfo();
            CreateDescriptorSetLayouts();
            CreatePipelineLayout();
            CreateGraphicsPipeline();
            AllocateDescriptorSets();
        }

        VkPipelineMultisampleStateCreateInfo multisample_state;
        std::vector<VkDynamicState> dynamic_states;
        VkPipelineRasterizationStateCreateInfo rasterization_state;
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
        VkPipelineViewportStateCreateInfo viewport_state;
        VkPipelineColorBlendAttachmentState color_blend_attachment_state;
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state;

        void CreateComputePipeline()
        {
            VkComputePipelineCreateInfo pipeline_info =
                dhh::vk::initializer::ComputePipelineCreateInfo(shader_stage_create_infos[0], pipeline_layout);
            vkCreateComputePipelines(device_, 0, 1, &pipeline_info, nullptr, &pipeline);
        }

        void CreateGraphicsPipeline()
        {
            VkPipelineDynamicStateCreateInfo dynamic_state_info =
                dhh::vk::initializer::PipelineDynamicStateCreateInfo(dynamic_states);


            VkPipelineColorBlendStateCreateInfo color_blend_info =
                dhh::vk::initializer::PipelineColorBlendStateCreateInfo(color_blend_attachment_state);

            VkPipelineVertexInputStateCreateInfo vertex_input_info =
                dhh::vk::initializer::PipelineVertexInputStateCreateInfo(
                    vertex_input_binding_descriptions, vertex_input_attribute_descriptions);

            VkGraphicsPipelineCreateInfo pipeline_info =
                dhh::vk::initializer::GraphicsPipelineCreateInfo(shader_stage_create_infos, renderPass_, pipeline_layout);
            pipeline_info.pRasterizationState = &rasterization_state;
            pipeline_info.pDynamicState       = &dynamic_state_info;
            pipeline_info.pMultisampleState   = &multisample_state;
            pipeline_info.pViewportState      = &viewport_state;
            pipeline_info.pColorBlendState    = &color_blend_info;
            pipeline_info.pDepthStencilState  = &depth_stencil_state;
            pipeline_info.pInputAssemblyState = &input_assembly_state;
            pipeline_info.pVertexInputState   = &vertex_input_info;
            vkCreateGraphicsPipelines(device_, 0, 1, &pipeline_info, NULL, &pipeline);
        }

        void GetVertexInputInfos()
        {
            for (const auto& shader : shaders)
            {
                if (shader->type == kVertex)
                {
                    vertex_input_binding_descriptions   = shader->GetVertexInputBindingDescription();
                    vertex_input_attribute_descriptions = shader->GetVertexInputAttributeDescriptions();
                }
            }
        }

        void CreateShaderStageCreateInfos()
        {
            for (const auto& shader : shaders)
            {
                shader_stage_create_infos.push_back(shader->GetPipelineShaderStageCreateInfo(shader_modules[shader->type]));
            }
        }

        void CreateShaderModules()
        {
            for (const auto& shader : shaders)
            {
                shader_modules[shader->type] = shader->CreateVulkanShaderModule(device_);
            }
        }

        void CreatePipelineLayout()
        {
            VkPipelineLayoutCreateInfo info = {};
            info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            info.setLayoutCount             = descriptor_set_layouts.size();
            info.pSetLayouts                = descriptor_set_layouts.data();

            vkCreatePipelineLayout(device_, &info, nullptr, &pipeline_layout);
        }

        // descriptor sets index by swapchain id
        void AllocateDescriptorSets()
        {
            descriptor_sets.resize(descriptor_set_layouts.size());
            VkDescriptorSetAllocateInfo allocate_info = {};
            allocate_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocate_info.descriptorPool              = descriptorPool_;
            allocate_info.descriptorSetCount          = descriptor_set_layouts.size();
            allocate_info.pSetLayouts                 = descriptor_set_layouts.data();
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device_, &allocate_info, descriptor_sets.data()));
        }

        void CreateDescriptorSetLayouts()
        {
            // iterate descriptor group in one set, create decriptor set layout
            for (const auto& bindings_in_set : bindings_)
            {
                VkDescriptorSetLayoutCreateInfo info = {};
                info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                info.bindingCount                    = bindings_in_set.second.size();
                info.pBindings                       = bindings_in_set.second.data();
                VkDescriptorSetLayout set_layout;

                vkCreateDescriptorSetLayout(device_, &info, nullptr, &set_layout);

                descriptor_set_layouts.push_back(set_layout);
            }
        }

    public:
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        std::vector<VkDescriptorSet> descriptor_sets;
        VkPipelineLayout pipeline_layout;
        std::map<ShaderType, VkShaderModule> shader_modules;
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos;
        std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions;
        std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions;
        VkPipeline pipeline;
        bool is_compute_pipeline;


    private:
        std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings_;  // map<set id, bindings group>
        VkDevice device_;
        VkDescriptorPool descriptorPool_;
        VkRenderPass renderPass_;

        /// A pipeline can only have at most one vertex shader or fragment shader, etc.
        void ValidateGraphicsPipelineShaders()
        {
            uint32_t type_counts[sizeof(ShaderType)] = {0};
            for (const auto& shader : shaders)
            {
                type_counts[shader->type]++;
            }
            for (auto count : type_counts)
            {
                if (count < 0 || count > 1)
                {
                    throw std::runtime_error("Invalid Pipeline");
                }
            }
        }

        void GatherDescriptorInfo()
        {
            // merge binding infos from different shaders
            std::map<uint32_t, DescriptorInfo> info_merged;  // map<binding, info>
            for (const auto& shader : shaders)
            {
                for (const auto& info : shader->descriptor_infos)
                {
                    if (info_merged.find(info.first) != info_merged.end())
                    {
                        info_merged[info.first].stages = (info_merged[info.first].stages | info.second.stages);
                    }
                    else
                    {
                        info_merged.insert({info.second.binding, info.second});
                    }
                }
            }

            for (const auto& info : info_merged)
            {
                VkDescriptorSetLayoutBinding binding = {};
                binding.binding                      = info.second.binding;
                binding.descriptorCount              = info.second.count;
                binding.descriptorType               = info.second.vk_descriptor_type;
                binding.stageFlags                   = info.second.stages;
                bindings_[info.second.set].push_back(binding);
            }
        }
    };
}
