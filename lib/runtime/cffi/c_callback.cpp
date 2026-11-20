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
// WHERE A CALLBACK MAY COME FROM. From a Sharp thread that is inside a C call: it runs right there.
// From a thread C STARTED ITSELF (an audio thread, a worker of a C library): that thread is not
// Sharp's - it has no Sharp stack and the garbage collector does not know it - so the function
// cannot run on it. It is handed to a Sharp thread that serves such calls (c_serve_other_threads()
// in Sharp starts it): the C thread waits, the serving thread runs the function, the C thread gets
// the answer. Without a serving thread the call is reported and answers 0.
//
// WHAT A CALLBACK TAKES AND RETURNS: numbers, pointers, and structs by value - a struct arrives as a
// new object of its class and a returned object is laid out as C's struct (c_bind.cpp does both).
// Not: more than 8 words of arguments on the stack.
//
// No machine code is made here either: the thunks are ordinary C++ functions, one per number,
// written once as a template (see c_call.cpp for the same idea in the other direction).
//
#include "c_callback.h"
#include "c_call.h"
#include "c_bind.h"
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
#include <condition_variable>
#include <atomic>
#include <deque>
#include <chrono>
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
static const int PER_FAMILY = 64;
#elif defined(__x86_64__)
static const int FAMILIES = 4;              // the registers the result travels in: c_call.cpp's four shapes
static const int PER_FAMILY = 64;
#elif defined(__aarch64__)
static const int FAMILIES = 3;              // the result travels in whole-number registers / in floating point ones / is written where x8 says
static const int PER_FAMILY = 64;
#else
static const int FAMILIES = 2;              // the result travels in whole-number / in floating point registers
static const int PER_FAMILY = 64;
#endif

static c_slot slots[FAMILIES][PER_FAMILY];
static std::recursive_mutex slots_mutex;

/** What a thunk hands on: the registers and stack words it was called with. */
struct c_arrival {
    uint8_t ints[8 * 8];
    uint8_t fps[8 * 8];
    uint8_t stack[STACK_WORDS * 8];
    uintptr_t memory = 0;                   // ARM64: where C wants a large struct result written (it says so in x8)
};

/** What a thunk hands back to C: the result registers, as bytes. */
struct c_returned {
    uint8_t ints[16];
    uint8_t fps[32];
};

static void foreign_thread(const c_slot &slot) {
    fprintf(stderr, "sharp: a C callback (handed to `%s`) arrived on a thread C started itself, and nothing serves such calls: it answers 0.\n"
                    "       Call c_serve_other_threads() once, before C can call from threads of its own.\n",
            slot.made_for == nullptr ? "?" : slot.made_for->fullName.c_str());
    fflush(stderr);
}

// ---- calls from threads that are not Sharp's: handed to the thread that serves them

/** One call waiting to be run: what it arrived with, and - when `done` - what it answers. */
struct c_request {
    int family, number;
    const c_arrival *arrival;
    c_returned *out;
    bool done = false;
};

static std::mutex serve_mutex;
static std::condition_variable serve_wake, serve_done;
static std::deque<c_request*> waiting_calls;
static std::atomic<bool> serving(false);

static void run_here(c_slot &slot, const c_arrival &arrival, c_returned &out, bool inside_c_call);

/** Run the Sharp function of a thunk; `out` is what the thunk returns to C. */
static void run(int family, int number, const c_arrival &arrival, c_returned &out) {
    memset(&out, 0, sizeof(out));
    c_slot &slot = slots[family][number];
    sharp_thread *thread = thread_self;
    if(!slot.used || slot.function == nullptr) return;
    if(thread == nullptr) {
        // a thread C started: the serving thread runs it, this one waits for the answer
        if(!serving) { foreign_thread(slot); return; }
        c_request request;
        request.family = family;
        request.number = number;
        request.arrival = &arrival;
        request.out = &out;
        std::unique_lock<std::mutex> lock(serve_mutex);
        waiting_calls.push_back(&request);
        serve_wake.notify_one();
        serve_done.wait(lock, [&] { return request.done; });
        return;
    }
    if(!sni_in_native(thread)) return;
    run_here(slot, arrival, out, true);
}

void c_serve_start() {
    serving = true;
}

bool c_serve_one() {
    sharp_thread *thread = thread_self;
    serving = true;
    c_request *request = nullptr;
    {
        gc_safe_region idle;                // waiting: the collector does not wait for this thread
        std::unique_lock<std::mutex> lock(serve_mutex);
        while(waiting_calls.empty()) {
            if(hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED || vm.state == VM_SHUTTING_DOWN) {
                // the program is ending: nobody waits for an answer for ever
                serving = false;
                return false;
            }
            serve_wake.wait_for(lock, std::chrono::milliseconds(50));
        }
        request = waiting_calls.front();
        waiting_calls.pop_front();
    }                                       // (waits here while a collection is running)
    c_slot &slot = slots[request->family][request->number];
    if(slot.used && slot.function != nullptr) run_here(slot, *request->arrival, *request->out, false);
    {
        std::lock_guard<std::mutex> lock(serve_mutex);
        request->done = true;
    }
    serve_done.notify_all();
    return true;
}

/** The arguments say something the callback cannot do: said once per kind of mistake, on the console. */
static void callback_problem(const c_slot &slot, const std::string &why) {
    fprintf(stderr, "sharp: a function handed to `%s` could not be called by C: %s\n",
            slot.made_for == nullptr ? "?" : slot.made_for->fullName.c_str(), why.c_str());
    fflush(stderr);
}

/**
 * Run the function on THIS thread, which is a Sharp thread. `inside_c_call`: it is inside a C call
 * (counted as paused for the collector); else it is the serving thread, in the VM already.
 */
static void run_here(c_slot &slot, const c_arrival &arrival, c_returned &out, bool inside_c_call) {
    sharp_thread *thread = thread_self;
    // an exception is already on its way out of an earlier callback: nothing more runs until C has returned
    if(hasSignal(thread->signal, tsig_except)) return;

    const c_signature &sig = slot.signature;
    if(inside_c_call) tgc_leave_safe(thread);       // back in the VM: objects may be touched again
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

            // the arguments, read out of the registers and the stack the way the plan says: a number's
            // bytes into `bits`, a struct's into a block of its own
            uint64_t bits[C_MAX_ARGUMENTS];
            memset(bits, 0, sizeof(uint64_t) * count);
            std::vector<std::vector<uint8_t> > structs(count);
            for(size_t a = 0; a < count; a++)
                if(sig.args[a].kind == C_STRUCT) structs[a].assign((size_t) sig.args[a].size + 8, (uint8_t) 0);
            void *result_memory = (void*) arrival.memory;   // where C wants a struct result written (a hidden argument, or x8)
            for(const c_move &move : sig.moves) {
                const uint8_t *from = (move.place == C_IN_INT ? arrival.ints : move.place == C_IN_FP ? arrival.fps : arrival.stack) + move.to;
                if(move.source == C_FROM_RESULT_ADDRESS) {
                    uintptr_t address = 0;
                    memcpy(&address, from, sizeof(address));
                    result_memory = (void*) address;
                } else if(move.source == C_FROM_COPY_ADDRESS) {
                    // the struct came as the address of a copy
                    uintptr_t address = 0;
                    memcpy(&address, from, sizeof(address));
                    if(address != 0) memcpy(structs[move.arg].data(), (const void*) address, sig.args[move.arg].size);
                } else if(sig.args[move.arg].kind == C_STRUCT) {
                    memcpy(structs[move.arg].data() + move.from, from, move.bytes);
                } else {
                    memcpy((uint8_t*) &bits[move.arg] + move.from, from, move.bytes);
                }
            }

            // the function value, then its arguments
            (++task->sp)->obj.o = slot.function;
            TC(TC_ST(task->sp) = tk_any;)      // (the slot holds an object: its number part is nothing in particular)
            std::string why;
            for(size_t a = 0; a < count; a++) {
                stack_item *item = ++task->sp;
                const uint8_t *raw = (const uint8_t*) &bits[a];
                if(sig.args[a].kind == C_STRUCT) {
                    item->obj.o = nullptr;
                    TC(TC_ST(item) = tk_any;)
                    if(!c_struct_to_object(sig.args[a].text, structs[a].data(), &item->obj, why)) throw vm_exception(vm.runtime_except, why);
                    continue;
                }
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
            task->current = inside_c_call && state->function != nullptr ? state->function : slot.made_for;
            call_function_value((Int) count);
            task = thread->task;
            task->current = caller;
            // (a return to native code puts back the code of the function it believes called, and steps
            //  past the call instruction it believes is there: neither is so here)
            task->rom = caller->bytecode;
            task->pc = resume_at;

            if(!hasSignal(thread->signal, tsig_except) && sig.result.kind != C_VOID && task->sp > before) {
                // the result is on top of where the function value was: into the registers C reads it from
                const c_type &r = sig.result;
                if(r.kind == C_STRUCT) {
                    std::vector<uint8_t> bytes((size_t) r.size + 8, (uint8_t) 0);
                    if(!c_struct_from_object(r.text, task->sp->obj.o, bytes.data(), why)) throw vm_exception(vm.runtime_except, why);
                    if(sig.result_in_memory) {
                        if(result_memory != nullptr) memcpy(result_memory, bytes.data(), r.size);
                        uintptr_t address = (uintptr_t) result_memory;
                        memcpy(out.ints, &address, sizeof(address));        // (and C gets the address back)
                    } else {
                        for(const c_move &piece : sig.pieces)
                            memcpy((piece.place == C_IN_INT ? out.ints : out.fps) + piece.from, bytes.data() + piece.to, piece.bytes);
                    }
                } else if(r.kind == C_F64) {
                    memcpy(out.fps, &task->sp->var.d, 8);
                } else if(r.kind == C_F32) {
                    float narrow = (float) task->sp->var.d;     // (a float is the low 4 bytes of the register)
                    memcpy(out.fps, &narrow, 4);
                } else {
                    int64_t whole = (int64_t) task->sp->var.i;
                    memcpy(out.ints, &whole, 8);
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
    if(inside_c_call) tgc_enter_safe(thread);       // C goes on
}

// ---------------------------------------------------------------------------------
// The thunks
// ---------------------------------------------------------------------------------

#define ROW8(F, K) (void*) F<K>, (void*) F<K + 1>, (void*) F<K + 2>, (void*) F<K + 3>, (void*) F<K + 4>, (void*) F<K + 5>, (void*) F<K + 6>, (void*) F<K + 7>
#define ROW64(F) { ROW8(F, 0), ROW8(F, 8), ROW8(F, 16), ROW8(F, 24), ROW8(F, 32), ROW8(F, 40), ROW8(F, 48), ROW8(F, 56) }

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
    c_returned out;
    run(M * 2, N, a, out);
    uint64_t result;
    memcpy(&result, out.ints, 8);
    return result;
}
template<int M, int N> static double thunk_decimal(CB_PARAMS) {
    CB_ARRIVE
    c_returned out;
    run(M * 2 + 1, N, a, out);
    double result;
    memcpy(&result, out.fps, 8);
    return result;
}

#define WROW8(F, M, K) (void*) F<M, K>, (void*) F<M, K + 1>, (void*) F<M, K + 2>, (void*) F<M, K + 3>, (void*) F<M, K + 4>, (void*) F<M, K + 5>, (void*) F<M, K + 6>, (void*) F<M, K + 7>
#define WROW64(F, M) { WROW8(F, M, 0), WROW8(F, M, 8), WROW8(F, M, 16), WROW8(F, M, 24), WROW8(F, M, 32), WROW8(F, M, 40), WROW8(F, M, 48), WROW8(F, M, 56) }
#define MASK(M) WROW64(thunk_whole, M), WROW64(thunk_decimal, M)
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

// what a thunk returns, by the registers the result travels in (the shapes of c_call.cpp)
struct cb_ii { word_t a, b; };
struct cb_dd { double a, b; };
struct cb_id { word_t a; double b; };
struct cb_di { double a; word_t b; };
struct cb_d4 { double a, b, c, d; };

#if CB_INTS == 6
// Linux x64: whole:whole, fp:fp, whole then fp, fp then whole
template<int N> static cb_ii thunk_0(CB_PARAMS) { CB_ARRIVE c_returned out; run(0, N, a, out); cb_ii back; memcpy(&back, out.ints, 16); return back; }
template<int N> static cb_dd thunk_1(CB_PARAMS) { CB_ARRIVE c_returned out; run(1, N, a, out); cb_dd back; memcpy(&back, out.fps, 16); return back; }
template<int N> static cb_id thunk_2(CB_PARAMS) { CB_ARRIVE c_returned out; run(2, N, a, out); cb_id back; memcpy(&back.a, out.ints, 8); memcpy(&back.b, out.fps, 8); return back; }
template<int N> static cb_di thunk_3(CB_PARAMS) { CB_ARRIVE c_returned out; run(3, N, a, out); cb_di back; memcpy(&back.a, out.fps, 8); memcpy(&back.b, out.ints, 8); return back; }
static void *const thunks[FAMILIES][PER_FAMILY] = { ROW64(thunk_0), ROW64(thunk_1), ROW64(thunk_2), ROW64(thunk_3) };
#elif CB_INTS == 8
// ARM64: two whole-number registers, or up to four floating point ones
template<int N> static cb_ii thunk_0(CB_PARAMS) { CB_ARRIVE c_returned out; run(0, N, a, out); cb_ii back; memcpy(&back, out.ints, 16); return back; }
template<int N> static cb_d4 thunk_1(CB_PARAMS) { CB_ARRIVE c_returned out; run(1, N, a, out); cb_d4 back; memcpy(&back, out.fps, 32); return back; }
// A struct of more than 16 bytes is written where the caller says in x8 - a register that is not an
// argument's, and that the thunk's own code is free to use before any statement of ours runs. So the
// thunk is itself a function that returns a large struct: the C++ compiler then keeps that address,
// as the address of the one object the function returns (`back` below IS the caller's memory - the
// compiler builds a function's only returned object in place). Only as many bytes as the real struct
// has are written into it; the rest of `back` is never touched.
struct cb_big { char bytes[C_MAX_RESULT]; };
template<int N> static cb_big thunk_2(CB_PARAMS) {
    cb_big back;
    CB_ARRIVE
    a.memory = (uintptr_t) &back;
    c_returned out;
    run(2, N, a, out);
    return back;
}
static void *const thunks[FAMILIES][PER_FAMILY] = { ROW64(thunk_0), ROW64(thunk_1), ROW64(thunk_2) };
#else
// ARM32: r0:r1, or the floating point registers
template<int N> static uint64_t thunk_0(CB_PARAMS) { CB_ARRIVE c_returned out; run(0, N, a, out); uint64_t back; memcpy(&back, out.ints, 8); return back; }
template<int N> static cb_d4 thunk_1(CB_PARAMS) { CB_ARRIVE c_returned out; run(1, N, a, out); cb_d4 back; memcpy(&back, out.fps, 32); return back; }
static void *const thunks[FAMILIES][PER_FAMILY] = { ROW64(thunk_0), ROW64(thunk_1) };
#endif

static int family_of(const c_signature &sig) {
    return sig.shape < FAMILIES ? sig.shape : 0;
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
    if(sig.variadic_from >= 0) return "a function handed to C cannot have variable arguments";
    if(sig.stack_bytes > (uint32_t) STACK_WORDS * WORD_BYTES) return "a function handed to C takes more arguments than fit (8 words on the stack beside those in registers)";
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
