//
// Created by bknun on 5/26/2018.
//

#ifndef SHARP_FILEIO_H
#define SHARP_FILEIO_H

#include "../../runtime/oo/string.h"

native_string resolve_path(native_string& path);


int check_access(native_string& path, int access_flag);

long long get_file_attrs(native_string& path);

long long last_update(native_string& path);

long long file_size(native_string &path);

void create_file(native_string &path);

long delete_file(native_string &path);

List<native_string> get_file_list(native_string &path);

long make_dir(native_string &path);

long delete_dir(native_string &path);

long rename_file(native_string &path, native_string &newName);

#endif //SHARP_FILEIO_H
