#pragma once

#include "../app_statistic.h"
#include "jrenderer.h"

class ImWinDebug
{
private:
    Statistics &m_statistics;
    jre::JRenderer &m_renderer;

public:
    ImWinDebug(Statistics &stat, jre::JRenderer &renderer);
    void tick();

    void control_info();
    void camera_info();
    void frame_info();
    void present_mode();
    void msaa();
    void shader_properties();
};
