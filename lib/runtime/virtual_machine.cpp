//
// Created by bknun on 9/18/2022.
//

#include <conio.h>
#include "virtual_machine.h"
#include "error/vm_exception.h"
#include "../core/opcode/opcode_macros.h"
#include "../core/thread_state.h"
#include "multitasking/thread/sharp_thread.h"
#include "multitasking/fiber/fiber.h"
#include "multitasking/thread/thread_controller.h"
#include "reflect/reflect_helpers.h"
#include "memory/garbage_collector.h"

virtual_machine vm;

void main_vm_loop()
{
    setupOpcodeTable
    auto thread = thread_self;
    auto task = thread->task;
    Int data = 0;

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
                exec_interrupt(single_arg);
                if(vm.state == VM_TERMINATED) return;
                check_state(1)
            _MOVI:
                registers[single_arg] = raw_arg2;
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
                        enable_exception_flag(thread, true);
                    }

                    task->pc++;
                    return;
                }

                if(data == ERR_STATE) {
                    enable_exception_flag(thread, true);
                    goto catch_exception;
                }
                branch
            HLT:
                send_interrupt_signal(thread);
                return;
            NEWARRAY:
                grow_stack
                stack_overflow_check
                copy_object(
                    &push_stack_object,
                    create_object(registers[dual_arg1], (data_type)registers[dual_arg2])
                );
                branch
            CAST:
                cast_object(task->ptr, single_arg);
                branch
            VARCAST:
                // todo: phase this instruction out
                branch
            MOV8:
                registers[dual_arg1] = (int8_t)registers[dual_arg2];
                branch
            MOV16:
                registers[dual_arg1] = (int16_t)registers[dual_arg2];
                branch
            MOV32:
                registers[dual_arg1] = (int32_t)registers[dual_arg2];
                branch
            MOV64:
                registers[dual_arg1] = (Int)registers[dual_arg2];
                branch
            MOVU8:
                registers[dual_arg1] = (uint8_t)registers[dual_arg2];
                branch
            MOVU16:
                registers[dual_arg1] = (uint16_t)registers[dual_arg2];
                branch
            MOVU32:
                registers[dual_arg1] = (uint32_t)registers[dual_arg2];
                branch
            MOVU64:
                registers[dual_arg1] = (uInt)registers[dual_arg2];
                branch
            RSTORE:
                grow_stack
                stack_overflow_check
                push_stack_number = registers[single_arg];
                branch
            ADD:
                registers[triple_arg3] = registers[triple_arg1] + registers[triple_arg2];
                branch
            SUB:
                registers[triple_arg3] = registers[triple_arg1] - registers[triple_arg2];
                branch
            MUL:
                registers[triple_arg3] = registers[triple_arg1] * registers[triple_arg2];
                branch
            DIV:
                registers[triple_arg3] = registers[triple_arg1] / registers[triple_arg2];
                branch
            MOD:
                registers[triple_arg3] = (Int)registers[triple_arg1] % (Int)registers[triple_arg2];
                branch
            IADD:
                registers[single_arg] += raw_arg2;
                branch_for(2)
            ISUB:
                registers[single_arg] -= raw_arg2;
                branch_for(2)
            IMUL:
                registers[single_arg] *= raw_arg2;
                branch_for(2)
            IDIV:
                registers[single_arg] /= raw_arg2;
                branch_for(2)
            IMOD:
                registers[single_arg] += raw_arg2;
                branch_for(2)
            POP:
                copy_object(&pop_stack_object, (sharp_object*)nullptr);
                branch
            INC:
                registers[single_arg]++;
                branch
            DEC:
                registers[single_arg]--;
                branch
            MOVR:
                registers[dual_arg1] = registers[dual_arg2];
                branch
            BRH:
                task->pc = task->rom + (Int)registers[ADX];
                check_state(0)
            IFE:
                if(registers[CMT]) {
                    task->pc = task->rom + (Int)registers[ADX];
                    check_state(0)
                } else check_state(1)
            IFNE:
                if(registers[CMT] == 0) {
                    task->pc = task->rom + (Int)registers[ADX];
                    check_state(0)
                } else check_state(1)
            LT:
                registers[CMT] = registers[dual_arg1] < registers[dual_arg2];
                branch
            GT:
                registers[CMT] = registers[dual_arg1] > registers[dual_arg2];
                branch
            LTE:
                registers[CMT] = registers[dual_arg1] <= registers[dual_arg2];
                branch
            GTE:
                registers[CMT] = registers[dual_arg1] >= registers[dual_arg2];
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
                registers[single_arg]= ( task->ptr!=NULL && task->ptr->o != NULL) ? task->ptr->o->size : 0;
                branch
            PUT:
                cout << registers[single_arg];
                branch
            PUTC:
                printf("%c", (char)registers[single_arg]);
                branch
            GET:
                registers[single_arg] = (registers[CMT] != 0) ? getche() : getch();
                branch
            CHECKLEN:
                require_object_with_value(
                        if(registers[single_arg] < task->ptr->o->size && registers[single_arg] >= 0) {
                            branch
                        }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << registers[single_arg] << " size is " << task->ptr->o->size;
                            throw vm_exception(vm.bounds_except, ss.str());
                        }
                )
            JMP:
                task->pc = task->rom + single_arg;
                check_state(0)
            LOADPC:
                registers[single_arg] = current_pc;
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
                check_state(0)
            CALLD:
                if((data = (Int)registers[single_arg]) <= 0 || data >= vm.manifest.methods) {
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
                branch_for(2)
            MOVN:
                require_object_with_value(
                        task->ptr = task->ptr->o->node + raw_arg2;
                )
                branch_for(2)
            SLEEP:
                __os_sleep(registers[single_arg]);
                branch
            TEST:
                registers[BMR] = registers[dual_arg1] == registers[dual_arg2];
                branch
            TNE:
                registers[BMR] = registers[dual_arg1] != registers[dual_arg2];
                branch
            LOCK:
                require_object_with_value(
                    lock_object(task->ptr->o);
                )
                check_state(1)
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
                    task->ptr = task->ptr->o->node + (Int)registers[single_arg];
                )
                branch
            NEWOBJARRAY:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, create_object(registers[single_arg]));
                branch
            NOT:
                registers[dual_arg1] = !registers[dual_arg2];
                branch
            SKIP: // todo: phase out
                task->pc += single_arg;
                branch
            LOADVAL:
                registers[single_arg] = pop_stack_number;
                branch
            SHL:
                registers[triple_arg3] = (Int)registers[triple_arg1] << (Int)registers[triple_arg2];
                branch
            SHR:
                registers[triple_arg3] = (Int)registers[triple_arg1] << (Int)registers[triple_arg2];
                branch
            SKPE: // todo: phase out
                if((Int)registers[dual_arg1] != 0) {
                    task->pc = task->pc + dual_arg2;
                    check_state(0)
                } else check_state(1)
            SKNE: // todo: phase out
                if((Int)registers[dual_arg1] == 0) {
                    task->pc = task->pc + dual_arg2;
                    check_state(0)
                } else check_state(1)
            CMP:
                registers[CMT] = registers[single_arg] == raw_arg2;
                branch_for(2)
            AND:
                registers[BMR] = registers[dual_arg1] && registers[dual_arg2];
                branch
            UAND:
                registers[BMR] = (Int)registers[dual_arg1] & (Int)registers[dual_arg2];
                branch
            OR:
                registers[BMR] = (Int)registers[dual_arg1] | (Int)registers[dual_arg2];
                branch
            XOR:
                registers[BMR] = (Int)registers[dual_arg1] ^ (Int)registers[dual_arg2];
                branch
            THROW:
                copy_object(&task->exceptionObject, pop_stack_object.o);
                enable_exception_flag(thread, true);
                check_state(0)
            CHECKNULL:
                registers[single_arg] = task->ptr == nullptr || task->ptr->o == nullptr;
                branch
            RETURNOBJ:
                copy_object(&task->fp->obj, task->ptr);
                branch
            NEWCLASSARRAY:
                grow_stack
                stack_overflow_check
                copy_object(
                        &push_stack_object,
                        create_object(vm.classes + raw_arg2, (Int)registers[single_arg])
                );
                branch_for(2)
            NEWSTRING:
                grow_stack
                stack_overflow_check
                copy_object(
                    &push_stack_object,
                    create_object(
                       vm.strings[single_arg].size(),
                        type_int8
                    )
                );

                assign_string_field(task->sp->obj.o, vm.strings[single_arg]);
                branch
            ADDL:
                (task->fp + dual_arg2)->var += registers[dual_arg1];
                branch
            SUBL:
                (task->fp + dual_arg2)->var -= registers[dual_arg1];
                branch
            MULL:
                (task->fp + dual_arg2)->var *= registers[dual_arg1];
                branch
            DIVL:
                (task->fp + dual_arg2)->var /= registers[dual_arg1];
                branch
            MODL:
                (task->fp + dual_arg2)->var = ((Int)(task->fp + dual_arg2)->var) % (Int)registers[dual_arg1];
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
                (task->fp + single_arg)->var = ((Int)(task->fp + single_arg)->var) % raw_arg2;
                branch_for(2)
            LOADL:
                registers[dual_arg1] = (task->fp + dual_arg2)->var;
                branch
            IALOAD:
                require_numeric_object_with_value(
                    registers[dual_arg1] = task->ptr->o->HEAD[(Int)registers[dual_arg2]];
                )
                branch
            ILOAD:
                require_numeric_object_with_value(
                        registers[single_arg] = task->ptr->o->HEAD[0];
                )
                branch
            POPOBJ:
                require_object(
                    copy_object(task->ptr, pop_stack_object.o);
                )
                branch
            SMOVR:
                (task->sp + dual_arg2)->var = registers[dual_arg1];
                branch
            SMOVR_2:
                (task->fp + dual_arg2)->var = registers[dual_arg1];
                branch
            SMOVR_3:
                copy_object(&(task->fp + single_arg)->obj, task->ptr);
                branch
            SMOVR_4:
                (task->fp + single_arg)->var = (task->fp + raw_arg2)->var;
                branch_for(2)
            ANDL:
                (task->fp + dual_arg2)->var = (Int)(task->fp + dual_arg2)->var & (Int)registers[dual_arg1];
                branch
            ORL:
                (task->fp + dual_arg2)->var = (Int)(task->fp + dual_arg2)->var | (Int)registers[dual_arg1];
                branch
            XORL:
                (task->fp + dual_arg2)->var = (Int)(task->fp + dual_arg2)->var ^ (Int)registers[dual_arg1];
                branch
            RMOV:
                require_numeric_object_with_value(
                    task->ptr->o->HEAD[(Int)registers[dual_arg1]] =
                            registers[dual_arg2];
                )
                branch
            IMOV:
                require_numeric_object_with_value(
                    task->ptr->o->HEAD[0] =
                            registers[single_arg];
                )
                branch
            NEG:
                registers[dual_arg1] = -registers[dual_arg2];
                branch
            SMOV:
                registers[dual_arg1] = (task->sp + dual_arg2)->var;
                branch
            RETURNVAL:
                task->fp->var = registers[single_arg];
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
                registers[single_arg] = (sharp_object*)data == pop_stack_object.o;
                branch
            INVOKE_DELEGATE:
                invoke_delegate(single_arg, dual_raw_arg2, dual_raw_arg1);
                check_state(0)
            ISADD:
                (task->sp + single_arg)->var += raw_arg2;
                branch_for(2)
            JE:
                if(registers[CMT]) {
                    task->pc = task->rom + single_arg;
                    check_state(0)
                } else check_state(1)
            JNE:
                if(registers[CMT] == 0) {
                    task->pc = task->rom + single_arg;
                    check_state(0)
                } else check_state(1)
            TLS_MOVL:
                task->ptr = &(task->stack + single_arg)->obj;
                branch
            MOV_ABS:
                (task->stack + single_arg)->var = registers[EBX];
                branch
            LOAD_ABS:
                registers[EBX] = (task->stack + single_arg)->var;
                branch
            DUP:
                data = (Int)&task->sp->obj;
                copy_object(&push_stack_object, (sharp_object*)data);
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
                registers[triple_arg3]=pow(registers[triple_arg1], registers[triple_arg2]);
                branch
            LDC: // tested
                registers[dual_arg1]=vm.constants[dual_arg2];
                branch
            IS:
                registers[single_arg] = is_type(task->ptr, raw_arg2);
                branch_for(2)
        }

        state_check:
        if(thread->signal) {
            if (hasSignal(thread->signal, tsig_context_switch))
                return;
            if (hasSignal(thread->signal, tsig_suspend))
                suspend_self();
            if(hasSignal(thread->signal, tsig_except))
                goto catch_exception;
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

        if(catch_exception()) {
            if(return_method())
                return;
            task->pc++;
        }

        goto run;
}

bool catch_exception() {
    auto thread = thread_self;
    auto task = thread->task;
    Int pc = current_pc;
    sharp_class *handlingClass = vm.classes + CLASS(task->exceptionObject.o->info);

    /**
     * This is the core code for the exception system, Its made to be as lightweight as possible.
     * Majority of the work such as ensuring that all finally blocks are executed within a given
     * function is handled by the compiler. We just simply need to tell the vm where to jump to.
     *
     * Here we look for a possible caught exception of a given class
     */
    for(try_catch_data &table : task->current->tryCatchTable) {
        if(pc >= table.blockStartPc && pc <= table.blockEndPc) {
            for(catch_data &caughtException : table.catchTable) {
                if(
                        handlingClass->guid == caughtException.exception->guid
                            || caughtException.exception == vm.exception_class
                            || caughtException.exception == vm.error_class
                            || caughtException.exception == vm.throwable
                ) {
                    if(caughtException.exceptionFieldAddress >= 0) {
                        copy_object(&(task->fp + caughtException.exceptionFieldAddress)->obj,
                                    task->exceptionObject.o);
                    }

                    task->pc = task->rom + caughtException.handlerPc;
                    copy_object(&task->exceptionObject, (sharp_object*) nullptr);
                    enable_exception_flag(thread, false);
                    return true;
                }
            }
        }
    }

    // Since we couldn't catch it we must jump to the first finally block (if applicable)
    for(try_catch_data &table : task->current->tryCatchTable) {
        if(table.tryStartPc <= pc && table.tryEndPc >= pc && table.finallyData)
        {
            task->pc = task->rom + table.finallyData->startPc;
            copy_object(&(task->fp + table.finallyData->exceptionFieldAddress)->obj,
                        task->exceptionObject.o);
            copy_object(&task->exceptionObject, (sharp_object*) nullptr);
            enable_exception_flag(thread, false);
        }
    }
}

void invoke_delegate(Int address, Int argSize, bool staticCall) {

}

void prepare_method(Int address) {

}

bool return_method() {

}

void exec_interrupt(Int interrupt)
{

}
