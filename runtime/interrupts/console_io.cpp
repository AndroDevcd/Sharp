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
    console_output_written();
}

//
// whole_text(number, unsigned): the digits of a WHOLE number, as an array of characters pushed on
// the stack. The number is read as the 64-bit whole number it is (registers[EBX].i) - printf above
// takes every number as a decimal, which holds a whole number exactly only up to 2^53, so
// 9007199254740993 came out as ...992. ECX is 1 when the 64 bits are an unsigned number.
//
void op_whole_text(Int interrupt) {
    auto task = thread_self->task;

    string digits = registers[ECX].i != 0 ? to_string((uint64_t) registers[EBX].i) : to_string((int64_t) registers[EBX].i);
    auto text = create_object(digits.size(), type_int8);
    copy_object(&push_stack_object, text);
    assign_string_field(text, digits);
}

//
// flush(): what the program printed so far is out when this returns. (Nothing has to call it for
// output to be seen: the VM flushes on its own, console_helper.h.)
//
void op_flush(Int interrupt) {
    cout << std::flush;
    console_output_flush();
}

//
// print_bytes(characters, line end): print a whole array of characters - a string's text - in one
// go, and a line end after it when EBX is not 0. The array is popped off the stack; a null one (an
// empty text is the null array) prints nothing. This is what print() and println() are made of:
// one call for a line, where there used to be one instruction for every character.
//
void op_print_bytes(Int interrupt) {
    auto task = thread_self->task;

    auto text = pop_stack_object.o;
    if(text != NULL && text->type <= type_var && text->data != NULL) {
        if(element_size(text->type) == 1) {
            fwrite(text->data, 1, text->size, stdout);
        } else {
            for(Int i = 0; i < text->size; i++) putc((char)integer_at(text, i), stdout);
        }
    }
    if(registers[EBX].i != 0) putc('\n', stdout);
    console_output_written();
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
