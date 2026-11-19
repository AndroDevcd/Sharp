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
    Int end = pop_stack_number.i;
    Int start = pop_stack_number.i;
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

            // same element type on both sides: copy the bytes of the range.
            // (An empty range has no array at all: create_object gives null for
            // a size of 0, and null is what the caller gets.)
            (void)index;
            if(newArray != nullptr) {
                memcpy(newArray->data, (char*)from->data + (size_t)start * element_size(from->type),
                       (size_t)copyLen * element_size(from->type));
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
    Int srcEnd = pop_stack_number.i;
    Int srcStart = pop_stack_number.i;
    Int destStart = pop_stack_number.i;
    Int copyLen = srcEnd - srcStart;
    auto from = pop_stack_object.o;
    auto to = pop_stack_object.o;

    /*
     * Copying nothing does nothing, whatever the arrays are. An empty array is
     * null on this VM (`new _int8[0]` gives null), so `string.append` of an
     * empty array copies 0 items into a null `data`; that raised
     * nullptr_exception - and an empty command-line argument stopped every
     * program in platform.srt_init. Only an empty copy is let through: a
     * copy of 1 item or more into or out of null still raises below.
     *
     * C++ comparison: memcpy(dest, src, 0) is valid for any pointers.
     */
    if(copyLen == 0 && srcStart >= 0 && destStart >= 0)
        return;

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

        // the usual case - both arrays hold the same kind of element -
        // is one block copy. Only a copy within ONE array to a HIGHER position
        // keeps the loop below: it reads elements it has already overwritten,
        // and that result is left exactly as it was.
        bool forwardOverlap = from == to && destStart > srcStart && destStart < srcEnd;
        if(from->type <= type_var && to->type == from->type && !forwardOverlap) {
            size_t width = (size_t)element_size(from->type);
            memmove((char*)to->data + (size_t)destStart * width, (char*)from->data + (size_t)srcStart * width, (size_t)copyLen * width);
        } else if(from->type > type_var && to->type > type_var && !forwardOverlap) {
            memmove(to->node + destStart, from->node + srcStart, (size_t)copyLen * sizeof(object));
            write_barrier_range(to, destStart, copyLen);    // references were stored into `to` (memory/sharp_object.h)
        } else if(from->type <= type_var) { // var[]
            // element by element through the accessors: the two arrays may hold
            // different number types (an _int8[] copied into an _int32[])
            for (Int i = srcStart; i < srcEnd; i++) {
                if(holds_decimals(to) || holds_decimals(from)) set_number_at(to, destStart++, number_at(from, i));
                else set_integer_at(to, destStart++, integer_at(from, i));
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
    Int len = pop_stack_number.i;
    Int start = pop_stack_number.i;
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
            const int width = element_size(from->type);
            for (Int i = (start + len) - 1; i >= start; i--) {
                memcpy((char*)newArray->data + (size_t)(iter++) * width, (char*)from->data + (size_t)i * width, width);
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
