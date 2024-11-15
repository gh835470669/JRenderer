#include "jrenderer/image.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/physical_device.h"
#include "jrenderer/command_buffer.h"

namespace jre
{
    Image2D::Image2D(gsl::not_null<const LogicalDevice *> logcial_device, const ImageCreateInfo &image_create_info) : m_logical_device(logcial_device)
    {
        vk::ImageCreateInfo vk_image_create_info = image_create_info.image_create_info;
        // auto image_properties = m_logical_device->physical_device()->physical_device().getImageFormatProperties(
        //     vk_image_create_info.format,
        //     vk_image_create_info.imageType,
        //     vk_image_create_info.tiling,
        //     vk_image_create_info.usage,
        //     vk_image_create_info.flags);
        auto format = logcial_device->physical_device()->find_supported_format(
            {vk::Format::eR8Unorm, vk::Format::eR8G8Unorm, vk::Format::eR8G8B8Unorm, vk::Format::eB8G8R8A8Unorm},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eSampledImage);
        m_image = m_logical_device->device().createImage(image_create_info.image_create_info);
        vk::MemoryRequirements mem_requirements = m_logical_device->device().getImageMemoryRequirements(m_image);

        vk::MemoryAllocateInfo alloc_info{};
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = m_logical_device->physical_device()->find_memory_type(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_memory = m_logical_device->device().allocateMemory(alloc_info);

        m_logical_device->device().bindImageMemory(m_image, m_memory, 0);

        // 创建image view
        vk::ImageViewCreateInfo view_info{};
        view_info.image = m_image;
        view_info.viewType = vk::ImageViewType::e2D;
        view_info.format = image_create_info.image_create_info.format;
        view_info.subresourceRange = image_create_info.image_subresource_range;
        m_image_view = m_logical_device->device().createImageView(view_info);
        m_format = image_create_info.image_create_info.format;
        m_image_layout = image_create_info.image_create_info.initialLayout;
        m_extent = image_create_info.image_create_info.extent;

        if (image_create_info.sampler_create_info.has_value())
        {
            m_sampler = m_logical_device->device().createSampler(image_create_info.sampler_create_info.value());
        }
    }

    Image2D::~Image2D()
    {
        if (m_sampler)
        {
            m_logical_device->device().destroySampler(m_sampler);
        }
        m_logical_device->device().destroyImageView(m_image_view);
        m_logical_device->device().destroyImage(m_image);
        m_logical_device->device().freeMemory(m_memory);
    }

    void Image2D::transition_image_layout(const CommandBuffer &command_buffer, vk::ImageLayout new_layout)
    {

        std::array<vk::ImageMemoryBarrier, 1> barriers;
        vk::ImageMemoryBarrier &barrier = barriers[0];
        barrier.oldLayout = m_image_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vk::PipelineStageFlagBits src_stage;
        vk::PipelineStageFlagBits dst_stage;
        vk::DependencyFlagBits dependency_flags;

        if (m_image_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
            dst_stage = vk::PipelineStageFlagBits::eTransfer;
            dependency_flags = vk::DependencyFlagBits::eByRegion;

            // command_buffer.command_buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
            //                                                 vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        else if (m_image_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            src_stage = vk::PipelineStageFlagBits::eTransfer;
            dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
            dependency_flags = vk::DependencyFlagBits::eByRegion;

            // command_buffer.command_buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
            //                                                 vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        command_buffer
            .begin()
            .pipelineBarrier(src_stage, dst_stage, dependency_flags, {}, {}, barriers)
            .end()
            .submit_wait_idle(m_logical_device->graphics_queue());

        m_image_layout = new_layout;
    }

    void Image2D::copy_from_buffer(const CommandBuffer &command_buffer, const vk::Buffer &buffer)
    {
        command_buffer.begin();

        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = m_extent;

        command_buffer.command_buffer().copyBufferToImage(buffer, m_image, m_image_layout, 1, &region);

        command_buffer.end().submit_wait_idle(m_logical_device->graphics_queue());
    }

    DepthImage2D::DepthImage2D(gsl::not_null<const LogicalDevice *> logical_device, const PhysicalDevice &physical_device, uint32_t width, uint32_t height) : Image2D(logical_device,
                                                                                                                                                                      ImageCreateInfo{
                                                                                                                                                                          vk::ImageCreateInfo(
                                                                                                                                                                              vk::ImageCreateFlags(),
                                                                                                                                                                              vk::ImageType::e2D,
                                                                                                                                                                              physical_device.find_supported_format(
                                                                                                                                                                                  {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                                                                                                                                                                  vk::ImageTiling::eOptimal,
                                                                                                                                                                                  vk::FormatFeatureFlagBits::eDepthStencilAttachment),
                                                                                                                                                                              {width, height, 1},
                                                                                                                                                                              1,
                                                                                                                                                                              1,
                                                                                                                                                                              vk::SampleCountFlagBits::e1,
                                                                                                                                                                              vk::ImageTiling::eOptimal,
                                                                                                                                                                              vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                                                                                                                                              vk::SharingMode::eExclusive),
                                                                                                                                                                          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1),
                                                                                                                                                                          {}})
    {
    }
}
