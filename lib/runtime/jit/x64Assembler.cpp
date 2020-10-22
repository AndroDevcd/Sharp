//
// Created by braxtonn on 8/23/2019.
//

#include "x64Assembler.h"
#include "../symbols/Method.h"
#include "../VirtualMachine.h"
#include "Jit.h"

#ifdef BUILD_JIT

using namespace asmjit;
using namespace asmjit::x86;

x86::Mem x64Assembler::getMemPtr(int64_t addr) {
    return x86::qword_ptr(ctx, addr);
}

x86::Mem x64Assembler::getMemPtr(x86::Gp reg, int64_t addr) {
    return x86::qword_ptr(reg, addr);
}

x86::Mem x64Assembler::getMemPtr(x86::Gp reg) {
    return x86::qword_ptr(reg);
}

int64_t x64Assembler::getRegisterSize() {
    return 8;
}

void x64Assembler::createFunctionAndAllocateRegisters() {
    compiler->addFunc(FuncSignatureT<void, jit_context*>());// Begin a function of `int fn(void)` signature.
    ctx = compiler->newI64("jctx");// Create `dst` register (destination pointer).

    compiler->setArg(0, ctx);                // Assign `jctx` argument.
    ctx32     = ctx.r32();
    tmp = compiler->newGpq();
    tmp32     = tmp.r32();
    tmp16     = tmp.r16();
    tmp8      = tmp.r8();
    fnPtr     = compiler->newI64();
    fnPtr32   = fnPtr.r32();
    arg       = compiler->newI64();
    regPtr    = compiler->newI64();
    threadPtr = compiler->newI64();
    fiberPtr  = compiler->newI64();
    arg2      = compiler->newI64();
    arg3      = compiler->newI64();
    arg4      = compiler->newI64();

    // floating point calculation regs
    vec0 = compiler->newXmm();
    vec1 = compiler->newXmm();

    ctxPtr = compiler->newInt64();              // store memory location of ctx pointer in the stack
    tmpPtr = compiler->newInt64();              // store memory location of o2 pointer in the stack
    tmpInt = compiler->newInt64();           // store memory location of tmiInt for temporary stored integers in the stack
    tmpPc = compiler->newInt64();           // store memory location of tmiInt for temporary stored integers in the stack
    returnAddress = compiler->newInt64();  // store memory location of return address for temporary storage to specify where to jump back from

    jumpAnnotationGraph = compiler->newJumpAnnotation();
}

void x64Assembler::createFunctionEpilogue() {
    compiler->bind(lendOfFunction);
    InvokeNode* invokeNode;
    compiler->invoke(&invokeNode,
               returnMethod,
              FuncSignatureT<bool, Thread*>());

    invokeNode->setArg(0, threadPtr);

    incPc();

    compiler->bind(lfunctionEpilogue);
    compiler->ret();
}

void x64Assembler::incPc() {
    compiler->mov(ctx, fiberPtr);                       // increment PC from thread
    compiler->add(Lfiber[fiber_pc], sizeof(opcode_instr));
}

void x64Assembler::beginCompilation(Method *method) {
    code = new CodeHolder();
    code->init(rt.environment());
    code->setErrorHandler(&errorHandler);

//    logger = new FileLogger(getLogFile());
//    logger->addFlags(FormatOptions::kFlagPositions);
//    logger->addFlags(FormatOptions::kFlagDebugPasses);
//    logger->addFlags(FormatOptions::kFlagAnnotations);
//    logger->addFlags(FormatOptions::kFlagDebugRA);
//    code->setLogger(logger);                           // Initialize logger temporarily to ensure quality of code

    compiler = new Compiler(code);
    constants = new Constants();
    compiledMethod = method;
    buildThreadSection = false;
    buildStackSection = false;
}

void x64Assembler::endCompilation() {
   compiledMethod->compiling = false;
   jumpAnnotationGraph = NULL;
   delete compiler; compiler = NULL;
   delete code; code = NULL;
   delete logger; logger = NULL;
   delete constants; constants = NULL;
   delete[] labels; labels = NULL;
}

void x64Assembler::logComment(std::string msg) {
    if(logger)
        logger->log(msg.c_str(), msg.size());
}

void x64Assembler::setupStackAndRegisterValues() {
    compiler->mov(ctxPtr, ctx);                           // send ctx to stack from ctx register via [ESP + paddr].

    // zero out registers & memory
    compiler->xor_(arg, arg);
    compiler->mov(tmpPc, 0);
    compiler->mov(tmpInt, 0);
    compiler->mov(returnAddress, 0);

    compiler->mov(fiberPtr, Ljit_context[jit_context_fiber]);
    compiler->mov(threadPtr, Ljit_context[jit_context_self]);
    compiler->mov(ctx, Ljit_context[jit_context_fiber]);
    compiler->mov(regPtr, Lfiber[fiber_regs]);
    compiler->lea(ctx, Lfiber[fiber_ptr]);
    compiler->mov(tmpPtr, ctx);
//        compiler->mov(ctx, tmpPtr);  //  how to assign pointer
//        compiler->mov(arg, 10);
//        compiler->mov(x86::ptr(ctx), arg);
}

void x64Assembler::setupGotoLabels() {
    labels = new Label[compiledMethod->cacheSize];                        // Each opcode has its own label but not all labels will be used
    for(Int i = 0; i < compiledMethod->cacheSize; i++) {       // Iterate through all the addresses to create labels for each address
        labels[i] = compiler->newLabel();
        jumpAnnotationGraph->addLabel(labels[i]);
    }
}

void x64Assembler::createFunctionLandmarks() {
    labelTable = compiler->newNamedLabel(".label_table", 12);
    lcodeStart = compiler->newNamedLabel(".code_start", 11);
    lendOfFunction = compiler->newNamedLabel(".func_end", 9);
    ldataSection = compiler->newNamedLabel(".data", 5);
    lsignalCheck = compiler->newNamedLabel(".thread_check", 13);
    lvirtualStackCheck = compiler->newNamedLabel(".virtual_stack_check", 20);
    lfunctionEpilogue = compiler->newNamedLabel(".function_epilogue", 18);
    lstackCheck = compiler->newNamedLabel(".stack_check", 12);
}

void x64Assembler::movConstToXmm(x86::Xmm xmm, double _const) {
    if(_const == 0) {
        compiler->pxor(xmm, xmm);
    } else {
        Int idx = constants->createConstant(*compiler, _const);
        x86::Mem lconst = x86::ptr(constants->getConstantLabel(idx));
        compiler->movsd(xmm, lconst);
    }
}

void x64Assembler::movRegister(x86::Xmm &vec, Int addr, bool store) {
    compiler->mov(ctx, regPtr);        // move the contex var into register
    if(addr != 0) {
        compiler->add(ctx, (int64_t )(sizeof(double) * addr));
    }

    if(store)
        compiler->movsd(x86::qword_ptr(ctx), vec);  // store into register
    else
        compiler->movsd(vec, x86::qword_ptr(ctx)); // get value from register
}

void x64Assembler::storeLabelValues() {
    compiler->align(kAlignData, 32);
    compiler->bind(labelTable);
    for (int64_t i = 0; i < compiledMethod->cacheSize; i++) {
        compiler->embedLabelDelta(labels[i], labelTable, 4);
    }
}

int x64Assembler::createJitFunc() {
    compiledMethod->compiling = false;

    storeLabelValues();
    compiler->endFunc();
    compiler->finalize();                    // Translate and assemble the whole 'cc' content.

    fptr fn;
    Error err = rt.add(&fn, code);   // Add the generated code to the runtime.
    if (err) {
        compiledMethod->isjit = false;
        cout << "jit code error " << err << " on function: " << compiledMethod->fullName.str() << endl;
        return jit_error_compile;
    }
    else {
        compiledMethod->isjit = true;
        compiledMethod->jit_func = fn;
        functions.push_back(fn);
        return jit_error_ok;
    }
}

//atomic<long> jjj = {0};
//extern void printRegs();
//void objCheck(Object* o, fiber* f) {
//    jjj++;
//    if(o == NULL || o->object == NULL) {
//        long kl = jjj.load();
//        int i = 0;
//    }
//}
//
//recursive_mutex printmut;
//void longsleep() {
//    GUARD(printmut);
//    cout << '[' << thread_self->id << ']' << thread_self->this_fiber->id << '-'
//    << (thread_self->this_fiber->fp-thread_self->this_fiber->dataStack)  << " $ "
//    << (thread_self->this_fiber->sp-thread_self->this_fiber->dataStack)
//            << (Int)(thread_self->this_fiber->ptr && thread_self->this_fiber->ptr->object ?
//                CLASS(thread_self->this_fiber->ptr->object->info) : 0) << " -- " << PC(thread_self) << endl;
////    Sleep(1);
//}
//
//void reopen() {
//    GUARD(printmut);
//    cout << '<' << thread_self->id << '>' << thread_self->this_fiber->id << '-'
//         << (thread_self->this_fiber->fp-thread_self->this_fiber->dataStack)  << " $ "
//         << (thread_self->this_fiber->sp-thread_self->this_fiber->dataStack)
//         << (Int)(thread_self->this_fiber->ptr && thread_self->this_fiber->ptr->object ?
//                  CLASS(thread_self->this_fiber->ptr->object->info) : 0) << " -- " << PC(thread_self) << endl;
//
////    Sleep(1);
//}

void numprint(Int num) {
    cout << num << endl;
}

void x64Assembler::validateVirtualStack() {
    Label contextSwitchCheck = compiler->newLabel();

    compiler->bind(lvirtualStackCheck); // it works we justb need to call the functions nessicary
    compiler->mov(ctx, threadPtr);
    compiler->movzx(tmp32, Lthread[thread_stack_rebuild]);
    compiler->movzx(tmp32, tmp8);
    compiler->cmp(tmp32, 0);
    compiler->je(lcodeStart);

    InvokeNode* invokeNode;
    compiler->invoke(&invokeNode,
                     shiftToNextMethod,
                     FuncSignatureT<fptr, Thread*, bool>());
    invokeNode->setArg(0, threadPtr);
    invokeNode->setArg(1, 1);
    invokeNode->setRet(0, tmp);

    compiler->cmp(tmp, 0);
    compiler->je(contextSwitchCheck);
    compiler->invoke(&invokeNode,
                     tmp,
                     FuncSignatureT<void, jit_context*>());
    invokeNode->setArg(0, ctxPtr);

    compiler->bind(contextSwitchCheck);
    compiler->mov(ctx, threadPtr);
    compiler->movzx(tmp32, Lthread[thread_context_switching]);
    compiler->movzx(tmp32, tmp8);
    compiler->cmp(tmp32, 0);
    compiler->je(lcodeStart);
    compiler->jmp(lfunctionEpilogue);
}

int x64Assembler::addUserCode() {
    InvokeNode* invokeNode;

    Label pcJump = compiler->newLabel();
    compiler->bind(lcodeStart);

    compiler->nop();
    compiler->nop();
    compiler->invoke(&invokeNode,
                     x64Assembler::getPc,
                     FuncSignatureT<Int, fiber*>());
    invokeNode->setArg(0, fiberPtr);
    invokeNode->setRet(0, tmp);

    jmpToLabel();

    int32_t Ir = 0, Ir2 = 0, result=0;
    for(Int i = 0; i < compiledMethod->cacheSize; i++) {
        Ir = compiledMethod->bytecode[i];
        if ((i + 1) < compiledMethod->cacheSize)
            Ir2 = compiledMethod->bytecode[i + 1];

        stringstream tmpStream;
        tmpStream << "; instr " << i;
        logComment(tmpStream.str());
        compiler->bind(labels[i]);

//        compiler->nop();
//        compiler->nop();
//        compiler->mov(ctx, i);
//        compiler->nop();
//        compiler->nop();
        if (GET_OP(Ir) == Opcode::JNE || GET_OP(Ir) == Opcode::JMP
            || GET_OP(Ir) == Opcode::BRH || GET_OP(Ir) == Opcode::IFE
            || GET_OP(Ir) == Opcode::IFNE) {
            compiler->mov(arg, i);
            updatePc();

            threadStatusCheck(labels[i], i, false);
        }

        switch (GET_OP(Ir)) {
            case Opcode::NOP: {
                compiler->nop();
                break;
            }
            case Opcode::INT: {
                compiler->mov(arg, i);
                updatePc();

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSysInt,
                                 FuncSignatureT<void, Int>());

                invokeNode->setArg(0, GET_Da(Ir));

                if(GET_Da(Ir) == OP_DELAY) {
                    compiler->jmp(lfunctionEpilogue);
                } else {
                    checkMasterShutdown(i);
                    Label continueLabel = compiler->newLabel();
                    threadStatusCheck(continueLabel, i, true);
                    compiler->bind(continueLabel);
                }
                break;
            }

            case Opcode::MOVI: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway

                movConstToXmm(vec0, Ir2);
                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::RET: {
                result = GET_Da(Ir);

                if (result == ERR_STATE) {
                    compiler->invoke(&invokeNode,
                                     popExceptionObject,
                                     FuncSignatureT<void, fiber*>());

                    invokeNode->setArg(0, fiberPtr);

                    compiler->invoke(&invokeNode,
                                     enableExceptionSignal,
                                     FuncSignatureT<void, Thread*>());

                    invokeNode->setArg(0, threadPtr);
                }

                compiler->jmp(lendOfFunction);
                break;
            }
            case Opcode::HLT: {
                compiler->invoke(&invokeNode,
                                 enableThreadKillSignal,
                                 FuncSignatureT<void, Thread*>());

                invokeNode->setArg(0, threadPtr);

                compiler->jmp(lendOfFunction);
                break;
            }
            case Opcode::NEWARRAY: {
                Label afterCheck = compiler->newLabel();
                Label pushObj = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                stackCheck(i, afterCheck);

                compiler->bind(afterCheck);
                movRegister(vec0, GET_Ca(Ir), false);
                compiler->cvttsd2si(ctx, vec0); // double to int
                compiler->mov(arg2, (Int)GET_Cb(Ir));

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitNewObject,
                                 FuncSignatureT<SharpObject*, Int, int>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                invokeNode->setRet(0, tmpInt);

                threadStatusCheck(pushObj, i,
                                  false); // TODO: should we modify this? its inefficent we could context switch after a new allocation

                compiler->bind(pushObj);
                compiler->mov(ctx, fiberPtr);
                compiler->mov(arg2, Lfiber[fiber_sp]);
                compiler->add(arg2, (Int)sizeof(StackElement));
                compiler->mov(Lfiber[fiber_sp], arg2);

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject0,
                                 FuncSignatureT<void, SharpObject*, StackElement*>());

                invokeNode->setArg(0, tmpInt);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::CAST: {
                Label end = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(arg2, vec0); // double to int
                compiler->mov(ctx, tmpPtr);
                compiler->mov(ctx, x86::ptr(ctx));

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitCast,
                                 FuncSignatureT<void, Object*, Int>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                threadStatusCheck(end, i, true);
                compiler->bind(end);
                break;
            }
            case Opcode::MOV8: {
                movRegister(vec0, GET_Cb(Ir), false);

                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->movsx(tmp32, tmp8);
                compiler->cvtsi2sd(vec0, tmp16);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOV16: {
                movRegister(vec0, GET_Cb(Ir), false);

                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->cwde(tmp);
                compiler->cvtsi2sd(vec0, tmp16);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOV32:
            case Opcode::MOV64: {
                movRegister(vec0, GET_Cb(Ir), false);

                if (GET_OP(Ir) == Opcode::MOV32) {
                    compiler->cvttsd2si(tmp, vec0); // double to int
                    compiler->mov(tmp, tmp32); // set lower 32 bits
                    compiler->cvtsi2sd(vec0, tmp32);
                }

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOVU8: {
                movRegister(vec0, GET_Cb(Ir), false);

                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->movzx(tmp, tmp8); // set lower 32 bits
                compiler->cvtsi2sd(vec0, tmp);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
                break;
            }
            case Opcode::MOVU16: {
                movRegister(vec0, GET_Cb(Ir), false);

                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->movzx(tmp, tmp16); // set lower 32 bits
                compiler->cvtsi2sd(vec0, tmp);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOVU32: {
                movRegister(vec0, GET_Cb(Ir), false);

                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->mov(ctx32, tmp32);
                compiler->cvtsi2sd(vec0, ctx);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOVU64: {
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jit64BitCast,
                                 FuncSignatureT<void, Int, Int>());

                invokeNode->setArg(0, GET_Ca(Ir));
                invokeNode->setArg(1, GET_Cb(Ir));
                break;
            }
            case Opcode::RSTORE: {
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);
                movRegister(vec0, GET_Da(Ir), false);

                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(ctx, Lfiber[fiber_sp]);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ADD: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                compiler->addsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::SUB: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                compiler->subsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::MUL: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                compiler->mulsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::DIV: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                compiler->divsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::MOD: {
                movRegister(vec0, GET_Ba(Ir), false);
                compiler->cvttsd2si(compiler->zax(), vec0); // double to int

                movRegister(vec1, GET_Bb(Ir), false);
                compiler->cvttsd2si(compiler->zcx(), vec1); // double to int

                compiler->cqo(compiler->zdx(), compiler->zax());
                compiler->idiv(compiler->zdx(), compiler->zdx(), compiler->zcx());
                compiler->cvtsi2sd(vec0, compiler->zdx());

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::IADD: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                compiler->addsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::ISUB: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                compiler->subsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::IMUL: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                compiler->mulsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::IDIV: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                compiler->divsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::IMOD: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(compiler->zax(), vec0); // double to int

                movConstToXmm(vec1, Ir2);
                compiler->cvttsd2si(compiler->zcx(), vec1); // double to int

                compiler->cqo(compiler->zdx(), compiler->zax());
                compiler->idiv(compiler->zdx(), compiler->zdx(), compiler->zcx());
                compiler->cvtsi2sd(vec0, compiler->zdx());

                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::POP: {
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitPop,
                                 FuncSignatureT<void, fiber*>());

                invokeNode->setArg(0, fiberPtr);
                break;
            }
            case Opcode::INC: {
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, 1);
                compiler->addsd(vec1, vec0);

                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::DEC: {
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, 1);
                compiler->subsd(vec0, vec1);

                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::MOVR: {
                movRegister(vec0, GET_Cb(Ir), false);
                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::BRH: {
                movRegister(vec0, ADX, false);
                compiler->cvttsd2si(tmp, vec0); // double to int
                jmpToLabel();
                break;
            }
            case Opcode::IFNE:
            case Opcode::IFE: {
                Label ifTrue = compiler->newLabel();
                movRegister(vec0, CMT, false);
                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->cmp(tmp, 0);
                if (GET_OP(Ir) == Opcode::IFE)
                    compiler->je(ifTrue);
                else
                    compiler->jne(ifTrue);

                movRegister(vec0, ADX, false);
                compiler->cvttsd2si(tmp, vec0); // double to int

                jmpToLabel();
                compiler->bind(ifTrue);
                break;
            }
            case Opcode::LT:
            case Opcode::GT:
            case Opcode::LTE:
            case Opcode::GTE: {
                movRegister((GET_OP(Ir) == Opcode::LT || GET_OP(Ir) == Opcode::LTE) ? vec0 : vec1, GET_Ca(Ir), false);
                movRegister((GET_OP(Ir) == Opcode::LT || GET_OP(Ir) == Opcode::LTE) ? vec1 : vec0, GET_Cb(Ir), false);
                compiler->comisd(vec0, vec1);

                Label ifFalse = compiler->newLabel();
                Label ifEnd = compiler->newLabel();
                if (GET_OP(Ir) == Opcode::LT || GET_OP(Ir) == Opcode::GT)
                    compiler->jae(ifFalse);
                else
                    compiler->ja(ifFalse);
                movConstToXmm(vec0, 1);
                compiler->jmp(ifEnd);
                compiler->bind(ifFalse);

                compiler->pxor(vec0, vec0);
                compiler->bind(ifEnd);

                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::MOVL: {
                compiler->mov(ctx, fiberPtr); // ctx->current
                compiler->mov(ctx, Lfiber[fiber_fp]); // ctx->current->fp

                if (GET_Da(Ir) != 0) {
                    compiler->add(ctx, (GET_Da(Ir) * sizeof(StackElement)));
                }

                compiler->lea(ctx, Lstack_element[stack_element_object]);
                compiler->mov(arg, tmpPtr);  //  how to assign pointer
                compiler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::MOVSL: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_sp]);

                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(Ir))); // sp+GET_DA(*pc)
                }

                compiler->lea(ctx, Lstack_element[stack_element_object]);
                compiler->mov(arg, tmpPtr);  //  how to assign pointer
                compiler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::SIZEOF: {
                compiler->mov(ctx, tmpPtr);
                compiler->mov(ctx, x86::ptr(ctx));
                compiler->cmp(ctx, 0);
                Label ifTrue = compiler->newLabel(), end = compiler->newLabel(), ifFalse = compiler->newLabel();
                compiler->jne(ifTrue);
                compiler->bind(ifFalse);
                movConstToXmm(vec0, 0);

                movRegister(vec0, GET_Da(Ir), true);
                compiler->jmp(end);
                compiler->bind(ifTrue);

                compiler->mov(ctx, qword_ptr(ctx));
                compiler->cmp(ctx, 0);
                compiler->je(ifFalse);

                compiler->mov(ctx, Lsharp_object[sharp_object_size]);
                compiler->mov(ctx32, ctx32);
                compiler->cvtsi2sd(vec0, ctx);
                movRegister(vec0, GET_Da(Ir), true);

                compiler->bind(end);
                break;
            }
            case Opcode::PUT: {
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitPut,
                                 FuncSignatureT<void, int, double*>());

                invokeNode->setArg(0, GET_Da(Ir));
                invokeNode->setArg(1, regPtr);
                break;
            }
            case Opcode::PUTC: {
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitPutC,
                                 FuncSignatureT<void, int, double*>());

                invokeNode->setArg(0, GET_Da(Ir));
                invokeNode->setArg(1, regPtr);
                break;
            }
            case Opcode::CHECKLEN: {
                checkTmpPtr(i);
                compiler->mov(fnPtr, Lsharp_object[sharp_object_size]);
                compiler->mov(fnPtr32, fnPtr32);

                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(arg2, vec0); // double to int

                Label isInvalid = compiler->newLabel(), end = compiler->newLabel();
                compiler->cmp(arg2, 0);
                compiler->jb(isInvalid);
                compiler->cmp(arg2, fnPtr);
                compiler->jae(isInvalid);

                compiler->jmp(end);
                compiler->bind(isInvalid);

                compiler->mov(arg, i);
                updatePc();
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitIndexOutOfBoundsException,
                                 FuncSignatureT<void, Int, Int, Thread*>());

                invokeNode->setArg(0, fnPtr);
                invokeNode->setArg(1, arg2);
                invokeNode->setArg(2, threadPtr);

                threadStatusCheck(end, i, true);
                compiler->bind(end);
                break;
            }
            case Opcode::JMP: {
                compiler->jmp(labels[GET_Da(Ir)]);
                break;
            }
            case Opcode::JNE:
            case Opcode::JE: {
                Label ifTrue = compiler->newLabel();
                movRegister(vec0, CMT, false);
                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->cmp(tmp, 0);
                if (GET_OP(Ir) == Opcode::JE)
                    compiler->je(ifTrue);
                else
                    compiler->jne(ifTrue);

                compiler->jmp(labels[GET_Da(Ir)]);
                compiler->bind(ifTrue);
                break;
            }
            case Opcode::LOADPC: {
                movConstToXmm(vec0, i);
                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::PUSHOBJ: {
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);
                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(ctx, Lfiber[fiber_sp]); // sp++
                compiler->lea(ctx, Lstack_element[stack_element_object]);

                compiler->mov(arg2, tmpPtr);
                compiler->mov(arg2, x86::ptr(arg2));

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject2,
                                 FuncSignatureT<void, Object*, Object*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::DEL: {
                compiler->mov(ctx, tmpPtr);
                compiler->mov(ctx, x86::ptr(ctx));

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitDelete,
                                 FuncSignatureT<void, Object*>());

                invokeNode->setArg(0, ctx);
                break;
            }
            case Opcode::CALL: {
                Label end = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitCall,
                                 FuncSignatureT<fptr, Thread*, int64_t>());

                invokeNode->setArg(0, threadPtr);
                invokeNode->setArg(1, GET_Da(Ir));
                invokeNode->setRet(0, tmp);

                compiler->cmp(tmp, 0);
                Label ifTrue = compiler->newLabel();
                compiler->je(end);

                compiler->invoke(&invokeNode,
                                 tmp,
                                 FuncSignatureT<void, jit_context*>());

                invokeNode->setArg(0, ctxPtr);
                compiler->bind(end);
                Label after = compiler->newLabel();
                threadStatusCheck(after, i, true);
                compiler->bind(after);
                break;
            }
            case Opcode::NEWCLASS: {
                compiler->mov(arg, i);
                updatePc();
                i++;

                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                Label afterCheck = compiler->newLabel();

                stackCheck(i-1, afterCheck);
                compiler->bind(afterCheck);

                Label pushObj = compiler->newLabel();
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitNewClass0,
                                 FuncSignatureT<SharpObject*, Int, Thread*>());

                invokeNode->setArg(0, Ir2);
                invokeNode->setArg(1, threadPtr);
                invokeNode->setRet(0, tmpInt);

                threadStatusCheck(pushObj, i-1,
                                  false);

                compiler->bind(pushObj);
                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(arg2, Lfiber[fiber_sp]); // sp++

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject0,
                                 FuncSignatureT<void, SharpObject*, StackElement*>());

                invokeNode->setArg(0, tmpInt);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::MOVN: {
                compiler->mov(arg, i);
                updatePc();
                i++;

                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                checkTmpPtrAsObject(i-1);

                if(Ir2 > 0)
                    compiler->add(ctx, (Ir2*sizeof(Object)));
                compiler->mov(arg, tmpPtr);  //  how to assign pointer
                compiler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::SLEEP: {
                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(ctx, vec0); // double to int

                compiler->invoke(&invokeNode,
                                 __os_sleep,
                                 FuncSignatureT<void, Int>());

                invokeNode->setArg(0, ctx);
                break;
            }
            case Opcode::TEST:
            case Opcode::TNE: {
                movRegister(vec1, GET_Ca(Ir), false);
                movRegister(vec0, GET_Cb(Ir), false);

                compiler->ucomisd(vec0, vec1);

                Label ifFalse = compiler->newLabel();
                Label ifEnd = compiler->newLabel();
                compiler->jp(ifFalse);

                compiler->ucomisd(vec0, vec1);
                if(GET_OP(Ir) == Opcode::TEST)
                    compiler->jne(ifFalse);
                else
                    compiler->je(ifFalse);
                movConstToXmm(vec0, 1);
                compiler->jmp(ifEnd);
                compiler->bind(ifFalse);

                compiler->pxor(vec0, vec0);
                compiler->bind(ifEnd);

                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::LOCK: {
                checkTmpPtrAsObject(i);

                compiler->mov(ctx, tmpPtr);
                compiler->mov(ctx, x86::ptr(ctx));

                compiler->invoke(&invokeNode,
                                 Object::monitorLock,
                                 FuncSignatureT<bool, Object*, Thread*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, threadPtr);
                invokeNode->setRet(0, tmp);

                Label skipCtxSwitch = compiler->newLabel();
                compiler->cmp(tmp32, 0);
                compiler->jne(skipCtxSwitch);
                compiler->mov(arg, i);
                updatePc();

                threadStatusCheck(skipCtxSwitch, i, true);
                compiler->bind(skipCtxSwitch);
                break;
            }
            case Opcode::ULOCK: {
                checkTmpPtrAsObject(i);

                compiler->mov(ctx, tmpPtr);
                compiler->mov(ctx, x86::ptr(ctx));
                compiler->invoke(&invokeNode,
                                 Object::monitorUnLock,
                                 FuncSignatureT<void, Object*>());

                invokeNode->setArg(0, ctx);
                break;
            }
            case Opcode::MOVG: {
                compiler->mov(arg, tmpPtr);  //  how to assign pointer
                compiler->mov(ctx, (Int)(vm.staticHeap+GET_Da(Ir)));  //  how to assign pointer
                compiler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::MOVND: {
                checkTmpPtrAsObject(i);
                compiler->mov(arg2, ctx);

                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(tmp, vec0); // double to int

                compiler->imul(tmp, (Int)sizeof(Object));
                compiler->add(arg2, tmp);

                compiler->mov(arg, tmpPtr);  //  how to assign pointer
                compiler->mov(x86::ptr(arg), arg2);
                break;
            }
            case Opcode::NEWOBJARRAY: {
                Label afterCheck = compiler->newLabel();
                Label pushObj = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitNewObject2,
                                 FuncSignatureT<SharpObject*, Int, Thread*>());

                invokeNode->setArg(0, GET_Da(Ir));
                invokeNode->setArg(1, threadPtr);
                invokeNode->setRet(0, tmpInt);

                threadStatusCheck(pushObj, i,
                                  false);

                compiler->bind(pushObj);
                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(arg2, Lfiber[fiber_sp]); // sp++

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject0,
                                 FuncSignatureT<void, SharpObject*, StackElement*>());

                invokeNode->setArg(0, tmpInt);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::NOT: {
                Label ifTrue = compiler->newLabel(), end = compiler->newLabel();
                movRegister(vec0, GET_Cb(Ir), false);
                compiler->cvttsd2si(tmp, vec0); // double to int

                compiler->cmp(tmp, 0);
                compiler->ja(ifTrue);
                compiler->mov(tmp, 1);
                compiler->jmp(end);

                compiler->bind(ifTrue);
                compiler->mov(tmp, 0);
                compiler->bind(end);

                compiler->cvtsi2sd(vec0, tmp); // int to Double
                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::SKIP: {
                if(GET_Da(Ir) > 0) {
                    compiler->jmp(labels[i+GET_Da(Ir)+1]);
                }
                break;
            }
            case Opcode::LOADVAL: {
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);

                compiler->mov(ctx, fiberPtr);
                compiler->mov(tmp, Lfiber[fiber_sp]); // sp--
                compiler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));

                compiler->mov(ctx, tmp);
                compiler->movsd(vec0, Lstack_element[stack_element_var]);

                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::SHR:
            case Opcode::SHL: {
                movRegister(vec0, GET_Ba(Ir), false);
                compiler->cvttsd2si(tmp, vec0); // double to int
                movRegister(vec1, GET_Bb(Ir), false);
                compiler->cvttsd2si(arg2, vec1); // double to int

                compiler->mov(compiler->zcx(), arg2);
                if(GET_OP(Ir) == Opcode::SHL)
                    compiler->sal(tmp, x86::cl);
                else
                    compiler->sar(tmp, x86::cl);

                compiler->cvtsi2sd(vec0, tmp); // int to Double
                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::SKNE:
            case Opcode::SKPE: {
                movRegister(vec0, GET_Ca(Ir), false);
                compiler->cvttsd2si(tmp, vec0); // double to int
                compiler->cmp(tmp, 0);
                Label ifEnd = compiler->newLabel();
                if(GET_OP(Ir) == Opcode::SKPE)
                    compiler->je(ifEnd);
                else
                    compiler->jne(ifEnd);
                compiler->jmp(labels[i+GET_Cb(Ir)]);

                compiler->bind(ifEnd);
                break;
            }
            case Opcode::AND: {
                Label ifFalse = compiler->newLabel();
                Label end = compiler->newLabel();
                movRegister(vec0, GET_Ca(Ir), false);
                compiler->cvttsd2si(tmp, vec0); // double to int
                movRegister(vec1, GET_Cb(Ir), false);
                compiler->cvttsd2si(arg2, vec0); // double to int

                compiler->cmp(tmp, 0);
                compiler->je(ifFalse);
                compiler->cmp(arg2, 0);
                compiler->je(ifFalse);
                movConstToXmm(vec0, 1);
                compiler->jmp(end);

                compiler->bind(ifFalse);
                movConstToXmm(vec0, 0);

                compiler->bind(end);
                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::UAND:
            case Opcode::OR:
            case Opcode::XOR: {
                movRegister(vec0, GET_Ca(Ir), false);
                compiler->cvttsd2si(tmp, vec0); // double to int
                movRegister(vec0, GET_Cb(Ir), false);
                compiler->cvttsd2si(ctx, vec0); // double to int
                if(GET_OP(Ir) == Opcode::UAND)
                    compiler->and_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::OR)
                    compiler->or_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::XOR)
                    compiler->xor_(tmp, ctx);

                compiler->cvtsi2sd(vec0, tmp); // int to Double
                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::THROW: { // untested from here
                Label end = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitThrow,
                                 FuncSignatureT<void, Thread*>());

                invokeNode->setArg(0, threadPtr);

                threadStatusCheck(end, i, true);
                compiler->bind(end);
                break;
            }
            case Opcode::CHECKNULL: {
                Label end = compiler->newLabel();

                compiler->mov(ctx, tmpPtr);
                compiler->mov(ctx, x86::ptr(ctx));
                compiler->cmp(ctx, 0);
                Label ifTrue = compiler->newLabel(), error = compiler->newLabel(), ifFalse = compiler->newLabel();
                compiler->jne(ifTrue);
                movConstToXmm(vec0, 1);
                compiler->jmp(end);

                compiler->bind(ifTrue);
                compiler->mov(ctx, qword_ptr(ctx));
                compiler->cmp(ctx, 0);
                compiler->jne(ifFalse);
                movConstToXmm(vec0, 1);
                compiler->jmp(end);
                compiler->bind(ifFalse);
                movConstToXmm(vec0, 0);
                compiler->bind(end);

                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::RETURNOBJ: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                compiler->lea(ctx, Lstack_element[stack_element_object]);

                compiler->mov(arg2, tmpPtr);
                compiler->mov(arg2, x86::ptr(arg2));

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject2,
                                 FuncSignatureT<void, Object*, Object*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::NEWCLASSARRAY: {
                compiler->mov(arg, i);
                updatePc();
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                Label afterCheck = compiler->newLabel();

                stackCheck(i-1, afterCheck);
                compiler->bind(afterCheck);

                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(ctx, vec0); // double to int

                Label pushObj = compiler->newLabel();
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitNewClass1,
                                 FuncSignatureT<SharpObject*, Int, Int, Thread*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, Ir2);
                invokeNode->setArg(2, threadPtr);
                invokeNode->setRet(0, tmpInt);

                threadStatusCheck(pushObj, i-1,
                                  false);

                compiler->bind(pushObj);
                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(arg2, Lfiber[fiber_sp]); // sp++

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject0,
                                 FuncSignatureT<void, SharpObject*, StackElement*>());

                invokeNode->setArg(0, tmpInt);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::NEWSTRING: {
                Label afterCheck = compiler->newLabel();
                Label end = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitNewString,
                                 FuncSignatureT<void, Thread*, int64_t>());

                invokeNode->setArg(0, threadPtr);
                invokeNode->setArg(1, GET_Da(Ir));

                threadStatusCheck(end, i,true);
                compiler->bind(end);
                break;
            }
            case Opcode::ADDL: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                compiler->mov(tmp, ctx);
                compiler->movsd(vec1, Lstack_element[stack_element_var]);

                movRegister(vec0, GET_Ca(Ir), false);
                compiler->addsd(vec0, vec1);
                compiler->mov(ctx, tmp);

                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::SUBL:
            case Opcode::MULL:
            case Opcode::DIVL: {
                compiler->mov(ctx, fiberPtr); // ctx->current
                compiler->mov(ctx, Lfiber[fiber_fp]); // ctx->current->fp
                if(GET_Cb(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                compiler->mov(tmp, ctx);
                compiler->movsd(vec0, Lstack_element[stack_element_var]);

                movRegister(vec1, GET_Ca(Ir), false);
                if(GET_OP(Ir) == Opcode::SUBL)
                    compiler->subsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::MULL)
                    compiler->mulsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::DIVL)
                    compiler->divsd(vec0, vec1);
                compiler->mov(ctx, tmp);

                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::MODL: {
                compiler->mov(ctx, fiberPtr); // ctx->current
                compiler->mov(ctx, Lfiber[fiber_fp]); // ctx->current->fp
                if(GET_Cb(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                compiler->mov(fnPtr, ctx);
                compiler->movsd(vec0, Lstack_element[stack_element_var]);
                compiler->cvttsd2si(compiler->zax(), vec0);

                movRegister(vec1, GET_Ca(Ir), false);
                compiler->cvttsd2si(compiler->zcx(), vec1);

                compiler->cqo(compiler->zdx(), compiler->zax());
                compiler->idiv(compiler->zdx(), compiler->zdx(), compiler->zcx());
                compiler->cvtsi2sd(vec0, compiler->zdx()); // int to Double

                compiler->mov(ctx, fnPtr);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::IADDL: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->movsd(vec0, Lstack_element[stack_element_var]);
                movConstToXmm(vec1, Ir2);
                compiler->addsd(vec0, vec1);

                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->movsd(vec0, Lstack_element[stack_element_var]);
                movConstToXmm(vec1, Ir2);
                if(GET_OP(Ir) == Opcode::ISUBL)
                    compiler->subsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::IMULL)
                    compiler->mulsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::IDIVL)
                    compiler->divsd(vec0, vec1);

                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::IMODL: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->mov(fnPtr, ctx);
                compiler->movsd(vec0, Lstack_element[stack_element_var]);
                compiler->cvttsd2si(compiler->zax(), vec0);

                compiler->mov(compiler->zcx(), Ir2);

                compiler->cqo(compiler->zdx(), compiler->zax());
                compiler->idiv(compiler->zdx(), compiler->zdx(), compiler->zcx());
                compiler->cvtsi2sd(vec0, compiler->zdx()); // int to Double

                compiler->mov(ctx, fnPtr);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::LOADL: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                compiler->movsd(vec0, Lstack_element[stack_element_var]);
                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::IALOAD: {
                movRegister(vec0, GET_Cb(Ir), false);
                checkTmpPtrAsNumber(i);
                compiler->cvttsd2si(tmp, vec0);

                compiler->imul(tmp, (Int)sizeof(double));
                compiler->add(ctx, tmp);
                compiler->movsd(vec0, x86::qword_ptr(ctx));
                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::POPOBJ: {
                checkTmpPtr(i, false);
                compiler->mov(arg3, ctx);

                compiler->mov(ctx, fiberPtr);
                compiler->mov(tmp, Lfiber[fiber_sp]); // sp--
                compiler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));

                compiler->mov(ctx, tmp);
                compiler->lea(arg2, Lstack_element[stack_element_object]);
                compiler->mov(ctx, arg3);

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject2,
                                 FuncSignatureT<void, Object*, Object*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::SMOVR: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_sp]);
                if(GET_Cb(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                compiler->mov(tmp, ctx);
                movRegister(vec0, GET_Ca(Ir), false);
                compiler->mov(ctx, tmp);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ANDL:
            case Opcode::ORL:
            case Opcode::XORL: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                compiler->mov(arg2, ctx);
                compiler->movsd(vec0, Lstack_element[stack_element_var]);
                compiler->cvttsd2si(tmp, vec0); // double to int

                movRegister(vec0, GET_Ca(Ir), false);
                compiler->cvttsd2si(ctx, vec0); // double to int
                if(GET_OP(Ir) == Opcode::ANDL)
                    compiler->and_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::ORL)
                    compiler->or_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::XORL)
                    compiler->xor_(tmp, ctx);

                compiler->cvtsi2sd(vec0, tmp); // int to Double
                compiler->mov(ctx, arg2);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::RETURNVAL: {
                movRegister(vec0, GET_Da(Ir), false);
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ISTORE: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);

                movConstToXmm(vec0, Ir2);

                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int)sizeof(StackElement));
                compiler->mov(ctx, Lfiber[fiber_sp]); // sp++
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::SMOVR_2: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                compiler->mov(tmp, ctx);
                movRegister(vec0, GET_Ca(Ir), false);
                compiler->mov(ctx, tmp);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ISTOREL: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway

                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                movConstToXmm(vec0, Ir2);
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::POPL: {
                compiler->mov(ctx, fiberPtr);

                compiler->mov(tmp, Lfiber[fiber_sp]);
                compiler->sub(Lfiber[fiber_sp], (Int)sizeof(StackElement));

                compiler->mov(arg2, tmp);

                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int)(sizeof(StackElement) * GET_Da(Ir))); // fp+GET_DA(*pc)
                }

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject1,
                                 FuncSignatureT<void, StackElement*, StackElement*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::PUSHNULL: {
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitPushNil,
                                 FuncSignatureT<void, Thread*>());

                invokeNode->setArg(0, threadPtr);
                break;
            }
            case Opcode::IPUSHL: {
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);

                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);

                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int)(sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->movsd(vec0, Lstack_element[stack_element_var]);

                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int)sizeof(StackElement));
                compiler->mov(ctx, Lfiber[fiber_sp]); // sp++
                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::PUSHL: {
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);

                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);

                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int)(sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->mov(arg2, ctx);

                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int)sizeof(StackElement));
                compiler->mov(ctx, Lfiber[fiber_sp]); // sp++

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject1,
                                 FuncSignatureT<void, StackElement*, StackElement*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::ITEST: { // untested
                Label afterCheck = compiler->newLabel();

                stackCheck(i, afterCheck);
                compiler->bind(afterCheck);
                compiler->mov(ctx, fiberPtr);
                compiler->mov(fnPtr, Lfiber[fiber_sp]); // sp--
                compiler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(ctx, fnPtr);
                compiler->lea(ctx, Lstack_element[stack_element_object]);
                compiler->mov(fnPtr, qword_ptr(ctx));

                compiler->mov(ctx, fiberPtr);
                compiler->mov(tmp, Lfiber[fiber_sp]); // sp--
                compiler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(ctx, tmp);
                compiler->lea(ctx, Lstack_element[stack_element_object]);
                compiler->mov(ctx, qword_ptr(ctx));

                compiler->cmp(fnPtr, ctx);
                Label ifTrue = compiler->newLabel(), end = compiler->newLabel();
                compiler->je(ifTrue);
                movConstToXmm(vec0, 0);
                compiler->jmp(end);

                compiler->bind(ifTrue);
                movConstToXmm(vec0, 1);

                compiler->bind(end);
                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::INVOKE_DELEGATE: {
                Label end = compiler->newLabel();
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitInvokeDelegate,
                                 FuncSignatureT<void, int64_t, int32_t, Thread*, bool>());

                invokeNode->setArg(0, GET_Da(Ir));
                invokeNode->setArg(1, GET_Cb(Ir2));
                invokeNode->setArg(2, threadPtr);
                invokeNode->setArg(3, (GET_Ca(Ir2) == 1 ? 1 : 0));

                threadStatusCheck(end, i-1, true);
                compiler->bind(end);
                break;
            }
            case Opcode::GET: {
                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitGet,
                                 FuncSignatureT<void, int64_t, int32_t, Thread*, bool>());

                invokeNode->setArg(0, GET_Da(Ir));
                break;
            }
            case Opcode::ISADD: {
                compiler->mov(ctx, fiberPtr); // ctx->current
                compiler->mov(ctx, Lfiber[fiber_sp]); // ctx->current->fp
                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->movsd(vec0, getMemPtr(ctx));

                movConstToXmm(vec1, Ir2);
                compiler->addsd(vec0, vec1);

                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::IPOPL: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(tmp, Lfiber[fiber_sp]); // sp--
                compiler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(ctx, tmp);
                compiler->movsd(vec0, Lstack_element[stack_element_var]);

                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_fp]);

                if(GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int)(sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::CMP: {
                i++;
                compiler->bind(labels[i]); // we wont use it but we need to bind it anyway


                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(tmp, vec0); // double to int

                compiler->mov(ctx, Ir2);
                compiler->cmp(tmp, ctx);
                Label ifFalse = compiler->newLabel(), end = compiler->newLabel();
                compiler->jne(ifFalse);
                movConstToXmm(vec0, 1);
                compiler->jmp(end);

                compiler->bind(ifFalse);
                movConstToXmm(vec0, 0);

                compiler->bind(end);
                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::CALLD: {

                Label end = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                movRegister(vec0, GET_Da(Ir), false);
                compiler->cvttsd2si(arg2, vec0); // double to int

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitCallDynamic,
                                 FuncSignatureT<fptr, Thread*, int64_t>());

                invokeNode->setArg(0, threadPtr);
                invokeNode->setArg(1, arg2);
                invokeNode->setRet(0, tmp);

                compiler->cmp(tmp, 0);
                Label ifTrue = compiler->newLabel();
                compiler->je(end);

                compiler->invoke(&invokeNode,
                                 tmp,
                                 FuncSignatureT<void, jit_context*>());

                invokeNode->setArg(0, ctxPtr);
                compiler->bind(end);
                Label after = compiler->newLabel();
                threadStatusCheck(after, i, true);
                compiler->bind(after);
                break;
            }
            case Opcode::DUP: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_sp]); // sp--
                compiler->lea(arg2, Lstack_element[stack_element_object]);

                compiler->mov(ctx, fiberPtr);
                compiler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(ctx, Lfiber[fiber_sp]); // sp--

                compiler->lea(ctx, Lstack_element[stack_element_object]);

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitSetObject2,
                                 FuncSignatureT<void, Object*, Object*>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, arg2);
                break;
            }
            case Opcode::POPOBJ_2: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(tmp, Lfiber[fiber_sp]); // sp--
                compiler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                compiler->mov(ctx, tmp);
                compiler->lea(ctx, Lstack_element[stack_element_object]);

                compiler->mov(arg, tmpPtr);  //  how to assign pointer
                compiler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::TLS_MOVL: {
                compiler->mov(ctx, fiberPtr);
                compiler->mov(ctx, Lfiber[fiber_dataStack]);

                if (GET_Da(Ir) != 0) {
                    compiler->add(ctx, (Int) (sizeof(StackElement) * GET_Da(Ir)));
                }

                compiler->lea(arg2, Lstack_element[stack_element_object]);

                compiler->mov(arg, tmpPtr);  //  how to assign pointer
                compiler->mov(x86::ptr(arg), arg2);
                break;
            }
            case Opcode::VARCAST: {
                Label end = compiler->newLabel();
                compiler->mov(arg, i);
                updatePc();

                compiler->mov(ctx, tmpPtr);
                compiler->mov(ctx, x86::ptr(ctx));

                compiler->invoke(&invokeNode,
                                 _BaseAssembler::jitCastVar,
                                 FuncSignatureT<void, Object*, int, int>());

                invokeNode->setArg(0, ctx);
                invokeNode->setArg(1, GET_Ca(Ir));
                invokeNode->setArg(2, GET_Cb(Ir));
                threadStatusCheck(end, i, true);
                compiler->bind(end);
                break;
            }
            default: {
//                cout << "unknown opcode " << GET_OP(ir) << " please contact the developer immediately!" << endl;
                return jit_error_compile;
            }
        }
    }

    return jit_error_ok;
}

void x64Assembler::checkTmpPtr(Int irAddr, bool accessInnerObj) {
    InvokeNode* invokeNode;

    compiler->mov(ctx, tmpPtr);
    compiler->mov(ctx, x86::ptr(ctx));
    compiler->cmp(ctx, 0);
    Label ifTrue = compiler->newLabel(), error = compiler->newLabel();
    compiler->jne(ifTrue);
    compiler->bind(error);
    compiler->mov(arg, irAddr);
    updatePc();
    compiler->invoke(&invokeNode,
                     _BaseAssembler::jitNullPtrException,
                     FuncSignatureT<void, Thread*>());

    invokeNode->setArg(0, threadPtr);

    threadStatusCheck(ifTrue, irAddr, true);
    compiler->bind(ifTrue);

    if(accessInnerObj) {
        Label ifFalse = compiler->newLabel();
        compiler->mov(ctx, qword_ptr(ctx));
        compiler->cmp(ctx, 0);
        compiler->jne(ifFalse);
        compiler->jmp(error);
        compiler->bind(ifFalse);
    }

}

void x64Assembler::checkTmpPtrAsObject(Int irAddr) {
    InvokeNode* invokeNode;

    compiler->mov(ctx, tmpPtr);
    compiler->mov(ctx, x86::ptr(ctx));
    compiler->cmp(ctx, 0);
    Label ifTrue = compiler->newLabel(),
    error = compiler->newLabel(),
    ifFalse = compiler->newLabel(),
    nodeIsGood = compiler->newLabel();

    compiler->jne(ifTrue);
    compiler->bind(error);
    compiler->mov(arg, irAddr);
    updatePc();
    compiler->invoke(&invokeNode,
                     _BaseAssembler::jitNullPtrException,
                     FuncSignatureT<void, Thread*>());

    invokeNode->setArg(0, threadPtr);
    threadStatusCheck(ifTrue, irAddr, true);
    compiler->bind(ifTrue);
    compiler->mov(ctx, qword_ptr(ctx));
    compiler->cmp(ctx, 0);
    compiler->jne(ifFalse);
    compiler->jmp(error);
    compiler->bind(ifFalse);
    compiler->mov(ctx, Lsharp_object[sharp_object_node]);
    compiler->cmp(ctx, 0);
    compiler->jne(nodeIsGood);
    compiler->jmp(error);
    compiler->bind(nodeIsGood);
}

void x64Assembler::checkTmpPtrAsNumber(Int irAddr) {
    InvokeNode* invokeNode;

    compiler->mov(ctx, tmpPtr);
    compiler->mov(ctx, x86::ptr(ctx));
    compiler->cmp(ctx, 0);
    Label ifTrue = compiler->newLabel(),
    error = compiler->newLabel(),
    ifFalse = compiler->newLabel(),
    headIsGood = compiler->newLabel();

    compiler->jne(ifTrue);
    compiler->bind(error);
    compiler->mov(arg, irAddr);
    updatePc();
    compiler->invoke(&invokeNode,
                     _BaseAssembler::jitNullPtrException,
                     FuncSignatureT<void, Thread*>());

    invokeNode->setArg(0, threadPtr);
    threadStatusCheck(ifTrue, irAddr, true);
    compiler->bind(ifTrue);
    compiler->mov(ctx, qword_ptr(ctx));
    compiler->cmp(ctx, 0);
    compiler->jne(ifFalse);
    compiler->jmp(error);
    compiler->bind(ifFalse);
    compiler->mov(ctx, Lsharp_object[sharp_object_HEAD]);
    compiler->cmp(ctx, 0);
    compiler->jne(headIsGood);
    compiler->jmp(error);
    compiler->bind(headIsGood);
}

/**
 * Requires:
 * tmp register to be set inotder to jump to address value dynamically
 */
void x64Assembler::jmpToLabel() {
    compiler->lea(arg2, x86::ptr(labelTable));
    if (compiler->is64Bit())
        compiler->movsxd(ctx, x86::dword_ptr(arg2, tmp.cloneAs(arg2), 2));
    else
        compiler->mov(ctx, x86::dword_ptr(arg2, tmp.cloneAs(arg2), 2));
    compiler->add(ctx, arg2);
    compiler->jmp(ctx, jumpAnnotationGraph);
}

/**
 * Requires:
 * arg register to be set to current PC or -1
 * fnPtr register to be set hold label address to jump bck to if all is good
 * arg3 register to hold whether or not to increment the PC in event of context switch
 */
void x64Assembler::addThreadSignalCheck() {                      // we need to update the PC just before this addr jump as well as save the return back addr in fnPtr

    if(buildThreadSection) {
        InvokeNode *invokeNode;

        Label isThreadKilled = compiler->newLabel();
        Label hasException = compiler->newLabel();
        Label hasKillSignal = compiler->newLabel();
        Label hasSuspend = compiler->newLabel();
        Label exceptionCaught = compiler->newLabel();
        Label contextSwitchCheck = compiler->newLabel();
        Label signalCheckEnd = compiler->newLabel();
        Label isContextSwitchEnabled = compiler->newLabel();

        /* Thread Suspended Check */
        compiler->bind(lsignalCheck);
        compiler->mov(ctx, threadPtr);
        compiler->movzx(tmp32, Lthread[thread_context_switching]);
        compiler->movzx(tmp32, tmp8);
        compiler->cmp(tmp32, 0);
        compiler->je(hasSuspend);
        compiler->jmp(lfunctionEpilogue);
        compiler->bind(hasSuspend);

        updatePc();                                                      // before we call we need to set arg register to -1 if we already have the pc updated
        compiler->mov(ctx, threadPtr);
        compiler->mov(tmp32, Lthread[thread_signal]);
        compiler->sar(tmp32, ((int) tsig_suspend));
        compiler->and_(tmp32, 1);
        compiler->test(tmp32, tmp32);
        compiler->je(isThreadKilled);
        compiler->invoke(&invokeNode,
                         Thread::suspendSelf,
                         FuncSignatureT<void>());

        /* end of check */

        /* Thread Killed Check */
        compiler->bind(isThreadKilled);
        compiler->mov(ctx, threadPtr);                                      // has it been shut down??
        compiler->mov(tmp32, Lthread[thread_state]);
        compiler->cmp(tmp32, THREAD_KILLED);
        compiler->jne(hasKillSignal);
        compiler->jmp(lendOfFunction); // verified
        /* end of check */

        /* Thread Killed Check */
        compiler->bind(hasKillSignal);
        compiler->mov(ctx, threadPtr);
        compiler->mov(tmp32, Lthread[thread_signal]);
        compiler->sar(tmp32, ((int) tsig_kill));
        compiler->and_(tmp32, 1);
        compiler->test(tmp32, tmp32);
        compiler->je(hasException);
        compiler->jmp(lendOfFunction); // verified
        /* end of check */

        /* Thread exception Check */
        compiler->bind(hasException);
        compiler->mov(ctx, threadPtr);                                    // Do we have an exception to catch?
        compiler->mov(tmp32, Lthread[thread_signal]);
        compiler->sar(tmp32, ((int) tsig_except));
        compiler->and_(tmp32, 1);
        compiler->test(tmp32, tmp32);
        compiler->je(contextSwitchCheck);
        compiler->invoke(&invokeNode,
                         VirtualMachine::catchException,
                         FuncSignatureT<bool>());
        invokeNode->setRet(0, tmp);

        compiler->cmp(tmp32, 1);
        compiler->je(exceptionCaught);
        compiler->jmp(lendOfFunction);
        compiler->bind(exceptionCaught);
        compiler->invoke(&invokeNode,
                         x64Assembler::getPc,
                         FuncSignatureT<Int, fiber *>());
        invokeNode->setArg(0, fiberPtr);
        invokeNode->setRet(0, tmp);

        jmpToLabel();
        /* end of check */

        /* Thread context switch Check */
        compiler->bind(contextSwitchCheck);
        compiler->mov(ctx, threadPtr);
        compiler->mov(tmp32, Lthread[thread_signal]);
        compiler->sar(tmp32, ((int) tsig_context_switch));
        compiler->and_(tmp32, 1);
        compiler->test(tmp32, tmp32);
        compiler->je(isContextSwitchEnabled);
        compiler->invoke(&invokeNode,
                         _BaseAssembler::jitTryContextSwitch,
                         FuncSignatureT<int, Thread *, bool>());
        invokeNode->setArg(0, threadPtr);
        invokeNode->setArg(1, tmpPc);
        invokeNode->setRet(0, tmp);

        compiler->cmp(tmp32, 0);
        compiler->je(isContextSwitchEnabled);
        compiler->jmp(lfunctionEpilogue);
        compiler->bind(isContextSwitchEnabled);

        compiler->jmp(fnPtr);
        /* end of check */
    }
}


void x64Assembler::checkMasterShutdown(int64_t pc) { // TODO: call this after functions as well in high and low level to protect against system shutdown
    using namespace asmjit::x86;

    compiler->movzx(tmp32, word_ptr((Int)&vm.state));
    compiler->cmp(tmp16, VM_TERMINATED);
    Label ifFalse = compiler->newLabel();
    compiler->jne(ifFalse);
    compiler->jmp(lfunctionEpilogue);

    compiler->bind(ifFalse);
}

/**
 * Requires:
 * arg register to be set to current PC or -1
 * fnPtr register to be set hold label address to jump bck to if all is good
 * arg3 register to hold whether or not to increment the PC in event of context switch
 */
void x64Assembler::threadStatusCheck(Label &retLbl, Int irAddr, bool incPc) {
    compiler->lea(fnPtr, x86::ptr(retLbl)); // set return addr
    compiler->mov(tmpPc, (incPc ? 1 : 0));

    compiler->mov(arg, irAddr);             // set PC index

    compiler->mov(ctx, threadPtr);
    compiler->mov(ctx32, Lthread[thread_signal]);
    compiler->cmp(ctx32, 0);
    compiler->jne(lsignalCheck);
    buildThreadSection = true;

}

void x64Assembler::threadStatusCheck(Label &retLbl, bool incPc) {
    compiler->lea(fnPtr, x86::ptr(retLbl)); // set return addr
    compiler->mov(arg4, (incPc ? 1 : 0));

    compiler->mov(arg, tmpInt);             // set PC index
    compiler->mov(ctx, threadPtr);
    compiler->mov(ctx32, Lthread[thread_signal]);
    compiler->cmp(ctx32, 0);
    compiler->jne(lsignalCheck);
    buildThreadSection = true;
}

void x64Assembler::addConstantsSection() {
    compiler->nop();
    compiler->nop();
    compiler->nop();
    compiler->align(kAlignData, 64);              // Align 64
    compiler->bind(ldataSection);                                  // emit constants to be used
    logComment("; data section start");
    constants->emitConstants(*compiler);
}

x86::Mem x64Assembler::getMemBytePtr(Int addr) {
    return x86::byte_ptr(ctx, addr);
}

void x64Assembler::updatePc() {
    Label end = compiler->newLabel();

    compiler->cmp(arg, -1);
    compiler->je(end);
    compiler->mov(ctx, fiberPtr);
    compiler->mov(tmp, Lfiber[fiber_cache]);
    compiler->sal(arg, 2);
    compiler->add(tmp, arg);
    compiler->mov(Lfiber[fiber_pc], tmp);
    compiler->bind(end);
    compiler->mov(arg, 0);
}

Int x64Assembler::getPc(fiber *fib) {
    return fib->pc-fib->cache;
}

void x64Assembler::popExceptionObject(fiber *fib) {
    fib->exceptionObject
            = (fib->sp--)->object.object;
}

void x64Assembler::enableExceptionSignal(Thread *thread) {
    GUARD(thread->mutex);
    sendSignal(thread->signal, tsig_except, 1);
}

void x64Assembler::enableThreadKillSignal(Thread *thread) {
    GUARD(thread->mutex);
    sendSignal(thread->signal, tsig_kill, 1);
}

/**
 * Requirement for branching to this section:
 * set tmpInt variabale to the current program counter value
 */
void x64Assembler::addStackCheck() {
    if(buildStackSection) {
        InvokeNode *invokeNode;

        Label overflowCheck = compiler->newLabel();
        Label underflowErr = compiler->newLabel();

        compiler->bind(lstackCheck);
        compiler->mov(ctx, fiberPtr);
        compiler->mov(arg2, Lfiber[fiber_sp]);
        compiler->mov(tmp, Lfiber[fiber_dataStack]);
        compiler->mov(fnPtr, Lfiber[fiber_stack_sz]);
        compiler->sub(arg2, tmp);
        compiler->sar(arg2, 4);
        compiler->add(arg2, 1);
        compiler->cmp(arg2, fnPtr);
        compiler->jb(overflowCheck);
        compiler->mov(arg, tmpInt);
        updatePc();
        compiler->invoke(&invokeNode,
                         growStack,
                         FuncSignatureT<void, fiber *>());
        invokeNode->setArg(0, fiberPtr);
        compiler->mov(arg, -1);
        threadStatusCheck(overflowCheck, false);

        compiler->bind(overflowCheck);
        //check for stack overflow
        compiler->mov(ctx, fiberPtr);
        compiler->mov(arg2, Lfiber[fiber_sp]);
        compiler->mov(tmp, Lfiber[fiber_dataStack]);
        compiler->mov(fnPtr, Lfiber[fiber_stack_lmt]);
        compiler->sub(arg2, tmp);
        compiler->sar(arg2, 4);
        compiler->add(arg2, 1);
        compiler->cmp(arg2, fnPtr);
        Label end = compiler->newLabel();
        compiler->jb(end);
        compiler->mov(arg, tmpInt);
        updatePc();
        compiler->invoke(&invokeNode,
                         jitStackOverflowException,
                         FuncSignatureT<void, Thread *>());
        invokeNode->setArg(0, threadPtr);
        compiler->mov(arg, -1);
        threadStatusCheck(end, false);
        compiler->bind(end);
        compiler->sub(arg2, 1);
        compiler->cmp(arg2, 0);
        compiler->jl(underflowErr);
        compiler->jmp(returnAddress);
        compiler->bind(underflowErr);
        compiler->mov(arg, tmpInt);
        updatePc();
        compiler->invoke(&invokeNode,
                         jitStackUnderflowException,
                         FuncSignatureT<void, Thread *>());
        invokeNode->setArg(0, threadPtr);
        compiler->mov(arg, -1);
        threadStatusCheck(end, false);
    }
}

void x64Assembler::stackCheck(Int pc, Label &returnAddr) {
    compiler->mov(tmpInt, pc);
    compiler->lea(ctx, x86::ptr(returnAddr));
    compiler->mov(returnAddress, ctx);
    compiler->jmp(lstackCheck);
    buildStackSection = true;
}

#endif
