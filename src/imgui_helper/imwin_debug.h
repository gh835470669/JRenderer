#pragma once

#include "imgui/imgui.h"
#include "imgui_window_base.h"
#include "../app_statistic.h"
#include "jrenderer.h"
#include <future>

class ImWinDebug : public ImguiWindow
{
private:
    Statistics &m_statistics;
    jre::JRenderer &m_renderer;

    std::array<const char *, 7> msaa_mode_names = {"disabled", "2x", "4x", "8x", "16x", "32x", "64x"};
    std::array<vk::SampleCountFlagBits, 7> msaa_modes = {vk::SampleCountFlagBits::e1, vk::SampleCountFlagBits::e2, vk::SampleCountFlagBits::e4, vk::SampleCountFlagBits::e8, vk::SampleCountFlagBits::e16, vk::SampleCountFlagBits::e32, vk::SampleCountFlagBits::e64};
    std::vector<int> supported_msaa_modes{};

    std::future<jre::PmxModel> async_load_model;

    std::vector<std::shared_ptr<jre::Texture2DDynamicMipmaps>> async_load_textures;

public:
    ImWinDebug(Statistics &stat, jre::JRenderer &renderer);
    void tick() override;

    void camera_info();
    void mimaps();
    void shader_properties();
};
