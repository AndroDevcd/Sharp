//
// Created by BNunnally on 8/31/2021.
//
#include "compiler_info.h"
#include "sharp_file.h"
#include "backend/types/sharp_module.h"
#include "settings/settings.h"
#include "taskdelegator/task_delegator.h"
#include "backend/types/types.h"

bool panic = false;
recursive_mutex globalLock;
List<sharp_module*> modules;
List<sharp_file*> sharpFiles;
List<sharp_class*> classes;
List<sharp_class*> genericClasses;
impl_location *alternateLocation = NULL; // only used for generic classes
thread_local sharp_module* currModule = NULL;
List<string> warnings;
component_manager componentManager;
atomic<uInt> uniqueId = { 0 };
sharp_function *user_main_method = NULL;
sharp_function *genesis_method = NULL;
int main_method_sig = 0;

void create_new_warning(error_type error, int type, Ast *ast, string xcmnts) {
    create_new_warning(error, type, ast->line, ast->col, xcmnts);
}

void create_new_warning(error_type error, int type, int line, int col, string xcmnts) {
    GUARD(globalLock)

    if(options.warnings && !warnings.find(xcmnts)) {
        if(warning_options[type]) {
            if(options.werrors){
                create_new_error(error, line, col, xcmnts);
            } else {
                warnings.add(xcmnts);
                currThread->currTask->file->errors->createNewWarning(error, line, col, xcmnts);
            }
        }
    }
}

bool create_new_error(error_type error, Token &t, string xcmnts) {
    return create_new_error(error, t.getLine(), t.getColumn(), xcmnts);
}

bool create_new_error(error_type error, Ast *ast, string xcmnts) {
    return create_new_error(error, ast->line, ast->col, xcmnts);
}

bool create_new_error(error_type error, int line, int col, string xcmnts) {
    GUARD(globalLock)
    sharp_class *primary = get_primary_class(&current_context);
    bool reported = currThread->currTask->file->errors->createNewError(error, line, col, xcmnts);

    if(primary != NULL && primary->genericBuilder != NULL) {
        print_impl_location(primary->fullName, "generic class", primary->implLocation);

        for(Int i = (Int)current_context.storedItems.size() - 1; i >= 0; i--) {
            stored_context_item *contextItem = current_context.storedItems.get(i);
            if(contextItem->type == class_context
               || contextItem->type == global_context) {
                sharp_class *sc = contextItem->classCxt;

                if(sc != primary && sc->genericBuilder != NULL) {
                    print_impl_location(sc->fullName, "generic class", sc->implLocation);
                }
            }
        }
    }

    return reported;
}

bool all_files_parsed() {
    for(Int i = 0; i < sharpFiles.size(); i++) {
        if(sharpFiles.get(i)->p == NULL || !sharpFiles.get(i)->p->parsed)
            return false;
    }

    return true;
}

bool all_files_compiled_successfully() {
    for(Int i = 0; i < sharpFiles.size(); i++) {
        if(sharpFiles.get(i)->errors->hasErrors())
            return false;
    }

    return true;
}
