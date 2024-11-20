#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <stb/stb_image.h>
#include <vector>
#include <filesystem>
#include "jrenderer/image.h"
#include "jrenderer/command_buffer.h"
#include "jrenderer/resources.hpp"

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

    class Texture2D : public Image2D
    {

    public:
        Texture2D(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const IImageDataView &image, const Image2DCreateInfo *create_info = nullptr);
        Texture2D(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const Texture2DCreateInfo &create_info);

        vk::DescriptorImageInfo descriptor() const;

        static uint32_t get_mipmap_levels(uint32_t width, uint32_t height);

    private:
        Texture2D(gsl::not_null<const LogicalDevice *> logical_device, const CommandBuffer &command_buffer, const IImageDataView &image, uint32_t mipmap_levels);
    };

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
}