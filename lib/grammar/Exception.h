//
// Created by BraxtonN on 2/5/2018.
//

#ifndef SHARP_EXCECPTION_H
#define SHARP_EXCECPTION_H


#include "../../stdimports.h"
#include <stdexcept>

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
        local=0;
        className="";
    }

    ~ExceptionTable()
    {
        className.clear();
    }

    void operator=(const ExceptionTable& e) {
        this->start_pc=e.start_pc;
        this->end_pc=e.end_pc;
        this->handler_pc=e.handler_pc;
        this->local=e.local;
        this->className=e.className;
    }

    uint64_t start_pc, end_pc;
    uint64_t handler_pc;
    int64_t local;
    string className;
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

    uint64_t start_pc, end_pc;
    uint64_t try_start_pc, try_end_pc;
};

enum FinallyCommands {
    EXEC_ALL_FINALLY=0,
    EXEC_SINGLE_FINALLY=1,
    EXEC_PRECEDING_FINALLY=3
};

#endif //SHARP_EXCECPTION_H
