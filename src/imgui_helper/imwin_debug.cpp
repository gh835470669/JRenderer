#include "imwin_debug.h"
#include "tracy/Tracy.hpp"
#include <variant>
#include "jrenderer.h"
#include <fmt/core.h>
#include "jrenderer/async_helper.hpp"

ImWinDebug::ImWinDebug(Statistics &stat, jre::JRenderer &renderer)
    : m_statistics(stat), m_renderer(renderer)
{
    vk::SampleCountFlags flags = m_renderer.graphics().physical_device()->get_supported_sample_counts();
    for (int i = 0; i < msaa_modes.size(); ++i)
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
    ImGui::Text("fps: %d", static_cast<int>(frame_counter_graph.frame_counter.fps()));
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

    static int msaa_mode_current = static_cast<int>(std::distance(msaa_modes.begin(), std::find(msaa_modes.begin(), msaa_modes.end(), m_renderer.graphics().settings().msaa)));
    if (ImGui::Combo("msaa modes", &msaa_mode_current, msaa_mode_names.data(), static_cast<int>(supported_msaa_modes.size())))
    {
        m_renderer.set_msaa(msaa_modes[supported_msaa_modes[msaa_mode_current]]);
    }

    mimaps();
    camera_info();
}

void ImWinDebug::mimaps()
{
    static bool use_mipmaps = true;
    bool old_use_mipmaps = use_mipmaps;
    ImGui::Checkbox("use mipmaps", &use_mipmaps);
    if (old_use_mipmaps != use_mipmaps)
    {
        if (async_load_model.valid())
        {
            ZoneScoped; // 用std::lauch::async的话，future销毁时也是要等的。避免再开个线程并行不如在这里直接等了。
            async_load_model.wait();
        }
        {
            // 异步加载
            ZoneScoped;
            async_load_model = std::async(std::launch::async,
                                          [command_buffer = std::move(m_renderer.graphics().transfer_command_pool()->allocate_command_buffer()),
                                           &graphics = this->m_renderer.graphics()]()
                                          { return jre::JRenderer::load_lingsha(graphics, *command_buffer, use_mipmaps); });
        }
    }

    if (async_load_model.valid() && jre::is_ready(async_load_model))
    {
        ZoneScoped;
        m_renderer.graphics().wait_idle();
        m_renderer.model_lingsha = std::move(async_load_model.get());
        m_renderer.star_rail_char_render_set.render_objects = {m_renderer.model_lingsha};
    }
}

void ImWinDebug::camera_info()
{
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Camera &camera = m_renderer.camera;
        CameraVec3 position = camera_eye(&camera);
        ImGui::Text("position: %f, %f, %f", position.x, position.y, position.z);

        glm::vec3 orient_eular = glm::eulerAngles(glm::quat{camera.orientation.w, camera.orientation.x, camera.orientation.y, camera.orientation.z});
        orient_eular = glm::degrees(orient_eular);
        ImGui::Text("orientation: %f, %f, %f", orient_eular.x, orient_eular.y, orient_eular.z);
    }
}
