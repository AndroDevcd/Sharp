//
// Created by BraxtonN on 2/7/2017.
//

#include "Environment.h"
#include "runtime.h"

void Environment::create_class(int_ClassObject obj) {
    for(int_ClassObject& o : *classes) {
        if(o.uid == obj.uid) {
            return;
        }
    }

    classes->push_back(obj);
}

int_Method::int_Method(const string &name, const string &module, ClassObject *klass, const list <Param> &params,
                       const list <AccessModifier> &modifiers, ClassObject *rtype, const RuntimeNote &note) : Method(
        name, module, klass, params, modifiers, rtype, note) {

}

int_Method::int_Method(string name, string module, ClassObject *klass, list <Param> params,
                       list <AccessModifier> modifiers, NativeField rtype, RuntimeNote note) : Method(name, module,
        klass, params, modifiers, rtype, note) {

}

void int_ClassObject::create(ClassObject *klass) {

}
