//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_ENVIRONMENT_H
#define SHARP_ENVIRONMENT_H

#include "List.h"
#include "oo/string.h"
#include "oo/ClassObject.h"
#include "oo/Method.h"

class Object;
class Method;
class ArrayObject;

class Environment {
public:
    Environment()
            :
            globalHeap(NULL),
            classes(NULL),
            strings(NULL),
            methods(NULL)
    {
    }

    int CallMainMethod(Method*, void*, int);
    void DropLocals();
    ClassObject* findClass(string name);
    ClassObject* tryFindClass(string name);
    ClassObject* findClassBySerial(int64_t id);
    //void checkClassCast()

    // TODO: create the aux classes to be used internally
    // Alloce'd by new()
    static ClassObject *Throwable;
    static ClassObject *StackOverflowErr;
    static ClassObject *RuntimeErr; // TODO: compare exceptions by name not id
    static ClassObject *ThreadStackException;
    static ClassObject *IndexOutOfBoundsException;
    static ClassObject *NullptrException;
    static ClassObject *ClassCastException;

    Object* globalHeap;                                 /* Mainly static classes/variables etc */
    Object* throwable;          /* Used for crashes */

    ClassObject* classes;

    native_string* sourceFiles;
    String* strings;
    Method* methods;

    void shutdown();

    native_string& getStringById(int64_t ref);

    Object* findField(std::string name, SharpObject *object);
};

extern Environment* env;

#define mvers versions.ALPHA

#define AUX_CLASSES 7

#endif //SHARP_ENVIRONMENT_H
