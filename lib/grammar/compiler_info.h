//
// Created by bnunnally on 8/27/21.
//

#ifndef SHARP_COMPILER_INFO_H
#define SHARP_COMPILER_INFO_H

#include "List.h"
#include "frontend/ErrorManager.h"
#include "backend/dependency/component_manager.h"
#include "taskdelegator/task_delegator.h"
#include <atomic>

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
extern impl_location *alternateLocation;
extern thread_local sharp_module* currModule;
extern component_manager componentManager;
extern atomic<uInt> uniqueId;
extern sharp_function *user_main_method;
extern sharp_function *genesis_method;
extern int main_method_sig;

#define global_class_name "__srt_global"
#define main_component_name "__main__"
#define static_init_function_name "static_init"
#define tls_init_function_name "tls_init"
#define platform_class_name "platform.kernel#platform"
#define instance_init_name(name) ("init<" + (name) + ">")
#define static_init_name(name)  ("static_init<" + (name) + ">")
#define set_internal_label_name(ss, name, id)  \
            (ss).str("");                                \
            (ss) << "$$01_internal_label_" << (name) << (id);
#define set_internal_variable_name(ss, name, id)  \
            (ss).str("");                                \
            (ss) << "$$02_internal_field_" << (name) << (id);
#define set_internal_function_pointer_type_name(ss, name, id)  \
            (ss).str("");                                \
            (ss) << "$$03_internal_fptr_" << (name) << (id);
#define any_component_name "?"
#define single_component_field_name_prefix "@sub_component_"
#define anonymous_func_prefix "@anonymous_fun_"
#define current_file currThread->currTask->file
#define error_manager currThread->currTask->file->errors
#define current_context currThread->currTask->file->context
#define invalid_block_id (-1)
#define valid_block_id(id) ((id) >= 0)

#define GUARD(mut) \
    std::lock_guard<recursive_mutex> guard(mut);

#define GUARD2(mut) \
    std::lock_guard<recursive_mutex> guard2(mut);

bool create_new_error(error_type error, Token &t, string xcmnts);
bool create_new_error(error_type error, int line, int col, string xcmnts);
void create_new_warning(error_type error, int type, int line, int col, string xcmnts);
void create_new_warning(error_type error, int type, Ast *ast, string xcmnts);
bool create_new_error(error_type error, Ast *ast, string xcmnts);
bool all_files_parsed();
bool all_files_compiled_successfully();

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
