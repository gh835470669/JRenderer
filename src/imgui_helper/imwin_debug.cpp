#include "imwin_debug.h"
#include "tracy/Tracy.hpp"
#include <variant>
#include "jrenderer.h"

ImWinDebug::ImWinDebug(Statistics &stat, jre::JRenderer &renderer)
    : m_statistics(stat), m_renderer(renderer)
{
    vk::SampleCountFlags flags = m_renderer.graphics().physical_device()->get_supported_sample_counts();
    for (size_t i = 0; i < msaa_modes.size(); ++i)
    {
        if (flags & msaa_modes[i])
        {
            supported_msaa_modes.push_back(i);
        }
    }
}

void ImWinDebug::tick()
{
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
    static int item_current = static_cast<int>(m_renderer.graphics().present_mode());
    if (ImGui::Combo("present modes", &item_current, present_modes, IM_ARRAYSIZE(present_modes)))
    {
        m_renderer.graphics().set_vsync(static_cast<vk::PresentModeKHR>(item_current));
    }

    static int msaa_mode_current = std::distance(msaa_modes.begin(), std::find(msaa_modes.begin(), msaa_modes.end(), m_renderer.graphics().settings().msaa));
    if (ImGui::Combo("msaa modes", &msaa_mode_current, msaa_mode_names.data(), supported_msaa_modes.size()))
    {
        m_renderer.set_msaa(msaa_modes[supported_msaa_modes[msaa_mode_current]]);
    }

    camera_info();
}

void ImWinDebug::camera_info()
{
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Camera &camera = m_renderer.camera();
        CameraVec3 position = camera_eye(&camera);
        ImGui::Text("position: %f, %f, %f", position.x, position.y, position.z);

        glm::vec3 orient_eular = glm::eulerAngles(glm::quat{camera.orientation.w, camera.orientation.x, camera.orientation.y, camera.orientation.z});
        orient_eular = glm::degrees(orient_eular);
        ImGui::Text("orientation: %f, %f, %f", orient_eular.x, orient_eular.y, orient_eular.z);
    }
}
