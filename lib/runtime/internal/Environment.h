//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_ENVIRONMENT_H
#define SHARP_ENVIRONMENT_H

#include "../oo/string.h"
#include "../interp/FastStack.h"
#include "../OO/ClassObject.h"

class gc_object;
class Method;
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

    void newClass(int64_t,int64_t);
    void newClass(gc_object*,int64_t);
    void newNative(gc_object*, int8_t);
    void newArray(gc_object*, int64_t);
    void newRefrence(gc_object*);

    // TODO: create the aux classes to be used internally
    // Alloce'd by new()
    static ClassObject Throwable;
    static ClassObject StackOverflowErr;
    static ClassObject RuntimeException; // TODO: compare exceptions by name not id
    static ClassObject ThreadStackException;
    static ClassObject IndexOutOfBoundsException;
    static ClassObject NullptrException;

    gc_object* objects;

    Method* methods;
    ClassObject* classes;
    String* strings;
    int64_t *bytecode;

    void shutdown();

    static void init(gc_object*,int64_t);

    static void free(gc_object*, int64_t);
};

extern Environment* env;

#define mvers 1


#endif //SHARP_ENVIRONMENT_H
