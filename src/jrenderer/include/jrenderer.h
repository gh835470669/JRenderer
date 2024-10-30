#pragma once

#include "details/imgui_context.h"
#include "jrenderer/graphics.h"
#include "jrenderer/window.h"
#include "jrenderer/render_set.h"
#include "jrenderer/mesh.h"
#include "jrenderer/model.h"

namespace jre
{
    class JRenderer
    {
        friend class JRendererRebuilder;

    private:
        Window &m_window;
        // VulkanPipelineBuilder m_pipeline_builder;
        // std::unique_ptr<VulkanPipeline> m_pipeline;

        Graphics m_graphics;

        MeshResources<> m_res_meshes;
        TextureResources m_res_textures;

        Model model;
        Model model2;

        DefaultRenderSet render_set;
        DefaultRenderSetRenderer render_set_renderer;

        imgui::ImguiContext m_imgui_context;

        void tick();

    public:
        JRenderer(Window &window);
        ~JRenderer();

        inline Graphics &graphics() { return m_graphics; }

        void new_imgui_frame();
        void new_frame();
    };

}
