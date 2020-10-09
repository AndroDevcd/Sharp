//
// Created by braxtonn on 8/21/2019.
//

#include "_BaseAssembler.h"
#ifdef BUILD_JIT

#include <fstream>
#include "../Thread.h"
#include "../Manifest.h"
#include "../main.h"
#include "Jit.h"
#include "../Opcode.h"
#include "../VirtualMachine.h"
#include "../register.h"
#ifdef WIN32_
#include <conio.h>
#endif
#ifdef POSIX_
#include "termios.h"
#endif

uInt jitMemory = 0;

void _BaseAssembler::shutdown() {
    for(Int i = 0; i < functions.len; i++) {
        rt.release(functions.get(i));
    }
    functions.free();
}

void _BaseAssembler::initialize() {
    initializeRegisters();
    setupContextFields();
    setupFiberFields();
    setupThreadFields();
    setupStackElementFields();
    setupMethodFields();
    setupSharpObjectFields();
}

void _BaseAssembler::setupContextFields() {
    Thread *self = Thread::getThread(main_threadid);
    Ljit_context[jit_context_self] = getMemPtr(relative_offset((self->jctx), self, self));
    Ljit_context[jit_context_fiber] = getMemPtr(relative_offset((self->jctx), self, this_fiber));
    Ljit_context[jit_context_caller] = getMemPtr(relative_offset((self->jctx), self, caller));
    Ljit_context[jit_context_starting_pc] = getMemPtr(relative_offset((self->jctx), self, startingPc));
}

void _BaseAssembler::setupFiberFields() {
    fiber f;
    Lfiber[fiber_calls] = getMemPtr(relative_offset((&f), id, calls));
    Lfiber[fiber_dataStack] = getMemPtr(relative_offset((&f), id, dataStack));
    Lfiber[fiber_sp] = getMemPtr(relative_offset((&f), id, sp));
    Lfiber[fiber_fp] = getMemPtr(relative_offset((&f), id, fp));
    Lfiber[fiber_current] = getMemPtr(relative_offset((&f), id, current));
    Lfiber[fiber_callStack] = getMemPtr(relative_offset((&f), id, callStack));
    Lfiber[fiber_stack_lmt] = getMemPtr(relative_offset((&f), id, stackLimit));
    Lfiber[fiber_cache] = getMemPtr(relative_offset((&f), id, cache));
    Lfiber[fiber_pc] = getMemPtr(relative_offset((&f), id, pc));
    Lfiber[fiber_regs] = getMemPtr(relative_offset((&f), id, registers));
}

void _BaseAssembler::setupThreadFields() {
    Thread* thread = Thread::getThread(main_threadid);
    Lthread[thread_state] = getMemPtr(relative_offset(thread, signal, state));
    Lthread[thread_signal] = getMemPtr(relative_offset(thread, signal, signal));
    Lthread[thread_stbase] = getMemPtr(relative_offset(thread, signal, stbase));
    Lthread[thread_stack] = getMemPtr(relative_offset(thread, signal, stackSize));
    Lthread[thread_stack_rebuild] = getMemBytePtr(relative_offset(thread, signal, stackRebuild));
    Lthread[thread_context_switching] = getMemBytePtr(relative_offset(thread, signal, contextSwitching));
}

void _BaseAssembler::setupStackElementFields() {
    StackElement stack;
    Lstack_element[stack_element_var] = getMemPtr(relative_offset((&stack), var, var));
    Lstack_element[stack_element_object] = getMemPtr(relative_offset((&stack), var, object));
}

void _BaseAssembler::setupMethodFields() {
    Method m;
    Lmethod[method_bytecode] = getMemPtr(relative_offset((&m), address, bytecode));
    Lmethod[method_jit_labels] = getMemPtr(relative_offset((&m), address, jit_labels));
}

void _BaseAssembler::setupSharpObjectFields() {
    SharpObject o;
    Lsharp_object[sharp_object_HEAD] = getMemPtr(relative_offset((&o), HEAD, HEAD));
    Lsharp_object[sharp_object_node] = getMemPtr(relative_offset((&o), HEAD, node));
    Lsharp_object[sharp_object_size] = x86::dword_ptr(ctx, relative_offset((&o), HEAD, size));
    Lsharp_object[sharp_object_info] = x86::dword_ptr(ctx, relative_offset((&o), HEAD, info));
}

int _BaseAssembler::performInitialCompile() {
    int error;
    for(Int i = 0; i < vm.manifest.methods; i++) {
        if(c_options.jit && (c_options.slowBoot || vm.methods[i].isjit)) {
            vm.methods[i].isjit=false;
            error = tryJit(vm.methods+i);

            switch (error) {
                case jit_error_compile:
                    vm.methods[i].jitAttempts = JIT_MAX_ATTEMPTS;
                    break;
                case jit_error_mem:
                    return error;
                default:
                    /* ignore */
                    break;
            }
        }
    }

    return jit_error_ok;
}

int _BaseAssembler::tryJit(Method* func) {
    int error = jit_error_ok;
    if(!func->isjit && func->jitAttempts < JIT_MAX_ATTEMPTS)
    {
        if((error = compile(func)) != jit_error_ok)
        {
            func->jitAttempts++;
        }
    } else if(func->jitAttempts >= JIT_MAX_ATTEMPTS)
        error = jit_error_max_attm;

    return error;
}

/*
void jit_func(jit_context *jctx) {
    if(jctx->caller->jit_labels[0]==0)
        goto lbl_;

    int32_t *labels = new int32_t[10];

    *nop:
      ...
    *int
     ...
    *call
     ...
     ebx = 3
     goto thread_stack;
    *nop
     ...
     edf = constants[0]


    thread_stack:

    if(signal) { \
        if (hasSignal(signal, tsig_suspend)) \
            suspendSelf(); \
        if (hasSignal(signal, tsig_kill) || state == THREAD_KILLED) \
            return; \
        if(hasSignal(signal, tsig_except)) \
            goto exception_catch; \
    }

    goto labels[ebx];

    lbl_:
    labels[0] = *nop;
    labels[1] = *int;

    constants:
        [0.12345]
}
 */

int _BaseAssembler::compile(Method *func) { // TODO: IMPORTANT!!!!! write code to check the sp overflow just like thread status check is written as well as the function call stack check for stack overslow on calls
    int error = jit_error_ok;
    if(func->bytecode != NULL)
    {
        if(func->cacheSize >= JIT_IR_MIN)
        {
            // make it easier for the JIT Compiler
            func->jit_labels = (int32_t *)malloc(sizeof(int32_t)*func->cacheSize);
            if(!func->jit_labels) {
                error = jit_error_mem;
                goto finish;
            }
            else
                std::memset(func->jit_labels, 0, sizeof(int32_t)*func->cacheSize);

            beginCompilation(func);

            logComment("; method " + func->fullName.str() + "\n");
            logComment("; starting prologue");

            createFunctionPrologue();
            allocateStackSpace();

            setupGotoLabels();
            setupStackAndRegisterValues();
            createFunctionLandmarks();

            setupAddressTable();
            validateVirtualStack();

            addUserCode();

            addThreadSignalCheck();
            storeLabelValues();
            createFunctionEpilogue();
            addConstantsSection();

            /*
            x86int_t ir, irTail;
            x86::Mem lconstMem, tmpMem;
            for(x86int_t i = 0; i < func->cacheSize; i++) {
                if (error) break;
                ir = func->bytecode[i];
                if((i+1) < func->cacheSize)
                    irTail = func->bytecode[i+1];

                if(is_op(ir, op_JNE) || is_op(ir, op_GOTO) || is_op(ir, op_BRH)
                   || is_op(ir, op_IFE) || is_op(ir, op_IFNE)) {
                    threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);
                }
                stringstream ss;
                ss <<  "; instr " << i;
                string s = ss.str();
                assembler.comment(s.c_str(), s.size());
                assembler.bind(labels[i]);
                switch(GET_OP(ir)) {
                    case op_NOP:
                        assembler.nop();
                        break;
                    case op_INT: {
                        assembler.mov(ctx32, GET_Da(ir));
                        assembler.call((x86int_t) _BaseAssembler::jitSysInt); // TODO: check stack address before and after call for allognment
                        checkSystemState(lbl_func_end, i, assembler, lbl_thread_chk);
                        break;
                    }
                    case op_MOVI: {                  // registers[*(pc+1)]=GET_Da(*pc);
                        x86int_t num = GET_Da(ir);
                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway

                        emitConstant(assembler, constant_pool, vec0, num);
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_RET: {
                        assembler.mov(arg, i);
                        updatePc(assembler);
                        assembler.jmp(lbl_func_end);
                        break;
                    }
                    case op_HLT: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(Lthread[thread_state], THREAD_KILLED); // kill ourselves
                        assembler.mov(regPtr, i);
                        updatePc(assembler);
                        assembler.jmp(lbl_func_end);
                        break;
                    }
                    case op_NEWARRAY: { // not tested
                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(ctx, vec0); // double to int
                        assembler.call((x86int_t)_BaseAssembler::jitNewObject);
                        assembler.mov(tmpInt, tmp);

                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(value, Lthread[thread_sp]);
                        assembler.lea(value, x86::ptr(value, sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], value);

                        assembler.mov(ctx, tmpInt);
                        assembler.call((x86int_t)_BaseAssembler::jitSetObject0);

                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_CAST: {
                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(value, vec0); // double to int
                        assembler.mov(ctx, o2Ptr);

                        assembler.call((x86int_t)_BaseAssembler::jitCast);
                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);
                        break;
                    }
                    case op_VARCAST: {
                        assembler.mov(ctx, o2Ptr);
                        assembler.mov(value, (x86int_t )GET_Da(ir));

                        assembler.call((x86int_t)_BaseAssembler::jitCastVar);
                        checkSystemState(lbl_func_end, i, assembler, lbl_thread_chk);
                        break;
                    }
                    case op_MOV8: {
                        movRegister(assembler, vec0, GET_Cb(ir), false);

                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.movsx(tmp32, tmp8);
                        assembler.cvtsi2sd(vec0, tmp16);

                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_MOV16: {
                        movRegister(assembler, vec0, GET_Cb(ir), false);

                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.cwde();
                        assembler.cvtsi2sd(vec0, tmp16);

                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_MOV32:
                    case op_MOV64: {
                        movRegister(assembler, vec0, GET_Cb(ir), false);

                        if(is_op(ir, op_MOV32)) {
                            assembler.cvttsd2si(tmp, vec0); // double to int
                            assembler.mov(tmp, tmp32); // set lower 32 bits
                            assembler.cvtsi2sd(vec0, tmp32);
                        }

                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_MOVU8: {
                        movRegister(assembler, vec0, GET_Cb(ir), false);

                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.movzx(tmp, tmp8); // set lower 32 bits
                        assembler.cvtsi2sd(vec0, tmp);

                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_MOVU16: {
                        movRegister(assembler, vec0, GET_Cb(ir), false);

                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.movzx(tmp, tmp16); // set lower 32 bits
                        assembler.cvtsi2sd(vec0, tmp);

                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_MOVU32: { // broken
                        movRegister(assembler, vec0, GET_Cb(ir), false);

                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.mov(ctx32, tmp32);
                        assembler.cvtsi2sd(vec0, ctx);

                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_MOVU64: {
                        assembler.mov(ctx, GET_Ca(ir));
                        assembler.mov(value, GET_Cb(ir));
                        assembler.call((int64_t)_BaseAssembler::jit64BitCast);
                        break;
                    }
                    case op_RSTORE: {
                        movRegister(assembler, vec0, GET_Da(ir), false);

                        assembler.mov(ctx, threadPtr);
                        assembler.add(Lthread[thread_sp], (x86int_t)sizeof(StackElement));
                        assembler.mov(value, Lthread[thread_sp]); // sp++
                        assembler.movsd(getMemPtr(value), vec0);

                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_ADD: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        movRegister(assembler, vec1, GET_Cb(ir), false);
                        assembler.addsd(vec0, vec1);

                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_SUB: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        movRegister(assembler, vec1, GET_Cb(ir), false);
                        assembler.subsd(vec0, vec1);

                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_MUL: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        movRegister(assembler, vec1, GET_Cb(ir), false);
                        assembler.mulsd(vec0, vec1);

                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_DIV: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        movRegister(assembler, vec1, GET_Cb(ir), false);
                        assembler.divsd(vec0, vec1);

                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_MOD: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(assembler.zax(), vec0); // double to int

                        movRegister(assembler, vec1, GET_Cb(ir), false);
                        assembler.cvttsd2si(assembler.zcx(), vec1); // double to int

                        assembler.cqo();
                        assembler.idiv(assembler.zcx());
                        assembler.cvtsi2sd(vec0, assembler.zdx());

                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_IADD: { // registers[GET_Ca(*pc)]+=GET_Cb(*pc);
                        movRegister(assembler, vec0, GET_Ca(ir), false);

                        emitConstant(assembler, constant_pool, vec1, GET_Cb(ir));
                        assembler.addsd(vec1, vec0);
                        movRegister(assembler, vec1, GET_Ca(ir));
                        break;
                    }
                    case op_ISUB: { // registers[GET_Ca(*pc)]+=GET_Cb(*pc);
                        movRegister(assembler, vec0, GET_Ca(ir), false);

                        emitConstant(assembler, constant_pool, vec1, GET_Cb(ir));
                        assembler.subsd(vec0, vec1);
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_IMUL: { // registers[GET_Ca(*pc)]+=GET_Cb(*pc);
                        movRegister(assembler, vec0, GET_Ca(ir), false);

                        emitConstant(assembler, constant_pool, vec1, GET_Cb(ir));
                        assembler.mulsd(vec1, vec0);
                        movRegister(assembler, vec1, GET_Ca(ir));
                        break;
                    }
                    case op_IDIV: { // registers[GET_Ca(*pc)]+=GET_Cb(*pc);
                        movRegister(assembler, vec0, GET_Ca(ir), false);

                        emitConstant(assembler, constant_pool, vec1, GET_Cb(ir));
                        assembler.divsd(vec0, vec1);
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_IMOD: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(assembler.zax(), vec0); // double to int

                        emitConstant(assembler, constant_pool, vec1, GET_Cb(ir));
                        assembler.cvttsd2si(assembler.zcx(), vec1); // double to int

                        assembler.cqo();
                        assembler.idiv(assembler.zcx());
                        assembler.cvtsi2sd(vec0, assembler.zdx());

                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_POP: {// --sp;
                        assembler.mov(ctx, threadPtr);
                        assembler.sub(Lthread[thread_sp], (int64_t)sizeof(StackElement));
                        break;
                    }
                    case op_INC: { // registers[GET_Da(*pc)]++;
                        movRegister(assembler, vec0, GET_Da(ir), false);

                        emitConstant(assembler, constant_pool, vec1, 1);
                        assembler.addsd(vec1, vec0);

                        movRegister(assembler, vec1, GET_Da(ir));
                        break;
                    }
                    case op_DEC: { // registers[GET_Da(*pc)]--;
                        movRegister(assembler, vec0, GET_Da(ir), false);

                        emitConstant(assembler, constant_pool, vec1, 1);
                        assembler.subsd(vec0, vec1);

                        movRegister(assembler, vec0, GET_Da(ir));
                        break;
                    }
                    case op_MOVR: { // registers[GET_Ca(*pc)]=registers[GET_Cb(*pc)];
                        movRegister(assembler, vec0, GET_Cb(ir), false);
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_IALOAD: { // change
                        *
                         *
                         *
                         *  o = sp->object.object;
                            if(o != NULL && o->HEAD != NULL) {
                                registers[GET_Ca(*pc)] = o->HEAD[(int64_t)registers[GET_Cb(*pc)]];
                            } else throw Exception(Environment::NullptrException, "");
                         *
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_sp]);
                        assembler.lea(ctx, Lstack_element[stack_element_object]);
                        assembler.mov(ctx, qword_ptr(ctx));

                        assembler.cmp(ctx, 0);
                        Label isNull = assembler.newLabel(), end = assembler.newLabel();
                        assembler.je(isNull);

                        assembler.mov(tmp32, Lsharp_object[sharp_object_info]);
                        assembler.shr(tmp32, 24);
                        assembler.mov(tmp32, tmp32);
                        assembler.and_(tmp32, TYPE_MASK);
                        assembler.cmp(tmp8, _stype_var);
                        assembler.jne(isNull);
                        assembler.mov(value, Lsharp_object[sharp_object_HEAD]);

                        movRegister(assembler, vec0, GET_Cb(ir), false);
                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.mov(ctx, value);
                        Label isZero = assembler.newLabel();
                        assembler.cmp(tmp, 0);
                        assembler.je(isZero);
                        assembler.imul(tmp, (size_t)sizeof(double));
                        assembler.add(ctx, tmp);
                        assembler.bind(isZero);

                        assembler.movsd(vec0, qword_ptr(ctx));

                        movRegister(assembler, vec0, GET_Ca(ir));
                        assembler.jmp(end);
                        assembler.bind(isNull); // were not doing exceptions right now

                        assembler.mov(arg, i);
                        updatePc(assembler);
                        assembler.call((x86int_t)_BaseAssembler::jitNullPtrException);
                        assembler.mov(arg, -1);
                        assembler.jmp(lbl_thread_chk);

                        assembler.bind(end);
                        break;
                    }
                    case op_BRH: { //  pc=cache+(int64_t)registers[i64adx];
                        movRegister(assembler, vec0, i64adx, false);
                        assembler.cvttsd2si(tmp, vec0); // double to int

                        jmpToLabel(assembler, tmp, value, labelsPtr);
                        break;
                    }
                    case op_IFNE:
                    case op_IFE: {
                        Label ifTrue = assembler.newLabel();
                        movRegister(assembler, vec0, i64cmt, false);
                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.cmp(tmp, 0);
                        if(is_op(ir, op_IFE))
                            assembler.je(ifTrue);
                        else
                            assembler.jne(ifTrue);

                        movRegister(assembler, vec0, i64adx, false);
                        assembler.cvttsd2si(value, vec0); // double to int

                        jmpToLabel(assembler, value, tmp, labelsPtr);
                        assembler.bind(ifTrue);
                        break;
                    }
                    case op_ISTOREL: {              // (fp+GET_Da(*pc))->var = *(pc+1);
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);

                        if(GET_Da(ir) != 0) {
                            assembler.add(ctx, (x86int_t)(sizeof(StackElement) * GET_Da(ir)));
                        }

                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        emitConstant(assembler, constant_pool, vec0, irTail);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_LOADL: {                   // registers[GET_Ca(*pc)]=(fp+GET_Cb(*pc))->var;
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.movsd(vec0, Lstack_element[stack_element_var]);
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_JNE:
                    case op_JE: { // if(registers[i64cmt]==0) { pc=cache+GET_Da(*pc); _brh_NOINCREMENT }
                        tmpMem = qword_ptr(regPtr, (int64_t )(sizeof(double) * i64cmt));
                        assembler.pxor(vec0, vec0);
                        assembler.ucomisd(vec0, tmpMem);
                        Label ifEnd = assembler.newLabel();
                        assembler.jp(ifEnd);
                        if(is_op(ir, op_JNE))
                            assembler.jne(ifEnd);
                        else
                            assembler.je(ifEnd);
                        assembler.jmp(labels[GET_Da(ir)]);

                        assembler.bind(ifEnd);

                        break;
                    }
                    case op_IADDL: {                        // (fp+GET_Cb(*pc))->var+=GET_Ca(*pc);
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.movsd(vec1, getMemPtr(ctx));
                        emitConstant(assembler, constant_pool, vec0, GET_Ca(ir));
                        assembler.addsd(vec0, vec1);

                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_ADDL: {                        // (fp+GET_Cb(*pc))->var+=registers[GET_Ca(*pc)];
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.mov(tmp, ctx);
                        assembler.movsd(vec1, getMemPtr(ctx));

                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.addsd(vec0, vec1);
                        assembler.mov(ctx, tmp);

                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_GT:
                    case op_GTE:
                    case op_LTE:
                    case op_LT: {                     // registers[i64cmt]=registers[GET_Ca(*pc)]<registers[GET_Cb(*pc)];

                        movRegister(assembler, (is_op(ir, op_LT) || is_op(ir, op_LTE)) ? vec0 : vec1, GET_Ca(ir), false);
                        movRegister(assembler, (is_op(ir, op_LT) || is_op(ir, op_LTE)) ? vec1 : vec0, GET_Cb(ir), false);
                        assembler.comisd(vec0, vec1);

                        Label ifFalse = assembler.newLabel();
                        Label ifEnd = assembler.newLabel();
                        if(is_op(ir, op_LT) || is_op(ir, op_GT))
                            assembler.jae(ifFalse);
                        else
                            assembler.ja(ifFalse);
                        emitConstant(assembler, constant_pool, vec0, 1);
                        assembler.jmp(ifEnd);
                        assembler.bind(ifFalse);

                        assembler.pxor(vec0, vec0);
                        assembler.bind(ifEnd);

                        movRegister(assembler, vec0, i64cmt);
                        break;
                    }
                    case op_MOVL: { // o2 = &(fp+GET_Da(*pc))->object;
                        assembler.mov(ctx, threadPtr); // ctx->current
                        assembler.mov(ctx, Lthread[thread_fp]); // ctx->current->fp

                        if(GET_Da(ir) != 0) {
                            assembler.add(ctx, (x86int_t)(GET_Da(ir)*sizeof(StackElement)));
                        }

                        assembler.lea(ctx, Lstack_element[stack_element_object]);
                        assembler.mov(o2Ptr, ctx);
                        break;
                    }
                    case op_POPL: { // (fp+GET_Da(*pc))->object = (sp--)->object.object;
                        assembler.mov(ctx, threadPtr);

                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.sub(Lthread[thread_sp], (x86int_t)sizeof(StackElement));

                        assembler.mov(value, tmp);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        if(GET_Da(ir) != 0) {
                            assembler.add(ctx, (x86int_t)(sizeof(StackElement) * GET_Da(ir))); // fp+GET_DA(*pc)
                        }

                        assembler.call((x86int_t)_BaseAssembler::jitSetObject1);
                        break;
                    }
                    case op_IPOPL: { // (fp+GET_Da(*pc))->var = (sp--)->var;
                        assembler.mov(ctx, threadPtr);

                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.sub(Lthread[thread_sp], (x86int_t)sizeof(StackElement));

                        assembler.mov(ctx, tmp);
                        assembler.movsd(vec0, Lstack_element[stack_element_var]);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);

                        if(GET_Da(ir) != 0) {
                            assembler.add(ctx, (x86int_t)(sizeof(StackElement) * GET_Da(ir)));
                        }

                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_IPUSHL: { // (++sp)->var = (fp+GET_Da(*pc))->var;

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);

                        if(GET_Da(ir) != 0) {
                            assembler.add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(ir)));
                        }

                        assembler.movsd(vec0, Lstack_element[stack_element_var]);

                        assembler.mov(ctx, threadPtr);

                        assembler.add(Lthread[thread_sp], (x86int_t)sizeof(StackElement));
                        assembler.mov(tmp, Lthread[thread_sp]); // sp++

                        assembler.mov(ctx, tmp);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);

                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_MOVSL: { // o2 = &((sp+GET_Da(*pc))->object);
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_sp]);

                        if(GET_Da(ir) != 0) {
                            assembler.add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(ir))); // sp+GET_DA(*pc)
                        }

                        assembler.lea(ctx, Lstack_element[stack_element_object]);
                        assembler.mov(o2Ptr, ctx);
                        break;
                    }
                    case op_MOVBI: { // registers[i64bmr]=GET_Da(*pc) + exponent(*(pc+1)); pc++;
                        int64_t highBytes = GET_Da(ir);
                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        assembler.mov(ctx, irTail);
                        assembler.call((int64_t)exponent);
                        emitConstant(assembler, constant_pool, vec1,  highBytes);

                        assembler.addsd(vec0, vec1);
                        movRegister(assembler, vec0, i64bmr);
                        break;
                    }
                    case op_SIZEOF: {
                        **
                         *
                            if(o2==NULL || o2->object == NULL)
                                registers[GET_Da(*pc)] = 0;
                            else
                                registers[GET_Da(*pc)]=o2->object->size;
                         *
                        assembler.mov(ctx, o2Ptr);
                        assembler.cmp(ctx, 0);
                        Label ifTrue = assembler.newLabel(), end = assembler.newLabel(), ifFalse = assembler.newLabel();
                        assembler.jne(ifTrue);
                        assembler.bind(ifFalse);
                        emitConstant(assembler, constant_pool, vec0,  0);

                        movRegister(assembler, vec0, GET_Da(ir));
                        assembler.jmp(end);
                        assembler.bind(ifTrue);

                        assembler.mov(ctx, qword_ptr(ctx));
                        assembler.cmp(ctx, 0);
                        assembler.je(ifFalse);

                        assembler.mov(ctx, Lsharp_object[sharp_object_size]);
                        assembler.mov(ctx32, ctx32);
                        assembler.cvtsi2sd(vec0, ctx);
                        movRegister(assembler, vec0, GET_Da(ir));

                        assembler.bind(end);
                        break;
                    }
                    case op_PUT: {                            // cout << registers[GET_Da(*pc)];
                        assembler.mov(ctx, GET_Da(ir));
                        assembler.call((int64_t)_BaseAssembler::jitPut);
                        break;
                    }
                    case op_PUTC: {                            // printf("%c", (char)registers[GET_Da(*pc)]);
                        assembler.mov(ctx, GET_Da(ir));
                        assembler.call((int64_t)_BaseAssembler::jitPutC);
                        break;
                    }
                    case op_GET: {
                        assembler.mov(ctx, GET_Da(ir));
                        assembler.call((int64_t)_BaseAssembler::jitGet);
                        break;
                    }
                    case op_GOTO: {// $                             // pc = cache+GET_Da(*pc);
                        assembler.jmp(labels[GET_Da(ir)]);
                        break;
                    }
                    case op_LOADPC: {
                        assembler.movsd(vec0, i);
                        movRegister(assembler, vec0, GET_Da(ir));
                        break;
                    }
                    case op_PUSHOBJ: {
                        assembler.mov(ctx, threadPtr);

                        assembler.add(Lthread[thread_sp], (x86int_t)sizeof(StackElement));
                        assembler.mov(ctx, Lthread[thread_sp]); // sp++
                        assembler.lea(ctx, Lstack_element[stack_element_object]);

                        assembler.mov(value, o2Ptr);
                        assembler.call((x86int_t)_BaseAssembler::jitSetObject2);

                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_DEL: {
                        assembler.mov(ctx, o2Ptr);
                        assembler.call((x86int_t)_BaseAssembler::jitDelete);
                        break;
                    }
                    case op_NEWCLASS: {
                        assembler.mov(ctx, GET_Da(ir)); // double to int
                        assembler.call((x86int_t)_BaseAssembler::jitNewClass0);
                        assembler.mov(tmpInt, tmp);

                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);

                        assembler.mov(ctx, threadPtr);
                        assembler.add(Lthread[thread_sp], (x86int_t)sizeof(StackElement));
                        assembler.mov(value, Lthread[thread_sp]); // sp++

                        assembler.mov(ctx, tmpInt);
                        assembler.call((x86int_t)_BaseAssembler::jitSetObject0);

                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_MOVN: {
                        checkO2Node(assembler, o2Ptr, lbl_thread_chk, i);

                        if(GET_Da(ir) > 0)
                            assembler.add(ctx, (x86int_t)(GET_Da(ir)*sizeof(Object)));
                        assembler.mov(o2Ptr, ctx);
                        break;
                    }
                    case op_SLEEP: {
                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(ctx, vec0); // double to int

                        assembler.call((x86int_t)__os_sleep);
                        break;
                    }
                    case op_TEST:
                    case op_TNE: {
                        movRegister(assembler, vec1, GET_Ca(ir), false);
                        movRegister(assembler, vec0, GET_Cb(ir), false);

                        assembler.ucomisd(vec0, vec1);

                        Label ifFalse = assembler.newLabel();
                        Label ifEnd = assembler.newLabel();
                        assembler.jp(ifFalse);
                        if(is_op(ir, op_TEST))
                            assembler.jne(ifFalse);
                        else
                            assembler.je(ifFalse);
                        emitConstant(assembler, constant_pool, vec0, 1);
                        assembler.jmp(ifEnd);
                        assembler.bind(ifFalse);

                        assembler.pxor(vec0, vec0);
                        assembler.bind(ifEnd);

                        movRegister(assembler, vec0, i64cmt);
                        break;
                    }
                    case op_LOCK: {
                        checkO2Object(assembler, o2Ptr, lbl_thread_chk, i);

                        assembler.mov(ctx, o2Ptr);
                        assembler.mov(value, threadPtr);
                        assembler.call((int64_t)Object::monitorLock);
                        break;
                    }
                    case op_ULOCK: {
                        checkO2Object(assembler, o2Ptr, lbl_thread_chk, i);

                        assembler.mov(ctx, o2Ptr);
                        assembler.mov(value, threadPtr);
                        assembler.call((int64_t)Object::monitorUnLock);
                        break;
                    }
                    case op_EXP: {
                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(ctx, vec0); // double to int

                        assembler.call((x86int_t)exponent);

                        movRegister(assembler, vec0, i64bmr);
                        break;
                    }
                    case op_MOVG: {
                        Object* o = env->globalHeap+GET_Da(ir);
                        assembler.mov(o2Ptr, (x86int_t)o);
                        break;
                    }
                    case op_MOVND: {
                        checkO2Node(assembler, o2Ptr, lbl_thread_chk, i);
                        assembler.mov(value, ctx);

                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(tmp, vec0); // double to int

                        assembler.imul(tmp, (x86int_t)sizeof(Object));
                        assembler.add(value, tmp);

                        assembler.mov(o2Ptr, value);
                        break;
                    }
                    case op_NEWOBJARRAY: {
                        assembler.mov(ctx, GET_Da(ir)); // double to int
                        assembler.call((x86int_t)_BaseAssembler::jitNewObject2);
                        assembler.mov(tmpInt, tmp);

                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(value, Lthread[thread_sp]);
                        assembler.lea(value, x86::ptr(value, sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], value);

                        assembler.mov(ctx, tmpInt);
                        assembler.call((x86int_t)_BaseAssembler::jitSetObject0);

                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_NOT: {
                        Label ifTrue = assembler.newLabel(), end = assembler.newLabel();
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(tmp, vec0); // double to int

                        assembler.cmp(tmp, 0);
                        assembler.ja(ifTrue);
                        assembler.mov(tmp, 1);
                        assembler.jmp(end);

                        assembler.bind(ifTrue);
                        assembler.mov(tmp, 0);
                        assembler.bind(end);

                        assembler.cvtsi2sd(vec0, tmp); // int to Double
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }

                    case op_SKIP: {
                        if(GET_Da(ir) > 0) {
                            assembler.jmp(labels[i+GET_Da(ir)+1]);
                        } else
                            assembler.nop();
                        break;
                    }
                    case op_SKNE:
                    case op_SKPE: {
                        tmpMem = getMemPtr(regPtr, (int64_t )(sizeof(double) * i64cmt));
                        assembler.pxor(vec0, vec0);
                        assembler.ucomisd(vec0, tmpMem);
                        Label ifEnd = assembler.newLabel();
                        assembler.jp(ifEnd);
                        if(is_op(ir, op_SKPE))
                            assembler.je(ifEnd);
                        else
                            assembler.jne(ifEnd);
                        assembler.jmp(labels[i+GET_Da(ir)]);

                        assembler.bind(ifEnd);

                        break;
                    }
                    case op_CMP: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);

                        emitConstant(assembler, constant_pool, vec1, GET_Cb(ir));
                        assembler.ucomisd(vec0, vec1);
                        Label end = assembler.newLabel(), ifTrue = assembler.newLabel(), ifFalse = assembler.newLabel();
                        assembler.jp(ifFalse);
                        assembler.je(ifTrue);

                        assembler.bind(ifFalse);
                        assembler.pxor(vec0, vec0);
                        assembler.jmp(end);

                        assembler.bind(ifTrue);
                        emitConstant(assembler, constant_pool, vec0, 1);
                        assembler.bind(end);

                        movRegister(assembler, vec0, i64cmt);
                        break;
                    }
                    case op_AND: {
                        Label isFalse = assembler.newLabel(), isTrue = assembler.newLabel(),
                                end = assembler.newLabel(), next = assembler.newLabel();
                        movRegister(assembler, vec0, GET_Ca(ir), false);

                        emitConstant(assembler, constant_pool, vec1, 0);
                        assembler.ucomisd(vec0, vec1);
                        assembler.jp(next);
                        assembler.je(isFalse);

                        assembler.bind(next);
                        movRegister(assembler, vec0, GET_Cb(ir), false);
                        assembler.ucomisd(vec0, vec1);
                        assembler.jp(isTrue);
                        assembler.jne(isTrue);

                        assembler.bind(isFalse);
                        emitConstant(assembler, constant_pool, vec0, 0);
                        assembler.jmp(end);
                        assembler.bind(isTrue);
                        emitConstant(assembler, constant_pool, vec0, 1);
                        assembler.bind(end);

                        movRegister(assembler, vec0, i64cmt);
                        break;
                    }
                    case op_UAND:
                    case op_OR:
                    case op_XOR: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(tmp, vec0); // double to int
                        movRegister(assembler, vec0, GET_Cb(ir), false);
                        assembler.cvttsd2si(ctx, vec0); // double to int
                        if(is_op(ir, op_UAND))
                            assembler.and_(tmp, ctx);
                        else if(is_op(ir, op_OR))
                            assembler.or_(tmp, ctx);
                        else if(is_op(ir, op_XOR))
                            assembler.xor_(tmp, ctx);

                        assembler.cvtsi2sd(vec0, tmp); // int to Double
                        movRegister(assembler, vec0, i64cmt);
                        break;
                    }
                    case op_THROW: {
                        assembler.mov(arg, i);
                        updatePc(assembler);

                        assembler.mov(ctx, threadPtr);
                        assembler.call((int64_t)_BaseAssembler::jitThrow);

                        assembler.mov(arg, -1); // we don't need to update pc again
                        assembler.jne(lbl_thread_chk);
                        break;
                    }
                    case op_CHECKNULL: {
                        assembler.mov(ctx, o2Ptr);
                        assembler.cmp(ctx, 0); // 02==NULL
                        Label nullCheckPassed = assembler.newLabel();
                        Label nullCheckFailed = assembler.newLabel();
                        Label end = assembler.newLabel();
                        assembler.je(nullCheckFailed);

                        assembler.mov(ctx, qword_ptr(ctx));
                        assembler.cmp(ctx, 0);
                        assembler.je(nullCheckFailed);
                        assembler.jmp(nullCheckPassed);

                        assembler.bind(nullCheckFailed);
                        emitConstant(assembler, constant_pool, vec0, 1);
                        assembler.jmp(end);

                        assembler.bind(nullCheckPassed);
                        emitConstant(assembler, constant_pool, vec0, 0);

                        assembler.bind(end);
                        movRegister(assembler, vec0, i64cmt);
                        break;
                    }
                    case op_RETURNOBJ: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        assembler.lea(ctx, Lstack_element[stack_element_object]);

                        assembler.mov(value, o2Ptr);
                        assembler.call((int64_t)_BaseAssembler::jitSetObject2);
                        break;
                    }
                    case op_NEWCLASSARRAY: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(ctx, vec0); // double to int
                        assembler.mov(value, GET_Cb(ir));
                        assembler.call((x86int_t)_BaseAssembler::jitNewClass1);
                        assembler.mov(tmpInt, tmp);

                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(value, Lthread[thread_sp]);
                        assembler.lea(value, x86::ptr(value, sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], value);

                        assembler.mov(ctx, tmpInt);
                        assembler.call((x86int_t)_BaseAssembler::jitSetObject0);

                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_NEWSTRING: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(value, GET_Da(ir));
                        assembler.call((x86int_t)_BaseAssembler::jitNewString);

                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);
                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_SUBL:
                    case op_MULL:
                    case op_DIVL:  {
                        assembler.mov(ctx, threadPtr); // ctx->current
                        assembler.mov(ctx, Lthread[thread_fp]); // ctx->current->fp
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.mov(value, ctx);
                        assembler.movsd(vec0, getMemPtr(ctx));

                        movRegister(assembler, vec1, GET_Ca(ir), false);
                        if(is_op(ir, op_SUBL))
                            assembler.subsd(vec0, vec1);
                        else if(is_op(ir, op_MULL))
                            assembler.mulsd(vec0, vec1);
                        else if(is_op(ir, op_DIVL))
                            assembler.divsd(vec0, vec1);
                        assembler.mov(ctx, value);

                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_MODL: {                        // (fp+GET_Cb(*pc))->modul(registers[GET_Ca(*pc)]);
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.mov(fnPtr, ctx);
                        assembler.movsd(vec0, getMemPtr(ctx));
                        assembler.cvttsd2si(assembler.zax(), vec0);

                        movRegister(assembler, vec1, GET_Ca(ir), false);
                        assembler.cvttsd2si(assembler.zcx(), vec1);

                        assembler.cqo();
                        assembler.idiv(assembler.zcx());
                        assembler.cvtsi2sd(vec0, assembler.zdx()); // int to Double

                        assembler.mov(ctx, fnPtr);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_ISUBL:
                    case op_IMULL:
                    case op_IDIVL: {                        // (fp+GET_Cb(*pc))->var-=GET_Ca(*pc);
                        assembler.mov(ctx, threadPtr); // ctx->current
                        assembler.mov(ctx, Lthread[thread_fp]); // ctx->current->fp
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.movsd(vec0, getMemPtr(ctx));
                        emitConstant(assembler, constant_pool, vec1, GET_Ca(ir));
                        if(is_op(ir, op_ISUBL))
                            assembler.subsd(vec0, vec1);
                        else if(is_op(ir, op_IMULL))
                            assembler.mulsd(vec0, vec1);
                        else if(is_op(ir, op_IDIVL))
                            assembler.divsd(vec0, vec1);

                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_IMODL: {                        // (fp+GET_Cb(*pc))->var*=GET_Ca(*pc);
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.mov(fnPtr, ctx);
                        assembler.movsd(vec0, getMemPtr(ctx));
                        assembler.cvttsd2si(assembler.zax(), vec0);

                        emitConstant(assembler, constant_pool, vec1, GET_Ca(ir));
                        assembler.cvttsd2si(assembler.zcx(), vec1);

                        assembler.cqo();
                        assembler.idiv(assembler.zcx());
                        assembler.cvtsi2sd(vec0, assembler.zdx()); // int to Double

                        assembler.mov(ctx, fnPtr);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_IALOAD_2: { // registers[GET_Ca(*pc)] = o2->object->HEAD[(int64_t)registers[GET_Cb(*pc)]];
                        checkO2Head(assembler, o2Ptr, lbl_thread_chk, i);
                        assembler.mov(value, ctx);

                        movRegister(assembler, vec0, GET_Cb(ir), false);
                        assembler.cvttsd2si(tmp, vec0);

                        assembler.imul(tmp, (x86int_t )sizeof(double));
                        assembler.movsd(vec0, x86::qword_ptr(value, tmp));
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_POPOBJ: {
                        checkO2(assembler, o2Ptr, lbl_thread_chk, i);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.sub(Lthread[thread_sp], (x86int_t)sizeof(StackElement));

                        assembler.mov(ctx, tmp);
                        assembler.lea(value, Lstack_element[stack_element_object]);
                        assembler.mov(ctx, o2Ptr);

                        assembler.call((int64_t)_BaseAssembler::jitSetObject2);
                        break;
                    }
                    case op_SMOVR: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_sp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.mov(tmp, ctx);
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.mov(ctx, tmp);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_SMOVR_2: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.mov(tmp, ctx);
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.mov(ctx, tmp);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_ANDL:
                    case op_ORL:
                    case op_XORL: {
                        assembler.mov(ctx, threadPtr); // ctx->current
                        assembler.mov(ctx, Lthread[thread_fp]); // ctx->current->fp
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.mov(value, ctx);
                        assembler.movsd(vec0, getMemPtr(ctx));
                        assembler.cvttsd2si(tmp, vec0); // double to int

                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(ctx, vec0); // double to int
                        if(is_op(ir, op_ANDL))
                            assembler.and_(tmp, ctx);
                        else if(is_op(ir, op_ORL))
                            assembler.or_(tmp, ctx);
                        else if(is_op(ir, op_XORL))
                            assembler.xor_(tmp, ctx);

                        assembler.cvtsi2sd(vec0, tmp); // int to Double
                        assembler.mov(ctx, value);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_RMOV: {
                        checkO2Head(assembler, o2Ptr, lbl_thread_chk, i);
                        assembler.mov(value, ctx);

                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(tmp, vec0); // double to int
                        assembler.imul(tmp, (x86int_t)sizeof(double));

                        movRegister(assembler, vec0, GET_Cb(ir), false);
                        assembler.movsd(x86::qword_ptr(value, tmp), vec0);
                        break;
                    }
                    case op_SMOV: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_sp]);
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.movsd(vec0, getMemPtr(ctx));
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_LOADPC_2: {
                        emitConstant(assembler, constant_pool, vec0, i+GET_Cb(ir));
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_RETURNVAL: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);

                        assembler.mov(tmp, ctx);
                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.mov(ctx, tmp);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_PUSHNIL: {
                        assembler.mov(ctx, threadPtr);
                        assembler.call((x86int_t)_BaseAssembler::jitPushNil);
                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_PUSHL: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_fp]);

                        if(GET_Da(ir) != 0) {
                            assembler.add(ctx, (x86int_t)(sizeof(StackElement) * GET_Da(ir)));
                        }

                        assembler.lea(value, Lstack_element[stack_element_object]);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.lea(tmp, ptr(tmp, (x86int_t)sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], tmp); // sp++

                        assembler.mov(ctx, tmp);
                        assembler.lea(ctx, Lstack_element[stack_element_object]);
                        assembler.call((x86int_t)_BaseAssembler::jitSetObject2);
                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_ITEST: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.lea(value, ptr(tmp, (x86int_t)-sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], value); // sp--

                        assembler.mov(fnPtr, tmp);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.lea(value, ptr(tmp, (x86int_t)-sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], value); // sp--

                        assembler.cmp(fnPtr, tmp);
                        Label ifFalse = assembler.newLabel(), end = assembler.newLabel();
                        assembler.je(ifFalse);
                        emitConstant(assembler, constant_pool, vec0, 1);
                        assembler.jmp(end);

                        assembler.bind(ifFalse);
                        emitConstant(assembler, constant_pool, vec0, 0);

                        assembler.bind(end);
                        movRegister(assembler, vec0, GET_Da(ir));
                        break;
                    }
                    case op_INVOKE_DELEGATE: {
                        assembler.mov(ctx, GET_Ca(ir));
                        assembler.mov(value, GET_Cb(ir));
                        assembler.mov(fnArg3, threadPtr);
                        assembler.mov(fnArg4, 0);
                        assembler.call((x86int_t)_BaseAssembler::jitInvokeDelegate);
                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);
                        break;
                    }
                    case op_INVOKE_DELEGATE_STATIC: {
                        assembler.mov(ctx, GET_Ca(ir));
                        assembler.mov(value, GET_Cb(ir));
                        assembler.mov(fnArg3, threadPtr);
                        i++; assembler.bind(labels[i]);
                        assembler.mov(fnArg4, irTail);
                        assembler.call((x86int_t)_BaseAssembler::jitInvokeDelegate);
                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);
                        break;
                    }
                    case op_ISADD: {
                        assembler.mov(ctx, threadPtr); // ctx->current
                        assembler.mov(ctx, Lthread[thread_sp]); // ctx->current->fp
                        if(GET_Cb(ir) != 0) {
                            assembler.add(ctx, (x86int_t )(sizeof(StackElement) * GET_Cb(ir)));
                        }

                        assembler.movsd(vec1, getMemPtr(ctx));

                        emitConstant(assembler, constant_pool, vec0, GET_Ca(ir));
                        assembler.addsd(vec0, vec1);

                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        break;
                    }
                    case op_ISTORE: { // (++sp)->var = GET_Da(*pc);

                        emitConstant(assembler, constant_pool, vec0, GET_Da(ir));

                        assembler.mov(ctx, threadPtr);

                        assembler.add(Lthread[thread_sp], (x86int_t)sizeof(StackElement));
                        assembler.mov(value, Lthread[thread_sp]); // sp++

                        assembler.mov(ctx, value);
                        assembler.movsd(Lstack_element[stack_element_var], vec0);
                        stackCheck(assembler, lbl_thread_chk, i);
                        break;
                    }
                    case op_SWITCH: {

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(value, GET_Da(ir));
                        assembler.call((x86int_t)executeSwitch);

                        jmpToLabel(assembler, tmp, value, labelsPtr);
                        break;
                    }
                    case op_CALL: {
                        assembler.mov(arg, i);
                        updatePc(assembler);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(value, GET_Da(ir));
                        assembler.call((x86int_t) _BaseAssembler::jitCall);
                        assembler.cmp(tmp, 0);
                        Label ifTrue = assembler.newLabel();
                        assembler.je(ifTrue);
                        assembler.mov(ctx, ctxPtr);
                        assembler.call(tmp);
                        assembler.bind(ifTrue);
                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);
                        break;
                    }
                    case op_CALLD: {
                        assembler.mov(arg, i);
                        updatePc(assembler);

                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(value, vec0); // double to int

                        assembler.mov(ctx, threadPtr);
                        assembler.call((x86int_t) _BaseAssembler::jitCallDynamic);
                        assembler.cmp(tmp, 0);
                        Label ifTrue = assembler.newLabel();
                        assembler.je(ifTrue);
                        assembler.mov(ctx, ctxPtr);
                        assembler.call(tmp);
                        assembler.bind(ifTrue);
                        threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);
                        break;
                    }
                    case op_LOADVAL: { // registers[GET_Da(*pc)]=(sp--)->var;
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.sub(Lthread[thread_sp], (x86int_t)sizeof(StackElement));

                        assembler.mov(ctx, tmp);
                        assembler.movsd(vec0, Lstack_element[stack_element_var]);

                        movRegister(assembler, vec0, GET_Da(ir));
                        break;
                    }
                    case op_CHECKLEN: {
                        *
                         *
                CHECK_NULL2(
                        if((registers[GET_Da(*pc)]<o2->object->size) &&!(registers[GET_Da(*pc)]<0)) { _brh }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << registers[GET_Da(*pc)] << " size is " << o2->object->size;
                            throw Exception(Environment::IndexOutOfBoundsException, ss.str());
                        }
                )
                         *

                        checkO2(assembler, o2Ptr, lbl_thread_chk, i, true);
                        assembler.mov(fnPtr, Lsharp_object[sharp_object_size]);
                        assembler.mov(fnPtr32, fnPtr32);

                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(value, vec0); // double to int

                        Label isInvalid = assembler.newLabel(), end = assembler.newLabel();
                        assembler.cmp(value, 0);
                        assembler.jb(isInvalid);
                        assembler.cmp(value, fnPtr);
                        assembler.jae(isInvalid);

                        assembler.jmp(end);
                        assembler.bind(isInvalid);

                        assembler.mov(arg, i);
                        updatePc(assembler);

                        assembler.mov(ctx, fnPtr);
                        assembler.call((x86int_t) _BaseAssembler::jitIndexOutOfBoundsException);
                        assembler.mov(arg, -1);
                        assembler.jmp(lbl_thread_chk);
                        assembler.bind(end);
                        break;
                    }
                    case op_SHL: { // registers[*(pc+1)]=(int64_t)registers[GET_Ca(*pc)]<<(int64_t)registers[GET_Cb(*pc)];
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(tmp, vec0); // double to int
                        movRegister(assembler, vec1, GET_Cb(ir), false);
                        assembler.cvttsd2si(value, vec1); // double to int

                        assembler.mov(assembler.zcx(), value);
                        assembler.sal(tmp, x86::cl);

                        assembler.cvtsi2sd(vec0, tmp); // int to Double
                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_SHR: { // registers[*(pc+1)]=(int64_t)registers[GET_Ca(*pc)]>>(int64_t)registers[GET_Cb(*pc)];
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(tmp, vec0); // double to int
                        movRegister(assembler, vec1, GET_Cb(ir), false);
                        assembler.cvttsd2si(value, vec1); // double to int

                        assembler.mov(assembler.zcx(), value);
                        assembler.sar(tmp, x86::cl);

                        assembler.cvtsi2sd(vec0, tmp); // int to Double
                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, irTail);
                        break;
                    }
                    case op_TLS_MOVL: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_dataStack]);

                        if (GET_Da(ir) != 0) {
                            assembler.add(ctx, (x86int_t) (sizeof(StackElement) * GET_Da(ir)));
                        }

                        assembler.lea(value, Lstack_element[stack_element_object]);
                        assembler.mov(o2Ptr, value);
                        break;
                    }
                    case op_DUP: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_sp]);
                        assembler.lea(value, Lstack_element[stack_element_object]);

                        assembler.add(Lthread[thread_sp], (x86int_t) sizeof(StackElement));
                        assembler.add(ctx, (x86int_t) sizeof(StackElement)); // sp++
                        assembler.lea(ctx, Lstack_element[stack_element_object]);

                        assembler.call((x86int_t) _BaseAssembler::jitSetObject2);
                        break;
                    }
                    case op_POPOBJ_2: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.lea(value, ptr(tmp, (x86int_t) -sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], value); // sp--
                        assembler.mov(ctx, tmp);
                        assembler.lea(value, Lstack_element[stack_element_object]);
                        assembler.mov(o2Ptr, value);
                        break;
                    }
                    case op_SWAP: {
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.mov(value, Lthread[thread_dataStack]);

                        assembler.sub(tmp, value);
                        Label ifTrue = assembler.newLabel();
                        assembler.cmp(tmp, 2);
                        assembler.jae(ifTrue);

                        assembler.mov(arg, i);
                        updatePc(assembler);

                        assembler.mov(ctx, threadPtr);
                        assembler.call((x86int_t) _BaseAssembler::jitIllegalStackSwapException);
                        assembler.mov(arg, -1);
                        assembler.jmp(lbl_thread_chk);
                        assembler.bind(ifTrue);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(ctx, Lthread[thread_sp]);
                        assembler.lea(tmp, Lstack_element[stack_element_object]);
                        assembler.mov(fnPtr, qword_ptr(tmp));

                        assembler.sub(ctx, (x86int_t) sizeof(StackElement));
                        assembler.lea(value, Lstack_element[stack_element_object]);
                        assembler.mov(tmpInt, value);

                        assembler.mov(ctx, tmp);
                        assembler.call((x86int_t) _BaseAssembler::jitSetObject2);

                        assembler.mov(ctx, tmpInt);
                        assembler.mov(value, fnPtr);
                        assembler.call((x86int_t) _BaseAssembler::jitSetObject3);
                        break;
                    }
                    default: {
                        cout << "unknown opcode " << GET_OP(ir) << " please contact the developer immediately!" << endl;
                        error = jit_error_compile;
                        goto finish;
                    }
                }

                assembler.nop(); // instruction differentiation for now
            }
            */


            // end of function


            // Thread State Check // branching instruction
//            assembler.bind(lbl_thread_chk);                                     // we need to update the PC just before this addr jump as well as save the return back addr in fnPtr
//            Label isThreadKilled = assembler.newLabel();
//            Label hasException = assembler.newLabel();
//            Label thread_chk_end = assembler.newLabel();
//            Label ifFalse = assembler.newLabel();
//
//            assembler.mov(ctx, threadPtr);                                      // Suspend our thread?
//            assembler.mov(tmp32, Lthread[thread_signal]);
//            assembler.sar(tmp32, ((int)tsig_suspend));
//            assembler.and_(tmp32, 1);
//            assembler.test(tmp32, tmp32);
//            assembler.je(isThreadKilled);
//
//            assembler.call((x86int_t)Thread::suspendSelf);
//            assembler.bind(isThreadKilled);
//
//            assembler.mov(ctx, threadPtr);                                      // has it been shut down??
//            assembler.mov(tmp32, Lthread[thread_state]);
//            assembler.cmp(tmp32, THREAD_KILLED);
//            assembler.jne(hasException);
//            assembler.jmp(lbl_func_end); // verified
//
//            assembler.bind(hasException);
//            assembler.mov(ctx, threadPtr);                                    // Do we have an exception to catch?
//            assembler.mov(tmp32, Lthread[thread_signal]);
//            assembler.sar(tmp32, ((int)tsig_except));
//            assembler.and_(tmp32, 1);
//            assembler.test(tmp32, tmp32);
//            assembler.je(thread_chk_end);                                    // at this point no need to check any more events
//
//            updatePc(assembler);
//
//            assembler.mov(ctx, Lthread[thread_current]);
//            assembler.call((x86int_t)_BaseAssembler::jitTryCatch);
//
//            assembler.cmp(tmp, 1);
//            assembler.je(ifFalse);
//            assembler.jmp(lbl_func_end);
//            assembler.bind(ifFalse);
//
//            assembler.mov(ctx, threadPtr);
//            assembler.call((x86int_t)_BaseAssembler::jitGetPc);
//
//            assembler.mov(value, labelsPtr);                              // reset pc to find location in function to jump to
//            assembler.imul(tmp, (size_t)sizeof(x86int_t));
//            assembler.add(value, tmp);
//            assembler.mov(fnPtr, x86::ptr(value));
//            assembler.jmp(fnPtr);
//
//            assembler.bind(thread_chk_end);
//            assembler.jmp(fnPtr);                                         // dynamically jump to last address in out function
//
            error = createJitFunc();
            endCompilation();

            if(error == jit_error_ok && jitMemory != rt.allocator()->statistics()._reservedSize) {
                gc.freeMemory(jitMemory);

                jitMemory = rt.allocator()->statistics()._reservedSize;
                gc.addMemory(jitMemory);
            }

            cout << "used Size " << rt.allocator()->statistics()._usedSize << endl;
            cout << "reserved Size " << rt.allocator()->statistics()._reservedSize << endl;
            cout << "overhead Size " << rt.allocator()->statistics()._overheadSize << endl;
            return error;
        } else {
            error = jit_error_size;
        }
    } else {
        error = jit_error_compile;
    }

    finish:
    error = jit_error_compile;
    func->compiling = false;
    return error;
}


int _BaseAssembler::jitTryContextSwitch(Thread *thread, bool incPc) {
    if((thread->contextSwitching || thread->try_context_switch())) {
        if(incPc) thread->this_fiber->pc++;
        return 1;
    }

    return 0;
}

void _BaseAssembler::stackCheck(x86::Assembler &assembler, const Label &lbl_thread_chk, Int pc) {
//    assembler.mov(ctx, threadPtr);
//    assembler.mov(value, Lthread[thread_sp]);
//    assembler.mov(tmp, Lthread[thread_dataStack]);
//    assembler.mov(fnPtr, Lthread[thread_stack_lmt]);
//    assembler.sub(value, tmp);
//    assembler.sar(value, 4);
//    assembler.add(value, 1);
//    assembler.cmp(value, fnPtr);
//    Label end = assembler.newLabel();
//    assembler.jb(end);
//    assembler.mov(arg, pc);
//    updatePc(assembler);
//    assembler.call((x86int_t) jitStackOverflowException);
//    assembler.mov(arg, -1);
//    assembler.jmp(lbl_thread_chk);
//    assembler.bind(end);
}

void _BaseAssembler::checkO2(x86::Assembler &assembler, const x86::Mem &o2Ptr, const Label &lbl_thread_chk, Int pc, bool checkContents) {
    assembler.mov(ctx, o2Ptr);
    assembler.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = assembler.newLabel();
    Label nullCheckFailed = assembler.newLabel();
    assembler.je(nullCheckFailed);

    if(checkContents) {

        assembler.mov(ctx, qword_ptr(ctx));
        assembler.cmp(ctx, 0);
        assembler.jne(nullCheckPassed);
    } else {
        assembler.jmp(nullCheckPassed);
    }

    assembler.bind(nullCheckFailed);
    assembler.mov(arg, pc);
    updatePc(assembler);
    assembler.call((Int) jitNullPtrException);
    assembler.mov(arg, -1);
    assembler.jmp(lbl_thread_chk);
    assembler.bind(nullCheckPassed);
}

void _BaseAssembler::checkO2Node(x86::Assembler &assembler, const x86::Mem &o2Ptr, const Label &thread_check, Int pc) {
    assembler.mov(ctx, o2Ptr);
    assembler.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = assembler.newLabel();
    Label nullCheckFailed = assembler.newLabel();
    assembler.je(nullCheckFailed);

    assembler.mov(ctx, qword_ptr(ctx));
    assembler.cmp(ctx, 0);
    assembler.je(nullCheckFailed);

    assembler.mov(tmp32, Lsharp_object[sharp_object_info]);
    assembler.shr(tmp32, 24);
    assembler.mov(tmp32, tmp32);
    assembler.and_(tmp32, TYPE_MASK);
    assembler.cmp(tmp8, _stype_struct);
    assembler.jne(nullCheckFailed);

    assembler.mov(ctx, Lsharp_object[sharp_object_node]);
    assembler.jmp(nullCheckPassed);

    assembler.bind(nullCheckFailed);
    assembler.mov(arg, pc);
    updatePc(assembler);
    assembler.call((Int) jitNullPtrException);
    assembler.mov(arg, -1);
    assembler.jmp(thread_check);
    assembler.bind(nullCheckPassed);
}


void _BaseAssembler::checkO2Head(x86::Assembler &assembler, const x86::Mem &o2Ptr, const Label &thread_check, Int pc) {
    assembler.mov(ctx, o2Ptr);
    assembler.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = assembler.newLabel();
    Label nullCheckFailed = assembler.newLabel();
    assembler.je(nullCheckFailed);

    assembler.mov(ctx, qword_ptr(ctx));
    assembler.cmp(ctx, 0);
    assembler.je(nullCheckFailed);

    assembler.mov(tmp32, Lsharp_object[sharp_object_info]);
    assembler.shr(tmp32, 24);
    assembler.mov(tmp32, tmp32);
    assembler.and_(tmp32, TYPE_MASK);
    assembler.cmp(tmp8, _stype_var);
    assembler.jne(nullCheckFailed);

    assembler.mov(ctx, Lsharp_object[sharp_object_HEAD]);
    assembler.jmp(nullCheckPassed);

    assembler.bind(nullCheckFailed);
    assembler.mov(arg, pc);
    updatePc(assembler);
    assembler.call((Int) jitNullPtrException);
    assembler.mov(arg, -1);
    assembler.jmp(thread_check);
    assembler.bind(nullCheckPassed);
}

void _BaseAssembler::checkO2Object(x86::Assembler &assembler, const x86::Mem &o2Ptr, const Label &lbl_thread_check,
                                   Int pc) {
    assembler.mov(ctx, o2Ptr);
    assembler.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = assembler.newLabel();
    Label nullCheckFailed = assembler.newLabel();
    assembler.je(nullCheckFailed);

    assembler.mov(ctx, qword_ptr(ctx));
    assembler.cmp(ctx, 0);
    assembler.je(nullCheckFailed);
    assembler.jmp(nullCheckPassed);

    assembler.bind(nullCheckFailed);
    assembler.mov(arg, pc);
    updatePc(assembler);
    assembler.call((Int) jitNullPtrException);
    assembler.mov(arg, -1);
    assembler.jmp(lbl_thread_check);
    assembler.bind(nullCheckPassed);
}

/*
 * Very expensive procedure so use it sparingly
 */
void
_BaseAssembler::checkSystemState(const Label &lbl_func_end, Int pc, x86::Assembler &assembler, Label &lbl_thread_chk) {
    Label thread_check_end = assembler.newLabel();
    threadStatusCheck(assembler, thread_check_end, lbl_thread_chk, pc);
    assembler.bind(thread_check_end);
    checkMasterShutdown(assembler, pc, lbl_func_end);
}

void _BaseAssembler::jitCast(Object *obj, Int klass) {
//    try {
//        if(obj!=NULL) {
//            obj->castObject(klass);
//        } else {
//            Exception nptr(Environment::NullptrException, "");
//            __srt_cxx_prepare_throw(nptr);
//        }
//    }catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//    }
}

void _BaseAssembler::jitCastVar(Object *obj, int array) {
//    if(obj!=NULL && obj->object != NULL) {
//        if(TYPE(obj->object->info) != _stype_var) {
//            stringstream ss;
//            ss << "illegal cast to var" << (array ? "[]" : "");
//            Exception err(Environment::ClassCastException, ss.str());
//            __srt_cxx_prepare_throw(err);
//        }
//    } else {
//        Exception nptr(Environment::NullptrException, "");
//        __srt_cxx_prepare_throw(nptr);
//    }
}

fptr _BaseAssembler::jitCall(Thread *thread, int64_t addr) {
//    try {
//        if ((thread->calls + 1) >= thread->stackLimit) {
//            Exception e(Environment::StackOverflowErr, "");
//            __srt_cxx_prepare_throw(e);
//            return NULL;
//        }
//        return executeMethod(addr, thread, true);
//    } catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//    }

    return NULL;
}

fptr _BaseAssembler::jitCallDynamic(Thread *thread, int64_t addr) {

//    try {
//        if(addr <= 0 || addr >= manifest.methods) {
//            stringstream ss;
//            ss << "invalid call to pointer of " << addr;
//            Exception e(ss.str());
//            __srt_cxx_prepare_throw(e);
//        }
//        if ((thread->calls + 1) >= thread->stackLimit) {
//            Exception e(Environment::StackOverflowErr, "");
//            __srt_cxx_prepare_throw(e);
//        }
//
//        return executeMethod(addr, thread, true);
//    } catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//    }

    return NULL;
}

void _BaseAssembler::jitPut(int reg) {
    cout << registers[reg];
}

void _BaseAssembler::jitPutC(int reg) {
    printf("%c", (char)registers[reg]);
}

void _BaseAssembler::jitGet(int reg) {
//    if(registers[i64cmt])
//        registers[reg] = getche();
//    else
//        registers[reg] = getch();
}

void _BaseAssembler::jitInvokeDelegate(Int address, Int args, Thread* thread, Int staticAddr) {
    try {
        invokeDelegate(address, args, thread, staticAddr);
    } catch(Exception &e) {
        __srt_cxx_prepare_throw(e);
    }
}

// REMEMBER!!! dont forget to check state of used registers befote and after this call as they might be different than what they werr before
void _BaseAssembler::test(Int proc, Int xtra) {

//    cout << "op " << proc << " (sp) " << (long long)thread_self->sp << endl << std::flush;
//    cout << "(ebx) " << registers[i64ebx] << " (egx) " << registers[i64egx] << endl << std::flush;
}

// had a hard time with this one so will do this for now
void _BaseAssembler::jit64BitCast(Int dest, Int src) {
    registers[dest] = (int64_t)registers[src];
}

void _BaseAssembler::emitConstant(x86::Assembler &assembler, Constants &cpool, x86::Xmm xmm, double _const) {
    if(_const == 0) {
        assembler.pxor(xmm, xmm);
    } else { \
        Int idx = cpool.createConstant(assembler, _const);
        x86::Mem lconst = x86::ptr(cpool.getConstantLabel(idx));
        assembler.movsd(xmm, lconst);
    }
}

void _BaseAssembler::jmpToLabel(x86::Assembler &assembler, const x86::Gp &idx, const x86::Gp &dest, x86::Mem &labelsPtr) {
    using namespace asmjit::x86;

    assembler.mov(dest, labelsPtr);      // were just using these registers because we can, makes life so much easier
    assembler.cmp(idx, 0);
    Label ifTrue = assembler.newLabel();
    assembler.je(ifTrue);
    assembler.imul(idx, (size_t)sizeof(int32_t));      // offset = labelAddr*sizeof(int64_t)
    assembler.add(dest, idx);
    assembler.bind(ifTrue);

    assembler.mov(dest, x86::ptr(dest));
    assembler.jmp(dest);
}

void _BaseAssembler::movRegister(x86::Assembler &assembler, x86::Xmm &vec, Int addr, bool store) {
    assembler.mov(ctx, regPtr);        // move the contex var into register
    if(addr != 0) {
        assembler.add(ctx, (int64_t )(sizeof(double) * addr));
    }

    if(store)
        assembler.movsd(x86::qword_ptr(ctx), vec);  // store into register
    else
        assembler.movsd(vec, x86::qword_ptr(ctx)); // get value from register
}

void _BaseAssembler::checkMasterShutdown(x86::Assembler &assembler, int64_t pc, const Label &lbl_funcend) {
    using namespace asmjit::x86;

//    assembler.movzx(ctx32, (x86int_t)&masterShutdown);
//    assembler.cmp(ctx32, 0);
//    Label ifFalse = assembler.newLabel();
//    assembler.je(ifFalse);
//    assembler.mov(arg, pc);
//    assembler.jmp(lbl_funcend);
//
//    assembler.bind(ifFalse);
}

void _BaseAssembler::jitSysInt(Int signal) {
    try {
        VirtualMachine::sysInterrupt(signal);
    } catch(Exception &e) {
        __srt_cxx_prepare_throw(e);
    }
}

SharpObject* _BaseAssembler::jitNewObject(Int size) {
//    try {
//        return GarbageCollector::self->newObject(size);
//    } catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//        return NULL;
//    }

return NULL;
}

SharpObject* _BaseAssembler::jitNewObject2(Int size) {
//    try {
//        return GarbageCollector::self->newObjectArray(size);
//    } catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//        return NULL;
//    }
    return NULL;
}

SharpObject* _BaseAssembler::jitNewClass0(Int classid) {
//    try {
//        return GarbageCollector::self->newObject(&env->classes[classid]);
//    } catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//        return NULL;
//    }
return NULL;
}

SharpObject* _BaseAssembler::jitNewClass1(Int size, Int classid) {
//    try {
//        return GarbageCollector::self->newObjectArray(size, &env->classes[classid]);
//    } catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//        return NULL;
//    }
    return NULL;
}

void _BaseAssembler::jitNewString(Thread* thread, int64_t strid) {
//    try {
//        GarbageCollector::self->createStringArray(&(++thread->sp)->object,
//                                                  env->getStringById(strid));
//    } catch(Exception &e) {
//        __srt_cxx_prepare_throw(e);
//    }
}

void _BaseAssembler::jitPushNil(Thread* thread) {
//    GarbageCollector::self->releaseObject(&(++thread->sp)->object);
}

void _BaseAssembler::jitNullPtrException() {
//    Exception nptr(Environment::NullptrException, "");
//    __srt_cxx_prepare_throw(nptr);
}

void _BaseAssembler::jitStackOverflowException() {
//    Exception nptr(Environment::StackOverflowErr, "");
//    __srt_cxx_prepare_throw(nptr);
}

void _BaseAssembler::jitIndexOutOfBoundsException(Int size, Int index) {
//    stringstream ss;
//    ss << "Access to Object at: " << index << " size is " << size;
//    Exception outOfBounds(Environment::IndexOutOfBoundsException, ss.str());
//    __srt_cxx_prepare_throw(outOfBounds);
}

void _BaseAssembler::jitIllegalStackSwapException(Thread *thread) {
//    stringstream ss;
//    ss << "Illegal stack swap while sp is ( " << (x86int_t )(thread->sp-thread->dataStack) << ") ";
//    Exception outOfBounds(Environment::ThreadStackException, ss.str());
//    __srt_cxx_prepare_throw(outOfBounds);
}

void _BaseAssembler::jitThrow(Thread *thread) {
//    thread->exceptionObject = thread->sp->object;
//    Exception e("", false);
//    __srt_cxx_prepare_throw(e);
}

void _BaseAssembler::jitSetObject0(SharpObject* o, StackElement *sp) {
    sp->object = o;
}

void _BaseAssembler::jitSetObject1(StackElement *dest, StackElement *src) {
    dest->object = src->object;
}

void _BaseAssembler::jitSetObject2(Object *dest, Object *src) {
    *dest = src;
}

void _BaseAssembler::jitSetObject3(Object *dest, SharpObject *src) {
    *dest = src;
}

void _BaseAssembler::jitDelete(Object* o) {
//    GarbageCollector::self->releaseObject(o);
}

void _BaseAssembler::__srt_cxx_prepare_throw(Exception &e) {
//    Thread *self = thread_self;
//    self->throwable = e.getThrowable();
//    Object *eobj = &self->exceptionObject;
//
//    if(eobj->object == NULL) {
//        VirtualMachine::fillStackTrace(self->throwable.stackTrace);
//    } else {
//        VirtualMachine::fillStackTrace(eobj);
//        self->throwable.handlingClass = &env->classes[CLASS(eobj->object->info)];
//    }
//    sendSignal(self->signal, tsig_except, 1);
}

void
_BaseAssembler::threadStatusCheck(x86::Assembler &assembler, Label &retLbl, Label &lbl_thread_sec, Int irAddr) {
    assembler.lea(fnPtr, x86::ptr(retLbl)); // set return addr

    assembler.mov(arg, irAddr);             // set PC index
    assembler.mov(ctx, threadPtr);
    assembler.mov(ctx32, Lthread[thread_signal]);
    assembler.cmp(ctx32, 0);
    assembler.jne(lbl_thread_sec);
}

/**
 * You must pass the index of opcode you are processing in order to
 * successfully update the pc in thread.
 *
 * This will simply update the pc to whatever opcode index you specify.
 * @param assembler
 */
void _BaseAssembler::updatePc(x86::Assembler &assembler) {
//    Label end = assembler.newLabel();
//
//    assembler.cmp(arg, -1);
//    assembler.je(end);
//    assembler.mov(ctx, threadPtr);
//    assembler.mov(tmp, Lthread[thread_cache]);
//    assembler.imul(arg, (size_t)sizeof(x86int_t*));
//    assembler.add(tmp, arg);
//    assembler.mov(Lthread[thread_pc], tmp);
//    assembler.bind(end);
//    assembler.mov(arg, 0);
}

Int _BaseAssembler::jitGetPc(Thread *thread) {
    return 0;//thread->pc-thread->cache;
}

int _BaseAssembler::jitTryCatch(Method *method) {
    return 0; //vm->TryCatch(method, &thread_self->exceptionObject) ? 1 : 0;
}

FILE *_BaseAssembler::getLogFile() {
    ofstream outfile ("JIT.s");        // Quickly create file
    outfile.close();

    FILE * pFile;                           // out logging file
    pFile = fopen ("JIT.s" , "rw+");
    return pFile;
}

#endif