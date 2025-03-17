#pragma once

#include <vulkan/vulkan_shared.hpp>
#include "jrenderer/concrete_uniform_buffers.h"
#include "jrenderer/utils/vk_shared_utils.h"
#include "jrenderer/buffer.h"
#include "jrenderer/descriptor_update.hpp"

namespace jre
{

    class ModelTransform
    {
    public:
        std::vector<vk::SharedDescriptorSet> descriptor_sets;

        ModelTransform(HostArrayBuffer<UniformPerObject> &&uniform_buffer, std::vector<vk::SharedDescriptorSet> &&descriptor_sets)
            : m_uniform_buffer(std::move(uniform_buffer)), descriptor_sets(descriptor_sets) {}

        const HostArrayBuffer<UniformPerObject> &uniform_buffer() const { return m_uniform_buffer; }
        const UniformPerObject ubo(uint32_t cur_frame = 0) const { return m_uniform_buffer[cur_frame]; }
        void set_ubo(const UniformPerObject &ubo, uint32_t cur_frame = 0) { m_uniform_buffer[cur_frame] = ubo; }
        const glm::mat4 model(uint32_t cur_frame = 0) const { return m_uniform_buffer[cur_frame].mvp.model; }
        void set_model(glm::mat4 model, uint32_t cur_frame) { m_uniform_buffer[cur_frame].mvp.model = model; }
        void set_model(glm::mat4 model)
        {
            for (uint32_t i = 0; i < m_uniform_buffer.count(); ++i)
            {
                m_uniform_buffer[i].mvp.model = model;
            }
        }

    private:
        HostArrayBuffer<UniformPerObject> m_uniform_buffer;
    };

    class ModelTransformFactory
    {
    public:
        vk::PhysicalDevice physical_device;
        vk::SharedDescriptorPool descriptor_pool;
        vk::SharedDescriptorSetLayout descriptor_set_layout;
        uint32_t frame_count = 1;

        ModelTransformFactory() = default;
        ModelTransformFactory(vk::SharedDevice device, vk::PhysicalDevice physical_device, uint32_t frame_count = 1, uint32_t max_sets = 100);

        vk::SharedDescriptorSet create_descriptor_set() { return vk::shared::allocate_descriptor_sets(
                                                                     descriptor_pool,
                                                                     {descriptor_set_layout.get()})
                                                              .front(); }
        ModelTransform create_transform()
        {
            ModelTransform transform(HostArrayBufferBuilder<UniformPerObject>(
                                         descriptor_pool.getDestructorType(),
                                         physical_device, std::array<UniformPerObject, 2>{})
                                         .set_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                                         .build(),
                                     vk::shared::allocate_descriptor_sets(
                                         descriptor_pool,
                                         std::vector<vk::DescriptorSetLayout>(frame_count, descriptor_set_layout.get())));
            for (uint32_t i = 0; i < frame_count; ++i)
            {
                DescripterSetUpdater(transform.descriptor_sets[i])
                    .write_uniform_buffer(vk::DescriptorBufferInfo{transform.uniform_buffer().vk_buffer(), sizeof(UniformPerObject) * i, sizeof(UniformPerObject)})
                    .update();
            }
            return transform;
        }
    };
}