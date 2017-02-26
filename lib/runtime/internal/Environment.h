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
class Object;
class ArrayObject;

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
    ClassObject* tryFindClass(string name);
    ClassObject* findClass(int64_t id);

    static ClassObject* newClass(int64_t);

    static ClassObject* nilClass;
    static Object* nilObject;
    static ArrayObject* nilArray;

    gc_object* objects;

    Method* methods;
    ClassObject* classes;
    String* strings;
    double* bytecode;

    void shutdown();
};

extern Environment* env;

#define mvers 1


#endif //SHARP_ENVIRONMENT_H
