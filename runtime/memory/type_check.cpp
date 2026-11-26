//
// The number kind checker's report. See type_check.h.
//

#include "type_check.h"

#ifdef SHARP_TYPE_CHECK

#include "../virtual_machine.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/fiber/fiber.h"
#include "../types/sharp_function.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "../types/sharp_type.h"
#include "garbage_collector.h"
#include "../opcode/opcode_macros.h"
#include "../interrupts/interrupt_signatures.h"
#include <cstring>
#include <mutex>
#include <set>
#include <tuple>

static std::mutex reportLock;
static std::set<std::tuple<Int, Int, Int>> reported;    // (function, pc, register or slot)

static const char *kind_name(int kind) {
    return kind == tk_int ? "an integer" : kind == tk_dec ? "a decimal" : "anything";
}

static const char *registerNames[] = {
    "adx", "cx", "cmt", "ebx", "ecx", "ecf", "edf", "ehf", "bmr", "egx"
};

void tc_report(int needed, int found, const char *what, Int index) {
    sharp_thread *thread = thread_self;
    if(thread == NULL || thread->task == NULL || thread->task->current == NULL) return;

    fiber *task = thread->task;
    Int pc = task->pc - task->rom;
    bool isRegister = what[0] == 'r';

    {
        // one line per place: a loop would otherwise print the same mistake a million times
        std::lock_guard<std::mutex> guard(reportLock);
        auto key = std::make_tuple((Int)task->current->address, pc, isRegister ? index : (Int)-1);
        if(!reported.insert(key).second) return;
    }

    string where = what;
    if(isRegister && index >= 0 && index < 10) where += string(" ") + registerNames[index];
    else if(!isRegister) where += " " + std::to_string(index - (task->fp - task->stack)) + " (from fp)";

    fprintf(stderr, "[type check] %s pc %lld (opcode 0x%x): %s holds %s, %s is needed\n",
            task->current->fullName.c_str(), (long long)pc, (unsigned)GET_OP(*task->pc),
            where.c_str(), kind_name(found), kind_name(needed));
}

/** "ebx" -> 3, or -1 */
static int register_index(const char *name, size_t length) {
    for(int i = 0; i < 10; i++) {
        if(strlen(registerNames[i]) == length && strncmp(registerNames[i], name, length) == 0)
            return i;
    }
    return -1;
}

/**
 * Walk a register list of a signature ("ebx:d egx:i") and call
 * visit(register index, kind) for each entry.
 */
template<class Visit>
static void for_each_register(const char *list, Visit visit) {
    const char *at = list;
    while(*at != 0) {
        while(*at == ' ') at++;
        const char *colon = strchr(at, ':');
        if(colon == NULL) break;

        int reg = register_index(at, (size_t)(colon - at));
        int kind = colon[1] == 'd' ? tk_dec : tk_int;
        if(reg >= 0) visit(reg, kind);

        at = colon + 2;
    }
}

void tc_before_interrupt(fiber *task, Int flag) {
    const interrupt_signature *signature = find_interrupt_signature((int)flag);
    if(signature == NULL) return;

    for_each_register(signature->in, [&](int reg, int kind) {
        int found = task->regTags[reg];
        if(found != tk_any && found != kind)
            tc_report(kind, found, "register", reg);
    });

    // the stack, top first: "o i i o o"
    stack_item *slot = task->sp;
    for(const char *at = signature->stack; *at != 0; at++) {
        if(*at == ' ') continue;
        if(slot < task->stack) break;

        if(*at == 'i' || *at == 'd') {
            int kind = *at == 'd' ? tk_dec : tk_int;
            int found = task->slotTags[slot - task->stack];
            if(found != tk_any && found != kind)
                tc_report(kind, found, "stack slot", slot - task->stack);
        }
        slot--;
    }
}

void tc_after_interrupt(fiber *task, Int flag) {
    memset(task->regTags, tk_any, sizeof(task->regTags));

    const interrupt_signature *signature = find_interrupt_signature((int)flag);
    if(signature == NULL) return;

    for_each_register(signature->out, [&](int reg, int kind) {
        task->regTags[reg] = (unsigned char)kind;
    });
}

int tc_field_kind(sharp_object *o, Int index) {
    if(o == NULL || o->type != type_class || o->arrayFlag || index < 0 || index >= (Int)o->size)
        return tk_any;

    sharp_class *sc = vm.classes + CLASS(o->info);
    sharp_field &field = sc->fields[o->staticObject ? sc->instanceFields + index : index];
    if(field.type == NULL || field.type->type > type_var || field.isArray) {
        tc_report(tk_int, tk_any, "register", 0);   // GETF / PUTF on a reference field
        return tk_any;
    }
    return field.type->type == type_var ? tk_dec : tk_int;
}

void tc_forget_slots(unsigned char *first, unsigned char *last) {
    for(unsigned char *tag = first; tag <= last; tag++)
        *tag = tk_any;
}

#endif
