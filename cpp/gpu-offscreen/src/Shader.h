#pragma once

#include "Filesystem.h"
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_reflect.hpp>
#include <vulkan/vulkan.h>

#include <filesystem>
#include <map>
#include <optional>

namespace dhh::shader
{
    enum ShaderType
    {
        Vertex,
        Geometry,
        Fragment,
        Compute,
    };

    struct DescriptorInfo
    {
        uint32_t binding;
        uint32_t count;
        uint32_t set;
        VkDescriptorType vkDescriptorType;
        VkShaderStageFlags stages;
    };

    inline VkShaderStageFlagBits getVulkanShaderType(ShaderType Type)
    {
        VkShaderStageFlagBits Table[] = {
            VK_SHADER_STAGE_VERTEX_BIT,
            VK_SHADER_STAGE_GEOMETRY_BIT,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_SHADER_STAGE_COMPUTE_BIT,
        };
        return Table[Type];
    }

    inline VkFormat getVulkanFormat(const spirv_cross::SPIRType type)
    {
        VkFormat float_types[] = {
            VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};

        VkFormat double_types[] = {
            VK_FORMAT_R64_SFLOAT,
            VK_FORMAT_R64G64_SFLOAT,
            VK_FORMAT_R64G64B64_SFLOAT,
            VK_FORMAT_R64G64B64A64_SFLOAT,
        };
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return float_types[type.vecsize - 1];
        case spirv_cross::SPIRType::Double:
            return double_types[type.vecsize - 1];
        default:
            throw std::runtime_error("Cannot find VK_Format");
        }
    }

    inline std::filesystem::path findShaderDirectory()
    {
#ifdef SHADER_DIR
        std::filesystem::path path(SHADER_DIR);
        if (!std::filesystem::is_empty(path))
        {
            return path;
        }
#endif

        std::filesystem::path current_path     = std::filesystem::current_path();
        const std::string ShadersDirectoryName = "shaders";
        while (true)
        {
            if (std::filesystem::exists(current_path / ShadersDirectoryName))
            {
                return (current_path / ShadersDirectoryName).string();
            }

            if (current_path.root_path() != current_path.parent_path())
            {
                current_path = current_path.parent_path();
            }
            else
            {
                throw std::runtime_error("Failed to find shaders directory");
            }
        }
    }


    class Shader
    {
    public:
        Shader(std::filesystem::path glslPath) : glslPath(glslPath), stageInputSize(0)
        {
            glslText = dhh::filesystem::loadFile(glslPath, false);
            type     = getShaderType(glslPath);
            spirv    = compile();
            reflect();
        }

        std::vector<VkVertexInputBindingDescription> getVertexInputBindingDescription() const
        {
            if (type != Vertex)
            {
                throw std::runtime_error("Need vertex shader");
            }
            VkVertexInputBindingDescription Description = {};
            Description.binding                         = 0;
            Description.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
            Description.stride                          = stageInputSize;

            return {Description};
        }

        std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescriptions() const
        {
            if (type != Vertex)
            {
                throw std::runtime_error("Need vertex shader");
            }
            return vertexInputAttributeDescriptions;
        }

        VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(VkShaderModule shaderModule)
        {
            VkPipelineShaderStageCreateInfo info = {};
            info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            info.module                          = shaderModule;
            info.pName                           = "main";
            info.stage                           = getVulkanShaderType(type);
            return info;
        }

        VkShaderModule createVulkanShaderModule(VkDevice device)
        {
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize                 = spirv.size() * 4;
            createInfo.pCode                    = spirv.data();

            VkShaderModule module;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create shader module!");
            }
            return module;
        }


    public:
        ShaderType type;
        std::map<uint32_t, DescriptorInfo> descriptorInfos;  // multimap<Descriptor binding, DescriptorInfo>
        std::filesystem::path glslPath;

    private:
        std::vector<char> glslText;
        std::vector<uint32_t> spirv;
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
        size_t stageInputSize;

        std::vector<uint32_t> compile(bool optimize = false)
        {
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;
            if (optimize)
            {
                options.SetOptimizationLevel(shaderc_optimization_level_performance);
            }
            shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                glslText.data(), getShadercShaderType(type), glslPath.filename().string().c_str(), options);
            if (module.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                throw std::runtime_error(module.GetErrorMessage().c_str());
            }
            return {module.cbegin(), module.cend()};
        }

        void reflect()
        {
            spirv_cross::CompilerReflection compiler(spirv);
            spirv_cross::ShaderResources shaderResources;
            shaderResources = compiler.get_shader_resources();

            for (const spirv_cross::Resource& resource : shaderResources.stage_inputs)
            {
                if (type != Vertex)
                    break;

                const spirv_cross::SPIRType& InputType = compiler.get_type(resource.type_id);

                VkVertexInputAttributeDescription Description = {};
                Description.binding  = compiler.get_decoration(resource.id, spv::DecorationBinding);
                Description.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
                Description.offset   = stageInputSize;
                Description.format   = getVulkanFormat(InputType);
                vertexInputAttributeDescriptions.push_back(Description);

                stageInputSize += InputType.width * InputType.vecsize / 8;
            }

            // Uniform buffer descriptor info
            for (const spirv_cross::Resource& resource : shaderResources.uniform_buffers)
            {
                DescriptorInfo info   = reflect_descriptor(compiler, resource);
                info.vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorInfos.insert({info.binding, info});
            }

            // Sampled image descriptor info
            for (const spirv_cross::Resource& resource : shaderResources.sampled_images)
            {
                DescriptorInfo info   = reflect_descriptor(compiler, resource);
                info.vkDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorInfos.insert({info.binding, info});
            }

            // storage buffer descriptor info
            for (const spirv_cross::Resource& resource : shaderResources.storage_buffers)
            {
                DescriptorInfo info   = reflect_descriptor(compiler, resource);
                info.vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptorInfos.insert({info.binding, info});
            }
        }

        DescriptorInfo reflect_descriptor(
            const spirv_cross::CompilerReflection& compiler, const spirv_cross::Resource& resource)
        {
            const spirv_cross::SPIRType& descriptorType = compiler.get_type(resource.type_id);
            uint32_t set        = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            DescriptorInfo info = {};
            info.binding        = compiler.get_decoration(resource.id, spv::DecorationBinding);
            info.count          = 1;  // TODO: need a fix for array
            info.set            = set;
            info.stages         = getVulkanShaderType(type);
            return info;
        }

        static ShaderType getShaderType(const std::filesystem::path& FilePath)
        {
            std::string extension                   = FilePath.filename().extension().string();
            std::map<std::string, ShaderType> table = {
                {".vert", Vertex},
                {".frag", Fragment},
                {".geom", Geometry},
                {".comp", Compute},
            };
            if (table.count(extension) == 0)
            {
                throw std::runtime_error("Cannot determine shader type");
            }
            return table[extension];
        }

        static shaderc_shader_kind getShadercShaderType(ShaderType Type)
        {
            shaderc_shader_kind Table[] = {
                shaderc_glsl_vertex_shader,
                shaderc_glsl_geometry_shader,
                shaderc_glsl_fragment_shader,
                shaderc_glsl_compute_shader,
            };
            return Table[Type];
        }
    };
}
