//
// Created by BNunnally on 8/31/2021.
//
#include "compiler_info.h"
#include "sharp_file.h"
#include "backend/types/sharp_module.h"
#include "settings/settings.h"
#include "taskdelegator/task_delegator.h"

bool panic = false;
recursive_mutex globalLock;
List<sharp_module*> modules;
List<sharp_file*> sharpFiles;
List<sharp_class*> classes;
List<sharp_class*> genericClasses;
thread_local sharp_module* currModule = NULL;
List<string> warnings;

void create_new_warning(error_type error, int type, int line, int col, string xcmnts) {
    GUARD(globalLock)

    if(options.warnings && !warnings.find(xcmnts)) {
        if(warning_options[type]) {
            if(options.werrors){
                currThread->currTask->file->errors->createNewError(error, line, col, xcmnts);
            } else {
                warnings.add(xcmnts);
                currThread->currTask->file->errors->createNewWarning(error, line, col, xcmnts);
            }
        }
    }
}