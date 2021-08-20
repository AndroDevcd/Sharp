//
// Created by BraxtonN on 2/12/2018.
//

#include "Object.h"
#include "../memory/GarbageCollector.h"
#include "../register.h"
#include "../Thread.h"
#include "../Exe.h"
#include "../Manifest.h"
#include "../VirtualMachine.h"
#include "../scheduler/thread_controller.h"
recursive_mutex refLock;

void Object::castObject(int64_t classPtr) {
    ClassObject* type =NULL, *base = NULL;

    if(this->object == NULL)
        throw Exception(vm.ClassCastExcept, "invalid cast on null object");

    if(classPtr < vm.manifest.classes && classPtr > 0) {
        type = &vm.classes[classPtr];
        base = &vm.classes[CLASS(object->info)];
    } else {
        stringstream msg;
        msg << "class not found @address:" << classPtr;

        throw Exception(msg.str());
    }

    if(!IS_CLASS(this->object->info)) {

        stringstream nonclass;
        nonclass << "attempt to perform invalid cast to [" << type->name << "] on non-class object ";

        throw Exception(vm.ClassCastExcept, nonclass.str());
    }

    if(type->guid != base->guid && !base->isClassRelated(type)) {
        // validate we have all our interfaces checked
        ClassObject *originalClass = base;
        while (base != NULL) {
            for(int i = 0; i < base->interfaceCount; i++) {
                ClassObject* _interface = base->interfaces[i];
                if(_interface->guid==type->guid || _interface->isClassRelated(type))
                    return;
            }


            if(type->guid == base->guid)
                return;
            base = base->super;
        }

        stringstream ss;
        ss << "illegal cast of class '" << originalClass->name << "' to '";
        ss << type->name << "'";
        throw Exception(vm.ClassCastExcept, ss.str());
    }
}

void Object::wait() {
    if(object == NULL) return;

    const long sMaxRetries = 128 * 1024;

    long retryCount = 0;
    Thread *current = thread_self;

    while (object->monitor == 0)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            __os_yield();
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        } else if(current->state == THREAD_KILLED
          || hasSignal(current->signal, tsig_kill)) {
            return;
        } else if(hasSignal(current->signal, tsig_suspend)) {
            suspend_self();
        } else if(vm.state == VM_SHUTTING_DOWN) {
            current->state = THREAD_KILLED;
            sendSignal(current->signal, tsig_kill, 1);
            return;
        }
    }

    object->monitor = 0;
}

void Object::notify() {
    if(object == NULL) return;

    const long sMaxRetries = 128 * 1024;

    long spinCount = 0;
    long retryCount = 0;
    Thread *current = thread_self;
    object->monitor = 1;

    while (object->monitor == 1)
    {
        if (retryCount++ == sMaxRetries)
        {
            spinCount++;
            retryCount = 0;
            __os_yield();
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        } else if(current->state == THREAD_KILLED
                  || hasSignal(current->signal, tsig_kill)) {
            return;
        } else if(hasSignal(current->signal, tsig_suspend)) {
            suspend_self();
        } else if(vm.state == VM_SHUTTING_DOWN) {
            current->state = THREAD_KILLED;
            sendSignal(current->signal, tsig_kill, 1);
            return;
        }
    }
}

void Object::notify(uInt mills) {

    if(object == NULL) return;

    Int base = NANO_TOMILL(Clock::realTimeInNSecs()), elapsedMs = 0;
    Thread *current = thread_self;
    object->monitor = 1;

    while (object->monitor == 1)
    {
        if ((mills - elapsedMs) > 0)
        {
            __os_yield();
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        } else if(current->state == THREAD_KILLED
                  || hasSignal(current->signal, tsig_kill)) {
            return;
        } else if(hasSignal(current->signal, tsig_suspend)) {
            suspend_self();
        }
        else if(vm.state == VM_SHUTTING_DOWN) {
            current->state = THREAD_KILLED;
            sendSignal(current->signal, tsig_kill, 1);
            return;
        } else if((mills - elapsedMs) <= 0) {
            object->monitor = 0;
            break;
        }

        elapsedMs = NANO_TOMILL(Clock::realTimeInNSecs()) - base;
    }
}

void SharpObject::print() {
    cout << "Object @0x" << this << endl;
    cout << "size " << size << endl;
    cout << "refrences " << refCount << endl;
    cout << "monitor " << monitor << endl;
    cout << "generation " << GENERATION(info) << endl;
    ClassObject *k = &vm.classes[CLASS(info)];
    if(IS_CLASS(info)) cout << "class: " << k->name << endl;

    if(TYPE(info)==_stype_var) {
        if(ntype == _INT8) {
            cout << "value: " << vm.stringValue(this) << endl;
        } else {
            cout << "{ ";
            for(Int i = 0; i < 50; i++) {
                if(i >= size)
                    break;

                cout << HEAD[i];
                if((i + 1 < 50) || (i+1) < size)
                   cout << ", ";
            }

            cout << " }" << endl;
        }
    } else if(TYPE(info)==_stype_struct){
        for(long i = 0; i < size; i++) {
            cout << '\t' << "(" << this << ")struct item -> #" << i << " ";
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
