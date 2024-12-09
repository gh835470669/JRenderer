#pragma once

#include "jrenderer/texture.h"
#include "jrenderer/descriptor.h"
#include "jrenderer/uniform_buffer.hpp"
#include "jrenderer/concrete_uniform_buffers.h"

namespace jre
{
    class Material
    {
    public:
        std::vector<std::shared_ptr<Texture2D>> textures;
        std::unique_ptr<DescriptorSet> descriptor_set;
        std::unique_ptr<UniformBuffer<UniformPerObject>> uniform_buffer;
    };
}