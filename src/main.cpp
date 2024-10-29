#include "app.h"
#include "fmt/core.h"

#include "jrenderer/resources.hpp"

int WinMain(HINSTANCE hinst,
            HINSTANCE hprev,
            LPSTR cmdline,
            int show)
{
    App app(hinst, hprev, cmdline, show);
    return app.MainLoop();
}
