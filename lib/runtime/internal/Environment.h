//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_ENVIRONMENT_H
#define SHARP_ENVIRONMENT_H

#include "../oo/string.h"
#include "../interp/FastStack.h"

class gc_object;
class Method;
class ClassObject;

class Environment {
public:
    Environment()
    :
            objects(NULL),
            methods(NULL),
            classes(NULL),
            strings(NULL),
            bytecode(NULL)
    {
    }

    int CallMainMethod(Method*, void*, int);
    void DropLocals();
    Method* getMethod(int64_t id);
    Method* getMethodFromClass(ClassObject* classObject, int64_t id);
    ClassObject* findClass(string name);
    ClassObject* findClass(int64_t id);

    gc_object* objects;

    Method* methods;
    ClassObject* classes;
    String* strings;
    double* bytecode;
};

extern Environment* env;

#define mvers 1


#endif //SHARP_ENVIRONMENT_H
