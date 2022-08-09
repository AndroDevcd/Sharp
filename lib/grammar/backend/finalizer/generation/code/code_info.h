//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_CODE_INFO_H
#define SHARP_CODE_INFO_H

#include "../../../../../../stdimports.h"
#include "../../../../compiler_info.h"
#include "../../../../../runtime/Opcode.h"

struct code_info {
    code_info()
            :
            address(-1),
            code()
    {}

    code_info(const code_info &item)
            :
            address(-1),
            code()
    {
        copy(item);
    }

    ~code_info() {
        free();
    }

    void free();

    void copy(const code_info &ci);

    List<opcode_instr> code;
    int address;
};

string code_to_string(code_info *info);

#endif //SHARP_CODE_INFO_H
