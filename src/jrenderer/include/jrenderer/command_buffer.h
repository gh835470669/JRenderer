#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_shared.hpp>
#include <gsl/pointers>

namespace jre
{

    class CommandBufferRecorder
    {
    public:
        CommandBufferRecorder Create(vk::SharedCommandBuffer buffer) { return {buffer.get()}; }

        vk::CommandBuffer command_buffer;

        CommandBufferRecorder &reset();
        CommandBufferRecorder &begin(vk::CommandBufferUsageFlags usage_flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        CommandBufferRecorder &end();

        CommandBufferRecorder &copy_buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size);
        CommandBufferRecorder &submit(vk::Queue queue,
                                      vk::ArrayProxy<const vk::Semaphore> wait_semaphores = {},
                                      vk::ArrayProxy<const vk::PipelineStageFlags> wait_stages = {},
                                      vk::ArrayProxy<const vk::Semaphore> signal_semaphores = {},
                                      vk::Fence signal_fence = nullptr);
        CommandBufferRecorder &submit_wait_idle(vk::Queue queue);
        CommandBufferRecorder &pipelineBarrier(vk::PipelineStageFlags src_stage,
                                               vk::PipelineStageFlags dst_stage,
                                               vk::DependencyFlags dependency_flags = {},
                                               const vk::ArrayProxy<const vk::MemoryBarrier> &memory_barriers = {},
                                               const vk::ArrayProxy<const vk::BufferMemoryBarrier> &buffer_memory_barriers = {},
                                               const vk::ArrayProxy<const vk::ImageMemoryBarrier> &image_memory_barriers = {});
    };
}