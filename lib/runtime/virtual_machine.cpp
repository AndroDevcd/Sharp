//
// Created by bknun on 9/18/2022.
//

#include "virtual_machine.h"
#include "sni/sni_runtime.h"
#include "error/vm_exception.h"
#include "../core/opcode/opcode_macros.h"
#include "../core/thread_state.h"
#include "multitasking/thread/sharp_thread.h"
#include "multitasking/fiber/fiber.h"
#include "multitasking/thread/thread_controller.h"
#include "reflect/reflect_helpers.h"
#include "memory/garbage_collector.h"
#include "memory/tracing_collector.h"
#include "memory/type_check.h"
#include "memory/function_value.h"
#include "types/sharp_field.h"
#include "console_helper.h"
#include "../util/time.h"
#include "multitasking/fiber/task_controller.h"
#include "../Modules/math/cmath.h"
#include "../Modules/std/Random.h"
#include "vm_initializer.h"
#include "../util/File.h"
#include "../Modules/std.io/memory.h"
#include "../Modules/std.io/fileio.h"
#include "../Modules/std.io/serialization.h"
#include "../core/access_flag.h"
#include "termios.h"
#include "reflect/reflection.h"
#include "jit/jit_compiler.h"
#include "memory/memory_helpers.h"
#include "main.h"
#include "telemetry/telemetry.h"

virtual_machine vm;
thread_local vm_cell *registers;

// A function is handed to the jit after this many calls...
#define JIT_CALL_THRESHOLD 10
// ...or every time one of its loops has gone around this many times in the
// interpreter (must be a power of two minus one, it is used as a bit mask).
#define JIT_LOOP_MASK 0x3ff

bool jit_try_compile(sharp_function *function);
int jit_enter_loop(Int targetPc);

/*
 * A backward jump is the bottom of a loop. Hot loops are worth compiling even
 * when the function that contains them is only called once, so every
 * JIT_LOOP_MASK+1 trips we try to continue the loop in compiled code.
 * Used by JMP and by taken JE / JNE (a loop whose test sits at the bottom ends
 * in a conditional backward jump).
 */
#define loop_back_edge(target) \
    if((target) <= current_pc && (++task->current->loopCount & JIT_LOOP_MASK) == 0 && c_options.jit) { \
        data = jit_enter_loop(target); \
        if(data == 2) return;               /* the function returned to a native/jit caller */ \
        if(data == 1) { check_state(0) }    /* compiled code ran, continue wherever task->pc is now */ \
    }

static bool continues_in_machine_code(fiber *task, sharp_function *function);    // below, with the rebuild walk

void main_vm_loop(sharp_function *frame)
{
    setupOpcodeTable
    TELEMETRY_SCOPE(tm_mode_interpreter) // everything in this loop is interpreter time unless a nested scope says otherwise
    TELEMETRY_OPCODE_SETUP
    auto thread = thread_self;
    auto task = thread->task;
    vm_cell *regs = registers;
    Int data;

    // Continuing a fiber after a context switch: walk up its call stack and
    // give every function that was running as machine code its machine frame
    // back. (By call depth, not by function - see next_rebuild_frame(). Which
    // functions: see continues_in_machine_code().)
    while(thread_self->task->rebuilding) {
        frame = next_rebuild_frame();
        if(frame == nullptr) break;

        if(continues_in_machine_code(thread_self->task, frame)) {
            // The compiled frame rebuilds and runs everything above it. Once it
            // comes back the call stack has changed, so stop walking and let the
            // interpreter continue from wherever task->pc is now.
            invoke_next_frame(frame, true);
            break;
        }
    }

    run:
    try
    {
        if(hasSignal(thread->signal, tsig_except))
            goto catch_exception;

        for(;;) {
            DISPATCH()

            _NOP:
                branch
            _INT:
                TC(Int interruptFlag = single_arg; tc_before_interrupt(task, interruptFlag);)
                {
                    TELEMETRY_SCOPE(tm_mode_interrupt)
                    TELEMETRY_COUNT(tm_interrupts)
                    exec_interrupt(single_arg);
                }
                TC(tc_after_interrupt(task, interruptFlag);)
                if(vm.state == VM_TERMINATED) {
                    send_interrupt_signal(thread);
                    return;
                }
                check_state(1)
            _MOVI:
                TC(TC_RT(single_arg) = tk_int;)
                regs[single_arg].i = raw_arg2;
                branch_for(2)
            RET:
                data = single_arg;
                if(data == ERR_STATE && task->exceptionObject.o == nullptr) {
                    copy_object(
                            &task->exceptionObject,
                            pop_stack_object.o
                    );
                }

                if(return_method()) {
                    if(data == ERR_STATE) {
                        // Leave task->pc on the CALL in the caller: that is the pc
                        // catch_exception() uses to find the enclosing try block.
                        enable_exception_flag(thread, true);
                    } else {
                        task->pc++;
                    }
                    return;
                }

                if(data == ERR_STATE) {
                    enable_exception_flag(thread, true);
                    goto catch_exception;
                }
                check_state(1)
            HLT:
                send_interrupt_signal(thread);
                return;
            NEWARRAY:
                TC(TC_REG_INT(dual_arg1))
                grow_stack
                stack_overflow_check
                copy_object(
                    &push_stack_object,
                    create_object(regs[dual_arg1].i, (data_type)dual_arg2)
                );
                check_state(1)
            CAST:
                require_object_with_value(
                    cast_object(task->ptr, single_arg);
                )
                branch
            VARCAST:
                require_object_with_value(
                    cast_numeric_array(task->ptr, dual_arg1);
                )
                branch
            // Narrowing casts go through a 64-bit integer first, so a value that does not fit
            // wraps around (like an integer cast in C or Java). Converting the vm_cell
            // straight to a small type is undefined for out of range values (x87 gives
            // INT_MIN) and would not match what the JIT does (emit_mov8.. in jit/x64/opcode).
            MOV8:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (int8_t)(int64_t)regs[dual_arg2].i;
                branch
            MOV16:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (int16_t)(int64_t)regs[dual_arg2].i;
                branch
            MOV32:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (int32_t)(int64_t)regs[dual_arg2].i;
                branch
            MOV64:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (int64_t)regs[dual_arg2].i;
                branch
            MOVU8:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (uint8_t)(int64_t)regs[dual_arg2].i;
                branch
            MOVU16:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (uint16_t)(int64_t)regs[dual_arg2].i;
                branch
            MOVU32:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (uint32_t)(int64_t)regs[dual_arg2].i;
                branch
            MOVU64:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = (uint64_t)regs[dual_arg2].i;
                branch
            RSTORE:
                // (the tag of the new top is set AFTER grow_stack: when this push is the one that grows the
                // stack, sp + 1 is past the end of the tag array before it - a heap overflow by one byte in
                // the kind-check build, found with the bootstrap's -O builds; the same in the pushes below)
                grow_stack
                stack_overflow_check
                TC(TC_ST(task->sp + 1) = TC_RT(single_arg);)
                push_stack_number = regs[single_arg];
                branch
            ADD:
                TC(TC_REG_INT(triple_arg1) TC_REG_INT(triple_arg2) TC_RT(triple_arg3) = tk_int;)
                regs[triple_arg3].i = regs[triple_arg1].i + regs[triple_arg2].i;
                branch
            SUB:
                TC(TC_REG_INT(triple_arg1) TC_REG_INT(triple_arg2) TC_RT(triple_arg3) = tk_int;)
                regs[triple_arg3].i = regs[triple_arg1].i - regs[triple_arg2].i;
                branch
            MUL:
                TC(TC_REG_INT(triple_arg1) TC_REG_INT(triple_arg2) TC_RT(triple_arg3) = tk_int;)
                regs[triple_arg3].i = regs[triple_arg1].i * regs[triple_arg2].i;
                branch
            DDIV:
                TC(TC_REG_DEC(triple_arg1) TC_REG_DEC(triple_arg2) TC_RT(triple_arg3) = tk_dec;)
                regs[triple_arg3].d = regs[triple_arg1].d / regs[triple_arg2].d;
                branch
            MOD:
                TC(TC_REG_INT(triple_arg1) TC_REG_INT(triple_arg2) TC_RT(triple_arg3) = tk_int;)
                regs[triple_arg3].i = vm_remainder((Int)regs[triple_arg1].i, (Int)regs[triple_arg2].i);
                branch
            IADD:
                TC(TC_REG_INT(single_arg))
                regs[single_arg].i += raw_arg2;
                branch_for(2)
            ISUB:
                TC(TC_REG_INT(single_arg))
                regs[single_arg].i -= raw_arg2;
                branch_for(2)
            IMUL:
                TC(TC_REG_INT(single_arg))
                regs[single_arg].i *= raw_arg2;
                branch_for(2)
            DIDIV:
                TC(TC_REG_DEC(single_arg))
                regs[single_arg].d /= raw_arg2;
                branch_for(2)
            IMOD:
                TC(TC_REG_INT(single_arg))
                regs[single_arg].i = vm_remainder((Int)regs[single_arg].i, (Int)raw_arg2);
                branch_for(2)
            POP:
                copy_object(&pop_stack_object, (sharp_object*)nullptr);
                branch
            INC:
                TC(TC_REG_INT(single_arg))
                regs[single_arg].i++;
                branch
            DEC:
                TC(TC_REG_INT(single_arg))
                regs[single_arg].i--;
                branch
            MOVR:
                TC(TC_RT(dual_arg1) = TC_RT(dual_arg2);)
                regs[dual_arg1] = regs[dual_arg2];
                branch
            BRH:
                TC(TC_REG_INT(ADX))
                task->pc = task->rom + (Int)regs[ADX].i;
                check_state(0)
            IFE:
                TC(TC_REG_INT(CMT) TC_REG_INT(ADX))
                if(regs[CMT].i) {
                    task->pc = task->rom + (Int)regs[ADX].i;
                    check_state(0)
                } else check_state(1)
            IFNE:
                TC(TC_REG_INT(CMT) TC_REG_INT(ADX))
                if(regs[CMT].i == 0) {
                    task->pc = task->rom + (Int)regs[ADX].i;
                    check_state(0)
                } else check_state(1)
            LT:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].i < regs[dual_arg2].i;
                branch
            GT:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].i > regs[dual_arg2].i;
                branch
            LTE:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].i <= regs[dual_arg2].i;
                branch
            GTE:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].i >= regs[dual_arg2].i;
                branch
            MOVL:
                task->ptr = &(task->fp+single_arg)->obj;
                branch
            POPL:
                copy_object(
                    &(task->fp+single_arg)->obj,
                    pop_stack_object.o
                );
                branch
            IPOPL:
                TC(TC_ST(task->fp + single_arg) = TC_ST(task->sp);)
                (task->fp+single_arg)->var = pop_stack_number;
                branch
            MOVSL:
                task->ptr = &((task->sp+single_arg)->obj);
                branch
            SIZEOF:
                TC(TC_RT(single_arg) = tk_int;)
                regs[single_arg].i= ( task->ptr!=NULL && task->ptr->o != NULL) ? task->ptr->o->size : 0;
                branch
            PUT:
                cout << regs[single_arg].i;
                branch
            PUTC:
                TC(TC_REG_INT(single_arg))
                printf("%c", (char)regs[single_arg].i);
                branch
            GET:
                TC(TC_REG_INT(CMT) TC_RT(single_arg) = tk_int;)
                {
                    gc_safe_region waitingForKey;
                    regs[single_arg].i = (regs[CMT].i != 0) ? getche() : getch();
                }
                branch
            CHECKLEN:
                TC(TC_REG_INT(single_arg))
                require_object_with_value(
                        if(regs[single_arg].i < task->ptr->o->size && regs[single_arg].i >= 0) {
                            branch
                        }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << regs[single_arg].i << " size is " << task->ptr->o->size;
                            throw vm_exception(vm.bounds_except, ss.str());
                        }
                )
            JMP:
                loop_back_edge(single_arg)
                task->pc = task->rom + single_arg;
                check_state(0)
            LOADPC:
                TC(TC_RT(single_arg) = tk_int;)
                regs[single_arg].i = current_pc;
                branch
            PUSHOBJ:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, task->ptr);
                branch
            DEL:
                copy_object(task->ptr, (sharp_object*) nullptr);
                branch
            CALL:
                prepare_method(single_arg);
                check_state(0) // a jit compiled callee can come back with an exception/signal pending
            CALLD:  // call the function value under the a arguments (memory/function_value.h)
                call_function_value(single_arg);
                check_state(0)
            NEWCLASS:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, create_object(&vm.classes[raw_arg2]));
                check_state(2)
            MOVN:
                require_object_with_value(
                        #ifdef SAFE_EXECUTION
                        if(task->ptr->o->type <= type_var) {
                            vm_exception err(vm.ill_state_except, "movn on number");
                            enable_exception_flag(thread, true);
                            goto catch_exception;
                        }
                        #endif

                        task->ptr = task->ptr->o->node + raw_arg2;
                )
                branch_for(2)
            SLEEP:
                TC(TC_REG_INT(single_arg))
                {
                    TELEMETRY_SCOPE(tm_mode_sleep)
                    __os_sleep(regs[single_arg].i);
                }
                branch
            TEST:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = regs[dual_arg1].i == regs[dual_arg2].i;
                branch
            TNE:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = regs[dual_arg1].i != regs[dual_arg2].i;
                branch
            LOCK:
                require_object_with_value(
                    if(lock_object(task->ptr->o)) {
                        return;
                    }
                )
                branch
            ULOCK:
                require_object_with_value(
                    unlock_object(task->ptr->o);
                )
                branch
            MOVG:
                task->ptr = vm.staticHeap + single_arg;
                branch
            MOVND:
                TC(TC_REG_INT(single_arg))
                require_object_with_value(
                    #ifdef SAFE_EXECUTION
                     if(task->ptr->o->type <= type_var) {
                         vm_exception err(vm.ill_state_except, "movn on number");
                         enable_exception_flag(thread, true);
                         goto catch_exception;
                     }
                    #endif

                    task->ptr = task->ptr->o->node + (Int)regs[single_arg].i;
                )
                branch
            NEWOBJARRAY:
                TC(TC_REG_INT(single_arg))
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, create_object(regs[single_arg].i));
                check_state(1)
            NOT:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = !regs[dual_arg2].i;
                branch
            SKIP: // todo: phase out
                task->pc += single_arg;
                branch
            LOADVAL:
                TC(TC_RT(single_arg) = TC_ST(task->sp);)
                regs[single_arg] = pop_stack_number;
                branch
            SHL:
                TC(TC_REG_INT(triple_arg1) TC_REG_INT(triple_arg2) TC_RT(triple_arg3) = tk_int;)
                regs[triple_arg3].i = (Int)regs[triple_arg1].i << (Int)regs[triple_arg2].i;
                branch
            SHR:
                TC(TC_REG_INT(triple_arg1) TC_REG_INT(triple_arg2) TC_RT(triple_arg3) = tk_int;)
                regs[triple_arg3].i = (Int)regs[triple_arg1].i >> (Int)regs[triple_arg2].i;
                branch
            SKPE: // todo: phase out
                TC(TC_REG_INT(dual_arg1))
                if((Int)regs[dual_arg1].i != 0) {
                    task->pc = task->pc + dual_arg2;
                    check_state(0)
                } else check_state(1)
            SKNE: // todo: phase out
                TC(TC_REG_INT(dual_arg1))
                if((Int)regs[dual_arg1].i == 0) {
                    task->pc = task->pc + dual_arg2;
                    check_state(0)
                } else check_state(1)
            CMP:
                TC(TC_REG_INT(single_arg) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[single_arg].i == raw_arg2;
                branch_for(2)
            AND:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = regs[dual_arg1].i && regs[dual_arg2].i;
                branch
            UAND:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = (Int)regs[dual_arg1].i & (Int)regs[dual_arg2].i;
                branch
            OR:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = (Int)regs[dual_arg1].i | (Int)regs[dual_arg2].i;
                branch
            XOR:
                TC(TC_REG_INT(dual_arg1) TC_REG_INT(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = (Int)regs[dual_arg1].i ^ (Int)regs[dual_arg2].i;
                branch
            THROW:
                copy_object(&task->exceptionObject, pop_stack_object.o);
                enable_exception_flag(thread, true);
                check_state(0)
            CHECKNULL:
                TC(TC_RT(single_arg) = tk_int;)
                regs[single_arg].i = task->ptr == nullptr || task->ptr->o == nullptr;
                branch
            RETURNOBJ:
                copy_object(&task->fp->obj, task->ptr);
                branch
            NEWCLASSARRAY:
                TC(TC_REG_INT(single_arg))
                grow_stack
                stack_overflow_check
                copy_object(
                        &push_stack_object,
                        create_object(vm.classes + raw_arg2, (Int)regs[single_arg].i)
                );
                check_state(2)
            NEWSTRING:
                grow_stack
                stack_overflow_check
                data = single_arg;
                copy_object(
                    &push_stack_object,
                    create_object(
                       vm.strings[data].size(),
                        type_int8
                    )
                );

                if(vm.strings[data].size() > 0)
                    assign_string_field(task->sp->obj.o, vm.strings[data]);
                check_state(1)
            ADDL:
                TC(TC_REG_INT(dual_arg1) TC_SLOT_INT(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.i += regs[dual_arg1].i;
                branch
            SUBL:
                TC(TC_REG_INT(dual_arg1) TC_SLOT_INT(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.i -= regs[dual_arg1].i;
                branch
            MULL:
                TC(TC_REG_INT(dual_arg1) TC_SLOT_INT(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.i *= regs[dual_arg1].i;
                branch
            DDIVL:
                TC(TC_REG_DEC(dual_arg1) TC_SLOT_DEC(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.d /= regs[dual_arg1].d;
                branch
            MODL:
                TC(TC_REG_INT(dual_arg1) TC_SLOT_INT(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.i = vm_remainder((Int)(task->fp + dual_arg2)->var.i, (Int)regs[dual_arg1].i);
                branch
            IADDL:
                TC(TC_SLOT_INT(task->fp + single_arg))
                (task->fp + single_arg)->var.i += raw_arg2;
                branch_for(2)
            ISUBL:
                TC(TC_SLOT_INT(task->fp + single_arg))
                (task->fp + single_arg)->var.i -= raw_arg2;
                branch_for(2)
            IMULL:
                TC(TC_SLOT_INT(task->fp + single_arg))
                (task->fp + single_arg)->var.i *= raw_arg2;
                branch_for(2)
            DIDIVL:
                TC(TC_SLOT_DEC(task->fp + single_arg))
                (task->fp + single_arg)->var.d /= raw_arg2;
                branch_for(2)
            IMODL:
                TC(TC_SLOT_INT(task->fp + single_arg))
                (task->fp + single_arg)->var.i = vm_remainder((Int)(task->fp + single_arg)->var.i, (Int)raw_arg2);
                branch_for(2)
            LOADL:
                TC(TC_RT(dual_arg1) = TC_ST(task->fp + dual_arg2);)
                regs[dual_arg1] = (task->fp + dual_arg2)->var;
                branch
            POPOBJ:
                require_object(
                    copy_object(task->ptr, pop_stack_object.o);
                )
                branch
            SMOVR:
                TC(TC_ST(task->sp + dual_arg2) = TC_RT(dual_arg1);)
                (task->sp + dual_arg2)->var = regs[dual_arg1];
                branch
            SMOVR_2:
                TC(TC_ST(task->fp + dual_arg2) = TC_RT(dual_arg1);)
                (task->fp + dual_arg2)->var = regs[dual_arg1];
                branch
            SMOVR_3:
                copy_object(&(task->fp + single_arg)->obj, task->ptr);
                branch
            SMOVR_4:
                TC(TC_ST(task->fp + single_arg) = TC_ST(task->fp + raw_arg2);)
                (task->fp + single_arg)->var = (task->fp + raw_arg2)->var;
                branch_for(2)
            ANDL:
                TC(TC_REG_INT(dual_arg1) TC_SLOT_INT(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.i = (Int)(task->fp + dual_arg2)->var.i & (Int)regs[dual_arg1].i;
                branch
            ORL:
                TC(TC_REG_INT(dual_arg1) TC_SLOT_INT(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.i = (Int)(task->fp + dual_arg2)->var.i | (Int)regs[dual_arg1].i;
                branch
            XORL:
                TC(TC_REG_INT(dual_arg1) TC_SLOT_INT(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.i = (Int)(task->fp + dual_arg2)->var.i ^ (Int)regs[dual_arg1].i;
                branch
            NEG:
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = -regs[dual_arg2].i;
                branch
            SMOV:
                TC(TC_RT(dual_arg1) = TC_ST(task->sp + dual_arg2);)
                regs[dual_arg1] = (task->sp + dual_arg2)->var;
                branch
            RETURNVAL:
                TC(TC_ST(task->fp) = TC_RT(single_arg);)
                task->fp->var = regs[single_arg];
                branch
            ISTORE:
                grow_stack
                stack_overflow_check
                TC(TC_ST(task->sp + 1) = tk_int;)
                push_stack_number.i = raw_arg2;
                branch_for(2)
            ISTOREL:
                TC(TC_ST(task->fp + single_arg) = tk_int;)
                (task->fp + single_arg)->var.i = raw_arg2;
                branch_for(2)
            PUSHNULL:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, (sharp_object*) nullptr);
                branch
            IPUSHL:
                grow_stack
                stack_overflow_check
                TC(TC_ST(task->sp + 1) = TC_ST(task->fp + single_arg);)
                push_stack_number = (task->fp + single_arg)->var;
                branch
            PUSHL:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, (task->fp + single_arg)->obj.o);
                branch
            ITEST:
                TC(TC_RT(single_arg) = tk_int;)
                data = (Int)pop_stack_object.o;
                regs[single_arg].i = (sharp_object*)data == pop_stack_object.o;
                branch
            INVOKE_DELEGATE:
                invoke_delegate(single_arg, dual_raw_arg2, dual_raw_arg1);
                check_state(0)
            ISADD:
                TC(TC_SLOT_INT(task->sp + single_arg))
                (task->sp + single_arg)->var.i += raw_arg2;
                branch_for(2)
            JE:
                TC(TC_REG_INT(CMT))
                if(regs[CMT].i) {
                    loop_back_edge(single_arg)   // the optimizer puts the loop test at the bottom of the loop
                    task->pc = task->rom + single_arg;
                    check_state(0)
                } else check_state(1)
            JNE:
                TC(TC_REG_INT(CMT))
                if(regs[CMT].i == 0) {
                    loop_back_edge(single_arg)
                    task->pc = task->rom + single_arg;
                    check_state(0)
                } else check_state(1)
            TLS_MOVL:
                task->ptr = &(task->stack + single_arg)->obj;
                branch
            MOV_ABS:
                TC(TC_ST(task->stack + single_arg) = TC_RT(EBX);)
                (task->stack + single_arg)->var = regs[EBX];
                branch
            LOAD_ABS:
                TC(TC_RT(EBX) = TC_ST(task->stack + single_arg);)
                regs[EBX] = (task->stack + single_arg)->var;
                branch
            DUP:
                // Like every other push: make room first. (This was the one push
                // without the check - with the stack exactly full it wrote one
                // slot past the end of the stack's memory.) The slot to copy is
                // looked up afterwards: growing may move the stack.
                grow_stack
                stack_overflow_check
                {
                    object *top = &task->sp->obj;
                    copy_object(&push_stack_object, top);
                }
                branch
            POPOBJ_2:
                task->ptr = &pop_stack_object;
                branch
            SWAP:
                TC({ unsigned char tcSwap = TC_ST(task->sp); TC_ST(task->sp) = TC_ST(task->sp - 1); TC_ST(task->sp - 1) = tcSwap; })
                if((task->sp-task->stack) >= 2) {
                    auto swappedObj = task->sp->obj.o;
                    (task->sp)->obj.o = (task->sp-1)->obj.o;
                    (task->sp-1)->obj.o = swappedObj;
                    vm_cell swappedVar = task->sp->var;
                    (task->sp)->var = (task->sp-1)->var;
                    (task->sp-1)->var = swappedVar;
                } else {
                    stringstream ss;
                    ss << "Illegal stack swap, not enough data";
                    throw vm_exception(vm.thread_stack_except, ss.str());
                }
                branch
            DEXP:
                TC(TC_REG_DEC(triple_arg1) TC_REG_DEC(triple_arg2) TC_RT(triple_arg3) = tk_dec;)
                regs[triple_arg3].d=pow(regs[triple_arg1].d, regs[triple_arg2].d);
                branch
            LDC: // tested
                TC(TC_RT(dual_arg1) = tk_dec;)
                regs[dual_arg1].d=vm.constants[dual_arg2];
                branch
            IS:
                TC(TC_RT(single_arg) = tk_int;)
                regs[single_arg].i = is_type(task->ptr, raw_arg2);
                branch_for(2)
        }

            // ---- typed numbers: the decimal twins and the conversions --------------
            // (see the opcode list in lib/core/opcode/opcode.h). A register holds
            // either a 64-bit integer (.i) or a 64-bit double (.d); the plain
            // instructions above work on .i, the D twins below on .d.
            DADD:
                TC(TC_REG_DEC(triple_arg1) TC_REG_DEC(triple_arg2) TC_RT(triple_arg3) = tk_dec;)
                regs[triple_arg3].d = regs[triple_arg1].d + regs[triple_arg2].d;
                branch
            DSUB:
                TC(TC_REG_DEC(triple_arg1) TC_REG_DEC(triple_arg2) TC_RT(triple_arg3) = tk_dec;)
                regs[triple_arg3].d = regs[triple_arg1].d - regs[triple_arg2].d;
                branch
            DMUL:
                TC(TC_REG_DEC(triple_arg1) TC_REG_DEC(triple_arg2) TC_RT(triple_arg3) = tk_dec;)
                regs[triple_arg3].d = regs[triple_arg1].d * regs[triple_arg2].d;
                branch
            DNEG:
                TC(TC_REG_DEC(dual_arg2) TC_RT(dual_arg1) = tk_dec;)
                regs[dual_arg1].d = -regs[dual_arg2].d;
                branch
            DINC:
                TC(TC_REG_DEC(single_arg))
                regs[single_arg].d++;
                branch
            DDEC:
                TC(TC_REG_DEC(single_arg))
                regs[single_arg].d--;
                branch
            DLT:
                TC(TC_REG_DEC(dual_arg1) TC_REG_DEC(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].d < regs[dual_arg2].d;
                branch
            DGT:
                TC(TC_REG_DEC(dual_arg1) TC_REG_DEC(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].d > regs[dual_arg2].d;
                branch
            DLTE:
                TC(TC_REG_DEC(dual_arg1) TC_REG_DEC(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].d <= regs[dual_arg2].d;
                branch
            DGTE:
                TC(TC_REG_DEC(dual_arg1) TC_REG_DEC(dual_arg2) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[dual_arg1].d >= regs[dual_arg2].d;
                branch
            DTEST:
                TC(TC_REG_DEC(dual_arg1) TC_REG_DEC(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = regs[dual_arg1].d == regs[dual_arg2].d;
                branch
            DTNE:
                TC(TC_REG_DEC(dual_arg1) TC_REG_DEC(dual_arg2) TC_RT(BMR) = tk_int;)
                regs[BMR].i = regs[dual_arg1].d != regs[dual_arg2].d;
                branch
            DNOT:
                TC(TC_REG_DEC(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = regs[dual_arg2].d == 0;    // 1 when the decimal is 0.0
                branch
            DCMP:
                TC(TC_REG_DEC(single_arg) TC_RT(CMT) = tk_int;)
                regs[CMT].i = regs[single_arg].d == raw_arg2;
                branch_for(2)
            DIADD:
                TC(TC_REG_DEC(single_arg))
                regs[single_arg].d += raw_arg2;
                branch_for(2)
            DISUB:
                TC(TC_REG_DEC(single_arg))
                regs[single_arg].d -= raw_arg2;
                branch_for(2)
            DIMUL:
                TC(TC_REG_DEC(single_arg))
                regs[single_arg].d *= raw_arg2;
                branch_for(2)
            DADDL:
                TC(TC_REG_DEC(dual_arg1) TC_SLOT_DEC(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.d += regs[dual_arg1].d;
                branch
            DSUBL:
                TC(TC_REG_DEC(dual_arg1) TC_SLOT_DEC(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.d -= regs[dual_arg1].d;
                branch
            DMULL:
                TC(TC_REG_DEC(dual_arg1) TC_SLOT_DEC(task->fp + dual_arg2))
                (task->fp + dual_arg2)->var.d *= regs[dual_arg1].d;
                branch
            DIADDL:
                TC(TC_SLOT_DEC(task->fp + single_arg))
                (task->fp + single_arg)->var.d += raw_arg2;
                branch_for(2)
            DISUBL:
                TC(TC_SLOT_DEC(task->fp + single_arg))
                (task->fp + single_arg)->var.d -= raw_arg2;
                branch_for(2)
            DIMULL:
                TC(TC_SLOT_DEC(task->fp + single_arg))
                (task->fp + single_arg)->var.d *= raw_arg2;
                branch_for(2)
            DISADD:
                TC(TC_SLOT_DEC(task->sp + single_arg))
                (task->sp + single_arg)->var.d += raw_arg2;
                branch_for(2)
            DISTORE:
                grow_stack
                stack_overflow_check
                TC(TC_ST(task->sp + 1) = tk_dec;)
                push_stack_number.d = raw_arg2;
                branch_for(2)
            DISTOREL:
                TC(TC_ST(task->fp + single_arg) = tk_dec;)
                (task->fp + single_arg)->var.d = raw_arg2;
                branch_for(2)
            DMOVI:
                TC(TC_RT(single_arg) = tk_dec;)
                regs[single_arg].d = raw_arg2;
                branch_for(2)
            DPUT:
                TC(TC_REG_DEC(single_arg))
                cout << natural_number(regs[single_arg].d);     // one way to write a number, everywhere (console_helper.cpp)
                branch
            I2D:    // integer -> decimal (nothing to do while both are stored as doubles)
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = tk_dec;)
                regs[dual_arg1].d = (double)regs[dual_arg2].i;
                branch
            D2I:    // decimal -> integer: the fraction is dropped (2.9 -> 2, -2.9 -> -2)
                TC(TC_REG_DEC(dual_arg2) TC_RT(dual_arg1) = tk_int;)
                regs[dual_arg1].i = decimal_to_integer(regs[dual_arg2].d);
                branch
            // ---- number fields of the object in the object pointer --------------
            // (see "THE SLOTS OF A CLASS INSTANCE" in memory/sharp_object.h)
            // The whole cell is copied: the compiler knows whether the field is an
            // integer or a decimal and uses the register accordingly.
            GETF:   // regs[a] = ptr->o.<number field b>
                require_object_with_value(
                    TC(TC_RT(dual_arg1) = TC_FIELD_KIND(dual_arg2);)
                    regs[dual_arg1] = number_slot(task->ptr->o, dual_arg2);
                )
                branch
            PUTF:   // ptr->o.<number field b> = regs[a]
                require_object_with_value(
                    TC(TC_NEED(TC_RT(dual_arg1), TC_FIELD_KIND(dual_arg2), "register", dual_arg1))
                    number_slot(task->ptr->o, dual_arg2) = regs[dual_arg1];
                )
                branch
            // ---- typed number array elements -----------------------------------
            // The array is in the object pointer, the index in an integer register.
            // The instruction says how wide an element is and whether it is signed;
            // the compiler picks it from the array's type (see element_size() in
            // memory/sharp_object.h). ALOAD64 / ASTORE64 copy 8 bytes as they are:
            // that is _int64, _uint64, a function pointer, or a decimal.
            //      C++ comparison: the cast in  (int64_t)((int8_t*)data)[i]
            ALOAD8:   // regs[a] = array[regs[b]]
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = TC_PTR_KIND();)
                require_numeric_object_with_value(
                    regs[dual_arg1].i = ((int8_t*)task->ptr->o->data)[regs[dual_arg2].i];
                )
                branch
            ALOADU8:   // regs[a] = array[regs[b]]
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = TC_PTR_KIND();)
                require_numeric_object_with_value(
                    regs[dual_arg1].i = ((uint8_t*)task->ptr->o->data)[regs[dual_arg2].i];
                )
                branch
            ALOAD16:   // regs[a] = array[regs[b]]
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = TC_PTR_KIND();)
                require_numeric_object_with_value(
                    regs[dual_arg1].i = ((int16_t*)task->ptr->o->data)[regs[dual_arg2].i];
                )
                branch
            ALOADU16:   // regs[a] = array[regs[b]]
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = TC_PTR_KIND();)
                require_numeric_object_with_value(
                    regs[dual_arg1].i = ((uint16_t*)task->ptr->o->data)[regs[dual_arg2].i];
                )
                branch
            ALOAD32:   // regs[a] = array[regs[b]]
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = TC_PTR_KIND();)
                require_numeric_object_with_value(
                    regs[dual_arg1].i = ((int32_t*)task->ptr->o->data)[regs[dual_arg2].i];
                )
                branch
            ALOADU32:   // regs[a] = array[regs[b]]
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = TC_PTR_KIND();)
                require_numeric_object_with_value(
                    regs[dual_arg1].i = ((uint32_t*)task->ptr->o->data)[regs[dual_arg2].i];
                )
                branch
            ALOAD64:   // regs[a] = array[regs[b]]
                TC(TC_REG_INT(dual_arg2) TC_RT(dual_arg1) = TC_PTR_KIND();)
                require_numeric_object_with_value(
                    regs[dual_arg1].i = ((int64_t*)task->ptr->o->data)[regs[dual_arg2].i];
                )
                branch
            ASTORE8:   // array[regs[a]] = regs[b]
                TC(TC_REG_INT(dual_arg1) TC_NEED(TC_RT(dual_arg2), TC_PTR_KIND(), "register", dual_arg2))
                require_numeric_object_with_value(
                    ((int8_t*)task->ptr->o->data)[regs[dual_arg1].i] = (int8_t)regs[dual_arg2].i;
                )
                branch
            ASTORE16:   // array[regs[a]] = regs[b]
                TC(TC_REG_INT(dual_arg1) TC_NEED(TC_RT(dual_arg2), TC_PTR_KIND(), "register", dual_arg2))
                require_numeric_object_with_value(
                    ((int16_t*)task->ptr->o->data)[regs[dual_arg1].i] = (int16_t)regs[dual_arg2].i;
                )
                branch
            ASTORE32:   // array[regs[a]] = regs[b]
                TC(TC_REG_INT(dual_arg1) TC_NEED(TC_RT(dual_arg2), TC_PTR_KIND(), "register", dual_arg2))
                require_numeric_object_with_value(
                    ((int32_t*)task->ptr->o->data)[regs[dual_arg1].i] = (int32_t)regs[dual_arg2].i;
                )
                branch
            ASTORE64:   // array[regs[a]] = regs[b]
                TC(TC_REG_INT(dual_arg1) TC_NEED(TC_RT(dual_arg2), TC_PTR_KIND(), "register", dual_arg2))
                require_numeric_object_with_value(
                    ((int64_t*)task->ptr->o->data)[regs[dual_arg1].i] = (int64_t)regs[dual_arg2].i;
                )
                branch
        state_check:
        if(thread->signal) {
            // the garbage collector's safe point (memory/tracing_collector.h)
            if (hasSignal(thread->signal, tsig_gc))
                tgc_safepoint();
            if (hasSignal(thread->signal, tsig_suspend))
                suspend_self();
            if(hasSignal(thread->signal, tsig_except))
                goto catch_exception;
            if (hasSignal(thread->signal, tsig_context_switch)) {
                // (a fiber with C++ on its stack stays on its thread: sni/sni_runtime.h)
                if(thread->nativeCalls == 0 && sni_may_switch_fiber(thread)) {
                    return;
                }
            }
            if (hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED)
                return;
        }

        DISPATCH();
    } catch(vm_exception &e) {
        enable_exception_flag(thread, true);
    }

    catch_exception:
        if(thread->state == THREAD_KILLED) {
            enable_exception_flag(thread, true);
            return;
        }

        if(!catch_exception()) {
            if(return_method())
                return;
            task->pc++;
        }

        goto run;
}

/**
 * Is `thrown` the class `handler`, or derived from it (directly or through
 * several levels), or does it or one of its base classes implement `handler`
 * as an interface?
 *
 * (are_classes_related() in types/sharp_class.cpp is not usable for this: it
 * compares name hashes and answered "related" for unrelated exception classes.)
 */
static bool is_class_or_subclass(sharp_class *thrown, sharp_class *handler) {
    for(sharp_class *c = thrown; c != nullptr; c = c->base) {
        if(c->guid == handler->guid) return true;

        for(Int i = 0; i < c->interfaceCount; i++) {
            if(c->interfaces[i] != nullptr && c->interfaces[i]->guid == handler->guid) return true;
        }
    }
    return false;
}

bool catch_exception() {
    auto thread = thread_self;
    auto task = thread->task;
    Int pc = current_pc;
    sharp_class *handlingClass = vm.classes + CLASS(task->exceptionObject.o->info);

    /**
     * This is the core code for the exception system, Its made to be as lightweight as possible.
     * Majority of the work, such as ensuring that all finally blocks are executed within a given
     * function is handled by the compiler. We just simply need to tell the vm where to jump to.
     *
     * Here we look for a possible caught exception of a given class
     */
    // Try tables are emitted in source order, so of the tables covering this pc the
    // innermost try is the one that starts last: walk them from last to first. For each
    // try its own catch clauses are checked first, then its finally block, so an outer
    // catch is never picked ahead of an inner finally (the finally re-throws when done).
    for(Int i = (Int)task->current->tryCatchTable.size - 1; i >= 0; i--) {
        try_catch_data *table = &task->current->tryCatchTable.node_at(i)->data;
        if(pc >= table->blockStartPc && pc <= table->blockEndPc) {
            for(catch_data &caughtException : table->catchTable) {
                // A handler catches the class it names AND every class derived
                // from it (catch(e: exception) catches an illegal_state_exception).
                // It used to compare for the exact class only, with `exception`,
                // `error` and `throwable` hard wired to catch everything - so a
                // user defined base class caught nothing but itself, and
                // catch(e: error) swallowed exceptions that are not errors.
                // `throwable` stays a catch-all: it is the root of the hierarchy.
                if(
                        caughtException.exception == vm.throwable
                            || is_class_or_subclass(handlingClass, caughtException.exception)
                ) {
                    if(caughtException.exceptionFieldAddress >= 0) {
                        copy_object(&(task->fp + caughtException.exceptionFieldAddress)->obj,
                                    task->exceptionObject.o);
                    }

                    // the exception leaves the try block: give back the locks
                    // of every lock(x) { } block inside it (sharp_object.cpp)
                    release_locks_left_by_exception(task, task->callFramePtr, table->blockStartPc, table->blockEndPc);

                    task->pc = task->rom + caughtException.handlerPc;
                    copy_object(&task->exceptionObject, (sharp_object*) nullptr);
                    enable_exception_flag(thread, false);
                    TELEMETRY_COUNT(tm_exceptions_caught)
                    return true;
                }
            }
        }

        // This try couldn't catch it, so jump to its finally block (if applicable)
        if(table->tryStartPc <= pc && table->tryEndPc >= pc && table->finallyData)
        {
            release_locks_left_by_exception(task, task->callFramePtr, table->tryStartPc, table->tryEndPc);

            task->pc = task->rom + table->finallyData->startPc;
            copy_object(&(task->fp + table->finallyData->exceptionFieldAddress)->obj,
                        task->exceptionObject.o);
            copy_object(&task->exceptionObject, (sharp_object*) nullptr);
            enable_exception_flag(thread, false);
            TELEMETRY_COUNT(tm_exceptions_caught)
            return true;
        }
    }

    // Nothing in this function handles the exception: the function is about to
    // be unwound, and with it every lock block that was entered in it.
    release_locks_left_by_exception(task, task->callFramePtr, 0, INT64_MAX);
    return false;
}

void invoke_delegate(Int address, Int argSize, bool staticCall) {
    if(staticCall) { // native calls are currently unsupported
        if(vm.methods[address].nativeFunc) {
            TELEMETRY_SCOPE(tm_mode_native)
            TELEMETRY_COUNT(tm_native_calls)
            // the arguments out, the C++ function, the result back (sni/sni_runtime.cpp)
            sni_call_native(&vm.methods[address], argSize);

            thread_self->task->pc+= 2; // skip past functions
            return;
        } else {
            throw vm_exception(vm.runtime_except, "attempting to call non-native static delegate function");
        }
    } else {
        auto task = thread_self->task;
        auto classObject = (task->sp - argSize)->obj.o;
        sharp_class *referenceClass = nullptr;

        if(classObject && classObject->type == type_class)
            referenceClass = vm.classes + CLASS(classObject->info);
        else if(classObject == nullptr)
            throw vm_exception(vm.nullptr_except, "attempting to invoke a function call on null object");
        else
            throw vm_exception(vm.runtime_except, "attempting to invoke a function call on a non-class object");

        while(referenceClass != nullptr) {
            for(Int i = referenceClass->methodCount - 1; i >= 0; i--) {
                if(referenceClass->methods[i]->delegateAddress == address) {
                    task->pc++;
                    prepare_method(referenceClass->methods[i]->address);
                    return;
                }
            }

            referenceClass = referenceClass->base;
        }

        throw vm_exception(vm.runtime_except, "delegate function has no subscribers!");
    }
}

void prepare_method(Int address) {
    if(address >= vm.mf.methods || address < 0) {
        stringstream ss;
        ss << "invalid call to method with address of " << address;
        throw vm_exception(ss.str());
    }

    auto task = thread_self->task;
    auto inNative = task->current && (task->current->nativeFunc || task->current->isHighFrequency);
    auto function = vm.methods + address;

    // THE MACHINE STACK. The interpreter keeps every Sharp call in the fiber's
    // own stack, but compiled code calls compiled code as real machine calls
    // (about 32 bytes of the thread's stack each). A recursion deep enough to
    // use that up used to end in a segmentation fault long before the fiber's
    // stack was full; now it ends like any other runaway recursion. `inNative`
    // is the only case that nests, so only it is tested. (Compiled code makes
    // the same test before a direct call and comes here when it fails.)
    if(inNative) {
        char here;
        if((uintptr_t) &here < thread_self->nativeStackFloor)
            throw vm_exception(vm.stack_overflow_except, "");
    }
    
    function->callCount++;
    TELEMETRY_COUNT(tm_calls_total)
    // jfunc != nullptr means we already tried (and possibly failed) to compile:
    // don't take the jit lock on every single call after that.
    if(c_options.jit && function->callCount >= JIT_CALL_THRESHOLD && !function->isHighFrequency
        && function->jfunc == nullptr && function != task->main) {
        jit_try_compile(function);
    }

    if(function->nativeFunc)
        thread_self->nativeCalls++;
//    cout << "call: " << function->fullName << "(" << function->address << ")";
//    if(task->current) {
//        cout << " current: " << task->current->fullName << "(" << task->current->address << ")"
//         << " pc: " << current_pc;
//    }
//    cout << endl;
    grow_stack_for(function->stackSize)
    stack_overflow_check_for(function->stackSize)

    if((task->callFramePtr + 1) >= task->frameSize) {
        task->growFrame();

        // The list of call frames has a ceiling too (frameLimit), and growFrame
        // does nothing once it is reached. Without this test the frame below was
        // written past the end of the list: a function with no parameters and no
        // locals that calls itself (`def f() { f(); }`) uses no stack slots at
        // all, so the stack check above never stops it, and the program died of
        // a segmentation fault instead of a stack_overflow_exception.
        // (Compiled code comes here as well: its fast path leaves to the slow
        // call whenever the list is full, jit/x64/opcode/direct_calls.cpp.)
        if((task->callFramePtr + 1) >= task->frameSize)
            throw vm_exception(vm.stack_overflow_except, "");
    }

    task->calls++;
    if(task->calls > 1) {
        // Record how to get back to the caller (written out here instead of
        // calling init_struct: this runs on every single call).
        frame *callerFrame = task->frames + ++task->callFramePtr;
        callerFrame->returnAddress = task->current->address;
        callerFrame->pc = task->pc;
        callerFrame->sp = (task->sp - function->spOffset) - task->stack;
        callerFrame->fp = task->fp - task->stack;
        callerFrame->native = inNative;
    }

    task->current = function;
    task->fp = task->sp - function->fpOffset;
    TC(stack_item *firstLocal = task->sp + 1;)
    task->sp += function->frameStackOffset;
    // the callee's own locals start out as "not known" for the kind checker
    TC(if(task->sp >= firstLocal) tc_forget_slots(&TC_ST(firstLocal), &TC_ST(task->sp));)
    task->rom = function->bytecode;
    task->pc = function->bytecode;

    #ifdef SAFE_EXECUTION
    if(!check_flag(function->flags, flag_static)) {
        auto instance = task->fp->obj.o;

        if(instance) {
            if(instance->type == type_class) {
                auto instanceClass = &vm.classes[CLASS(instance->info)];
                if(!are_classes_related(instanceClass, function->owner)) {
                    stringstream ss;
                    ss << "invalid call to method " << function->fullName << ": current instance (" << instanceClass->fullName << ") does not match function class (" << function->owner->fullName << ")";
                    throw vm_exception(vm.ill_state_except, ss.str());
                }
            } else {
                stringstream ss;
                ss << "invalid call to method " << function->fullName << ": current instance is not a class type";
                throw vm_exception(vm.ill_state_except, ss.str());
            }
        } else {
            stringstream ss;
            ss << "invalid call to method " << function->fullName << ": current instance is null";
            throw vm_exception(vm.nullptr_except, ss.str());
        }
    }
    #endif

    /*
     * How the callee runs:
     *
     *  - it has compiled code: run that, no matter who the caller is. The compiled
     *    function runs to completion and pops its own VM frame before it comes
     *    back here (or it comes back early because of a context switch/kill, in
     *    which case its frame is still on the VM stack and the caller unwinds).
     *
     *  - no compiled code, and the caller is native or compiled code: the caller
     *    is a C++/machine code frame that expects the call to be finished when
     *    prepare_method returns, so run a nested interpreter loop for it.
     *
     *  - no compiled code and the caller is the interpreter: just return, the
     *    interpreter loop that called us continues at the callee's first
     *    instruction (task->pc was pointed at it above).
     */
    if(task->current->isHighFrequency) {
        TELEMETRY_COUNT(tm_calls_into_jit)
        TELEMETRY_SCOPE(tm_mode_jit)
        int jitResult = task->current->jfunc->compiledCode(thread_self, task->current->jfunc, registers);
        (void)jitResult;
        TELEMETRY(telemetry_jit_exit(jitResult);)
    }
    else if(!task->current->nativeFunc && inNative) {
        TELEMETRY_COUNT(tm_calls_nested_interp)
        main_vm_loop(task->current);
    }
}

/**
 * Makes sure `function` has been handed to the jit compiler (at most once).
 *
 * Returns true if compiled code is installed (function->isHighFrequency), false
 * if the function cannot be compiled (it uses an opcode the jit does not
 * implement yet, it is native, ...). A failed attempt is remembered through
 * function->jfunc != nullptr so we never retry.
 */
bool jit_try_compile(sharp_function *function) {
    if(function->isHighFrequency) return true;
    if(function->jfunc != nullptr || function->nativeFunc || jitCompiler == nullptr) return false;

    guard_mutex(jit_mutex)
    if(function->jfunc == nullptr) {
        function->jfunc = calloc_mem<jit_compiled_function>(1, sizeof(jit_compiled_function));
        TELEMETRY_SCOPE(tm_mode_jit_compile)
        TELEMETRY(unsigned long long compileStart = telemetry_now();)
        if(jitCompiler->compileFunction(function, function->jfunc)) {
            function->isHighFrequency = true; // compiled code is installed and may be called from now on
            TELEMETRY(telemetry_jit_compiled(function, telemetry_now() - compileStart, function->jfunc->codeSize);)
        }
    }

    return function->isHighFrequency;
}

/**
 * "On stack replacement": called by the interpreter at the bottom of a hot loop
 * to continue the function it is currently interpreting in compiled code.
 *
 * This works because compiled code keeps ALL of its state in the fiber (stack,
 * registers, fp/sp, pc) exactly like the interpreter does, and a compiled
 * function can be entered at any instruction: its prologue looks at task->pc
 * and jumps there.
 *
 * Returns
 *   0  compiled code was not entered, keep interpreting
 *   1  compiled code ran and came back, the interpreter must continue at
 *      task->pc after checking the thread state (the function may have
 *      returned to its caller, been interrupted, or thrown)
 *   2  the function finished and its caller is native/jit code, so
 *      main_vm_loop has to return to that caller (same as the RET opcode does
 *      when return_method() returns true)
 */
int jit_enter_loop(Int targetPc) {
    auto thread = thread_self;
    auto task = thread->task;
    auto function = task->current;

    // a fiber's main function is driven by vm_thread_entry and is never compiled
    if(function == task->main || !jit_try_compile(function))
        return 0;

    // return_method() pops frames[callFramePtr]; remember now whether that frame
    // says "the caller is native code", because compiled code does not tell us.
    bool returnsToNative = task->calls <= 1 || task->frames[task->callFramePtr].native;

    task->pc = task->rom + targetPc;

    int jitResult;
    {
        TELEMETRY_COUNT(tm_calls_into_jit)
        TELEMETRY_COUNT(tm_jit_loop_entries)
        TELEMETRY_SCOPE(tm_mode_jit)
        jitResult = function->jfunc->compiledCode(thread, function->jfunc, registers);
        TELEMETRY(telemetry_jit_exit(jitResult);)
    }

    // JIT_OK / JIT_EXCEPTION: the function is over, its frame was popped and
    // task->pc already points after the CALL in the caller.
    if((jitResult == JIT_OK || jitResult == JIT_EXCEPTION) && returnsToNative)
        return 2;
    return 1;
}

/**
 * The next function up the call stack of the fiber that is being continued,
 * or nullptr when the walk is over.
 *
 *      frames[d].returnAddress   the function at call depth d, for d <= callFramePtr
 *      task->current             the function on top (depth callFramePtr + 1)
 *
 * The position is kept in the fiber (rebuildDepth), so it does not matter how
 * often a function appears on the stack. Handing out the top function ends
 * the walk (rebuilding = false): from there on the code simply runs.
 *
 * C++ picture: the call stack as an array walked with an index,
 *      for(depth = 1; depth <= top; depth++) rebuild(function_at(depth));
 * instead of "find the frame that belongs to this function" - which, with
 * f -> f -> f on the stack, finds the first f every time.
 */
sharp_function* next_rebuild_frame() {
    auto task = thread_self->task;
    if(!task->rebuilding) return nullptr;

    Int depth = ++task->rebuildDepth;
    if(depth > task->callFramePtr) {
        task->rebuilding = false;
        return task->current;
    }

    return vm.methods + task->frames[depth].returnAddress;
}

/**
 * Called by JIT compiled code that finds itself in the middle of the stack of
 * a fiber that is being continued (`current` is that function; it is not used
 * to find the place any more).
 */
sharp_function* get_next_frame(sharp_function *current) {
    if(thread_self->task->rebuilding)
        return next_rebuild_frame();

    if(current == nullptr) return nullptr;

    // frames[0..callFramePtr] are the only valid entries (one per caller). The
    // function on top of the stack is task->current and has no entry of its own.
    auto callStack = thread_self->task->frames;
    auto frames = thread_self->task->callFramePtr + 1;

    for(long i = 0; i < frames; i++) {
        if(callStack[i].returnAddress == current->address) {
            sharp_function *fun;

            if((i + 1) >= frames) {
                // invoke current function
                fun = thread_self->task->current;
            } else {
                fun = vm.methods + callStack[i+1].returnAddress;
            }

            // the depth of the function handed out, as next_rebuild_frame()
            // keeps it (continues_in_machine_code() reads it)
            thread_self->task->rebuildDepth = i + 1;
            return fun;
        }
    }

    return nullptr;
}

/**
 * While a fiber is being continued: may `function`, the one the walk just
 * handed out (it is at call depth task->rebuildDepth), be continued in its
 * compiled code?
 *
 * Only if it was running as machine code when it made the call it is waiting
 * in. That is what the frame of that call says: frames[depth].native was
 * set by prepare_method() / a direct call to "the caller is native or
 * compiled code, it expects the callee to come back to it when it returns".
 * Being compiled NOW is not enough. A function the interpreter was running
 * when it made its call can be compiled since - by another thread that
 * called it often enough while this fiber was switched out. Its callee's
 * frame says native = false, so when the callee returns, return_method()
 * does not hand control back to the caller's machine code: the interpreter
 * loop simply carries on with the caller's bytecode. If the walk had entered
 * the caller's machine code, that interpreter loop is a NESTED one, started
 * from inside it (jit_rt_rebuild_above), and it carries on past the caller,
 * through every interpreted function below, possibly to the end of the
 * fiber - while the machine code and the outer interpreter loop that called
 * it still believe they are in the middle. The outer loop then went on at a
 * pc that was no longer valid: past the last instruction of fiber_main.
 *
 *      fiber_main   (interpreted: a fiber's main function is never compiled)
 *        body       (interpreted when it called; compiled since by another thread)
 *          ...      lock(x) taken by someone else: context switch, and the
 *                   fiber is continued later, on another thread
 *
 *      before: the walk entered body's machine code, which interpreted the
 *              rest of body AND of fiber_main in a nested loop - the fiber
 *              ended in there - and then the outer loop dispatched at
 *              fiber_main's pc 36 of 36 (heap-buffer-overflow under the
 *              address sanitizer, "(SIGSEGV) found" now and then without it;
 *              found with several bootstrap compilers running at once)
 *      after:  body is left to the interpreter, as it was when it made the
 *              call; nothing changes for frames that called from machine code
 *
 * The function on top of the stack has no call of its own to come back to,
 * and compiled code can be entered at any instruction: it may always continue
 * in machine code.
 *
 *      C++ comparison: resuming a coroutine must restore each frame in the
 *      form it was suspended in - a frame cannot be suspended as one kind
 *      (interpreted) and resumed as another (native) when its callee will
 *      return to the first kind.
 */
static bool continues_in_machine_code(fiber *task, sharp_function *function) {
    if(!(function->isHighFrequency && function->jfunc != nullptr))
        return false;

    Int depth = task->rebuildDepth;
    if(depth >= 0 && depth <= task->callFramePtr)
        return task->frames[depth].native;

    return true;    // the function on top of the stack
}

void invoke_next_frame(sharp_function *frame, bool isInterpreter) {
    if(frame == nullptr) {
        cout << "frame is empty!!! Something went wrong with rebuilding the frame." << endl;
        return;
    }

    if(continues_in_machine_code(thread_self->task, frame)) {
        TELEMETRY_COUNT(tm_calls_into_jit)
        TELEMETRY_SCOPE(tm_mode_jit)
        int jitResult = frame->jfunc->compiledCode(thread_self, frame->jfunc, registers);
        (void)jitResult;
        TELEMETRY(telemetry_jit_exit(jitResult);)
    } else if(!isInterpreter) {
        try {
            main_vm_loop(frame);
        } catch(vm_exception &e) {
            enable_exception_flag(thread_self, true);
        }
    }
}

bool return_method() {
    auto task = thread_self->task;

    task->calls--;
    if(task->calls == 0) {
        task->finished = true;
        return true;
    }

    if(task->current->nativeFunc)
        thread_self->nativeCalls--;
    auto frame = task->frames + task->callFramePtr--;
//    cout << "return from : " << task->current->fullName << "(" << task->current->address << ")";
//    cout << " to: " << (vm.methods + frame->returnAddress)->fullName
//         << "(" << (vm.methods + frame->returnAddress)->address << ")"
//         << " pc: " << current_pc;
    task->current = vm.methods + frame->returnAddress;
    task->rom = task->current->bytecode;
    task->pc = frame->pc;
    task->sp = task->stack + frame->sp;
    task->fp = task->stack + frame->fp;

    if(frame->native)
        return true;
//    cout << " pc now: " << current_pc;
//    cout << endl;
    return false;
}


