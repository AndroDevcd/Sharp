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
    return &(thread_self->this_fiber->fp+fpOffset)->var;
}

object getField(object obj, const char* name){
    Object *lastField = NULL, *tmp;
    Int len = strlen(name);
    if(obj != NULL) {
        lastField = ((Object*)obj);
        stringstream fieldName;
        for(long i = 0; i < len; i++) {
            if(name[i] != '.')
                fieldName << name[i];

            if(name[i] == '.' || (i + 1) >= len){
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
    return &(thread_self->this_fiber->fp+fpOffset)->object;
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

object staticClassInstance(const char* name) {
    ClassObject *klass = vm.resolveClass(name);
    if(klass) {
        return &vm.staticHeap[klass->address];
    }

    return NULL;
}

void incSp() {
    grow_stack2(thread_self)
    THREAD_STACK_CHECK(thread_self)
    thread_self->this_fiber->sp++;
}

void decSp(int32_t amount) {
    try {
        if (((thread_self->this_fiber->sp - thread_self->this_fiber->dataStack) - amount) < 0) {
            throw Exception(vm.ThreadStackExcept, "the thread stack cannot be negative");
        }

        thread_self->this_fiber->sp -= amount;
    } catch(Exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

double* getspNumAt(int32_t spOffset) {
    return &(thread_self->this_fiber->sp+spOffset)->var;
}

object getspObjAt(int32_t spOffset) {
    return &(thread_self->this_fiber->sp+spOffset)->object;
}

object newVarArray(int32_t size, unsigned short ntype) {
    try {
        grow_stack2(thread_self)
        THREAD_STACK_CHECK(thread_self)
        (thread_self->this_fiber->sp + 1)->object =
                gc.newObject(size, ntype);
        return &(thread_self->this_fiber->sp + 1)->object;
    } catch(Exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return NULL;
    }
}

object newClass(const char* name) {
    try {
        grow_stack2(thread_self)
        THREAD_STACK_CHECK(thread_self)
        ClassObject *klass = vm.resolveClass(name);
        if(klass) {
            (thread_self->this_fiber->sp+1)->object =
                    gc.newObject(klass);
            return &(thread_self->this_fiber->sp+1)->object;
        }

        return NULL;
    } catch(Exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return NULL;
    }
}

object newObjArray(int32_t size) {
    try {
        grow_stack2(thread_self)
        THREAD_STACK_CHECK(thread_self)
        (thread_self->this_fiber->sp+1)->object =
                gc.newObjectArray(size);
        return &(thread_self->this_fiber->sp+1)->object;
    } catch(Exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return NULL;
    }
}

object newClassArray(const char *name, int32_t size) {
    try {
        ClassObject *klass = vm.resolveClass(name);
        if(klass) {
            (thread_self->this_fiber->sp+1)->object =
                    gc.newObjectArray(size, klass);
            return &(thread_self->this_fiber->sp+1)->object;
        }

        return NULL;
    } catch(Exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return NULL;
    }
}

object getItem(object obj, int32_t index) {
    try {
        Object *data = (Object*)obj;
        if(data == NULL || data->object == NULL) {
            throw Exception(vm.NullptrExcept, "");
        }

        if(index < 0 || index >= data->object->size) {
            stringstream ss;
            ss << "index out of bounds accessing array at: " << index << " where size is: " << data->object->size;
            throw Exception(vm.IndexOutOfBoundsExcept, ss.str());
        }

        return data->object->node+index;
    } catch(Exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return NULL;
    }
}

void pushNum(double value) {
    grow_stack2(thread_self)
    THREAD_STACK_CHECK(thread_self)
    (++thread_self->this_fiber->sp)->var = value;
}

void pushObj(object value) {
    grow_stack2(thread_self)
    THREAD_STACK_CHECK(thread_self)
    (++thread_self->this_fiber->sp)->object =
            ((Object*) value);
}

void call(int32_t address) {
    try {
        executeMethod(address, thread_self, true)(thread_self);
    } catch(Exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

bool exceptionCheck() {
    return hasSignal(thread_self->signal, tsig_except);
}

object getExceptionObject() {
    if(thread_self->this_fiber->exceptionObject.object)
        return &thread_self->this_fiber->exceptionObject;
    else return &(thread_self->this_fiber->sp)->object;
}

const char* className(object klazz) {
    Object *obj = (Object*)klazz;
    if(obj && obj->object && TYPE(obj->object->info) == _stype_struct) {
        if(IS_CLASS(obj->object->info))
            return vm.classes[CLASS(obj->object->info)].fullName.c_str();
    }

    return NULL;
}

void prepareException(object exceptionClass) {
    thread_self->this_fiber->exceptionObject = (Object*)exceptionClass;
    sendSignal(thread_self->signal, tsig_except, 1);
}

void clearException() {
    thread_self->this_fiber->exceptionObject = (SharpObject*)NULL;
    sendSignal(thread_self->signal, tsig_except, 0);
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
        (void*)call,
        (void*)exceptionCheck,
        (void*)getExceptionObject,
        (void*)className,
        (void*)prepareException,
        (void*)clearException,
        (void*)getItem
};

bool setupLibrary(lib_handshake lhand) {
    return lhand(lib_funcs, 26);
}