#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>

namespace jre
{
    class LogicalDevice;
    class PhysicalDevice;
    class CommandBuffer;
    class Buffer
    {
    protected:
        vk::Buffer m_buffer;
        vk::DeviceMemory m_memory;
        vk::DeviceSize m_size;
        gsl::not_null<const LogicalDevice *> m_device;
        void *m_mapped_memory;

    public:
        Buffer(gsl::not_null<const LogicalDevice *> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        Buffer(const Buffer &) = delete;
        Buffer &operator=(const Buffer &) = delete;
        Buffer(Buffer &&) = default;
        Buffer &operator=(Buffer &&) = default;
        virtual ~Buffer();

        const vk::Buffer &buffer() const { return m_buffer; }
        const vk::DeviceMemory &memory() const { return m_memory; }
        vk::DeviceSize size() const { return m_size; }

        Buffer &map_memory(vk::DeviceSize size);
        Buffer &unmap_memory();
        Buffer &update_buffer(const void *data, vk::DeviceSize size);

        static void copy_buffer(const LogicalDevice &device, const CommandBuffer &command_buffer, vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size);
    };

    class DeviceLocalBuffer : public Buffer
    {
    public:
        DeviceLocalBuffer(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, vk::BufferUsageFlags usage, vk::DeviceSize size, const void *data);
        DeviceLocalBuffer(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, vk::BufferUsageFlags usage, const std::vector<std::byte> &data) : DeviceLocalBuffer(device, command_buffer, usage, data.size(), data.data()) {}
    };

    class HostVisibleBuffer : public Buffer
    {
    public:
        HostVisibleBuffer(gsl::not_null<const LogicalDevice *> device, vk::BufferUsageFlags usage, vk::DeviceSize size);
        HostVisibleBuffer(gsl::not_null<const LogicalDevice *> device, const vk::BufferUsageFlags usage, vk::DeviceSize size, const void *data);
        HostVisibleBuffer(gsl::not_null<const LogicalDevice *> device, vk::BufferUsageFlags usage, const std::vector<std::byte> &data) : HostVisibleBuffer(device, usage, data.size(), data.data()) {}
    };

    template <typename VertexType>
    class HostVertexBuffer : public HostVisibleBuffer
    {
    public:
        HostVertexBuffer(gsl::not_null<const LogicalDevice *> device, const std::vector<VertexType> &data)
            : HostVisibleBuffer(device, vk::BufferUsageFlagBits::eVertexBuffer, data.size() * sizeof(VertexType), data.data()) {}

        uint32_t count() const { return m_size / sizeof(VertexType); }
        void update_buffer(const std::vector<VertexType> &data) { HostVisibleBuffer::update_buffer(data.data(), data.size() * sizeof(VertexType)); }
    };

    template <typename VertexType>
    class VertexBuffer : public DeviceLocalBuffer
    {
    public:
        VertexBuffer(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, const std::vector<VertexType> &data)
            : DeviceLocalBuffer(device, command_buffer, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, data.size() * sizeof(VertexType), data.data()) {}

        uint32_t count() const { return m_size / sizeof(VertexType); }
    };

    template <typename IndexType>
    class HostIndexBuffer : public HostVisibleBuffer
    {
    public:
        HostIndexBuffer(gsl::not_null<const LogicalDevice *> device, const std::vector<IndexType> &data)
            : HostVisibleBuffer(device, vk::BufferUsageFlagBits::eIndexBuffer, data.size() * sizeof(IndexType), data.data()) {}

        uint32_t count() const { return static_cast<uint32_t>(m_size) / sizeof(IndexType); }
        void update_buffer(const std::vector<IndexType> &data) { HostVisibleBuffer::update_buffer(data.data(), data.size() * sizeof(IndexType)); }
    };

    template <typename IndexType>
    class IndexBuffer : public DeviceLocalBuffer
    {
    public:
        IndexBuffer(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, const std::vector<IndexType> &data)
            : DeviceLocalBuffer(device, command_buffer, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, data.size() * sizeof(IndexType), data.data()) {}

        uint32_t count() const { return static_cast<uint32_t>(m_size) / sizeof(IndexType); }
        void update_buffer(const std::vector<IndexType> &data) { DeviceLocalBuffer::update_buffer(data.data(), data.size() * sizeof(IndexType)); }
    };

}