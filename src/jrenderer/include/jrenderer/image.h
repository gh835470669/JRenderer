#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <optional>

namespace jre
{
    struct ImageCreateInfo
    {
        vk::ImageCreateInfo image_create_info;
        vk::ImageSubresourceRange image_subresource_range;
        std::optional<vk::SamplerCreateInfo> sampler_create_info;
    };

    class LogicalDevice;
    class PhysicalDevice;
    class CommandBuffer;
    class Image2D
    {
    protected:
        gsl::not_null<const LogicalDevice *> m_logical_device;
        vk::Image m_image;
        vk::DeviceMemory m_memory;
        vk::ImageView m_image_view;
        vk::Format m_format;
        vk::ImageLayout m_image_layout;
        vk::Extent3D m_extent;
        vk::Sampler m_sampler;

    public:
        Image2D(gsl::not_null<const LogicalDevice *> logcial_device, const ImageCreateInfo &image_create_info);
        Image2D(gsl::not_null<const LogicalDevice *> logcial_device) : m_logical_device(logcial_device), m_image(VK_NULL_HANDLE), m_memory(VK_NULL_HANDLE), m_image_view(VK_NULL_HANDLE), m_format() {};
        virtual ~Image2D();

        inline const vk::Image &image() const { return m_image; }
        operator vk::Image() const { return m_image; }
        inline const vk::DeviceMemory &memory() const { return m_memory; }
        inline vk::ImageView image_view() const { return m_image_view; }
        inline vk::Format format() const { return m_format; }
        inline vk::Sampler sampler() const { return m_sampler; }

        void transition_image_layout(const CommandBuffer &command_buffer, vk::ImageLayout new_layout);
        void copy_from_buffer(const CommandBuffer &command_buffer, const vk::Buffer &buffer);
    };

    class DepthImage2D : public Image2D
    {
    public:
        DepthImage2D(gsl::not_null<const LogicalDevice *> logical_device, const PhysicalDevice &physical_device, uint32_t width, uint32_t height);
    };

}