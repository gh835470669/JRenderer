#include "statistics.h"
#include <sstream>

jre::Statistics::Statistics()
{
    ticks.QuadPart = 0;
    counts_per_frame.QuadPart = 0;
    fps.QuadPart = 0;
    ms_per_frame = 0;
    QueryPerformanceFrequency(&frequency); // https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
}

void jre::Statistics::Tick()
{
    LARGE_INTEGER last_ticks = ticks;
    QueryPerformanceCounter(&ticks);
    counts_per_frame.QuadPart = ticks.QuadPart - last_ticks.QuadPart;

    fps.QuadPart = frequency.QuadPart / counts_per_frame.QuadPart;
    ms_per_frame = float(1000 * counts_per_frame.QuadPart) / frequency.QuadPart;

    // std::stringstream strstream;
    // strstream << "counts: " << counts.QuadPart << "\nfrequency: " << frequency.QuadPart << "\nfps: " << fps.QuadPart << "\nms: " << ms.QuadPart;
    // OutputDebugString(strstream.str().c_str());
}