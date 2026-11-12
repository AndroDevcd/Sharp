//
// Created by bknun on 9/18/2022.
//

#include "virtual_machine.h"
#include "error/vm_exception.h"
#include "../core/opcode/opcode_macros.h"
#include "../core/thread_state.h"
#include "multitasking/thread/sharp_thread.h"
#include "multitasking/fiber/fiber.h"
#include "multitasking/thread/thread_controller.h"
#include "reflect/reflect_helpers.h"
#include "memory/garbage_collector.h"
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
thread_local long double *registers;

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

void main_vm_loop(sharp_function *frame)
{
    setupOpcodeTable
    TELEMETRY_SCOPE(tm_mode_interpreter) // everything in this loop is interpreter time unless a nested scope says otherwise
    TELEMETRY_OPCODE_SETUP
    auto thread = thread_self;
    auto task = thread->task;
    long double *regs = registers;
    Int data;

    // Continuing a fiber after a context switch: walk up its call stack and
    // give every JIT compiled function its machine frame back. (By call depth,
    // not by function - see next_rebuild_frame().)
    while(thread_self->task->rebuilding) {
        frame = next_rebuild_frame();
        if(frame == nullptr) break;

        if(frame->isHighFrequency && frame->jfunc != nullptr) {
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
                {
                    TELEMETRY_SCOPE(tm_mode_interrupt)
                    TELEMETRY_COUNT(tm_interrupts)
                    exec_interrupt(single_arg);
                }
                if(vm.state == VM_TERMINATED) {
                    send_interrupt_signal(thread);
                    return;
                }
                check_state(1)
            _MOVI:
                regs[single_arg] = raw_arg2;
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
                grow_stack
                stack_overflow_check
                copy_object(
                    &push_stack_object,
                    create_object(regs[dual_arg1], (data_type)dual_arg2)
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
            // wraps around (like an integer cast in C or Java). Converting the long double
            // straight to a small type is undefined for out of range values (x87 gives
            // INT_MIN) and would not match what the JIT does (emit_mov8.. in jit/x64/opcode).
            MOV8:
                regs[dual_arg1] = (int8_t)(int64_t)regs[dual_arg2];
                branch
            MOV16:
                regs[dual_arg1] = (int16_t)(int64_t)regs[dual_arg2];
                branch
            MOV32:
                regs[dual_arg1] = (int32_t)(int64_t)regs[dual_arg2];
                branch
            MOV64:
                regs[dual_arg1] = (int64_t)regs[dual_arg2];
                branch
            MOVU8:
                regs[dual_arg1] = (uint8_t)(int64_t)regs[dual_arg2];
                branch
            MOVU16:
                regs[dual_arg1] = (uint16_t)(int64_t)regs[dual_arg2];
                branch
            MOVU32:
                regs[dual_arg1] = (uint32_t)(int64_t)regs[dual_arg2];
                branch
            MOVU64:
                regs[dual_arg1] = (uint64_t)regs[dual_arg2];
                branch
            RSTORE:
                grow_stack
                stack_overflow_check
                push_stack_number = regs[single_arg];
                branch
            ADD:
                regs[triple_arg3] = regs[triple_arg1] + regs[triple_arg2];
                branch
            SUB:
                regs[triple_arg3] = regs[triple_arg1] - regs[triple_arg2];
                branch
            MUL:
                regs[triple_arg3] = regs[triple_arg1] * regs[triple_arg2];
                branch
            DIV:
                regs[triple_arg3] = regs[triple_arg1] / regs[triple_arg2];
                branch
            MOD:
                regs[triple_arg3] = vm_remainder((Int)regs[triple_arg1], (Int)regs[triple_arg2]);
                branch
            IADD:
                regs[single_arg] += raw_arg2;
                branch_for(2)
            ISUB:
                regs[single_arg] -= raw_arg2;
                branch_for(2)
            IMUL:
                regs[single_arg] *= raw_arg2;
                branch_for(2)
            IDIV:
                regs[single_arg] /= raw_arg2;
                branch_for(2)
            IMOD:
                regs[single_arg] = vm_remainder((Int)regs[single_arg], (Int)raw_arg2);
                branch_for(2)
            POP:
                copy_object(&pop_stack_object, (sharp_object*)nullptr);
                branch
            INC:
                regs[single_arg]++;
                branch
            DEC:
                regs[single_arg]--;
                branch
            MOVR:
                regs[dual_arg1] = regs[dual_arg2];
                branch
            BRH:
                task->pc = task->rom + (Int)regs[ADX];
                check_state(0)
            IFE:
                if(regs[CMT]) {
                    task->pc = task->rom + (Int)regs[ADX];
                    check_state(0)
                } else check_state(1)
            IFNE:
                if(regs[CMT] == 0) {
                    task->pc = task->rom + (Int)regs[ADX];
                    check_state(0)
                } else check_state(1)
            LT:
                regs[CMT] = regs[dual_arg1] < regs[dual_arg2];
                branch
            GT:
                regs[CMT] = regs[dual_arg1] > regs[dual_arg2];
                branch
            LTE:
                regs[CMT] = regs[dual_arg1] <= regs[dual_arg2];
                branch
            GTE:
                regs[CMT] = regs[dual_arg1] >= regs[dual_arg2];
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
                (task->fp+single_arg)->var = pop_stack_number;
                branch
            MOVSL:
                task->ptr = &((task->sp+single_arg)->obj);
                branch
            SIZEOF:
                regs[single_arg]= ( task->ptr!=NULL && task->ptr->o != NULL) ? task->ptr->o->size : 0;
                branch
            PUT:
                cout << regs[single_arg];
                branch
            PUTC:
                printf("%c", (char)regs[single_arg]);
                branch
            GET:
                regs[single_arg] = (regs[CMT] != 0) ? getche() : getch();
                branch
            CHECKLEN:
                require_object_with_value(
                        if(regs[single_arg] < task->ptr->o->size && regs[single_arg] >= 0) {
                            branch
                        }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << regs[single_arg] << " size is " << task->ptr->o->size;
                            throw vm_exception(vm.bounds_except, ss.str());
                        }
                )
            JMP:
                loop_back_edge(single_arg)
                task->pc = task->rom + single_arg;
                check_state(0)
            LOADPC:
                regs[single_arg] = current_pc;
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
            CALLD:
                if((data = (Int)regs[single_arg]) <= 0 || data >= vm.mf.methods) {
                    stringstream ss;
                    ss << "invalid call to method (@" << data << ") out of range!";
                    throw vm_exception(ss.str());
                }

                prepare_method(data);
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
                {
                    TELEMETRY_SCOPE(tm_mode_sleep)
                    __os_sleep(regs[single_arg]);
                }
                branch
            TEST:
                regs[BMR] = regs[dual_arg1] == regs[dual_arg2];
                branch
            TNE:
                regs[BMR] = regs[dual_arg1] != regs[dual_arg2];
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
                require_object_with_value(
                    #ifdef SAFE_EXECUTION
                     if(task->ptr->o->type <= type_var) {
                         vm_exception err(vm.ill_state_except, "movn on number");
                         enable_exception_flag(thread, true);
                         goto catch_exception;
                     }
                    #endif

                    task->ptr = task->ptr->o->node + (Int)regs[single_arg];
                )
                branch
            NEWOBJARRAY:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, create_object(regs[single_arg]));
                check_state(1)
            NOT:
                regs[dual_arg1] = !regs[dual_arg2];
                branch
            SKIP: // todo: phase out
                task->pc += single_arg;
                branch
            LOADVAL:
                regs[single_arg] = pop_stack_number;
                branch
            SHL:
                regs[triple_arg3] = (Int)regs[triple_arg1] << (Int)regs[triple_arg2];
                branch
            SHR:
                regs[triple_arg3] = (Int)regs[triple_arg1] >> (Int)regs[triple_arg2];
                branch
            SKPE: // todo: phase out
                if((Int)regs[dual_arg1] != 0) {
                    task->pc = task->pc + dual_arg2;
                    check_state(0)
                } else check_state(1)
            SKNE: // todo: phase out
                if((Int)regs[dual_arg1] == 0) {
                    task->pc = task->pc + dual_arg2;
                    check_state(0)
                } else check_state(1)
            CMP:
                regs[CMT] = regs[single_arg] == raw_arg2;
                branch_for(2)
            AND:
                regs[BMR] = regs[dual_arg1] && regs[dual_arg2];
                branch
            UAND:
                regs[BMR] = (Int)regs[dual_arg1] & (Int)regs[dual_arg2];
                branch
            OR:
                regs[BMR] = (Int)regs[dual_arg1] | (Int)regs[dual_arg2];
                branch
            XOR:
                regs[BMR] = (Int)regs[dual_arg1] ^ (Int)regs[dual_arg2];
                branch
            THROW:
                copy_object(&task->exceptionObject, pop_stack_object.o);
                enable_exception_flag(thread, true);
                check_state(0)
            CHECKNULL:
                regs[single_arg] = task->ptr == nullptr || task->ptr->o == nullptr;
                branch
            RETURNOBJ:
                copy_object(&task->fp->obj, task->ptr);
                branch
            NEWCLASSARRAY:
                grow_stack
                stack_overflow_check
                copy_object(
                        &push_stack_object,
                        create_object(vm.classes + raw_arg2, (Int)regs[single_arg])
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
                (task->fp + dual_arg2)->var += regs[dual_arg1];
                branch
            SUBL:
                (task->fp + dual_arg2)->var -= regs[dual_arg1];
                branch
            MULL:
                (task->fp + dual_arg2)->var *= regs[dual_arg1];
                branch
            DIVL:
                (task->fp + dual_arg2)->var /= regs[dual_arg1];
                branch
            MODL:
                (task->fp + dual_arg2)->var = vm_remainder((Int)(task->fp + dual_arg2)->var, (Int)regs[dual_arg1]);
                branch
            IADDL:
                (task->fp + single_arg)->var += raw_arg2;
                branch_for(2)
            ISUBL:
                (task->fp + single_arg)->var -= raw_arg2;
                branch_for(2)
            IMULL:
                (task->fp + single_arg)->var *= raw_arg2;
                branch_for(2)
            IDIVL:
                (task->fp + single_arg)->var /= raw_arg2;
                branch_for(2)
            IMODL:
                (task->fp + single_arg)->var = vm_remainder((Int)(task->fp + single_arg)->var, (Int)raw_arg2);
                branch_for(2)
            LOADL:
                regs[dual_arg1] = (task->fp + dual_arg2)->var;
                branch
            IALOAD:
                require_numeric_object_with_value(
                    #ifdef SAFE_EXECUTION
                    if(task->ptr->o->type > type_var) {
                        vm_exception err(vm.ill_state_except, "numeric access on data structure");
                        enable_exception_flag(thread, true);
                        goto catch_exception;
                    }
                    #endif

                    regs[dual_arg1] = task->ptr->o->HEAD[(Int)regs[dual_arg2]];
                )
                branch
            ILOAD:
                require_numeric_object_with_value(
                    #ifdef SAFE_EXECUTION
                    if(task->ptr->o->type > type_var) {
                        vm_exception err(vm.ill_state_except, "numeric access on data structure");
                        enable_exception_flag(thread, true);
                        goto catch_exception;
                    }
                    #endif

                    regs[single_arg] = task->ptr->o->HEAD[0];
                )
                branch
            POPOBJ:
                require_object(
                    copy_object(task->ptr, pop_stack_object.o);
                )
                branch
            SMOVR:
                (task->sp + dual_arg2)->var = regs[dual_arg1];
                branch
            SMOVR_2:
                (task->fp + dual_arg2)->var = regs[dual_arg1];
                branch
            SMOVR_3:
                copy_object(&(task->fp + single_arg)->obj, task->ptr);
                branch
            SMOVR_4:
                (task->fp + single_arg)->var = (task->fp + raw_arg2)->var;
                branch_for(2)
            ANDL:
                (task->fp + dual_arg2)->var = (Int)(task->fp + dual_arg2)->var & (Int)regs[dual_arg1];
                branch
            ORL:
                (task->fp + dual_arg2)->var = (Int)(task->fp + dual_arg2)->var | (Int)regs[dual_arg1];
                branch
            XORL:
                (task->fp + dual_arg2)->var = (Int)(task->fp + dual_arg2)->var ^ (Int)regs[dual_arg1];
                branch
            RMOV:
                require_numeric_object_with_value(
                    #ifdef SAFE_EXECUTION
                    if(task->ptr->o->type > type_var) {
                        vm_exception err(vm.ill_state_except, "numeric access on data structure");
                        enable_exception_flag(thread, true);
                        goto catch_exception;
                    }
                    #endif

                    task->ptr->o->HEAD[(Int)regs[dual_arg1]] =
                            regs[dual_arg2];
                )
                branch
            IMOV:
                require_numeric_object_with_value(
                    #ifdef SAFE_EXECUTION
                    if(task->ptr->o->type > type_var) {
                        vm_exception err(vm.ill_state_except, "numeric access on data structure");
                        enable_exception_flag(thread, true);
                        goto catch_exception;
                    }
                    #endif

                    task->ptr->o->HEAD[0] =
                            regs[single_arg];
                )
                branch
            NEG:
                regs[dual_arg1] = -regs[dual_arg2];
                branch
            SMOV:
                regs[dual_arg1] = (task->sp + dual_arg2)->var;
                branch
            RETURNVAL:
                task->fp->var = regs[single_arg];
                branch
            ISTORE:
                grow_stack
                stack_overflow_check
                push_stack_number = raw_arg2;
                branch_for(2)
            ISTOREL:
                (task->fp + single_arg)->var = raw_arg2;
                branch_for(2)
            PUSHNULL:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, (sharp_object*) nullptr);
                branch
            IPUSHL:
                grow_stack
                stack_overflow_check
                push_stack_number = (task->fp + single_arg)->var;
                branch
            PUSHL:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, (task->fp + single_arg)->obj.o);
                branch
            ITEST:
                data = (Int)pop_stack_object.o;
                regs[single_arg] = (sharp_object*)data == pop_stack_object.o;
                branch
            INVOKE_DELEGATE:
                invoke_delegate(single_arg, dual_raw_arg2, dual_raw_arg1);
                check_state(0)
            ISADD:
                (task->sp + single_arg)->var += raw_arg2;
                branch_for(2)
            JE:
                if(regs[CMT]) {
                    loop_back_edge(single_arg)   // the optimizer puts the loop test at the bottom of the loop
                    task->pc = task->rom + single_arg;
                    check_state(0)
                } else check_state(1)
            JNE:
                if(regs[CMT] == 0) {
                    loop_back_edge(single_arg)
                    task->pc = task->rom + single_arg;
                    check_state(0)
                } else check_state(1)
            TLS_MOVL:
                task->ptr = &(task->stack + single_arg)->obj;
                branch
            MOV_ABS:
                (task->stack + single_arg)->var = regs[EBX];
                branch
            LOAD_ABS:
                regs[EBX] = (task->stack + single_arg)->var;
                branch
            DUP:
                data = (Int)&task->sp->obj;
                copy_object(&push_stack_object, (object*)data);
                branch
            POPOBJ_2:
                task->ptr = &pop_stack_object;
                branch
            SWAP:
                if((task->sp-task->stack) >= 2) {
                    auto swappedObj = task->sp->obj.o;
                    (task->sp)->obj.o = (task->sp-1)->obj.o;
                    (task->sp-1)->obj.o = swappedObj;
                    double swappedVar = task->sp->var;
                    (task->sp)->var = (task->sp-1)->var;
                    (task->sp-1)->var = swappedVar;
                } else {
                    stringstream ss;
                    ss << "Illegal stack swap, not enough data";
                    throw vm_exception(vm.thread_stack_except, ss.str());
                }
                branch
            EXP:
                regs[triple_arg3]=pow(regs[triple_arg1], regs[triple_arg2]);
                branch
            LDC: // tested
                regs[dual_arg1]=vm.constants[dual_arg2];
                branch
            IS:
                regs[single_arg] = is_type(task->ptr, raw_arg2);
                branch_for(2)
        }

        state_check:
        if(thread->signal) {
            if (hasSignal(thread->signal, tsig_suspend))
                suspend_self();
            if(hasSignal(thread->signal, tsig_except))
                goto catch_exception;
            if (hasSignal(thread->signal, tsig_context_switch)) {
                if(thread->nativeCalls == 0) {
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

recursive_mutex library_mutex;
extern_lib* locate_lib(string name) {
    for(long i = 0; i < vm.libs.size; i++) {
        if(vm.libs.node_at(i)->data->name == name)
            return vm.libs.node_at(i)->data;
    }

    return NULL;
}

int release_lib(string name) {
    auto lib = locate_lib(name);

    guard_mutex(library_mutex)

    if(lib != nullptr) {
        bridge_fun bridge = (bridge_fun) load_func(lib->handle, "snb_main");
        for(Int i = 0; i < vm.mf.methods; i++) {
            if(vm.methods[i].bridge == bridge) {
                vm.methods[i].bridge = NULL;
                vm.methods[i].linkAddr = -1;
            }
        }

        free_lib(lib->handle);
        vm.libs.delete_at(lib, [](void *data, node<extern_lib*> *node) {
            return data == node;
        });
        return 0;
    }

    return 1;
}

bool link(extern_lib *lib, sharp_function *fun) {
    if(lib != nullptr) {
        if(!fun->nativeFunc) return false;
        else if(fun->linkAddr >= 0 && fun->bridge != nullptr) {
            throw vm_exception(vm.unsatisfied_link_except, "native function already linked");
        }

        auto linker =
                (link_proc) load_func(lib->handle,
                                     "snb_link_proc");
        int32_t linkAddr;

        if (linker != nullptr && (linkAddr = linker(fun->fullName.c_str())) >= 0) {
            fun->linkAddr = linkAddr;
            fun->bridge = (bridge_fun) load_func(lib->handle, "snb_main");

            if(fun->bridge == nullptr) {
                fun->linkAddr = -1;
                return false;
            } else return true;
        }
    }

    return false;
}

void try_link_function(sharp_function *fun) {
    for(Int i = 0; i < vm.libs.size; i++) {
        if(link(vm.libs.node_at(i)->data, fun)) {
            return;
        }
    }

    throw vm_exception(vm.unsatisfied_link_except, "native function could not be linked");
}

void invoke_delegate(Int address, Int argSize, bool staticCall) {
    if(staticCall) { // native calls are currently unsupported
        if(vm.methods[address].nativeFunc) {
            TELEMETRY_SCOPE(tm_mode_native)
            TELEMETRY_COUNT(tm_native_calls)
            if(vm.methods[address].bridge != NULL) {
                vm.methods[address].bridge(vm.methods[address].linkAddr);
            } else {
                try_link_function(&vm.methods[address]);
                vm.methods[address].bridge(vm.methods[address].linkAddr);
            }

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

    if((task->callFramePtr + 1) >= task->frameSize)
        task->growFrame();

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
    task->sp += function->frameStackOffset;
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

            return fun;
        }
    }

    return nullptr;
}

void invoke_next_frame(sharp_function *frame, bool isInterpreter) {
    if(frame == nullptr) {
        cout << "frame is empty!!! Something went wrong with rebuilding the frame." << endl;
        return;
    }

    if(frame->isHighFrequency && frame->jfunc != nullptr) {
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


void populate_string(string& s, long double* arry, uInt len) {
    for(Int i = 0; i < len; i++)
        s += (unsigned char)arry[i];
}

void exec_interrupt(Int interrupt)
{
    auto task = thread_self->task;
    switch(interrupt)
    {
        case OP_NOP:
            return;
        case OP_PRINTF:
            dbl_printf((int)registers[EGX], registers[EBX], (int)registers[ECX]);
            return;
        case OP_STRTOD: {
            auto strObject = pop_stack_object.o;
            if (strObject != NULL && strObject->type <= type_var && strObject->HEAD != NULL) {
                string num_str;
                populate_string(num_str, strObject);
                registers[EBX] = strtod(num_str.c_str(), NULL);
            } else
                throw vm_exception(vm.nullptr_except, "");
            return;
        }
        case OP_OS_TIME:
            registers[BMR] = Clock::__os_time((int)registers[EBX]);
            return;
        case OP_GC_EXPLICIT:
            /**
             * Make an explicit call to the garbage collector. This
             * does not garuntee that it will run
             */
            send_message(policy_full_sweep);
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
            registers[CMT] = gc.state;
            break;
        case OP_NAN_TIME:
            registers[BMR] = Clock::realTimeInNSecs();
            return;
        case OP_DELAY:
            delay_task(registers[EBX]);
            return;
        case OP_FIBER_START: {
            auto *name = pop_stack_object.o;
            Int mainFunc = pop_stack_number;
            Int threadid = pop_stack_number;
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
                        registers[EBX] = -1;
                        return;
                    }
                }

                try {
                    fib = create_task(fiberName, &vm.methods[mainFunc % vm.mf.methods], fibObj, fibArgs, binder);
                } catch(vm_exception &e) {
                    fiberName.clear();
                    throw;
                }

                registers[EBX] = fib->id;
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }
            return;
        }
        case OP_FIBER_SUSPEND: {
            Int fiberId = pop_stack_number;
            registers[EBX] = suspend_task(fiberId);
            return;
        }
        case OP_FIBER_UNSUSPEND: {
            Int fiberId = pop_stack_number;
            registers[EBX] = unsuspend_task(fiberId);
            return;
        }
        case OP_FIBER_KILL: {
            Int fiberId = pop_stack_number;
            registers[EBX] = kill_task(fiberId);
            return;
        }
        case OP_FIBER_BIND: {
            Int fiberId = pop_stack_number;
            Int threadId = pop_stack_number;
            auto bindTo = get_thread(threadId); // before task_mutex: thread_mutex must never be taken inside it

            guard_mutex(task_mutex) // the fiber cannot be freed while we use it
            fiber *fib = locate_task(fiberId);
            registers[EBX] = 2;

            if(fib) {
                registers[EBX] = bind_task(fib, bindTo);
            }
            return;
        }
        case OP_FIBER_BOUND_COUNT: {
            Int threadId = pop_stack_number;
            auto thread = get_thread(threadId);
            registers[EBX] = thread ? thread->boundFibers.load() : 0;
            return;
        }
        case OP_FIBER_STATE: {
            Int fiberId = pop_stack_number;
            guard_mutex(task_mutex) // the fiber cannot be freed while we use it
            fiber *fib = locate_task(fiberId);
            registers[EBX] = fib ? get_state(fib) : -1;
            return;
        }
        case OP_FIBER_CURRENT: {
            grow_stack
            stack_overflow_check
            copy_object(&push_stack_object, task->fiberObject.o);
            return;
        }
        case OP_THREAD_START: {
            auto thread = get_thread((int32_t )registers[ADX]);

            if(thread != NULL) {
                copy_object(&thread->currentThread, pop_stack_object.o);
                copy_object(&thread->args, pop_stack_object.o);

                auto priorityEnum = resolve_field("priority", thread->currentThread.o);
                if(priorityEnum != NULL) {
                    thread->priority = (int) read_numeric_value(resolve_field("ordinal", priorityEnum->o)->o, 0);
                }

                auto stackSize = resolve_field("stack_size", thread->currentThread.o);
                if(stackSize != NULL) {
                    thread->stackSize = (int) read_numeric_value(stackSize->o, 0);
                }

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
            registers[CMT]=start_thread((int32_t )registers[ADX], (size_t )registers[EBX]);
            return;
        }
        case OP_STRTOL: {
            Int baseNum = pop_stack_number;
            auto str = pop_stack_object.o;
            if(str != NULL && str->type <= type_var) {
                string numStr;
                populate_string(numStr, str);
                registers[EBX] = strtoll(numStr.c_str(), NULL, baseNum);
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }
            return;
        }
        case OP_THREAD_JOIN:
            registers[CMT]=join_thread((int32_t )registers[ADX]);
            return;
        case OP_THREAD_INTERRUPT:
            registers[CMT]=interrupt_thread((int32_t )registers[ADX]);
            return;
        case OP_THREAD_CREATE:
            registers[EBX]=create_thread((int32_t )registers[ADX], (bool)registers[EBX]);
            return;
        case OP_THREAD_PRIORITY:
            registers[CMT]=set_thread_priority((int32_t )registers[ADX], (int)registers[EGX]);
            return;
        case OP_THREAD_SUSPEND:
            registers[CMT]=suspend_thread((int32_t )registers[ADX]);
            return;
        case OP_THREAD_UNSUSPEND:
            registers[CMT]=unsuspend_thread((int32_t )registers[ADX], (int32_t )registers[EBX]);
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
            registers[CMT]=__cmath(registers[EBX], registers[EGX], (int)registers[ECX]);
            return;
        case OP_RANDOM_INT:
            registers[EBX] = randInt();
            return;
        case OP_RANDOM_DOUBLE:
            registers[EBX] = randDouble();
            return;
        case OP_RANDOM_INT_RANGED:
            registers[EBX] = randInt((Int)registers[EBX], (Int)registers[EGX]);
            return;
        case OP_RANDOM_DOUBLE_RANGED:
            registers[EBX] = randDouble(registers[EBX], registers[EGX]);
            return;
        case OP_RANDOM_SEED:
            setSeed(registers[ADX]);
            return;
        case OP_SYSTEM_EXE: {
            auto str = pop_stack_object.o;
            if(str != NULL && str->type <= type_var) {
                string cmd;
                populate_string(cmd, str);
                registers[CMT] = system(cmd.c_str());
            } else
                throw vm_exception(vm.nullptr_except, "");
            return;
        }
        case OP_KBHIT:
            registers[CMT] = _kbhit();
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
            task->exitVal = (int)pop_stack_number;
            shutdown();
            return;
        case OP_MEMORY_LIMIT:
            registers[CMT] = gc.memoryLimit;
            return;
        case OP_MEMORY:
            registers[CMT] = gc.managedBytes;
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
            size_t len = pop_stack_number;
            auto array = task->sp->obj.o;

            if(array != NULL) {
                realloc_object(array, len);
            } else
                throw vm_exception(vm.nullptr_except, "");
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
                    registers[EBX] = check_access(path, (int)registers[EBX]);
                else if(interrupt==OP_FILE_ATTRS)
                    registers[EBX] = get_file_attrs(path);
                else if(interrupt==OP_FILE_UPDATE_TM)
                    registers[EBX] = last_update(path, (int)registers[EBX]);
                else if(interrupt==OP_FILE_SIZE)
                    registers[EBX] = file_size(path);
                else if(interrupt==OP_FILE_CREATE)
                    create_file(path);
                else if(interrupt==OP_FILE_DELETE)
                    registers[EBX] = delete_file(path);
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
                    registers[EBX] = File::makeDir(path);
                else if(interrupt==OP_DELETE_DIR)
                    registers[EBX] = delete_dir(path);
                else if(interrupt==OP_UPDATE_FILE_TM)
                    registers[EBX] = update_time(path, (time_t)registers[EBX]);
                else if(interrupt==OP_CHMOD)
                    registers[EBX] = __chmod(path, (mode_t)registers[EBX], (bool)registers[EGX], (bool)registers[ECX]);
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
                    registers[EBX] = rename_file(path, rename);
                else if(interrupt==OP_WRITE_FILE) {
                    File::buffer buf;
                    buf.operator<<(rename); // rename will contain our actual unicode data
                    registers[EBX] = File::write(path.c_str(), buf);
                    buf.end();
                }
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }

            return;
        }
        case OP_LOAD_LIBRARY: {
            sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;

            if (libNameObj != NULL && libNameObj->type <= type_var) {
                string name;

                populate_string(name, libNameObj->HEAD, libNameObj->size);
                guard_mutex(library_mutex)

                if(locate_lib(name) == NULL) {
                    extern_lib *lib = new extern_lib();
                    lib->name = name;

#ifndef _WIN32
                    char *libError = dlerror(); // we need this variable to ensure everything is okay
#endif
                    lib->handle = load_lib(name);

#ifndef _WIN32
                    libError = dlerror();
#endif

                    if(!lib->handle

#ifndef _WIN32
                        || libError != NULL
#endif
                            ) {
                        string libName = name;
                        delete lib;
#ifdef _WIN32
                        throw vm_exception(vm.ill_state_except, string("could not load library ") + libName);
#else
                        throw vm_exception(vm.ill_state_except, string("could not load library: ") + libError);
#endif
                    }

                    lib_init _lib_init =
                            (lib_init)load_func(lib->handle, "snb_initialize");

                    int result;
                    if(_lib_init && (result = initializeLibrary(_lib_init)) == 0) {
                        vm.libs.createnode(lib);
                    } else {
                        delete lib;
                        stringstream ss;
                        ss << "handshake failed, could not load library, error: " << result;
                        throw vm_exception(vm.ill_state_except, ss.str());
                    }
                }
            } else {
                throw vm_exception(vm.nullptr_except, "");
            }
            return;
        }
        case OP_FREE_LIBRARY: {
            sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;

            if (libNameObj != NULL && libNameObj->type <= type_var) {
                string name;
                populate_string(name, libNameObj->HEAD, libNameObj->size);
                guard_mutex(library_mutex)
                registers[EBX] = release_lib(name);
            }
            return;
        }
        case OP_LINK_FUNC: {
            sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;
            sharp_object *funcNameObj = (thread_self->task->sp--)->obj.o;

            if (libNameObj != NULL && libNameObj->type <= type_var
                && funcNameObj != NULL && funcNameObj->type <= type_var) {
                string libname, funcname;

                populate_string(libname, libNameObj->HEAD, libNameObj->size);
                populate_string(funcname, funcNameObj->HEAD, funcNameObj->size);
                guard_mutex(library_mutex)
                sharp_function *sf;

                if((sf = locate_function(funcname.c_str())))
                    registers[EBX] = link(locate_lib(libname), sf);
                else registers[EBX] = 0;
            }
            return;
        }
        case OP_DISK_SPACE:
            registers[EBX]=disk_space((int32_t )registers[EBX]);
            return;
        case OP_SIZEOF:
            registers[EBX] = sizeof_object(pop_stack_object.o);
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
                methodsField->o->HEAD[iter] = frameInfo.key;
                pcField->o->HEAD[iter++] = frameInfo.value;
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
            registers[EBX]=std::thread::hardware_concurrency();
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
            auto index = (Int)(task->sp)->var;

            if(instance && IS_CLASS(instance) && index < vm.classes[CLASS(instance->info)].instanceFields) {
                auto field = instance->node[index].o;
                if(field) {
                    if (field->type == type_object || field->type == type_class
                            || field->arrayFlag == 1) {
                        copy_object(&task->sp->obj, field);
                    } else {
                        string field_name = "value";
                        double value = field->HEAD[0];

                        switch(field->type) {
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
                                assign_numeric_class_field(obj, (long)value);
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
                                assign_numeric_class_field(obj, (unsigned long)value);
                                break;
                            }
                            case type_function_ptr: {
                                // char
                                auto obj = create_object(vm.long_class);
                                copy_object(&task->sp->obj, obj);
                                assign_numeric_class_field(obj, (unsigned long)value);
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
