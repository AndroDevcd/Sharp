//
// Created by BraxtonN on 2/17/2017.
//

#include "Environment.h"
#include "Exe.h"
#include "../oo/Method.h"
#include "../oo/Exception.h"
#include "../oo/ClassObject.h"

ClassObject* Environment::nilClass = NULL;
Object* Environment::nilObject = NULL;
ArrayObject* Environment::nilArray = NULL;

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
    cout << "out" << endl;
    delete (this->bytecode);
    cout << "out" << endl;
    delete (this->strings);

    cout << "out" << endl;
    for(int64_t i = 0; i < manifest.classes; i++)
        this->classes->free();
    cout << "out" << endl;
    delete (this->classes);
    cout << "out" << endl;
}
