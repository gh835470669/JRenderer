#pragma once

#include <windows.h>
#include <list>
#include "jmath.h"
#include <functional>

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

        MSG ProcessMessage(std::function<void(const MSG &)> callback = nullptr);

        HINSTANCE hinstance() const noexcept { return m_hinst; };
        HWND hwnd() const noexcept { return m_hwnd; };
        using WinSize = jmath::ivec2;
        WinSize size() const;
        int width() const { return size().x; }
        int height() const { return size().y; }

        std::list<std::reference_wrapper<IWindowMessageHandler>> message_handlers;
    };

}