#include "imwin_debug.h"
#include "tracy/Tracy.hpp"
#include <variant>
#include "jrenderer.h"
#include <fmt/core.h>
#include "jrenderer/async_helper.hpp"
#include "jrenderer/concrete_uniform_buffers.h"
#include <ranges>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

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

    auto transform_to_textures = m_renderer.model_lingsha.sub_mesh_materials |
                                 std::ranges::views::transform([](const auto &material)
                                                               { return material.textures; }) |
                                 std::ranges::views::join |
                                 std::ranges::views::transform([](const auto &texture) -> std::shared_ptr<jre::Texture2DDynamicMipmaps>
                                                               { return std::dynamic_pointer_cast<jre::Texture2DDynamicMipmaps>(texture); }) |
                                 std::ranges::views::filter([](const auto &texture)
                                                            { return texture != nullptr; });
    async_load_textures.reserve(std::ranges::distance(transform_to_textures));
    for (const auto &texture : transform_to_textures)
        async_load_textures.push_back(texture);
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
    shader_properties();
}

void ImWinDebug::mimaps()
{
    static bool use_mipmaps = true;
    bool old_use_mipmaps = use_mipmaps;
    ImGui::Checkbox("use mipmaps", &use_mipmaps);
    if (old_use_mipmaps != use_mipmaps)
    {
        m_renderer.graphics().wait_idle();
        for (auto &texture : async_load_textures)
        {
            texture->set_mipmaps(use_mipmaps);
        }
        for (auto &material : m_renderer.model_lingsha.sub_mesh_materials)
        {
            material.descriptor_set->update_descriptor_sets(
                {
                    m_renderer.model_lingsha.uniform_buffer->descriptor(),
                    material.textures[0]->descriptor(),
                    material.textures[1]->descriptor(),
                    material.textures[2]->descriptor(),
                    material.textures[3]->descriptor(),
                });
        }
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

void ImWinDebug::shader_properties()
{
    if (ImGui::CollapsingHeader("Shader Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // jre::PmxUniformPerRenderSet &ubo_per_render_set(m_renderer.model_lingsha.mesh.ubo.value_ref());
        // ImGui::DragFloat4("debug_control", glm::value_ptr(ubo_per_render_set.debug_control), 0.01f, 0.0f, 1.0f);
        jre::PmxUniformPerObject &ubo_per_object(m_renderer.model_lingsha.uniform_buffer->value_ref());
        ImGui::DragFloat4("debug_control", glm::value_ptr(ubo_per_object.debug_control), 0.01f, 0.0f, 1.0f);

        static auto origin_dir = m_renderer.star_rail_char_render_set.main_light.direction();
        static float angle = 0;
        if (ImGui::DragFloat("light direction angle", &angle, 1.0f, 0.0f, 360.0f))
        {
            glm::vec3 dir = glm::rotate(glm::angleAxis(glm::radians(angle), glm::vec3(0, 1, 0)), origin_dir);
            m_renderer.star_rail_char_render_set.main_light.set_direction(dir);
        }
        auto current_dir = m_renderer.star_rail_char_render_set.main_light.direction();
        if (ImGui::DragFloat3("light direction", glm::value_ptr(current_dir), 0.01f, -1.0f, 1.0f))
        {
            m_renderer.star_rail_char_render_set.main_light.set_direction(current_dir);
        }
    }
}
