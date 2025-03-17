#pragma once

#include <windows.h>
#include <vector>
#include <functional>
#include <memory>
#include "jmath.h"

namespace jre
{
    using WindowMessageHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

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
        using WinSize = jmath::uvec2;
        WinSize size() const;
        uint32_t width() const { return size().x; }
        uint32_t height() const { return size().y; }

        std::vector<WindowMessageHandler> message_handlers;
    };

}