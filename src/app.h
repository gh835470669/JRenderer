#pragma once

#include "jrenderer.h"
#include "imgui_helper/imgui_helper.h"

class App
{
private:
    /* data */
   
public:
    App(HINSTANCE hinst, HWND hwnd);
    ~App();

    JRenderer renderer;
    VulkanExampleBase imgui_base;
    ImguiHelper *imgui;

    void main_loop();
};
