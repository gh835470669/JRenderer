#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan_utils/utils.hpp>
#include <gsl/pointers>
#include <ranges>
#include "jrenderer/utils/vk_utils.h"
#include "jrenderer/utils/vk_shared_utils.h"

namespace jre
{
    template <typename T>
    class Buffer
    {
    public:
        Buffer() = default;
        Buffer(vk::SharedBuffer buffer, vk::SharedDeviceMemory memory)
            : m_buffer(buffer), m_memory(memory), m_mapped_memory() {}

        vk::SharedBuffer buffer() { return m_buffer; }
        vk::Buffer vk_buffer() { return m_buffer.get(); }
        vk::SharedDeviceMemory memory() { return m_memory; }
        vk::DeviceSize size() const { return sizeof(T); }
        T &mapped_memory()
        {
            assert(m_mapped_memory);
            return *m_mapped_memory;
        }

        const T &mapped_memory() const
        {
            assert(m_mapped_memory);
            return *m_mapped_memory;
        }

        Buffer &map_memory()
        {
            if (!m_mapped_memory)
            {
                m_mapped_memory = vk::shared::map_memory<T>(m_buffer.getDestructorType(), m_memory, 0, sizeof(T));
            }
            return *this;
        }
        Buffer &unmap_memory()
        {
            m_mapped_memory.reset();
            return *this;
        }
        Buffer &update(const T &data)
        {
            map_memory();
            std::memcpy(m_mapped_memory.get(), &data, sizeof(T));
            return *this;
        }

        class Builder
        {
        public:
            vk::SharedDevice device;
            vk::PhysicalDevice physical_device;
            vk::BufferCreateInfo info;
            vk::MemoryPropertyFlags properties;
            Builder(vk::SharedDevice device,
                    vk::PhysicalDevice physical_device,
                    vk::BufferCreateInfo info = {},
                    vk::MemoryPropertyFlags properties = {})
                : device(device), physical_device(physical_device), info(info), properties(properties)
            {
                info.size = sizeof(T);
            }

            Builder &set_usage(vk::BufferUsageFlags usage)
            {
                info.setUsage(usage);
                return *this;
            }

            Buffer<T> build()
            {
                vk::Buffer buffer = device->createBuffer(info);
                vk::DeviceMemory memory = vk::su::allocateDeviceMemory(device.get(), physical_device.getMemoryProperties(), device->getBufferMemoryRequirements(buffer), properties);
                device->bindBufferMemory(buffer, memory, 0);
                return {vk::SharedBuffer(buffer, device), vk::SharedDeviceMemory(memory, device)};
            }

            Buffer<T> build(const T &data)
            {
                return std::move(build().update(data));
            }
        };

    protected:
        vk::SharedBuffer m_buffer;
        vk::SharedDeviceMemory m_memory;
        vk::shared::MapMemoryUniquePtr<T> m_mapped_memory;
    };

    template <typename T>
    using BufferBuilder = Buffer<T>::Builder;

    template <>
    class Buffer<void>
    {
    public:
        Buffer() : m_buffer(), m_memory(), m_mapped_memory(), m_size(0) {};
        Buffer(vk::SharedBuffer buffer, vk::SharedDeviceMemory memory, vk::DeviceSize size)
            : m_buffer(buffer), m_memory(memory), m_mapped_memory(), m_size(size) {}

        vk::SharedBuffer buffer() { return m_buffer; }
        const vk::Buffer vk_buffer() const { return m_buffer.get(); }
        vk::Buffer vk_buffer() { return m_buffer.get(); }
        vk::SharedDeviceMemory memory() { return m_memory; }
        vk::DeviceSize size() const { return m_size; }

        Buffer &map_memory()
        {
            if (!m_mapped_memory)
            {
                m_mapped_memory = vk::shared::map_memory<void>(m_buffer.getDestructorType(), m_memory, 0, m_size);
            }
            return *this;
        }
        Buffer &unmap_memory()
        {
            m_mapped_memory.reset();
            return *this;
        }
        Buffer &update(const void *const data, size_t size)
        {
            assert(size <= m_size);
            map_memory();
            std::memcpy(m_mapped_memory.get(), data, size);
            return *this;
        }

        void *mapped_memory()
        {
            map_memory();
            return m_mapped_memory.get();
        }

        const void *mapped_memory() const
        {
            assert(m_mapped_memory);
            return m_mapped_memory.get();
        }

        class Builder
        {
        public:
            vk::SharedDevice device;
            vk::PhysicalDevice physical_device;
            vk::BufferCreateInfo info;
            vk::MemoryPropertyFlags properties;
            Builder(vk::SharedDevice device,
                    vk::PhysicalDevice physical_device,
                    vk::BufferCreateInfo info = {},
                    vk::MemoryPropertyFlags properties = {})
                : device(device), physical_device(physical_device), info(info), properties(properties)
            {
            }

            Builder &set_usage(vk::BufferUsageFlags usage)
            {
                info.setUsage(usage);
                return *this;
            }

            Buffer<void> build()
            {
                vk::Buffer buffer = device->createBuffer(info);
                vk::DeviceMemory memory = vk::su::allocateDeviceMemory(device.get(), physical_device.getMemoryProperties(), device->getBufferMemoryRequirements(buffer), properties);
                device->bindBufferMemory(buffer, memory, 0);
                return {vk::SharedBuffer(buffer, device), vk::SharedDeviceMemory(memory, device), info.size};
            }

            Buffer<void> build(const void *const data, size_t size)
            {
                return std::move(build().update(data, size));
            }
        };

    protected:
        vk::SharedBuffer m_buffer;
        vk::SharedDeviceMemory m_memory;
        vk::shared::MapMemoryUniquePtr<void> m_mapped_memory;
        vk::DeviceSize m_size;
    };

    using DynamicBuffer = Buffer<void>;

    template <typename T>
    class HostVisibleBufferBuilder : public BufferBuilder<T>
    {
    public:
        HostVisibleBufferBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device)
            : BufferBuilder<T>(device, physical_device, vk::BufferCreateInfo().setSize(sizeof(T)), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        {
        }

        HostVisibleBufferBuilder &set_usage(vk::BufferUsageFlags usage)
        {
            this->info.setUsage(usage);
            return *this;
        }
    };

    template <>
    class HostVisibleBufferBuilder<void> : public BufferBuilder<void>
    {
    public:
        HostVisibleBufferBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::DeviceSize size)
            : BufferBuilder<void>(device, physical_device, vk::BufferCreateInfo().setSize(size), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        {
        }
    };

    using HostVisibleDynamicBufferBuilder = HostVisibleBufferBuilder<void>;

    template <typename T>
    class DeviceLocalBufferBuilder : public BufferBuilder<T>
    {
    public:
        vk::CommandBuffer command_buffer;
        vk::Queue transfer_queue;
        DeviceLocalBufferBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::CommandBuffer command_buffer, vk::Queue transfer_queue)
            : BufferBuilder<T>(device, physical_device, {}, vk::MemoryPropertyFlagBits::eDeviceLocal), command_buffer(command_buffer), transfer_queue(transfer_queue)
        {
            this->info.setUsage(this->info.usage | vk::BufferUsageFlagBits::eTransferDst);
        }

        DeviceLocalBufferBuilder &set_usage(vk::BufferUsageFlags usage)
        {
            this->info.setUsage(usage);
            return *this;
        }

        Buffer<T> build(const T &data)
        {
            Buffer<T> staging_buffer = HostVisibleBufferBuilder<T>(this->device, this->physical_device).set_usage(vk::BufferUsageFlagBits::eTransferSrc).build(data);
            this->info.usage = this->info.usage | vk::BufferUsageFlagBits::eTransferDst;
            vk::Buffer buffer = this->device->createBuffer(this->info);
            vk::DeviceMemory memory = vk::su::allocateDeviceMemory(this->device.get(), this->physical_device.getMemoryProperties(), this->device->getBufferMemoryRequirements(buffer), this->properties);
            this->device->bindBufferMemory(buffer, memory, 0);
            copy_buffer_to_buffer(command_buffer, transfer_queue, staging_buffer.buffer(), buffer, sizeof(T));
            return {vk::SharedBuffer(buffer, this->device), vk::SharedDeviceMemory(memory, this->device)};
        }
    };

    template <>
    class DeviceLocalBufferBuilder<void> : public BufferBuilder<void>
    {
    public:
        vk::CommandBuffer command_buffer;
        vk::Queue transfer_queue;
        DeviceLocalBufferBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::CommandBuffer command_buffer, vk::Queue transfer_queue, vk::DeviceSize size)
            : BufferBuilder<void>(device, physical_device, vk::BufferCreateInfo().setSize(size), vk::MemoryPropertyFlagBits::eDeviceLocal), command_buffer(command_buffer), transfer_queue(transfer_queue)
        {
            this->info.setUsage(this->info.usage | vk::BufferUsageFlagBits::eTransferDst);
        }

        DeviceLocalBufferBuilder<void> &set_usage(vk::BufferUsageFlags usage)
        {
            this->info.setUsage(usage);
            return *this;
        }

        Buffer<void> build(const void *const data, size_t size)
        {
            Buffer<void> staging_buffer = HostVisibleBufferBuilder<void>(this->device, this->physical_device, info.size).set_usage(vk::BufferUsageFlagBits::eTransferSrc).build(data, size);
            this->info.usage = this->info.usage | vk::BufferUsageFlagBits::eTransferDst;
            vk::Buffer buffer = this->device->createBuffer(this->info);
            vk::DeviceMemory memory = vk::su::allocateDeviceMemory(this->device.get(), this->physical_device.getMemoryProperties(), this->device->getBufferMemoryRequirements(buffer), this->properties);
            this->device->bindBufferMemory(buffer, memory, 0);
            copy_buffer_to_buffer(command_buffer, transfer_queue, staging_buffer.buffer().get(), buffer, info.size);
            return {vk::SharedBuffer(buffer, this->device), vk::SharedDeviceMemory(memory, this->device), info.size};
        }
    };

    using DeviceLocalDynamicBufferBuilder = DeviceLocalBufferBuilder<void>;

    template <typename ElementType, bool Padding = false>
    class HostArrayBuffer : public DynamicBuffer
    {
    public:
        uint32_t count() { return static_cast<uint32_t>(this->m_size / sizeof(ElementType)); }
        void update(const std::vector<ElementType> &data) { DynamicBuffer::update(data.data(), data.size() * sizeof(ElementType)); }
        ElementType &operator[](uint32_t index) { return *(reinterpret_cast<ElementType *>(this->mapped_memory()) + index); }
        const ElementType &operator[](uint32_t index) const { return *(reinterpret_cast<const ElementType *>(this->mapped_memory()) + index); }

        class Builder : public HostVisibleDynamicBufferBuilder
        {
        public:
            vk::ArrayProxy<ElementType> data;
            Builder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::DeviceSize size)
                : HostVisibleDynamicBufferBuilder(device, physical_device, size), data()
            {
            }

            Builder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::ArrayProxy<ElementType> data)
                : HostVisibleDynamicBufferBuilder(device, physical_device, data.size() * sizeof(ElementType)),
                  data(data)
            {
            }

            Builder &set_usage(vk::BufferUsageFlags usage)
            {
                HostVisibleDynamicBufferBuilder::set_usage(usage);
                return *this;
            }

            HostArrayBuffer build()
            {
                return HostArrayBuffer<ElementType>(HostVisibleDynamicBufferBuilder::build(data.data(), data.size() * sizeof(ElementType)));
            }

            HostArrayBuffer build(vk::ArrayProxy<ElementType> data_)
            {
                return HostArrayBuffer<ElementType>(HostVisibleDynamicBufferBuilder::build(data_.data(), data_.size() * sizeof(ElementType)));
            }
        };
    };

    template <typename ElementType>
    class HostArrayBuffer<ElementType, true> : public DynamicBuffer
    {
    public:
        uint32_t padding = 0;
        uint32_t count() const { return static_cast<uint32_t>(this->m_size / (sizeof(ElementType) + padding)); }
        uint32_t element_size() const { return sizeof(ElementType) + padding; }

        HostArrayBuffer() : DynamicBuffer(), padding(0) {}
        HostArrayBuffer(DynamicBuffer &&buffer, uint32_t padding) : DynamicBuffer(std::move(buffer)), padding(padding) {}

        template <std::ranges::input_range Range>
        HostArrayBuffer<ElementType, true> &update(const Range &data)
        {
            map_memory();
            for (const auto [index, element] : data | std::views::enumerate)
            {
                *(reinterpret_cast<ElementType *>(get_element_address(index))) = element;
            }
            return *this;
        }
        ElementType &operator[](size_t index) { return *reinterpret_cast<ElementType *>(get_element_address(index)); }
        const ElementType &operator[](size_t index) const { return *reinterpret_cast<ElementType *>(get_element_address(index)); }

        class Builder : public HostVisibleDynamicBufferBuilder
        {
        public:
            static uint32_t get_padded_element_size(vk::PhysicalDevice physical_device)
            {
                vk::DeviceSize min_align = physical_device.getProperties().limits.minUniformBufferOffsetAlignment;
                return static_cast<uint32_t>(std::ceil(static_cast<float>(sizeof(ElementType)) / min_align)) * min_align;
            }

            uint32_t padding = 0;
            vk::ArrayProxy<ElementType> data;
            Builder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::DeviceSize size)
                : HostVisibleDynamicBufferBuilder(device, physical_device, size), data(), padding(get_padded_element_size(physical_device) - sizeof(ElementType))
            {
            }

            Builder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::ArrayProxy<ElementType> data)
                : HostVisibleDynamicBufferBuilder(device, physical_device, data.size() * get_padded_element_size(physical_device)),
                  data(data), padding(get_padded_element_size(physical_device) - sizeof(ElementType))
            {
            }

            Builder &set_usage(vk::BufferUsageFlags usage)
            {
                HostVisibleDynamicBufferBuilder::set_usage(usage);
                return *this;
            }

            HostArrayBuffer<ElementType, true> build()
            {
                return std::move(HostArrayBuffer<ElementType, true>(HostVisibleDynamicBufferBuilder::build(), padding).update(data));
            }

            HostArrayBuffer build(vk::ArrayProxy<ElementType> data_)
            {
                return std::move(HostArrayBuffer<ElementType, true>(HostVisibleDynamicBufferBuilder::build(), padding).update(data_));
            }
        };

    private:
        void *get_element_address(size_t index)
        {
            return reinterpret_cast<std::byte *>(this->mapped_memory()) + index * (sizeof(ElementType) + padding);
        }
    };

    template <typename ElementType, bool Padding = false>
    using HostArrayBufferBuilder = HostArrayBuffer<ElementType, Padding>::Builder;

    template <typename ElementType>
    class DeviceArrayBuffer : public DynamicBuffer
    {
    public:
        uint32_t count() { return static_cast<uint32_t>(this->m_size / sizeof(ElementType)); }
        void update(const std::vector<ElementType> &data) { DynamicBuffer::update(data.data(), data.size() * sizeof(ElementType)); }

        class Builder : public DeviceLocalDynamicBufferBuilder
        {
        public:
            vk::ArrayProxy<ElementType> data;
            Builder(vk::SharedDevice device, vk::PhysicalDevice physical_device, vk::CommandBuffer command_buffer, vk::Queue transfer_queue, vk::ArrayProxy<ElementType> data)
                : DeviceLocalDynamicBufferBuilder(device, physical_device, command_buffer, transfer_queue, data.size() * sizeof(ElementType)),
                  data(data)
            {
            }

            Builder &set_usage(vk::BufferUsageFlags usage)
            {
                DeviceLocalDynamicBufferBuilder::set_usage(usage);
                return *this;
            }

            DeviceArrayBuffer build()
            {
                return DeviceArrayBuffer(DeviceLocalDynamicBufferBuilder::build(data.data(), data.size() * sizeof(ElementType)));
            }
        };
    };

    template <typename ElementType>
    using DeviceArrayBufferBuilder = DeviceArrayBuffer<ElementType>::Builder;

    template <typename T>
    class UniformBufferBuilder : public HostArrayBufferBuilder<T, true>
    {
    public:
        UniformBufferBuilder(vk::SharedDevice device, vk::PhysicalDevice physical_device, uint32_t frame_count = 1)
            : HostArrayBufferBuilder<T, true>(device, physical_device, frame_count * HostArrayBufferBuilder<T, true>::get_padded_element_size(physical_device))
        {
            this->info.setUsage(this->info.usage | vk::BufferUsageFlagBits::eUniformBuffer);
        }

        HostArrayBuffer<T, true> build()
        {
            return std::move(HostArrayBufferBuilder<T, true>::build(std::vector<T>(this->info.size / HostArrayBufferBuilder<T, true>::get_padded_element_size(this->physical_device))));
        }

        HostArrayBuffer<T> build(const T &data)
        {
            return std::move(HostArrayBufferBuilder<T, true>::build(std::vector<T>(this->info.size / HostArrayBufferBuilder<T, true>::get_padded_element_size(this->physical_device), data)));
        }
    };
}