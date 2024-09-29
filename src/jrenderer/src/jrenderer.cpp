#include "jrenderer.h"
#include <iostream>
#include "tracy/Tracy.hpp"

namespace jre
{
    JRenderer::JRenderer(Window &window) : m_window(window),
                                           m_pipeline_builder(window.hinstance(), window.hwnd(),
                                                              VulkanPipelineSwapChainBuilder(
                                                                  std::get<0>(m_window.size()),
                                                                  std::get<1>(m_window.size()),
                                                                  m_render_setting.vsync)),
                                           m_pipeline(m_pipeline_builder.Build()),
                                           m_imgui_context(*m_pipeline, window)
    {
        m_window.message_handlers.push_back(m_imgui_context);
    }

    JRenderer::~JRenderer()
    {
    }

    void JRenderer::new_imgui_frame()
    {
        m_imgui_context.new_frame();
    }

    void JRenderer::new_frame()
    {
        // m_imgui_context.draw();
        try
        {
            ZoneScoped;
            jre::VulkanPipeline::VulkanPipelineDrawContext pipeline_draw_context = m_pipeline->BeginDraw();
            jre::DrawContext draw_context =
                {
                    pipeline_draw_context.command_buffer(),
                };
            m_pipeline->Draw();
            m_imgui_context.draw(draw_context);
        }
        catch (vk::OutOfDateKHRError)
        {
            // 当窗口的大小变化的时候，这个异常会爆出来
            // 一般是presentKHR爆出来的
            // C接口的话是判断presentKHR的返回值，https://github.com/KhronosGroup/Vulkan-Hpp/issues/274 这里会说，我自己创造性的用捕捉异常的方式解决这个问题，看来我C++理解的还是不错><
            // 处理参考代码https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation#page_Recreating-the-swap-chain
            // 这个重新创建需要在presentKHR之后，为的是保证m_render_finished_semaphore信号量被signal了
            // 如果需要显式的改变分辨率，就用触发变量 https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation#page_Handling-resizes-explicitly
            ReInitSwapChain();
            // present error
            // VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering. Usually happens after a 【window resize】.
            // VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly. 这个还没试过，如果这个出现也是ReInitSwapChain
        }
    }
    void JRenderer::ReInitSwapChain()
    {
        auto size = m_window.size();
        m_pipeline_builder.swap_chain_builder.SetSize(std::get<0>(size), std::get<1>(size));
        m_pipeline->ReInitSwapChain(m_pipeline_builder.swap_chain_builder.Build(*m_pipeline));
    }

    void JRendererRebuilder::setVsync(bool vsync)
    {
        if (m_renderer.m_render_setting.vsync == vsync)
            return;
        m_renderer.m_render_setting.vsync = vsync;
        m_renderer.m_pipeline_builder.swap_chain_builder.SetVsync(vsync);
        m_renderer.m_pipeline->ReInitSwapChain(m_renderer.m_pipeline_builder.swap_chain_builder.Build(*m_renderer.m_pipeline));
    }
} // namespace jre
