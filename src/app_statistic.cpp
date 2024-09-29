#include "app_statistic.h"
#include "tracy/Tracy.hpp"

void Statistics::Tick()
{
    ZoneScoped;
    frame_counter.Tick();
}