//
// Created by bknun on 7/29/2018.
//

#ifndef SHARP_GUI_H
#define SHARP_GUI_H

#include "../../../../stdimports.h"

#include "../../../runtime/List.h"
#include <Windows.h>
#include <vector>

typedef int64_t wnd_id;
extern wnd_id ugwid;

struct _Message {
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
};

struct Window {
    HWND wnd;
    vector<_Message> queue;
    wnd_id id;

    void init() {
        new (&queue) std::vector<_Message>();
        id = 0;
        wnd = nullptr;
    }
};

class Gui {
public:
    int setupMain();
    int createDefaultWindow(native_string winName, native_string winTitle, long width, long height);

    void show(wnd_id wnd);
    int getMessage(wnd_id wnd);
    int draw(wnd_id wnd);
    _Message dispatchMessage(wnd_id wnd);
    void shutdown();

private:
    List<Window> windows;
};

LRESULT  winProc(HWND, UINT, WPARAM, LPARAM);
#endif //SHARP_GUI_H
