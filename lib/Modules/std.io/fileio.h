//
// Created by bknun on 5/26/2018.
//

#ifndef SHARP_FILEIO_H
#define SHARP_FILEIO_H

#include "../../runtime/symbols/string.h"
#include "../../runtime/List.h"

native_string resolve_path(native_string& path);


int check_access(native_string& path, int access_flag);

Int get_file_attrs(native_string& path);

Int last_update(native_string& path, int tm_request);

Int file_size(native_string &path);

void create_file(native_string &path);

long delete_file(native_string &path);

void get_file_list(native_string &path, _List<native_string> &list);

long make_dir(native_string &path);

long delete_dir(native_string &path);

long rename_file(native_string &path, native_string &newName);

time_t update_time(native_string &path, time_t time);

int __chmod(native_string &path, mode_t set_mode, bool enable, bool userOnly);

long long disk_space(long request);

#endif //SHARP_FILEIO_H
