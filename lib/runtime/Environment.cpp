//
// Created by BraxtonN on 2/15/2018.
//

#include "Environment.h"
#include "Exe.h"
#include "oo/Exception.h"
#include "oo/Object.h"
#include "memory/GarbageCollector.h"
#include "oo/Field.h"
#include "Manifest.h"

ClassObject *Environment::Throwable;
ClassObject *Environment::StackOverflowErr;
ClassObject *Environment::RuntimeErr;
ClassObject *Environment::ThreadStackException;
ClassObject *Environment::IndexOutOfBoundsException;
ClassObject *Environment::NullptrException;
ClassObject *Environment::ClassCastException;

ClassObject *Environment::findClass(string name) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].name.str() == name)
            return &env->classes[i];
    }

    throw Exception("class not found `" + name + "'");
}

void Environment::shutdown() {

}

native_string& Environment::getStringById(int64_t ref) {
    if(ref < 0 || ref >= manifest.strings)
        return strings[0].value;
    else if(ref == strings[ref].id)
        return strings[ref].value;
    else {
        for(unsigned int i = 0; i < manifest.strings; i++) {
            if(strings[i].id == ref)
                return strings[i].value;
        }
    }

    return strings[0].value;
}

Object* Environment::findField(std::string name, SharpObject *object) {
    if(object == NULL || !IS_CLASS(object->info))
        return NULL;

    int64_t index;
    if((index = env->classes[CLASS(object->info)].fieldindex(name)) != -1) {
        return &object->node[index];
    }

    return NULL;
}

void Environment::createString(Object *o, native_string &str) {
    if(o->object->HEAD != NULL) {
        str.free();
        for(long i = 0; i < o->object->size; i++) {
            str += o->object->HEAD[1];
        }
    }
}

double Environment::__sgetFieldVar(string field, SharpObject *object) {
    Object *FIELD = env->findField(field, object);

    if(FIELD != NULL && FIELD->object != NULL
       && FIELD->object->HEAD != NULL) {
        return FIELD->object->HEAD[0];
    }
    return 0;
}
