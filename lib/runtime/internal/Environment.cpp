//
// Created by BraxtonN on 2/17/2017.
//

#include "Environment.h"
#include "Exe.h"
#include "../oo/Method.h"
#include "../oo/Exception.h"
#include "../oo/ClassObject.h"
#include "../oo/Object.h"
#include "../oo/Reference.h"

ClassObject* Environment::nilClass = NULL;
Object* Environment::nilObject = NULL;
ArrayObject* Environment::nilArray = NULL;
Reference* Environment::nilReference = NULL;

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
        if (env->classes[i].name == name)
            return &env->classes[i];
    }

    throw Exception("class not found `" + name + "'");
}

ClassObject *Environment::tryFindClass(string name) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].name == name)
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
}

void Environment::newClass(int64_t object, int64_t klass) {
    if(objects[object].mark == gc_green)
        objects[object].free();

    objects[object].mark = gc_green;
    objects[object].klass = findClass(klass)->newdup();
}

void Environment::init() {
    for(int64_t i = 0; i < manifest.classes; i++) {
        objects[i].mark = gc_green;
        objects[i].klass = classes[i].newdup();
    }
}
