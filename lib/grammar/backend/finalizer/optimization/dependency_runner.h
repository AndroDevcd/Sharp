//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_DEPENDENCY_RUNNER_H
#define SHARP_DEPENDENCY_RUNNER_H

#include "../../dependency/dependancy.h"

void markRelevantDependencyMembers();
void validate_required_dependencies();

void mark(sharp_field *sf);
void mark(sharp_function *fun);
void mark(sharp_class *sc);
void mark(sharp_file *sf);

#endif //SHARP_DEPENDENCY_RUNNER_H
