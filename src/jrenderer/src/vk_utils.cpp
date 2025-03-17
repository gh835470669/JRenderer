#include "jrenderer/utils/vk_utils.h"
#include "jrenderer/command_buffer.h"

namespace jre
{
    void submit(vk::Queue queue,
                vk::ArrayProxy<vk::CommandBuffer> command_buffers,
                vk::ArrayProxy<const vk::Semaphore> wait_semaphores,
                vk::ArrayProxy<const vk::PipelineStageFlags> wait_stages,
                vk::ArrayProxy<const vk::Semaphore> signal_semaphores,
                vk::Fence signal_fence)
    {
        queue.submit(vk::SubmitInfo(
                         wait_semaphores,
                         wait_stages,
                         command_buffers,
                         signal_semaphores),
                     signal_fence);
    }

    void present(vk::Queue queue,
                 vk::ArrayProxy<const vk::Semaphore> wait_semaphores,
                 vk::ArrayProxy<vk::SwapchainKHR> swap_chains,
                 uint32_t image_index)
    {
        queue.presentKHR(vk::PresentInfoKHR(
            wait_semaphores,
            swap_chains,
            image_index));
    }

    void copy_buffer_to_buffer(vk::CommandBuffer command_buffer,
                               vk::Queue queue,
                               vk::Buffer src_buffer,
                               vk::Buffer dst_buffer,
                               vk::DeviceSize size)
    {
        CommandBufferRecorder command{command_buffer};
        command.begin().copy_buffer(src_buffer, dst_buffer, size).end().submit_wait_idle(queue);
    }

    void transition_image_layout(vk::CommandBuffer command_buffer,
                                 vk::Queue transfer_queue,
                                 vk::Image image,
                                 vk::ImageLayout src_layout,
                                 vk::ImageLayout dst_layout,
                                 vk::ImageSubresourceRange subresource_range)
    {

        std::array<vk::ImageMemoryBarrier, 1> barriers;
        vk::ImageMemoryBarrier &barrier = barriers[0];
        barrier.oldLayout = src_layout;
        barrier.newLayout = dst_layout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;
        barrier.image = image;
        barrier.subresourceRange = subresource_range;

        vk::PipelineStageFlagBits src_stage;
        vk::PipelineStageFlagBits dst_stage;
        vk::DependencyFlagBits dependency_flags;

        if (src_layout == vk::ImageLayout::eUndefined && dst_layout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
            dst_stage = vk::PipelineStageFlagBits::eTransfer;
            dependency_flags = vk::DependencyFlagBits::eByRegion;
        }
        else if (src_layout == vk::ImageLayout::eTransferDstOptimal && dst_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            src_stage = vk::PipelineStageFlagBits::eTransfer;
            dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
            dependency_flags = vk::DependencyFlagBits::eByRegion;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        CommandBufferRecorder command(command_buffer);
        command
            .begin()
            .pipelineBarrier(src_stage, dst_stage, dependency_flags, {}, {}, barriers)
            .end()
            .submit_wait_idle(transfer_queue);
    }

    void copy_buffer_to_image(vk::CommandBuffer command_buffer,
                              vk::Queue transfer_queue,
                              vk::Buffer buffer,
                              vk::Image image,
                              vk::ImageLayout dst_layout,
                              vk::Extent3D extent)
    {
        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = extent;

        CommandBufferRecorder command(command_buffer);
        command.begin();
        command_buffer.copyBufferToImage(buffer, image, dst_layout, 1, &region);
        command.end().submit_wait_idle(transfer_queue);
    }

    vk::SamplerCreateInfo make_sampler_create_info(vk::SamplerAddressMode address_mode)
    {
        return vk::SamplerCreateInfo(vk::SamplerCreateFlags(),
                                     vk::Filter::eLinear,
                                     vk::Filter::eLinear,
                                     vk::SamplerMipmapMode::eLinear,
                                     address_mode,
                                     address_mode,
                                     address_mode,
                                     0.0f,
                                     VK_FALSE,
                                     0.0f,
                                     VK_FALSE,
                                     vk::CompareOp::eNever,
                                     0.0f,
                                     0.0f,
                                     vk::BorderColor::eFloatTransparentBlack,
                                     {},
                                     nullptr);
    }
}