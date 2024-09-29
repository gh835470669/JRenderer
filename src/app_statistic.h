#pragma once

#include "helper/statistics.h"

class Statistics
{
public:
    Statistics() = default;
    ~Statistics() = default;

    jre::stats::FrameCounter frame_counter;

    void Tick();
};