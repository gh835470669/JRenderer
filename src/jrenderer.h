#pragma once

#include "vulkan_pipeline.h"

class JRenderer
{
private:
    VulkanPipeline pipeline;

    void tick();
    void draw();

public:
    JRenderer(HINSTANCE hinst, HWND hwnd) : pipeline(hinst, hwnd) {};
    ~JRenderer();

    void main_loop();
};