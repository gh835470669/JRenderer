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

    public:
        JRenderer(Window &window);
        ~JRenderer();

        inline Graphics &graphics() { return m_graphics; }

        void set_msaa(const vk::SampleCountFlagBits &msaa);

        void new_imgui_frame();
        void new_frame(const TickContext &context);

    private:
        Window &m_window;
        // VulkanPipelineBuilder m_pipeline_builder;
        // std::unique_ptr<VulkanPipeline> m_pipeline;

        Graphics m_graphics;

        imgui::ImguiContext m_imgui_context;

        void tick(const TickContext &context) override;

    public:
        PmxModel model_lingsha;
        StarRailCharRenderSet star_rail_char_render_set;
        StarRailCharRenderSetRenderer star_rail_char_render_set_renderer;
        static PmxModel load_lingsha(const Graphics &graphics, const CommandBuffer &command_buffer, bool use_mipmaps = true);

        InputManager input_manager;

        Camera camera;
        CameraController camera_controller;
    };

}
