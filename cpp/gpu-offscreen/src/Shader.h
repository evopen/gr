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
        kVertex,
        kGeometry,
        kFragment,
        kCompute,
    };

    struct DescriptorInfo
    {
        uint32_t binding;
        uint32_t count;
        uint32_t set;
        VkDescriptorType vk_descriptor_type;
        VkShaderStageFlags stages;
    };

    inline VkShaderStageFlagBits GetVulkanShaderType(ShaderType Type)
    {
        VkShaderStageFlagBits table[] = {
            VK_SHADER_STAGE_VERTEX_BIT,
            VK_SHADER_STAGE_GEOMETRY_BIT,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_SHADER_STAGE_COMPUTE_BIT,
        };
        return table[Type];
    }

    inline VkFormat GetVulkanFormat(const spirv_cross::SPIRType kType)
    {
        VkFormat float_types[] = {
            VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};

        VkFormat double_types[] = {
            VK_FORMAT_R64_SFLOAT,
            VK_FORMAT_R64G64_SFLOAT,
            VK_FORMAT_R64G64B64_SFLOAT,
            VK_FORMAT_R64G64B64A64_SFLOAT,
        };
        switch (kType.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return float_types[kType.vecsize - 1];
        case spirv_cross::SPIRType::Double:
            return double_types[kType.vecsize - 1];
        default:
            throw std::runtime_error("Cannot find VK_Format");
        }
    }

    inline std::filesystem::path FindShaderDirectory()
    {
#ifdef SHADER_DIR
        std::filesystem::path path(SHADER_DIR);
        if (!std::filesystem::is_empty(path))
        {
            return path;
        }
#endif

        std::filesystem::path current_path      = std::filesystem::current_path();
        const std::string kShadersDirectoryName = "shaders";
        while (true)
        {
            if (std::filesystem::exists(current_path / "src" / kShadersDirectoryName))
            {
                return (current_path / "src" / kShadersDirectoryName).string();
            }

            if (std::filesystem::exists(current_path / kShadersDirectoryName))
            {
                return (current_path / kShadersDirectoryName).string();
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
        Shader(std::filesystem::path glslPath) : glsl_path(glslPath), stageInputSize_(0)
        {
            glslText_ = dhh::filesystem::LoadFile(glslPath, false);
            type      = GetShaderType(glslPath);
            spirv_    = Compile();
            Reflect();
        }

        std::vector<VkVertexInputBindingDescription> GetVertexInputBindingDescription() const
        {
            if (type != kVertex)
            {
                throw std::runtime_error("Need vertex shader");
            }
            VkVertexInputBindingDescription description = {};
            description.binding                         = 0;
            description.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
            description.stride                          = stageInputSize_;

            return {description};
        }

        std::vector<VkVertexInputAttributeDescription> GetVertexInputAttributeDescriptions() const
        {
            if (type != kVertex)
            {
                throw std::runtime_error("Need vertex shader");
            }
            return vertexInputAttributeDescriptions_;
        }

        VkPipelineShaderStageCreateInfo GetPipelineShaderStageCreateInfo(VkShaderModule shaderModule)
        {
            VkPipelineShaderStageCreateInfo info = {};
            info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            info.module                          = shaderModule;
            info.pName                           = "main";
            info.stage                           = GetVulkanShaderType(type);
            return info;
        }

        VkShaderModule CreateVulkanShaderModule(VkDevice device)
        {
            VkShaderModuleCreateInfo create_info = {};
            create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize                 = spirv_.size() * 4;
            create_info.pCode                    = spirv_.data();

            VkShaderModule module;
            if (vkCreateShaderModule(device, &create_info, nullptr, &module) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create shader module!");
            }
            return module;
        }


    public:
        ShaderType type;
        std::map<uint32_t, DescriptorInfo> descriptor_infos;  // multimap<Descriptor binding, DescriptorInfo>
        std::filesystem::path glsl_path;

    private:
        std::vector<char> glslText_;
        std::vector<uint32_t> spirv_;
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions_;
        size_t stageInputSize_;

        std::vector<uint32_t> Compile(bool optimize = true)
        {
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;
            if (optimize)
            {
                options.SetOptimizationLevel(shaderc_optimization_level_performance);
            }
            shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                glslText_.data(), GetShadercShaderType(type), glsl_path.filename().string().c_str(), options);
            if (module.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                throw std::runtime_error(module.GetErrorMessage().c_str());
            }
            return {module.cbegin(), module.cend()};
        }

        void Reflect()
        {
            spirv_cross::CompilerReflection compiler(spirv_);
            spirv_cross::ShaderResources shader_resources;
            shader_resources = compiler.get_shader_resources();

            for (const spirv_cross::Resource& resource : shader_resources.stage_inputs)
            {
                if (type != kVertex)
                    break;

                const spirv_cross::SPIRType& input_type = compiler.get_type(resource.type_id);

                VkVertexInputAttributeDescription description = {};
                description.binding  = compiler.get_decoration(resource.id, spv::DecorationBinding);
                description.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
                description.offset   = stageInputSize_;
                description.format   = GetVulkanFormat(input_type);
                vertexInputAttributeDescriptions_.push_back(description);

                stageInputSize_ += input_type.width * input_type.vecsize / 8;
            }

            // Uniform buffer descriptor info
            for (const spirv_cross::Resource& resource : shader_resources.uniform_buffers)
            {
                DescriptorInfo info     = ReflectDescriptor(compiler, resource);
                info.vk_descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_infos.insert({info.binding, info});
            }

            // Sampled image descriptor info
            for (const spirv_cross::Resource& resource : shader_resources.sampled_images)
            {
                DescriptorInfo info     = ReflectDescriptor(compiler, resource);
                info.vk_descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_infos.insert({info.binding, info});
            }

            // storage buffer descriptor info
            for (const spirv_cross::Resource& resource : shader_resources.storage_buffers)
            {
                DescriptorInfo info     = ReflectDescriptor(compiler, resource);
                info.vk_descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptor_infos.insert({info.binding, info});
            }
        }

        DescriptorInfo ReflectDescriptor(
            const spirv_cross::CompilerReflection& compiler, const spirv_cross::Resource& resource)
        {
            const spirv_cross::SPIRType& descriptor_type = compiler.get_type(resource.type_id);
            uint32_t set        = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            DescriptorInfo info = {};
            info.binding        = compiler.get_decoration(resource.id, spv::DecorationBinding);
            info.count          = 1;  // TODO: need a fix for array
            info.set            = set;
            info.stages         = GetVulkanShaderType(type);
            return info;
        }

        static ShaderType GetShaderType(const std::filesystem::path& FilePath)
        {
            std::string extension                   = FilePath.filename().extension().string();
            std::map<std::string, ShaderType> table = {
                {".vert", kVertex},
                {".frag", kFragment},
                {".geom", kGeometry},
                {".comp", kCompute},
            };
            if (table.count(extension) == 0)
            {
                throw std::runtime_error("Cannot determine shader type");
            }
            return table[extension];
        }

        static shaderc_shader_kind GetShadercShaderType(ShaderType Type)
        {
            shaderc_shader_kind table[] = {
                shaderc_glsl_vertex_shader,
                shaderc_glsl_geometry_shader,
                shaderc_glsl_fragment_shader,
                shaderc_glsl_compute_shader,
            };
            return table[Type];
        }
    };
}
