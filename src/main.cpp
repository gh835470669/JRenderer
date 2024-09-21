#include "app.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_vulkan.h"

int WinMain(HINSTANCE hinst,
            HINSTANCE hprev,
            LPSTR cmdline,
            int show)
{

    jre::App app(hinst, hprev, cmdline, show);
    return app.main_loop();
}
