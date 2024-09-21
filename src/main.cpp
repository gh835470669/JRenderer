#include "app.h"

int WinMain(HINSTANCE hinst,
            HINSTANCE hprev,
            LPSTR cmdline,
            int show)
{

    jre::App app(hinst, hprev, cmdline, show);
    return app.MainLoop();
}
