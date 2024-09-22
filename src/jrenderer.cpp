#include "jrenderer.h"
#include <iostream>

namespace jre
{
    JRenderer::JRenderer(Window &window) : m_window(window)
    {
        auto size = window.size();
        m_pipeline_builder.SetWindow(window.hinstance(), window.hwnd());
        m_pipeline_builder.swap_chain_builder.SetSize(std::get<0>(size), std::get<1>(size));
        m_pipeline_builder.swap_chain_builder.SetVsync(m_render_setting.vsync);
        m_pipeline = m_pipeline_builder.Build();
    }

    JRenderer::~JRenderer()
    {
        m_pipeline_builder.Destroy(m_pipeline);
    }

    void JRenderer::Tick(const TickContext &context)
    {
    }

    void JRenderer::Draw(const DrawContext &context)
    {
        this->m_pipeline.Draw();
    }
    void JRenderer::ReInitSwapChain()
    {
        auto size = m_window.size();
        m_pipeline_builder.swap_chain_builder.SetSize(std::get<0>(size), std::get<1>(size));
        m_pipeline.ReInitSwapChain(m_pipeline_builder.swap_chain_builder.Build(m_pipeline));
    }
    void JRendererRebuilder::setVsync(bool vsync)
    {
        if (m_renderer.m_render_setting.vsync == vsync)
            return;
        m_renderer.m_render_setting.vsync = vsync;
        m_renderer.m_pipeline_builder.swap_chain_builder.SetVsync(vsync);
        m_renderer.m_pipeline.ReInitSwapChain(m_renderer.m_pipeline_builder.swap_chain_builder.Build(m_renderer.m_pipeline));
    }
} // namespace jre
