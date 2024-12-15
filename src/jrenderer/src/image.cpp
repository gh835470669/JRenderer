#include "jrenderer/image.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/physical_device.h"
#include "jrenderer/command_buffer.h"

namespace jre
{
    Image2D::Image2D(gsl::not_null<const LogicalDevice *> logcial_device, const Image2DCreateInfo &image_create_info) : m_logical_device(logcial_device)
    {
        vk::ImageCreateInfo vk_image_create_info = image_create_info.image_create_info;
        // auto image_properties = m_logical_device->physical_device()->physical_device().getImageFormatProperties(
        //     vk_image_create_info.format,
        //     vk_image_create_info.imageType,
        //     vk_image_create_info.tiling,
        //     vk_image_create_info.usage,
        //     vk_image_create_info.flags);
        m_image = m_logical_device->device().createImage(image_create_info.image_create_info);
        m_mipmap_levels = vk_image_create_info.mipLevels;
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

    Image2D::Image2D(Image2D &&other) noexcept
        : m_logical_device(other.m_logical_device),
          m_image(std::exchange(other.m_image, VK_NULL_HANDLE)),
          m_memory(std::exchange(other.m_memory, VK_NULL_HANDLE)),
          m_image_view(std::exchange(other.m_image_view, VK_NULL_HANDLE)),
          m_format(std::exchange(other.m_format, vk::Format::eUndefined)),
          m_image_layout(std::exchange(other.m_image_layout, vk::ImageLayout::eUndefined)),
          m_extent(std::exchange(other.m_extent, vk::Extent3D())),
          m_sampler(std::exchange(other.m_sampler, VK_NULL_HANDLE))
    {
    }

    void swap(Image2D &left, Image2D &right) noexcept
    {
        using std::swap;
        swap(left.m_logical_device, right.m_logical_device);
        swap(left.m_image, right.m_image);
        swap(left.m_memory, right.m_memory);
        swap(left.m_image_view, right.m_image_view);
        swap(left.m_format, right.m_format);
        swap(left.m_image_layout, right.m_image_layout);
        swap(left.m_extent, right.m_extent);
        swap(left.m_sampler, right.m_sampler);
    }

    Image2D &Image2D::operator=(Image2D &&other) noexcept
    {
        Image2D tmp(std::move(other));
        swap(*this, tmp);
        return *this;
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
        barrier.subresourceRange.levelCount = m_mipmap_levels;
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
            .submit_wait_idle(m_logical_device->transfer_queue());

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

        command_buffer.end().submit_wait_idle(m_logical_device->transfer_queue());
    }

    void Image2D::generate_mipmaps(const CommandBuffer &command_buffer)
    {
        if (!(m_logical_device->physical_device()->physical_device().getFormatProperties(m_format).optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        {
            throw std::runtime_error("image format does not support linear blitting!");
        }

        command_buffer.begin();

        vk::ImageMemoryBarrier barrier{};
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.levelCount = 1; // 每次只转1层
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        int32_t mip_width = m_extent.width;
        int32_t mip_height = m_extent.height;

        for (uint32_t i = 1; i < m_mipmap_levels; ++i)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion, {}, {}, barrier);

            vk::ImageBlit blit{};
            blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
            blit.srcOffsets[1] = vk::Offset3D{mip_width, mip_height, 1};
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
            blit.dstOffsets[1] = vk::Offset3D{std::max<int32_t>(mip_width / 2, 1), std::max<int32_t>(mip_height / 2, 1), 1};
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            command_buffer.command_buffer().blitImage(m_image, vk::ImageLayout::eTransferSrcOptimal, m_image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlagBits::eByRegion, {}, {}, barrier);

            if (mip_width > 1)
                mip_width /= 2;
            if (mip_height > 1)
                mip_height /= 2;
        }

        barrier.subresourceRange.baseMipLevel = m_mipmap_levels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlagBits::eByRegion, {}, {}, barrier);

        command_buffer.end();
        command_buffer.submit_wait_idle(m_logical_device->transfer_queue());
    }

    DepthImage2D::DepthImage2D(gsl::not_null<const LogicalDevice *> logical_device, const PhysicalDevice &physical_device, DepthImage2DCreateInfo create_info) : Image2D(logical_device,
                                                                                                                                                                         Image2DCreateInfo{
                                                                                                                                                                             vk::ImageCreateInfo(
                                                                                                                                                                                 vk::ImageCreateFlags(),
                                                                                                                                                                                 vk::ImageType::e2D,
                                                                                                                                                                                 physical_device.find_supported_format(
                                                                                                                                                                                     {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                                                                                                                                                                     vk::ImageTiling::eOptimal,
                                                                                                                                                                                     vk::FormatFeatureFlagBits::eDepthStencilAttachment),
                                                                                                                                                                                 {create_info.width, create_info.height, 1},
                                                                                                                                                                                 1,
                                                                                                                                                                                 1,
                                                                                                                                                                                 create_info.sample_count,
                                                                                                                                                                                 vk::ImageTiling::eOptimal,
                                                                                                                                                                                 vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                                                                                                                                                 vk::SharingMode::eExclusive),
                                                                                                                                                                             vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1),
                                                                                                                                                                             {}})
    {
        // https://vulkan-tutorial.com/Depth_buffering#page_Explicitly-transitioning-the-depth-image
        // We don't need to explicitly transition the layout of the image to a depth attachment because we'll take care of this in the render pass
    }

    ColorImage2D::ColorImage2D(gsl::not_null<const LogicalDevice *> logical_device, ColorImage2DCreateInfo color_image_2d_create_info)
        : Image2D(logical_device,
                  Image2DCreateInfo{
                      vk::ImageCreateInfo(
                          vk::ImageCreateFlags(),
                          vk::ImageType::e2D,
                          color_image_2d_create_info.format,
                          {color_image_2d_create_info.width, color_image_2d_create_info.height, 1},
                          1,
                          1,
                          color_image_2d_create_info.sample_count,
                          vk::ImageTiling::eOptimal,
                          color_image_2d_create_info.usage,
                          vk::SharingMode::eExclusive),
                      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                      {}})

    {
    }
}
