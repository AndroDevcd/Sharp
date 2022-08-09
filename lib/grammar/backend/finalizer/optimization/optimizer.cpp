//
// Created by bknun on 8/8/2022.
//

#include "optimizer.h"
#include "../../../sharp_file.h"
#include "../../../compiler_info.h"
#include "dependency_runner.h"

// this field represents a compressed list of all source files to only include the source files that the user
// code depends on
List<sharp_file*> compressedCompilationFiles;

void optimize() {
    if(genesis_method != NULL && user_main_method != NULL) {
        markRelevantDependencyMembers();

        // todo optimize code later
    } else {
        current_file->errors->createNewError(INTERNAL_ERROR, sharpFiles.get(0)->p->astAt(0),
                                             "could not locate genesis/main method!");
    }
}