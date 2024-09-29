#include "imwin_debug.h"
#include "tracy/Tracy.hpp"
void ImWinDebug::Tick()
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
    ImGui::Text("fps: %lld", m_statistics.frame_counter.fps());
    ImGui::Text("%f ms per frame", m_statistics.frame_counter.mspf());
    jre::RenderSetting setting = m_renderer.render_setting();
    if (ImGui::Checkbox("vsync", &setting.vsync))
    {
        m_renderer.GetRebuilder().setVsync(setting.vsync);
    }
}
