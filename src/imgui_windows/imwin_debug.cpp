#include "imwin_debug.h"
#include "tracy/Tracy.hpp"
#include "jrenderer.h"
#include "jrenderer/concrete_uniform_buffers.h"
#include "jrenderer/camera/camera.h"
#include "jrenderer/drawer/scene_drawer.h"
#include "jrenderer/asset/star_rail_material.h"
#include <fmt/core.h>
#include <ranges>
#include <set>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan_raii.hpp>

ImWinDebug::ImWinDebug(Statistics &stat, jre::JRenderer &renderer)
    : m_statistics(stat), m_renderer(renderer)
{
}

static const char *combo_vector_getter(void *user_data, int idx)
{
    std::string *strings = reinterpret_cast<std::string *>(user_data);
    return strings[idx].c_str();
}

void ImWinDebug::tick()
{
    ZoneScoped;
    frame_info();
    present_mode();
    msaa();
    camera_info();
    shader_properties();
}

void ImWinDebug::frame_info()
{
    auto &frame_counter_graph = m_statistics.frame_counter_graph;
    ImGui::Text("fps: %d", static_cast<int>(frame_counter_graph.frame_counter.fps()));
    ImGui::Text("%f ms per frame", frame_counter_graph.frame_counter.mspf());

    auto &frame_fps = frame_counter_graph.frame_fps();
    ImGui::PlotLines("Frame Times", &frame_fps[0], 50, 0, "", 0.0f, frame_counter_graph.max_fps(), ImVec2(0, 80));
}

void ImWinDebug::present_mode()
{
    //   enum class PresentModeKHR
    //   {
    //     eImmediate               = VK_PRESENT_MODE_IMMEDIATE_KHR,
    //     eMailbox                 = VK_PRESENT_MODE_MAILBOX_KHR,
    //     eFifo                    = VK_PRESENT_MODE_FIFO_KHR,
    //     eFifoRelaxed             = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
    //     eSharedDemandRefresh     = VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR,
    //     eSharedContinuousRefresh = VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR
    //   };
    static std::vector<vk::PresentModeKHR> present_modes = m_renderer.graphics().physical_device().getSurfacePresentModesKHR(m_renderer.graphics().surface().get());
    static std::vector<std::string> present_mode_names = present_modes |
                                                         std::views::transform([](vk::PresentModeKHR mode)
                                                                               { return vk::to_string(mode); }) |
                                                         std::ranges::to<std::vector>();
    static int item_current = static_cast<int>(std::distance(present_modes.begin(), std::find(present_modes.begin(), present_modes.end(), m_renderer.graphics().present_mode())));
    if (ImGui::Combo("present modes", &item_current, combo_vector_getter, present_mode_names.data(), present_mode_names.size()))
    {
        m_renderer.graphics().set_vsync(present_modes[item_current]);
    }
}

void ImWinDebug::msaa()
{
    static std::vector<vk::SampleCountFlagBits> msaa_modes_supported = std::to_array({vk::SampleCountFlagBits::e1,
                                                                                      vk::SampleCountFlagBits::e2,
                                                                                      vk::SampleCountFlagBits::e4,
                                                                                      vk::SampleCountFlagBits::e8,
                                                                                      vk::SampleCountFlagBits::e16,
                                                                                      vk::SampleCountFlagBits::e32,
                                                                                      vk::SampleCountFlagBits::e64}) |
                                                                       std::views::filter([this](vk::SampleCountFlagBits mode)
                                                                                          { 
              static vk::SampleCountFlags flags = 
               m_renderer.graphics().physical_device_info().properties.limits.framebufferColorSampleCounts &
               m_renderer.graphics().physical_device_info().properties.limits.framebufferDepthSampleCounts;
              return bool(flags & mode); }) |
                                                                       std::ranges::to<std::vector>();
    static std::vector<std::string> msaa_mode_names = msaa_modes_supported | std::views::transform([](vk::SampleCountFlagBits mode)
                                                                                                   { return vk::to_string(mode); }) |
                                                      std::ranges::to<std::vector>();
    static int msaa_mode_current = static_cast<int>(std::distance(msaa_modes_supported.begin(), std::find(msaa_modes_supported.begin(), msaa_modes_supported.end(), m_renderer.graphics().settings().msaa)));
    if (ImGui::Combo("msaa samples", &msaa_mode_current, combo_vector_getter, msaa_mode_names.data(), msaa_mode_names.size()))
    {
        m_renderer.set_msaa(msaa_modes_supported[msaa_mode_current]);
    }
}

void ImWinDebug::camera_info()
{
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Camera &camera = m_renderer.scene_drawer().scene.render_viewports[0].camera;
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
        std::set<std::shared_ptr<jre::StarRailMaterialInstance>> base_materials = m_renderer.scene_drawer().scene.models[0].materials |
                                                                                  std::views::transform([](std::shared_ptr<jre::IMaterialInstance> &material_instance)
                                                                                                        { return std::dynamic_pointer_cast<jre::StarRailMaterialInstance>(material_instance); }) |
                                                                                  std::views::filter([](std::shared_ptr<jre::StarRailMaterialInstance> material_instance)
                                                                                                     { return material_instance != nullptr; }) |
                                                                                  std::ranges::to<std::set>();

        std::set<std::shared_ptr<jre::StarRailOutlineMaterialInstance>> outline_materials = m_renderer.scene_drawer().scene.models[0].materials |
                                                                                            std::views::transform([](std::shared_ptr<jre::IMaterialInstance> &material_instance)
                                                                                                                  { return std::dynamic_pointer_cast<jre::StarRailOutlineMaterialInstance>(material_instance); }) |
                                                                                            std::views::filter([](std::shared_ptr<jre::StarRailOutlineMaterialInstance> material_instance)
                                                                                                               { return material_instance != nullptr; }) |
                                                                                            std::ranges::to<std::set>();
        static glm::vec3 origin_dir = m_renderer.scene_drawer().scene.main_light.direction();
        static float angle = 0;
        if (ImGui::DragFloat("light direction angle", &angle, 1.0f, 0.0f, 360.0f))
        {
            glm::vec3 dir = glm::rotate(glm::angleAxis(glm::radians(angle), glm::vec3(0, 1, 0)), origin_dir);
            m_renderer.scene_drawer().scene.main_light.set_direction(dir);
        }

        auto changed = false;
        if (!base_materials.empty())
        {
            static jre::UniformStarRailDebug ubo_debug = base_materials.begin()->get()->buffer_data_debug;
            changed |= ImGui::Checkbox("show material region", &ubo_debug.show_material_region);
            changed |= ImGui::DragFloat("debug control x", &ubo_debug.debug_control.x, 0.01f, 0.0f, 1.0f);
            changed |= ImGui::DragFloat("debug control y", &ubo_debug.debug_control.y, 0.1f, 0.0f, 10.0f);
            changed |= ImGui::DragFloat("debug control z", &ubo_debug.debug_control.z, 1.f, 0.0f, 100.0f);
            if (changed)
            {
                for (std::shared_ptr<jre::StarRailMaterialInstance> material_instance : base_materials)
                {
                    material_instance->buffer_data_debug = ubo_debug;
                }
            }

            if (ImGui::CollapsingHeader("Specular"))
            {
                static jre::UniformPropertiesStarRail ubo_props = base_materials.begin()->get()->buffer_data_props;
                changed = false;
                for (int i = 0; i < jre::MATERIAL_REGION_COUNT; ++i)
                {
                    changed |= ImGui::ColorEdit4(fmt::format("specular color {}", i).c_str(), glm::value_ptr(ubo_props.specular_colors[i]));
                    changed |= ImGui::DragFloat(fmt::format("specular shinness {}", i).c_str(), &ubo_props.specular_shininess[i], 0.01f, 0.0f, 1.0f);
                    changed |= ImGui::DragFloat(fmt::format("specular roughness {}", i).c_str(), &ubo_props.specular_roughness[i], 0.01f, 0.0f, 1.0f);
                }
                if (changed)
                {
                    for (std::shared_ptr<jre::StarRailMaterialInstance> material_instance : base_materials)
                    {
                        material_instance->buffer_data_props = ubo_props;
                    }
                }
            }
        }

        if (!outline_materials.empty())
        {
            static jre::UniformPropertiesStarRail ubo_props = outline_materials.begin()->get()->buffer_data_props;
            changed = false;
            changed |= ImGui::DragFloat("outline width", &ubo_props.outline.width, 0.01f, 0.0f, 1.0f);
            changed |= ImGui::DragFloat("outline factor_of_color", &ubo_props.outline.factor_of_color, 0.01f, 0.0f, 1.0f);
            changed |= ImGui::ColorEdit3("outline color", glm::value_ptr(ubo_props.outline.color));
            if (changed)
            {
                for (std::shared_ptr<jre::StarRailOutlineMaterialInstance> material_instance : outline_materials)
                {
                    material_instance->buffer_data_props = ubo_props;
                }
            }
        }
    }
}
