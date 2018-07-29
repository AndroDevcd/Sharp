//
// Created by bknun on 7/29/2018.
//

#ifndef SHARP_GUI_H
#define SHARP_GUI_H

#include "../../../../stdimports.h"

#ifdef WIN32_ //  windows GUI interface
#include "../../../runtime/List.h"
#include <Windows.h>
class Gui {
    List<HWND*> windows;
};

#endif
#endif //SHARP_GUI_H
