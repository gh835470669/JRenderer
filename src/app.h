#pragma once

#include "jrenderer.h"
#include "imgui_helper/imgui_context.h"
#include "imgui_helper/imgui_window/imwin_debug.h"
#include "window.h"
#include "statistics/statistics.h"

namespace jre
{
    class App
    {
    private:
        /* data */

    public:
        App(HINSTANCE hinst,
            HINSTANCE hprev,
            LPSTR cmdline,
            int show);
        ~App();

        Window window;

        int MainLoop();
        void Tick();
        void Draw();

    private:
        JRenderer m_renderer;
        Statistics m_statistics;
        imgui::ImguiContext imgui_context;
        imgui::ImWinDebug imwin_debug;
    };
}
