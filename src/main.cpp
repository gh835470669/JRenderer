#include "app.h"
#include "fmt/core.h"

int WinMain(HINSTANCE hinst,
            HINSTANCE hprev,
            LPSTR cmdline,
            int show)
{
    App app(hinst, hprev, cmdline, show);
    return app.MainLoop();
}
