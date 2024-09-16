#include "shader.h"

#include "vulkan/vulkan.hpp"

#include <fstream>
#include <iostream>

Shader::Shader(const char* path, const vk::Device& device)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    // std::ios::ate, Start reading at the end of the file
    // The advantage of starting to read at the end of the file is that we can use the read position to determine the size of the file and allocate a buffer:
    // https://cplusplus.com/reference/istream/istream/tellg/
    // Returns the position of the current character in the input stream.
    size_t file_size = (size_t) file.tellg();
    std::vector<char> m_buffer(file_size);

    file.seekg(0);
    file.read(m_buffer.data(), file_size);
    file.close();

    vk::ShaderModuleCreateInfo create_info{};
    create_info.codeSize = m_buffer.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(m_buffer.data());
    m_shader_module = device.createShaderModule(create_info);
} 

