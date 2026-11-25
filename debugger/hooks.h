//
// THE ONE HEADER OF THE DEBUGGER THAT THE VM INCLUDES.
//
// sdb, the Sharp debugger, is the VM itself with this folder compiled in: the
// same interpreter, the same threads, the same collector - so there is never a
// second VM to keep in step. Build the VM with -DSHARP_DEBUGGER_ (and the files
// of debugger) and the hooks below call into the debugger; build it without
// and every hook is empty: the plain `sharp` holds no debugger code and pays
// nothing for it.
//
// RULES FOR A HOOK
//   - the VM names a place ("before an instruction"), never a debugger feature;
//     what happens there is decided in debugger/control
//   - a hook is one line where it is used: no #ifdef in the VM's own files
//   - what a hook hands over is the VM's own structures, read where they are
//
// THE HOOKS
//   SDB_BEFORE_INSTRUCTION(thread, task)
//       in the interpreter's DISPATCH (core/opcode/opcode_macros.h): the
//       instruction at task->pc is about to run on `thread`. Costs one test of
//       `sdb_attention` when the debugger has nothing to watch for.
//
//   SDB_SCHEDULER_TIME(real)
//       in the scheduler's clock (multitasking/scheduler/scheduler.cpp,
//       sched_now_micros): gives the time the scheduler goes by. The plain VM
//       gives `real` back. The debugger leaves out the time the program was
//       held, so that a `delay(1000)` with 400 ms to go still has 400 ms to go
//       after a person looked around for a minute, and no time slice runs out
//       while nothing runs.
//
//   SDB_KEEPS_FIBER(thread)
//       in the scheduler's monitor, where a thread that used up its time slice
//       is asked to switch to another fiber: true when the thread must keep the
//       fiber it runs (it is being stepped). The plain VM: always false.
//
//   SDB_BEFORE_PROGRAM_END(thread)
//       at the top of the VM's shutdown() (vm_initializer.cpp): `thread` is
//       about to end the program - main returned, or exit() was called - and
//       will stop every other thread and wait for each to be gone. While the
//       program is stopped that waits too, like everything else; and from then
//       on no thread stops for the debugger any more, so that every thread can
//       be ended. (Without it: a thread held at a breakpoint is never gone, and
//       the thread ending the program waits for it for ever.)
//

#ifndef SHARP_DEBUGGER_HOOKS_H
#define SHARP_DEBUGGER_HOOKS_H

#ifdef SHARP_DEBUGGER_

#include <cstdint>

struct sharp_thread;
class fiber;

// not 0 while the debugger wants to see every instruction: a breakpoint is set,
// a fiber is being stepped, or the program is being held (control/control.cpp keeps it)
extern volatile int sdb_attention;

void sdb_before_instruction(sharp_thread *thread, fiber *task);
uint64_t sdb_scheduler_time(uint64_t real);
bool sdb_keeps_fiber(sharp_thread *thread);
void sdb_before_program_end(sharp_thread *thread);

#define SDB_BEFORE_INSTRUCTION(thread, task) \
    if(sdb_attention) { sdb_before_instruction(thread, task); }
#define SDB_SCHEDULER_TIME(real) sdb_scheduler_time(real)
#define SDB_KEEPS_FIBER(thread) sdb_keeps_fiber(thread)
#define SDB_BEFORE_PROGRAM_END(thread) sdb_before_program_end(thread)

#else

#define SDB_BEFORE_INSTRUCTION(thread, task)
#define SDB_SCHEDULER_TIME(real) (real)
#define SDB_KEEPS_FIBER(thread) false
#define SDB_BEFORE_PROGRAM_END(thread)

#endif

#endif //SHARP_DEBUGGER_HOOKS_H
