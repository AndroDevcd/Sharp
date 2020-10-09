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

void x64Assembler::initializeRegisters() {
    /* x86 Windows standard convention is followed */
    ctx   = rcx;           // registers ctx, value, and tmp are volitle and must be stored on the stack if deemed to be preserved
    ctx32 = ecx;

    tmp       = rax;        // tmp acts as a return value from functions
    tmp32     = eax;
    tmp16     = ax;
    tmp8      = al;
    value     = rdx;        // value acts as the second argument for function params
    fnPtr     = r12;       // registers fnPtr, arg, regPtr, & threadPtr are non volitile and do not have to be saved
    fnPtr32   = r12d;
    arg       = r13;
    regPtr    = r14;
    threadPtr = r15;
    fiberPtr  = rdi;
    arg3      = r8;        // acts as a temporary and 3rd argument for calling functions
    arg4      = r9;


    // stack manip registers
    bp = rbp;
    sp = rsp;

    // floating point calculation regs
    vec0 = xmm0;
    vec1 = xmm1;

    /* Argument registers are as follows for Windows: (ctx, value, fnArg3, fnArg4) */
}

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

void x64Assembler::createFunctionPrologue() {
    if(OS_id==win_os) {
        assembler->push(bp);
        assembler->mov(bp, sp);

        assembler->push(arg3);                          // Store used registers (windows x86 convention)
        assembler->push(arg4);                          // Store used registers (windows x86 convention)
        assembler->push(fnPtr);
        assembler->push(arg);
        assembler->push(regPtr);
        assembler->push(threadPtr);
    } else {

    }
}

void x64Assembler::createFunctionEpilogue() {
    if(OS_id==win_os) {
        assembler->bind(lendOfFunction);
        assembler->mov(ctx, threadPtr);
        assembler->call((int64_t) returnMethod);               // we need to update the PC just before this call
        incPc();

        assembler->bind(lfunctionEpilogue);
        assembler->add(sp, (stackSize));
        assembler->pop(threadPtr);
        assembler->pop(regPtr);
        assembler->pop(arg);
        assembler->pop(fnPtr);
        assembler->pop(arg4);
        assembler->pop(arg3);
        assembler->pop(bp);
        assembler->ret();
    }
}

void x64Assembler::incPc() {
    if(OS_id==win_os) {
        assembler->mov(ctx, fiberPtr);                       // increment PC from thread
        assembler->mov(tmp, Lfiber[fiber_pc]);
        assembler->lea(tmp, x86::ptr(tmp, sizeof(int32_t)));
        assembler->add(tmp, sizeof(int32_t));
        assembler->mov(Lfiber[fiber_pc], tmp);
    }
}


void x64Assembler::beginCompilation(Method *method) {
    code = new CodeHolder();
    code->init(rt.codeInfo());

    logger = new FileLogger(getLogFile());
    code->setLogger(logger);                           // Initialize logger temporarily to ensure quality of code

    assembler = new x86::Assembler(code);              // Create and attach x86::Assembler to `code`.
    constants = new Constants();
    compiledMethod = method;
}

void x64Assembler::endCompilation() {
   compiledMethod->compiling = false;
   delete assembler; assembler = NULL;
   delete code; code = NULL;
   delete logger; logger = NULL;
   delete constants; constants = NULL;
   delete[] labels; labels = NULL;
}

void x64Assembler::logComment(std::string msg) {
    assembler->comment(msg.c_str(), msg.size());
}

void x64Assembler::setupStackAndRegisterValues() {
    if(OS_id==win_os) {
        assembler->mov(ctxPtr, ctx);                           // send ctx to stack from ctx register via [ESP + paddr].

        // zero out registers & memory
        assembler->xor_(arg, arg);
        assembler->mov(tmpPtr, 0);
        assembler->mov(labelsPtr, 0);
        assembler->mov(tmpInt, 0);

        assembler->mov(fiberPtr, Ljit_context[jit_context_fiber]);
        assembler->mov(threadPtr, Ljit_context[jit_context_self]);
        assembler->mov(fnPtr, Ljit_context[jit_context_starting_pc]);
        assembler->mov(ctx, Ljit_context[jit_context_fiber]);
        assembler->mov(regPtr, Lfiber[fiber_regs]);
    }
}

void x64Assembler::allocateStackSpace() {
    if(OS_id==win_os) {
        // allocate space for the stack
        int64_t storedRegs = getRegisterSize() * 6;
        int ptrSize = sizeof(jit_context *), paddr = storedRegs + ptrSize;
        int labelsSize = sizeof(int64_t *), laddr = paddr + labelsSize;
        int tmpPtrSize = sizeof(Object *), o2addr = laddr + tmpPtrSize;
        int tmpIntSize = sizeof(int64_t), tmpIntaddr = o2addr + tmpIntSize; // NOTE: make sure the stack is alligned to 16 bits if I add or subtract a stack variable
        int returnAddressSize = sizeof(int64_t), returnAddressaddr = tmpIntaddr + returnAddressSize; // NOTE: make sure the stack is alligned to 16 bits if I add or subtract a stack variable
        stackSize = ptrSize + labelsSize + tmpPtrSize + tmpIntSize + returnAddressSize + sizeof(int64_t);
        assembler->sub(sp, (stackSize));

        ctxPtr = getMemPtr(bp, -(paddr));              // store memory location of ctx pointer in the stack
        labelsPtr = getMemPtr(bp, -(laddr));           // store memory location of labels* pointer in the stack
        tmpPtr = getMemPtr(bp, -(o2addr));              // store memory location of o2 pointer in the stack
        tmpInt = getMemPtr(bp, -(tmpIntaddr));           // store memory location of tmiInt for temporary stored integers in the stack
        returnAddress = getMemPtr(bp, -(returnAddressaddr));  // store memory location of return address for temporary storage to specify where to jump back from
    }
}

void x64Assembler::setupGotoLabels() {
    labels = new Label[compiledMethod->cacheSize];                        // Each opcode has its own label but not all labels will be used
    for(Int i = 0; i < compiledMethod->cacheSize; i++) {       // Iterate through all the addresses to create labels for each address
        labels[i] = assembler->newLabel();
    }
}

void x64Assembler::createFunctionLandmarks() {
    lcodeStart = assembler->newNamedLabel(".code_start", 11);
    lsetupAddressTable = assembler->newNamedLabel(".init_addr_tbl", 14);
    lendOfFunction = assembler->newNamedLabel(".func_end", 9);
    ldataSection = assembler->newNamedLabel(".data", 5);
    lsignalCheck = assembler->newNamedLabel(".thread_check", 13);
    lvirtualStackCheck = assembler->newNamedLabel(".virtual_stack_check", 20);
    lfunctionEpilogue = assembler->newNamedLabel(".function_epilogue", 18);
    lstackCheck = assembler->newNamedLabel(".stack_check", 12);
}

void x64Assembler::movConstToXmm(x86::Xmm xmm, double _const) {
    if(_const == 0) {
        assembler->pxor(xmm, xmm);
    } else { \
        Int idx = constants->createConstant(*assembler, _const);
        x86::Mem lconst = x86::ptr(constants->getConstantLabel(idx));
        assembler->movsd(xmm, lconst);
    }
}

void x64Assembler::movRegister(x86::Xmm &vec, Int addr, bool store) {
    assembler->mov(ctx, regPtr);        // move the contex var into register
    if(addr != 0) {
        assembler->add(ctx, (int64_t )(sizeof(double) * addr));
    }

    if(store)
        assembler->movsd(x86::qword_ptr(ctx), vec);  // store into register
    else
        assembler->movsd(vec, x86::qword_ptr(ctx)); // get value from register
}

void x64Assembler::setupAddressTable() {
    if(OS_id==win_os) {
        assembler->nop();
        assembler->mov(ctx, ctxPtr);
        assembler->mov(ctx,
                       Ljit_context[jit_context_caller]);              // First we gain access to the int32_t* jit_labels; field
        assembler->mov(ctx, Lmethod[method_jit_labels]);
        assembler->mov(labelsPtr, ctx);

        // Next we need to see if we need to jump to setup all the address labels
        assembler->mov(ctx, getMemPtr(ctx, 0));                    // if(ctx->func->jit_labels[0]==0)
        assembler->test(ctx, ctx);                                              //      goto setupAddresses;
        assembler->jne(lvirtualStackCheck);
        assembler->jmp(lsetupAddressTable);
    }
}

void x64Assembler::storeLabelValues() {
    if(OS_id==win_os) {
        assembler->bind(lsetupAddressTable);
        assembler->nop();

        // labeles[] setting here
        logComment("; setting label values");
        assembler->mov(tmp, labelsPtr);

        x86::Mem ptrIdx = getMemPtr(tmp);
        x86::Mem lbl;
        for (int64_t i = 0; i < compiledMethod->cacheSize; i++) {
            lbl = x86::ptr(labels[i]);
            assembler->lea(ctx, lbl);
            assembler->mov(ptrIdx, ctx);

            if ((i + 1) < compiledMethod->cacheSize)                       // omit unessicary add instruction
                assembler->add(tmp, (int32_t) sizeof(int32_t));
        }

        assembler->nop();
        assembler->jmp(lvirtualStackCheck);                          // jump back to top to execute user code
    }
}

int x64Assembler::createJitFunc() {
    compiledMethod->compiling = false;

    fptr fn;
    Error err = rt.add(&fn, code);   // Add the generated code to the runtime.
    if (err) {
        cout << "jit code error " << err << endl;
        return jit_error_compile;
    }
    else {
        compiledMethod->isjit = true;
        compiledMethod->jit_func = fn;
        functions.push_back(fn);
        return jit_error_ok;
    }
}

void x64Assembler::validateVirtualStack() {
    if(OS_id==win_os) {
        assembler->bind(lvirtualStackCheck);
        assembler->mov(ctx, threadPtr);
        assembler->movzx(tmp32, Lthread[thread_stack_rebuild]);
        assembler->movzx(tmp32, tmp8);
        assembler->cmp(tmp32, 1);
        assembler->jne(lcodeStart);
        assembler->mov(ctx, threadPtr);
        assembler->mov(value, 1);
        assembler->call((int64_t) shiftToNextMethod);
        assembler->cmp(tmp, 0);
        assembler->je(lcodeStart);
        assembler->mov(ctx, ctxPtr);
        assembler->call(tmp);
    }
}

void x64Assembler::addUserCode() {
    Label opcodeStart = assembler->newNamedLabel(".opcode_start", 13);
    Label checkStatus = assembler->newLabel();

    if(OS_id==win_os) {
        assembler->bind(lcodeStart);
        assembler->test(fnPtr, fnPtr);
        assembler->jle(checkStatus);
        assembler->mov(ctx, labelsPtr);
        assembler->imul(fnPtr, (int64_t)sizeof(int32_t));
        assembler->add(ctx, fnPtr);
        assembler->mov(ctx, qword_ptr(ctx));
        assembler->jmp(ctx);
        assembler->bind(checkStatus);
        threadStatusCheck(opcodeStart, 0, false);
        assembler->bind(opcodeStart);

        Int Ir = 0, Ir2 = 0, result=0;
        for(Int i = 0; i < compiledMethod->cacheSize; i++) {
            Ir = compiledMethod->bytecode[i];
            if((i+1) < compiledMethod->cacheSize)
                Ir2 = compiledMethod->bytecode[i+1];

            if(GET_OP(Ir) == Opcode::JNE || GET_OP(Ir) == Opcode::JMP
             || GET_OP(Ir) == Opcode::BRH || GET_OP(Ir) == Opcode::IFE
             || GET_OP(Ir) == Opcode::IFNE) {
                threadStatusCheck(labels[i], i, false);
            }

            stringstream tmpStream;
            tmpStream <<  "; instr " << i;
            logComment(tmpStream.str());
            assembler->bind(labels[i]);

            switch(GET_OP(Ir)) {
                case Opcode::NOP: {
                    assembler->nop();
                    break;
                }
                case Opcode::INT: {
                    assembler->mov(ctx, GET_Da(Ir));
                    assembler->call((Int)_BaseAssembler::jitSysInt);
                    checkMasterShutdown(i);
                    Label continueLabel = assembler->newLabel();
                    threadStatusCheck(continueLabel, i, true);
                    break;
                }

                case Opcode::MOVI: {
                    Int num = Ir2;
                    i++; assembler->bind(labels[i]); // we wont use it but we need to bind it anyway

                    movConstToXmm(vec0, num);
                    movRegister(vec0, GET_Da(Ir), true);
                    break;
                }
                case Opcode::RET: {
                    result = GET_Da(Ir);

                    if(result == ERR_STATE) {
                       assembler->mov(ctx, fiberPtr);
                       assembler->call((Int)popExceptionObject);
                    }

                    assembler->mov(ctx, threadPtr);
                    assembler->call((int64_t) returnMethod);
                    incPc();

                    if(result == ERR_STATE) {
                        assembler->mov(ctx, threadPtr);
                        assembler->call((Int)enableExceptionSignal);
                    }

                    assembler->jmp(lfunctionEpilogue);
                    break;
                }
                case Opcode::HLT: {
                    assembler->mov(ctx, threadPtr);
                    assembler->call((Int)enableThreadKillSignal);
                    break;
                }
                case Opcode::NEWARRAY: {
                    Label afterCheck = assembler->newLabel();
                    Label pushObj = assembler->newLabel();
                    stackCheck(i, afterCheck);

                    assembler->bind(afterCheck);
                    movRegister(vec0, GET_Ca(Ir), false);
                    assembler->cvttsd2si(ctx, vec0); // double to int
                    assembler->mov(value, GET_Cb(Ir));
                    assembler->call((Int)_BaseAssembler::jitNewObject);
                    assembler->mov(tmpInt, tmp);

                    threadStatusCheck(pushObj, i, false);

                    assembler->bind(pushObj);
                    assembler->mov(ctx, fiberPtr);
                    assembler->mov(value, Lfiber[fiber_sp]);
                    assembler->lea(value, x86::ptr(value, sizeof(StackElement)));
                    assembler->mov(Lfiber[fiber_sp], value);

                    assembler->mov(ctx, tmpInt);
                    assembler->call((Int)_BaseAssembler::jitSetObject0);
                    break;
                }
            }
        }

        // TODO: add for loop for all opcodes
        assembler->jmp(lendOfFunction);                  // if we reach the end of our function we dont want to set the labels again
    }
}

/**
 * Requires:
 * arg register to be set to current PC or -1
 * fnPtr register to be set hold label address to jump bck to if all is good
 * arg3 register to hold whether or not to increment the PC in event of context switch
 */
void x64Assembler::addThreadSignalCheck() {                      // we need to update the PC just before this addr jump as well as save the return back addr in fnPtr
    Label isThreadKilled = assembler->newLabel();
    Label hasException = assembler->newLabel();
    Label hasKillSignal = assembler->newLabel();
    Label exceptionCaught = assembler->newLabel();
    Label contextSwitchCheck = assembler->newLabel();
    Label signalCheckEnd = assembler->newLabel();
    Label isContextSwitchEnabled = assembler->newLabel();

    if(OS_id==win_os) {

        /* Thread Suspended Check */
        assembler->bind(lsignalCheck);
        assembler->mov(ctx, threadPtr);
        assembler->mov(tmp32, Lthread[thread_signal]);
        assembler->sar(tmp32, ((int)tsig_suspend));
        assembler->and_(tmp32, 1);
        assembler->test(tmp32, tmp32);
        assembler->je(isThreadKilled);
        assembler->call((Int)Thread::suspendSelf);
        /* end of check */

        /* Thread Killed Check */
        assembler->bind(isThreadKilled);
        assembler->mov(ctx, threadPtr);                                      // has it been shut down??
        assembler->mov(tmp32, Lthread[thread_state]);
        assembler->cmp(tmp32, THREAD_KILLED);
        assembler->jne(hasKillSignal);
        assembler->jmp(lendOfFunction); // verified
        /* end of check */

        /* Thread Killed Check */
        assembler->bind(hasKillSignal);
        assembler->mov(ctx, threadPtr);
        assembler->mov(tmp32, Lthread[thread_signal]);
        assembler->sar(tmp32, ((int)tsig_kill));
        assembler->and_(tmp32, 1);
        assembler->test(tmp32, tmp32);
        assembler->je(hasException);
        assembler->jmp(lendOfFunction); // verified
        /* end of check */

        /* Thread exception Check */
        assembler->bind(hasException);
        assembler->mov(ctx, threadPtr);                                    // Do we have an exception to catch?
        assembler->mov(tmp32, Lthread[thread_signal]);
        assembler->sar(tmp32, ((int)tsig_except));
        assembler->and_(tmp32, 1);
        assembler->test(tmp32, tmp32);
        assembler->je(contextSwitchCheck);

        updatePc();                                                      // before we call we need to set arg register to -1 if we already have the pc updated

        assembler->call((Int)VirtualMachine::catchException);

        assembler->cmp(tmp32, 1);
        assembler->je(exceptionCaught);
        assembler->jmp(lendOfFunction);
        assembler->bind(exceptionCaught);

        assembler->mov(ctx, fiberPtr);
        assembler->call((Int)x64Assembler::getPc);

        assembler->mov(value, labelsPtr);                              // reset pc to find location in function to jump to
        assembler->imul(tmp, (size_t)sizeof(int32_t));
        assembler->add(value, tmp);
        assembler->mov(fnPtr, x86::ptr(value));
        assembler->jmp(fnPtr);
        /* end of check */

        /* Thread context switch Check */
        assembler->bind(contextSwitchCheck);
        assembler->mov(ctx, threadPtr);
        assembler->mov(tmp32, Lthread[thread_signal]);
        assembler->sar(tmp32, ((int)tsig_context_switch));
        assembler->and_(tmp32, 1);
        assembler->test(tmp32, tmp32);
        assembler->je(isContextSwitchEnabled);
        assembler->mov(ctx, threadPtr);
        assembler->mov(value, arg4);
        assembler->call((Int)_BaseAssembler::jitTryContextSwitch);

        assembler->cmp(tmp32, 0);
        assembler->je(isContextSwitchEnabled);
        assembler->jmp(lfunctionEpilogue);
        assembler->bind(isContextSwitchEnabled);

        assembler->mov(ctx, threadPtr);
        assembler->movzx(tmp32, Lthread[thread_context_switching]);
        assembler->movzx(tmp32, tmp8);
        assembler->cmp(tmp32, 1);
        assembler->jne(signalCheckEnd);
        assembler->jmp(lfunctionEpilogue);

        assembler->bind(signalCheckEnd);
        assembler->jmp(fnPtr);
        /* end of check */
    }
}


void x64Assembler::checkMasterShutdown(int64_t pc) {
    using namespace asmjit::x86;

    assembler->movzx(tmp32, word_ptr((Int)&vm.state));
    assembler->cmp(tmp16, VM_TERMINATED);
    Label ifFalse = assembler->newLabel();
    assembler->je(ifFalse);
    assembler->jmp(lendOfFunction);

    assembler->bind(ifFalse);
}

/**
 * Requires:
 * arg register to be set to current PC or -1
 * fnPtr register to be set hold label address to jump bck to if all is good
 * arg3 register to hold whether or not to increment the PC in event of context switch
 */
void x64Assembler::threadStatusCheck(Label &retLbl, Int irAddr, bool incPc) {
    assembler->lea(fnPtr, x86::ptr(retLbl)); // set return addr
    assembler->mov(arg4, (incPc ? 1 : 0));

    assembler->mov(arg, irAddr);             // set PC index
    assembler->mov(ctx, threadPtr);
    assembler->mov(ctx32, Lthread[thread_signal]);
    assembler->cmp(ctx32, 0);
    assembler->jne(lsignalCheck);
}

void x64Assembler::threadStatusCheck(Label &retLbl, bool incPc) {
    assembler->lea(fnPtr, x86::ptr(retLbl)); // set return addr
    assembler->mov(arg4, (incPc ? 1 : 0));

    assembler->mov(arg, tmpInt);             // set PC index
    assembler->mov(ctx, threadPtr);
    assembler->mov(ctx32, Lthread[thread_signal]);
    assembler->cmp(ctx32, 0);
    assembler->jne(lsignalCheck);
}

void x64Assembler::addConstantsSection() {
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->align(kAlignData, 64);              // Align 64
    assembler->bind(ldataSection);                                  // emit constants to be used
    logComment("; data section start");
    constants->emitConstants(*assembler);
}

x86::Mem x64Assembler::getMemBytePtr(Int addr) {
    return x86::byte_ptr(ctx, addr);
}

void x64Assembler::updatePc() {
    Label end = assembler->newLabel();

    assembler->cmp(arg, -1);
    assembler->je(end);
    assembler->mov(ctx, fiberPtr);
    assembler->mov(tmp, Lfiber[fiber_cache]);
    assembler->imul(arg, (size_t)sizeof(int32_t*));
    assembler->add(tmp, arg);
    assembler->mov(Lfiber[fiber_pc], tmp);
    assembler->bind(end);
    assembler->mov(arg, 0);
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
    Label overflowCheck = assembler->newLabel();

    assembler->bind(lstackCheck);
    assembler->mov(ctx, fiberPtr);
    assembler->mov(value, Lfiber[fiber_sp]);
    assembler->mov(tmp, Lfiber[fiber_dataStack]);
    assembler->mov(fnPtr, Lfiber[fiber_stack_sz]);
    assembler->sub(value, tmp);
    assembler->sar(value, 4);
    assembler->add(value, 1);
    assembler->cmp(value, fnPtr);
    assembler->jb(overflowCheck);
    assembler->mov(arg, tmpInt);
    updatePc();
    assembler->mov(ctx, fiberPtr);
    assembler->call((Int) growStack);
    assembler->mov(arg, -1);
    threadStatusCheck(overflowCheck, false);

    assembler->bind(overflowCheck);
    //check for stack overflow
    assembler->mov(ctx, fiberPtr);
    assembler->mov(value, Lfiber[fiber_sp]);
    assembler->mov(tmp, Lfiber[fiber_dataStack]);
    assembler->mov(fnPtr, Lfiber[fiber_stack_lmt]);
    assembler->sub(value, tmp);
    assembler->sar(value, 4);
    assembler->add(value, 1);
    assembler->cmp(value, fnPtr);
    Label end = assembler->newLabel();
    assembler->jb(end);
    assembler->mov(arg, tmpInt);
    updatePc();
    assembler->mov(ctx, threadPtr);
    assembler->call((Int) jitStackOverflowException);
    assembler->mov(arg, -1);
    threadStatusCheck(end, false);
    assembler->bind(end);
    assembler->jmp(returnAddress);
}

void x64Assembler::stackCheck(Int pc, Label &returnAddr) {
    assembler->mov(tmpInt, pc);
    assembler->lea(ctx, x86::ptr(returnAddr));
    assembler->mov(returnAddress, ctx);
    assembler->jmp(lstackCheck);
}

#endif
