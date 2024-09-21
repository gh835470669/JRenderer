#pragma once

#include "../../imgui/imgui.h"
#include "imgui_window_base.h"
#include "../../statistics/statistics.h"

namespace jre
{
    namespace imgui
    {

        class ImWinDebug : public ImguiWindow
        {
        private:
            const Statistics &m_statistics;

        public:
            ImWinDebug(const Statistics &stat) : m_statistics(stat) {};
            void Tick() override;
        };

    }
}
