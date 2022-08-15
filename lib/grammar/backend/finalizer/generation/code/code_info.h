//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_CODE_INFO_H
#define SHARP_CODE_INFO_H

#include "../../../../../../stdimports.h"
#include "../../../../compiler_info.h"
#include "../../../../../runtime/Opcode.h"

struct line_info {
    line_info()
            :
            start_pc(0),
            line(0)
    {
    }

    line_info(uInt startPc, Int line)
            :
            start_pc(startPc),
            line(line)
    {
    }

    line_info(const line_info &li)
            :
            start_pc(0),
            line(0)
    {
        copy(li);
    }

    void copy(const line_info &li);

    uInt start_pc;
    Int line;
};

struct code_info {
    code_info()
            :
            address(-1),
            uuid(-1),
            stackSize(-1),
            code(),
            lineTable()
    {}

    code_info(const code_info &item)
            :
            address(-1),
            uuid(-1),
            stackSize(-1),
            code(),
            lineTable()
    {
        copy(item);
    }

    ~code_info() {
        free();
    }

    void free();

    void copy(const code_info &ci);

    int stackSize;
    List<line_info*> lineTable;
    List<opcode_instr> code;
    int address;
    Int uuid;
};

string code_to_string(code_info *info);

#endif //SHARP_CODE_INFO_H
