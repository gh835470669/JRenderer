#pragma once

#include <windows.h>
#include <profileapi.h>

namespace jre
{
    namespace stats
    {

        class FrameCounter
        {
        public:
            FrameCounter();
            ~FrameCounter() = default;
            inline long long fps() { return m_frequency.QuadPart / m_counts_per_frame.QuadPart; };
            inline float mspf() { return float(1000 * m_counts_per_frame.QuadPart) / m_frequency.QuadPart; };
            void Tick();

        private:
            LARGE_INTEGER m_counts, m_counts_per_frame, m_frequency;
        };
    }
}
