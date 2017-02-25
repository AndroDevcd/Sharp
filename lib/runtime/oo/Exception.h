//
// Created by bknun on 2/24/2017.
//

#ifndef SHARP_EXCEPTION_H
#define SHARP_EXCEPTION_H

#include <stdexcept>

class ClassObject;

class Exception : public std::runtime_error {
public:
    Exception(const char *msg) : runtime_error(msg){}
    Exception(const std::string &__arg)
            :
            throwable(NULL), // native exception
            runtime_error(__arg)
    {}
    Exception(ClassObject* throwable, const std::string &__arg)
            :
            throwable(throwable),
            runtime_error(__arg)
    {
    }

    ClassObject* throwable;
};

#endif //SHARP_EXCEPTION_H
