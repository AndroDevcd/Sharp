//
// Created by bnunnally on 8/30/21.
//
#include "task_delegator.h"
#include "../../util/File.h"
#include "../compiler_info.h"
#include "../backend/preprocessor/class_preprocessor.h"

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
        if(currThread->nextTask != NULL) {
            currThread->currTask = currThread->nextTask;
            currThread->nextTask = NULL;
        }

        if(currThread->currTask != NULL) {
            execute_task();
            currThread->currTask = NULL;
            currThread->state = worker_idle;
        }

#ifdef WIN32_
        Sleep(50);
#endif
#ifdef POSIX_
        usleep(50*POSIX_USEC_INTERVAL);
#endif
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
    }

    file->stage = tokenized;
}

void parse_file() {
    sharp_file *file = currThread->currTask->file;

    file->p = new parser(file->tok);
    if(file->p->getErrors()->hasErrors()) {
        GUARD(errorMutex)

        file->compilationFailed = true;
        file->p->getErrors()->printErrors();
        if(file->p->panic)
            panic = true;
        delete file->p; file->p = NULL;
    }

    file->stage = parsed;
}

void pre_process_class_() {
    sharp_file *file = currThread->currTask->file;

    pre_process_class();
    file->stage = classes_preprocessed;
}

void execute_task() {
    {
        GUARD(errorMutex);
        cout << "executing task(" << currThread->currTask->type << ") on: " << currThread->currTask->file->name << endl;
    }
    switch(currThread->currTask->type) {
        case task_none_: { break; }
        case task_tokenize_: {
            tokenize_file();
            break;
        }
        case task_parse_: {
            parse_file();
            break;
        }
        case task_preprocess_class_: {
            break;
        }
    }
}