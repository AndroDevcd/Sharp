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
#include "register.h"
#include "../oo/Object.h"
#include "../startup.h"
#include "../../util/file.h"
#include <iomanip>

void CallStack::push(Method *method) {
    sp++;

    if(sp >= default_cstack) throw Exception(&Environment::StackOverflowErr, ""); // stack overflow error
    current = method;
    stack[sp].callee = method;
    if(current->locals == 0)
        stack[sp].locals = NULL;
    else {
        stack[sp].locals = (gc_object*)malloc(sizeof(gc_object)*current->locals);
        env->init(stack[sp].locals, current->locals);
    }
    regs = stack[sp].rgs;
    locals = stack[sp].locals;
}

void CallStack::pop() {

    if((sp-1) == -1) {
        Environment::free(locals, current->locals);
        stack[sp].locals=NULL;
        current = NULL;
        regs=NULL;
        sp--;
        return;
    }

    Environment::free(locals, current->locals);
    sp--;
    current = stack[sp].callee;
    locals = stack[sp].locals;
    regs = stack[sp].rgs;
}


double exponent(int64_t n){
    if (n < 100000){
        // 5 or less
        if (n < 100){
            // 1 or 2
            if (n < 10)
                return n*0.1;
            else
                return n*0.01;
        }else{
            // 3 or 4 or 5
            if (n < 1000)
                return n*0.001;
            else{
                // 4 or 5
                if (n < 10000)
                    return n*0.0001;
                else
                    return n*0.00001;
            }
        }
    } else {
        // 6 or more
        if (n < 10000000) {
            // 6 or 7
            if (n < 1000000)
                return n*0.000001;
            else
                return n*0.0000001;
        } else if(n < 1000000000) {
            // 8 to 10
            if (n < 100000000)
                return n*0.00000001;
            else {
                // 9 or 10
                if (n < 1000000000)
                    return n*0.000000001;
                else
                    return n*0.0000000001;
            }
        } else if(n < 1000000000000000) {
            // 11 to 15
            if (n < 100000000000)
                return n*0.00000000001;
            else {
                // 12 to 15
                if (n < 1000000000000)
                    return n*0.000000000001;
                else if (n < 10000000000000)
                    return n*0.0000000000001;
                else if (n < 100000000000000)
                    return n*0.00000000000001;
                else
                    return n*0.000000000000001;
            }
        }
        else {
            return n*0.0000000000000001;
        }
    }
}

void CallStack::Execute() {
    int64_t *pc = NULL;
    Thread* self = thread_self;

    gc_object *ptr=NULL;


    pc = &env->bytecode[current->entry];
            
    try {
        for (;;) {
            _interp:
            if(self->suspendPending)
                Thread::suspendSelf();
            if(self->state == thread_killed)
                return;

            switch(GET_OP(*pc)) {
                case _NOP:
                    NOP
                case _INT:
                    _int(GET_Da(*pc))
                case MOVI:
                    movi(GET_Da(*pc))
                case RET:
                    ret
                case HLT:
                    hlt
                case NEW: /* Requires register value */
                    _new(GET_Ca(*pc),GET_Cb(*pc))
                case CHECK_CAST:
                    check_cast
                case MOV8:
                    mov8(GET_Ca(*pc),GET_Cb(*pc))
                case MOV16:
                    mov16(GET_Ca(*pc),GET_Cb(*pc))
                case MOV32:
                    mov32(GET_Ca(*pc),GET_Cb(*pc))
                case MOV64:
                    mov64(GET_Ca(*pc),GET_Cb(*pc))
                case PUSHR:
                    pushr(GET_Da(*pc))
                case ADD:
                    add(GET_Ca(*pc),GET_Cb(*pc))
                case SUB:
                    sub(GET_Ca(*pc),GET_Cb(*pc))
                case MUL:
                    mul(GET_Ca(*pc),GET_Cb(*pc))
                case DIV:
                    div(GET_Ca(*pc),GET_Cb(*pc))
                case MOD:
                    mod(GET_Ca(*pc),GET_Cb(*pc))
                case POP:
                    _pop
                case INC:
                    inc(GET_Da(*pc))
                case DEC:
                    dec(GET_Da(*pc))
                case MOVR:
                    movr(GET_Ca(*pc),GET_Cb(*pc))
                case MOVX: /* Requires register value */
                    movx(GET_Ca(*pc),GET_Cb(*pc))
                case LT:
                    lt(GET_Ca(*pc),GET_Cb(*pc))
                case BRH:
                    brh
                case BRE:
                    bre
                case IFE:
                    ife
                case IFNE:
                    ifne
                case GT:
                    gt(GET_Ca(*pc),GET_Cb(*pc))
                case GTE:
                    gte(GET_Ca(*pc),GET_Cb(*pc))
                case LTE:
                    lte(GET_Ca(*pc),GET_Cb(*pc))
                case MOVL:
                    movl(&locals[GET_Da(*pc)])
                case OBJECT_NXT:
                    object_nxt
                case OBJECT_PREV:
                    object_prev
                case RMOV:
                    _nativewrite2((int64_t)regs[GET_Ca(*pc)],regs[GET_Cb(*pc)]) _brh
                case MOV:
                    _nativewrite3((int64_t)regs[GET_Ca(*pc)],GET_Cb(*pc)) _brh
                case MOVD:
                    _nativewrite2((int64_t)regs[GET_Ca(*pc)],GET_Cb(*pc)) _brh
                case MOVBI:
                    movbi(GET_Da(*pc) + exponent(*(pc+1)))
                case _SIZEOF:
                    _sizeof(GET_Da(*pc))
                case PUT:
                    _put(GET_Da(*pc))
                case PUTC:
                    putc(GET_Da(*pc))
                default:
                    // unsupported
                    continue;
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
