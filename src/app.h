#pragma once

#include "jrenderer.h"

class App
{
private:
    /* data */
    JRenderer m_renderer;
public:
    App(HINSTANCE hinst, HWND hwnd) : m_renderer(hinst, hwnd) {}
    ~App() = default;

    void main_loop();
};
