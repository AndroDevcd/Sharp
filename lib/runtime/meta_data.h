//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_META_DATA_H
#define SHARP_META_DATA_H

#include "../../stdimports.h"
#include "../util/linked_list.h"

struct file_data {
    file_data()
    :
        name(),
        lines()
    {}

    string name;
   linkedlist<string> lines;
};

struct meta_data {
    meta_data()
    :
        files()
    {}

    linkedlist<file_data*> files;
};

void init_struct(file_data *fd);
string get_info(file_data *file, Int line);
string get_line_info(file_data *file, Int line);

#endif //SHARP_META_DATA_H
