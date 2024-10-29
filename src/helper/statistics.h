#pragma once

#include <windows.h>
#include <profileapi.h>
#include <vector>

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
            void tick();

        private:
            LARGE_INTEGER m_counts, m_counts_per_frame, m_frequency;
        };

        class FrameCounterGraph
        {
        public:
            FrameCounter frame_counter;

            FrameCounterGraph(int max_frame_count) : m_frames(max_frame_count) {}

            void tick();
            const std::vector<float> &frame_fps() { return m_frames; }
            float max_fps() const noexcept { return m_max_fps; }
            float min_fps() const noexcept { return m_min_fps; }

        private:
            std::vector<float> m_frames;
            float m_max_fps = 0.0f;
            float m_min_fps = 10000.0f;
        };
    }
}
