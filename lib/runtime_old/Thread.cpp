//
// Created by BraxtonN on 2/12/2018.
//

#include <cmath>
#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "Exe.h"
#include "VirtualMachine.h"
#include "Opcode.h"
#include "register.h"
#include "Manifest.h"
#include "symbols/Object.h"
#include "../util/time.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread_controller.h"
#include "scheduler/task_controller.h"

#ifdef WIN32_
#include <conio.h>
#elif defined(POSIX_)
#include "termios.h"
#endif

recursive_mutex library_mutex;
thread_local Thread* thread_self = NULL;
size_t threadStackSize = STACK_SIZE, internalStackSize = INTERNAL_STACK_SIZE;

/*
 * Local registers for the thread to use
 */
thread_local double* registers = NULL;


void Thread::free() {
    GUARD(mutex);
    if(this_fiber) {
        set_attached_thread(this_fiber, NULL);
        bind_task(this_fiber, NULL);
    }

    sendSignal(this->signal, tsig_kill, 1);
    this->terminated = true;

#ifdef SHARP_PROF_
    tprof->free();
    delete tprof;
#endif
    this->name.clear();

    if(this_fiber) {
        set_task_state(this, this_fiber, FIB_KILLED, NO_DELAY);
    }

    SharpObject *nill = NULL;
    currentThread = nill;
    args = nill;
    this->state = THREAD_KILLED;
#ifdef BUILD_JIT
    if(jctx != NULL) {
        delete jctx; jctx = NULL;
    }
#endif
}

#ifdef DEBUGGING
void printRegs() {
    cout << endl;
    if(registers != NULL) {
        cout << "Registers: \n";
        cout << "adx = " << registers[ADX] << endl;
        cout << "cx = " << registers[CX] << endl;
        cout << "cmt = " << registers[CMT] << endl;
        cout << "ebx = " << registers[EBX] << endl;
        cout << "ecx = " << registers[ECX] << endl;
        cout << "ecf = " << registers[ECF] << endl;
        cout << "edf = " << registers[EDF] << endl;
        cout << "ehf = " << registers[EHF] << endl;
        cout << "bmr = " << registers[BMR] << endl;
        cout << "egx = " << registers[EGX] << endl;
    }

    if(thread_self && thread_self->this_fiber) {
        cout << "sp -> " << (thread_self->this_fiber->sp - thread_self->this_fiber->dataStack) << endl;
        cout << "fp -> " << (thread_self->this_fiber->fp - thread_self->this_fiber->dataStack) << endl;
        cout << "pc -> " << PC(thread_self) << endl;


        string stackTrace;
        vm.fillStackTrace(stackTrace);
        cout << "call stack (most recent call last):\n" << stackTrace << endl;
        if (thread_self->this_fiber->current != NULL) {
            cout << "current function -> " << thread_self->this_fiber->current->fullName << endl;
        }

        if (thread_self->this_fiber->dataStack) {
            for (long i = 0; i < 15; i++) {
                cout << "fp.var [" << i << "] = " << thread_self->this_fiber->dataStack[i].var << ";" << endl;
            }
        }
    }
}

void printStack() {
    string str;
    vm.fillStackTrace(str);
    cout << " stack\n" << str << endl << std::flush;
}
#endif


string dataTypeToString(data_type varType, bool isArray) {
    string type;
    switch(varType) {
        case type_uint8:
            type = "_uint8";
            break;
        case type_uint16:
            type = "_uint16";
            break;
        case type_uint32:
            type = "_uint32";
            break;
        case type_uint64:
            type = "_uint64";
            break;
        case type_int8:
            type = "_int8";
            break;
        case type_int16:
            type = "_int16";
            break;
        case type_int32:
            type = "_int32";
            break;
        case type_int64:
            type = "_int64";
            break;
        case type_var:
            type = "var";
            break;
    }
    stringstream ss;
    ss << type << (isArray ? "[]" : "");
    return ss.str();
}

string getVarCastExceptionMsg(data_type varType, bool isArray, SharpObject *obj) {

    stringstream ss;
    ss << "illegal cast to `"<< dataTypeToString(varType, isArray) << "` from type `" << dataTypeToString(obj->ntype == 8 ? type_var : (data_type)obj->ntype, obj->size > 1) << "`";
    return ss.str();
}

/**
 * TODO: update exception system
 * I will keep the finally table but add an instruction to the VM for checking if we are in an "exception state" and
 * if so jump to the next respective finally block and then return if the last finally block in the function has been hit
 * we return from the function and keep doing that until we bounce out of the main function
 *
 * This will make the exception system very fast as it will only rely on the "initial firing" of the exception to figure out where it needs to jump
 * and the compiler will do the rest of the hard work by just running code
 *
 * also for the throw instruction instead of throwing the exception simply just call the respective function for the intitial firing of the exception
 * any other code that throws a exception while maybe calling a system interrupt or something will be caught and call the same thing
 *
 */
unsigned long irCount = 0, overflow = 0;

void Thread::exec() {
    Object *tmpPtr;
    Int result;

#ifdef SHARP_PROF_
    tprof->hit(this_fiber->current);
#endif

    _initOpcodeTable
    run:
    try {
        for (;;) {
            top:
//                if(this->id == 3 && this_fiber->current->address == 4226
//                  && this_fiber->callStack[this_fiber->calls].returnAddress == 4137) { // tutoriall!!!!!!!!!!!!!!!!!!
////                    if(this_fiber->sp->object.object) this_fiber->sp->object.object->print();
//                    Int i = 0;
//                }
                DISPATCH();
            _NOP: // tested
                _brh
            _INT:
                VirtualMachine::sysInterrupt(GET_Da(*this_fiber->pc));
                if(vm.state == VM_TERMINATED) return;

            HAS_SIGNAL
            context_switch_check(true)
            _brh
            _MOVI: // tested
                registers[GET_Da(*this_fiber->pc)]=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            RET: // tested
                result = GET_Da(*this_fiber->pc); // error state
                if(result == ERR_STATE && this_fiber->exceptionObject.object == NULL) {
                    this_fiber->exceptionObject
                            = (this_fiber->sp--)->object.object;
                }

                if(returnMethod(this)) {
                    // handle the exception even if we hit the last method or return back to JIT
                    if(result == ERR_STATE) {
                        GUARD(mutex);
                        sendSignal(signal, tsig_except, 1);
                    }

                    this_fiber->pc++;
                    return;
                }

                if(result == ERR_STATE) {
                    GUARD(mutex);
                    sendSignal(signal, tsig_except, 1);
                    goto exception_catch;
                }
                _brh
            HLT: // tested
                sendSignal(signal, tsig_kill, 1);
                _brh
            NEWARRAY: // tested
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->object =
                        gc.newObject(registers[GET_Ca(*this_fiber->pc)], GET_Cb(*this_fiber->pc));
                _brh
            CAST:
//                old version CHECK_NULL(this_fiber->ptr->castObject(registers[GET_Da(*this_fiber->pc)]);)
                CHECK_NULL(this_fiber->ptr->castObject(GET_Da(*this_fiber->pc));)
                _brh
            VARCAST:
                CHECK_NULL2(
                        result = GET_Ca(*this_fiber->pc) < type_function_ptr ? GET_Ca(*this_fiber->pc) : NTYPE_VAR; // ntype
                        if(!(TYPE(this_fiber->ptr->object->info) == _stype_var && this_fiber->ptr->object->ntype == result)) {
                            throw Exception(vm.IllStateExcept,
                                    getVarCastExceptionMsg((data_type)GET_Ca(*this_fiber->pc), GET_Cb(*this_fiber->pc), this_fiber->ptr->object));
                        }
                )
                _brh
            MOV8: // tested
                registers[GET_Ca(*this_fiber->pc)]=(int8_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOV16: // tested
                registers[GET_Ca(*this_fiber->pc)]=(int16_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOV32: // tested
                registers[GET_Ca(*this_fiber->pc)]=(int32_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOV64: // tested
                registers[GET_Ca(*this_fiber->pc)]=(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU8: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uint8_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU16: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uint16_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU32: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uint32_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU64: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uInt)registers[GET_Cb(*this_fiber->pc)];
                _brh
            RSTORE: // tested
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->var = registers[GET_Da(*this_fiber->pc)];
                 _brh
            ADD: // tested
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]+registers[GET_Bb(*this_fiber->pc)];
                _brh
            SUB: // tested
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]-registers[GET_Bb(*this_fiber->pc)];
                _brh
            MUL: // tested
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]*registers[GET_Bb(*this_fiber->pc)];
                _brh
            DIV: // tested
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]/registers[GET_Bb(*this_fiber->pc)];
                _brh
            MOD: // tested
                registers[GET_Bc(*this_fiber->pc)]=(Int)registers[GET_Ba(*this_fiber->pc)]%(Int)registers[GET_Bb(*this_fiber->pc)];
                _brh
            IADD: // tested
                registers[GET_Da(*this_fiber->pc)]+=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            ISUB: // tested
                registers[GET_Da(*this_fiber->pc)]-=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            IMUL: // tested
                registers[GET_Da(*this_fiber->pc)]*=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            IDIV: // tested
                if(((int32_t)*(this_fiber->pc+1))==0) throw Exception("divide by 0");
                registers[GET_Da(*this_fiber->pc)]/=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            IMOD: // tested
                registers[GET_Da(*this_fiber->pc)]=(Int)registers[GET_Da(*this_fiber->pc)]%(Int)*(this_fiber->pc+1);
                _brh_inc(2)
            POP: // tested
                this_fiber->sp->object = (SharpObject*)NULL;
                --this_fiber->sp;
                _brh
            INC: // tested
                registers[GET_Da(*this_fiber->pc)]++;
                _brh
            DEC: // tested
                registers[GET_Da(*this_fiber->pc)]--;
                _brh
            MOVR: // tested
                registers[GET_Ca(*this_fiber->pc)]=registers[GET_Cb(*this_fiber->pc)];
                _brh
            BRH: // tested
                HAS_SIGNAL
                this_fiber->pc=this_fiber->cache+(Int)registers[ADX];
                context_switch_check(false)
                _brh_NOINCREMENT
            IFE:
                HAS_SIGNAL
                if(registers[CMT]) {
                    this_fiber->pc=this_fiber->cache+(int64_t)registers[ADX];
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            IFNE:
                HAS_SIGNAL
                if(registers[CMT]==0) {
                    this_fiber->pc=this_fiber->cache+(int64_t)registers[ADX];
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            LT:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]<registers[GET_Cb(*this_fiber->pc)];
                _brh
            GT:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]>registers[GET_Cb(*this_fiber->pc)];
                _brh
            LTE:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]<=registers[GET_Cb(*this_fiber->pc)];
                _brh
            GTE:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]>=registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVL: // tested
                this_fiber->ptr = &(this_fiber->fp+GET_Da(*this_fiber->pc))->object;
                _brh
            POPL: // tested
                (this_fiber->fp+GET_Da(*this_fiber->pc))->object
                        = (this_fiber->sp--)->object.object;
                _brh
            IPOPL: // tested
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var
                        = (this_fiber->sp--)->var;
                _brh
            MOVSL: // tested
                this_fiber->ptr = &((this_fiber->sp+GET_Da(*this_fiber->pc))->object);
                _brh
            SIZEOF:
                if(this_fiber->ptr==NULL || this_fiber->ptr->object == NULL)
                    registers[GET_Da(*this_fiber->pc)] = 0;
                else
                    registers[GET_Da(*this_fiber->pc)]=this_fiber->ptr->object->size;
                _brh
            PUT: // tested
                cout << registers[GET_Da(*this_fiber->pc)];
                _brh
            PUTC:
                printf("%c", (char)registers[GET_Da(*this_fiber->pc)]);
                _brh
            GET:
                if(_64CMT)
                    registers[GET_Da(*this_fiber->pc)] = getche();
                else
                    registers[GET_Da(*this_fiber->pc)] = getch();
                _brh
            CHECKLEN:
                CHECK_NULL2(
                        if((registers[GET_Da(*this_fiber->pc)]<this_fiber->ptr->object->size) &&!(registers[GET_Da(*this_fiber->pc)]<0)) { _brh }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << registers[GET_Da(*this_fiber->pc)] << " size is " << this_fiber->ptr->object->size;
                            throw Exception(vm.IndexOutOfBoundsExcept, ss.str());
                        }
                )
            JMP: // tested
                HAS_SIGNAL
                this_fiber->pc = this_fiber->cache+GET_Da(*this_fiber->pc);
                context_switch_check(false)
                _brh_NOINCREMENT
            LOADPC: // tested
                registers[GET_Da(*this_fiber->pc)] = PC(this);
                _brh
            PUSHOBJ:
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->object = this_fiber->ptr;
                _brh
            DEL:
                gc.releaseObject(this_fiber->ptr);
                _brh
            CALL:
            HAS_SIGNAL
                executeMethod(GET_Da(*this_fiber->pc), this);
                HAS_SIGNAL
                context_switch_check(false)
                _brh_NOINCREMENT
            CALLD:
                HAS_SIGNAL
                if((result = (int64_t )registers[GET_Da(*this_fiber->pc)]) <= 0 || result >= vm.manifest.methods) {
                    stringstream ss;
                    ss << "invalid call to method with address of " << result;
                    throw Exception(ss.str());
                }
                executeMethod(result, this);
                HAS_SIGNAL
                context_switch_check(false)
                _brh_NOINCREMENT
            NEWCLASS:
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->object =
                        gc.newObject(&vm.classes[*(this_fiber->pc+1)]);
                _brh_inc(2)
            MOVN:
                CHECK_NULLOBJ(
                        if(((int32_t)*(this_fiber->pc+1)) >= this_fiber->ptr->object->size || ((int32_t)*(this_fiber->pc+1)) < 0)
                            throw Exception("movn");

                        this_fiber->ptr = &this_fiber->ptr->object->node[((int32_t)*(this_fiber->pc+1))];
                )
                _brh_inc(2)
            SLEEP:
                __os_sleep((Int)registers[GET_Da(*this_fiber->pc)]);
                _brh
            TEST:
                registers[BMR]=registers[GET_Ca(*this_fiber->pc)]==registers[GET_Cb(*this_fiber->pc)];
                _brh
            TNE:
                registers[BMR]=registers[GET_Ca(*this_fiber->pc)]!=registers[GET_Cb(*this_fiber->pc)];
                _brh
            LOCK:
                CHECK_NULL2(
                        if(!Object::monitorLock(this_fiber->ptr, this)) {
                            return;
                        }
                )
                _brh
            ULOCK:
                CHECK_NULL2(Object::monitorUnLock(this_fiber->ptr);)
                _brh
            MOVG: // TODO: update to 2 part instruction to support 16M classes
                this_fiber->ptr = vm.staticHeap+GET_Da(*this_fiber->pc);
                _brh
            MOVND:
                CHECK_NULLOBJ(
                        if(((int32_t)registers[GET_Da(*this_fiber->pc)]) >= this_fiber->ptr->object->size || ((int32_t)registers[GET_Da(*this_fiber->pc)]) < 0)
                            throw Exception("movn");

                        this_fiber->ptr = &this_fiber->ptr->object->node[(Int)registers[GET_Da(*this_fiber->pc)]];
                )
                _brh
            NEWOBJARRAY:
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->object = gc.newObjectArray(registers[GET_Da(*this_fiber->pc)]);
                _brh
            NOT:
                registers[GET_Ca(*this_fiber->pc)]=!registers[GET_Cb(*this_fiber->pc)];
                _brh
            SKIP:
                HAS_SIGNAL
                this_fiber->pc = this_fiber->pc+GET_Da(*this_fiber->pc);
                _brh
            LOADVAL:
                registers[GET_Da(*this_fiber->pc)]=(this_fiber->sp--)->var;
                _brh
            SHL:
                registers[GET_Bc(*this_fiber->pc)]=(int64_t)registers[GET_Ba(*this_fiber->pc)]<<(int64_t)registers[GET_Bb(*this_fiber->pc)];
                _brh
            SHR:
                registers[GET_Bc(*this_fiber->pc)]=(int64_t)registers[GET_Ba(*this_fiber->pc)]>>(int64_t)registers[GET_Bb(*this_fiber->pc)];
                _brh
            SKPE:
                HAS_SIGNAL
                if(((Int)registers[GET_Ca(*this_fiber->pc)]) != 0) {
                    this_fiber->pc = this_fiber->pc+GET_Cb(*this_fiber->pc); _brh_NOINCREMENT
                } else _brh
            SKNE:
                HAS_SIGNAL
                if(((Int)registers[GET_Ca(*this_fiber->pc)])==0) {
                    this_fiber->pc = this_fiber->pc+GET_Cb(*this_fiber->pc); _brh_NOINCREMENT
                } else _brh
            CMP:
                registers[CMT]=registers[GET_Da(*this_fiber->pc)]==((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            AND:
                registers[BMR]=registers[GET_Ca(*this_fiber->pc)]&&registers[GET_Cb(*this_fiber->pc)];
                _brh
            UAND:
                registers[BMR]=(Int)registers[GET_Ca(*this_fiber->pc)]&(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            OR:
                registers[BMR]=(Int)registers[GET_Ca(*this_fiber->pc)]|(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            XOR:
                registers[BMR]=(Int)registers[GET_Ca(*this_fiber->pc)]^(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            THROW:
                this_fiber->exceptionObject = (this_fiber->sp--)->object;
                sendSignal(signal, tsig_except, 1);
                goto exception_catch;
                _brh
            CHECKNULL:
                registers[GET_Da(*this_fiber->pc)]=this_fiber->ptr == NULL || this_fiber->ptr->object==NULL;
                _brh
            RETURNOBJ:
                this_fiber->fp->object=this_fiber->ptr;
                _brh
            NEWCLASSARRAY:
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->object = gc.newObjectArray(
                        registers[GET_Da(*this_fiber->pc)], &vm.classes[*(this_fiber->pc+1)]);
                _brh_inc(2)
            NEWSTRING:
                grow_stack
                STACK_CHECK
                gc.createStringArray(&(++this_fiber->sp)->object,
                        vm.strings[GET_Da(*this_fiber->pc)]);
                 _brh
            ADDL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var+=registers[GET_Ca(*this_fiber->pc)];
                _brh
            SUBL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var-=registers[GET_Ca(*this_fiber->pc)];
                _brh
            MULL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var*=registers[GET_Ca(*this_fiber->pc)];
                _brh
            DIVL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var/=registers[GET_Ca(*this_fiber->pc)];
                _brh
            MODL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->modul(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            IADDL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var+=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            ISUBL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var-=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            IMULL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var*=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            IDIVL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var/=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            IMODL:
                result = (this_fiber->fp+GET_Da(*this_fiber->pc))->var;
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var=result%((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            LOADL:
                registers[GET_Ca(*this_fiber->pc)]=(this_fiber->fp+GET_Cb(*this_fiber->pc))->var;
                _brh
            IALOAD:
                CHECK_NULLVAR(
                        if(((int32_t)registers[GET_Cb(*this_fiber->pc)]) >= this_fiber->ptr->object->size || ((int32_t)registers[GET_Cb(*this_fiber->pc)]) < 0)
                            throw Exception("iaload");

                        registers[GET_Ca(*this_fiber->pc)] =
                                this_fiber->ptr->object->HEAD[(Int)registers[GET_Cb(*this_fiber->pc)]];
                )
                _brh
            ILOAD:
                CHECK_NULLVAR(
                        registers[GET_Da(*this_fiber->pc)] = this_fiber->ptr->object->HEAD[0];
                )
                _brh
            POPOBJ:
                CHECK_NULL(
                        *this_fiber->ptr = (this_fiber->sp--)->object;
                )
                _brh
            SMOVR:
                (this_fiber->sp+GET_Cb(*this_fiber->pc))->var=registers[GET_Ca(*this_fiber->pc)];
                _brh
            SMOVR_2:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var=registers[GET_Ca(*this_fiber->pc)];
                _brh
            SMOVR_3:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->object=this_fiber->ptr;
                _brh
            SMOVR_4:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var=(this_fiber->fp+((int32_t)*(this_fiber->pc+1)))->var;
            _brh_inc(2)
            ANDL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->andl(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            ORL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->orl(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            XORL:
            (this_fiber->fp + GET_Cb(*this_fiber->pc))->xorl(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            RMOV:
                CHECK_NULLVAR(
                        if(((int32_t)registers[GET_Ca(*this_fiber->pc)]) >= this_fiber->ptr->object->size || ((int32_t)registers[GET_Ca(*this_fiber->pc)]) < 0)
                            throw Exception("movn");

                        this_fiber->ptr->object->HEAD[(Int)registers[GET_Ca(*this_fiber->pc)]]=
                                registers[GET_Cb(*this_fiber->pc)];
                )
                _brh
            IMOV:
                CHECK_NULLVAR(
                        this_fiber->ptr->object->HEAD[0]=
                                registers[GET_Da(*this_fiber->pc)];
                )
                _brh
            NEG:
                registers[GET_Ca(*this_fiber->pc)]=-registers[GET_Cb(*this_fiber->pc)];
                _brh
            SMOV:
                registers[GET_Ca(*this_fiber->pc)]=(this_fiber->sp+GET_Cb(*this_fiber->pc))->var;
                _brh
            RETURNVAL:
                (this_fiber->fp)->var=registers[GET_Da(*this_fiber->pc)];
                _brh
            ISTORE:
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->var = ((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            ISTOREL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            PUSHNULL:
                grow_stack
                STACK_CHECK
                gc.releaseObject(&(++this_fiber->sp)->object);
                gc.releaseObject(&(++this_fiber->sp)->object);
                _brh
            IPUSHL:
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->var = (this_fiber->fp+GET_Da(*this_fiber->pc))->var;
                _brh
            PUSHL:
                grow_stack
                STACK_CHECK
                (++this_fiber->sp)->object = (this_fiber->fp+GET_Da(*this_fiber->pc))->object;
                _brh
            ITEST:
                tmpPtr = &(this_fiber->sp--)->object;
                registers[GET_Da(*this_fiber->pc)] = tmpPtr->object == (this_fiber->sp--)->object.object;
                _brh
            INVOKE_DELEGATE:
                HAS_SIGNAL
                invokeDelegate(GET_Da(*this_fiber->pc), GET_Cb(*(this_fiber->pc+1)), this, GET_Ca(*(this_fiber->pc+1)) == 1, false);

                context_switch_check(false)
                _brh_NOINCREMENT
            ISADD:
                (this_fiber->sp+GET_Da(*this_fiber->pc))->var+=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            JE:
                HAS_SIGNAL
                if(registers[CMT]) {
                    this_fiber->pc=this_fiber->cache+GET_Da(*this_fiber->pc);
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            JNE:
                HAS_SIGNAL
                if(registers[CMT]==0) {
                    this_fiber->pc=this_fiber->cache+GET_Da(*this_fiber->pc);
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            TLS_MOVL:
                this_fiber->ptr = &(this_fiber->dataStack+GET_Da(*this_fiber->pc))->object;
                _brh
            MOV_ABS:
                (this_fiber->dataStack+GET_Da(*this_fiber->pc))->var = registers[EBX];
                _brh
            LOAD_ABS:
                registers[EBX] = (this_fiber->dataStack+GET_Da(*this_fiber->pc))->var;
                _brh
            DUP:
                tmpPtr = &this_fiber->sp->object;
                (++this_fiber->sp)->object = tmpPtr;
                _brh
            POPOBJ_2:
                this_fiber->ptr = &(this_fiber->sp--)->object;
                _brh
            SWAP:
                if((this_fiber->sp-this_fiber->dataStack) >= 2) {
                    SharpObject *swappedObj = this_fiber->sp->object.object;
                    (this_fiber->sp)->object = (this_fiber->sp-1)->object;
                    (this_fiber->sp-1)->object = swappedObj;
                    double swappedVar = this_fiber->sp->var;
                    (this_fiber->sp)->var = (this_fiber->sp-1)->var;
                    (this_fiber->sp-1)->var = swappedVar;
                } else {
                    stringstream ss;
                    ss << "Illegal stack swap while sp is ( " << (Int )(this_fiber->sp - this_fiber->dataStack) << ") ";
                    throw Exception(vm.ThreadStackExcept, ss.str());
                }
                _brh
            EXP:
                registers[GET_Bc(*this_fiber->pc)]=pow(registers[GET_Ba(*this_fiber->pc)], registers[GET_Bb(*this_fiber->pc)]);
                _brh
            LDC: // tested
                registers[GET_Ca(*this_fiber->pc)]=vm.constants[GET_Cb(*this_fiber->pc)];
                _brh
            IS:
                registers[GET_Da(*this_fiber->pc)] = VirtualMachine::isType(this_fiber->ptr, *(this_fiber->pc+1));
                _brh_inc(2)



        }
    } catch (Exception &e) {
        sendSignal(signal, tsig_except, 1);
    }

    exception_catch:
    if(state == THREAD_KILLED) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return;
    }

    if(!VirtualMachine::catchException()) {
        if(returnMethod(this))
            return;
        this_fiber->pc++;
    }

    if(this_fiber->current->nativeFunc) {
        return;
    }

    goto run;
}

