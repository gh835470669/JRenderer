#include "statistics.h"
#include <sstream>

namespace jre
{
    namespace stats
    {
        FrameCounter::FrameCounter()
        {
            m_counts.QuadPart = 0;
            m_counts_per_frame.QuadPart = 1;
            QueryPerformanceFrequency(&m_frequency); // https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
        }

        void FrameCounter::Tick()
        {
            LARGE_INTEGER last_ticks = m_counts;
            QueryPerformanceCounter(&m_counts);
            m_counts_per_frame.QuadPart = max(m_counts.QuadPart - last_ticks.QuadPart, 1);
        }

    }
}