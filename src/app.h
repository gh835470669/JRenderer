#pragma once

#include "jrenderer.h"
#include "imgui_helper/imgui_helper.h"
#include "window.h"

namespace jre
{
private:
    /* data */
   
public:
    App(HINSTANCE hinst, HWND hwnd);
    ~App();

        Window window;

        int main_loop();

    private:
        JRenderer m_renderer;
    };
}
