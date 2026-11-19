//
// SNI INSIDE THE VM: everything the VM does for a `native` function
// (the library's side is sni.h, in this folder)
//
//      a native call        sni_call_native      the arguments out, the C++ function, the result back
//      finding the C++      sni_link             the function's entry in a loaded library's table
//      libraries            sni_load_library, sni_free_library   (the program loads them: nothing is automatic)
//      the collector        sni_mark_locals      the objects a running native function was given or made
//      stopping             sni_in_native        "is this thread inside C++ right now?"
//
// THE ONE RULE ABOUT FIBERS. A fiber that has a C++ function somewhere on its
// stack cannot be put aside for another fiber: the C++ part of its stack lives
// on the THREAD's machine stack and cannot be taken apart and put back. So while
// `depth` (below) is above 0 the fiber stays on its thread - it may be suspended
// or asked to stop, never switched out (sni_may_switch_fiber).
//

#ifndef SHARP_SNI_RUNTIME_H
#define SHARP_SNI_RUNTIME_H

#include "../../../stdimports.h"
#include "sni.h"
#include <vector>

struct sharp_function;
struct sharp_object;
struct sharp_thread;

/** How long a thread that was asked to stop may stay inside C++ before the VM stops waiting for it (milliseconds). */
extern Int sni_grace_millis;

/** What a thread keeps for its native calls. Made when the thread makes its first one. */
struct sni_thread_state {
    int depth = 0;                              // how many native functions are on this thread's stack right now
    sharp_function *function = nullptr;         // the innermost one (for messages)
    std::vector<sharp_object*> locals;          // objects native code may be using: the collector treats them as in use
    bool pending = false;                       // native code reported a problem: raised when it returns
    std::string exceptionClass, exceptionMessage;
    bool abandoned = false;                     // it ignored a stop for longer than the grace time: nobody waits for it any more
};

/** What `thread` keeps for its native calls (made when it is first asked for). */
sni_thread_state *sni_state_of(sharp_thread *thread);

/** A native function is called: `argSize` arguments (the object first, for a function of a class) are on the stack. */
void sni_call_native(sharp_function *fun, Int argSize);

/** Find the C++ function for `fun` in the libraries the program loaded. Throws when none has it. */
void sni_link(sharp_function *fun);

/** load_library("path") from Sharp - the ONLY way a library gets loaded; throws with the reason when it cannot be. */
void sni_load_library(const std::string &name);
/** free_library("name") from Sharp: 1 when it was loaded and is now released. */
Int sni_free_library(const std::string &name);
/** link_function("library", "function") from Sharp: 1 when that library has the function and it is now linked. */
Int sni_link_from(const std::string &library, sharp_function *fun);

/** For the collector: every object a native function on this thread may be using. */
void sni_mark_locals(sharp_thread *thread, void (*mark)(sharp_object *o));

/** Is this thread inside a native function right now? */
bool sni_in_native(sharp_thread *thread);
/** The name of the native function the thread is in ("" when it is in none). */
std::string sni_native_name(sharp_thread *thread);
/** May the fiber this thread runs be put aside for another one? Not while a native function is on its stack. */
bool sni_may_switch_fiber(sharp_thread *thread);
/** The thread ignored a stop for too long: remember that nobody waits for it. */
void sni_abandon(sharp_thread *thread);
/** Was this thread left behind inside C++ (and is it still there)? */
bool sni_was_abandoned(sharp_thread *thread);
void sni_free_thread_state(sharp_thread *thread);

#endif //SHARP_SNI_RUNTIME_H
