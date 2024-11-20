#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <optional>

namespace jre
{
    struct Image2DCreateInfo
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
        uint32_t m_mipmap_levels;

    public:
        Image2D(gsl::not_null<const LogicalDevice *> logcial_device, const Image2DCreateInfo &image_create_info);
        Image2D(gsl::not_null<const LogicalDevice *> logcial_device) : m_logical_device(logcial_device), m_image(VK_NULL_HANDLE), m_memory(VK_NULL_HANDLE), m_image_view(VK_NULL_HANDLE), m_format() {};
        Image2D(const Image2D &) = delete;            // non-copyable
        Image2D &operator=(const Image2D &) = delete; // non-copyable
        Image2D(Image2D &&other) noexcept;            // movable
        Image2D &operator=(Image2D &&other) noexcept; // movable
        virtual ~Image2D();

        friend void swap(Image2D &left, Image2D &right) noexcept;

        inline const vk::Image &image() const { return m_image; }
        operator vk::Image() const { return m_image; }
        inline const vk::DeviceMemory &memory() const { return m_memory; }
        inline vk::ImageView image_view() const { return m_image_view; }
        inline vk::Format format() const { return m_format; }
        inline vk::Sampler sampler() const { return m_sampler; }

        void transition_image_layout(const CommandBuffer &command_buffer, vk::ImageLayout new_layout);
        void copy_from_buffer(const CommandBuffer &command_buffer, const vk::Buffer &buffer);
        void generate_mipmaps(const CommandBuffer &command_buffer);
    };

    struct DepthImage2DCreateInfo
    {
        uint32_t width;
        uint32_t height;
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
    };

    class DepthImage2D : public Image2D
    {
    public:
        DepthImage2D(gsl::not_null<const LogicalDevice *> logical_device, const PhysicalDevice &physical_device, DepthImage2DCreateInfo create_info);
        DepthImage2D(const DepthImage2D &) = delete;                                // non-copyable
        DepthImage2D &operator=(const DepthImage2D &) = delete;                     // non-copyable
        DepthImage2D(DepthImage2D &&other) noexcept : Image2D(std::move(other)) {}; // movable
        DepthImage2D &operator=(DepthImage2D &&other) noexcept
        {
            Image2D::operator=(std::move(other));
            return *this;
        }; // movable
    };

    struct ColorImage2DCreateInfo
    {
        uint32_t width;
        uint32_t height;
        vk::ImageUsageFlags usage;
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
        vk::Format format = vk::Format::eR8G8B8A8Unorm;
    };

    class ColorImage2D : public Image2D
    {
    public:
        ColorImage2D(gsl::not_null<const LogicalDevice *> logical_device, ColorImage2DCreateInfo color_image_2d_create_info);
        ColorImage2D(const ColorImage2D &) = delete;                                // non-copyable
        ColorImage2D &operator=(const ColorImage2D &) = delete;                     // non-copyable
        ColorImage2D(ColorImage2D &&other) noexcept : Image2D(std::move(other)) {}; // movable
        ColorImage2D &operator=(ColorImage2D &&other) noexcept
        {
            Image2D::operator=(std::move(other));
            return *this;
        }; // movable
    };

}