#pragma once

#include <vulkan/vulkan_shared.hpp>
#include "jrenderer/drawer/scene_drawer.h"

namespace jre
{
    Model load_lingsha(SceneDrawer &scene_drawer,
                       uint32_t frame_count,
                       vk::SharedDevice device,
                       vk::PhysicalDevice physical_device,
                       vk::SharedQueue transfer_queue,
                       vk::SharedCommandBuffer command_buffer);

}