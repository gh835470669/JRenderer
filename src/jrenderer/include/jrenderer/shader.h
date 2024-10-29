#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan.hpp"
#include <gsl/pointers>

namespace jre
{
    class ShaderLoader
    {
    public:
        static std::vector<char> read_file(const std::string &filename);
    };

    class LogicalDevice;
    class Shader
    {
    protected:
        gsl::not_null<const LogicalDevice *> m_device;
        vk::ShaderModule m_shader_module;
        const std::string m_entry;

    public:
        Shader(gsl::not_null<const LogicalDevice *> device, const char *path, const std::string &entry = "main", const ShaderLoader &loader = ShaderLoader());
        Shader(gsl::not_null<const LogicalDevice *> device, const std::vector<char> &buffer, const std::string &entry = "main");
        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;
        Shader(Shader &&) = default;
        Shader &operator=(Shader &&) = default;
        ~Shader();

        const vk::ShaderModule &shader_module() const { return m_shader_module; }
        operator vk::ShaderModule() const { return m_shader_module; }

        virtual vk::PipelineShaderStageCreateInfo shader_stage_info() const = 0;
    };

    class VertexShader : public Shader
    {
    public:
        VertexShader(gsl::not_null<const LogicalDevice *> device, const char *path, const std::string &entry = "main", const ShaderLoader &loader = ShaderLoader())
            : Shader(device, path, entry, loader) {}
        VertexShader(gsl::not_null<const LogicalDevice *> device, const std::vector<char> &buffer, const std::string &entry = "main")
            : Shader(device, buffer, entry) {}
        vk::PipelineShaderStageCreateInfo shader_stage_info() const override;
    };

    class FragmentShader : public Shader
    {
    public:
        FragmentShader(gsl::not_null<const LogicalDevice *> device, const char *path, const std::string &entry = "main", const ShaderLoader &loader = ShaderLoader())
            : Shader(device, path, entry, loader) {}
        FragmentShader(gsl::not_null<const LogicalDevice *> device, const std::vector<char> &buffer, const std::string &entry = "main")
            : Shader(device, buffer, entry) {}
        vk::PipelineShaderStageCreateInfo shader_stage_info() const override;
    };

    class ShaderProgram
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        vk::PipelineShaderStageCreateInfo m_vertex_shader;
        vk::PipelineShaderStageCreateInfo m_fragment_shader;

    public:
        ShaderProgram(const char *vertex_shader_path, const char *fragment_shader_path, gsl::not_null<const LogicalDevice *> device);
        ShaderProgram(const Shader &vertex_shader, const Shader &fragment_shader, gsl::not_null<const LogicalDevice *> device);
        ShaderProgram(const ShaderProgram &) = delete;
        ShaderProgram &operator=(const ShaderProgram &) = delete;
        ShaderProgram(ShaderProgram &&) = default;
        ShaderProgram &operator=(ShaderProgram &&) = default;
        ~ShaderProgram() = default;
    };
}
