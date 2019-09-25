//
// Created by bknun on 7/29/2018.
//

#ifndef SHARP_GUI_H
#define SHARP_GUI_H

#include "../../../../stdimports.h"

#include "../../../runtime/List.h"
#include <Windows.h>
#include <vector>
#include <mutex>

typedef int64_t wnd_id;
extern wnd_id ugwid;
#define GUI_ERR 547

struct Poly;

struct _Message {
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;

    _Message()
    :
        msg(0),
        wParam(0),
        lParam(0)
    {
    }

    _Message(UINT msg, WPARAM wp, LPARAM lp)
            :
            msg(msg),
            wParam(wp),
            lParam(lp)
    {
    }
};

struct Window {
    HWND wnd;
    vector<_Message> queue;
    wnd_id id;

    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hBrush; // our current brush
    HPEN hPen, origPen; // current and original pen
    vector<HPEN> pens;
    vector<HBRUSH> brushes;

    void init() {
        new (&queue) std::vector<_Message>();
        new (&pens) std::vector<HPEN>();
        new (&brushes) std::vector<HBRUSH>();
        id = 0;
        wnd = nullptr;
        memset(&ps, 0, sizeof(PAINTSTRUCT));
        hdc= nullptr;
        hBrush=nullptr;
        hPen = nullptr;
        origPen = nullptr;
    }
};

class Gui {
public:
    int setupMain();
    wnd_id createDefaultWindow(native_string winName, native_string winTitle, long width, long height);

    int show(wnd_id wnd, int cmd);
    _Message getMessage(wnd_id wnd);
    int update(wnd_id wnd);
    int dispatchMessage();
    void shutdown();
    int paintStart(wnd_id);
    int paintEnd(wnd_id);
    int setContext(wnd_id);
    wnd_id currentContext();
    Window* getWindow(HWND);

    /*
     * Pipe to interface to the gui
     */
    void winPaint(long long proc);
    void winGuiIntf(long long proc);

private:
    bool createPolygon(Poly *poly);
    Window* getContextFast(wnd_id);
    Window* getWindow(wnd_id);
    _List<Window> windows;
    Window *ctx;

};

enum _win_paintproc
{
    _pt_text=0,
    _pt_start=1,
    _pt_end=2,
    _pt_move=3,
    _pt_line=4,
    _pt_rect=5,
    _pt_fillrect=6,
    _pt_ellipsize=7,
    _pt_polygon=8,
    _pt_createPen=9,
    _pt_selectPen=10,
    _pt_deletePen=11,
    _pt_createBrush=12,
    _pt_selectBrush=13,
    _pt_deleteBrush=14,
};

enum _win_guiproc
{
    _g_quit=0,
    _g_paint=1,
    _g_set_ctx=2,
    _g_ctx=3,
    _g_dwnd=4,
    _g_show=5,
    _g_upd=6,
    _g_dsp=7,
    _g_msg=8,
};

// higher level paint structs
struct Poly { // polygon struct
    POINT* pts;
    int size;
};

LRESULT  winProc(HWND, UINT, WPARAM, LPARAM);
#endif //SHARP_GUI_H
