//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_VM_EXCEPTION_H
#define SHARP_VM_EXCEPTION_H

#include "../../../stdimports.h"

struct sharp_class;

class vm_err {
public:
    vm_err()
            :
            handlingClass(NULL),
            internal(false),
            message(),
            stackTrace()
    {
    }

    void init() {
        handlingClass =NULL;
        internal = false;
    }

    ~vm_err() {
        handlingClass = NULL;
        internal = false;
        message = "";
        stackTrace = "";
    }

    vm_err(sharp_class* throwable, const std::string &message, bool internal)
            :
            handlingClass(throwable),
            internal(internal),
            message(),
            stackTrace()
    {
        this->message=message;
    }

    void init(sharp_class* throwable, const char* message, bool internal) {
        this->handlingClass = throwable;
        this->internal = internal;
        this->message = (message);
    }

    void operator=(vm_err& err) {
        handlingClass=err.handlingClass;
        message = err.message;
        stackTrace = err.stackTrace;
        internal = err.internal;
    }

    bool internal;
    sharp_class* handlingClass;
    string message, stackTrace;
};

class vm_exception : public std::runtime_error {
public:
    vm_exception(const char *msg, bool internal = false);
    vm_exception(const std::string &__arg, bool internal = false);
    vm_exception(sharp_class* throwable, const std::string &__arg, bool internal = false);
    vm_exception(vm_err& err);

    void operator=(vm_exception e) {
        err=e.err;
    }

    vm_err& getErr() { return err; }

    vm_err err;

private:
    void push_exception();
};

void fill_stack_trace(string &str);

#define EXCEPTION_PRINT_MAX 20

#endif //SHARP_VM_EXCEPTION_H
