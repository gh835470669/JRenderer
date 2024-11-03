#include "app.h"
#include "tracy/Tracy.hpp"
#include "details/main_loop_context.h"

App::App(HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int show)
    : window(hinst, hprev, cmdline, show),
      m_renderer(this->window),
      imwin_debug(m_statistics, m_renderer)
{

    // AllocConsole();
    // freopen("CONOUT$", "w", stdout);
    // FreeConsole();
}

App::~App()
{
}

int App::MainLoop()
{
    bool exit = false;
    while (!exit)
    {
        ZoneScoped;
        m_renderer.input_manager().input_manager().Update();

        MSG msg = window.ProcessMessage(std::bind(&gainput::InputManager::HandleMessage, &m_renderer.input_manager().input_manager(), std::placeholders::_1));
        if (msg.message == WM_QUIT)
        {
            exit = true;
        }

        m_statistics.tick();
        jre::TickContext tick_context;
        tick_context.delta_time = m_statistics.frame_counter_graph.frame_counter.mspf() / 1000.f;
        m_renderer.new_imgui_frame();
        imwin_debug.tick();

        m_renderer.new_frame(tick_context);

        FrameMark;
    }
    m_renderer.graphics().wait_idle(); // wait for the device to be idle before shutting down
    return exit;
}
