//
// Created by bknun on 9/16/2022.
//

#include "manifest_processor.h"
#include "exe_processor.h"
#include "../../core/exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"

void process_manifest(KeyPair<int, string> &result) {
    check_section(manif, CORRUPT_MANIFEST, result);

    int processedFlags = 0;
    PROCESS_SECTION(eoh,

         case 0x2:
            next_string(vm.mf.application);
            processedFlags++;
            continue;
         case 0x4:
             next_string(vm.mf.version);
             processedFlags++;
             continue;
         case 0x5:
             vm.mf.debug = next_char() == '1';
             processedFlags++;
             continue;
         case 0x6:
             vm.mf.entryMethod = next_int32();
             processedFlags++;
             continue;
         case 0x7:
             vm.mf.methods = next_int32();
             processedFlags++;
             continue;
         case 0x8:
             vm.mf.classes = next_int32();
             processedFlags++;
             continue;
         case 0x9:
             vm.mf.fvers = parse_int();
             processedFlags++;
             continue;
         case 0x0c:
             vm.mf.strings = next_int32();
             processedFlags++;
             continue;
         case 0x0e:
             vm.mf.target = parse_int();
             processedFlags++;
             continue;
         case 0x0f:
             vm.mf.sourceFiles = next_int32();
             processedFlags++;
             continue;
         case 0x1b:
             vm.mf.threadLocals = next_int32();
             processedFlags++;
             continue;
         case 0x1c:
             vm.mf.constants = next_int32();
             processedFlags++;
             continue;
         case 0x2a:
             vm.mf.applicationId = next_int32();
             processedFlags++;
             continue;
         , // on section_end

         if(processedFlags != HEADER_SIZE) {
             result.key = CORRUPT_MANIFEST;
             stringstream ss;
             ss << processedFlags << " header size";
             throw runtime_error(ss.str());
         }
         else if(vm.mf.target > BUILD_VERS) {
             result.key = UNSUPPORTED_BUILD_VERSION;
             throw runtime_error("build vers");
         }

         if(!(vm.mf.fvers >= min_file_vers && vm.mf.fvers <= file_vers)) {
             stringstream err;
             err << "unsupported file version of: " << vm.mf.fvers << ". Sharp supports file versions from `"
                 << min_file_vers << "-" << file_vers << "` that can be processed. Are you possibly targeting the incorrect virtual machine?";

             result.with(UNSUPPORTED_FILE_VERS,  err.str());
             throw runtime_error("unsupported");
         }

         break;
    )
}
