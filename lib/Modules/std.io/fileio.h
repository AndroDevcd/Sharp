//
// Created by bknun on 5/26/2018.
//

#ifndef SHARP_FILEIO_H
#define SHARP_FILEIO_H

#include "../../runtime/List.h"

void resolve_path(string& path, string&);


int check_access(string& path, int access_flag);

uInt get_file_attrs(string& path);

Int last_update(string& path, int tm_request);

Int file_size(string &path);

void create_file(string &path);

long delete_file(string &path);

void get_file_list(string &path, _List<string> &list);

long make_dir(string &path);

long delete_dir(string &path);

long rename_file(string &path, string &newName);

time_t update_time(string &path, time_t time);

int __chmod(string &path, mode_t set_mode, bool enable, bool userOnly);

long long disk_space(long request);

void current_directory(string &path);

void read_file(string &path, string &outStr);

#endif //SHARP_FILEIO_H
