#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>

namespace jre
{

    class LogicalDevice;
    class RenderPass
    {
    private:
        vk::RenderPass m_render_pass;
        gsl::not_null<const LogicalDevice *> m_device;

    public:
        RenderPass() = default;
        RenderPass(gsl::not_null<const LogicalDevice *> logical_device, vk::Format color_format, vk::Format depth_format, vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);
        RenderPass(const RenderPass &) = delete;            // non-copyable
        RenderPass &operator=(const RenderPass &) = delete; // non-copyable
        RenderPass(RenderPass &&);
        RenderPass &operator=(RenderPass &&);
        ~RenderPass();

        vk::RenderPass render_pass() const { return m_render_pass; }
        operator vk::RenderPass() const { return m_render_pass; }

        void begin(vk::CommandBuffer command_buffer, vk::Framebuffer frame_buffer, vk::Rect2D render_area, vk::ClearColorValue color, vk::ClearDepthStencilValue depth_stencil) const;
        void end(vk::CommandBuffer command_buffer) const;
    };
}