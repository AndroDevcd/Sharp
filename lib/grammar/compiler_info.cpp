//
// Created by BNunnally on 8/31/2021.
//
#include "compiler_info.h"
#include "sharp_file.h"
#include "backend/types/sharp_module.h"

bool panic = false;
recursive_mutex globalLock;
List<sharp_module*> modules;
List<sharp_file*> sharpFiles;
List<sharp_class*> classes;
List<sharp_class*> genericClasses;
thread_local sharp_module* currModule = NULL;
