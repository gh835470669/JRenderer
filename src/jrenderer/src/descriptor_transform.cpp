#include "jrenderer/descriptor_transform.h"
#include <array>
#include "jrenderer/utils/vk_shared_utils.h"

namespace jre
{
    ModelTransformFactory::ModelTransformFactory(vk::SharedDevice device, vk::PhysicalDevice physical_device, uint32_t frame_count, uint32_t max_sets)
        : physical_device(physical_device), frame_count(frame_count)
    {
        std::tie(descriptor_pool, descriptor_set_layout) = vk::shared::make_descriptor_pool_with_layout(
            device, max_sets, {{{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}});
    }
}
