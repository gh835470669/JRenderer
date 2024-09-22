#pragma once

#include "vulkan_pipeline.h"
#include "vulkan_pipeline_builder.h"
#include "main_loop_context.h"
#include "window.h"

namespace jre
{
    class RenderSetting
    {
    public:
        bool vsync = false;
    };

    class JRenderer;
    class JRendererRebuilder
    {
    public:
        JRendererRebuilder(JRenderer &renderer) : m_renderer(renderer) {};
        ~JRendererRebuilder() = default;

        void setVsync(bool vsync);

    private:
        JRenderer &m_renderer;
    };

    class JRenderer
    {
        friend class JRendererRebuilder;

    private:
        VulkanPipeline m_pipeline;
        VulkanPipelineBuilder m_pipeline_builder;
        Window &m_window;
        RenderSetting m_render_setting;

    public:
        JRenderer(Window &window);
        ~JRenderer();

        inline VulkanPipeline &pipeline() { return m_pipeline; }
        inline const RenderSetting &render_setting() { return m_render_setting; }

        JRendererRebuilder GetRebuilder() { return JRendererRebuilder(*this); };

        void Tick(const TickContext &context);
        void Draw(const DrawContext &context);

        void ReInitSwapChain();
    };
}
