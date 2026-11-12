//
// Created by bnunnally on 8/30/21.
//
#include "task_delegator.h"
#include "../compiler_timings.h"
#include "../../util/File.h"
#include "../compiler_info.h"
#include "../backend/preprocessor/class_preprocessor.h"
#include "../backend/postprocessor/class_processor.h"
#include "../backend/postprocessor/delegate_processor.h"
#include "../backend/compiler/di/component_compiler.h"
#include "../backend/compiler/compiler.h"
#include "../backend/compiler/mutate_compiler.h"
#include "../backend/compiler/class_compiler.h"
#include "../backend/postprocessor/import_processor.h"
#include "../settings/settings.h"

thread_local worker_thread *currThread;

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
workerStart(void *arg) {
    currThread = (worker_thread*)arg;
    currThread->state = worker_idle;

    do {
        if(currThread->state == worker_dead)
            break;

        if(currThread->nextTask != NULL) {
            currThread->currTask = currThread->nextTask;
            currThread->nextTask = NULL;
        }

        if(currThread->currTask != NULL) {
            execute_task();
            currThread->currTask = NULL;
            currThread->state = worker_idle;
            continue;   // look for the next task right away
        }

        // Nothing to do. This used to sleep 5 ms after EVERY task; a compile
        // hands out several hundred tasks (files x stages), so the naps added
        // up to seconds in which nothing was computed (see sharpc -timings).
        task_idle_wait();
    } while(true);

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void tokenize_file() {
    sharp_file *file = currThread->currTask->file;

    File::buffer buf;
    File::read_alltext(file->name.c_str(), buf);
    string data = buf.to_str();
    buf.end();

    file->tok = new tokenizer(data, file->name);
    if(file->tok->getErrors()->hasErrors()) {
        GUARD(errorMutex)

        file->compilationFailed = true;
        file->tok->getErrors()->printErrors();
        delete file->tok; file->tok = NULL;
        remove_all_posted_tasks(file);
    }

    file->stage = tokenized;
}

void parse_file() {
    sharp_file *file = currThread->currTask->file;

    file->p = new parser(file->tok);
    if(file->p->getErrors() && file->p->getErrors()->hasErrors()) {
        GUARD(errorMutex)

        file->compilationFailed = true;
        file->p->getErrors()->printErrors();
        if(file->p->panic)
            panic = true;
        delete file->p; file->p = NULL;
        remove_all_posted_tasks(file);
    } else if(file->p->parsed)
        file->stage = parsed;
}

void pre_process_() {
    sharp_file *file = currThread->currTask->file;

    pre_process();
    file->stage = classes_preprocessed;
}

void pre_process_imports_() {
    sharp_file *file = currThread->currTask->file;

    process_imports();
    file->stage = imports_processed;
}

void post_process_() {
    sharp_file *file = currThread->currTask->file;

    post_process();
    file->stage = classes_post_processed;
}

void process_generics_() {
    sharp_file *file = currThread->currTask->file;

    process_generics();
    file->stage = generics_processed;
}

void process_delegates_() {
    sharp_file *file = currThread->currTask->file;

    process_delegates();
    file->stage = class_delegates_processed;
}

void compile_components_() {
    sharp_file *file = currThread->currTask->file;

    compile_components();
    file->stage = components_processed;
}

void compile_mutations_() {
    sharp_file *file = currThread->currTask->file;

    compile_global_mutations(file);
    file->stage = mutations_compiled;
}

void compile_members_() {
    sharp_file *file = currThread->currTask->file;

    compile_global_members();
    file->stage = members_compiled;
}

void compile_classes_() {
    sharp_file *file = currThread->currTask->file;

    compile_classes();
    file->stage = compiled;
}

void execute_task() {
    {
        GUARD(errorMutex);
        if(options.debug_mode)
            cout << "executing task(" << currThread->currTask->type << ") on: " << currThread->currTask->file->name << endl;
    }
    switch(currThread->currTask->type) {
        case task_none_: { break; }
        case task_tokenize_: {
            TIME_STAGE("tokenize");
            tokenize_file();
            break;
        }
        case task_parse_: {
            TIME_STAGE("parse");
            parse_file();
            break;
        }
        case task_preprocess_: {
            TIME_STAGE("pre-process (declare classes)");
            pre_process_();
            break;
        }
        case task_process_imports: {
            TIME_STAGE("process imports");
            pre_process_imports_();
            break;
        }
        case task_post_process_: {
            TIME_STAGE("post-process (resolve types)");
            post_process_();
            break;
        }
        case task_process_generics_: {
            TIME_STAGE("process generics");
            process_generics_();
            break;
        }
        case task_compile_components_: {
            TIME_STAGE("compile components");
            compile_components_();
            break;
        }
        case task_process_delegates_: {
            TIME_STAGE("process delegates");
            process_delegates_();
            break;
        }
        case task_compile_mutations_: {
            TIME_STAGE("compile mutations");
            compile_mutations_();
            break;
        }
        case task_compile_global_members_: {
            TIME_STAGE("compile global members");
            compile_members_();
            break;
        }
        case task_compile_classes_: {
            TIME_STAGE("compile classes (function bodies)");
            compile_classes_();
            break;
        }
    }
}
