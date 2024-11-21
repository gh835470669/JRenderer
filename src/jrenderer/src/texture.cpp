#include "jrenderer/texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <filesystem>
#include "jrenderer/logical_device.h"

namespace jre
{

    STBImageData::STBImageData(const std::string &file_name)
    {
        m_data = stbi_load(file_name.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
        m_channels = 4;
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

    std::shared_ptr<Texture2D> TextureResources::get(const std::string &name, const ITextureLoader &loader, const CommandBuffer *command_buffer)
    {
        return std::make_shared<Texture2D>(m_device, command_buffer ? *command_buffer : *m_default_command_buffer, *loader.load(name));
    }

    std::unique_ptr<ITextureLoader> TextureLoaderSelector::get_loader_from_name(const std::string &file_name) const
    {
        return get_loader_from_extension(std::filesystem::path(file_name).extension());
    }

    std::unique_ptr<ITextureLoader> TextureLoaderSelector::get_loader_from_extension(const std::filesystem::path &ext) const
    {
        return std::make_unique<STBTextureLoader>();
    }

    Texture2DDynamicMipmaps::Texture2DDynamicMipmaps(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const Texture2DCreateInfo &create_info)
        : Texture2D(logical_device, command_buffer, create_info),
          m_mipmaps_using(create_info.use_mipmaps)
    {
    }

    void Texture2DDynamicMipmaps::set_mipmaps(bool use_mipmaps)
    {
        if (m_mipmaps_using == use_mipmaps)
            return;

        m_sampler = m_logical_device->device().createSampler(
            vk::SamplerCreateInfo(vk::SamplerCreateFlags(),
                                  vk::Filter::eLinear, vk::Filter::eLinear,
                                  vk::SamplerMipmapMode::eLinear,
                                  vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                                  0.0f,
                                  VK_FALSE, 16,
                                  VK_FALSE, vk::CompareOp::eNever,
                                  0.0f, use_mipmaps ? m_mipmap_levels : 0.0f,
                                  vk::BorderColor::eFloatTransparentBlack,
                                  {},
                                  nullptr));
        m_mipmaps_using = use_mipmaps;
    }
}
