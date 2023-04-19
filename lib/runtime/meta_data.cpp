//
// Created by bknun on 9/18/2022.
//

#include "meta_data.h"

void init_struct(file_data *fd) {
    new (&fd->name) string();
    new (&fd->lines) list<string>();
}
