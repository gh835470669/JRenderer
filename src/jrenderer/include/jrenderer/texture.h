#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <stb/stb_image.h>
#include <vector>
#include <filesystem>
#include "jrenderer/image.h"
#include "jrenderer/command_buffer.h"
#include "jrenderer/resources.hpp"
#include "jrenderer/buffer.h"

namespace jre
{

    class IImageDataView
    {
    public:
        virtual uint32_t width() const = 0;
        virtual uint32_t height() const = 0;
        virtual uint32_t channels() const { return 4; }
        virtual const void *data() const = 0;
        virtual size_t data_size() const = 0;
    };

    class ImageDataRef : public IImageDataView
    {
    public:
        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_channels;
        const unsigned char *m_data;

        ImageDataRef(uint32_t width, uint32_t height, uint32_t channels, const unsigned char *data)
            : m_width(width), m_height(height), m_channels(channels), m_data(data) {}

        uint32_t width() const override { return m_width; }
        uint32_t height() const override { return m_height; }
        uint32_t channels() const override { return m_channels; }
        const void *data() const override { return m_data; }
        size_t data_size() const override { return m_width * m_height * m_channels * sizeof(unsigned char); }
    };

    class STBImageData : public IImageDataView
    {
    private:
        int m_width;
        int m_height;
        int m_channels;
        stbi_uc *m_data;

    public:
        STBImageData(const std::string &file_name);
        ~STBImageData();
        uint32_t width() const override { return m_width; }
        uint32_t height() const override { return m_height; }
        const void *data() const override { return m_data; }
        size_t data_size() const override { return m_width * m_height * m_channels; }
    };

    struct Texture2DCreateInfo
    {
        const IImageDataView *image_view;
        bool use_mipmaps = false;
    };

    // 尝试Conditional Member Definition
    // 也尝试下我的设想，MetaData 与 最小定义集（Texture） 分开
    // 这样会让数据异构，但是可以减少内存占用
    // 带Meta与没带的异构，Meta内部如果太细的话，异构会更严重。
    // 异构数据之间的转换很麻烦。想到的解决方案 1. Inteface 抽象类 2. 使用的地方用模板
    template <bool Name = false>
    struct Texture2DMeta
    {
    };

    template <>
    struct Texture2DMeta<true>
    {
        std::string name;
    };

    template <bool Name = false>
    class Texture2DWithMeta : public Image2D, public Texture2DMeta<Name>
    {

    public:
        Texture2DWithMeta(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const IImageDataView &image, const Image2DCreateInfo *create_info = nullptr, Texture2DMeta<Name> meta = {})
            : Image2D(logical_device,
                      create_info ? *create_info : Image2DCreateInfo{vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(image.width(), image.height(), 1), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::SharingMode::eExclusive), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1), vk::SamplerCreateInfo(vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.0f, VK_FALSE, 16, VK_FALSE, vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack, {}, nullptr)}),
              Texture2DMeta<Name>(meta)
        {
            HostVisibleBuffer staging_buffer(logical_device, vk::BufferUsageFlagBits::eTransferSrc, image.data_size(), image.data());
            transition_image_layout(command_buffer, vk::ImageLayout::eTransferDstOptimal);
            copy_from_buffer(command_buffer, staging_buffer.buffer());
            transition_image_layout(command_buffer, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        Texture2DWithMeta(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const Texture2DCreateInfo &create_info, Texture2DMeta<Name> meta = {})
            : Texture2DWithMeta(logical_device,
                                command_buffer,
                                *create_info.image_view,
                                create_info.use_mipmaps ? get_mipmap_levels(create_info.image_view->width(), create_info.image_view->height()) : 1, meta)
        {
        }

        vk::DescriptorImageInfo descriptor() const
        {
            vk::DescriptorImageInfo image_info;
            image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            image_info.imageView = m_image_view;
            image_info.sampler = m_sampler;
            return image_info;
        }

        static uint32_t get_mipmap_levels(uint32_t width, uint32_t height)
        {
            // 具有N个（N>0）结点的完全二叉树的高度为 ⌈log2(N+1)⌉或 ⌊log2N⌋ +1
            // 我还以为 ceiling(x) = floor(x) + 1呢，除整数外就是。正确的公式是：⌈x⌉=⌊x⌋+[x∉Z] where [⋯] are Iverson Brackets
            return static_cast<uint32_t>(std::floor(std::log2(std::max<uint32_t>(width, height)))) + 1;
        }

    private:
        Texture2DWithMeta(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const IImageDataView &image_view, uint32_t mipmap_levels, Texture2DMeta<Name> meta = {})
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
                                                nullptr)}),
              Texture2DMeta<Name>(meta)
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
    };

    using Texture2D = Texture2DWithMeta<>;

    class ITextureLoader
    {
    public:
        virtual std::unique_ptr<IImageDataView> load(const std::string &name) const = 0;
    };

    class STBTextureLoader : public ITextureLoader
    {
    public:
        std::unique_ptr<IImageDataView> load(const std::string &name) const override;
    };

    class TextureLoaderSelector
    {
    public:
        std::unique_ptr<ITextureLoader> get_loader_from_name(const std::string &file_name) const;
        std::unique_ptr<ITextureLoader> get_loader_from_extension(const std::filesystem::path &ext) const;
    };

    class TextureResources
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        std::unique_ptr<const CommandBuffer> m_default_command_buffer;
        Resources<std::string, Texture2D> m_textures;

    public:
        TextureResources(gsl::not_null<const LogicalDevice *> device, std::unique_ptr<const CommandBuffer> default_command_buffer) : m_device(device), m_default_command_buffer(std::move(default_command_buffer)) {}

        void insert(const std::string &name, std::shared_ptr<Texture2D> texture) { m_textures.insert(name, texture); }
        std::shared_ptr<Texture2D> get(const std::string &name, const ITextureLoader &loader = STBTextureLoader(), const CommandBuffer *command_buffer = nullptr);
    };

    class Texture2DDynamicMipmaps : public Texture2D
    {
    public:
        Texture2DDynamicMipmaps(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const Texture2DCreateInfo &create_info);

        void set_mipmaps(bool use_mipmaps);

    private:
        bool m_mipmaps_using;
    };

}