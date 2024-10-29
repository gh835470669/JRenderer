#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <gsl/pointers>

namespace jre
{
    class LogicalDevice;
    class Frame
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;

        vk::Semaphore m_image_available_semaphore;
        vk::Semaphore m_render_finished_semaphore;
        vk::Fence m_in_flight_fence;

    public:
        Frame(gsl::not_null<const LogicalDevice *> logical_device);
        Frame(const Frame &) = delete;
        Frame &operator=(const Frame &) = delete;
        Frame(Frame &&) = default;
        Frame &operator=(Frame &&) = default;
        ~Frame();

        vk::Semaphore image_available_semaphore() const { return m_image_available_semaphore; }
        vk::Semaphore render_finished_semaphore() const { return m_render_finished_semaphore; }
        vk::Fence in_flight_fence() const { return m_in_flight_fence; }
    };

    class CommandBuffer;
    class FrameBuffer
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        vk::Framebuffer m_frame_buffer;
        std::unique_ptr<CommandBuffer> m_command_buffer;

    public:
        FrameBuffer(gsl::not_null<const LogicalDevice *> logical_device, vk::RenderPass render_pass, vk::Extent2D extent, std::vector<vk::ImageView> attachments, std::unique_ptr<CommandBuffer> command_buffer);
        FrameBuffer(const FrameBuffer &) = delete;
        FrameBuffer &operator=(const FrameBuffer &) = delete;
        FrameBuffer(FrameBuffer &&) = default;
        FrameBuffer &operator=(FrameBuffer &&) = default;
        ~FrameBuffer();

        vk::Framebuffer frame_buffer() const { return m_frame_buffer; }
        operator vk::Framebuffer() const { return m_frame_buffer; }
        const CommandBuffer *command_buffer() const { return m_command_buffer.get(); }
    };

}
