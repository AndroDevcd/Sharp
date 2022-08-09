//
// Created by bknun on 8/9/2022.
//

#include "code_context.h"
#include "code_info.h"

code_context cc;

void code_context::free() {
    instructions.free();
    container = NULL;
    ci = NULL;
}

void update_context(code_info* ci, sharp_function *container) {
    cc.ci = ci;
    cc.container = container;
}

void flush_context() {
    cc.ci->code.appendAll(cc.instructions);
    cc.free();
}
