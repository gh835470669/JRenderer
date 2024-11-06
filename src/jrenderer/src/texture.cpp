#include "jrenderer/texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "jrenderer/buffer.h"

namespace jre
{
    Texture2D::Texture2D(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const IImageDataView &image, const ImageCreateInfo *create_info)
        : Image2D(logical_device,
                  create_info ? *create_info : ImageCreateInfo{vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(image.width(), image.height(), 1), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::SharingMode::eExclusive), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1), vk::SamplerCreateInfo(vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.0f, VK_FALSE, 16, VK_FALSE, vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack, {}, nullptr)})
    {
        HostVisibleBuffer staging_buffer(logical_device, vk::BufferUsageFlagBits::eTransferSrc, image.data_size(), image.data());
        transition_image_layout(command_buffer, vk::ImageLayout::eTransferDstOptimal);
        copy_from_buffer(command_buffer, staging_buffer.buffer());
        transition_image_layout(command_buffer, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    vk::DescriptorImageInfo Texture2D::descriptor() const
    {
        vk::DescriptorImageInfo image_info;
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = m_image_view;
        image_info.sampler = m_sampler;
        return image_info;
    }

    STBImageData::STBImageData(const std::string &file_name) : m_format(vk::Format::eR8G8B8A8Unorm)
    {
        m_channels = 4;
        m_data = stbi_load(file_name.c_str(), &m_width, &m_height, nullptr, STBI_rgb_alpha);
        if (!m_data)
        {
            throw std::runtime_error("failed to load texture image!");
        }
    }

    STBImageData::~STBImageData()
    {
        stbi_image_free(m_data);
    }

    std::unique_ptr<IImageDataView> STBTextureLoader::load(const std::string &name) const
    {
        return std::make_unique<STBImageData>(name);
    }

    std::shared_ptr<Texture2D> TextureResources::get_texture(const std::string &name, const ITextureLoader &loader, const CommandBuffer *command_buffer)
    {
        return std::make_shared<Texture2D>(m_device, command_buffer ? *command_buffer : *m_default_command_buffer, *loader.load(name));
    }

}
