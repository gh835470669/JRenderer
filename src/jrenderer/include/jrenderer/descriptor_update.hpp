#pragma once

#include <vulkan/vulkan_shared.hpp>
#include <vector>
#include <list>
#include <optional>
#include "jrenderer/buffer.h"

namespace jre
{
    class DescripterSetUpdater
    {
    public:
        vk::Device device;
        std::vector<vk::WriteDescriptorSet> descriptor_writes;
        std::list<vk::DescriptorImageInfo> descriptor_image_infos;
        std::list<vk::DescriptorBufferInfo> descriptor_buffer_infos;
        std::optional<vk::DescriptorSet> descriptor_set;

        DescripterSetUpdater(vk::Device device, std::vector<vk::WriteDescriptorSet> descriptor_writes = {})
            : device(device), descriptor_writes(descriptor_writes), descriptor_set() {}
        DescripterSetUpdater(vk::SharedDescriptorSet descriptor_set, std::vector<vk::WriteDescriptorSet> descriptor_writes = {})
            : device(descriptor_set.getDestructorType().get()), descriptor_writes(descriptor_writes), descriptor_set(descriptor_set.get()) {}

        DescripterSetUpdater &write_uniform_buffer(vk::DescriptorBufferInfo info, int binding_index = -1)
        {
            auto &tmp_info = descriptor_buffer_infos.emplace_back(info);
            descriptor_writes.emplace_back(
                vk::DescriptorSet(),
                binding_index == -1 ? descriptor_writes.size() : binding_index,
                0,
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                tmp_info);
            return *this;
        }

        template <typename ElementType, bool Padding>
        DescripterSetUpdater &write_uniform_buffer(const HostArrayBuffer<ElementType, Padding> &buffer, uint32_t index = 0, int binding_index = -1)
        {
            return write_uniform_buffer(vk::DescriptorBufferInfo(buffer.vk_buffer(), index * buffer.element_size(), buffer.element_size()), binding_index);
        }

        template <typename T, typename = std::enable_if<!std::is_same_v<T, void>>::value>
        DescripterSetUpdater &write_uniform_buffer(const Buffer<T> &buffer, int binding_index = -1)
        {
            return write_uniform_buffer(vk::DescriptorBufferInfo(buffer.vk_buffer(), 0, sizeof(T)), binding_index);
        }

        DescripterSetUpdater &write_combined_image_sampler(vk::Sampler sampler, vk::ImageView image_view, int binding_index = -1)
        {
            return write_combined_image_sampler(vk::DescriptorImageInfo(
                                                    sampler,
                                                    image_view,
                                                    vk::ImageLayout::eShaderReadOnlyOptimal),
                                                binding_index);
        }

        DescripterSetUpdater &write_combined_image_sampler(vk::DescriptorImageInfo info, int binding_index = -1)
        {
            auto &tmp_info = descriptor_image_infos.emplace_back(info);
            descriptor_writes.emplace_back(
                vk::DescriptorSet(),
                binding_index == -1 ? descriptor_writes.size() : binding_index,
                0,
                vk::DescriptorType::eCombinedImageSampler,
                tmp_info,
                nullptr);
            return *this;
        }

        DescripterSetUpdater &clear()
        {
            descriptor_writes.clear();
            descriptor_image_infos.clear();
            descriptor_buffer_infos.clear();
            return *this;
        }

        void update(std::optional<vk::DescriptorSet> descriptor_set_ = {})
        {
            vk::DescriptorSet set = descriptor_set.value_or(descriptor_set_.value_or(VK_NULL_HANDLE));
            for (auto &write : descriptor_writes)
            {
                write.setDstSet(set);
            }
            device.updateDescriptorSets(descriptor_writes, {});
        }
    };

}