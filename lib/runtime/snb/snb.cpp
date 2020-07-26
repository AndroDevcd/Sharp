//
// Created by BNunnally on 6/27/2020.
//

#include "snb.h"
#include "../symbols/Object.h"
#include "../VirtualMachine.h"

void inc_ref(void *object) {
    if(object != NULL)
        INC_REF(((Object*)object)->object)
}

void dec_ref(void *object) {
    if(object != NULL)
        DEC_REF(((Object*)object)->object)
}

double* getfpNumAt(int32_t fpOffset) {
    return &(thread_self->fp+fpOffset)->var;
}

object getField(object obj, const string& name){
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

double* getVarPtr(object obj){
    if(obj) {
        SharpObject *shobj = ((Object*)obj)->object;
        if(shobj && TYPE(shobj->info) == _stype_var) {
            return shobj->HEAD;
        }
    }

    return NULL;
}

object getfpLocalAt(int32_t fpOffset){
    return &(thread_self->fp+fpOffset)->object;
}

int32_t getSize(object obj){
    if(obj && ((Object*)obj)->object) {
        return ((Object*)obj)->object->size;
    }

    return -1;
}

int32_t setObject(object dest, object src) {
    if(dest) {
        Object *_dest = (Object*)dest, *_src = (Object*)src;
        *_dest = _src;
        return 0;
    } else return 1;
}

object staticClassInstance(const string& name) {
    ClassObject *klass = vm.resolveClass(name);
    if(klass) {
        return &vm.staticHeap[klass->address];
    }

    return NULL;
}

void incSp() {
    THREAD_STACK_CHECK(thread_self)
    thread_self->sp++;
}

void decSp(int32_t amount) {
    if(((thread_self->sp - thread_self->dataStack) - amount) < 0) {
        throw Exception(vm.ThreadStackExcept, "the thread stack cannot be negative");
    }

    thread_self->sp -= amount;
}

double* getspNumAt(int32_t spOffset) {
    return &(thread_self->sp+spOffset)->var;
}

object getspObjAt(int32_t spOffset) {
    return &(thread_self->sp+spOffset)->object;
}

object newVarArray(int32_t size) {
    (thread_self->sp+1)->object =
            gc.newObject(size);
    return &(thread_self->sp+1)->object;
}

object newClass(const string& name) {
    ClassObject *klass = vm.resolveClass(name);
    if(klass) {
        (thread_self->sp+1)->object =
                gc.newObject(klass);
        return &(thread_self->sp+1)->object;
    }

    return NULL;
}

object newObjArray(int32_t size) {
    (thread_self->sp+1)->object =
            gc.newObjectArray(size);
    return &(thread_self->sp+1)->object;
}

object newClassArray(const string &name, int32_t size) {
    ClassObject *klass = vm.resolveClass(name);
    if(klass) {
        (thread_self->sp+1)->object =
                gc.newObjectArray(size, klass);
        return &(thread_self->sp+1)->object;
    }

    return NULL;
}

void pushNum(double value) {
    (++thread_self->sp)->var = value;
}

void pushObj(object value) {
    (++thread_self->sp)->object =
            ((Object*) value);
}

void call(int32_t address) {
    fptr jitFun;
    if((jitFun = executeMethod(address, thread_self, true)) != NULL) {

#ifdef BUILD_JIT
        jitFun(jctx);
#endif
    }
}

void *lib_funcs[] =
{
        (void*)inc_ref,
        (void*)dec_ref,
        (void*)getfpNumAt,
        (void*)getField,
        (void*)getVarPtr,
        (void*)getfpLocalAt,
        (void*)getSize,
        (void*)setObject,
        (void*)staticClassInstance,
        (void*)incSp,
        (void*)getspNumAt,
        (void*)getspObjAt,
        (void*)newVarArray,
        (void*)newClass,
        (void*)newObjArray,
        (void*)newClassArray,
        (void*)decSp,
        (void*)pushNum,
        (void*)pushObj,
        (void*)call
};

bool setupLibrary(lib_handshake lhand) {
    return lhand(lib_funcs, 20);
}