//
// Created by bknun on 8/8/2022.
//

#include "optimizer.h"
#include "../../../sharp_file.h"
#include "../../../compiler_info.h"
#include "dependency_runner.h"
#include "field_injector.h"
#include "obfuscator.h"

// this field represents a compressed list of all source files to only include the source files that the user
// code depends on
List<sharp_file*> compressedCompilationFiles;

void optimize() {
    if(genesis_method != NULL && user_main_method != NULL) {
        markRelevantDependencyMembers();
        injectAllRelevantFields();
        markExplicitObfuscatedItems();

        // todo optimize code later
    } else {
        create_new_error(INTERNAL_ERROR, sharpFiles.get(0)->p->astAt(0),
                                             "could not locate genesis/main method!");
    }
}