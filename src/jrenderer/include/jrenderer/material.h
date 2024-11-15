#pragma once

#include "jrenderer/texture.h"
#include "jrenderer/descriptor.h"
#include "jrenderer/uniform_buffer.h"

namespace jre
{
    class Material
    {
    public:
        std::vector<std::shared_ptr<Texture2D>> textures;
        std::unique_ptr<DescriptorSet> descriptor_set;
        std::unique_ptr<UniformBuffer<UniformBufferObject>> uniform_buffer;
    };
}