//
// System interrupts for the console and for turning text into numbers.
//
// These were the OP_PRINTF / OP_FLUSH / OP_KBHIT / OP_STRTOD / OP_STRTOL cases
// of the old switch in virtual_machine.cpp. The bodies are unchanged - each one
// still reads its inputs from the registers and the stack and writes its result
// back the same way. See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"

//
// printf(format, value, precision): print one decimal. The number is ALWAYS a
// decimal here (registers[EBX].d), whatever the Sharp side printed - the format
// and precision are integers.
//
void op_printf(Int interrupt) {
    dbl_printf((int)registers[EGX].i, registers[EBX].d, (int)registers[ECX].i);
}

//
// flush(): push whatever is buffered on stdout out to the terminal now.
//
void op_flush(Int interrupt) {
    cout << std::flush;
}

//
// kbhit(): has a key been pressed and not yet read? Result (0 or 1) in CMT.
//
void op_kbhit(Int interrupt) {
    registers[CMT].i = _kbhit();
}

//
// strtod(string): parse a decimal out of a string. The string is popped off the
// stack; the decimal result goes in EBX. A null or non-text object is a fault.
//
void op_strtod(Int interrupt) {
    auto task = thread_self->task;

    auto strObject = pop_stack_object.o;
    if (strObject != NULL && strObject->type <= type_var && strObject->data != NULL) {
        string num_str;
        populate_string(num_str, strObject);
        registers[EBX].d = strtod(num_str.c_str(), NULL);
    } else
        throw vm_exception(vm.nullptr_except, "");
}

//
// strtol(string, base): parse a whole number out of a string in the given base.
// Base is popped first (it was pushed last), then the string; result in EBX.
//
void op_strtol(Int interrupt) {
    auto task = thread_self->task;

    Int baseNum = pop_stack_number.i;
    auto str = pop_stack_object.o;
    if(str != NULL && str->type <= type_var) {
        string numStr;
        populate_string(numStr, str);
        registers[EBX].i = strtoll(numStr.c_str(), NULL, baseNum);
    } else {
        throw vm_exception(vm.nullptr_except, "");
    }
}
