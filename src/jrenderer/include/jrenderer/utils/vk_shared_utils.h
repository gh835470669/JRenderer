#pragma once

#include <vector>
#include <vulkan/vulkan_shared.hpp>

namespace vk
{
    namespace shared
    {
        std::vector<vk::SharedCommandBuffer> allocate_command_buffers(vk::SharedCommandPool command_pool,
                                                                      uint32_t count = 1,
                                                                      vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

        vk::SharedCommandBuffer inline allocate_one_command_buffer(vk::SharedCommandPool command_pool,
                                                                   vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary)
        {
            return allocate_command_buffers(command_pool, 1, level).front();
        }

        std::tuple<vk::SharedDescriptorPool, vk::SharedDescriptorSetLayout> make_descriptor_pool_with_layout(
            vk::SharedDevice device,
            uint32_t max_sets,
            vk::ArrayProxy<vk::DescriptorSetLayoutBinding> bindings);

        std::vector<vk::SharedDescriptorSet> allocate_descriptor_sets(vk::SharedDescriptorPool descriptor_pool,
                                                                      const std::vector<vk::DescriptorSetLayout> &descriptor_set_layouts);

        vk::SharedDescriptorSet inline allocate_one_descriptor_set(vk::SharedDescriptorPool descriptor_pool,
                                                                   vk::DescriptorSetLayout descriptor_set_layout)
        {
            return allocate_descriptor_sets(descriptor_pool, {descriptor_set_layout}).front();
        }

        template <typename T>
        struct MapMemoryDeleter
        {
            vk::SharedDevice device;
            vk::SharedDeviceMemory memory;
            using pointer = T *;
            void operator()(pointer p)
            {
                if (p)
                {
                    device->unmapMemory(*memory);
                }
            }
        };

        template <typename T>
        using MapMemoryUniquePtr = std::unique_ptr<T, MapMemoryDeleter<T>>;

        template <typename T>
        MapMemoryUniquePtr<T> map_memory(
            vk::SharedDevice device,
            vk::SharedDeviceMemory memory,
            vk::DeviceSize offset,
            vk::DeviceSize size,
            vk::MemoryMapFlags flags = {})
        {
            return MapMemoryUniquePtr<T>(
                (T *)device->mapMemory(memory.get(), offset, size, flags),
                MapMemoryDeleter<T>{device, memory});
        }

        std::vector<char> load_spv_shader_file(const std::string &file_path);
        vk::SharedShaderModule create_shader_from_spv(vk::SharedDevice device,
                                                      const std::vector<char> &content);
        vk::SharedShaderModule create_shader_from_spv_file(vk::SharedDevice device,
                                                           const std::string &file_path);
        vk::SharedPipelineLayout create_pipeline_layout(vk::SharedDevice device,
                                                        vk::ArrayProxy<vk::DescriptorSetLayout> descriptor_set_layouts,
                                                        vk::ArrayProxy<vk::PushConstantRange> push_constant_ranges);
    }
} // namespace vk
