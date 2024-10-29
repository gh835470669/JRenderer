#include "app.h"
#include "tracy/Tracy.hpp"

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
    int running_msg = 0;
    while (!running_msg)
    {
        ZoneScoped;
        m_statistics.tick();
        m_renderer.new_imgui_frame();
        imwin_debug.tick();

        m_renderer.new_frame();
        running_msg = window.ProcessMessage();
        FrameMark;
    }
    m_renderer.graphics().wait_idle(); // wait for the device to be idle before shutting down
    return running_msg;
}
