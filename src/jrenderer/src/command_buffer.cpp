#include "jrenderer/command_buffer.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/command_pool.h"

namespace jre
{
    CommandBuffer::~CommandBuffer()
    {
        m_device->device().freeCommandBuffers(*m_command_pool, m_command_buffer);
    };

    const CommandBuffer &CommandBuffer::reset() const
    {
        m_command_buffer.reset(vk::CommandBufferResetFlags());
        return *this;
    }

    const CommandBuffer &CommandBuffer::begin(vk::CommandBufferUsageFlags usage_flags) const
    {
        m_command_buffer.begin(vk::CommandBufferBeginInfo{usage_flags});
        return *this;
    }

    const CommandBuffer &CommandBuffer::end() const
    {
        m_command_buffer.end();
        return *this;
    }

    const CommandBuffer &CommandBuffer::copy_buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size) const
    {
        vk::BufferCopy copy_region{};
        copy_region.size = size;
        m_command_buffer.copyBuffer(src_buffer, dst_buffer, copy_region);
        return *this;
    }

    const CommandBuffer &CommandBuffer::submit(vk::Queue queue, vk::ArrayProxy<const vk::Semaphore> wait_semaphores, vk::ArrayProxy<const vk::PipelineStageFlags> wait_stages, vk::ArrayProxy<const vk::Semaphore> signal_semaphores, vk::Fence signal_fence) const
    {
        vk::SubmitInfo submit_info{};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_command_buffer;
        submit_info.waitSemaphoreCount = wait_semaphores.size();
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.signalSemaphoreCount = signal_semaphores.size();
        submit_info.pSignalSemaphores = signal_semaphores.data();
        queue.submit(submit_info, signal_fence);
        return *this;
    }
    const CommandBuffer &CommandBuffer::submit_wait_idle(vk::Queue queue) const
    {
        vk::SubmitInfo submit_info{};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_command_buffer;
        queue.submit(submit_info, nullptr);
        queue.waitIdle();
        return *this;
    }

    const CommandBuffer &CommandBuffer::pipelineBarrier(vk::PipelineStageFlags src_stage,
                                                        vk::PipelineStageFlags dst_stage,
                                                        vk::DependencyFlags dependency_flags,
                                                        const vk::ArrayProxy<const vk::MemoryBarrier> &memory_barriers,
                                                        const vk::ArrayProxy<const vk::BufferMemoryBarrier> &buffer_memory_barriers,
                                                        const vk::ArrayProxy<const vk::ImageMemoryBarrier> &image_memory_barriers) const
    {
        m_command_buffer.pipelineBarrier(src_stage, dst_stage, dependency_flags, memory_barriers, buffer_memory_barriers, image_memory_barriers);
        return *this;
    }
}
