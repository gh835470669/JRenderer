#pragma once

#include <vulkan/vulkan.hpp>
#include "jrenderer/camera/camera.h"
#include <optional>

namespace jre
{
    struct RenderViewport
    {
        vk::Viewport viewport;
        std::optional<vk::Rect2D> scissor;
        Camera camera;
        glm::mat4 projection;
    };
}