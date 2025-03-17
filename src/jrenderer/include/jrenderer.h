#pragma once

#include "jrenderer/graphics.h"
#include "jrenderer/window.h"
#include "jrenderer/input_manager.h"
#include "jrenderer/ticker/scene_ticker.h"
#include "jrenderer/drawer/imgui_drawer.h"
#include "jrenderer/drawer/scene_drawer.h"

namespace jre
{
    namespace imgui
    {
        class ImguiDrawer;
        class ScopedFrame;
    }
    class SceneDrawer;
    class JRenderer : public Tickable, public Drawable
    {
    public:
        JRenderer(Window &window);

        inline Graphics &graphics() { return m_graphics; }

        void set_msaa(const vk::SampleCountFlagBits &msaa);

        std::unique_ptr<imgui::ScopedFrame> new_imgui_frame() { return std::make_unique<imgui::ScopedFrame>(); }
        void new_frame(TickContext context);

        imgui::ImguiDrawer &imgui_drawer() { return *m_imgui_drawer; }
        SceneDrawer &scene_drawer() { return *m_scene_drawer; }
        SceneTicker &scene_ticker() { return m_scene_ticker; }

    private:
        Window &m_window;
        Graphics m_graphics;
        std::shared_ptr<imgui::ImguiDrawer> m_imgui_drawer;
        std::shared_ptr<SceneDrawer> m_scene_drawer;
        SceneTicker m_scene_ticker;

        void on_tick(TickContext context) override;
        void on_draw() override;

        void on_resize(uint32_t width, uint32_t height);

        void add_tickers();
        void add_renderers();

    public:
        InputManager input_manager;
    };
}
