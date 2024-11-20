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

        void FrameCounter::tick()
        {
            LARGE_INTEGER last_ticks = m_counts;
            QueryPerformanceCounter(&m_counts);
            m_counts_per_frame.QuadPart = std::max<long long>(m_counts.QuadPart - last_ticks.QuadPart, 1);
        }

        void FrameCounterGraph::tick()
        {
            frame_counter.tick();

            std::rotate(m_frames.begin(), m_frames.begin() + 1, m_frames.end());
            float frame_time = static_cast<float>(frame_counter.fps());
            m_frames.back() = frame_time;
            if (frame_time < m_min_fps)
            {
                m_min_fps = frame_time;
            }
            if (frame_time > m_max_fps)
            {
                m_max_fps = frame_time;
            }
        }

    }
}