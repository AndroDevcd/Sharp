//
// System interrupts for reflection: look at the call stack, and read an object's
// fields and description at run time.
//
// Moved verbatim from the old switch in virtual_machine.cpp (OP_GET_FRAME_INFO,
// OP_GET_STACK_TRACE, OP_DESCRIBE_OBJECT, OP_REFLECT_FIELD_VALUE).
// See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"

//
// get_frame_info(): build and push a StackState object - two arrays, one of the
// method ids and one of the program counters, one entry per live call frame.
//
void op_get_frame_info(Int interrupt) {
    auto task = thread_self->task;
    grow_stack
    stack_overflow_check
    auto info = get_frame_info(task);
    auto stackState = create_object(vm.stack_sate);
    auto methodsField = resolve_field("methods", stackState);
    auto pcField = resolve_field("pc", stackState);
    Int iter = 0;

    copy_object(methodsField, create_object(info.size(), type_int64));
    copy_object(pcField, create_object(info.size(), type_int64));
    for(KeyPair<Int, Int> &frameInfo : info) {
        set_integer_at(methodsField->o, iter, frameInfo.key);
        set_integer_at(pcField->o, iter++, frameInfo.value);
    }

    copy_object(&push_stack_object, stackState);
}

//
// get_stack_trace(): fill in the current exception's stack trace from the live
// frames (used when an error is being raised).
//
void op_get_stack_trace(Int interrupt) {
    fill_stack_trace_from_frame_info();
}

//
// describe_object(object): replace the object on the stack with a description of
// it (its class, fields and so on) for reflection.
//
void op_describe_object(Int interrupt) {
    auto task = thread_self->task;
    auto object = task->sp->obj.o;
    copy_object(&task->sp->obj, describe_object(object));
}

//
// reflect_field_value(object, index): read field number `index` of an object and
// leave its value on the stack. A reference field is handed back as-is; a number
// field is boxed into the matching number class (Char, Int, Long, Double ...)
// according to how the field was declared. Out of range, or a null object, gives
// null.
//
void op_reflect_field_value(Int interrupt) {
    auto task = thread_self->task;
    auto instance = (task->sp--)->obj.o;
    auto index = (Int)(task->sp)->var.i;

    if(instance && IS_CLASS(instance) && index < vm.classes[CLASS(instance->info)].instanceFields) {
        // what the slot holds is decided by the field's declaration
        sharp_field &declared = vm.classes[CLASS(instance->info)].fields[index];
        bool isNumber = declared.type->type <= type_var && !declared.isArray;
        auto field = isNumber ? nullptr : instance->node[index].o;
        if(isNumber || field) {
            if (!isNumber) {
                copy_object(&task->sp->obj, field);
            } else {
                // a number field: hand out a number object (int, double ...) holding it
                string field_name = "value";
                vm_cell cell = number_slot(instance, index);
                double value = declared.type->type == type_var ? cell.d : (double)cell.i;

                switch(declared.type->type) {
                    case type_int8: {
                        // char
                        auto obj = create_object(vm.char_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (char)value);
                        break;
                    }
                    case type_int16: {
                        // char
                        auto obj = create_object(vm.short_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (short)value);
                        break;
                    }
                    case type_int32: {
                        // char
                        auto obj = create_object(vm.int_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (int)value);
                        break;
                    }
                    case type_int64: {
                        // char
                        auto obj = create_object(vm.long_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (int64_t)value);
                        break;
                    }
                    case type_uint8: {
                        // char
                        auto obj = create_object(vm.uchar_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (unsigned char)value);
                        break;
                    }
                    case type_uint16: {
                        // char
                        auto obj = create_object(vm.ushort_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (unsigned short)value);
                        break;
                    }
                    case type_uint32: {
                        // char
                        auto obj = create_object(vm.uint_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (unsigned int)value);
                        break;
                    }
                    case type_uint64: {
                        // char
                        auto obj = create_object(vm.ulong_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (uint64_t)value);
                        break;
                    }
                    case type_function_ptr: {
                        // char
                        auto obj = create_object(vm.long_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, (uint64_t)value);
                        break;
                    }
                    case type_var: {
                        // char
                        auto obj = create_object(vm.double_class);
                        copy_object(&task->sp->obj, obj);
                        assign_numeric_class_field(obj, value);
                        break;
                    }
                }
            }
        } else {
            copy_object(&task->sp->obj, (sharp_object*)nullptr);
        }
    } else {
        copy_object(&task->sp->obj, (sharp_object*)nullptr);
    }
}
