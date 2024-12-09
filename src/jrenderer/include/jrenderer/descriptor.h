#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include "jrenderer/uniform_buffer.hpp"
#include "jrenderer/texture.h"
#include "jrenderer/logical_device.h"
#include <variant>

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

    class DescriptorSet
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        gsl::not_null<const DescriptorPool *> m_descriptor_pool;
        vk::DescriptorSet m_descriptor_set;
        vk::DescriptorSetLayout m_descriptor_set_layout;
        const std::vector<vk::DescriptorSetLayoutBinding> m_bindings;

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

        template <
            typename UniformBufferObjectType = jre::UniformMVP,
            typename UniformBufferIterator,
            typename Texture2DIterator,
            typename Texture2DIteratorValueType = typename std::iterator_traits<Texture2DIterator>::value_type,
            typename UniformBufferIteratorValueType = typename std::iterator_traits<UniformBufferIterator>::value_type,
            std::enable_if_t<
                std::is_same_v<std::remove_const_t<typename std::pointer_traits<UniformBufferIteratorValueType>::element_type>, typename jre::UniformBuffer<UniformBufferObjectType>> &&
                std::is_same_v<std::remove_const_t<typename std::pointer_traits<Texture2DIteratorValueType>::element_type>, Texture2D>> * = nullptr>
        void update_descriptor_sets(UniformBufferIterator uniform_buffers_begin,
                                    Texture2DIterator images_begin)
        {
            UniformBufferIterator uniform_vec_cur = uniform_buffers_begin;
            Texture2DIterator image_vec_cur = images_begin;
            std::vector<vk::WriteDescriptorSet> descriptor_writes;
            descriptor_writes.reserve(m_bindings.size());
            for (size_t i = 0; i < m_bindings.size(); ++i)
            {
                if (m_bindings[i].descriptorType == vk::DescriptorType::eUniformBuffer)
                {
                    vk::DescriptorBufferInfo buffer_info = (*uniform_vec_cur)->descriptor();
                    uniform_vec_cur++;
                    descriptor_writes.emplace_back(m_descriptor_set, static_cast<uint32_t>(i), 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &buffer_info, nullptr);
                }
                else if (m_bindings[i].descriptorType == vk::DescriptorType::eCombinedImageSampler)
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

        void update_descriptor_sets(const std::vector<std::variant<vk::DescriptorBufferInfo, vk::DescriptorImageInfo>> &descriptor_buffers);
    };

}