#pragma once

#include <windows.h>
#include <list>

namespace jre
{

    class IWindowMessageHandler
    {
    public:
        virtual ~IWindowMessageHandler() = default;

        virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) = 0;
    };

    class Window
    {
    private:
        static LRESULT sWindowProc(HWND h, UINT msg, WPARAM wp, LPARAM lp);
        LRESULT WindowProc(HWND h, UINT msg, WPARAM wp, LPARAM lp);

        HINSTANCE m_hinst;
        HWND m_hwnd;

    public:
        Window(HINSTANCE hinst_,
               HINSTANCE hprev,
               LPSTR cmdline,
               int show);
        ~Window();

        int ProcessMessage();

        HINSTANCE hinstance() { return m_hinst; };
        HWND hwnd() { return m_hwnd; };

        std::list<std::reference_wrapper<IWindowMessageHandler>> message_handlers;
    };

}