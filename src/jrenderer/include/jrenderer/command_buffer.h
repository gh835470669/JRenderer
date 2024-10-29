#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>

namespace jre
{
    class LogicalDevice;
    class CommandPool;
    class CommandBuffer
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        gsl::not_null<const CommandPool *> m_command_pool;
        vk::CommandBuffer m_command_buffer;

    public:
        CommandBuffer(gsl::not_null<const LogicalDevice *> logical_device, gsl::not_null<const CommandPool *> command_pool, vk::CommandBuffer command_buffer) : m_device(logical_device), m_command_pool(command_pool), m_command_buffer(command_buffer) {}
        CommandBuffer(const CommandBuffer &) = delete;            // non-copyable
        CommandBuffer &operator=(const CommandBuffer &) = delete; // non-copyable
        CommandBuffer(CommandBuffer &&) = default;                // movable
        CommandBuffer &operator=(CommandBuffer &&) = default;     // movable
        ~CommandBuffer();

        const vk::CommandBuffer &command_buffer() const { return m_command_buffer; }
        operator vk::CommandBuffer() const { return m_command_buffer; }

        const CommandBuffer &begin(vk::CommandBufferUsageFlags usage_flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit) const;
        const CommandBuffer &end() const;

        const CommandBuffer &copy_buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size) const;
        const CommandBuffer &submit(vk::Queue queue, vk::ArrayProxy<const vk::Semaphore> wait_semaphores = {}, vk::ArrayProxy<const vk::PipelineStageFlags> wait_stages = {}, vk::ArrayProxy<const vk::Semaphore> signal_semaphores = {}, vk::Fence signal_fence = nullptr) const;
        const CommandBuffer &submit_wait_idle(vk::Queue queue) const;
        const CommandBuffer &pipelineBarrier(vk::PipelineStageFlags src_stage,
                                             vk::PipelineStageFlags dst_stage,
                                             vk::DependencyFlags dependency_flags = {},
                                             const vk::ArrayProxy<const vk::MemoryBarrier> &memory_barriers = {},
                                             const vk::ArrayProxy<const vk::BufferMemoryBarrier> &buffer_memory_barriers = {},
                                             const vk::ArrayProxy<const vk::ImageMemoryBarrier> &image_memory_barriers = {}) const;
    };
}