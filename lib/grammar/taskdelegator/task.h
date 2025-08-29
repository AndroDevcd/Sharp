//
// Created by BNunnally on 8/30/2021.
//

#ifndef SHARP_TASK_H
#define SHARP_TASK_H

#include "task_type.h"
#include "../sharp_file.h"

struct task {
    task()
    :
        file(NULL),
        type(task_none_)
    {}

    task(const task &t)
            :
            file(t.file),
            type(t.type)
    {}

    sharp_file *file;
    task_type type;
};

#endif //SHARP_TASK_H
