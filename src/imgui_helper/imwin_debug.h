#pragma once

#include "../../imgui/imgui.h"
#include "imgui_window_base.h"
#include "../app_statistic.h"
#include "jrenderer.h"

class ImWinDebug : public ImguiWindow
{
private:
    Statistics &m_statistics;
    jre::JRenderer &m_renderer;

public:
    ImWinDebug(Statistics &stat, jre::JRenderer &renderer) : m_statistics(stat), m_renderer(renderer) {};
    void Tick() override;
};
