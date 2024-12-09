#include "jrenderer/descriptor.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/texture.h"
#include <ranges>

namespace jre
{
    DescriptorSetLayout::DescriptorSetLayout(gsl::not_null<const LogicalDevice *> logical_device, const std::vector<vk::DescriptorSetLayoutBinding> &bindings) : m_device(logical_device)
    {
        vk::DescriptorSetLayoutCreateInfo vk_create_info{};
        vk_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
        vk_create_info.pBindings = bindings.data();

        m_descriptor_set_layout = m_device->device().createDescriptorSetLayout(vk_create_info);
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        m_device->device().destroyDescriptorSetLayout(m_descriptor_set_layout);
    }

    DescriptorPool::DescriptorPool(gsl::not_null<const LogicalDevice *> logical_device, DescriptorPoolCreateInfo create_info) : m_device(logical_device)
    {
        vk::DescriptorPoolCreateInfo vk_create_info{};
        vk_create_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        vk_create_info.maxSets = create_info.max_sets;

        std::vector<vk::DescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(create_info.max_sets_per_type);
        pool_sizes.emplace_back(vk::DescriptorType::eUniformBuffer, create_info.max_sets);
        pool_sizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, create_info.max_sets);
        vk_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        vk_create_info.pPoolSizes = pool_sizes.data();

        m_descriptor_pool = m_device->device().createDescriptorPool(vk_create_info);
    }

    DescriptorPool::~DescriptorPool()
    {
        m_device->device().destroyDescriptorPool(m_descriptor_pool);
    }

    DescriptorSet::DescriptorSet(gsl::not_null<const LogicalDevice *> logical_device,
                                 gsl::not_null<const DescriptorPool *> descriptor_pool,
                                 const std::vector<vk::DescriptorSetLayoutBinding> &bindings) : m_device(logical_device), m_descriptor_pool(descriptor_pool), m_bindings(bindings)
    {
        vk::DescriptorSetLayoutCreateInfo vk_create_info{};
        vk_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
        vk_create_info.pBindings = bindings.data();

        m_descriptor_set_layout = m_device->device().createDescriptorSetLayout(vk_create_info);

        vk::DescriptorSetAllocateInfo alloc_info{};
        alloc_info.descriptorPool = descriptor_pool->descriptor_pool();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_set_layout;
        m_descriptor_set = m_device->device().allocateDescriptorSets(alloc_info)[0];
    }

    DescriptorSet::~DescriptorSet()
    {
        m_device->device().destroyDescriptorSetLayout(m_descriptor_set_layout);
        m_device->device().freeDescriptorSets(*m_descriptor_pool, m_descriptor_set);
    }

    void DescriptorSet::update_descriptor_sets(const std::vector<std::variant<vk::DescriptorBufferInfo, vk::DescriptorImageInfo>> &descriptor_buffers)
    {
        std::vector<vk::WriteDescriptorSet> descriptor_writes;
        auto binding_ranges = std::views::iota(0u, static_cast<uint32_t>(m_bindings.size()));
        std::transform(binding_ranges.begin(), binding_ranges.end(),
                       descriptor_buffers.begin(),
                       std::back_inserter(descriptor_writes),
                       [this](uint32_t binding_index, const auto &descriptor_buffer)
                       {
                           const vk::DescriptorSetLayoutBinding &binding = this->m_bindings.at(binding_index);
                           if (binding.descriptorType == vk::DescriptorType::eUniformBuffer)
                           {
                               return vk::WriteDescriptorSet(this->m_descriptor_set, binding_index, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &std::get<vk::DescriptorBufferInfo>(descriptor_buffer), nullptr);
                           }
                           else if (binding.descriptorType == vk::DescriptorType::eCombinedImageSampler)
                           {
                               return vk::WriteDescriptorSet(this->m_descriptor_set, binding_index, 0, 1, vk::DescriptorType::eCombinedImageSampler, &std::get<vk::DescriptorImageInfo>(descriptor_buffer), nullptr);
                           }
                           else
                           {
                               throw std::runtime_error("Unsupported descriptor type");
                           }

                           binding_index++;
                       });
        m_device->device().updateDescriptorSets(descriptor_writes, {});
    }
}
