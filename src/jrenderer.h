#pragma once

#include "vulkan_pipeline.h"

class JRenderer
{
private:
    void tick();
    void draw();

public:
    JRenderer(HINSTANCE hinst, HWND hwnd) : pipeline(hinst, hwnd) {};
    ~JRenderer();

    VulkanPipeline pipeline;

    void main_loop();
};