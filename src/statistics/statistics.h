#pragma once

#include <windows.h>
#include <profileapi.h>

namespace jre
{
    class Statistics
    {
    public:
        Statistics();
        ~Statistics() = default;

        LARGE_INTEGER ticks, counts_per_frame, frequency, fps;
        float ms_per_frame;

        void Tick();
    };

}
