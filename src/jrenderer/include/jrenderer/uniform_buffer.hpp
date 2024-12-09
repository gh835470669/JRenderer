#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include "jrenderer/buffer.h"
#include "jmath.h"

namespace jre
{

    class LogicalDevice;

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

        void update(const UniformBufferObjectType &ubo) const
        {
            memcpy(m_mapped_memory, &ubo, sizeof(UniformBufferObjectType));
        }

        UniformBufferObjectType value() const
        {
            return *static_cast<UniformBufferObjectType *>(m_mapped_memory);
        }

        // 【能耗注意】 每改里面的一个数据，应该都会同步给GPU，毕竟用的是k::MemoryPropertyFlagBits::eHostCoherent。当然可以不用这个，自己flush，那为啥不用value()拷贝出去，改完后再全部update？
        UniformBufferObjectType &value_ref()
        {
            return *static_cast<UniformBufferObjectType *>(m_mapped_memory);
        }

        vk::DescriptorBufferInfo descriptor() const
        {
            return vk::DescriptorBufferInfo(buffer(), 0, sizeof(UniformBufferObjectType));
        }
    };
}
