#pragma once

#include "details/vulkan_pipeline.h"
#include "details/vulkan_pipeline_builder.h"
#include "details/imgui_context.h"
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
        Window &m_window;
        RenderSetting m_render_setting;
        VulkanPipelineBuilder m_pipeline_builder;
        std::unique_ptr<VulkanPipeline> m_pipeline;
        imgui::ImguiContext m_imgui_context;

    public:
        JRenderer(Window &window);
        ~JRenderer();

        inline VulkanPipeline &pipeline() { return *m_pipeline; }
        inline const RenderSetting &render_setting() { return m_render_setting; }

        JRendererRebuilder GetRebuilder() { return JRendererRebuilder(*this); };

        void new_imgui_frame();
        void new_frame();

        void ReInitSwapChain();
    };
}
