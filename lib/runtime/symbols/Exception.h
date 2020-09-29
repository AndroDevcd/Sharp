//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_EXCEPTION_H
#define SHARP_EXCEPTION_H

#include "../../../stdimports.h"
#include "string.h"
#include <stdexcept>

class ClassObject;
class Method;

class Throwable {
public:
    Throwable()
            :
            handlingClass(NULL),
            native(false)
    {
        message.init();
        this->stackTrace.init();
    }

    void init() {
        handlingClass =NULL;
        message.init();
        this->stackTrace.init();
        native = false;
    }

    ~Throwable() {
        drop();
    }

    Throwable(ClassObject* throwable, const std::string &message, bool native)
            :
            handlingClass(throwable),
            native(native)
    {
        this->message.init();
        this->stackTrace.init();
        this->message=message;
    }

    void init(ClassObject* throwable, const char* message, bool native) {
        this->handlingClass = throwable;
        this->native = native;
        this->message.init();
        this->stackTrace.init();

        this->message.set(message);
    }

    void operator=(Throwable& t) {
        handlingClass=t.handlingClass;
        message = t.message;
        stackTrace = t.stackTrace;
        native = t.native;
    }

    void drop();

    bool native;
    ClassObject* handlingClass;
    native_string message, stackTrace;
};

class Exception : public std::runtime_error {
public:
    Exception(const char *msg, bool native = true);
    Exception(const std::string &__arg, bool native = true);
    Exception(ClassObject* throwable, const std::string &__arg, bool native = true);
    Exception(Throwable& throwable);

    ~Exception();

    void operator=(Exception e) {
        throwable=e.throwable;
    }

    Throwable& getThrowable() { return throwable; }

    Throwable throwable;

private:
    void pushException();
};

#define EXCEPTION_PRINT_MAX 20

#endif //SHARP_EXCEPTION_H
