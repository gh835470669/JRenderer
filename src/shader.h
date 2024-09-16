#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan.hpp"

class Shader
{   
public:
    Shader(const char* path, const vk::Device& device);
    ~Shader() = default;

    vk::ShaderModule m_shader_module;
};

