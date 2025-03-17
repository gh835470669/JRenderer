#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <vector>
#include <filesystem>
#include "jrenderer/image.h"
#include "jrenderer/command_buffer.h"
#include "jrenderer/resources.hpp"
#include "jrenderer/buffer.h"
#include "jrenderer/utils/vk_utils.h"

namespace jre
{

    struct TextureData
    {
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        const unsigned char *data;

        uint32_t size() const { return width * height * channels * sizeof(unsigned char); }
    };

    inline uint32_t get_mipmap_levels(uint32_t width, uint32_t height)
    {
        // 具有N个（N>0）结点的完全二叉树的高度为 ⌈log2(N+1)⌉或 ⌊log2N⌋ +1
        // 我还以为 ceiling(x) = floor(x) + 1呢，除整数外就是。正确的公式是：⌈x⌉=⌊x⌋+[x∉Z] where [⋯] are Iverson Brackets
        return static_cast<uint32_t>(std::floor(std::log2(std::max<uint32_t>(width, height)))) + 1;
    }

    class TextureBuilder : public DeviceImageBuilder
    {
    public:
        const TextureData &data;
        vk::CommandBuffer command_buffer;
        vk::Queue transfer_queue;
        bool generate_mipmaps = true;
        TextureBuilder(vk::SharedDevice device,
                       vk::PhysicalDevice physical_device,
                       vk::CommandBuffer command_buffer,
                       vk::Queue transfer_queue,
                       const TextureData &data);

        TextureBuilder &set_generate_mipmaps(bool generate_mipmaps_)
        {
            generate_mipmaps = generate_mipmaps_;
            return *this;
        }

        TextureBuilder &set_sampler(vk::SamplerCreateInfo sampler_create_info_)
        {
            sampler_create_info = sampler_create_info_;
            return *this;
        }

        DeviceImage build();
    };
}