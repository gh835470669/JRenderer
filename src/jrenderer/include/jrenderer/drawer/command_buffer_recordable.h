#pragma once

#include <vulkan/vulkan.hpp>

namespace jre
{
    class Graphics;
    class CommandBufferRecordable
    {
    public:
        bool visible = true;
        virtual ~CommandBufferRecordable() = default;
        void draw(Graphics &graphics, vk::CommandBuffer command_buffer)
        {
            if (visible)
            {
                on_draw(graphics, command_buffer);
            }
        }

        virtual void on_draw(Graphics &graphics, vk::CommandBuffer command_buffer) = 0;
    };
}