#pragma once

#include "jrenderer.h"
// #include "imgui_helper/imgui_context.h"
#include "imgui_helper/imwin_debug.h"
// #include "window.h"
#include "app_statistic.h"

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

    jre::Window window;

    int MainLoop();

private:
    jre::JRenderer m_renderer;
    Statistics m_statistics;
    // jre::imgui::ImguiContext imgui_context;
    ImWinDebug imwin_debug;
};
