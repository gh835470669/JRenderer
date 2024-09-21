#pragma once

#include "imwin_debug.h"
#include "../../statistics/statistics.h"

namespace jre
{
    namespace imgui
    {
        class ImWindows
        {
        private:
            ImWinDebug debug;

        public:
            ImWindows(const Statistics &stat) : debug(stat) {};
            ~ImWindows() = default;
            void Tick();
        };

    }
}