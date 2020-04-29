//
// Created by BraxtonN on 2/5/2018.
//

#ifndef SHARP_EXCECPTION_H
#define SHARP_EXCECPTION_H


#include "../../stdimports.h"
#include <stdexcept>
#include "../runtime/oo/string.h"

struct ExceptionTable{
    ExceptionTable()
            :
            start_pc(0),
            end_pc(0),
            local(0),
            className("")
    {
    }

    void init() {
        start_pc=0;
        end_pc=0;
        handler_pc=0;
        local=0;
        className.init();
    }

    ~ExceptionTable()
    {
        className.free();
    }

    void operator=(const ExceptionTable& e) {
        this->start_pc=e.start_pc;
        this->end_pc=e.end_pc;
        this->handler_pc=e.handler_pc;
        this->local=e.local;
        this->className=e.className;
    }

    uInt start_pc, end_pc;
    uInt handler_pc;
    uInt local;
    String className;
};

struct FinallyTable {
    FinallyTable()
            :
            start_pc(0),
            end_pc(0)
    {
    }

    void operator=(const FinallyTable& ft) {
        start_pc=ft.start_pc;
        end_pc=ft.end_pc;
        try_start_pc=ft.try_start_pc;
        try_end_pc=ft.try_end_pc;
    }

    uInt start_pc, end_pc;
    uInt try_start_pc, try_end_pc;
};

#endif //SHARP_EXCECPTION_H
