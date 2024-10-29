#include "app_statistic.h"
#include "tracy/Tracy.hpp"

void Statistics::tick()
{
    ZoneScoped;
    frame_counter_graph.tick();
}