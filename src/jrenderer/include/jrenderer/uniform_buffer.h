#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include "jrenderer/buffer.h"
#include "jmath.h"

namespace jre
{
    struct UniformBufferObject
    {
        jmath::mat4 model;
        jmath::mat4 view;
        jmath::mat4 proj;
    };

    class LogicalDevice;

    // limits.minUniformBufferOffsetAlignment

    template <typename UniformBufferObjectType>
    class UniformBuffer : public HostVisibleBuffer
    {
    public:
        UniformBuffer(gsl::not_null<const LogicalDevice *> device) : HostVisibleBuffer(device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformBufferObjectType))
        {
            map_memory(sizeof(UniformBufferObjectType)); // [persistent mapping]
        }
        ~UniformBuffer()
        {
            unmap_memory();
        }

        void update_buffer(const UniformBufferObjectType &ubo)
        {
            memcpy(m_mapped_memory, &ubo, sizeof(UniformBufferObjectType));
        }

        vk::DescriptorBufferInfo descriptor() const
        {
            return vk::DescriptorBufferInfo(buffer(), 0, sizeof(UniformBufferObjectType));
        }
    };
}
