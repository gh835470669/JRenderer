#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <vector>
#include <variant>
#include <ranges>
#include "jrenderer/mesh.h"
#include "jrenderer/specilization_constant.hpp"

namespace jre
{
    struct PipelineHashInfo
    {
        const std::string &vertex_shader_path;
        const std::string &fragment_shader_path;
        const SpecializationConstants &spec_const_vertex;
        const SpecializationConstants &spec_const_fragment;
    };
}

namespace std
{
    template <>
    struct hash<jre::bytes>
    {
        size_t operator()(const jre::bytes &v) const noexcept
        {
            return std::accumulate(v.begin(), v.end(), size_t{}, [](size_t acc, const std::byte &b)
                                   {
                                    glm::detail::hash_combine(acc, std::to_integer<uint32_t>(b));
                                    return acc; });
        }
    };

    template <>
    struct hash<std::map<uint32_t, jre::bytes>>
    {
        size_t operator()(const std::map<uint32_t, jre::bytes> &v) const noexcept
        {
            return std::accumulate(v.begin(), v.end(), size_t{}, [](size_t acc, const std::pair<uint32_t, jre::bytes> &p)
                                   { 
                                    glm::detail::hash_combine(acc, std::hash<uint32_t>{}(p.first));
                                    glm::detail::hash_combine(acc, std::hash<jre::bytes>{}(p.second));
                                    return acc; });
        }
    };

    template <>
    struct hash<jre::PipelineHashInfo>
    {
        size_t operator()(const jre::PipelineHashInfo &v) const noexcept
        {
            size_t hash_of_pipeline{};
            std::hash<std::string> hasher_string;
            std::hash<std::map<uint32_t, jre::bytes>> hasher_spec_constants;
            glm::detail::hash_combine(hash_of_pipeline, hasher_string(v.vertex_shader_path));
            glm::detail::hash_combine(hash_of_pipeline, hasher_string(v.fragment_shader_path));
            glm::detail::hash_combine(hash_of_pipeline, hasher_spec_constants(v.spec_const_vertex.constants()));
            glm::detail::hash_combine(hash_of_pipeline, hasher_spec_constants(v.spec_const_fragment.constants()));
            return hash_of_pipeline;
        }
    };
}

namespace jre
{

    class PipelineLayoutBuilder
    {
    public:
        vk::SharedDevice device;
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
        std::vector<vk::PushConstantRange> push_constant_ranges;

        vk::SharedPipelineLayout build()
        {
            return vk::shared::create_pipeline_layout(device, descriptor_set_layouts, push_constant_ranges);
        }
    };

    class PipelineBuilder
    {
    public:
        vk::SharedDevice device;
        vk::PipelineLayout pipeline_layout;
        vk::RenderPass render_pass;
        vk::PipelineCache cache;
        vk::GraphicsPipelineCreateInfo pipeline_info;
        std::vector<vk::PipelineShaderStageCreateInfo> stages;
        std::vector<std::string> shader_stage_entries;
        std::vector<std::pair<jre::bytes, std::vector<vk::SpecializationMapEntry>>> specialization_constants;
        std::vector<vk::DynamicState> dynamic_states;
        std::vector<vk::Viewport> viewports = {vk::Viewport(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f)};
        std::vector<vk::Rect2D> scissors = {vk::Rect2D()};
        vk::PipelineInputAssemblyStateCreateInfo input_assembly = {{}, vk::PrimitiveTopology::eTriangleList, vk::False};
        vk::PipelineRasterizationStateCreateInfo rasterizer;
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineColorBlendStateCreateInfo color_blend;
        std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
        vk::PipelineDepthStencilStateCreateInfo depth_stencil;
        std::vector<vk::VertexInputBindingDescription> vertex_binding_descriptions;
        std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions;

        struct ColorBlendAttachment
        {
            static vk::PipelineColorBlendAttachmentState alpha();
            static vk::PipelineColorBlendAttachmentState overwrite();
        };

        PipelineBuilder(vk::SharedDevice device,
                        vk::PipelineLayout pipeline_layout,
                        vk::RenderPass render_pass,
                        vk::PipelineCache cache = {})
            : device(device),
              pipeline_layout(pipeline_layout),
              render_pass(render_pass),
              cache(cache)
        {
        }

        PipelineBuilder &add_shader(
            vk::ShaderModule module,
            vk::ShaderStageFlagBits stage,
            const SpecializationConstants &specialization = {},
            const std::string &entry = "main")
        {
            specialization_constants.push_back(
                std::move(convert_to<std::pair<jre::bytes, std::vector<vk::SpecializationMapEntry>>>(specialization)));
            stages.emplace_back(
                vk::PipelineShaderStageCreateFlags{},
                stage,
                module);
            shader_stage_entries.emplace_back(entry);
            return *this;
        }

        PipelineBuilder &clear_shaders()
        {
            stages.clear();
            specialization_constants.clear();
            shader_stage_entries.clear();
            return *this;
        }

        PipelineBuilder &add_vertex_shader(
            vk::ShaderModule module,
            const SpecializationConstants &specialization = {},
            const std::string &entry = "main")
        {
            return add_shader(module, vk::ShaderStageFlagBits::eVertex, specialization, entry);
        }

        PipelineBuilder &add_fragment_shader(
            vk::ShaderModule module,
            const SpecializationConstants &specialization = {},
            const std::string &entry = "main")
        {
            return add_shader(module, vk::ShaderStageFlagBits::eFragment, specialization, entry);
        }

        PipelineBuilder &add_dynamic_state(vk::DynamicState state)
        {
            dynamic_states.push_back(state);
            return *this;
        }

        PipelineBuilder &add_viewport(vk::Viewport viewport)
        {
            viewports.push_back(viewport);
            return *this;
        }

        PipelineBuilder &add_scissor(vk::Rect2D scissor)
        {
            scissors.push_back(scissor);
            return *this;
        }

        PipelineBuilder &set_rasterizer(vk::PipelineRasterizationStateCreateInfo info)
        {
            rasterizer = info;
            return *this;
        }

        PipelineBuilder &set_rasterizer(vk::PolygonMode polygon_mode = vk::PolygonMode::eFill,
                                        float line_width = 1.0f,
                                        vk::CullModeFlagBits cull_mode = vk::CullModeFlagBits::eBack,
                                        vk::FrontFace front_face = vk::FrontFace::eCounterClockwise)
        {
            rasterizer.setCullMode(cull_mode)
                .setFrontFace(front_face)
                .setLineWidth(line_width)
                .setPolygonMode(polygon_mode);
            return *this;
        }

        PipelineBuilder &set_multisampling(vk::PipelineMultisampleStateCreateInfo info)
        {
            multisampling = info;
            return *this;
        }

        PipelineBuilder &set_multisampling(vk::SampleCountFlagBits samples)
        {
            multisampling.setRasterizationSamples(samples);
            return *this;
        }
        PipelineBuilder &set_color_blend(vk::PipelineColorBlendStateCreateInfo info)
        {
            color_blend = info;
            return *this;
        }

        PipelineBuilder &add_color_blend_attachment(vk::PipelineColorBlendAttachmentState state)
        {
            color_blend_attachments.push_back(state);
            return *this;
        }

        PipelineBuilder &set_depth_stencil(vk::PipelineDepthStencilStateCreateInfo info)
        {
            depth_stencil = info;
            return *this;
        }

        PipelineBuilder &enable_depth(bool enable = true)
        {
            depth_stencil
                .setDepthCompareOp(enable ? vk::CompareOp::eLess : vk::CompareOp::eNever)
                .setDepthTestEnable(enable)
                .setDepthWriteEnable(enable);
            return *this;
        }

        PipelineBuilder &add_vertex_input_binding(vk::VertexInputBindingDescription binding)
        {
            vertex_binding_descriptions.push_back(binding);
            return *this;
        }

        PipelineBuilder &add_vertex_input_attribute(vk::VertexInputAttributeDescription attrib)
        {
            vertex_attribute_descriptions.push_back(attrib);
            return *this;
        }

        PipelineBuilder &add_vertex_input_attributes(vk::ArrayProxy<vk::VertexInputAttributeDescription> attribs)
        {
            vertex_attribute_descriptions.append_range(attribs);
            return *this;
        }

        vk::SharedPipeline build();
    };

    class RenderPipeline
    {
    public:
        vk::SharedPipeline pipeline;
        vk::SharedPipelineLayout pipeline_layout;
        vk::SharedShaderModule vertex_shader;
        vk::SharedShaderModule fragment_shader;
        PipelineBuilder pipeline_builder;
        void recreate_pipeline()
        {
            pipeline = pipeline_builder.build();
        }
    };

    using SharedRenderPipeline = std::shared_ptr<RenderPipeline>;

    struct PipelineCreator
    {
        PipelineBuilder pipeline_builder;
        vk::SharedPipeline operator()()
        {
            return pipeline_builder.build();
        }
    };

    class RenderPipelineResources
    {
    public:
        std::hash<PipelineHashInfo> hasher;
        std::unordered_map<size_t, SharedRenderPipeline> pipelines;
    };
}