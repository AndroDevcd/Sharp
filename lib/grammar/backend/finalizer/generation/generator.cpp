//
// Created by bknun on 8/8/2022.
//

#include "generator.h"
#include "../optimization/optimizer.h"
#include "file_generator.h"
#include "../../../compiler_info.h"

void generate() {
    for(Int i = 0; i < compressedCompilationFiles.size(); i++) {
        generate(compressedCompilationFiles.get(i));
    }
}

void generation_error(string message) {
    cout << PROG_NAME << ": <code-generator> fatal error: " << message << endl;
    exit(1);
}