#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include "jrenderer/uniform_buffer.h"

namespace jre
{
    class LogicalDevice;

    class DescriptorSetLayout
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        vk::DescriptorSetLayout m_descriptor_set_layout;

    public:
        DescriptorSetLayout() = default;
        DescriptorSetLayout(gsl::not_null<const LogicalDevice *> logical_device, const std::vector<vk::DescriptorSetLayoutBinding> &bindings);
        DescriptorSetLayout(const DescriptorSetLayout &) = delete;            // non-copyable
        DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete; // non-copyable
        DescriptorSetLayout(DescriptorSetLayout &&) = default;                // movable
        DescriptorSetLayout &operator=(DescriptorSetLayout &&) = default;     // movable
        ~DescriptorSetLayout();

        const vk::DescriptorSetLayout &descriptor_set_layout() const { return m_descriptor_set_layout; }
        operator vk::DescriptorSetLayout() const { return m_descriptor_set_layout; }
    };

    struct DescriptorPoolCreateInfo
    {
        uint32_t max_sets = 100;
        uint32_t max_sets_per_type = 100;
    };

    class DescriptorPool
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        vk::DescriptorPool m_descriptor_pool;

    public:
        DescriptorPool() = default;
        DescriptorPool(gsl::not_null<const LogicalDevice *> logical_device, DescriptorPoolCreateInfo create_info);
        DescriptorPool(const DescriptorPool &) = delete;            // non-copyable
        DescriptorPool &operator=(const DescriptorPool &) = delete; // non-copyable
        DescriptorPool(DescriptorPool &&) = default;                // movable
        DescriptorPool &operator=(DescriptorPool &&) = default;     // movable
        ~DescriptorPool();

        const vk::DescriptorPool &descriptor_pool() const { return m_descriptor_pool; }
        operator vk::DescriptorPool() const { return m_descriptor_pool; }
    };

    class Texture2D;
    class DescriptorSet
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        gsl::not_null<const DescriptorPool *> m_descriptor_pool;
        vk::DescriptorSet m_descriptor_set;
        vk::DescriptorSetLayout m_descriptor_set_layout;
        const std::vector<vk::DescriptorSetLayoutBinding> bindings;

    public:
        DescriptorSet() = default;
        DescriptorSet(gsl::not_null<const LogicalDevice *> logical_device, gsl::not_null<const DescriptorPool *> descriptor_pool, const std::vector<vk::DescriptorSetLayoutBinding> &bindings);
        DescriptorSet(const DescriptorSet &) = delete;            // non-copyable
        DescriptorSet &operator=(const DescriptorSet &) = delete; // non-copyable
        DescriptorSet(DescriptorSet &&) = default;                // movable
        DescriptorSet &operator=(DescriptorSet &&) = default;     // movable
        ~DescriptorSet();

        const vk::DescriptorSet &descriptor_set() const { return m_descriptor_set; }
        operator vk::DescriptorSet() const { return m_descriptor_set; }
        const vk::DescriptorSetLayout &descriptor_set_layout() const { return m_descriptor_set_layout; }

        template <typename UniformBufferObjectType = void>
        void update_descriptor_sets(std::vector<const UniformBuffer<UniformBufferObjectType> *> uniform_buffers = {}, std::vector<const Texture2D *> images = {})
        {
            auto uniform_vec_cur = uniform_buffers.begin();
            auto image_vec_cur = images.begin();
            std::vector<vk::WriteDescriptorSet> descriptor_writes;
            descriptor_writes.reserve(bindings.size());
            for (size_t i = 0; i < bindings.size(); ++i)
            {
                if (bindings[i].descriptorType == vk::DescriptorType::eUniformBuffer)
                {
                    vk::DescriptorBufferInfo buffer_info = (*uniform_vec_cur)->descriptor();
                    uniform_vec_cur++;
                    descriptor_writes.emplace_back(m_descriptor_set, static_cast<uint32_t>(i), 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &buffer_info, nullptr);
                }
                else if (bindings[i].descriptorType == vk::DescriptorType::eCombinedImageSampler)
                {
                    vk::DescriptorImageInfo image_info = (*image_vec_cur)->descriptor();
                    image_vec_cur++;
                    descriptor_writes.emplace_back(m_descriptor_set, static_cast<uint32_t>(i), 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info, nullptr);
                }
                else
                {
                    throw std::runtime_error("Unsupported descriptor type");
                }
            }
            m_device->device().updateDescriptorSets(descriptor_writes, {});
        }

        void update_descriptor_sets(std::vector<const Texture2D *> images);
    };

}