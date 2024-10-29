#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <vector>
#include <variant>
#include "jrenderer/mesh.h"

namespace jre
{
    class LogicalDevice;
    class VertexShader;
    class FragmentShader;

    struct GraphicsPipelineCreateInfo
    {
        vk::RenderPass render_pass;
        std::shared_ptr<const VertexShader> vertex_shader;
        std::shared_ptr<const FragmentShader> fragment_shader;
        PipelineVertexInputState vertex_input_info;
        const std::vector<vk::DescriptorSetLayout> &descriptor_set_layouts;
        const std::vector<vk::PushConstantRange> &push_constant_ranges;
        bool enable_depth_stencil = true;
        std::variant<bool, vk::PipelineColorBlendAttachmentState> alpha_blending = true; // true: alpha blending, false: overwrite
    };

    class GraphicsPipeline
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        vk::Pipeline m_pipeline;
        vk::PipelineLayout m_pipeline_layout;

    public:
        GraphicsPipeline(gsl::not_null<const LogicalDevice *> device, const GraphicsPipelineCreateInfo &create_info);
        GraphicsPipeline(gsl::not_null<const LogicalDevice *> device, const vk::GraphicsPipelineCreateInfo &create_info);
        GraphicsPipeline(const GraphicsPipeline &) = delete;            // non-copyable
        GraphicsPipeline &operator=(const GraphicsPipeline &) = delete; // non-copyable
        GraphicsPipeline(GraphicsPipeline &&) = default;                // movable
        GraphicsPipeline &operator=(GraphicsPipeline &&) = default;     // movable
        ~GraphicsPipeline();

        vk::Pipeline pipeline() const { return m_pipeline; }
        vk::PipelineLayout pipeline_layout() const { return m_pipeline_layout; }
        operator vk::Pipeline() const { return m_pipeline; }

        static vk::PipelineColorBlendAttachmentState defulat_color_blend_attachment_alpha();
        static vk::PipelineColorBlendAttachmentState defulat_color_blend_attachment_overwrite();
    };
}