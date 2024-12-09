#include "app.h"

int WinMain(HINSTANCE hinst,
            HINSTANCE hprev,
            LPSTR cmdline,
            int show)
{
    App app(hinst, hprev, cmdline, show);
    return app.main_loop();
}
