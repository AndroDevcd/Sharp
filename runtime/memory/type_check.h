//
// The number kind checker: a debugging build of the interpreter that verifies
// the compiler's typed bytecode.
//
// WHY IT EXISTS
// -------------
// A number is an INTEGER or a DECIMAL (see the opcode list in
// runtime/opcode/opcode.h). The compiler decides which, and emits integer
// instructions, decimal instructions and conversions accordingly. Nothing at
// run time knows the kind of a value: an integer instruction handed a decimal
// just computes garbage, silently.
//
// So when the compiler forgets a conversion somewhere (passing an integer to a
// decimal parameter, say) the result is a wrong number far away from the
// mistake. This checker makes that mistake loud and exact. Build the runtime
// with
//
//      -DSHARP_TYPE_CHECK
//
// and every register, every stack slot and every number object element carries
// a TAG next to its value: integer, decimal, or "any" (not known yet: a fresh
// local, the result of a system interrupt). Each instruction that needs a
// particular kind checks the tag of what it reads and sets the tag of what it
// writes. A mismatch prints, once per place:
//
//      [type check] main#calc pc 12 (add): register ebx holds a decimal, an integer is needed
//
// The pc is the one in the compiler's listing (`sharpc -d`, the .aout file).
// Run it with -nojit: only the interpreter checks.
//
// Without SHARP_TYPE_CHECK every macro below expands to nothing.
//
// HOW AN INSTRUCTION IS ANNOTATED (virtual_machine.cpp)
// ------------------------------------------------------
//      ADD:
//          TC(TC_REG_INT(triple_arg1) TC_REG_INT(triple_arg2) TC_RT(triple_arg3) = tk_int;)
//          regs[triple_arg3] = regs[triple_arg1] + regs[triple_arg2];
//
// When you add an instruction that reads or writes numbers, annotate it the
// same way.
//

#ifndef SHARP_TYPE_CHECK_H
#define SHARP_TYPE_CHECK_H

#ifdef SHARP_TYPE_CHECK

#include "../stdimports.h"

/** the tag of a value */
enum number_kind_tag {
    tk_any = 0,     // not known: anything may use it
    tk_int = 1,
    tk_dec = 2
};

/** `what` is "register" or "stack slot", `index` the register number or slot index */
void tc_report(int needed, int found, const char *what, Int index);

/**
 * A system interrupt (INT <flag>) is held against its signature in
 * runtime/interrupt_signatures.cpp: before it runs, the registers and stack
 * slots it reads must hold the kinds it expects; afterwards the registers it
 * wrote get their kinds and every other register is "any" (an interrupt may
 * use any of them).
 */
struct fiber;
struct sharp_object;
void tc_before_interrupt(fiber *task, Int flag);
void tc_after_interrupt(fiber *task, Int flag);

/** forget the tags of the slots first..last (a new frame's locals, ...) */
void tc_forget_slots(unsigned char *first, unsigned char *last);

#define TC(...) __VA_ARGS__

// the tag of register r / of the stack slot p points at (lvalues)
#define TC_RT(r) (task->regTags[(r)])
#define TC_ST(p) (task->slotTags[(p) - task->stack])

#define TC_NEED(tag, kind, what, index) \
    { int tcFound = (tag); if(tcFound != tk_any && tcFound != (kind)) tc_report((kind), tcFound, (what), (Int)(index)); }

#define TC_REG_INT(r)  TC_NEED(TC_RT(r), tk_int, "register", (r))
#define TC_REG_DEC(r)  TC_NEED(TC_RT(r), tk_dec, "register", (r))
#define TC_SLOT_INT(p) TC_NEED(TC_ST(p), tk_int, "stack slot", (p) - task->stack)
#define TC_SLOT_DEC(p) TC_NEED(TC_ST(p), tk_dec, "stack slot", (p) - task->stack)

// what the elements of the number object in task->ptr are (any when there is none)
#define TC_PTR_KIND() \
    ((task->ptr != NULL && task->ptr->o != NULL && task->ptr->o->type <= type_var) \
        ? (task->ptr->o->type == type_var ? tk_dec : tk_int) : tk_any)

// what the number field in slot `index` of the object in task->ptr is
#define TC_FIELD_KIND(index) tc_field_kind(task->ptr->o, (index))
int tc_field_kind(sharp_object *o, Int index);

#else

#define TC(...)

#endif

#endif //SHARP_TYPE_CHECK_H
