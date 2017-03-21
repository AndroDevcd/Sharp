//
// Created by BraxtonN on 2/17/2017.
//

#include "Environment.h"
#include "Exe.h"
#include "../oo/Method.h"
#include "../oo/Exception.h"
#include "../oo/ClassObject.h"
#include "../oo/Object.h"
#include "../oo/Array.h"
#include "../internal/Monitor.h"

ClassObject* Environment::Throwable = NULL;
ClassObject* Environment::StackOverflowErr = NULL;
ClassObject* Environment::RuntimeException = NULL;
ClassObject* Environment::ThreadStackException = NULL;
ClassObject* Environment::IndexOutOfBoundsException = NULL;
ClassObject* Environment::NullptrException = NULL;

Method *Environment::getMethod(int64_t id) {
    for(uint64_t i = 0; i < manifest.methods; i++) {
        if(env->methods[i].id == id)
            return &env->methods[i];
    }

    stringstream msg;
    msg << "method not found @id:" << id;
    throw Exception(msg.str());
}

ClassObject *Environment::findClass(string name) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].name.str() == name)
            return &env->classes[i];
    }

    throw Exception("class not found `" + name + "'");
}

ClassObject *Environment::tryFindClass(string name) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].name.str() == name)
            return &env->classes[i];
    }

    return NULL;
}

Method *Environment::getMethodFromClass(ClassObject *classObject, int64_t id) {
    if(classObject == NULL || id >= classObject->methodCount) {
        stringstream msg;
        msg << "method not found @id:" << id;
        throw Exception(msg.str());
    }

    return &classObject->methods[id];
}

ClassObject *Environment::findClass(int64_t id) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].id == id)
            return &env->classes[i];
    }

    stringstream msg;
    msg << "class not found @id:" << id;
    throw Exception(msg.str());
}

void Environment::shutdown() {
    updateStackFile("Destroying environment");
    std::free (this->bytecode);
    for(int64_t i = 0; i < manifest.strings; i++)
        this->strings->value = "";
    std::free (this->strings);

    for(int64_t i = 0; i < manifest.classes; i++)
        this->classes->free();
    std::free (this->classes);

    // TODO: free objects and aux classes
}

void Environment::newClass(int64_t object, int64_t klass) {
    if(objects[object].mark == gc_green)
        objects[object].free();

    objects[object].type = classobject;
    objects[object].mark = gc_green;
}

void Environment::newClass(gc_object* object, int64_t klass) {
    if(object->mark == gc_green)
        object->free();

    object->mark = gc_green;
}

void Environment::init(gc_object* objects, int64_t size) {
    for(int64_t i = 0; i < size; i++) {
        objects[i].mark = gc_orange;
        objects[i].monitor = new Monitor();
    }
}

void Environment::newNative(gc_object *object, int8_t type) {
    if(object->mark == gc_green)
        object->free();

    object->type = (Type)type;
    switch(type) {
        case nativeint:
        case nativeshort:
        case nativelong:
        case nativechar:
        case nativebool:
        case nativefloat:
        case nativedouble:
            return;
        default:
            object->type = nilobject;
            return;
    }
}

void Environment::newArray(gc_object *object, int64_t len) {
    if(object->mark == gc_green)
        object->free();

}

void Environment::newRefrence(gc_object *object) {
    if(object->mark == gc_green)
        object->free();

    object->type = refrenceobject;
}

void Environment::free(gc_object *objects, int64_t len) {
    if(len > 0 && objects != NULL) {
        for(int64_t i = 0; i < len; i++) {
            //objects[i].invalidate();
            //objects[i].free();
            //objects[i].refs->clear();
            //std::free (objects[i].refs);

            //objects[i].monitor->~Monitor();
        }
        //std::free (objects);
    }
}
