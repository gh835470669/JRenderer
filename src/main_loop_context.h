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
        virtual void Tick(const TickContext &context) = 0;
    };

    class IDrawable
    {
    public:
        virtual void Draw(const DrawContext &context) = 0;
    };

}