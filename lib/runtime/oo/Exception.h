//
// Created by bknun on 2/24/2017.
//

#ifndef SHARP_EXCEPTION_H
#define SHARP_EXCEPTION_H

#include "../../../stdimports.h"
#include <stdexcept>

class ClassObject;

class Throwable {
public:
    Throwable()
            :
            throwable(NULL),
            message("")
    {
    }

    Throwable(ClassObject* throwable, string message)
            :
            throwable(throwable),
            message(message)
    {
    }

    string buildMessage(list<Method*> callStack);

    ClassObject* throwable;
    string message;
};

class Exception : public std::runtime_error {
public:
    Exception(const char *msg) : runtime_error(msg)
    {
        this->msg = string(msg);
    }
    Exception(const std::string &__arg)
            :
            throwable(NULL), // native exception
            runtime_error(__arg)
    {
        msg = __arg;
    }
    Exception(ClassObject* throwable, const std::string &__arg)
            :
            throwable(throwable),
            runtime_error(__arg)
    {
        msg = __arg;
    }

    Throwable getThrowable() { return Throwable(throwable, msg); }

    string msg;
    ClassObject* throwable;
};

#endif //SHARP_EXCEPTION_H
