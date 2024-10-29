#pragma once

#include "helper/statistics.h"

class Statistics
{
public:
    Statistics() : frame_counter_graph(50) {}
    ~Statistics() = default;

    jre::stats::FrameCounterGraph frame_counter_graph;

    void tick();
};