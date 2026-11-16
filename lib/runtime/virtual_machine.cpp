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
                cout << regs[single_arg].d;
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


void exec_interrupt(Int interrupt)
{
    auto task = thread_self->task;
    switch(interrupt)
    {
        case OP_NOP:
            return;
        case OP_PRINTF:
            dbl_printf((int)registers[EGX].i, registers[EBX].d, (int)registers[ECX].i);   // the number printed is a decimal
            return;
        case OP_STRTOD: {
            auto strObject = pop_stack_object.o;
            if (strObject != NULL && strObject->type <= type_var && strObject->data != NULL) {
                string num_str;
                populate_string(num_str, strObject);
                registers[EBX].d = strtod(num_str.c_str(), NULL);
            } else
                throw vm_exception(vm.nullptr_except, "");
            return;
        }
        case OP_OS_TIME:
            registers[BMR].i = Clock::__os_time((int)registers[EBX].i);
            return;
        case OP_GC_EXPLICIT:
            /**
             * Make an explicit call to the garbage collector. This
             * does not garuntee that it will run
             */
            tgc_request(true); // runs at the next safe point, right after this instruction
            return;
        case OP_GC_SLEEP:
            sedate_gc();
            break;
        case OP_GC_WAKEUP:
            wake_gc();
            break;
        case OP_GC_KILL:
            kill_gc();
            break;
        case OP_GC_STATE:
            registers[CMT].i = gc.state;
            break;
        case OP_NAN_TIME:
            registers[BMR].i = Clock::realTimeInNSecs();
            return;
        case OP_DELAY:
            delay_task(registers[EBX].i);
            return;
        case OP_FIBER_START: {
            auto *name = pop_stack_object.o;
            // the function the fiber runs is a function value (an object);
            // only its index is needed, a fiber's main captures nothing
            Int mainFunc = function_value_address(pop_stack_object.o);
            Int threadid = pop_stack_number.i;
            auto fibObj = pop_stack_object.o;
            auto fibArgs = pop_stack_object.o;

            if(name != NULL && name->type <= type_var) {
                string fiberName;
                populate_string(fiberName, name);
                fiber *fib;
                sharp_thread *binder = nullptr;

                if(threadid != -1) {
                    if(threadid != gc_threadid && threadid != idle_threadid) {
                        binder = get_thread(threadid);
                    } else {
                        registers[EBX].i = -1;
                        return;
                    }
                }

                // create_task() hands the fiber to the scheduler. From that
                // moment another thread may run it to its end and free it, so
                // `fib` must not be used after the scheduler lock is let go:
                // the id is read while the lock still keeps the fiber alive
                // (freeing a fiber, sched_reap, needs the same lock). Reading
                // fib->id afterwards was a use after free - the rare crash in
                // start_fiber.
                Int fiberId;
                try {
                    std::lock_guard<recursive_mutex> keepFiberAlive(task_mutex);
                    fib = create_task(fiberName, &vm.methods[mainFunc % vm.mf.methods], fibObj, fibArgs, binder);
                    fiberId = fib->id;
                } catch(vm_exception &e) {
                    fiberName.clear();
                    throw;
                }

                registers[EBX].i = fiberId;
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }
            return;
        }
        case OP_FIBER_SUSPEND: {
            Int fiberId = pop_stack_number.i;
            registers[EBX].i = suspend_task(fiberId);
            return;
        }
        case OP_FIBER_UNSUSPEND: {
            Int fiberId = pop_stack_number.i;
            registers[EBX].i = unsuspend_task(fiberId);
            return;
        }
        case OP_FIBER_KILL: {
            Int fiberId = pop_stack_number.i;
            registers[EBX].i = kill_task(fiberId);
            return;
        }
        case OP_FIBER_BIND: {
            Int fiberId = pop_stack_number.i;
            Int threadId = pop_stack_number.i;
            auto bindTo = get_thread(threadId); // before task_mutex: thread_mutex must never be taken inside it

            guard_mutex(task_mutex) // the fiber cannot be freed while we use it
            fiber *fib = locate_task(fiberId);
            registers[EBX].i = 2;

            if(fib) {
                registers[EBX].i = bind_task(fib, bindTo);
            }
            return;
        }
        case OP_FIBER_BOUND_COUNT: {
            Int threadId = pop_stack_number.i;
            auto thread = get_thread(threadId);
            registers[EBX].i = thread ? thread->boundFibers.load() : 0;
            return;
        }
        case OP_FIBER_STATE: {
            Int fiberId = pop_stack_number.i;
            guard_mutex(task_mutex) // the fiber cannot be freed while we use it
            fiber *fib = locate_task(fiberId);
            registers[EBX].i = fib ? get_state(fib) : -1;
            return;
        }
        case OP_FIBER_CURRENT: {
            grow_stack
            stack_overflow_check
            copy_object(&push_stack_object, task->fiberObject.o);
            return;
        }
        case OP_THREAD_START: {
            auto thread = get_thread((int32_t )registers[ADX].i);

            if(thread != NULL) {
                copy_object(&thread->currentThread, pop_stack_object.o);
                copy_object(&thread->args, pop_stack_object.o);

                auto priorityEnum = resolve_field("priority", thread->currentThread.o);
                if(priorityEnum != NULL && priorityEnum->o != NULL) {
                    thread->priority = (int) number_field(priorityEnum->o, "ordinal");
                }

                thread->stackSize = (int) number_field(thread->currentThread.o, "stack_size");

                auto name = resolve_field("name", thread->currentThread.o);
                if(name != NULL) {
                    auto data = resolve_field("data", name->o);

                    if(data) {
                        string threadName = read_string_value(data->o);
                        if(threadName != "") {
                            thread->name = threadName;
                        }
                    }
                }
            }
            registers[CMT].i=start_thread((int32_t )registers[ADX].i, (size_t )registers[EBX].i);
            return;
        }
        case OP_STRTOL: {
            Int baseNum = pop_stack_number.i;
            auto str = pop_stack_object.o;
            if(str != NULL && str->type <= type_var) {
                string numStr;
                populate_string(numStr, str);
                registers[EBX].i = strtoll(numStr.c_str(), NULL, baseNum);
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }
            return;
        }
        case OP_THREAD_JOIN:
            registers[CMT].i=join_thread((int32_t )registers[ADX].i);
            return;
        case OP_THREAD_INTERRUPT:
            registers[CMT].i=interrupt_thread((int32_t )registers[ADX].i);
            return;
        case OP_THREAD_CREATE:
            registers[EBX].i=create_thread((int32_t )registers[ADX].i, (bool)registers[EBX].i);
            return;
        case OP_THREAD_PRIORITY:
            registers[CMT].i=set_thread_priority((int32_t )registers[ADX].i, (int)registers[EGX].i);
            return;
        case OP_THREAD_SUSPEND:
            registers[CMT].i=suspend_thread((int32_t )registers[ADX].i);
            return;
        case OP_THREAD_UNSUSPEND:
            registers[CMT].i=unsuspend_thread((int32_t )registers[ADX].i, (int32_t )registers[EBX].i);
            return;
        case OP_THREAD_CURRENT: // native getCurrentThread()
            grow_stack
            stack_overflow_check
            copy_object(&push_stack_object, thread_self->currentThread.o);
            return;
        case OP_THREAD_ARGS: // native getCurrentThreadArgs()
            grow_stack
            stack_overflow_check
            copy_object(&push_stack_object, thread_self->args.o);
            return;
        case OP_THREAD_SET_CURRENT: // native setCurrentThread(Thread)
            copy_object(&thread_self->currentThread, pop_stack_object.o);
            return;
        case OP_MATH:
            registers[CMT].d=__cmath(registers[EBX].d, registers[EGX].d, (int)registers[ECX].i);
            return;
        case OP_RANDOM_INT:
            registers[EBX].i = randInt();
            return;
        case OP_RANDOM_DOUBLE:
            registers[EBX].d = randDouble();
            return;
        case OP_RANDOM_INT_RANGED:
            registers[EBX].i = randInt((Int)registers[EBX].i, (Int)registers[EGX].i);
            return;
        case OP_RANDOM_DOUBLE_RANGED:
            registers[EBX].d = randDouble(registers[EBX].d, registers[EGX].d);
            return;
        case OP_RANDOM_SEED:
            setSeed(registers[ADX].i);
            return;
        case OP_SYSTEM_EXE: {
            auto str = pop_stack_object.o;
            if(str != NULL && str->type <= type_var) {
                string cmd;
                populate_string(cmd, str);
                gc_safe_region waitingForCommand;
                registers[CMT].i = system(cmd.c_str());
            } else
                throw vm_exception(vm.nullptr_except, "");
            return;
        }
        case OP_KBHIT:
            registers[CMT].i = _kbhit();
            return;
        case OP_YIELD:
            std::this_thread::yield();
            return;
        case OP_EXIT:
#ifdef SHARP_PROF_
            thread_self->tprof->endtm=Clock::realTimeInNSecs();
            thread_self->tprof->profile();
            thread_self->tprof->dump();

#endif
            task->exitVal = (int)pop_stack_number.i;
            shutdown();
            return;
        case OP_MEMORY_LIMIT:
            registers[CMT].i = gc.memoryLimit;
            return;
        case OP_MEMORY:
            registers[CMT].i = gc.managedBytes;
            return;
        case OP_ABS_PATH: {
            auto *relPath = &task->sp->obj;

            if(relPath->o != NULL && relPath->o->type <= type_var) {
                string path, absolute;
                populate_string(path, relPath->o);

                File::resolvePath(path, absolute);
                auto strObject = create_object(absolute.size(), type_int8);

                copy_object(relPath, strObject);
                if(strObject) assign_string_field(strObject, absolute);
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }
            return;
        }
        case OP_MEM_COPY: {
            memcopy();
            return;
        }
        case OP_INVERT: {
            invert();
            return;
        }
        case OP_REALLOC: {
            size_t len = pop_stack_number.i;
            auto array = task->sp->obj.o;

            if(array != NULL) {
                realloc_object(array, len);
            } else
                throw vm_exception(vm.nullptr_except, "");
            return;
        }
        case OP_OS_NAME: {
            // `build.os` of the library. It is ASKED FOR, not only handed to the program's start function,
            // because classes decide things by it while their static parts are set up (the file class picks
            // its file system) - and that runs before the start function has stored anything.
#ifdef WIN32_
            string name = "win";
#else
            string name = "posix";
#endif
            auto strObject = create_object(name.size(), type_int8);

            copy_object(&push_stack_object, strObject);
            if(strObject) assign_string_field(strObject, name);
            return;
        }
        case OP_PROGRAM_PATH: {
            // program_folder() of the library: a program finds what is installed beside it
            // (its native libraries: load_library(program_folder() + "/foo")) wherever it is started from
            string absolute;
            File::resolvePath(executable, absolute);
            auto strObject = create_object(absolute.size(), type_int8);

            copy_object(&push_stack_object, strObject);
            if(strObject) assign_string_field(strObject, absolute);
            return;
        }
        case OP_CURRENT_DIRECTORY: {
            string path;
            current_directory(path);
            auto strObject = create_object(path.size(), type_int8);

            copy_object(&push_stack_object, strObject);
            assign_string_field(strObject, path);
            return;
        }
        case OP_FILE_ACCESS:
        case OP_FILE_ATTRS:
        case OP_FILE_UPDATE_TM:
        case OP_FILE_SIZE:
        case OP_FILE_CREATE:
        case OP_FILE_DELETE:
        case OP_GET_FILES:
        case OP_CREATE_DIR:
        case OP_DELETE_DIR:
        case OP_UPDATE_FILE_TM:
        case OP_CHMOD:
        case OP_READ_FILE: {
            auto arry = &pop_stack_object;
            if(arry->o != NULL && arry->o->size > 0 && arry->o->type <= type_var) {
                string path;
                populate_string(path, arry->o);

                if(interrupt==OP_FILE_ACCESS)
                    registers[EBX].i = check_access(path, (int)registers[EBX].i);
                else if(interrupt==OP_FILE_ATTRS)
                    registers[EBX].i = get_file_attrs(path);
                else if(interrupt==OP_FILE_UPDATE_TM)
                    registers[EBX].i = last_update(path, (int)registers[EBX].i);
                else if(interrupt==OP_FILE_SIZE)
                    registers[EBX].i = file_size(path);
                else if(interrupt==OP_FILE_CREATE)
                    create_file(path);
                else if(interrupt==OP_FILE_DELETE)
                    registers[EBX].i = delete_file(path);
                else if(interrupt==OP_GET_FILES) {
                    std::list<string> files;
                    get_file_list(path, files);
                    arry = &push_stack_object;

                    if(files.size()>0) {
                        copy_object(arry, create_object(vm.string_class, (Int)files.size()));

                        Int iter = 0;
                        for(string fileName : files) {
                            copy_object(arry->o->node + (iter), create_object(vm.string_class));
                            auto dataField = resolve_field("data", arry->o->node[iter].o);
                            auto strObject = create_object(fileName.size(), type_int8);

                            copy_object(dataField, strObject);
                            assign_string_field(strObject, fileName);
                            iter++;
                        }
                    } else {
                        copy_object(arry, (sharp_object*) nullptr);
                    }
                }
                else if(interrupt==OP_CREATE_DIR)
                    registers[EBX].i = File::makeDir(path);
                else if(interrupt==OP_DELETE_DIR)
                    registers[EBX].i = delete_dir(path);
                else if(interrupt==OP_UPDATE_FILE_TM)
                    registers[EBX].i = update_time(path, (time_t)registers[EBX].i);
                else if(interrupt==OP_CHMOD)
                    registers[EBX].i = __chmod(path, (mode_t)registers[EBX].i, (bool)registers[EGX].i, (bool)registers[ECX].i);
                else if(interrupt==OP_READ_FILE) {
                    arry = &push_stack_object;
                    string str;
                    read_file(path, str);

                    if(str.size() > 0) {
                        copy_object(arry, create_object(vm.string_class));
                        auto dataField = resolve_field("data", arry->o);
                        auto strObject = create_object(str.size(), type_int8);

                        copy_object(dataField, strObject);
                        assign_string_field(strObject, str);
                    } else {
                        copy_object(arry, (sharp_object*) nullptr);
                    }
                }
            } else
                throw vm_exception(vm.nullptr_except, "");
            return;
        }
        case OP_RENAME_FILE:
        case OP_WRITE_FILE:  {
            auto pathObj = pop_stack_object.o;
            auto newNameObj = pop_stack_object.o;

            if (pathObj != NULL && pathObj->type <= type_var && newNameObj != NULL
                && newNameObj->type <= type_var) {
                string path, rename;

                populate_string(path, pathObj);
                populate_string(rename, newNameObj);

                if(interrupt==OP_RENAME_FILE)
                    registers[EBX].i = rename_file(path, rename);
                else if(interrupt==OP_WRITE_FILE) {
                    File::buffer buf;
                    buf.operator<<(rename); // rename will contain our actual unicode data
                    registers[EBX].i = File::write(path.c_str(), buf);
                    buf.end();
                }
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }

            return;
        }
        case OP_LOAD_LIBRARY: {
            // load_library("path"): the program loads a native library (sni/sni_runtime.cpp)
            sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;
            if (libNameObj == NULL || libNameObj->type > type_var)
                throw vm_exception(vm.nullptr_except, "");
            string name;
            populate_string(name, libNameObj);
            sni_load_library(name);
            return;
        }
        case OP_FREE_LIBRARY: {
            sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;

            if (libNameObj != NULL && libNameObj->type <= type_var) {
                string name;
                populate_string(name, libNameObj);
                registers[EBX].i = sni_free_library(name);
            }
            return;
        }
        case OP_LINK_FUNC: {
            sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;
            sharp_object *funcNameObj = (thread_self->task->sp--)->obj.o;

            if (libNameObj != NULL && libNameObj->type <= type_var
                && funcNameObj != NULL && funcNameObj->type <= type_var) {
                string libname, funcname;

                populate_string(libname, libNameObj);
                populate_string(funcname, funcNameObj);
                registers[EBX].i = sni_link_from(libname, locate_function(funcname.c_str()));
            }
            return;
        }
        case OP_DISK_SPACE:
            registers[EBX].i=disk_space((int32_t )registers[EBX].i);
            return;
        case OP_SIZEOF:
            registers[EBX].i = sizeof_object(pop_stack_object.o);
            return;
        case OP_FLUSH:
            cout << std::flush;
            break;
        case OP_GET_FRAME_INFO: {
            grow_stack
            stack_overflow_check
            auto info = get_frame_info(task);
            auto stackState = create_object(vm.stack_sate);
            auto methodsField = resolve_field("methods", stackState);
            auto pcField = resolve_field("pc", stackState);
            Int iter = 0;

            copy_object(methodsField, create_object(info.size(), type_int64));
            copy_object(pcField, create_object(info.size(), type_int64));
            for(KeyPair<Int, Int> &frameInfo : info) {
                set_integer_at(methodsField->o, iter, frameInfo.key);
                set_integer_at(pcField->o, iter++, frameInfo.value);
            }

            copy_object(&push_stack_object, stackState);
            break;
        }
        case OP_GET_STACK_TRACE: {
            fill_stack_trace_from_frame_info();
            break;
        }
        case OP_COPY: {
            copy();
            return;
        }
        case OP_CORES: {
            registers[EBX].i=std::thread::hardware_concurrency();
            return;
        }
        case OP_SERIALIZE: {
            serialize(&task->sp->obj, &task->sp->obj);
            return;
        }
        case OP_DESERIALIZE: {
            deserialize(&task->sp->obj, &(task->sp+1)->obj);
            copy_object(&task->sp->obj, &(task->sp+1)->obj);
            return;
        }
        case OP_DESCRIBE_OBJECT: {
            auto object = task->sp->obj.o;
            copy_object(&task->sp->obj, describe_object(object));
            return;
        }
        case OP_REFLECT_FIELD_VALUE: {
            auto instance = (task->sp--)->obj.o;
            auto index = (Int)(task->sp)->var.i;

            if(instance && IS_CLASS(instance) && index < vm.classes[CLASS(instance->info)].instanceFields) {
                // what the slot holds is decided by the field's declaration
                sharp_field &declared = vm.classes[CLASS(instance->info)].fields[index];
                bool isNumber = declared.type->type <= type_var && !declared.isArray;
                auto field = isNumber ? nullptr : instance->node[index].o;
                if(isNumber || field) {
                    if (!isNumber) {
                        copy_object(&task->sp->obj, field);
                    } else {
                        // a number field: hand out a number object (int, double ...) holding it
                        string field_name = "value";
                        vm_cell cell = number_slot(instance, index);
                        double value = declared.type->type == type_var ? cell.d : (double)cell.i;

                        switch(declared.type->type) {
                            case type_int8: {
                                // char
                                auto obj = create_object(vm.char_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (char)value);
                                break;
                            }
                            case type_int16: {
                                // char
                                auto obj = create_object(vm.short_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (short)value);
                                break;
                            }
                            case type_int32: {
                                // char
                                auto obj = create_object(vm.int_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (int)value);
                                break;
                            }
                            case type_int64: {
                                // char
                                auto obj = create_object(vm.long_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (int64_t)value);
                                break;
                            }
                            case type_uint8: {
                                // char
                                auto obj = create_object(vm.uchar_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (unsigned char)value);
                                break;
                            }
                            case type_uint16: {
                                // char
                                auto obj = create_object(vm.ushort_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (unsigned short)value);
                                break;
                            }
                            case type_uint32: {
                                // char
                                auto obj = create_object(vm.uint_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (unsigned int)value);
                                break;
                            }
                            case type_uint64: {
                                // char
                                auto obj = create_object(vm.ulong_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (uint64_t)value);
                                break;
                            }
                            case type_function_ptr: {
                                // char
                                auto obj = create_object(vm.long_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (uint64_t)value);
                                break;
                            }
                            case type_var: {
                                // char
                                auto obj = create_object(vm.double_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, value);
                                break;
                            }
                        }
                    }
                } else {
                    copy_object(&task->sp->obj, (sharp_object*)nullptr);
                }
            } else {
                copy_object(&task->sp->obj, (sharp_object*)nullptr);
            }
            break;
        }
        default: {
            stringstream ss;
            ss << "invalid system interrupt signal: " << interrupt;
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }
    }
}
