#include "app.h"
#include "imgui_helper/VulkanExampleBase.h"
#include "imgui_helper/VulkanDevice.h"

namespace jre
{

    App::App(HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int show)
        : window(hinst, hprev, cmdline, show),
          m_renderer(this->window.hinst, this->window.hwnd)
    {
        // AllocConsole();
        // freopen("CONOUT$", "w", stdout);
        // FreeConsole();
    }

    int App::main_loop()
    {
        int running_msg = 0;
        while (!running_msg)
        {
            m_renderer.main_loop();
            running_msg = window.main_loop();
        }
        return running_msg;
    }

}
