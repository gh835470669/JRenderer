#include "jrenderer.h"

namespace jre
{
    JRenderer::JRenderer(Window &window) : m_window(window),
                                           m_graphics(&window),
                                           m_imgui_drawer(std::make_shared<imgui::ImguiDrawer>(m_window, m_graphics)),
                                           m_scene_drawer(std::make_shared<SceneDrawer>(m_graphics))
    {
        m_window.message_handlers.push_back(std::bind(&imgui::ImguiDrawer::WindowProc, m_imgui_drawer.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        m_graphics.resize_funcs.push_back(std::bind(&imgui::ImguiDrawer::on_resize, m_imgui_drawer.get(), std::placeholders::_1, std::placeholders::_2));
        m_graphics.resize_funcs.push_back(std::bind(&JRenderer::on_resize, this, std::placeholders::_1, std::placeholders::_2));
        input_manager.input_manager().SetDisplaySize(m_window.width(), m_window.height());
        add_tickers();
        add_renderers();
    }

    void JRenderer::set_msaa(const vk::SampleCountFlagBits &msaa)
    {
        m_graphics.set_msaa(msaa);
        add_renderers();
        m_scene_drawer->on_set_msaa(m_graphics);
        m_imgui_drawer->on_set_msaa(m_graphics);
    }

    void JRenderer::new_frame(TickContext context)
    {
        tick(context);
        draw();
    }

    void JRenderer::on_tick(TickContext context)
    {
        SceneTickContext scene_tick_context{
            context.delta_time,
            m_graphics.current_cpu_frame(),
            m_scene_drawer->scene};
        for (auto &ticker : m_scene_ticker.tickers)
        {
            ticker->tick(scene_tick_context);
        }
    }

    void JRenderer::on_draw()
    {
        m_graphics.draw();
    }

    void JRenderer::on_resize(uint32_t width, uint32_t height)
    {
        input_manager.input_manager().SetDisplaySize(width, height);
        m_scene_drawer->scene.render_viewports[0].viewport = vk::Viewport{0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
    }

    void JRenderer::add_tickers()
    {
        m_scene_ticker.tickers.push_back(std::make_shared<SceneUBOTicker>());
    }

    void JRenderer::add_renderers()
    {
        m_graphics.render_pass_drawer().subpass_drawers[0].recorders.push_back(m_scene_drawer);
        m_graphics.render_pass_drawer().subpass_drawers[0].recorders.push_back(m_imgui_drawer);
    }

} // namespace jre
