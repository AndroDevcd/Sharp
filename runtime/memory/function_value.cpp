//
// Function values: what `fn : (int)(int)` holds at run time.
// The description is in function_value.h.
//

#include "function_value.h"
#include "sharp_object.h"
#include "memory_helpers.h"
#include "type_check.h"
#include "../virtual_machine.h"
#include "../error/vm_exception.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/fiber/fiber.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "../reflect/reflect_helpers.h"

// The slots of platform.kernel#function_value, found by name at startup so the
// library class can be reordered without touching this file.
static sharp_class *functionValueClass = nullptr;
static Int addressSlot = 0;
static Int capturedSlot = 1;

void setup_function_values() {
    functionValueClass = locate_class("platform.kernel#function_value");
    if(functionValueClass == nullptr) return;

    for(Int slot = 0; slot < functionValueClass->instanceFields; slot++) {
        const sharp_field &field = functionValueClass->fields[slot];
        if(field.name == "address") addressSlot = slot;
        else if(field.name == "captured") capturedSlot = slot;
    }
}

Int function_value_address_slot() { return addressSlot; }
Int function_value_captured_slot() { return capturedSlot; }

Int function_value_address(sharp_object *functionValue) {
    if(functionValue == nullptr)
        throw vm_exception(vm.nullptr_except, "attempting to call a function value that is null");

    Int address = number_slot(functionValue, addressSlot).i;
    if(address <= 0 || address >= vm.mf.methods) {
        stringstream ss;
        ss << "invalid call to method (@" << address << ") out of range!";
        throw vm_exception(ss.str());
    }
    return address;
}

void call_function_value(Int argumentCount) {
    fiber *task = thread_self->task;
    stack_item *slot = task->sp - argumentCount;     // where the function value sits
    sharp_object *functionValue = slot->obj.o;

    Int address = function_value_address(functionValue);
    sharp_object *captured = functionValue->node[capturedSlot].o;

    // the arguments close the gap the function value leaves
    for(Int i = 0; i < argumentCount; i++) {
        slot[i] = slot[i + 1];
        TC(TC_ST(slot + i) = TC_ST(slot + i + 1);)
    }

    if(captured != nullptr) {
        // the hidden last parameter of a lambda that captures: the top slot
        // is free now, the captured object goes there
        task->sp->obj.o = captured;
    } else {
        task->sp->obj.o = nullptr;      // the old top no longer refers to anything
        task->sp--;
    }

    prepare_method(address);
}
