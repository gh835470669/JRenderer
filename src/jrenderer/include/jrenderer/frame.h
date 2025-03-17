#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <gsl/pointers>
#include <vulkan/vulkan_shared.hpp>

namespace jre
{
    class CPUFrame
    {
    public:
        vk::SharedSemaphore image_available_semaphore;
        vk::SharedSemaphore render_finished_semaphore;
        vk::SharedFence in_flight_fence;
        vk::SharedCommandBuffer command_buffer;
    };

}
