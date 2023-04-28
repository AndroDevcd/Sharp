//
// Created by bknun on 9/19/2022.
//

#include "meta_data_processor.h"
#include "exe_processor.h"
#include "../../core/exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../types/sharp_field.h"
#include "../../core/opcode/opcode.h"

void parseSourceFile(file_data &sourceFile, string &data) {
    string line;
    for(unsigned int i = 0; i < data.size(); i++) {
        if(data[i] == '\n')
        {
            sourceFile.lines.emplace_back("");
            *sourceFile.lines.end() = line;
        } else {
            line += data[i];
        }
    }

    sourceFile.lines.emplace_back("");
    *sourceFile.lines.end() = line;
}

void process_meta_data(KeyPair<int, string> &result) {
    check_section(smeta, CORRUPT_FILE, result);

    uInt itemsProcessed=0;
    PROCESS_SECTION(eos,

       case data_file: {
           vm.mdata.files.emplace_back(new file_data());
           file_data *sourceFile = *vm.mdata.files.end();
           sourceFile->name = next_string();

           if(vm.manifest.debug) {
               string sourceFileData(next_chars(next_int32()));
               parseSourceFile(*sourceFile, sourceFileData);
           }
           break;
       }, // on section_end
           break;
    )
}
