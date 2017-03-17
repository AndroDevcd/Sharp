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

void CallStack::push(Method *method) {
    sp++;

    if(sp >= default_cstack) throw Exception(Environment::StackOverflowErr, ""); // stack overflow error
    current = method;
    stack[sp].callee = method;
    if(current->locals == 0)
        stack[sp].locals = NULL;
    else
        stack[sp].locals = new gc_object[current->locals];
    regs = stack[sp].rgs;
    locals = stack[sp].locals;
}

void CallStack::pop() {
    sp--;

    if(sp <= -1) {
        Environment::free(locals, current->locals);
        current = NULL;
        regs=NULL;
        return;
    }

    Environment::free(locals, current->locals);
    current = stack[sp].callee;
    locals = stack[sp].locals;
    regs = stack[sp].rgs;
}

void CallStack::Execute() {
    uint64_t *pc = &Thread::self->pc;
    Thread* self = Thread::self;

    *pc = current->entry;

    gc_object *ptr=NULL;

    int64_t i;
    SET_Ei(i, 3)

    cout << "class E\n"
         << "op " << GET_OP(i);

    cout << endl << endl;

    SET_Di(i, 33, -36028797018963968)

    cout << "class D\n"
         << "op " << GET_OP(i) << endl
         << "arg 1 " << GET_Da(i);

    cout << endl << endl;

    SET_Ci(i, 33, 13421776,0, 134217725)

    cout << " i " << i << endl;
    cout << "class C\n"
         << "op " << GET_OP(i) << endl
         << "arg 1 " << GET_Ca(i) << endl
         << "arg 2 " << GET_Cb(i);

    cout << endl << endl;

    /*
     * Loop speed test
     */
    env->bytecode = new double[64] {
            0,      // nop
            2,      // movi %ebx,#0
            ebx, 0,
            2,      // movi %ecx, #100000000
            ecx, 100,

            33,     // movl 3
            3,
            5,      // new {int},1
            nativeint, 1,
            // .L1 (@12)
            33,     // movl 3
            3,
            2,      // movi %adx,#0
            adx, 0,
            39,     // movd %adx,1
            adx, 1,

            19,     // inc %ebx
            ebx,
            2,      // movi %ecf,#5.6
            ecf, 5.6,
            2,      // movi %edf,#1.442
            edf, 1.442,
            13,     // add %ecf,%edf
            ecf, edf,
            23,     // lt %ebx,%ecx
            ebx, ecx,
            37,     // bre
            12,
            3,      // ret
    };

    try {
        for (;;) {

            interp:
            if(self->suspendPending)
                Thread::suspendSelf();
            if(self->state == thread_killed)
                return;

            switch((int)env->bytecode[(*pc)++]) {
                case 0:
                    NOP
                case 1:
                    _int(env->bytecode[(*pc)++])
                case 2:
                    movi((int64_t)env->bytecode[(*pc)++],env->bytecode[(*pc)++])
                case 3:
                    ret
                case 4:
                    hlt
                case 5:
                    _new((int)env->bytecode[(*pc)],(int64_t)env->bytecode[(*pc)+1])
                case 6:
                    check_cast
                case 7:
                    pushd(env->bytecode[(*pc)++])
                case 8:
                    mov8((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 9:
                    mov16((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 10:
                    mov32((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 11:
                    mov64((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 12:
                    pushr((int64_t)env->bytecode[(*pc)++])
                case 13:
                    add((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 14:
                    sub((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 15:
                    mult((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 16:
                    div((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 17:
                    mod((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 18:
                    _pop
                case 19:
                    inc((int64_t)env->bytecode[(*pc)++])
                case 20:
                    dec((int64_t)env->bytecode[(*pc)++])
                case 21:
                    swapr((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 22:
                    movx((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 23:
                    lt((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 24:
                    movpc
                case 25:
                    brh
                case 26:
                    bre
                case 27:
                    _join((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 28:
                    ife
                case 29:
                    ifne
                case 30:
                    gt((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 31:
                    gte((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 32:
                    lte((int64_t)env->bytecode[(*pc)++],(int64_t)env->bytecode[(*pc)++])
                case 33:
                    movl(&locals[(int64_t)env->bytecode[(*pc)++]])
                case 34:
                    object_nxt
                case 35:
                    object_prev
                case 36: // rmov
                    _nativewrite2((int)env->bytecode[(*pc)++], regs[(int64_t)env->bytecode[(*pc)++]]) _brh
                case 37:
                    je(env->bytecode[(*pc)++])
                case 38: // mov
                    _nativewrite3((int)env->bytecode[(*pc)++],env->bytecode[(*pc)++]) _brh
                case 39: // movd
                    _nativewrite2((int)env->bytecode[(*pc)++],env->bytecode[(*pc)++]) _brh
                case 40:
                    movbi(env->bytecode[(*pc)++])
                case 41:
                    _sizeof((int64_t)env->bytecode[(*pc)++])
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
