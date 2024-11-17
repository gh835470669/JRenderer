#include "jrenderer/physical_device.h"
#include "jrenderer/instance.h"
#include "jrenderer/logical_device.h"

namespace jre
{
    PhysicalDevice::PhysicalDevice(const vk::PhysicalDevice &physical_device) : m_physical_device(physical_device)
    {
        // Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)
        m_properties = m_physical_device.getProperties();
        m_features = m_physical_device.getFeatures();
        m_memory_properties = m_physical_device.getMemoryProperties();
    }

    vk::Format PhysicalDevice::find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const
    {
        for (vk::Format format : candidates)
        {
            vk::FormatProperties props = m_physical_device.getFormatProperties(format);
            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    uint32_t PhysicalDevice::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) const
    {
        for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; ++i)
        {
            if ((type_filter & (1 << i)) && (m_memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    vk::SampleCountFlagBits PhysicalDevice::get_max_usable_sample_count() const
    {
        vk::SampleCountFlags counts = m_properties.limits.framebufferColorSampleCounts & m_properties.limits.framebufferDepthSampleCounts;
        if (counts & vk::SampleCountFlagBits::e64)
            return vk::SampleCountFlagBits::e64;
        if (counts & vk::SampleCountFlagBits::e32)
            return vk::SampleCountFlagBits::e32;
        if (counts & vk::SampleCountFlagBits::e16)
            return vk::SampleCountFlagBits::e16;
        if (counts & vk::SampleCountFlagBits::e8)
            return vk::SampleCountFlagBits::e8;
        if (counts & vk::SampleCountFlagBits::e4)
            return vk::SampleCountFlagBits::e4;
        if (counts & vk::SampleCountFlagBits::e2)
            return vk::SampleCountFlagBits::e2;
        return vk::SampleCountFlagBits::e1;
    }

    vk::SampleCountFlags PhysicalDevice::get_supported_sample_counts() const
    {
        return m_properties.limits.framebufferColorSampleCounts & m_properties.limits.framebufferDepthSampleCounts;
    }

    std::unique_ptr<LogicalDevice> PhysicalDevice::create_logical_device()
    {
        return std::make_unique<LogicalDevice>(this);
    }
}
