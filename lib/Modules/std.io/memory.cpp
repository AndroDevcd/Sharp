//
// Created by BNunnally on 5/27/2020.
//
#include "memory.h"
#include "../../runtime/Thread.h"
#include "../../runtime/VirtualMachine.h"

void memcopy() {
    Int end = (thread_self->sp--)->var;
    Int start = (thread_self->sp--)->var;
    Object *arry = &thread_self->sp->object;
    SharpObject *oldArray = arry->object;

    if(oldArray != NULL) {
        if(end > oldArray->size || end < 0 || start < 0 || start >= oldArray->size) {
            stringstream ss;
            ss << "invalid call to memcopy() start: " << start << ", end: " << end
               << ", size: " << oldArray->size;
            throw Exception(vm.InvalidOperationExcept, ss.str());
        }

        if(TYPE(oldArray->info) == _stype_var) { // var[]
            *arry = GarbageCollector::self->newObject(end);
            std::memcpy(arry->object->HEAD+start, oldArray->HEAD, sizeof(double) * end);
        } else if(TYPE(oldArray->info) == _stype_struct) { // object? maybe...
            if(oldArray->node != NULL) {
                if(IS_CLASS(oldArray->info)) {
                    *arry = GarbageCollector::self->newObjectArray(end, &vm.classes[CLASS(oldArray->info)]);
                } else
                    *arry = GarbageCollector::self->newObject(end);

                for (Int i = start; i < end; i++) {
                    arry->object->node[i] = oldArray->node[i];
                }
            }
        } else
            throw Exception(vm.NullptrExcept, "");

    } else
        throw Exception(vm.NullptrExcept, "");
}

void invert() {
    Int len = (thread_self->sp--)->var;
    Int start = (thread_self->sp--)->var;
    Object *arry = &thread_self->sp->object;
    SharpObject *oldArray = arry->object;

    if(oldArray != NULL) {
        if((start+len) > oldArray->size || len < 0 || start < 0 || start >= oldArray->size) {
            stringstream ss;
            ss << "invalid call to invert() start: " << start << ", len: " << len
               << ", size: " << oldArray->size;
            throw Exception(vm.InvalidOperationExcept, ss.str());
        }

        if(oldArray->size <= 1) return;

        if(TYPE(oldArray->info) == _stype_var) { // var[]
            *arry = GarbageCollector::self->newObject(len);
            Int iter = 0;
            for (Int i = (start + len) - 1; i >= start; i--) {
                arry->object->HEAD[iter++] = oldArray->HEAD[i];
            }
        } else if(TYPE(oldArray->info) == _stype_struct) { // object? maybe...
            if(oldArray->node != NULL) {
                if(IS_CLASS(oldArray->info)) {
                    *arry = GarbageCollector::self->newObjectArray(len, &vm.classes[CLASS(oldArray->info)]);
                } else
                    *arry = GarbageCollector::self->newObject(len);

                for (Int i = (start + len) - 1; i >= start; i--) {
                    arry->object->node[i] = oldArray->node[i];
                }
            }
        } else
            throw Exception(vm.NullptrExcept, "");

    } else
        throw Exception(vm.NullptrExcept, "");
}
