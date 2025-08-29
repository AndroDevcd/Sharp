//
// Created by bknun on 9/19/2022.
//

#include "constant_processor.h"
#include "exe_processor.h"
#include "../../core/exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../types/sharp_field.h"

void process_constants(KeyPair<int, string> &result) {
    check_section(sconst, CORRUPT_FILE, result);


    uInt itemsProcessed=0;
    PROCESS_SECTION(eos,
        case data_const: {
            if(itemsProcessed >= vm.mf.constants) {
                result.with(CORRUPT_FILE, "file `" + executable + "` may be corrupt");
                throw runtime_error("");
            }

            string constNum;
            next_string(constNum);
            vm.constants[itemsProcessed++] = std::strtod(constNum.c_str(), NULL);
            continue;
        }, // on section_end

        if (itemsProcessed != vm.mf.constants) {
            throw invalid_argument("Failed to process all constants in executable.");
        }

        break;
    )
}
