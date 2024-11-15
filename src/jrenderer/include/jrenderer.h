#pragma once

#include "details/imgui_context.h"
#include "jrenderer/graphics.h"
#include "jrenderer/window.h"
#include "jrenderer/render_set.h"
#include "jrenderer/mesh.h"
#include "jrenderer/model.h"
#include "jrenderer/camera/camera_controller.h"
#include "jrenderer/input_manager.h"
#include "jrenderer/star_rail_char_render_set.h"

namespace jre
{
    class JRenderer : public ITickable
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

        PmxModel model_lingsha;
        StarRailCharRenderSet star_rail_char_render_set;
        StarRailCharRenderSetRenderer star_rail_char_render_set_renderer;

        imgui::ImguiContext m_imgui_context;

        InputManager m_input_manager;

        Camera m_camera;
        CameraController m_camera_controller;

        void tick(const TickContext &context) override;

    public:
        JRenderer(Window &window);
        ~JRenderer();

        inline Graphics &graphics() { return m_graphics; }
        inline InputManager &input_manager() { return m_input_manager; }
        inline Camera &camera() { return m_camera; }

        void new_imgui_frame();
        void new_frame(const TickContext &context);
    };

}
