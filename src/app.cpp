#include "app.h"
#include "main_loop_context.h"

namespace jre
{

    App::App(HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int show)
        : window(hinst, hprev, cmdline, show),
          m_renderer(this->window),
          imgui_context(m_renderer.pipeline(),
                        window),
          imwin_debug(m_statistics, m_renderer)
    {
        // AllocConsole();
        // freopen("CONOUT$", "w", stdout);
        // FreeConsole();

        window.message_handlers.push_back(imgui_context);
        imgui_context.imgui_windows.push_back(imwin_debug);
    }

    App::~App()
    {
    }

    int App::MainLoop()
    {
        int running_msg = 0;
        while (!running_msg)
        {
            Tick();
            Draw();
            running_msg = window.ProcessMessage();
        }
        return running_msg;
    }

    void App::Tick()
    {
        TickContext tick_context = {0};
        m_statistics.Tick();
        imgui_context.Tick(tick_context);
        m_renderer.Tick(tick_context);
    }
    void App::Draw()
    {
        try
        {
            VulkanPipeline::VulkanPipelineDrawContext pipeline_draw_context = m_renderer.pipeline().BeginDraw();
            DrawContext draw_context =
                {
                    pipeline_draw_context.command_buffer(),
                };
            m_renderer.Draw(draw_context);
            imgui_context.Draw(draw_context);
        }
        catch (vk::OutOfDateKHRError &e)
        {
            std::cerr << e.what() << '\n';
            // 当窗口的大小变化的时候，这个异常会爆出来
            // 一般是presentKHR爆出来的
            // C接口的话是判断presentKHR的返回值，https://github.com/KhronosGroup/Vulkan-Hpp/issues/274 这里会说，我自己创造性的用捕捉异常的方式解决这个问题，看来我C++理解的还是不错><
            // 处理参考代码https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation#page_Recreating-the-swap-chain
            // 这个重新创建需要在presentKHR之后，为的是保证m_render_finished_semaphore信号量被signal了
            // 如果需要显式的改变分辨率，就用触发变量 https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation#page_Handling-resizes-explicitly
            m_renderer.ReInitSwapChain();
            // present error
            // VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering. Usually happens after a 【window resize】.
            // VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly. 这个还没试过，如果这个出现也是ReInitSwapChain
        }
    }
}
