//
// Created by bknun on 7/29/2018.
//

#include <tchar.h>
#include "Gui.h"
#include "../../../runtime/Thread.h"
#include "../../../runtime/Environment.h"
#include "../../../runtime/register.h"

wnd_id ugwid = 0;
recursive_mutex wndMutex;

static TCHAR defaultWindowClass[] = _T("default window");

thread_local PAINTSTRUCT ps;
thread_local HDC hdc;

int Gui::setupMain() {
    ctx = NULL;
    windows.init();
    return 0;
}

void Gui::shutdown() {
    windows.free();
}

wnd_id Gui::createDefaultWindow(native_string winName, native_string winTitle, long width, long height) {
    WNDCLASSEX wcex;
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    memset(&wcex,0,sizeof(wcex));

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = winProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, IDC_ARROW);
    wcex.hCursor        = LoadCursor(NULL, IDI_WINLOGO);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = defaultWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, IDI_WINLOGO);

    if (!RegisterClassEx(&wcex))
    {
        return -1;
    }

    string name = string(winName.chars, winName.len);
    string title = string(winTitle.chars, winTitle.len);
    HWND hWnd = CreateWindow(
            _T(defaultWindowClass),
            _T(title.c_str()),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            NULL,
            NULL,
            hInstance,
            NULL
    );

    if (hWnd == NULL)
    {
        return -2;
    } else {
        windows.push_back();
        Window &wnd = windows.last();

        wnd.init();
        wnd.wnd = hWnd;
        wnd.id = ugwid++;
        return wnd.id;
    }
}

int Gui::show(wnd_id wnd, int cmd) {
    Window *w = getContextFast(wnd);
    if(w) {
        ShowWindow(w->wnd,
                   cmd);
        return 0;
    }
    return 1;
}

int Gui::update(wnd_id wnd) {
    Window *w = getContextFast(wnd);
    if(w) {
        UpdateWindow(w->wnd);
        return 0;
    }
    return 1;
}

int Gui::dispatchMessage() {
    MSG msg;
    if(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        return 0;
    } else return 1;
}

_Message Gui::getMessage(wnd_id wnd) {
    Window *w = getContextFast(wnd);
    _Message msg;
    if(w) {
        msg = w->queue.back();
        w->queue.pop_back();
    }
    return msg;
}

int Gui::paintStart(wnd_id wnd) {
    Window *w = getContextFast(wnd);
    if(w) {
        hdc = BeginPaint(w->wnd, &ps);
        return 0;
    }

    return 1;
}

int Gui::paintEnd(wnd_id wnd) {
    Window *w = getContextFast(wnd);
    if(w) {
        EndPaint(w->wnd, &ps);
        return 0;
    }
    return 1;
}

int Gui::setContext(wnd_id wnd) {
    for(wnd_id i = 0; i < windows.size(); i++) {
        Window &window = windows.get(i);
        if(window.id == wnd) {
            ctx = &window;
            return 0;
        }
    }
    return 1;
}

wnd_id Gui::currentContext() {
    return ctx == NULL? -1 : ctx->id;
}

Window *Gui::getWindow(wnd_id wnd) {
    for(wnd_id i = 0; i < windows.size(); i++) {
        Window &window = windows.get(i);
        if(window.id == wnd) {
            return &window;
        }
    }

    return nullptr;
}

Window *Gui::getContextFast(wnd_id wnd) {
    if(ctx) {
        if(ctx->id==wnd)
            return ctx;
    }
    return getWindow(wnd);
}

Window *Gui::getWindow(HWND wnd) {
    for(wnd_id i = 0; i < windows.size(); i++) {
        Window &window = windows.get(i);
        if(window.wnd == wnd) {
            return &window;
        }
    }

    return nullptr;
}

LRESULT winProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    wndMutex.lock();
    LRESULT lresult = 0;

    switch (message)
    {
        case WM_PAINT:
        case WM_DESTROY:
            env->gui->getWindow(hWnd)->queue.emplace_back(message, wParam, lParam);
            break;
        default:
            lresult = DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }

    wndMutex.unlock();
    return lresult;
}

void Gui::winGuiIntf(long long proc) {
    switch(proc) {
        case _g_quit:
            PostQuitMessage(0);
            break;
        case _g_paint:
            winPaint(ECX);
            break;
        case _g_set_ctx:
            CMT = setContext(ADX);
            break;
        case _g_ctx:
            CMT = currentContext();
            break;
        case _g_dwnd: {
            Thread* self = thread_self;
            SharpObject* name = (self->sp--)->object.object;
            SharpObject* title = (self->sp--)->object.object;

            if(name && title) {

                CMT = createDefaultWindow(native_string(name->HEAD, name->size),
                        native_string(title->HEAD, title->size), ECX, EGX);
            } else CMT = 547;
            break;
        }
        case _g_show:
            CMT = show(ADX, EGX);
            break;
        case _g_upd:
            CMT = update(ADX);
            break;
        case _g_dsp:
            CMT = dispatchMessage();
            break;
        case _g_msg: {
            _Message msg = getMessage(ADX);
            EBX = msg.msg;
            ECX = msg.lParam;
            EGX = msg.wParam;
            break;
        }
        default:
            break;
    }
}

void Gui::winPaint(long long proc) {
    Thread* self = thread_self;
    switch(proc) {
        case _pt_text: {
            SharpObject* str = (self->sp--)->object.object;
            if(str) {
                native_string msg(str->HEAD, str->size);
                const char *wMsg = msg.str().c_str();
                TextOut(hdc,
                        (int)(self->sp)->var, (int)(self->sp-1)->var,
                        wMsg, _tcslen(wMsg));
                self->sp-=2;
            }
            break;
        }
        case _pt_start:
            CMT = paintStart(ADX);
            break;
        case _pt_end:
            CMT = paintEnd(ADX);
            break;
        default:
            break;
    }
}