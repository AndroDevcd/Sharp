//
// Created by bknun on 2/24/2017.
//

#ifndef SHARP_EXCEPTION_H
#define SHARP_EXCEPTION_H

#include "../../../stdimports.h"
#include "string.h"
#include <stdexcept>

class ClassObject;
class Method;
class CallStack;

class Throwable {
public:
    Throwable()
            :
            throwable(NULL),
            message()
    {
    }

    Throwable(ClassObject* throwable, string message)
            :
            throwable(throwable),
            message(message)
    {
    }

    string buildMessage(CallStack &cs);

    void drop();

    ClassObject* throwable;
    nString message;
};

class Exception : public std::runtime_error {
public:
    Exception(const char *msg);
    Exception(const std::string &__arg);
    Exception(ClassObject* throwable, const std::string &__arg);

    Throwable getThrowable() { return Throwable(throwable, msg); }

    string msg;
    ClassObject* throwable;
};

#endif //SHARP_EXCEPTION_H
