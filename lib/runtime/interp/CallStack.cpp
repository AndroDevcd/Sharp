//
// Created by BraxtonN on 2/28/2017.
//

#include "CallStack.h"
#include "../oo/Method.h"
#include "Opcode.h"
#include "../internal/Thread.h"
#include "vm.h"
#include "../oo/Field.h"
#include "../oo/Method.h"
#include "../oo/Array.h"

void CallStack::push(Method *method) {
    sp++;

    if(sp >= default_cstack) throw Exception(Environment::StackOverflowErr, ""); // stack overflow error
    current = method;
    stack[sp] = method;
    if(current->locals == 0)
        lstack[sp] = NULL;
    else
        lstack[sp] = new gc_object[current->locals];
    locals = lstack[sp];
}

void CallStack::pop() {
    sp--;

    if(sp <= -1) {
        Environment::free(locals, current->locals);
        current = NULL;
        return;
    }

    Environment::free(locals, current->locals);
    current = stack[sp];
    locals = lstack[sp];
}

void CallStack::Execute() {
    uint64_t *pc = &Thread::self->pc;
    Thread* self = Thread::self;

    *pc = current->entry;

    int64_t address;
    gc_object* obj, *ref;
    double ival, ival2;
    string strVal;

    try {
        for (;;) {

            interp:
            if(self->suspendPending)
                Thread::suspendSelf();
            if(self->state == thread_killed)
                return;

            switch((int)env->bytecode[(*pc)++]) {
                case nop:
                    goto interp;
//                case push_str:
//                    self->stack.pushs(env->strings[(int64_t )env->bytecode[(*pc)++]].value);
//                    goto interp;
                case _int:
                    vm->interrupt((int32_t )env->bytecode[(*pc)++]);
                    goto interp;
                case pushi:
                    self->stack.push(env->bytecode[(*pc)++]);
                    goto interp;
                case ret:
                    pop();
                    return;
                case hlt:
                    self->state = thread_killed;
                    goto interp;
                case _new:
                    address = (int64_t )env->bytecode[(*pc)++];
                    env->newClass(address, (int32_t )env->bytecode[(*pc)++]);
                    goto interp;
                case casti:
                    self->stack.cast32();
                    goto interp;
                case casts:
                    self->stack.cast16();
                    goto interp;
                case castl:
                    self->stack.cast64();
                    goto interp;
                case castc:
                    self->stack.cast8();
                    goto interp;
                case castb:
                    self->stack.castbool();
                    goto interp;
                case castf:
                    self->stack.castfloat();
                    goto interp;
                case castd:
                    goto interp;
                case add:
                    ival = self->stack.popn();
                    self->stack.push(ival+self->stack.popn());
                    goto interp;
                case sub:
                    ival = self->stack.popn();
                    self->stack.push(ival-self->stack.popn());
                    goto interp;
                case mult:
                    ival = self->stack.popn();
                    self->stack.push(ival*self->stack.popn());
                    goto interp;
                case _div:
                    ival = self->stack.popn();
                    self->stack.push(ival/self->stack.popn());
                    goto interp;
                case mod:
                    ival = self->stack.popn();
                    self->stack.push((int64_t )ival%(int64_t )self->stack.popn());
                    goto interp;
                case _pop:
                    self->stack.popvoid();
                    goto interp;
                case load:
                    self->stack.push(&env->objects[(int64_t )env->bytecode[(*pc)++]]);
                    goto interp;
                case geti:
                case gets:
                case getl:
                case getc:
                case getb:
                case getf:
                case getd:
                    self->stack.push(self->stack.pop()->obj->prim);
                    goto interp;
                case get_str:
                    //self->stack.pushs(self->stack.pop()->obj->str);
                    goto interp;
                case get_arrx:
                    self->stack.push(self->stack.pop()->arry->get((int64_t )env->bytecode[(*pc)++]));
                    goto interp;
                case _new2:
                    env->newClass(self->stack.pop(), (int32_t )env->bytecode[(*pc)++]);
                    goto interp;
                case null:
                    self->stack.pop()->free();
                    goto interp;
                case _new3:
                    env->newNative(self->stack.pop(), (int8_t )env->bytecode[(*pc)++]);
                    goto interp;
                case _new4:
                    env->newArray(self->stack.pop(), (int8_t )env->bytecode[(*pc)++]);
                    goto interp;
                case _new5:
                    env->newRefrence(self->stack.pop());
                    goto interp;
                case get_classx:
                    self->stack.push(
                            self->stack.pop()->klass->get_field((int64_t )env->bytecode[(*pc)++]));
                    goto interp;
                case get_ref:
                    self->stack.push(self->stack.pop()->ref->get());
                    goto interp;
                case rstore:
                    obj = self->stack.pop();
                    ref = self->stack.pop();
                    ref->ref->add(obj);
                    goto interp;
                case istore:
                case sstore:
                case lstore:
                case cstore:
                case bstore:
                case fstore:
                case dstore:
                    ival = self->stack.popn();
                    self->stack.pop()->obj->prim = ival;
                    goto interp;
                case store_str:
//                    strVal = self->stack.popString();
//                    self->stack.pop()->obj->str = strVal;
                    goto interp;
                case _copy:
                    obj = self->stack.pop();
                    self->stack.pop()->copy_object(obj);
                    goto interp;
                case ifeq:
                    ival = self->stack.popn();
                    self->stack.push(ival==self->stack.popn());
                    goto interp;
                case ifneq:
                    ival = self->stack.popn();
                    self->stack.push(ival!=self->stack.popn());
                    goto interp;
                case iflt:
                    ival = self->stack.popn();
                    self->stack.push(ival<self->stack.popn());
                    goto interp;
                case ifge:
                    ival = self->stack.popn();
                    self->stack.push(ival >= self->stack.popn());
                    goto interp;
                case ifgt:
                    ival = self->stack.popn();
                    self->stack.push(ival > self->stack.popn());
                    goto interp;
                case ifle:
                    ival = self->stack.popn();
                    self->stack.push(ival <= self->stack.popn());
                    goto interp;
                case str_cmpeq:
//                    strVal = self->stack.popString();
//                    self->stack.push(strVal == self->stack.popString());
                    goto interp;
                case str_cmpne:
//                    strVal = self->stack.popString();
//                    self->stack.push(strVal != self->stack.popString());
                    goto interp;
                case str_cmplt:
//                    strVal = self->stack.popString();
//                    self->stack.push(strVal < self->stack.popString());
                    goto interp;
                case str_cmpgt:
//                    strVal = self->stack.popString();
//                    self->stack.push(strVal > self->stack.popString());
                    goto interp;
                case str_cmple:
//                    strVal = self->stack.popString();
//                    self->stack.push(strVal <= self->stack.popString());
                    goto interp;
                case str_cmpge:
//                    strVal = self->stack.popString();
//                    self->stack.push(strVal >= self->stack.popString());
                    goto interp;
                case jmpeq: {
                    if(self->stack.popn() == 1) {
                        (*pc) +=(uint64_t )env->bytecode[(*pc)];
                    }
                    (*pc)++;
                    goto interp;
                }
                case jmpne:{
                    if(self->stack.popn() == 0) {
                        (*pc) +=(uint64_t )env->bytecode[(*pc)];
                    }
                    (*pc)++;
                    goto interp;
                }
                case neg:
                    self->stack.push(-self->stack.popn());
                    goto interp;
                case _and:
                    ival = self->stack.popn();
                    self->stack.push((int64_t )ival&(int64_t )self->stack.popn());
                    goto interp;
                case _or:
                    ival = self->stack.popn();
                    self->stack.push((int64_t )ival|(int64_t )self->stack.popn());
                    goto interp;
                case _xor:
                    ival = self->stack.popn();
                    self->stack.push((int64_t )ival^(int64_t )self->stack.popn());
                    goto interp;
                case and2:
                    ival = self->stack.popn();
                    self->stack.push(ival&&self->stack.popn());
                    goto interp;
                case or2:
                    ival = self->stack.popn();
                    self->stack.push(ival||self->stack.popn());
                    goto interp;
                case _goto: {
                    (*pc) =(uint64_t )self->stack.popn();
                    goto interp;
                }
                case _iadr:
                    self->stack.push((*pc));
                    goto interp;
                case invoke: {
                    (*pc) = vm->Call(env->getMethod((int64_t )env->bytecode[(*pc)++]));
                    goto interp;
                }
                case instance_store:
                    instance = self->stack.pop();
                    goto interp;
                case get_self:
                    self->stack.push(instance);
                    goto interp;
                case arry_len:
                    self->stack.push(self->stack.pop()->arry->len);
                    goto interp;
                case _throw:
                    throw Exception(self->stack.pop()->klass, "");
                case lload:
                    self->stack.push(&locals[(int64_t )env->bytecode[(*pc)++]]);
                    goto interp;
                case _catch:
                    goto interp;
                case str_append:
                    // strVal = self->stack.popString();
                    //self->stack.pushs(strVal += self->stack.popn());
                    goto interp;
                case str_append2:
                    //strVal = self->stack.popString();
                    //self->stack.pushs(strVal + self->stack.popString());
                    goto interp;
                case _strtod:
                    //self->stack.push(strtod(self->stack.popString().c_str(), NULL));
                    goto interp;
                case _strtol:
                    //self->stack.push(strtoll(self->stack.popString().c_str(), NULL, 0));
                    goto interp;
                case _lsh:
                    ival = self->stack.popn();
                    self->stack.push((int64_t )ival<<(int64_t )self->stack.popn());
                    goto interp;
                case _rsh:
                    ival = self->stack.popn();
                    self->stack.push((int64_t )ival>>(int64_t )self->stack.popn());
                    goto interp;
                case _lbl:
                    self->stack.pop()->obj->prim = (*pc);
                    goto interp;
                case iinc:
                case sinc:
                case linc:
                case cinc:
                case binc:
                case finc:
                case dinc:
                    self->stack.pop()->obj->prim++;
                    goto interp;
                case idec:
                case sdec:
                case ldec:
                case cdec:
                case bdec:
                case fdec:
                case ddec:
                    self->stack.pop()->obj->prim--;
                    goto interp;
                case _goto_e: {
                    ival = self->stack.popn();
                    ival2 = self->stack.popn();
                    if(ival == 1) (*pc) =(uint64_t )ival2;
                    goto interp;
                }
                case _goto_ne: {
                    ival = self->stack.popn();
                    if(ival == 0) (*pc) =(uint64_t )self->stack.popn();
                    goto interp;
                }
                case _swap:
                    self->stack.swap();
                    goto interp;
                case loadi:
                    self->stack.push(env->objects[(int64_t )env->bytecode[(*pc)++]].obj->prim);
                    goto interp;
                default:
                    // unsupported
                    goto interp;
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
