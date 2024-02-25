//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_OPTIMIZER_H
#define SHARP_OPTIMIZER_H

#include "../../../sharp_file.h"

extern List<sharp_file*> compressedCompilationFiles;

void optimize();

struct variable_assignment_context {
    sharp_field *field;
    List<operation_schema*> assignSchemes;

    variable_assignment_context(
        sharp_field *field,
        operation_schema *schema
    )
        :
        field(field),
        assignSchemes()
    {
        assignSchemes.add(schema);
    }
};

struct optimize_context {
    sharp_function* subject;
    List<variable_assignment_context*> variableAssignments;
    List<operation_schema *> processedSchemes;
};

void add_variable_assignment(sharp_field *field, operation_schema *schema);

void setup_context(
    sharp_function *sub
);

void flush_octx();

extern optimize_context octx;

#endif //SHARP_OPTIMIZER_H
