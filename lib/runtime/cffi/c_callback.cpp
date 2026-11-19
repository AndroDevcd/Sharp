//
// C CALLING SHARP: a Sharp function handed to C as a C function pointer
//
//      extern "c" { def qsort(items: _int32[], count: c_size, width: c_size, compare: (pointer, pointer)(c_int)); }
//      qsort(values, n, 4, { a: pointer, b: pointer -> return ...; });
//
// C needs the address of a real C function. There is a fixed set of them here - THUNKS, compiled
// with the VM, each with a number - and a table that says which Sharp function each stands for
// right now. A thunk does the calling convention backwards: it takes every argument register (and
// the first words of the stack), finds its Sharp function in the table, puts the arguments on the
// VM's stack as that function's signature says, runs the function, and returns its result in the
// register C expects it in.
//
//      c_callback_for    a thunk for this function value and signature: its address, for C
//      c_callback_done   the call that was handed it is over: the thunk is free again
//                        (unless the program KEPT the function: c_keep)
//
// HOW LONG A CALLBACK LIVES. By default as long as the C call it was handed to: enough for qsort,
// for "call this for every item". When C keeps the pointer and calls it later (an event handler,
// a timer), the program says so first - c_keep(function) - and the thunk stays that function's
// until c_drop(function); handed over again, the same function gets the same address.
//
// WHERE A CALLBACK MAY COME FROM: the thread that made the C call, while that call runs. A
// callback on a thread C made itself has no Sharp thread to run on; it is reported and answers 0.
//
// WHAT IS NOT DONE YET: struct arguments and struct results of a callback (numbers and pointers
// are), and more than 8 words of stack arguments.
//
// No machine code is made here either: the thunks are ordinary C++ functions, one per number,
// written once as a template (see c_call.cpp for the same idea in the other direction).
//
#include "c_callback.h"
#include "c_call.h"
#include "../sni/sni_runtime.h"
#include "../virtual_machine.h"
#include "../memory/tracing_collector.h"
#include "../memory/garbage_collector.h"
#include "../memory/function_value.h"
#include "../memory/type_check.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/thread/thread_controller.h"
#include "../multitasking/fiber/fiber.h"
#include "../../core/thread_state.h"
#include "../../core/opcode/opcode_macros.h"
#include <mutex>
#include <cstring>
#include <cstdio>

#if defined(_WIN64)
#define CB_WIN64 1
#elif defined(__x86_64__)
#define CB_INTS 6
#elif defined(__aarch64__)
#define CB_INTS 8
#elif defined(__arm__) && defined(__ARM_PCS_VFP)
#define CB_INTS 4
#else
#define CB_NONE 1
#endif

typedef uintptr_t word_t;
static const uint32_t WORD_BYTES = (uint32_t) sizeof(void*);
static const int STACK_WORDS = 8;

/** One thunk's entry in the table: which Sharp function it stands for right now. */
struct c_slot {
    bool used = false;
    bool kept = false;
    sharp_object *function = nullptr;       // the function value (pinned while the slot is used)
    c_signature signature;                  // what C calls it with
    sharp_function *made_for = nullptr;     // the extern function it was handed to (for messages)
};

#if defined(CB_WIN64)
static const int FAMILIES = 32;             // which of the four first arguments are floating point (16) x the result is (2)
static const int PER_FAMILY = 8;
#else
static const int FAMILIES = 2;              // the result is a whole number / a floating point number
static const int PER_FAMILY = 32;
#endif

static c_slot slots[FAMILIES][PER_FAMILY];
static std::recursive_mutex slots_mutex;

/** What a thunk hands on: the registers and stack words it was called with; what it gets back to return. */
struct c_arrival {
    uint8_t ints[8 * 8];
    uint8_t fps[8 * 8];
    uint8_t stack[STACK_WORDS * 8];
};

static void foreign_thread(const c_slot &slot) {
    fprintf(stderr, "sharp: a C callback (handed to `%s`) arrived on a thread Sharp did not start; it cannot run there and answers 0.\n"
                    "       A callback runs on the thread that made the C call, while that call lasts.\n",
            slot.made_for == nullptr ? "?" : slot.made_for->fullName.c_str());
    fflush(stderr);
}

/** Run the Sharp function of a thunk. The result: a whole number's bits, or a decimal in `decimal`. */
static uint64_t run(int family, int number, const c_arrival &arrival, double &decimal) {
    decimal = 0;
    c_slot &slot = slots[family][number];
    sharp_thread *thread = thread_self;
    if(!slot.used || slot.function == nullptr) return 0;
    if(thread == nullptr || !sni_in_native(thread)) { foreign_thread(slot); return 0; }
    // an exception is already on its way out of an earlier callback: nothing more runs until C has returned
    if(hasSignal(thread->signal, tsig_except)) return 0;

    const c_signature &sig = slot.signature;
    uint64_t result = 0;
    tgc_leave_safe(thread);                 // back in the VM: objects may be touched again
    {
        fiber *task = thread->task;
        stack_item *before = task->sp;
        sharp_function *caller = task->current;
        auto resume_at = task->pc;              // where the Sharp caller goes on once the C call is over
        try {
            size_t count = sig.args.size();
            grow_stack_for((Int) count + 2)
            stack_overflow_check_for((Int) count + 2)
            task = thread->task;
            before = task->sp;

            // the function value, then its arguments, read out of the registers the way the plan says
            (++task->sp)->obj.o = slot.function;
            TC(TC_ST(task->sp) = tk_obj;)
            uint64_t bits[C_MAX_ARGUMENTS];
            memset(bits, 0, sizeof(uint64_t) * count);
            for(const c_move &move : sig.moves) {
                const uint8_t *from = (move.place == C_IN_INT ? arrival.ints : move.place == C_IN_FP ? arrival.fps : arrival.stack) + move.to;
                memcpy((uint8_t*) &bits[move.arg] + move.from, from, move.bytes);
            }
            for(size_t a = 0; a < count; a++) {
                stack_item *item = ++task->sp;
                const uint8_t *raw = (const uint8_t*) &bits[a];
                switch(sig.args[a].kind) {
                    case C_I8: { int8_t v; memcpy(&v, raw, 1); item->var.i = v; break; }
                    case C_U8: { uint8_t v; memcpy(&v, raw, 1); item->var.i = v; break; }
                    case C_I16: { int16_t v; memcpy(&v, raw, 2); item->var.i = v; break; }
                    case C_U16: { uint16_t v; memcpy(&v, raw, 2); item->var.i = v; break; }
                    case C_I32: { int32_t v; memcpy(&v, raw, 4); item->var.i = v; break; }
                    case C_U32: { uint32_t v; memcpy(&v, raw, 4); item->var.i = v; break; }
                    case C_F32: { float v; memcpy(&v, raw, 4); item->var.d = v; break; }
                    case C_F64: { memcpy(&item->var.d, raw, 8); break; }
                    case C_PTR: { uintptr_t v = 0; memcpy(&v, raw, sizeof(v)); item->var.i = (Int) v; break; }
                    default: { int64_t v; memcpy(&v, raw, 8); item->var.i = v; break; }
                }
                TC(TC_ST(item) = (sig.args[a].kind == C_F32 || sig.args[a].kind == C_F64) ? tk_dec : tk_int;)
            }

            // The VM runs a Sharp function to its end and comes back here only when the caller is
            // native code (prepare_method). The Sharp function that made the C call is not; the
            // extern function it called is - so for this call that one is the caller.
            sni_thread_state *state = sni_state_of(thread);
            task->current = state->function != nullptr ? state->function : caller;
            call_function_value((Int) count);
            task = thread->task;
            task->current = caller;
            // (a return to native code puts back the code of the function it believes called, and steps
            //  past the call instruction it believes is there: neither is so here)
            task->rom = caller->bytecode;
            task->pc = resume_at;

            if(!hasSignal(thread->signal, tsig_except)) {
                // a result is on top of where the function value was
                bool returns = sig.result.kind != C_VOID;
                if(returns && task->sp > before) {
                    if(sig.result.kind == C_F32 || sig.result.kind == C_F64) decimal = task->sp->var.d;
                    else result = (uint64_t) task->sp->var.i;
                }
            }
            task->sp = before;
        } catch(vm_exception &problem) {
            // (it is raised in Sharp when the C call that is running returns)
            enable_exception_flag(thread, true);
            task = thread->task;
            task->current = caller;
            task->rom = caller->bytecode;
            task->pc = resume_at;
        }
    }
    tgc_enter_safe(thread);                 // C goes on
    return result;
}

/** A float result travels as the low 32 bits of the floating point register. */
static double as_returned(const c_slot &slot, double decimal) {
    if(slot.signature.result.kind != C_F32) return decimal;
    float narrow = (float) decimal;
    uint64_t bits = 0;
    memcpy(&bits, &narrow, 4);
    double carried;
    memcpy(&carried, &bits, 8);
    return carried;
}

// ---------------------------------------------------------------------------------
// The thunks
// ---------------------------------------------------------------------------------

#if defined(CB_WIN64)

template<bool FP> struct cb_slot {
    typedef uint64_t type;
    static void keep(c_arrival &a, int i, type v) { memcpy(a.ints + i * 8, &v, 8); }
};
template<> struct cb_slot<true> {
    typedef double type;
    static void keep(c_arrival &a, int i, type v) { memcpy(a.fps + i * 8, &v, 8); }
};

#define CB_PARAMS typename cb_slot<(M & 1) != 0>::type p0, typename cb_slot<(M & 2) != 0>::type p1, \
                  typename cb_slot<(M & 4) != 0>::type p2, typename cb_slot<(M & 8) != 0>::type p3, \
                  word_t s0, word_t s1, word_t s2, word_t s3, word_t s4, word_t s5, word_t s6, word_t s7
#define CB_ARRIVE c_arrival a; memset(&a, 0, sizeof(a)); \
                  cb_slot<(M & 1) != 0>::keep(a, 0, p0); cb_slot<(M & 2) != 0>::keep(a, 1, p1); \
                  cb_slot<(M & 4) != 0>::keep(a, 2, p2); cb_slot<(M & 8) != 0>::keep(a, 3, p3); \
                  word_t s[8] = { s0, s1, s2, s3, s4, s5, s6, s7 }; memcpy(a.stack, s, sizeof(s));

template<int M, int N> static uint64_t thunk_whole(CB_PARAMS) {
    CB_ARRIVE
    double decimal;
    return run(M * 2, N, a, decimal);
}
template<int M, int N> static double thunk_decimal(CB_PARAMS) {
    CB_ARRIVE
    double decimal;
    run(M * 2 + 1, N, a, decimal);
    return as_returned(slots[M * 2 + 1][N], decimal);
}

#define ROW8(F, M) { (void*) F<M, 0>, (void*) F<M, 1>, (void*) F<M, 2>, (void*) F<M, 3>, (void*) F<M, 4>, (void*) F<M, 5>, (void*) F<M, 6>, (void*) F<M, 7> }
#define MASK(M) ROW8(thunk_whole, M), ROW8(thunk_decimal, M)
static void *const thunks[FAMILIES][PER_FAMILY] = {
    MASK(0), MASK(1), MASK(2), MASK(3), MASK(4), MASK(5), MASK(6), MASK(7),
    MASK(8), MASK(9), MASK(10), MASK(11), MASK(12), MASK(13), MASK(14), MASK(15)
};

static int family_of(const c_signature &sig) {
    return sig.fp_slots * 2 + (sig.shape == 1 ? 1 : 0);
}

#elif defined(CB_INTS)

#if CB_INTS == 6
#define CB_INT_PARAMS word_t r0, word_t r1, word_t r2, word_t r3, word_t r4, word_t r5
#define CB_INT_LIST { r0, r1, r2, r3, r4, r5 }
#elif CB_INTS == 8
#define CB_INT_PARAMS word_t r0, word_t r1, word_t r2, word_t r3, word_t r4, word_t r5, word_t r6, word_t r7
#define CB_INT_LIST { r0, r1, r2, r3, r4, r5, r6, r7 }
#else
#define CB_INT_PARAMS word_t r0, word_t r1, word_t r2, word_t r3
#define CB_INT_LIST { r0, r1, r2, r3 }
#endif
#define CB_PARAMS CB_INT_PARAMS, double f0, double f1, double f2, double f3, double f4, double f5, double f6, double f7, \
                  word_t s0, word_t s1, word_t s2, word_t s3, word_t s4, word_t s5, word_t s6, word_t s7
#define CB_ARRIVE c_arrival a; memset(&a, 0, sizeof(a)); \
                  word_t r[CB_INTS] = CB_INT_LIST; memcpy(a.ints, r, sizeof(r)); \
                  double f[8] = { f0, f1, f2, f3, f4, f5, f6, f7 }; memcpy(a.fps, f, sizeof(f)); \
                  word_t s[8] = { s0, s1, s2, s3, s4, s5, s6, s7 }; memcpy(a.stack, s, sizeof(s));

template<int N> static uint64_t thunk_whole(CB_PARAMS) {
    CB_ARRIVE
    double decimal;
    return run(0, N, a, decimal);
}
template<int N> static double thunk_decimal(CB_PARAMS) {
    CB_ARRIVE
    double decimal;
    run(1, N, a, decimal);
    return as_returned(slots[1][N], decimal);
}

#define ROW8(F, K) (void*) F<K>, (void*) F<K + 1>, (void*) F<K + 2>, (void*) F<K + 3>, (void*) F<K + 4>, (void*) F<K + 5>, (void*) F<K + 6>, (void*) F<K + 7>
static void *const thunks[FAMILIES][PER_FAMILY] = {
    { ROW8(thunk_whole, 0), ROW8(thunk_whole, 8), ROW8(thunk_whole, 16), ROW8(thunk_whole, 24) },
    { ROW8(thunk_decimal, 0), ROW8(thunk_decimal, 8), ROW8(thunk_decimal, 16), ROW8(thunk_decimal, 24) }
};

static int family_of(const c_signature &sig) {
    return sig.shape == 1 ? 1 : 0;
}

#else

static void *const thunks[FAMILIES][PER_FAMILY] = { { nullptr } };
static int family_of(const c_signature &sig) { return 0; }

#endif

// ---------------------------------------------------------------------------------
// Handing a function to C
// ---------------------------------------------------------------------------------

/** Can a callback have this signature (as far as this version goes)? "" when it can. */
static std::string refused(const c_signature &sig) {
#if defined(CB_NONE)
    return "handing a function to C is not available on this processor yet";
#endif
    if(sig.result.kind == C_STRUCT) return "a function handed to C cannot return a struct yet (numbers and pointers can)";
    for(const c_type &type : sig.args)
        if(type.kind == C_STRUCT) return "a function handed to C cannot take a struct by value yet (a pointer to one can)";
    for(const c_move &move : sig.moves)
        if(move.source != C_FROM_VALUE) return "a function handed to C cannot take this argument yet";
    if(sig.stack_bytes > (uint32_t) STACK_WORDS * WORD_BYTES) return "a function handed to C takes more arguments than fit (8 on the stack beside those in registers)";
    return "";
}

void *c_callback_for(sharp_object *function, const std::string &signature, sharp_function *made_for, int &family, int &number) {
    family = -1;
    number = -1;
    if(function == nullptr) return nullptr;                 // null is C's NULL
    std::lock_guard<std::recursive_mutex> guard(slots_mutex);

    c_signature sig;
    std::string why;
    if(!c_parse_signature(signature, sig, why) || !(why = refused(sig)).empty()) {
        throw vm_exception(vm.unsatisfied_link_except, "the function handed to `" + made_for->fullName + "` (C sees it as " + signature + "): " + why);
    }
    int wanted = family_of(sig);

    // a function the program keeps has its thunk already
    int free_one = -1;
    for(int n = 0; n < PER_FAMILY; n++) {
        c_slot &slot = slots[wanted][n];
        if(slot.used && slot.kept && slot.function == function && slot.signature.text == signature) {
            family = wanted;
            number = n;
            return thunks[wanted][n];
        }
        if(!slot.used && free_one < 0) free_one = n;
    }
    if(free_one < 0) {
        throw vm_exception(vm.ill_state_except,
            "too many functions are handed to C at once: " + std::to_string(PER_FAMILY) + " of this shape is the most (handing one to `" + made_for->fullName + "`)."
            "\n    A function C keeps (c_keep) holds its place until c_drop(function).");
    }
    c_slot &slot = slots[wanted][free_one];
    slot.used = true;
    slot.kept = c_is_kept(function);
    slot.function = function;
    slot.signature = sig;
    slot.made_for = made_for;
    tgc_pin(function);
    family = wanted;
    number = free_one;
    return thunks[wanted][free_one];
}

void c_callback_done(int family, int number) {
    if(family < 0 || number < 0) return;
    std::lock_guard<std::recursive_mutex> guard(slots_mutex);
    c_slot &slot = slots[family][number];
    if(!slot.used || slot.kept) return;
    tgc_unpin(slot.function);
    slot = c_slot();
}

// ---------------------------------------------------------------------------------
// Functions C keeps
// ---------------------------------------------------------------------------------

static std::vector<sharp_object*> kept_functions;

bool c_is_kept(sharp_object *function) {
    std::lock_guard<std::recursive_mutex> guard(slots_mutex);
    for(sharp_object *kept : kept_functions)
        if(kept == function) return true;
    return false;
}

void c_keep(sharp_object *function) {
    if(function == nullptr) return;
    std::lock_guard<std::recursive_mutex> guard(slots_mutex);
    if(c_is_kept(function)) return;
    kept_functions.push_back(function);
    tgc_pin(function);                      // (C may be the only one that still knows it)
    // a thunk it has right now, from a call that is still running, stays its own
    for(int f = 0; f < FAMILIES; f++)
        for(int n = 0; n < PER_FAMILY; n++)
            if(slots[f][n].used && slots[f][n].function == function) slots[f][n].kept = true;
}

void c_drop(sharp_object *function) {
    if(function == nullptr) return;
    std::lock_guard<std::recursive_mutex> guard(slots_mutex);
    for(size_t i = 0; i < kept_functions.size(); i++) {
        if(kept_functions[i] != function) continue;
        kept_functions.erase(kept_functions.begin() + (long) i);
        tgc_unpin(function);
        break;
    }
    for(int f = 0; f < FAMILIES; f++) {
        for(int n = 0; n < PER_FAMILY; n++) {
            c_slot &slot = slots[f][n];
            if(slot.used && slot.kept && slot.function == function) {
                tgc_unpin(slot.function);
                slot = c_slot();
            }
        }
    }
}
