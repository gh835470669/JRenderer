#pragma once

#include <vulkan/vulkan_shared.hpp>
#include <variant>
#include <list>

namespace jre
{
    void submit(vk::Queue queue,
                vk::ArrayProxy<vk::CommandBuffer> command_buffers,
                vk::ArrayProxy<const vk::Semaphore> wait_semaphores,
                vk::ArrayProxy<const vk::PipelineStageFlags> wait_stages,
                vk::ArrayProxy<const vk::Semaphore> signal_semaphores,
                vk::Fence signal_fence);
    void copy_buffer_to_buffer(vk::CommandBuffer command_buffer,
                               vk::Queue queue,
                               vk::Buffer src_buffer,
                               vk::Buffer dst_buffer,
                               vk::DeviceSize size);
    void transition_image_layout(vk::CommandBuffer command_buffer,
                                 vk::Queue transfer_queue,
                                 vk::Image image,
                                 vk::ImageLayout src_layout,
                                 vk::ImageLayout dst_layout,
                                 vk::ImageSubresourceRange subresource_range = {});
    void copy_buffer_to_image(vk::CommandBuffer command_buffer,
                              vk::Queue transfer_queue,
                              vk::Buffer buffer,
                              vk::Image image,
                              vk::ImageLayout dst_layout,
                              vk::Extent3D extent);
    void present(vk::Queue queue,
                 vk::ArrayProxy<const vk::Semaphore> wait_semaphores,
                 vk::ArrayProxy<vk::SwapchainKHR> swap_chains,
                 uint32_t image_index);

    vk::SamplerCreateInfo make_sampler_create_info(vk::SamplerAddressMode address_mode = vk::SamplerAddressMode::eClampToEdge);
}