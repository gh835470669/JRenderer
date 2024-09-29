#pragma once

#include "vulkan/vulkan.hpp"

namespace jre
{

    struct TickContext
    {
        float delta_time;
    };

    struct DrawContext
    {
        vk::CommandBuffer command_buffer;
    };

    class ITickable
    {
    public:
        virtual void tick(const TickContext &context) = 0;
    };

    class IDrawable
    {
    public:
        virtual void draw(const DrawContext &context) = 0;
    };

}