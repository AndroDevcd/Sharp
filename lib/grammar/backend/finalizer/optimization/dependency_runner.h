//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_DEPENDENCY_RUNNER_H
#define SHARP_DEPENDENCY_RUNNER_H

#include "../../dependency/dependancy.h"

void markRelevantDependencyMembers();
void mark(sharp_function *fun);

#endif //SHARP_DEPENDENCY_RUNNER_H
