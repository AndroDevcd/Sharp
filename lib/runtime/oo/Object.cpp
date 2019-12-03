//
// Created by BraxtonN on 2/12/2018.
//

#include "Object.h"
#include "../memory/GarbageCollector.h"
#include "../Environment.h"
#include "../register.h"
#include "../Thread.h"
#include "../Exe.h"
#include "../Manifest.h"

void Object::castObject(int64_t classPtr) {
    ClassObject* type =NULL, *base = NULL;

    if(this->object == NULL)
        throw Exception(Environment::ClassCastException, "invalid cast on null object");

    if(classPtr < manifest.classes && classPtr > 0) {
        type = &env->classes[classPtr];
        base = &env->classes[CLASS(object->info)];
    } else {
        stringstream msg;
        msg << "class not found @address:" << classPtr;

        throw Exception(msg.str());
    }

    stringstream nonclass;
    nonclass << "attempt to perform invalid cast to [" << type->name.str() << "] on non-class object ";

    if(!IS_CLASS(this->object->info))
        throw Exception(Environment::ClassCastException, nonclass.str());

    if(type->serial!= base->serial && !base->isClassRelated(type)) {
        // validate we have all our interfaces checked
        for(int i = 0; i < base->interfaceCount; i++) {
            ClassObject* _interface = &env->classes[base->interfaces[i]];
            if(_interface->serial==type->serial || _interface->isClassRelated(type))
                return;
        }

        stringstream ss;
        ss << "illegal cast of class '" << base->name.str() << "' to '";
        ss << type->name.str() << "'";
        throw Exception(Environment::ClassCastException, ss.str());
    }
}

void SharpObject::print() {
    cout << "Object @0x" << this << endl;
    cout << "size " << size << endl;
    cout << "refrences " << refCount << endl;
    cout << "generation " << GENERATION(info) << endl;
    ClassObject *k = &env->classes[CLASS(info)];
    if(IS_CLASS(info)) cout << "class: " << k->name.str() << endl;

    if(TYPE(info)==_stype_var) {
        cout << "HEAD[]" << endl;
    } else if(TYPE(info)==_stype_struct){
        for(long i = 0; i < size; i++) {
            cout << '\t' << this << " -> #" << i << " ";
            if(node[i].object == NULL) {
                cout << "NULL";
            } else
                node[i].object->print();
            cout << endl;
        }
    } else {
        cout << "both assets are Null!!" << endl;
    }
}
