//
// Created by bknun on 9/19/2022.
//

#include "string_processor.h"
#include "exe_processor.h"
#include "../../core/exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../types/sharp_field.h"

void process_strings(KeyPair<int, string> &result) {
    check_section(sstring, CORRUPT_FILE, result);


    uInt itemsProcessed=0;
    PROCESS_SECTION(eos,
       case data_string: {
           if(itemsProcessed >= vm.manifest.strings) {
               result.with(CORRUPT_FILE, "file `" + executable + "` may be corrupt");
               throw runtime_error("");
           }

           next_chars(vm.strings[itemsProcessed++], next_int32());
           continue;
       }, // on section_end

       if (itemsProcessed != vm.manifest.strings) {
           throw invalid_argument("Failed to process all strings in executable.");
       }

       break;
    )
}
