//
// Created by bknun on 7/29/2018.
//

#include <tchar.h>
#include "Gui.h"

HINSTANCE hInst;
wnd_id ugwid = 0;

int Gui::setupMain() {
    WNDCLASSEX wcex;
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    hInst = hInstance;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = winProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, IDC_ARROW);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = "gui window";
    wcex.hIconSm        = LoadIcon(wcex.hInstance, IDC_ARROW);

    if (!RegisterClassEx(&wcex))
    {
        return 1;
    }



    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
               SW_SHOWMINIMIZED);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void Gui::shutdown() {
    windows.free();
}

int Gui::createDefaultWindow(native_string winName, native_string winTitle, long width, long height) {
    string name = string(winName.chars, winName.len);
    string title = string(winTitle.chars, winTitle.len);
    HWND hWnd = CreateWindow(
            name.c_str(),
            title.c_str(),
            WS_VISIBLE|WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            NULL,
            NULL,
            hInst,
            NULL
    );

    if (!hWnd)
    {
        return 1;
    } else {
        windows.push_back();
        Window &wnd = windows.last();

        wnd.init();
        wnd.wnd = hWnd;
        wnd.id = ugwid++;
    }
    return 0;
}

LRESULT winProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, World!");

    switch (message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);

            // Here your application is laid out.
            // For this introduction, we just print out "Hello, World!"
            // in the top left corner.
            TextOut(hdc,
                    5, 5,
                    greeting, _tcslen(greeting));
            // End application-specific layout section.

            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }

    return 0;
}
