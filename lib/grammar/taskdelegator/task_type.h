//
// Created by BNunnally on 8/30/2021.
//

#ifndef SHARP_TASK_TYPE_H
#define SHARP_TASK_TYPE_H

enum task_type {
    task_none_ = 0,
    task_tokenize_ = 1,
    task_parse_ = 2,
    task_preprocess_ = 3,
    task_post_process_ = 4,
    task_compile_components_ = 5,
    task_process_delegates_ = 6
};

#endif //SHARP_TASK_TYPE_H
