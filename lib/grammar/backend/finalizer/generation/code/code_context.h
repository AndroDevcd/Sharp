//
// Created by bknun on 8/9/2022.
//

#ifndef SHARP_CODE_CONTEXT_H
#define SHARP_CODE_CONTEXT_H

#include "../../../../../../stdimports.h"
#include "../../../../compiler_info.h"
#include "../../../../../runtime/Opcode.h"

struct code_info;

struct code_context {
    code_context()
            :
            ci(NULL),
            container(NULL)
    {}

    code_context(const code_context &cc)
            :
            ci(cc.ci),
            instructions(cc.instructions),
            container(cc.container)
    {
    }

    ~code_context() {
        free();
    }

    void free();

    code_info* ci;
    List<opcode_instr> instructions;
    sharp_function *container;
};

void update_context(code_info* ci, sharp_function *container);
void flush_context();

extern code_context cc;

#endif //SHARP_CODE_CONTEXT_H
