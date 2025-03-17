#pragma once

#include <vulkan/vulkan_shared.hpp>
#include "jrenderer/drawer/command_buffer_recordable.h"

namespace jre
{
    class RenderSubpassDrawable : public CommandBufferRecordable
    {
    public:
        virtual ~RenderSubpassDrawable() = default;
        virtual void on_set_msaa(Graphics &graphics) = 0;
        virtual void on_draw(Graphics &graphics, vk::CommandBuffer command_buffer) override = 0;
    };

    class RenderSubpassDrawers
    {
    public:
        std::vector<std::shared_ptr<CommandBufferRecordable>> recorders;
    };

    class Graphics;
    class RenderPassDrawers
    {
    public:
        vk::SharedRenderPass render_pass;
        std::vector<RenderSubpassDrawers> subpass_drawers;
        void draw(Graphics &graphics, vk::CommandBuffer command_buffer, vk::SharedFramebuffer frame_buffer, vk::ArrayProxy<vk::ClearValue> clear_values, vk::Rect2D render_area)
        {
            command_buffer.beginRenderPass(vk::RenderPassBeginInfo(render_pass.get(), frame_buffer.get(), render_area, clear_values), vk::SubpassContents::eInline);
            for (auto &subpass_drawer : subpass_drawers)
            {
                for (auto &recorder : subpass_drawer.recorders)
                {
                    recorder->draw(graphics, command_buffer);
                }
            }
            command_buffer.endRenderPass();
        }
    };
}