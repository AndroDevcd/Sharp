//
// Created by bknun on 9/19/2022.
//

#include "bytecode_processor.h"
#include "exe_processor.h"
#include "../../core/exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../types/sharp_field.h"
#include "../../core/opcode/opcode.h"

void process_bytecode(KeyPair<int, string> &result) {
    uInt itemsProcessed=0;
    PROCESS_SECTION(eos,

        case data_byte: {
            if(itemsProcessed >= vm.manifest.methods) {
                result.with(CORRUPT_FILE, "file `" + executable + "` may be corrupt");
                throw runtime_error("");
            }

            sharp_function* method = &vm.methods[itemsProcessed++];

            if(method->bytecodeSize > 0) {
                method->bytecode = (opcode_instr*)malloc(sizeof(opcode_instr)*method->bytecodeSize); // deprecated
                for(int64_t i = 0; i < method->bytecodeSize; i++) {
                    method->bytecode[i] = next_int32();
                }
            } else if(method->fnType != delegate_function) {
                result.with(CORRUPT_FILE, "method `" + method->fullName + "` is missing bytecode");
                throw runtime_error("");
            }
            break;
        }, // on section_end
            break;
    )
}
