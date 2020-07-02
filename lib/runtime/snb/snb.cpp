//
// Created by BNunnally on 6/27/2020.
//

#include "snb.h"
#include "../symbols/Object.h"
#include "../VirtualMachine.h"

#ifdef __cplusplus
extern "C" {
#endif

    EXPORTED void inc_ref(void *object) {
        if(object != NULL)
            INC_REF(((Object*)object)->object)
    }

    EXPORTED void def_ref(void *object) {
        if(object != NULL)
            DEC_REF(((Object*)object)->object)
    }

    EXPORTED double* getfpNumAt(int32_t fpOffset) {
        return &(thread_self->fp+fpOffset)->var;
    }

    EXPORTED object getField(object obj, const string& name){
        Object *lastField = NULL, *tmp;
        if(obj != NULL) {
            lastField = ((Object*)obj);
            stringstream fieldName;
            for(long i = 0; i < name.size(); i++) {
                if(name[i] != '.')
                    fieldName << name[i];

                if(name[i] == '.' || (i + 1) >= name.size()){
                    string f = fieldName.str();
                    tmp = vm.resolveField(fieldName.str(), lastField->object);
                    if(tmp == NULL) {
                        return NULL;
                    }

                    lastField = tmp;
                    fieldName.str("");
                }
            }
        }

        return lastField;
    }

    EXPORTED double* getVarPtr(object obj){
        if(obj) {
            SharpObject *shobj = ((Object*)obj)->object;
            if(shobj && TYPE(shobj->info) == _stype_var) {
                return shobj->HEAD;
            }
        }

        return NULL;
    }

    EXPORTED object getfpLocalAt(int32_t fpOffset){
        return &(thread_self->fp+fpOffset)->object;
    }

    EXPORTED int32_t getSize(object obj){
        if(obj && ((Object*)obj)->object) {
            return ((Object*)obj)->object->size;
        }

        return -1;
    }

    EXPORTED int32_t setObject(object dest, object src) {
        if(dest) {
            Object *_dest = (Object*)dest, *_src = (Object*)src;
            *_dest = _src;
            return 0;
        } else return 1;
    }

    EXPORTED object staticClassInstance(const string& name) {
        ClassObject *klass = vm.resolveClass(name);
        if(klass) {
            return &vm.staticHeap[klass->address];
        }

        return NULL;
    }

    EXPORTED void incSp() {
        THREAD_STACK_CHECK(thread_self)
        thread_self->sp++;
    }

    EXPORTED void decSp(int32_t amount) {
        if(((thread_self->sp - thread_self->dataStack) - amount) < 0) {
            throw Exception(vm.ThreadStackExcept, "the thread stack cannot be negative");
        }

        thread_self->sp -= amount;
    }

    EXPORTED double* getspNumAt(int32_t spOffset) {
        return &(thread_self->sp+spOffset)->var;
    }

    EXPORTED object getspObjAt(int32_t spOffset) {
        return &(thread_self->sp+spOffset)->object;
    }

    EXPORTED object newVarArray(int32_t size) {
        (thread_self->sp+1)->object =
                GarbageCollector::self->newObject(size);
        return &(thread_self->sp)->object;
    }

    EXPORTED object newClass(const string& name) {
        ClassObject *klass = vm.resolveClass(name);
        if(klass) {
            (thread_self->sp+1)->object =
                    GarbageCollector::self->newObject(klass);
            return &(thread_self->sp)->object;
        }

        return NULL;
    }

    EXPORTED object newObjArray(int32_t size) {
        (thread_self->sp+1)->object =
                GarbageCollector::self->newObjectArray(size);
        return &(thread_self->sp)->object;
    }

    EXPORTED object newClassArray(const string &name, int32_t size) {
        ClassObject *klass = vm.resolveClass(name);
        if(klass) {
            (thread_self->sp+1)->object =
                    GarbageCollector::self->newObjectArray(size, klass);
            return &(thread_self->sp)->object;
        }

        return NULL;
    }

    EXPORTED void pushNum(double value) {
        (++thread_self->sp)->var = value;
    }

    EXPORTED void pushObj(object value) {
        (++thread_self->sp)->object =
                ((Object*) value);
    }

    #pragma optimize( "", off )
    EXPORTED void call(int32_t address) {
        fptr jitFun;
        if((jitFun = executeMethod(address, thread_self, true)) != NULL) {

#ifdef BUILD_JIT
            jitFun(jctx);
#endif
        }
    }
    #pragma optimize( "", on )
#ifdef __cplusplus
}
#endif