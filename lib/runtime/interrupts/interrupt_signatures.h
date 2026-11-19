//
// What every system interrupt (INT <flag>, exec_interrupt in virtual_machine.cpp)
// expects in the registers and on the stack, and what it leaves behind.
//
// WHY THIS IS WRITTEN DOWN
// ------------------------
// An interrupt is called from a hand written `asm { }` block in the standard
// library. Nothing checks that block: it loads registers, pushes values, and
// says `int 0x18`. A number is an INTEGER or a DECIMAL (lib/core/opcode/
// opcode.h), the two are stored differently, and the interrupt reads each
// register as one or the other. A block that passes the wrong kind does not
// fail - the interrupt reads garbage, and with an object on the stack in the
// wrong place it crashes the VM.
//
// The table in interrupt_signatures.cpp is the one place that says what each
// interrupt takes. The number kind checker (memory/type_check.h, a runtime built
// with -DSHARP_TYPE_CHECK) holds every `int` instruction against it and reports
// the asm block that got it wrong.
//
// HOW A SIGNATURE IS WRITTEN
// --------------------------
//      in      registers read:      "ebx:d egx:d ecx:i"    (i = integer, d = decimal)
//      stack   what is popped, top of the stack first:
//                                   "o i i o o"            (o = an object, i/d = a number)
//      out     registers written:   "cmt:d"
//
//      { OP_MATH, "math", "ebx:d egx:d ecx:i", "", "cmt:d" }
//
// WHEN YOU ADD OR CHANGE AN INTERRUPT: update its line in the table. An
// interrupt that is not in the table is not checked.
//

#ifndef SHARP_INTERRUPT_SIGNATURES_H
#define SHARP_INTERRUPT_SIGNATURES_H

#include "../../core/vm_interrupt_flag.h"

struct interrupt_signature {
    int flag;               // interrupt_flag
    const char *name;
    const char *in;         // registers read
    const char *stack;      // values popped, top first
    const char *out;        // registers written
};

/** The signature of an interrupt, or nullptr when it has none in the table. */
const interrupt_signature* find_interrupt_signature(int flag);

#endif //SHARP_INTERRUPT_SIGNATURES_H
