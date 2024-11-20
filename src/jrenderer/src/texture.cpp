#include "jrenderer/texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "jrenderer/buffer.h"
#include <filesystem>

namespace jre
{
    Texture2D::Texture2D(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const IImageDataView &image, const Image2DCreateInfo *create_info)
        : Image2D(logical_device,
                  create_info ? *create_info : Image2DCreateInfo{vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(image.width(), image.height(), 1), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::SharingMode::eExclusive), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1), vk::SamplerCreateInfo(vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.0f, VK_FALSE, 16, VK_FALSE, vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack, {}, nullptr)})
    {
        HostVisibleBuffer staging_buffer(logical_device, vk::BufferUsageFlagBits::eTransferSrc, image.data_size(), image.data());
        transition_image_layout(command_buffer, vk::ImageLayout::eTransferDstOptimal);
        copy_from_buffer(command_buffer, staging_buffer.buffer());
        transition_image_layout(command_buffer, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    Texture2D::Texture2D(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const IImageDataView &image_view, uint32_t mipmap_levels)
        : Image2D(logical_device,
                  Image2DCreateInfo{
                      vk::ImageCreateInfo(
                          vk::ImageCreateFlags(),
                          vk::ImageType::e2D,
                          vk::Format::eR8G8B8A8Unorm,
                          vk::Extent3D(image_view.width(), image_view.height(), 1),
                          mipmap_levels,
                          1,
                          vk::SampleCountFlagBits::e1,
                          vk::ImageTiling::eOptimal,
                          mipmap_levels > 1 ? (vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled) : (vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled),
                          vk::SharingMode::eExclusive),
                      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,
                                                0, mipmap_levels,
                                                0, 1),
                      vk::SamplerCreateInfo(vk::SamplerCreateFlags(),
                                            vk::Filter::eLinear, vk::Filter::eLinear,
                                            vk::SamplerMipmapMode::eLinear,
                                            vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                                            0.0f,
                                            VK_FALSE, 16,
                                            VK_FALSE, vk::CompareOp::eNever,
                                            0.0f, mipmap_levels > 1 ? mipmap_levels : 0.0f,
                                            vk::BorderColor::eFloatTransparentBlack,
                                            {},
                                            nullptr)})
    {
        HostVisibleBuffer staging_buffer(logical_device, vk::BufferUsageFlagBits::eTransferSrc, image_view.data_size(), image_view.data());

        // 如果是mipmaps，会将所有的level都转成eTransferDstOptimal。因为barrier的baselevel是0，levelCount是最高
        // 如果都转成VK_IMAGE_LAYOUT_GENERAL的话，就直接blit就行了，但是会慢，所以选择转src和dst
        transition_image_layout(command_buffer, vk::ImageLayout::eTransferDstOptimal);

        copy_from_buffer(command_buffer, staging_buffer.buffer());
        if (mipmap_levels > 1)
        {
            // 生成剩下的mipmaps，并将所有的mipmaps转成eShaderReadOnlyOptimal
            generate_mipmaps(command_buffer);
        }
        else
        {
            transition_image_layout(command_buffer, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

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
    }

    Texture2D::Texture2D(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const Texture2DCreateInfo &create_info)
        : Texture2D(logical_device,
                    command_buffer,
                    *create_info.image_view,
                    create_info.use_mipmaps ? get_mipmap_levels(create_info.image_view->width(), create_info.image_view->height()) : 1)
    {
    }

    uint32_t Texture2D::get_mipmap_levels(uint32_t width, uint32_t height)
    {
        // 具有N个（N>0）结点的完全二叉树的高度为 ⌈log2(N+1)⌉或 ⌊log2N⌋ +1
        // 我还以为 ceiling(x) = floor(x) + 1呢，除整数外就是。正确的公式是：⌈x⌉=⌊x⌋+[x∉Z] where [⋯] are Iverson Brackets
        return static_cast<uint32_t>(std::floor(std::log2(std::max<uint32_t>(width, height)))) + 1;
    }

    vk::DescriptorImageInfo Texture2D::descriptor() const
    {
        vk::DescriptorImageInfo image_info;
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = m_image_view;
        image_info.sampler = m_sampler;
        return image_info;
    }

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
}
