#pragma once

#include "../../imgui/imgui.h"
#include "imgui_window_base.h"
#include "../../statistics/statistics.h"
#include "../../jrenderer.h"

namespace jre
{
    namespace imgui
    {

        class ImWinDebug : public ImguiWindow
        {
        private:
            const Statistics &m_statistics;
            JRenderer &m_renderer;

        public:
            ImWinDebug(const Statistics &stat, JRenderer &renderer) : m_statistics(stat), m_renderer(renderer) {};
            void Tick() override;
        };

    }
}
