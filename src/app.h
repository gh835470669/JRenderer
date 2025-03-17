#pragma once

#include "jrenderer.h"
#include "imgui_windows/imwin_debug.h"
#include "app_statistic.h"

class App
{
public:
    App(HINSTANCE hinst,
        HINSTANCE hprev,
        LPSTR cmdline,
        int show);

    jre::Window window;
    jre::JRenderer renderer;
    Statistics statistics;
    ImWinDebug imwin_debug;

    int main_loop();
};
