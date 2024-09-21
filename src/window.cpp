#include "window.h"

namespace jre
{
    LRESULT Window::sWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        Window *pThis; // our "this" pointer will go here
        if (msg == WM_NCCREATE)
        {
            // Recover the "this" pointer which was passed as a parameter
            // to CreateWindow(Ex).
            LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lp);
            pThis = static_cast<Window *>(lpcs->lpCreateParams);
            // Put the value in a safe place for future use
            SetWindowLongPtr(hwnd, GWLP_USERDATA,
                             reinterpret_cast<LONG_PTR>(pThis));
        }
        else
        {
            // Recover the "this" pointer from where our WM_NCCREATE handler
            // stashed it.
            pThis = reinterpret_cast<Window *>(
                GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        if (pThis)
        {
            // Now that we have recovered our "this" pointer, let the
            // member function finish the job.
            return pThis->WindowProc(hwnd, msg, wp, lp);
        }
        // We don't know what our "this" pointer is, so just do the default
        // thing. Hopefully, we didn't need to customize the behavior yet.
        return DefWindowProc(hwnd, msg, wp, lp);
    }

    Window::Window(HINSTANCE hinst_, HINSTANCE hprev, LPSTR cmdline, int show) : m_hinst(hinst_)
    {
        if (!hprev)
        {
            WNDCLASS c = {0};
            c.lpfnWndProc = Window::sWindowProc;
            c.hInstance = m_hinst;
            c.hIcon = LoadIcon(0, IDI_APPLICATION);
            c.hCursor = LoadCursor(0, IDC_ARROW);
            c.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
            c.lpszClassName = "Window";
            RegisterClass(&c);
        }

        m_hwnd = CreateWindow("Window",                     /* window class name*/
                              "JRenderer",                  /* title  */
                              WS_OVERLAPPEDWINDOW,          /* style */
                              CW_USEDEFAULT, CW_USEDEFAULT, /* position */
                              1920, 1080,                   /* size */
                              0,                            /* parent */
                              0,                            /* menu */
                              m_hinst,
                              this /* lparam -- crazytown */
        );
        ShowWindow(m_hwnd, show);
    }

    Window::~Window()
    {
    }

    LRESULT Window::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        /* NOTES
        1. when googling for api docs, etc. prefix everything with msdn.  e.g: msdn winmain
        2. prefer msdn docs to stack overflow for most things.
            When in doubt look for Raymond Chen's blog "The Old New Thing."

        3. Overview of the event loop is at [1].
        4. This event-loop spins when no events are available, consuming all available cpu.
            Also see GetMessage, which blocks until the next message is received.

            Use PeekMessage when you want your application loop to do something while not recieving
            events from the message queue.  You would do that work after the while(PeekMessage(...))
            block.
        */
        /* REFERENCES
        [1] : https://msdn.microsoft.com/en-us/library/windows/desktop/ms644927%28v=vs.85%29.aspx
        [2]: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633570(v=vs.85).aspx#designing_proc
        */
        switch (msg)
        {
        /*                                                              */
        /* Add a win32 push button and do something when it's clicked.  */
        /* Google will help you with the other widget types.  There are */
        /* many tutorials.                                              */
        /*                                                              */
        // case WM_CREATE: {
        //     HWND hbutton=CreateWindow("BUTTON","Hey There",  /* class and title */
        //                               WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON, /* style */
        //                               0,0,100,30,            /* position */
        //                               h,                     /* parent */
        //                               (HMENU)101,            /* unique (within the application) integer identifier */
        //                               GetModuleHandle(0),0   /* GetModuleHandle(0) gets the hinst */
        //                               );
        // } break;
        // case WM_COMMAND: {
        //     switch(LOWORD(wp)) {
        //         case 101: /* the unique identifier used above. These are usually #define's */
        //             PostQuitMessage(0);
        //             break;
        //         default:;
        //     }
        // } break;

        /*                                 */
        /* Minimally need the cases below: */
        /*                                 */
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        default:
            for (auto &handler : message_handlers)
            {
                if (auto res = handler.get().WindowProc(hwnd, msg, wp, lp) != 0)
                {
                    return res;
                }
            }

            return DefWindowProc(hwnd, msg, wp, lp);
        }
        return 0;
    }

    int Window::ProcessMessage()
    {
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        { /* See Note 3,4 */
            if (msg.message == WM_QUIT)
                // return (int)msg.wParam;
                return WM_QUIT;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    }
}
