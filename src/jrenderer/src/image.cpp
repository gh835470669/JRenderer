#include "jrenderer/image.h"
#include "jrenderer/command_buffer.h"
#include <vulkan_utils/utils.hpp>

namespace jre
{
    DeviceImage DeviceImageBuilder::build()
    {
        auto image = image_builder.build();
        auto image_view_builder = image_builder.get_view_builder(*image);
        image_view_builder.image_view_create_info = image_view_create_info;
        image_view_builder.image_view_create_info.setImage(image.get()).setFormat(image_builder.image_create_info.format);
        vk::SharedDevice device = image_builder.device;
        vk::DeviceMemory memory = vk::su::allocateDeviceMemory(device.get(),
                                                               physical_device.getMemoryProperties(),
                                                               device->getImageMemoryRequirements(*image),
                                                               memory_properties);

        device->bindImageMemory(*image, memory, 0);
        return {
            image,
            vk::SharedDeviceMemory(memory, device),
            image_view_builder.build(),
            sampler_create_info ? vk::SharedSampler(
                                      device->createSampler(sampler_create_info.value()),
                                      device)
                                : vk::SharedSampler()};
    }

    DepthStencilAttachment2DBuilder::DepthStencilAttachment2DBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device)
        : DeviceImageBuilder(device, physical_device)
    {
        image_builder.image_create_info = vk::ImageCreateInfo{
            {},
            vk::ImageType::e2D,
            vk::su::pickDepthFormat(physical_device),
            {},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::SharingMode::eExclusive};
        image_view_create_info.setViewType(vk::ImageViewType::e2D)
            .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
        memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    }

    ColorAttachment2DBuilder::ColorAttachment2DBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device)
        : DeviceImageBuilder(device, physical_device)
    {
        image_builder.image_create_info = vk::ImageCreateInfo{
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive};
        image_view_create_info.setViewType(vk::ImageViewType::e2D)
            .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    }
}
