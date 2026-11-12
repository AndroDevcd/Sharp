//
// Created by bknun on 8/20/2022.
//

#include "scheme_compound_assign_value.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

/**
 * Does generating `scheme` (or a scheme nested in one of its steps) call a
 * function? A callee is free to overwrite every register.
 */
static bool scheme_makes_call(operation_schema *scheme) {
    if(scheme == NULL) return false;

    for(Int i = 0; i < scheme->steps.size(); i++) {
        operation_step *step = scheme->steps.get(i);
        if(step->type == operation_call_instance_function
           || step->type == operation_call_static_function
           || step->type == operation_call_dynamic_function
           || scheme_makes_call(step->scheme))
            return true;
    }
    return false;
}

void process_compound_assign_value_scheme(operation_schema *scheme) {
    sharp_field *closureRef = NULL, *closure = NULL;
    Int stepPos = 0;

    process_allocate_register(next_step);
    process_allocate_register(next_step);
    process_get_value(next_step);
    operation_step *retainValue = scheme->steps.get(stepPos);
    process_retain_numeric_value(next_step);

    // The value on the right now waits in a register while the variable on the
    // left is worked out. If working that out calls a function - `a[next()] += 7`,
    // `find(x).count += 1` - the callee may overwrite the register, so the value
    // is kept on the stack across it:
    //      rstore  R        ; push
    //      ...left side...
    //      loadval R        ; pop
    // (it used to be lost: a[next()] += 7 added whatever the callee left in R)
    internal_register *valueRegister = find_register(retainValue->integer);
    bool keepOnStack = valueRegister != NULL && valueRegister->type == normal_register
                       && scheme_makes_call(scheme->steps.get(stepPos)->scheme);
    if(keepOnStack)
        add_instruction(Opcode::Builder::rstore((_register)valueRegister->address));

    operation_step *asigneeStep = scheme->steps.get(stepPos);
    process_get_value(next_step);
    machine_data asignee(cc.machineData);

    // `a[i] += v`: the variable is a number array element. Reading it ends with
    //      popobj2 / chklen adx / iaload ebx, adx
    // which leaves the array in the object pointer and the index in adx, and
    // nothing below changes either. So when that `iaload` is the instruction
    // that produced the value, the result is written straight back with
    //      rmov adx, bmr        ; array[adx] = bmr
    // The array and the index expression are evaluated once, as in C++.
    bool arrayElement = !cc.instructions.empty()
                        && cc.instructions.last() == Opcode::Builder::iaload(EBX, ADX);

    // the element type, for sized arrays (`bytes[i] += 100` wraps like any
    // other store into an _int8): the type recorded on the step that read it
    Int elementType = type_var;
    if(arrayElement && asigneeStep->scheme != NULL) {
        for(Int i = asigneeStep->scheme->steps.size() - 1; i >= 0; i--) {
            operation_step *s = asigneeStep->scheme->steps.get(i);
            if(s->type == operation_get_array_element_at_index) {
                elementType = s->nativeType;
                break;
            }
        }
    }

    if(keepOnStack)
        add_instruction(Opcode::Builder::loadValue((_register)valueRegister->address));

    // Is the variable being assigned one that a lambda captured? Then the new
    // value is also written to the closure object (below).
    //
    // The field comes with the machine data, exactly as in scheme_assign_value.cpp.
    // It used to be looked up as locals[dataAddress], but dataAddress is the
    // frame slot, not an index into the locals list: in a method slot 0 is
    // `self`, so the first local sits in slot 1 and the lookup read one past
    // the end ("index out of bounds list::get() _X: 1 length: 1") and aborted
    // the compiler on `sum += seven();`.
    if((cc.machineData.type == local_field_object_data || cc.machineData.type == numeric_local_field)
       && cc.machineData.field != NULL
       && cc.machineData.field->closure != NULL) {
        closureRef = cc.machineData.field->closureRef;
        closure = cc.machineData.field->closure;
    }

    operation_step *retainOld = scheme->steps.get(stepPos);
    process_retain_numeric_value(next_step);

    process_step(next_step); // add, sub, mult, div, mod, etc

    // `a[i]++` (see compile_array_element_step in unary_expression.cpp) marks
    // the assign step: the statement's value is the one from BEFORE the add.
    bool resultIsOldValue = scheme->steps.get(stepPos)->_bool;

    if(arrayElement) {
        stepPos++;  // the assign step: done here instead
        if(elementType < type_function_ptr)
            cast_machine_data(BMR, (data_type)elementType);
        add_instruction(Opcode::Builder::rmov(ADX, BMR));
        // (the machine data already says "the value is in bmr": the result of
        //  the statement, like any other compound assignment)
    } else {
        cc.machineData.field = asignee.field;
        cc.machineData.dataAddress = asignee.dataAddress;
        cc.machineData.type = asignee.type;

        process_assign_numeric_value(next_step);
    }

    if(resultIsOldValue) {
        // the register the old value was kept in is untouched by the add
        // (the sum went to bmr):   bmr = old
        internal_register *old = get_register_or_consume(retainOld->integer, true);
        add_instruction(Opcode::Builder::movr(BMR, (_register)old->address));
    }

    process_deallocate_register(next_step);
    process_deallocate_register(next_step);

    if(closureRef && closure) {
        push_machine_data_to_stack();
        add_instruction(Opcode::Builder::dup());

        add_instruction(Opcode::Builder::movg(closureRef->owner->ci->address));
        add_instruction(Opcode::Builder::movn(closureRef->ci->address));
        add_instruction(Opcode::Builder::movn(closure->ci->address));

        set_machine_data(closure, false);
        pop_machine_data_from_stack();

        if(is_numeric_type(closure->closure->type) && !closure->type.isArray) {
            set_machine_data(get_register(EBX));
            pop_machine_data_from_stack();
            set_machine_data(get_register(EBX));
        } else {
            add_instruction(Opcode::Builder::popObject2());
            set_machine_data(generic_object_data);
        }
    }
}
