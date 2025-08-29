//
// Created by BNunnally on 5/27/2020.
//
#include "memory.h"
#include "../../core/opcode/opcode_macros.h"
#include "../../runtime/multitasking/thread/sharp_thread.h"
#include "../../runtime/types/sharp_class.h"
#include "../../runtime/virtual_machine.h"
#include "../../runtime/error/vm_exception.h"
#include "../../runtime/memory/garbage_collector.h"

void copy() {
    auto task = thread_self->task;
    Int end = pop_stack_number;
    Int start = pop_stack_number;
    auto from = pop_stack_object.o;
    Int copyLen = end - start, index=0;

    if(from != NULL) {
        if(copyLen < 0 || end > from->size || end < 0 || start < 0 || start >= from->size) {
            stringstream ss;
            ss << "invalid call to copy() start: " << start << ", end: " << end
               << ", size: " << from->size;
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }

        if(from->type <= type_var) { // var[]
            auto newArray = create_object(copyLen, (data_type)from->type);

            for (Int i = start; i < end; i++) {
                newArray->HEAD[index++] = from->HEAD[i];
            }

            copy_object(&push_stack_object, newArray);
        } else { // object? maybe...
            sharp_object *newArray;
            if(IS_CLASS(from)) {
                newArray = create_object(&vm.classes[CLASS(from->info)], copyLen);
            } else
                newArray = create_object(copyLen);

            for (Int i = start; i < end; i++) {
                copy_object(newArray->node + (index++), from->node[i].o);
            }

            copy_object(&push_stack_object, newArray);
        }
    } else
        throw vm_exception(vm.nullptr_except, "");
}

void memcopy() {
    auto task = thread_self->task;
    Int srcEnd = pop_stack_number;
    Int srcStart = pop_stack_number;
    Int destStart = pop_stack_number;
    Int copyLen = srcEnd - srcStart;
    auto from = pop_stack_object.o;
    auto to = pop_stack_object.o;

    if(from != NULL && to != NULL) {
        if(srcEnd > from->size || srcEnd < 0 || srcStart < 0 || srcStart >= from->size
            || copyLen < 0) {
            stringstream ss;
            ss << "invalid call to memcopy(); start: " << srcStart << ", end: " << srcEnd
               << ", size: " << from->size;
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }

        if(destStart >= to->size || destStart < 0) {
            stringstream ss;
            ss << "invalid call to memcopy(); dest start: " << destStart
               << ", dest size: " << to->size;
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }

        if( (destStart + copyLen) > to->size) {
            stringstream ss;
            ss << "invalid call to memcopy() array is not large enough to receive all data; copy size: "
                << (copyLen) << ", starting at: " << destStart << ", dest len: " << to->size;
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }

        if(from->type <= type_var) { // var[]
            for (Int i = srcStart; i < srcEnd; i++) {
                to->HEAD[destStart++] = from->HEAD[i];
            }
        } else {
            for (Int i = srcStart; i < srcEnd; i++) {
                copy_object(to->node + (destStart++), from->node[i].o);
            }
        }

    } else
        throw vm_exception(vm.nullptr_except, "");
}

void invert() {
    auto task = thread_self->task;
    Int len = pop_stack_number;
    Int start = pop_stack_number;
    auto from = pop_stack_object.o;

    if(from != NULL) {
        if((start+len) > from->size || len < 0 || start < 0 || start >= from->size) {
            stringstream ss;
            ss << "invalid call to invert() start: " << start << ", len: " << len
               << ", size: " << from->size;
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }

        if(from->size <= 1) return;

        if(from->type <= type_var) { // var[]
            auto newArray = create_object(len, from->type);

            Int iter = 0;
            for (Int i = (start + len) - 1; i >= start; i--) {
                newArray->HEAD[iter++] = from->HEAD[i];
            }

            copy_object(&push_stack_object, newArray);
        } else {
            sharp_object *newArray;
            if(IS_CLASS(from)) {
                newArray = create_object(&vm.classes[CLASS(from->info)], len);
            } else
                newArray = create_object(len);

            Int iter = 0;
            for (Int i = (start + len) - 1; i >= start; i--) {
                copy_object(newArray->node + (iter++), from->node[i].o);
            }
        }

    } else
        throw vm_exception(vm.nullptr_except, "");
}
