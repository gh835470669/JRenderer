#include "imwin_debug.h"
#include "tracy/Tracy.hpp"
#include <variant>

void ImWinDebug::tick()
{

    // SRS - Display Vulkan API version and device driver information if available (otherwise blank)
    // ImGui::Text("Vulkan API %i.%i.%i", VK_API_VERSION_MAJOR(device.properties.apiVersion), VK_API_VERSION_MINOR(device.properties.apiVersion), VK_API_VERSION_PATCH(device.properties.apiVersion));
    // ImGui::Text("%s %s", driverProperties.driverName, driverProperties.driverInfo);

    // Update frame time display
    // if (updateFrameGraph)
    // {
    //     std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
    //     float frameTime = 1000.0f / (frameTimer * 1000.0f);
    //     uiSettings.frameTimes.back() = frameTime;
    //     if (frameTime < uiSettings.frameTimeMin)
    //     {
    //         uiSettings.frameTimeMin = frameTime;
    //     }
    //     if (frameTime > uiSettings.frameTimeMax)
    //     {
    //         uiSettings.frameTimeMax = frameTime;
    //     }
    // }
    ZoneScoped;
    auto &frame_counter_graph = m_statistics.frame_counter_graph;
    ImGui::Text("fps: %lld", frame_counter_graph.frame_counter.fps());
    ImGui::Text("%f ms per frame", frame_counter_graph.frame_counter.mspf());

    auto &frame_fps = frame_counter_graph.frame_fps();
    ImGui::PlotLines("Frame Times", &frame_fps[0], 50, 0, "", 0.0f, frame_counter_graph.max_fps(), ImVec2(0, 80));

    //   enum class PresentModeKHR
    //   {
    //     eImmediate               = VK_PRESENT_MODE_IMMEDIATE_KHR,
    //     eMailbox                 = VK_PRESENT_MODE_MAILBOX_KHR,
    //     eFifo                    = VK_PRESENT_MODE_FIFO_KHR,
    //     eFifoRelaxed             = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
    //     eSharedDemandRefresh     = VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR,
    //     eSharedContinuousRefresh = VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR
    //   };
    static const char *present_modes[] = {"Immediate", "Mailbox", "Fifo", "Fifo Relaxed", "Shared Demand Refresh", "Shared Continuous Refresh"};
    static int item_current = 1;
    if (ImGui::Combo("present modes", &item_current, present_modes, IM_ARRAYSIZE(present_modes)))
    {
        m_renderer.graphics().set_vsync(static_cast<vk::PresentModeKHR>(item_current));
    }
}
