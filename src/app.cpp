#include "app.h"
#include "main_loop_context.h"

namespace jre
{

    App::App(HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int show)
        : window(hinst, hprev, cmdline, show),
          m_renderer(this->window.hinstance(), this->window.hwnd()),
          imgui(m_renderer.pipeline().physical_device(),
                m_renderer.pipeline().device(),
                m_renderer.pipeline().command_pool(),
                m_renderer.pipeline().render_pass(),
                m_renderer.pipeline().graphics_queue(),
                window,
                m_statistics)
    {
        // AllocConsole();
        // freopen("CONOUT$", "w", stdout);
        // FreeConsole();

        window.message_handlers.push_back(imgui);
    }

    App::~App()
    {
    }

    int App::MainLoop()
    {
        int running_msg = 0;
        while (!running_msg)
        {
            Tick();
            Draw();
            running_msg = window.ProcessMessage();
        }
        return running_msg;
    }

    void App::Tick()
    {
        TickContext tick_context = {0};
        m_statistics.Tick();
        imgui.Tick(tick_context);
        m_renderer.Tick(tick_context);
    }
    void App::Draw()
    {
        VulkanPipeline::VulkanPipelineDrawContext pipeline_draw_context = m_renderer.pipeline().BeginDraw();
        DrawContext draw_context =
            {
                pipeline_draw_context.command_buffer(),
            };
        m_renderer.Draw(draw_context);
        imgui.Draw(draw_context);
    }
}
