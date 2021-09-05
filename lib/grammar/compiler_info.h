//
// Created by bnunnally on 8/27/21.
//

#ifndef SHARP_COMPILER_INFO_H
#define SHARP_COMPILER_INFO_H

#include "List.h"

#define PROG_NAME "sharp"
#define PROG_VERS "0.3.0"

#define opt(v) strcmp(args[i], v) == 0

struct sharp_file;
struct sharp_module;
struct sharp_class;

// globaly used vars
extern bool panic;
extern List<sharp_file*> sharpFiles;
extern List<sharp_module*> modules;
extern List<sharp_class*> classes;
extern List<sharp_class*> genericClasses;
extern recursive_mutex globalLock;
extern thread_local sharp_module* currModule;

#define global_class_name "__srt_global"

#define GUARD(mut) \
    std::lock_guard<recursive_mutex> guard(mut);


template<class T>
static void deleteList(List<T> &lst)
{
    for(unsigned int i = 0; i < lst.size(); i++)
    {
        delete lst.get(i);
    }

    lst.free();
}

#endif //SHARP_COMPILER_INFO_H
