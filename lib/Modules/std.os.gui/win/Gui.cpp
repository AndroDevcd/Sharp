//
// Created by bknun on 7/29/2018.
//

#include "Gui.h"

#ifdef WIN32_
#include <tchar.h>
#include "../../../runtime/Thread.h"
#include "../../../runtime/register.h"
#include "../../../runtime/VirtualMachine.h"

wnd_id ugwid = 0;
recursive_mutex wndMutex;

static TCHAR defaultWindowClass[] = _T("default window");


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
        windows.__new();
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
    if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        return 0;
    } else return 1;
}

_Message Gui::getMessage(wnd_id wnd) {
    Window *w = getContextFast(wnd);
    _Message msg;
    if(w && w->queue.size() > 0) {
        msg = w->queue.back();
        w->queue.pop_back();
    }
    return msg;
}

int Gui::paintStart(wnd_id wnd) {
    Window *w = getContextFast(wnd);
    if(w) {
        w->hdc = BeginPaint(w->wnd, &w->ps);
        return 0;
    }

    return 1;
}

int Gui::paintEnd(wnd_id wnd) {
    Window *w = getContextFast(wnd);
    if(w) {
        EndPaint(w->wnd, &w->ps);
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
            vm.gui->getWindow(hWnd)->queue.emplace_back(message, wParam, lParam);
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
            winPaint(_64ECX);
            break;
        case _g_set_ctx:
            _64CMT = setContext(_64ADX);
            break;
        case _g_ctx:
            _64CMT = currentContext();
            break;
        case _g_dwnd: {
            Thread* self = thread_self;
            SharpObject* name = (self->sp--)->object.object;
            SharpObject* title = (self->sp--)->object.object;

            if(name && title) {

                _64CMT = createDefaultWindow(native_string(name->HEAD, name->size),
                        native_string(title->HEAD, title->size), _64ECX, _64EGX);
            } else _64CMT = GUI_ERR;
            break;
        }
        case _g_show:
            _64CMT = show(_64ADX, _64EGX);
            break;
        case _g_upd:
            _64CMT = update(_64ADX);
            break;
        case _g_dsp:
            _64CMT = dispatchMessage();
            break;
        case _g_msg: {
            _Message msg = getMessage(_64ADX);
            _64EBX = msg.msg;
            _64ECX = msg.lParam;
            _64EGX = msg.wParam;
            break;
        }
        default: {
            // unsupported
            stringstream ss;
            ss << "unsupported signal to gui pipe: " << signal;
            throw Exception(ss.str());
        }
    }
}

void Gui::winPaint(long long proc) {
    Thread* self = thread_self;
    switch(proc) {
        case _pt_text: {
            SharpObject* str = (self->sp--)->object.object;
            if(str) {
                native_string msg(str->HEAD, str->size);
                string wMsg = msg.str();
                TextOut(ctx->hdc,
                        (int)(self->sp)->var, (int)(self->sp-1)->var,
                        wMsg.c_str(), _tcslen(wMsg.c_str()));
            }
            break;
        }
        case _pt_start:
            _64CMT = paintStart(_64ADX);
            break;
        case _pt_end:
            _64CMT = paintEnd(_64ADX);
            break;
        case _pt_move:
            _64CMT = MoveToEx(ctx->hdc, (int)(self->sp)->var, (int)(self->sp-1)->var, NULL);
            break;
        case _pt_line:
            _64CMT = LineTo(ctx->hdc, (int)(self->sp)->var, (int)(self->sp-1)->var);
            break;
        case _pt_rect:
            _64CMT = Rectangle(ctx->hdc, (int)(self->sp)->var, (int)(self->sp-1)->var, (int)(self->sp-2)->var, (int)(self->sp-3)->var);
            break;
        case _pt_fillrect: {
            if(ctx->hBrush == nullptr) { _64CMT = 1; return; }
            RECT rect = {(int)(self->sp)->var, (int)(self->sp-1)->var, (int)(self->sp-2)->var, (int)(self->sp-3)->var};
            _64CMT = FillRect(ctx->hdc, &rect, ctx->hBrush);
            break;
        }
        case _pt_ellipsize:
            _64CMT = Ellipse(ctx->hdc, (int)(self->sp)->var, (int)(self->sp-1)->var, (int)(self->sp-2)->var, (int)(self->sp-3)->var);
            break;
        case _pt_polygon:
            Poly poly;
            if(createPolygon(&poly)) {
                _64CMT = Polygon(ctx->hdc, poly.pts, poly.size);
                delete[] poly.pts;
            } else _64CMT = GUI_ERR;
            break;
        case _pt_createPen: {
            ctx->pens.push_back(
                    CreatePen((int)(self->sp)->var, (int)(self->sp-1)->var,
                              (COLORREF)(self->sp-2)->var));
            break;
        }
        case _pt_selectPen: {
            if(_64ADX < ctx->pens.size()) {
                _64CMT = 0;
                HPEN select = ctx->pens.at(_64ADX);
                HPEN old = (HPEN)SelectObject(ctx->hdc, select);
                if(ctx->origPen == nullptr)
                    ctx->origPen = old;
                ctx->hPen = select;
            } else _64CMT = 1;
            break;
        }
        case _pt_deletePen: {
            if(_64ADX < ctx->pens.size()) {
                _64CMT = 0;
                HPEN select = ctx->pens.at(_64ADX);
                DeleteObject(select);

                if(ctx->hPen == select)
                    ctx->hPen = nullptr;
                ctx->pens.erase(ctx->pens.begin() + _64ADX);
            } else _64CMT = 1;
            break;
        }
        case _pt_selectBrush: {
            if(_64ADX < ctx->pens.size()) {
                _64CMT = 0;
                HBRUSH select = ctx->brushes.at(_64ADX);
                SelectObject(ctx->hdc, select);
                ctx->hBrush = select;
            }
            break;
        }
        case _pt_deleteBrush: {
            if(_64ADX < ctx->pens.size()) {
                _64CMT = 0;
                HBRUSH select = ctx->brushes.at(_64ADX);
                DeleteObject(select);
                ctx->brushes.erase(ctx->brushes.begin() + _64ADX);
            } else _64CMT = 1;
            break;
        }
        case _pt_createBrush: {
            ctx->brushes.push_back(
                    CreateSolidBrush((COLORREF)(self->sp)->var));
            break;
        }
        default: {
            // unsupported
            stringstream ss;
            ss << "unsupported signal to gui-pt pipe: " << signal;
            throw Exception(ss.str());
        }
    }
}

bool Gui::createPolygon(Poly *poly) {
    Thread* self = thread_self;
    SharpObject *polygonObject = (self->sp)->object.object;

    if(polygonObject && IS_CLASS(polygonObject->info)) {
        ClassObject *k = &vm.classes[CLASS(polygonObject->info)];
        if(k->name == "std.os.gui#Polygon") {
            Object *points = vm.resolveField("points", polygonObject);
            if(points && points->object) {
                ClassObject *k2 = &vm.classes[CLASS(points->object->info)];
                if(IS_CLASS(points->object->info) && k2->name == "std.os.gui#Point") {
                    poly->size = (int)points->object->size;
                    poly->pts = new POINT[poly->size];

                    SharpObject* pt;
                    Object *x, *y;
                    for(int i = 0; i < poly->size; i++) {
                        x = vm.resolveField("x", points->object->node[i].object);
                        y = vm.resolveField("y", points->object->node[i].object);

                        if(x && y) {
                            poly->pts[i].x = (int)x->object->HEAD[0];
                            poly->pts[i].y = (int)y->object->HEAD[0];
                        } else { delete[] poly->pts; return false; }
                    }

                    return true;
                }
            }
        }
    }
    return false;
}

#endif
