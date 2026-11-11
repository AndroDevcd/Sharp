//
// Created by bknun on 8/30/2022.
//

#include "scheme_asm.h"
#include "step_processor.h"
#include "scheme_processor.h"
#include "../code_context.h"

void process_asm_scheme(operation_schema *scheme) {
    if(cc.ci != NULL) cc.ci->hasAsm = true; // tag for the bytecode optimizer, see code_info::hasAsm
    process_scheme_steps(scheme);
}
