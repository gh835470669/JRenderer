#pragma once

#include "vulkan_pipeline.h"
#include "main_loop_context.h"

namespace jre
{
    class JRenderer
    {
    private:
        VulkanPipeline m_pipeline;

    public:
        JRenderer(HINSTANCE hinst, HWND hwnd) : m_pipeline(hinst, hwnd) {};
        ~JRenderer();

        inline VulkanPipeline &pipeline() { return m_pipeline; }

        void Tick(const TickContext &context);
        void Draw(const DrawContext &context);
    };
}
