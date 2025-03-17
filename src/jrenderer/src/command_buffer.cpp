#include "jrenderer/command_buffer.h"

namespace jre
{

    CommandBufferRecorder &CommandBufferRecorder::reset()
    {
        command_buffer.reset(vk::CommandBufferResetFlags());
        return *this;
    }

    CommandBufferRecorder &CommandBufferRecorder::begin(vk::CommandBufferUsageFlags usage_flags)
    {
        command_buffer.begin(vk::CommandBufferBeginInfo{usage_flags});
        return *this;
    }

    CommandBufferRecorder &CommandBufferRecorder::end()
    {
        command_buffer.end();
        return *this;
    }

    CommandBufferRecorder &CommandBufferRecorder::copy_buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size)
    {
        vk::BufferCopy copy_region{};
        copy_region.size = size;
        command_buffer.copyBuffer(src_buffer, dst_buffer, copy_region);
        return *this;
    }

    CommandBufferRecorder &CommandBufferRecorder::submit(vk::Queue queue,
                                                         vk::ArrayProxy<const vk::Semaphore> wait_semaphores,
                                                         vk::ArrayProxy<const vk::PipelineStageFlags> wait_stages,
                                                         vk::ArrayProxy<const vk::Semaphore> signal_semaphores,
                                                         vk::Fence signal_fence)
    {
        vk::SubmitInfo submit_info{};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.waitSemaphoreCount = wait_semaphores.size();
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.signalSemaphoreCount = signal_semaphores.size();
        submit_info.pSignalSemaphores = signal_semaphores.data();
        queue.submit(submit_info, signal_fence);
        return *this;
    }

    CommandBufferRecorder &CommandBufferRecorder::submit_wait_idle(vk::Queue queue)
    {
        vk::SubmitInfo submit_info{};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        queue.submit(submit_info, nullptr);
        queue.waitIdle();
        return *this;
    }

    CommandBufferRecorder &CommandBufferRecorder::pipelineBarrier(vk::PipelineStageFlags src_stage,
                                                                  vk::PipelineStageFlags dst_stage,
                                                                  vk::DependencyFlags dependency_flags,
                                                                  const vk::ArrayProxy<const vk::MemoryBarrier> &memory_barriers,
                                                                  const vk::ArrayProxy<const vk::BufferMemoryBarrier> &buffer_memory_barriers,
                                                                  const vk::ArrayProxy<const vk::ImageMemoryBarrier> &image_memory_barriers)
    {
        command_buffer.pipelineBarrier(src_stage, dst_stage, dependency_flags, memory_barriers, buffer_memory_barriers, image_memory_barriers);
        return *this;
    }
}
