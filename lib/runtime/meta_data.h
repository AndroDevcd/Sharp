//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_META_DATA_H
#define SHARP_META_DATA_H

#include "../../stdimports.h"

struct file_data {
    file_data()
    :
        name(""),
        lines()
    {}

    string name;
    list<string> lines;
};

struct meta_data {
    meta_data()
    :
        files()
    {}

    list<file_data*> files;
};

void init_struct(file_data *fd);

#endif //SHARP_META_DATA_H
