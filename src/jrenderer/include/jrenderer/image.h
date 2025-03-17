#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_shared.hpp>
#include <gsl/pointers>
#include <optional>
#include <variant>

namespace jre
{
    struct SharedImageViewBuilder
    {
        vk::SharedDevice device;
        vk::ImageViewCreateInfo image_view_create_info;

        SharedImageViewBuilder(vk::SharedDevice device, vk::Image image) : device(device), image_view_create_info({}, image) {}
        vk::SharedImageView build() { return vk::SharedImageView{device->createImageView(image_view_create_info), device}; }
    };

    struct SharedImageBuilder
    {
        vk::SharedDevice device;
        // initialLyout 只能是 vk::ImageLayout::eUndefined或vk::ImageLayout::ePreinitialized
        vk::ImageCreateInfo image_create_info;

        SharedImageBuilder &set_image_create_info(vk::ImageCreateInfo image_create_info_)
        {
            image_create_info = image_create_info_;
            return *this;
        }

        vk::SharedImage build() { return vk::SharedImage{device->createImage(image_create_info), device}; }

        SharedImageViewBuilder get_view_builder(vk::Image image)
        {
            SharedImageViewBuilder builder{device, image};
            builder.image_view_create_info.setFormat(image_create_info.format);
            return builder;
        }
    };

    struct DeviceImage
    {
        vk::SharedImage image;
        vk::SharedDeviceMemory memory;
        vk::SharedImageView image_view;
        vk::SharedSampler sampler;

        operator vk::DescriptorImageInfo()
        {
            return vk::DescriptorImageInfo{sampler.get(), image_view.get(), vk::ImageLayout::eShaderReadOnlyOptimal};
        }
    };

    using Texture = DeviceImage;

    class DeviceImageBuilder
    {
    public:
        SharedImageBuilder image_builder;
        vk::PhysicalDevice physical_device;
        vk::ImageViewCreateInfo image_view_create_info;
        vk::MemoryPropertyFlagBits memory_properties;
        std::optional<vk::SamplerCreateInfo> sampler_create_info;

        DeviceImageBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device)
            : image_builder(device), physical_device(physical_device) {}

        DeviceImageBuilder &set_image_create_info(vk::ImageCreateInfo info)
        {
            image_builder.set_image_create_info(info);
            return *this;
        }

        DeviceImageBuilder &set_extent(vk::Extent2D extent)
        {
            image_builder.image_create_info.extent = vk::Extent3D{extent.width, extent.height, 1};
            return *this;
        }

        DeviceImageBuilder &set_sample_count(vk::SampleCountFlagBits sample_count)
        {
            image_builder.image_create_info.samples = sample_count;
            return *this;
        }

        DeviceImageBuilder &set_sampler(std::optional<vk::SamplerCreateInfo> sampler_create_info_)
        {
            this->sampler_create_info = sampler_create_info_;
            return *this;
        }

        DeviceImageBuilder &set_usage(vk::ImageUsageFlags usage)
        {
            image_builder.image_create_info.usage = usage;
            return *this;
        }

        DeviceImage build();
    };

    class DepthStencilAttachment2DBuilder : public DeviceImageBuilder
    {
    public:
        DepthStencilAttachment2DBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device);
    };

    class ColorAttachment2DBuilder : public DeviceImageBuilder
    {
    public:
        ColorAttachment2DBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device);
    };

}