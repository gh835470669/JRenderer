#include "jrenderer/texture.h"

namespace jre
{
    static void generate_image_mipmaps(vk::CommandBuffer command_buffer, vk::Queue transfer_queue, vk::Image image, uint32_t width, uint32_t height, uint32_t mipmap_levels)
    {
        // if (!(physical_device.getFormatProperties(m_format).optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        // {
        //     throw std::runtime_error("image format does not support linear blitting!");
        // }

        // mimap sampler pseudo code (https://vulkan-tutorial.com/Generating_Mipmaps#page_Sampler)
        // lod = getLodLevelFromScreenSize(); //smaller when the object is close, may be negative
        // lod = clamp(lod + mipLodBias, minLod, maxLod);

        // level = clamp(floor(lod), 0, texture.mipLevels - 1);  //clamped to the number of mip levels in the texture

        // if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
        //     color = sample(level);
        // } else {
        //     color = blend(sample(level), sample(level + 1));
        // }

        // if (lod <= 0) {
        //     color = readTexture(uv, magFilter);
        // } else {
        //     color = readTexture(uv, minFilter);
        // }

        CommandBufferRecorder recorder(command_buffer);
        recorder.begin();

        vk::ImageMemoryBarrier barrier{};
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.levelCount = 1; // 每次只转1层
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        int32_t mip_width = width;
        int32_t mip_height = height;

        for (uint32_t i = 1; i < mipmap_levels; ++i)
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

            command_buffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

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

        barrier.subresourceRange.baseMipLevel = mipmap_levels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlagBits::eByRegion, {}, {}, barrier);

        recorder.end();
        recorder.submit_wait_idle(transfer_queue);
    }

    TextureBuilder::TextureBuilder(vk::SharedDevice device,
                                   vk::PhysicalDevice physical_device,
                                   vk::CommandBuffer command_buffer,
                                   vk::Queue transfer_queue,
                                   const TextureData &data)
        : DeviceImageBuilder(device, physical_device),
          data(data),
          command_buffer(command_buffer),
          transfer_queue(transfer_queue)
    {
        image_builder.image_create_info =
            vk::ImageCreateInfo{
                {},
                vk::ImageType::e2D,
                vk::Format::eR8G8B8A8Unorm,
                vk::Extent3D(data.width, data.height, 1),
                1,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                vk::SharingMode::eExclusive,
                {}};
        image_view_create_info.setViewType(vk::ImageViewType::e2D)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    }

    DeviceImage TextureBuilder::build()
    {
        assert(sampler_create_info.has_value());
        if (generate_mipmaps)
        {
            image_builder.image_create_info.mipLevels = get_mipmap_levels(image_builder.image_create_info.extent.width, image_builder.image_create_info.extent.height);
            image_builder.image_create_info.usage |= vk::ImageUsageFlagBits::eTransferSrc;
            image_view_create_info.subresourceRange.levelCount = image_builder.image_create_info.mipLevels;
            sampler_create_info.value().setMaxLod(static_cast<float>(image_builder.image_create_info.mipLevels));
        }
        DeviceImage image_data = DeviceImageBuilder::build();
        DynamicBuffer staging_buffer = HostVisibleDynamicBufferBuilder(image_builder.device, physical_device, data.size())
                                           .set_usage(vk::BufferUsageFlagBits::eTransferSrc)
                                           .build(data.data, data.size());

        // 如果是mipmaps，会将所有的level都转成eTransferDstOptimal。因为barrier的baselevel是0，levelCount是最高
        // 如果都转成VK_IMAGE_LAYOUT_GENERAL的话，就直接blit就行了，但是会慢，所以选择转src和dst
        transition_image_layout(command_buffer,
                                transfer_queue,
                                image_data.image.get(),
                                image_builder.image_create_info.initialLayout,
                                vk::ImageLayout::eTransferDstOptimal,
                                vk::ImageSubresourceRange(
                                    vk::ImageAspectFlagBits::eColor,
                                    0,
                                    generate_mipmaps ? image_builder.image_create_info.mipLevels : 1,
                                    0,
                                    1));

        copy_buffer_to_image(command_buffer,
                             transfer_queue,
                             staging_buffer.buffer().get(),
                             image_data.image.get(),
                             vk::ImageLayout::eTransferDstOptimal,
                             image_builder.image_create_info.extent);

        if (generate_mipmaps)
        {
            generate_image_mipmaps(command_buffer,
                                   transfer_queue,
                                   image_data.image.get(),
                                   image_builder.image_create_info.extent.width,
                                   image_builder.image_create_info.extent.height,
                                   image_builder.image_create_info.mipLevels);
        }
        else
        {
            transition_image_layout(command_buffer,
                                    transfer_queue,
                                    image_data.image.get(),
                                    vk::ImageLayout::eTransferDstOptimal,
                                    vk::ImageLayout::eShaderReadOnlyOptimal,
                                    vk::ImageSubresourceRange(
                                        vk::ImageAspectFlagBits::eColor,
                                        0,
                                        1,
                                        0,
                                        1));
        }
        return image_data;
    }
}
