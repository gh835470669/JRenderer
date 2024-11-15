#pragma once

#include <vulkan/vulkan.hpp>

namespace jre
{
    class Instance;
    class LogicalDevice;
    class PhysicalDevice
    {
    private:
        vk::PhysicalDevice m_physical_device;

        vk::PhysicalDeviceProperties m_properties;
        vk::PhysicalDeviceFeatures m_features;
        vk::PhysicalDeviceMemoryProperties m_memory_properties;

    public:
        PhysicalDevice(const vk::PhysicalDevice &physical_device);
        PhysicalDevice(const PhysicalDevice &) = delete;            // non-copyable
        PhysicalDevice &operator=(const PhysicalDevice &) = delete; // non-copyable
        PhysicalDevice(PhysicalDevice &&) = default;                // movable
        PhysicalDevice &operator=(PhysicalDevice &&) = default;     // movable
        ~PhysicalDevice() = default;

        inline const vk::PhysicalDevice &physical_device() const noexcept { return m_physical_device; }
        operator const vk::PhysicalDevice &() const noexcept { return m_physical_device; }

        inline const vk::PhysicalDeviceProperties &properties() const noexcept { return m_properties; }
        inline const vk::PhysicalDeviceFeatures &features() const noexcept { return m_features; }
        inline const vk::PhysicalDeviceMemoryProperties &memory_properties() const noexcept { return m_memory_properties; }

        vk::Format find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
        uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) const;
        vk::SampleCountFlagBits get_max_usable_sample_count() const;

        std::unique_ptr<LogicalDevice> create_logical_device();
    };

}