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

virtual_machine vm;
thread_local long double *registers;

void main_vm_loop()
{
    setupOpcodeTable
    auto thread = thread_self;
    auto task = thread->task;
    long double *regs = registers;
    Int data;

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
                        enable_exception_flag(thread, true);
                    }

                    task->pc++;
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
                branch
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
            MOV8:
                regs[dual_arg1] = (int8_t)regs[dual_arg2];
                branch
            MOV16:
                regs[dual_arg1] = (int16_t)regs[dual_arg2];
                branch
            MOV32:
                regs[dual_arg1] = (int32_t)regs[dual_arg2];
                branch
            MOV64:
                regs[dual_arg1] = (Int)regs[dual_arg2];
                branch
            MOVU8:
                regs[dual_arg1] = (uint8_t)regs[dual_arg2];
                branch
            MOVU16:
                regs[dual_arg1] = (uint16_t)regs[dual_arg2];
                branch
            MOVU32:
                regs[dual_arg1] = (uint32_t)regs[dual_arg2];
                branch
            MOVU64:
                regs[dual_arg1] = (uInt)regs[dual_arg2];
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
                regs[triple_arg3] = (Int)regs[triple_arg1] % (Int)regs[triple_arg2];
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
                regs[single_arg] += raw_arg2;
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
                branch_for(0)
            CALLD:
                if((data = (Int)regs[single_arg]) <= 0 || data >= vm.mf.methods) {
                    stringstream ss;
                    ss << "invalid call to method (@" << data << ") out of range!";
                    throw vm_exception(ss.str());
                }

                prepare_method(data);
                branch_for(0)
            NEWCLASS:
                grow_stack
                stack_overflow_check
                copy_object(&push_stack_object, create_object(&vm.classes[raw_arg2]));
                branch_for(2)
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
                __os_sleep(regs[single_arg]);
                branch
            TEST:
                regs[BMR] = regs[dual_arg1] == regs[dual_arg2];
                branch
            TNE:
                regs[BMR] = regs[dual_arg1] != regs[dual_arg2];
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
                branch
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
                branch_for(2)
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
                branch
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
                (task->fp + dual_arg2)->var = ((Int)(task->fp + dual_arg2)->var) % (Int)regs[dual_arg1];
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
                    task->pc = task->rom + single_arg;
                    check_state(0)
                } else check_state(1)
            JNE:
                if(regs[CMT] == 0) {
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
            if (hasSignal(thread->signal, tsig_context_switch)) {
                if(thread->nativeCalls == 0) {
                    return;
                }
            }
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

        if(!catch_exception()) {
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
     * Majority of the work, such as ensuring that all finally blocks are executed within a given
     * function is handled by the compiler. We just simply need to tell the vm where to jump to.
     *
     * Here we look for a possible caught exception of a given class
     */
    for(Int i = 0; i < task->current->tryCatchTable.size; i++) {
        try_catch_data *table = &task->current->tryCatchTable.node_at(i)->data;
        if(pc >= table->blockStartPc && pc <= table->blockEndPc) {
            for(catch_data &caughtException : table->catchTable) {
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
    for(Int i = 0; i < task->current->tryCatchTable.size; i++) {
        try_catch_data *table = &task->current->tryCatchTable.node_at(i)->data;
        if(table->tryStartPc <= pc && table->tryEndPc >= pc && table->finallyData)
        {
            task->pc = task->rom + table->finallyData->startPc;
            copy_object(&(task->fp + table->finallyData->exceptionFieldAddress)->obj,
                        task->exceptionObject.o);
            copy_object(&task->exceptionObject, (sharp_object*) nullptr);
            enable_exception_flag(thread, false);
            return true;
        }
    }

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
    auto inNative = task->current && task->current->nativeFunc;
    auto function = vm.methods + address;
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
        init_struct(task->frames + ++task->callFramePtr, task->current->address,
                    task->pc, (task->sp - function->spOffset) - task->stack, task->fp - task->stack);
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

    if(!task->current->nativeFunc && inNative)
        main_vm_loop();
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

    if(task->current->nativeFunc)
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

            if(name != NULL && name->type <= type_var) {
                string fiberName;
                populate_string(fiberName, name);
                fiber *fib;

                try {
                    fib = create_task(fiberName, &vm.methods[mainFunc % vm.mf.methods]);
                } catch(vm_exception &e) {
                    fiberName.clear();
                    throw;
                }

                if(threadid != -1 && threadid != gc_threadid && threadid != idle_threadid) {
                    auto *thread = get_thread(threadid);
                    bind_task(fib, thread);
                }

                copy_object(&fib->fiberObject, pop_stack_object.o);
                copy_object(&(++fib->sp)->obj, pop_stack_object.o); // apply args to fiber's stack
                set_task_state(NULL, fib, FIB_SUSPENDED, NO_DELAY);

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
            fiber *fib = locate_task(fiberId);
            registers[EBX] = 2;

            if(fib) {
                registers[EBX] = bind_task(fib, get_thread(threadId));
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
        default: {
            stringstream ss;
            ss << "invalid system interrupt signal: " << interrupt;
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }
    }
}
