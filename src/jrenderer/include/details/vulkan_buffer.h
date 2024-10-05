#pragma once

#include <vulkan/vulkan.hpp>

namespace jre
{

    class VulkanMemoryCreateInfo
    {
    public:
        vk::MemoryPropertyFlags properties;
    };

    class VulkanMemoryHandle
    {
        friend class VulkanPipeline;

    public:
        VulkanMemoryHandle() = default;
        VulkanMemoryHandle(vk::DeviceMemory memory) : m_memory(memory) {};
        ~VulkanMemoryHandle() = default;
        explicit operator vk::DeviceMemory() const { return m_memory; }

    private:
        vk::DeviceMemory m_memory;
    };

    class VulkanBufferCreateInfo
    {
    public:
        size_t size;
        vk::BufferUsageFlags usage;
    };

    class VulkanBufferHandle
    {
        friend class VulkanPipeline;

    public:
        VulkanBufferHandle(VulkanPipeline &pipeline, vk::Buffer m_buffer) : m_pipeline(pipeline), m_buffer(m_buffer) {};
        ~VulkanBufferHandle() = default;
        explicit operator vk::Buffer() const { return m_buffer; }
        VulkanMemoryHandle memory() const { return m_memory; }
        vk::Buffer buffer() const { return m_buffer; }
        template <typename T>
        void map_memory(const std::vector<T> &data)
        {
            m_pipeline.map_memory(m_memory, static_cast<const void *>(data.data()), data.size() * sizeof(T));
        }
        void destroy();
        void destroy_with_memory();

    private:
        vk::Buffer m_buffer;
        VulkanMemoryHandle m_memory;
        VulkanPipeline &m_pipeline;
    };

}