#pragma once

#include "details/vulkan_pipeline.h"
#include "details/vulkan_pipeline_builder.h"
#include "details/imgui_context.h"
#include "details/vulkan_vertex.h"
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

    class JRenderer : IDrawable
    {
        friend class JRendererRebuilder;

    private:
        Window &m_window;
        RenderSetting m_render_setting;
        VulkanPipelineBuilder m_pipeline_builder;
        std::unique_ptr<VulkanPipeline> m_pipeline;
        imgui::ImguiContext m_imgui_context;

        // 坐标系https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules#page_Vertex-shader
        const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                              {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                              {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                              {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
        std::shared_ptr<VulkanBufferHandle> m_vertex_buffer;
        const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0};
        std::shared_ptr<VulkanBufferHandle> m_index_buffer;

        void draw(const DrawContext &draw_context) override;
        void copy_buffer(const VulkanBufferHandle &src_buffer, const VulkanBufferHandle &dst_buffer, size_t size);

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
