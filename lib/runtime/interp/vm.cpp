//
// Created by BraxtonN on 2/17/2017.
//

#include "../internal/Environment.h"
#include "vm.h"
#include "../internal/Exe.h"
#include "../internal/Thread.h"
#include "../oo/Field.h"
#include "../oo/Method.h"
#include "../oo/Array.h"
#include "../oo/Object.h"
#include "../oo/Exception.h"
#include "../oo/Reference.h"
#include "Opcode.h"

SharpVM* vm;
Environment* env;

int CreateSharpVM(SharpVM** pVM, Environment** pEnv, std::string exe, std::list<string> pArgs)
{
    updateStackFile("Creating virtual machine:");
    vm = new SharpVM();
    env = new Environment();

    *pVM = vm;
    *pEnv = env;

    if(Process_Exe(exe) != 0)
        return 1;

    Thread::Startup();

    updateStackFile("(internal) Adding helper classes and objects");

    env->nilClass = new ClassObject(
            "$internal#NillClass",
            new Field[0] {},
            0,
            new Method[0] {},
            0,
            NULL,
            ++manifest.baseaddr
    );

    /**
     * Native objects cannot be null but this will be used
     * for assigning objects to prevent seg faults on invalid
     * instruction executions
     */
    env->nilObject = new Object();
    env->nilArray = new ArrayObject();
    env->nilReference = new Reference();
    env->emptyObject = new gc_object();

    /**
     * Aux classes
     */
    env->Throwable = new ClassObject(
            "sharp.lang#Throwable",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            NULL,
            ++manifest.baseaddr
    );

    env->RuntimeException = new ClassObject(
            "sharp.lang#RuntimeException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->Throwable,
            ++manifest.baseaddr
    );

    env->StackOverflowErr = new ClassObject(
            "sharp.lang#StackOverflowErr",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    env->ThreadStackException = new ClassObject(
            "sharp.lang#ThreadStackException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    env->IndexOutOfBoundsException = new ClassObject(
            "sharp.lang#IndexOutOfBoundsException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    env->NullptrException = new ClassObject(
            "sharp.lang#NullptrException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    updateStackFile("initializing memory objects");
    env->init();

    return 0;
}

void SharpVM::DestroySharpVM() {
    updateStackFile("Shutting down threads");
    if(Thread::self != NULL) {
        Thread::self->exit();
        exitVal = Thread::self->exitVal;
    } else
        exitVal = 1;
    Thread::shutdown();
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
    SharpVM::InterpreterThreadStart(void *arg) {
        Thread::self = (Thread*)arg;
        Thread::self->state = thread_running;

        try {
            Method* main = Thread::self->main;
            if(main != NULL) {
                Thread::self->cstack.push(main);
                Thread::self->cstack.instance = env->emptyObject;

                vm->Execute(main);
            } else {
                // handle error
            }
        } catch (Exception &e) {
            Thread::self->throwable = e.getThrowable();
        }

        if (Thread::self->id == main_threadid)
        {
            /*
            * Shutdown all running threads
            * and de-allocate al allocated
            * memory. If we do not call join()
            * to wait for all other threads
            * regardless of what they are doing, we
            * stop them.
            */
            vm->Shutdown();
        }
        else
        {
            /*
             * Check for uncaught exception in thread before exit
             */
            Thread::self->exit();
        }

#ifdef WIN32_
        return 0;
#endif
#ifdef POSIX_
        return NULL;
#endif
    }

void SharpVM::Shutdown() {
    DestroySharpVM();
    env->shutdown();
}

void SharpVM::Execute(Method *method) {
    uint64_t *pc = &Thread::self->pc;
    Thread* self = Thread::self;

    *pc = method->entry;

    Method* func;
    int64_t address;
    gc_object* obj, *ref;
    double ival, ival2;
    string strVal;

    try {
        for (;;) {

            if(self->suspendPending)
                Thread::suspendSelf();
            if(self->state == thread_killed)
                return;

            switch((int)env->bytecode[(*pc)++]) {
                case nop:
                    break;
                case push_str:
                    self->stack.pushs(env->strings[(int64_t )env->bytecode[(*pc)++]].value);
                    break;
                case _int:
                    interrupt((int32_t )env->bytecode[(*pc)++]);
                    break;
                case pushi:
                    self->stack.push(env->bytecode[(*pc)++]);
                    break;
                case ret:
                    self->cstack.pop();
                    return;
                case hlt:
                    self->state = thread_killed;
                    break;
                case _new:
                    address = (int64_t )env->bytecode[(*pc)++];
                    env->newClass(address, (int32_t )env->bytecode[(*pc)++]);
                    break;
                case casti:
                    self->stack.push((int32_t )self->stack.popInt());
                    break;
                case casts:
                    self->stack.push((int16_t )self->stack.popInt());
                    break;
                case castl:
                    self->stack.push((int64_t )self->stack.popInt());
                    break;
                case castc:
                    self->stack.push((int8_t )self->stack.popInt());
                    break;
                case castb:
                    self->stack.push((bool )self->stack.popInt());
                    break;
                case castf:
                    self->stack.push((float )self->stack.popInt());
                    break;
                case castd:
                    break;
                case add:
                    ival = self->stack.popInt();
                    self->stack.push(ival+self->stack.popInt());
                    break;
                case sub:
                    ival = self->stack.popInt();
                    self->stack.push(ival-self->stack.popInt());
                    break;
                case mult:
                    ival = self->stack.popInt();
                    self->stack.push(ival*self->stack.popInt());
                    break;
                case _div:
                    ival = self->stack.popInt();
                    self->stack.push(ival/self->stack.popInt());
                    break;
                case mod:
                    ival = self->stack.popInt();
                    self->stack.push((int64_t )ival%(int64_t )self->stack.popInt());
                    break;
                case pop:
                    self->stack.popvoid();
                    break;
                case load:
                    self->stack.pusho(&env->objects[(int64_t )env->bytecode[(*pc)++]]);
                    break;
                case geti:
                    self->stack.push(self->stack.popObject()->obj->nInt);
                    break;
                case gets:
                    self->stack.push(self->stack.popObject()->obj->nShort);
                    break;
                case getl:
                    self->stack.push(self->stack.popObject()->obj->nLong);
                    break;
                case getc:
                    self->stack.push(self->stack.popObject()->obj->nChar);
                    break;
                case getb:
                    self->stack.push(self->stack.popObject()->obj->nBool);
                    break;
                case getf:
                    self->stack.push(self->stack.popObject()->obj->nFloat);
                    break;
                case getd:
                    self->stack.push(self->stack.popObject()->obj->nDouble);
                    break;
                case get_str:
                    self->stack.pushs(self->stack.popObject()->obj->str);
                    break;
                case get_arrx:
                    self->stack.pusho(self->stack.popObject()->arry->get((int64_t )env->bytecode[(*pc)++]));
                    break;
                case _new2:
                    env->newClass(self->stack.popObject(), (int32_t )env->bytecode[(*pc)++]);
                    break;
                case null:
                    self->stack.popObject()->free();
                    break;
                case _new3:
                    env->newNative(self->stack.popObject(), (int8_t )env->bytecode[(*pc)++]);
                    break;
                case _new4:
                    env->newArray(self->stack.popObject(), (int8_t )env->bytecode[(*pc)++]);
                    break;
                case _new5:
                    env->newRefrence(self->stack.popObject());
                    break;
                case get_classx:
                    self->stack.pusho(
                            self->stack.popObject()->klass->get_field((int64_t )env->bytecode[(*pc)++]));
                    break;
                case get_ref:
                    self->stack.pushr(self->stack.popObject()->ref);
                    break;
                case rstore:
                    obj = self->stack.popObject();
                    ref = self->stack.popObject();
                    ref->ref->add(obj);
                    break;
                case istore:
                    ival = self->stack.popInt();
                    self->stack.popObject()->obj->nInt = (int32_t )ival;
                    break;
                case sstore:
                    ival = self->stack.popInt();
                    self->stack.popObject()->obj->nShort = (int16_t )ival;
                    break;
                case lstore:
                    ival = self->stack.popInt();
                    self->stack.popObject()->obj->nLong = (int64_t )ival;
                    break;
                case cstore:
                    ival = self->stack.popInt();
                    self->stack.popObject()->obj->nChar = (int8_t )ival;
                    break;
                case bstore:
                    ival = self->stack.popInt();
                    self->stack.popObject()->obj->nBool = (bool )ival;
                    break;
                case fstore:
                    ival = self->stack.popInt();
                    self->stack.popObject()->obj->nFloat = (float )ival;
                    break;
                case dstore:
                    ival = self->stack.popInt();
                    self->stack.popObject()->obj->nDouble = ival;
                    break;
                case store_str:
                    strVal = self->stack.popString();
                    self->stack.popObject()->obj->str = strVal;
                    break;
                case _copy:
                    obj = self->stack.popObject();
                    self->stack.popObject()->copy_object(obj);
                    break;
                case ifeq:
                    ival = self->stack.popInt();
                    self->stack.push(ival==self->stack.popInt());
                    break;
                case ifneq:
                    ival = self->stack.popInt();
                    self->stack.push(ival!=self->stack.popInt());
                    break;
                case iflt:
                    ival = self->stack.popInt();
                    self->stack.push(ival<self->stack.popInt());
                    break;
                case ifge:
                    ival = self->stack.popInt();
                    self->stack.push(ival >= self->stack.popInt());
                    break;
                case ifgt:
                    ival = self->stack.popInt();
                    self->stack.push(ival > self->stack.popInt());
                    break;
                case ifle:
                    ival = self->stack.popInt();
                    self->stack.push(ival <= self->stack.popInt());
                    break;
                case str_cmpeq:
                    strVal = self->stack.popString();
                    self->stack.push(strVal == self->stack.popString());
                    break;
                case str_cmpne:
                    strVal = self->stack.popString();
                    self->stack.push(strVal != self->stack.popString());
                    break;
                case str_cmplt:
                    strVal = self->stack.popString();
                    self->stack.push(strVal < self->stack.popString());
                    break;
                case str_cmpgt:
                    strVal = self->stack.popString();
                    self->stack.push(strVal > self->stack.popString());
                    break;
                case str_cmple:
                    strVal = self->stack.popString();
                    self->stack.push(strVal <= self->stack.popString());
                    break;
                case str_cmpge:
                    strVal = self->stack.popString();
                    self->stack.push(strVal >= self->stack.popString());
                    break;
                case jmpeq: {
                    if(self->stack.popInt() == 1) {
                        ival = (*pc)+env->bytecode[(*pc)++];
                        if(ival >= manifest.isize || ival < 0)
                            throw Exception("invalid address jump");
                        (*pc) =(uint64_t )ival;
                    }
                    break;
                }
                case jmpne:{
                    if(self->stack.popInt() == 0) {
                        ival = (*pc)+env->bytecode[(*pc)++];
                        if(ival >= manifest.isize || ival < 0)
                            throw Exception("invalid address jump");
                        (*pc) =(uint64_t )ival;
                    }
                    break;
                }
                case neg:
                    self->stack.push(-self->stack.popInt());
                    break;
                case _and:
                    ival = self->stack.popInt();
                    self->stack.push((int64_t )ival&(int64_t )self->stack.popInt());
                    break;
                case _or:
                    ival = self->stack.popInt();
                    self->stack.push((int64_t )ival|(int64_t )self->stack.popInt());
                    break;
                case _xor:
                    ival = self->stack.popInt();
                    self->stack.push((int64_t )ival^(int64_t )self->stack.popInt());
                    break;
                case and2:
                    ival = self->stack.popInt();
                    self->stack.push(ival&&self->stack.popInt());
                    break;
                case or2:
                    ival = self->stack.popInt();
                    self->stack.push(ival||self->stack.popInt());
                    break;
                case _goto: {
                    ival = self->stack.popInt();
                    if(ival >= manifest.isize || ival < 0)
                        throw Exception("invalid address jump");
                    (*pc) =(uint64_t )ival;
                    break;
                }
                case _iadr:
                    self->stack.push((*pc));
                    break;
                case invoke: {
                    obj = self->cstack.instance;
                    (*pc) = Call(env->getMethod((int64_t )env->bytecode[(*pc)++]));
                    self->cstack.instance = obj;
                    break;
                }
                case instance_store:
                    self->cstack.instance = self->stack.popObject();
                    break;
                case get_self:
                    self->stack.pusho(self->cstack.instance);
                    break;
                case arry_len:
                    self->stack.push(self->stack.popObject()->arry->len);
                    break;
                case _throw:
                    throw Exception(self->stack.popObject()->klass, "");
                case lload:
                    self->stack.pusho(&self->cstack.locals[(int64_t )env->bytecode[(*pc)++]]);
                    break;
                case _catch:
                    break;
                case str_append:
                    strVal = self->stack.popString();
                    self->stack.pushs(strVal += self->stack.popInt());
                    break;
                case str_append2:
                    strVal = self->stack.popString();
                    self->stack.pushs(strVal + self->stack.popString());
                    break;
                case _strtod:
                    self->stack.push(strtod(self->stack.popString().c_str(), NULL));
                    break;
                case _strtol:
                    self->stack.push(strtoll(self->stack.popString().c_str(), NULL, 0));
                    break;
                case _lsh:
                    ival = self->stack.popInt();
                    self->stack.push((int64_t )ival<<(int64_t )self->stack.popInt());
                    break;
                case _rsh:
                    ival = self->stack.popInt();
                    self->stack.push((int64_t )ival>>(int64_t )self->stack.popInt());
                    break;
                case _lbl:
                    self->stack.popObject()->obj->nLong = (*pc);
                    break;
                case iinc:
                    self->stack.popObject()->obj->nInt++;
                    break;
                case sinc:
                    self->stack.popObject()->obj->nShort++;
                    break;
                case linc:
                    self->stack.popObject()->obj->nLong++;
                    break;
                case cinc:
                    self->stack.popObject()->obj->nChar++;
                    break;
                case binc:
                    self->stack.popObject()->obj->nBool++;
                    break;
                case finc:
                    self->stack.popObject()->obj->nFloat++;
                    break;
                case dinc:
                    self->stack.popObject()->obj->nDouble++;
                    break;

                case idec:
                    self->stack.popObject()->obj->nInt++;
                    break;
                case sdec:
                    self->stack.popObject()->obj->nShort++;
                    break;
                case ldec:
                    self->stack.popObject()->obj->nLong++;
                    break;
                case cdec:
                    self->stack.popObject()->obj->nChar++;
                    break;
                case bdec:
                    self->stack.popObject()->obj->nBool++;
                    break;
                case fdec:
                    self->stack.popObject()->obj->nFloat++;
                    break;
                case ddec:
                    self->stack.popObject()->obj->nDouble++;
                    break;
                case _goto_e: {
                    ival = self->stack.popInt();
                    ival2 = self->stack.popInt();
                    if(ival == 1) {
                        if((uint64_t )ival2 >= manifest.isize || (uint64_t )ival2 < 0)
                            throw Exception("invalid address jump");
                        (*pc) =(uint64_t )ival2;
                    }
                    break;
                }
                case _goto_ne: {
                    ival = self->stack.popInt();
                    ival2 = self->stack.popInt();
                    if(ival == 0) {
                        if((uint64_t )ival2 >= manifest.isize || (uint64_t )ival2 < 0)
                            throw Exception("invalid address jump");
                        (*pc) =(uint64_t )ival2;
                    }
                    break;
                }
                case _swap:
                    self->stack.swap();
                    break;
                default:
                    // unsupported
                    break;
            }
        }
    } catch (std::bad_alloc &e) {
        // TODO: throw out of memory error
    } catch (Exception &e) {
        self->throwable = e.getThrowable();
        self->exceptionThrown = true;

        // TODO: handle exception
    }
}

void SharpVM::interrupt(int32_t signal) {
    switch (signal) {
        case 0x9f:
            cout << Thread::self->stack.popString();
            break;
        case 0xa0:
            Thread::self->stack.popObject()->monitor->lock();
            break;
        case 0xa1:
            Thread::self->stack.popObject()->monitor->unlock();
            break;
        default:
            // unsupported
            break;
    }
}

uint64_t SharpVM::Call(Method *func) {
    uint64_t pc = Thread::self->pc;
    Thread::self->cstack.push(func);
    Thread::self->cstack.instance = env->emptyObject;

    Execute(func);
    return pc;
}
