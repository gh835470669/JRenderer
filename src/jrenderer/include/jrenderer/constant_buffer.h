#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>

namespace jre
{
    class LogicalDevice;

    template <typename ConstantBufferType>
    class ConstantBuffer
    {
    public:
        vk::PushConstantRange get_push_constant_range(vk::ShaderStageFlag stage) const
        {
            vk::PushConstantRange range;
            range.stageFlags = stage;
            range.offset = 0;
            range.size = sizeof(ConstantBufferType);
            return range;
        }
    };
}