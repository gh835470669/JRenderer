#include "jrenderer/shader.h"
#include "jrenderer/logical_device.h"

#include "vulkan/vulkan.hpp"

#include <fstream>
#include <iostream>

namespace jre
{
    std::vector<char> ShaderLoader::read_file(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        // std::ios::ate, Start reading at the end of the file
        // The advantage of starting to read at the end of the file is that we can use the read position to determine the size of the file and allocate a buffer:
        // https://cplusplus.com/reference/istream/istream/tellg/
        // Returns the position of the current character in the input stream.
        size_t file_size = (size_t)file.tellg();
        std::vector<char> m_buffer(file_size);

        file.seekg(0);
        file.read(m_buffer.data(), file_size);
        file.close();

        return m_buffer;
    }

    Shader::Shader(gsl::not_null<const LogicalDevice *> device, const char *path, const std::string &entry, const ShaderLoader &loader)
        : Shader(device, loader.read_file(path), entry) {}

    Shader::Shader(gsl::not_null<const LogicalDevice *> device, const std::vector<char> &buffer, const std::string &entry) : m_device(device), m_entry(entry)
    {
        vk::ShaderModuleCreateInfo create_info{};
        create_info.codeSize = buffer.size();
        create_info.pCode = reinterpret_cast<const uint32_t *>(buffer.data());
        m_shader_module = m_device->device().createShaderModule(create_info);
    }

    Shader::~Shader()
    {
        m_device->device().destroyShaderModule(m_shader_module);
    }

    vk::PipelineShaderStageCreateInfo VertexShader::shader_stage_info() const
    {
        vk::PipelineShaderStageCreateInfo shader_stage_info{};

        shader_stage_info.setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(m_shader_module)
            .setPName(m_entry.c_str());

        return shader_stage_info;
    }

    vk::PipelineShaderStageCreateInfo FragmentShader::shader_stage_info() const
    {
        vk::PipelineShaderStageCreateInfo shader_stage_info{};

        shader_stage_info.setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(m_shader_module)
            .setPName(m_entry.c_str());

        return shader_stage_info;
    }
}
