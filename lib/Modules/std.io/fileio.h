//
// Created by bknun on 5/26/2018.
//

#ifndef SHARP_FILEIO_H
#define SHARP_FILEIO_H

#include "../../runtime/oo/string.h"

extern native_string resolve_path(native_string& path);


extern int check_access(native_string& path, int access_flag);

extern long get_file_attrs(native_string& path);

#endif //SHARP_FILEIO_H
