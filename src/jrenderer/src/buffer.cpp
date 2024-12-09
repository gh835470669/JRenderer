#include "jrenderer/buffer.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/physical_device.h"
#include "jrenderer/command_buffer.h"
namespace jre
{
    Buffer::Buffer(gsl::not_null<const LogicalDevice *> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) : m_device(device), m_size(size), m_mapped_memory(nullptr)
    {
        vk::BufferCreateInfo buffer_info;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = vk::SharingMode::eExclusive;
        m_buffer = device->device().createBuffer(buffer_info);

        vk::MemoryRequirements mem_requirements = device->device().getBufferMemoryRequirements(m_buffer);

        vk::MemoryAllocateInfo alloc_info{};
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = device->physical_device()->find_memory_type(mem_requirements.memoryTypeBits, properties);

        m_memory = device->device().allocateMemory(alloc_info);

        device->device().bindBufferMemory(m_buffer, m_memory, 0);
    }

    Buffer::~Buffer()
    {
        m_device->device().destroyBuffer(m_buffer);
        m_device->device().freeMemory(m_memory);
    }

    Buffer &Buffer::map_memory(vk::DeviceSize size)
    {
        if (m_mapped_memory == nullptr)
            m_mapped_memory = m_device->device().mapMemory(m_memory, 0, size);
        return *this;
    }

    Buffer &Buffer::unmap_memory()
    {
        if (m_mapped_memory != nullptr)
        {
            m_device->device().unmapMemory(m_memory);
            m_mapped_memory = nullptr;
        }
        return *this;
    }

    Buffer &Buffer::update(const void *data, vk::DeviceSize size)
    {
        map_memory(size);
        memcpy(m_mapped_memory, data, size);
        unmap_memory();
        return *this;
    }

    void Buffer::copy_buffer(const LogicalDevice &device, const CommandBuffer &command_buffer, vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size)

    {
        command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
            .copy_buffer(src_buffer, dst_buffer, size)
            .end()
            .submit_wait_idle(device.transfer_queue());
    }

    DeviceLocalBuffer::DeviceLocalBuffer(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, vk::BufferUsageFlags usage, vk::DeviceSize size, const void *data) : Buffer(device, size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal)
    {
        HostVisibleBuffer staging_buffer(device, vk::BufferUsageFlagBits::eTransferSrc, size, data);
        copy_buffer(*device, command_buffer, staging_buffer.buffer(), m_buffer, size);
    }

    HostVisibleBuffer::HostVisibleBuffer(gsl::not_null<const LogicalDevice *> device, vk::BufferUsageFlags usage, vk::DeviceSize size) : Buffer(device, size, usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) {}

    HostVisibleBuffer::HostVisibleBuffer(gsl::not_null<const LogicalDevice *> device, vk::BufferUsageFlags usage, vk::DeviceSize size, const void *data) : Buffer(device, size, usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    {
        update(data, size);
    }
}
