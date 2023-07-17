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
            sourceFile.lines.createnode(line);
            line = "";
        } else {
            line += data[i];
        }
    }

    sourceFile.lines.createnode(line);
}

void process_meta_data(KeyPair<int, string> &result) {
    check_section(smeta, CORRUPT_FILE, result);

    uInt itemsProcessed=0;
    PROCESS_SECTION(eos,

       case data_file: {
           auto fileData = new file_data();
           next_string(fileData->name);

           if(vm.mf.debug) {
               string sourceFileData;
               next_chars(sourceFileData, next_int32());
               parseSourceFile(*fileData, sourceFileData);
           }

           vm.mdata.files.createnode(fileData);
           continue;
       }, // on section_end
           break;
    )
}
