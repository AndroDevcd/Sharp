//
// Created by bknun on 8/8/2022.
//

#include "optimizer.h"
#include "../../../sharp_file.h"
#include "../../../compiler_info.h"
#include "dependency_runner.h"
#include "field_injector.h"
#include "obfuscator.h"
#include "../../../settings/settings.h"
#include "codemotion/code_motion.h"
#include "../generation/generator.h"
#include "initialization_validator.h"

// this field represents a compressed list of all source files to only include the source files that the user
// code depends on
List<sharp_file*> compressedCompilationFiles;
optimize_context octx;

void optimize() {
    if(genesis_method != NULL && user_main_method != NULL) {
        markRelevantDependencyMembers();
        markExplicitObfuscatedItems();
        injectAllRelevantFields();
        validateNonNullableInitializations();
        pre_generate_addresses();
        validate_required_dependencies();

//        if(options.optimize_level == high_performance_optimization) {
//            optimize_code_motion();
//        }
//
//        if(options.optimize_level >= basic_optimization) {
//            // todo:
//        }
    } else {
        create_new_error(INTERNAL_ERROR, sharpFiles.get(0)->p->astAt(0),
                                             "could not locate genesis/main method!");
    }
}

void add_variable_assignment(sharp_field *field, operation_schema *schema) {
    for(Int i = 0; i < octx.variableAssignments.size(); i++) {
        if(octx.variableAssignments[i]->field == field) {
            octx.variableAssignments[i]->assignSchemes.addif(schema);
            return;
        }
    }

    octx.variableAssignments.add(new variable_assignment_context(field, schema));
}

void setup_context(
        sharp_function *sub
) {
    octx.subject = sub;
}

void flush_octx() {
    deleteList(octx.variableAssignments);
    octx.processedSchemes.free();
}
