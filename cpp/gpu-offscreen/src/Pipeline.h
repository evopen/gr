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
            : device(device), shaders({shader}), descriptorPool(pool)
        {
            isComputePipeline = true;
            createShaderModules();
            createShaderStageCreateInfos();
            gatherDescriptorInfo();
            createDescriptorSetLayouts();
            createPipelineLayout();
            createComputePipeline();
            allocateDescriptorSets();
        }


        explicit Pipeline(VkDevice device, const std::vector<Shader*>& shaders, VkDescriptorPool pool,
            VkRenderPass renderPass, VkPipelineMultisampleStateCreateInfo multisampleState,
            std::vector<VkDynamicState> dynamicStates, VkPipelineRasterizationStateCreateInfo rasterizationState,
            VkPipelineDepthStencilStateCreateInfo depthStencilState, VkPipelineViewportStateCreateInfo viewportState,
            VkPipelineColorBlendAttachmentState colorBlendAttachmentState,
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState)
            : device(device), shaders(shaders), descriptorPool(pool), renderPass(renderPass),
              multisampleState(multisampleState), dynamicStates(dynamicStates), rasterizationState(rasterizationState),
              depthStencilState(depthStencilState), viewportState(viewportState),
              colorBlendAttachmentState(colorBlendAttachmentState), inputAssemblyState(inputAssemblyState)

        {
            isComputePipeline = false;
            validateGraphicsPipelineShaders();
            getVertexInputInfos();
            createShaderModules();
            createShaderStageCreateInfos();
            gatherDescriptorInfo();
            createDescriptorSetLayouts();
            createPipelineLayout();
            createGraphicsPipeline();
            allocateDescriptorSets();
        }

        VkPipelineMultisampleStateCreateInfo multisampleState;
        std::vector<VkDynamicState> dynamicStates;
        VkPipelineRasterizationStateCreateInfo rasterizationState;
        VkPipelineDepthStencilStateCreateInfo depthStencilState;
        VkPipelineViewportStateCreateInfo viewportState;
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;

        void createComputePipeline()
        {
            VkComputePipelineCreateInfo pipelineInfo =
                dhh::vk::initializer::computePipelineCreateInfo(shaderStageCreateInfos[0], pipelineLayout);
            vkCreateComputePipelines(device, 0, 1, &pipelineInfo, nullptr, &pipeline);
        }

        void createGraphicsPipeline()
        {
            VkPipelineDynamicStateCreateInfo dynamicStateInfo =
                dhh::vk::initializer::pipelineDynamicStateCreateInfo(dynamicStates);


            VkPipelineColorBlendStateCreateInfo colorBlendInfo =
                dhh::vk::initializer::pipelineColorBlendStateCreateInfo(colorBlendAttachmentState);

            VkPipelineVertexInputStateCreateInfo vertexInputInfo =
                dhh::vk::initializer::pipelineVertexInputStateCreateInfo(
                    vertexInputBindingDescriptions, vertexInputAttributeDescriptions);

            VkGraphicsPipelineCreateInfo pipelineInfo =
                dhh::vk::initializer::graphicsPipelineCreateInfo(shaderStageCreateInfos, renderPass, pipelineLayout);
            pipelineInfo.pRasterizationState = &rasterizationState;
            pipelineInfo.pDynamicState       = &dynamicStateInfo;
            pipelineInfo.pMultisampleState   = &multisampleState;
            pipelineInfo.pViewportState      = &viewportState;
            pipelineInfo.pColorBlendState    = &colorBlendInfo;
            pipelineInfo.pDepthStencilState  = &depthStencilState;
            pipelineInfo.pInputAssemblyState = &inputAssemblyState;
            pipelineInfo.pVertexInputState   = &vertexInputInfo;
            vkCreateGraphicsPipelines(device, 0, 1, &pipelineInfo, NULL, &pipeline);
        }

        void getVertexInputInfos()
        {
            for (const auto& shader : shaders)
            {
                if (shader->type == Vertex)
                {
                    vertexInputBindingDescriptions   = shader->getVertexInputBindingDescription();
                    vertexInputAttributeDescriptions = shader->getVertexInputAttributeDescriptions();
                }
            }
        }

        void createShaderStageCreateInfos()
        {
            for (const auto& shader : shaders)
            {
                shaderStageCreateInfos.push_back(shader->getPipelineShaderStageCreateInfo(shaderModules[shader->type]));
            }
        }

        void createShaderModules()
        {
            for (const auto& shader : shaders)
            {
                shaderModules[shader->type] = shader->createVulkanShaderModule(device);
            }
        }

        void createPipelineLayout()
        {
            VkPipelineLayoutCreateInfo info = {};
            info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            info.setLayoutCount             = descriptorSetLayouts.size();
            info.pSetLayouts                = descriptorSetLayouts.data();

            vkCreatePipelineLayout(device, &info, nullptr, &pipelineLayout);
        }

        // descriptor sets index by swapchain id
        void allocateDescriptorSets()
        {
            descriptorSets.resize(descriptorSetLayouts.size());
            VkDescriptorSetAllocateInfo allocateInfo = {};
            allocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocateInfo.descriptorPool              = descriptorPool;
            allocateInfo.descriptorSetCount          = descriptorSetLayouts.size();
            allocateInfo.pSetLayouts                 = descriptorSetLayouts.data();
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()));
        }

        void createDescriptorSetLayouts()
        {
            // iterate descriptor group in one set, create decriptor set layout
            for (const auto& bindingsInSet : bindings)
            {
                VkDescriptorSetLayoutCreateInfo info = {};
                info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                info.bindingCount                    = bindingsInSet.second.size();
                info.pBindings                       = bindingsInSet.second.data();
                VkDescriptorSetLayout setLayout;

                vkCreateDescriptorSetLayout(device, &info, nullptr, &setLayout);

                descriptorSetLayouts.push_back(setLayout);
            }
        }

    public:
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkDescriptorSet> descriptorSets;
        VkPipelineLayout pipelineLayout;
        std::map<ShaderType, VkShaderModule> shaderModules;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
        std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
        VkPipeline pipeline;
        bool isComputePipeline;


    private:
        std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings;  // map<set id, bindings group>
        VkDevice device;
        VkDescriptorPool descriptorPool;
        VkRenderPass renderPass;

        /// A pipeline can only have at most one vertex shader or fragment shader, etc.
        void validateGraphicsPipelineShaders()
        {
            uint32_t typeCounts[sizeof(ShaderType)] = {0};
            for (const auto& shader : shaders)
            {
                typeCounts[shader->type]++;
            }
            for (auto count : typeCounts)
            {
                if (count < 0 || count > 1)
                {
                    throw std::runtime_error("Invalid Pipeline");
                }
            }
        }

        void gatherDescriptorInfo()
        {
            // merge binding infos from different shaders
            std::map<uint32_t, DescriptorInfo> infoMerged;  // map<binding, info>
            for (const auto& shader : shaders)
            {
                for (const auto& info : shader->descriptorInfos)
                {
                    if (infoMerged.find(info.first) != infoMerged.end())
                    {
                        infoMerged[info.first].stages = (infoMerged[info.first].stages | info.second.stages);
                    }
                    else
                    {
                        infoMerged.insert({info.second.binding, info.second});
                    }
                }
            }

            for (const auto& info : infoMerged)
            {
                VkDescriptorSetLayoutBinding binding = {};
                binding.binding                      = info.second.binding;
                binding.descriptorCount              = info.second.count;
                binding.descriptorType               = info.second.vkDescriptorType;
                binding.stageFlags                   = info.second.stages;
                bindings[info.second.set].push_back(binding);
            }
        }
    };
}
