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
/* volatile */ ctx   = rcx;           // registers ctx, value, and tmp are volitle and must be stored on the stack if deemed to be preserved
               ctx32 = ecx;

/* volatile */ tmp       = rax;        // tmp acts as a return value from functions
               tmp32     = eax;
               tmp16     = ax;
               tmp8      = al;
/* volatile */ value     = rdx;        // value acts as the second argument for function params
               fnPtr     = r12;;       // registers fnPtr, arg, regPtr, & threadPtr are non volitile and do not have to be saved
               fnPtr32   = r12d;
               arg       = r13;;
               regPtr    = r14;;
               threadPtr = r15;;
               fiberPtr  = rdi;;
/* volatile */ arg3      = r8;;        // acts as a temporary and 3rd argument for calling functions
/* volatile */ arg4      = r9;;


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
        assembler->push(fiberPtr);
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
        assembler->pop(fiberPtr);
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
        assembler->lea(tmp, x86::ptr(tmp, sizeof(opcode_instr)));
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
        assembler->mov(labelsPtr, 0);
        assembler->mov(tmpPc, 0);
        assembler->mov(tmpInt, 0);
        assembler->mov(returnAddress, 0);

        assembler->mov(fiberPtr, Ljit_context[jit_context_fiber]);
        assembler->mov(threadPtr, Ljit_context[jit_context_self]);
        assembler->mov(fnPtr, Ljit_context[jit_context_starting_pc]);
        assembler->mov(ctx, Ljit_context[jit_context_fiber]);
        assembler->mov(regPtr, Lfiber[fiber_regs]);
        assembler->lea(ctx, Lfiber[fiber_ptr]);
        assembler->mov(tmpPtr, ctx);
//        assembler->mov(ctx, tmpPtr);  //  how to assign pointer
//        assembler->mov(arg, 10);
//        assembler->mov(x86::ptr(ctx), arg);
    }
}

void x64Assembler::allocateStackSpace() {
    if(OS_id==win_os) {
        // allocate space for the stack
        int64_t storedRegs = getRegisterSize() * 7;
        int ptrSize = sizeof(jit_context *), paddr = storedRegs + ptrSize;
        int labelsSize = sizeof(int64_t *), laddr = paddr + labelsSize;
        int tmpPtrSize = sizeof(Object **), o2addr = laddr + tmpPtrSize;
        int tmpIntSize = sizeof(int64_t), tmpIntaddr = o2addr + tmpIntSize; // NOTE: make sure the stack is alligned to 16 bits if I add or subtract a stack variable
        int tmpPcSize = sizeof(int64_t), tmpPcaddr = tmpIntaddr + tmpPcSize; // NOTE: make sure the stack is alligned to 16 bits if I add or subtract a stack variable
        int returnAddressSize = sizeof(int64_t), returnAddressaddr = tmpPcaddr + returnAddressSize; // NOTE: make sure the stack is alligned to 16 bits if I add or subtract a stack variable
        stackSize = ptrSize + labelsSize + tmpPtrSize + tmpPcSize + tmpIntSize + returnAddressSize + (sizeof(int64_t)*1);
        assembler->sub(sp, (stackSize));

        ctxPtr = getMemPtr(bp, -(paddr));              // store memory location of ctx pointer in the stack
        labelsPtr = getMemPtr(bp, -(laddr));           // store memory location of labels* pointer in the stack
        tmpPtr = getMemPtr(bp, -(o2addr));              // store memory location of o2 pointer in the stack
        tmpInt = getMemPtr(bp, -(tmpIntaddr));           // store memory location of tmiInt for temporary stored integers in the stack
        tmpPc = getMemPtr(bp, -(tmpPcaddr));           // store memory location of tmiInt for temporary stored integers in the stack
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
    } else {
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
        assembler->mov(ctx32, dword_ptr(ctx));                    // if(ctx->func->jit_labels[0]==0)
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

        x86::Mem ptrIdx = dword_ptr(tmp);
        for (int64_t i = 0; i < compiledMethod->cacheSize; i++) {
            assembler->lea(ctx, x86::ptr(labels[i]));
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

int x64Assembler::addUserCode() {
    Label opcodeStart = assembler->newNamedLabel(".opcode_start", 13);

    if(OS_id==win_os) {
        assembler->bind(lcodeStart);
        assembler->test(fnPtr, fnPtr);
        assembler->jle(opcodeStart);
        assembler->mov(ctx, labelsPtr);
        assembler->sal(fnPtr, 2);
        assembler->add(ctx, fnPtr);
        assembler->mov(ctx32, dword_ptr(ctx));
        assembler->jmp(ctx32);
        assembler->bind(opcodeStart);
    }

    Int Ir = 0, Ir2 = 0, result=0;
    for(Int i = 0; i < compiledMethod->cacheSize; i++) {
        Ir = compiledMethod->bytecode[i];
        if ((i + 1) < compiledMethod->cacheSize)
            Ir2 = compiledMethod->bytecode[i + 1];

//        assembler->nop();
//        assembler->nop();
//        assembler->mov(ctx, i);
//        assembler->nop();
//        assembler->nop();
        assembler->mov(arg, i);
        updatePc();
        if (GET_OP(Ir) == Opcode::JNE || GET_OP(Ir) == Opcode::JMP
            || GET_OP(Ir) == Opcode::BRH || GET_OP(Ir) == Opcode::IFE
            || GET_OP(Ir) == Opcode::IFNE) {
            threadStatusCheck(labels[i], i, false);
        }

        stringstream tmpStream;
        tmpStream << "; instr " << i;
        logComment(tmpStream.str());
        assembler->bind(labels[i]);

        switch (GET_OP(Ir)) {
            case Opcode::NOP: {
                assembler->nop();
                break;
            }
            case Opcode::INT: {
                assembler->mov(ctx, GET_Da(Ir));
                assembler->call((Int) _BaseAssembler::jitSysInt);
                checkMasterShutdown(i);
                Label continueLabel = assembler->newLabel();
                threadStatusCheck(continueLabel, i, true);
                assembler->bind(continueLabel);
                break;
            }

            case Opcode::MOVI: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway

                movConstToXmm(vec0, Ir2);
                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::RET: {
                result = GET_Da(Ir);

                if (result == ERR_STATE) {
                    assembler->mov(ctx, fiberPtr);
                    assembler->call((Int) popExceptionObject);

                    assembler->mov(ctx, threadPtr);
                    assembler->call((Int) enableExceptionSignal);
                }

                assembler->jmp(lendOfFunction);
                break;
            }
            case Opcode::HLT: {
                assembler->mov(ctx, threadPtr);
                assembler->call((Int) enableThreadKillSignal);
                assembler->jmp(lendOfFunction);
                break;
            }
            case Opcode::NEWARRAY: {
                Label afterCheck = assembler->newLabel();
                Label pushObj = assembler->newLabel();
                stackCheck(i, afterCheck);

                assembler->bind(afterCheck);
                movRegister(vec0, GET_Ca(Ir), false);
                assembler->cvttsd2si(ctx, vec0); // double to int
                int rult = GET_Cb(Ir);
                int rult2 = GET_Ca(Ir);
                assembler->mov(value, (Int)GET_Cb(Ir));
                assembler->call((Int) _BaseAssembler::jitNewObject);
                assembler->mov(tmpInt, tmp);

                threadStatusCheck(pushObj, i,
                                  false); // TODO: should we modify this? its inefficent we could context switch after a new allocation

                assembler->bind(pushObj);
                assembler->mov(ctx, fiberPtr);
                assembler->mov(value, Lfiber[fiber_sp]);
                assembler->lea(value, x86::ptr(value, sizeof(StackElement)));
                assembler->mov(Lfiber[fiber_sp], value);

                assembler->mov(ctx, tmpInt);
                assembler->call((Int) _BaseAssembler::jitSetObject0);
                break;
            }
            case Opcode::CAST: {
                Label end = assembler->newLabel();

                movRegister(vec0, GET_Da(Ir), false);
                assembler->cvttsd2si(value, vec0); // double to int
                assembler->mov(ctx, tmpPtr);
                assembler->mov(ctx, x86::ptr(ctx));
                assembler->call((Int) _BaseAssembler::jitCast);
                threadStatusCheck(end, i, true);
                assembler->bind(end);
                break;
            }
            case Opcode::MOV8: {
                movRegister(vec0, GET_Cb(Ir), false);

                assembler->cvttsd2si(tmp, vec0); // double to int
                assembler->movsx(tmp32, tmp8);
                assembler->cvtsi2sd(vec0, tmp16);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOV16: {
                movRegister(vec0, GET_Cb(Ir), false);

                assembler->cvttsd2si(tmp, vec0); // double to int
                assembler->cwde();
                assembler->cvtsi2sd(vec0, tmp16);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOV32:
            case Opcode::MOV64: {
                movRegister(vec0, GET_Cb(Ir), false);

                if (GET_OP(Ir) == Opcode::MOV32) {
                    assembler->cvttsd2si(tmp, vec0); // double to int
                    assembler->mov(tmp, tmp32); // set lower 32 bits
                    assembler->cvtsi2sd(vec0, tmp32);
                }

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOVU8: {
                movRegister(vec0, GET_Cb(Ir), false);

                assembler->cvttsd2si(tmp, vec0); // double to int
                assembler->movzx(tmp, tmp8); // set lower 32 bits
                assembler->cvtsi2sd(vec0, tmp);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
                break;
            }
            case Opcode::MOVU16: {
                movRegister(vec0, GET_Cb(Ir), false);

                assembler->cvttsd2si(tmp, vec0); // double to int
                assembler->movzx(tmp, tmp16); // set lower 32 bits
                assembler->cvtsi2sd(vec0, tmp);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOVU32: {
                movRegister(vec0, GET_Cb(Ir), false);

                assembler->cvttsd2si(tmp, vec0); // double to int
                assembler->mov(ctx32, tmp32);
                assembler->cvtsi2sd(vec0, ctx);

                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::MOVU64: {
                assembler->mov(ctx, GET_Ca(Ir));
                assembler->mov(value, GET_Cb(Ir));
                assembler->call((Int) _BaseAssembler::jit64BitCast);
                break;
            }
            case Opcode::RSTORE: {
                Label afterCheck = assembler->newLabel();

                stackCheck(i, afterCheck);
                assembler->bind(afterCheck);
                movRegister(vec0, GET_Da(Ir), false);

                assembler->mov(ctx, fiberPtr);
                assembler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                assembler->mov(value, Lfiber[fiber_sp]); // sp++
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ADD: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                assembler->addsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::SUB: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                assembler->subsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::MUL: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                assembler->mulsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::DIV: {
                movRegister(vec0, GET_Ba(Ir), false);
                movRegister(vec1, GET_Bb(Ir), false);
                assembler->divsd(vec0, vec1);

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::MOD: {

                movRegister(vec0, GET_Ba(Ir), false);
                assembler->cvttsd2si(assembler->zax(), vec0); // double to int

                movRegister(vec1, GET_Bb(Ir), false);
                assembler->cvttsd2si(assembler->zcx(), vec1); // double to int

                assembler->cqo();
                assembler->idiv(assembler->zcx());
                assembler->cvtsi2sd(vec0, assembler->zdx());

                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::IADD: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                assembler->addsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
//                assembler->cvttsd2si(ctx, vec1); // double to int
//                assembler->call((Int)numprint);
                break;
            }
            case Opcode::ISUB: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                assembler->subsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::IMUL: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                assembler->mulsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::IDIV: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, Ir2);
                assembler->divsd(vec1, vec0);
                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::IMOD: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                movRegister(vec0, GET_Da(Ir), false);
                assembler->cvttsd2si(assembler->zax(), vec0); // double to int

                movConstToXmm(vec1, Ir2);
                assembler->cvttsd2si(assembler->zcx(), vec1); // double to int

                assembler->cqo();
                assembler->idiv(assembler->zcx());
                assembler->cvtsi2sd(vec0, assembler->zdx());

                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::POP: {
                assembler->mov(ctx, fiberPtr);
                assembler->call((Int) _BaseAssembler::jitPop);
                break;
            }
            case Opcode::INC: {
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, 1);
                assembler->addsd(vec1, vec0);

                movRegister(vec1, GET_Da(Ir), true);
                break;
            }
            case Opcode::DEC: {
                movRegister(vec0, GET_Da(Ir), false);

                movConstToXmm(vec1, 1);
                assembler->subsd(vec0, vec1);

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
                assembler->cvttsd2si(tmp, vec0); // double to int
                jmpToLabel();
                break;
            }
            case Opcode::IFNE:
            case Opcode::IFE: {
                Label ifTrue = assembler->newLabel();
                movRegister(vec0, CMT, false);
                assembler->cvttsd2si(tmp, vec0); // double to int
                assembler->cmp(tmp, 0);
                if (GET_OP(Ir) == Opcode::IFE)
                    assembler->je(ifTrue);
                else
                    assembler->jne(ifTrue);

                movRegister(vec0, ADX, false);
                assembler->cvttsd2si(tmp, vec0); // double to int

                jmpToLabel();
                assembler->bind(ifTrue);
                break;
            }
            case Opcode::LT:
            case Opcode::GT:
            case Opcode::LTE:
            case Opcode::GTE: {
                movRegister((GET_OP(Ir) == Opcode::LT || GET_OP(Ir) == Opcode::LTE) ? vec0 : vec1, GET_Ca(Ir), false);
                movRegister((GET_OP(Ir) == Opcode::LT || GET_OP(Ir) == Opcode::LTE) ? vec1 : vec0, GET_Cb(Ir), false);
                assembler->comisd(vec0, vec1);

                Label ifFalse = assembler->newLabel();
                Label ifEnd = assembler->newLabel();
                if (GET_OP(Ir) == Opcode::LT || GET_OP(Ir) == Opcode::GT)
                    assembler->jae(ifFalse);
                else
                    assembler->ja(ifFalse);
                movConstToXmm(vec0, 1);
                assembler->jmp(ifEnd);
                assembler->bind(ifFalse);

                assembler->pxor(vec0, vec0);
                assembler->bind(ifEnd);

                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::MOVL: {
                assembler->mov(ctx, fiberPtr); // ctx->current
                assembler->mov(ctx, Lfiber[fiber_fp]); // ctx->current->fp

                if (GET_Da(Ir) != 0) {
                    assembler->lea(ctx, getMemPtr(ctx, (GET_Da(Ir) * sizeof(StackElement))));
                }

                assembler->lea(ctx, Lstack_element[stack_element_object]);
                assembler->mov(arg, tmpPtr);  //  how to assign pointer
                assembler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::MOVSL: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_sp]);

                if(GET_Da(Ir) != 0) {
                    assembler->add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(Ir))); // sp+GET_DA(*pc)
                }

                assembler->lea(ctx, Lstack_element[stack_element_object]);
                assembler->mov(arg, tmpPtr);  //  how to assign pointer
                assembler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::SIZEOF: {
                assembler->mov(ctx, tmpPtr);
                assembler->mov(ctx, x86::ptr(ctx));
                assembler->cmp(ctx, 0);
                Label ifTrue = assembler->newLabel(), end = assembler->newLabel(), ifFalse = assembler->newLabel();
                assembler->jne(ifTrue);
                assembler->bind(ifFalse);
                movConstToXmm(vec0, 1);

                movRegister(vec0, GET_Da(Ir), true);
                assembler->jmp(end);
                assembler->bind(ifTrue);

                assembler->mov(ctx, qword_ptr(ctx));
                assembler->cmp(ctx, 0);
                assembler->je(ifFalse);

                assembler->mov(ctx, Lsharp_object[sharp_object_size]);
                assembler->mov(ctx32, ctx32);
                assembler->cvtsi2sd(vec0, ctx);
                movRegister(vec0, GET_Da(Ir), true);

                assembler->bind(end);
                break;
            }
            case Opcode::PUT: {
                assembler->mov(ctx, GET_Da(Ir));
                assembler->mov(value, regPtr);
                assembler->call((int64_t)_BaseAssembler::jitPut);
                break;
            }
            case Opcode::PUTC: {
                assembler->mov(ctx, GET_Da(Ir));
                assembler->mov(value, regPtr);
                assembler->call((int64_t)_BaseAssembler::jitPutC);
                break;
            }
            case Opcode::CHECKLEN: {
                checkTmpPtr(i);
                assembler->mov(fnPtr, Lsharp_object[sharp_object_size]);
                assembler->mov(fnPtr32, fnPtr32);

                movRegister(vec0, GET_Da(Ir), false);
                assembler->cvttsd2si(value, vec0); // double to int

                Label isInvalid = assembler->newLabel(), end = assembler->newLabel();
                assembler->cmp(value, 0);
                assembler->jb(isInvalid);
                assembler->cmp(value, fnPtr);
                assembler->jae(isInvalid);

                assembler->jmp(end);
                assembler->bind(isInvalid);

                assembler->mov(arg, i);
                updatePc();

                assembler->mov(ctx, fnPtr);
                assembler->mov(arg3, threadPtr);
                assembler->call((Int) _BaseAssembler::jitIndexOutOfBoundsException);
                threadStatusCheck(end, i, true);
                assembler->bind(end);
                break;
            }
            case Opcode::JMP: {
                assembler->jmp(labels[GET_Da(Ir)]);
                break;
            }
            case Opcode::LOADPC: {
                movConstToXmm(vec0, i);
                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::PUSHOBJ: {
                Label afterCheck = assembler->newLabel();

                stackCheck(i, afterCheck);
                assembler->bind(afterCheck);
                assembler->mov(ctx, fiberPtr);
                assembler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                assembler->mov(ctx, Lfiber[fiber_sp]); // sp++
                assembler->lea(ctx, Lstack_element[stack_element_object]);

                assembler->mov(value, tmpPtr);
                assembler->mov(value, x86::ptr(value));
                assembler->call((Int)_BaseAssembler::jitSetObject2);
                break;
            }
            case Opcode::DEL: {
                assembler->mov(ctx, tmpPtr);
                assembler->mov(ctx, x86::ptr(ctx));
                assembler->call((Int)_BaseAssembler::jitDelete);
                break;
            }
            case Opcode::CALL: {
                Label end = assembler->newLabel();
                assembler->mov(arg, i);
                updatePc();

                assembler->mov(ctx, threadPtr);
                assembler->mov(value, GET_Da(Ir));
                assembler->call((Int) _BaseAssembler::jitCall);
                assembler->cmp(tmp, 0);
                Label ifTrue = assembler->newLabel();
                assembler->je(end);
                assembler->mov(ctx, ctxPtr);
                assembler->call(tmp);
                assembler->bind(end);
                Label after = assembler->newLabel();
                threadStatusCheck(after, i, true);
                assembler->bind(after);
                break;
            }
            case Opcode::NEWCLASS: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                Label afterCheck = assembler->newLabel();

                stackCheck(i-1, afterCheck);
                assembler->bind(afterCheck);

                Label pushObj = assembler->newLabel();
                assembler->mov(ctx, Ir2); // double to int
                assembler->mov(value, threadPtr); // double to int
                assembler->call((Int)_BaseAssembler::jitNewClass0);
                assembler->mov(tmpInt, tmp);

                threadStatusCheck(pushObj, i-1,
                                  false);

                assembler->bind(pushObj);
                assembler->mov(ctx, fiberPtr);
                assembler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                assembler->mov(value, Lfiber[fiber_sp]); // sp++

                assembler->mov(ctx, tmpInt);
                assembler->call((Int)_BaseAssembler::jitSetObject0);
                break;
            }
            case Opcode::MOVN: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                checkTmpPtrAsObject(i);

                if(Ir2 > 0)
                    assembler->lea(ctx, getMemPtr(ctx, (Ir2*sizeof(Object))));
                assembler->mov(arg, tmpPtr);  //  how to assign pointer
                assembler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::SLEEP: {
                movRegister(vec0, GET_Da(Ir), false);
                assembler->cvttsd2si(ctx, vec0); // double to int

                assembler->call((Int)__os_sleep);
                break;
            }
            case Opcode::TEST:
            case Opcode::TNE: {
                movRegister(vec1, GET_Ca(Ir), false);
                movRegister(vec0, GET_Cb(Ir), false);

                assembler->ucomisd(vec0, vec1);

                Label ifFalse = assembler->newLabel();
                Label ifEnd = assembler->newLabel();
                assembler->jp(ifFalse);
                if(GET_OP(Ir) == Opcode::TEST)
                    assembler->jne(ifFalse);
                else
                    assembler->je(ifFalse);
                movConstToXmm(vec0, 1);
                assembler->jmp(ifEnd);
                assembler->bind(ifFalse);

                assembler->pxor(vec0, vec0);
                assembler->bind(ifEnd);

                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::LOCK: {
                checkTmpPtrAsObject(i);

                assembler->mov(ctx, tmpPtr);
                assembler->mov(ctx, x86::ptr(ctx));
                assembler->mov(value, threadPtr);
                assembler->call((Int)Object::monitorLock);
                break;
            }
            case Opcode::ULOCK: {
                checkTmpPtrAsObject(i);

                assembler->mov(ctx, tmpPtr);
                assembler->mov(ctx, x86::ptr(ctx));
                assembler->call((Int)Object::monitorUnLock);
                break;
            }
            case Opcode::MOVG: {
                assembler->mov(arg, tmpPtr);  //  how to assign pointer
                assembler->mov(ctx, (Int)(vm.staticHeap+GET_Da(Ir)));  //  how to assign pointer
                assembler->mov(x86::ptr(arg), ctx);
                break;
            }
            case Opcode::MOVND: {
                checkTmpPtrAsObject(i);
                assembler->mov(value, ctx);

                movRegister(vec0, GET_Da(Ir), false);
                assembler->cvttsd2si(tmp, vec0); // double to int

                assembler->imul(tmp, (Int)sizeof(Object));
                assembler->add(value, tmp);

                assembler->mov(arg, tmpPtr);  //  how to assign pointer
                assembler->mov(x86::ptr(arg), value);
                break;
            }
            case Opcode::NEWOBJARRAY: {
                Label afterCheck = assembler->newLabel();
                Label pushObj = assembler->newLabel();

                stackCheck(i, afterCheck);
                assembler->bind(afterCheck);

                assembler->mov(ctx, GET_Da(Ir)); // double to int
                assembler->mov(value, threadPtr);
                assembler->call((Int)_BaseAssembler::jitNewObject2);
                assembler->mov(tmpInt, tmp);

                threadStatusCheck(pushObj, i,
                                  false);

                assembler->bind(pushObj);
                assembler->mov(ctx, fiberPtr);
                assembler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                assembler->mov(value, Lfiber[fiber_sp]); // sp++

                assembler->mov(ctx, tmpInt);
                assembler->call((Int)_BaseAssembler::jitSetObject0);
                break;
            }
            case Opcode::NOT: {
                Label ifTrue = assembler->newLabel(), end = assembler->newLabel();
                movRegister(vec0, GET_Cb(Ir), false);
                assembler->cvttsd2si(tmp, vec0); // double to int

                assembler->cmp(tmp, 0);
                assembler->ja(ifTrue);
                assembler->mov(tmp, 1);
                assembler->jmp(end);

                assembler->bind(ifTrue);
                assembler->mov(tmp, 0);
                assembler->bind(end);

                assembler->cvtsi2sd(vec0, tmp); // int to Double
                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::SKIP: {
                if(GET_Da(Ir) > 0) {
                    assembler->jmp(labels[i+GET_Da(Ir)+1]);
                }
                break;
            }
            case Opcode::LOADVAL: {
                Label afterCheck = assembler->newLabel();

                stackCheck(i, afterCheck);
                assembler->bind(afterCheck);

                assembler->mov(ctx, fiberPtr);
                assembler->mov(tmp, Lfiber[fiber_sp]); // sp--
                assembler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));

                assembler->mov(ctx, tmp);
                assembler->movsd(vec0, Lstack_element[stack_element_var]);

                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::SHR:
            case Opcode::SHL: {
                movRegister(vec0, GET_Ba(Ir), false);
                assembler->cvttsd2si(tmp, vec0); // double to int
                movRegister(vec1, GET_Bb(Ir), false);
                assembler->cvttsd2si(value, vec1); // double to int

                assembler->mov(assembler->zcx(), value);
                if(GET_OP(Ir) == Opcode::SHL)
                    assembler->sal(tmp, x86::cl);
                else
                    assembler->sar(tmp, x86::cl);

                assembler->cvtsi2sd(vec0, tmp); // int to Double
                movRegister(vec0, GET_Bc(Ir), true);
                break;
            }
            case Opcode::SKNE:
            case Opcode::SKPE: {
                movRegister(vec0, GET_Ca(Ir), false);
                assembler->cvttsd2si(tmp, vec0); // double to int
                assembler->cmp(tmp, 1);
                Label ifEnd = assembler->newLabel();
                if(GET_OP(Ir) == Opcode::SKPE)
                    assembler->jne(ifEnd);
                else
                    assembler->je(ifEnd);
                assembler->jmp(labels[i+GET_Cb(Ir)]);

                assembler->bind(ifEnd);
                break;
            }
            case Opcode::AND: {
                Label ifFalse = assembler->newLabel();
                Label end = assembler->newLabel();
                movRegister(vec0, GET_Ca(Ir), false);
                assembler->cvttsd2si(tmp, vec0); // double to int
                movRegister(vec1, GET_Cb(Ir), false);
                assembler->cvttsd2si(value, vec0); // double to int

                assembler->cmp(tmp, 0);
                assembler->je(ifFalse);
                assembler->cmp(value, 0);
                assembler->je(ifFalse);
                movConstToXmm(vec0, 1);
                assembler->jmp(end);

                assembler->bind(ifFalse);
                movConstToXmm(vec0, 0);

                assembler->bind(end);
                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::UAND:
            case Opcode::OR:
            case Opcode::XOR: {
                movRegister(vec0, GET_Ca(Ir), false);
                assembler->cvttsd2si(tmp, vec0); // double to int
                movRegister(vec0, GET_Cb(Ir), false);
                assembler->cvttsd2si(ctx, vec0); // double to int
                if(GET_OP(Ir) == Opcode::UAND)
                    assembler->and_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::OR)
                    assembler->or_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::XOR)
                    assembler->xor_(tmp, ctx);

                assembler->cvtsi2sd(vec0, tmp); // int to Double
                movRegister(vec0, CMT, true);
                break;
            }
            case Opcode::THROW: { // untested from here
                Label end = assembler->newLabel();
                assembler->mov(arg, i);
                updatePc();

                assembler->mov(ctx, threadPtr);
                assembler->call((int64_t)_BaseAssembler::jitThrow);

                threadStatusCheck(end, i, true);
                assembler->bind(end);
                break;
            }
            case Opcode::CHECKNULL: {
                Label end = assembler->newLabel();

                assembler->mov(ctx, tmpPtr);
                assembler->mov(ctx, x86::ptr(ctx));
                assembler->cmp(ctx, 0);
                Label ifTrue = assembler->newLabel(), error = assembler->newLabel(), ifFalse = assembler->newLabel();
                assembler->jne(ifTrue);
                movConstToXmm(vec0, 1);
                assembler->jmp(end);

                assembler->bind(ifTrue);
                assembler->mov(ctx, qword_ptr(ctx));
                assembler->cmp(ctx, 0);
                assembler->jne(ifFalse);
                movConstToXmm(vec0, 1);
                assembler->jmp(end);
                assembler->bind(ifFalse);
                movConstToXmm(vec0, 0);
                assembler->bind(end);

                movRegister(vec0, GET_Da(Ir), true);
                break;
            }
            case Opcode::RETURNOBJ: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                assembler->lea(ctx, Lstack_element[stack_element_object]);

                assembler->mov(value, tmpPtr);
                assembler->mov(value, x86::ptr(value));
                assembler->call((int64_t)_BaseAssembler::jitSetObject2);
                break;
            }
            case Opcode::NEWCLASSARRAY: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                Label afterCheck = assembler->newLabel();

                stackCheck(i-1, afterCheck);
                assembler->bind(afterCheck);

                movRegister(vec0, GET_Da(Ir), false);
                assembler->cvttsd2si(ctx, vec0); // double to int

                Label pushObj = assembler->newLabel();
                assembler->mov(value, Ir2);
                assembler->mov(arg3, threadPtr);
                assembler->call((Int)_BaseAssembler::jitNewClass1);
                assembler->mov(tmpInt, tmp);

                threadStatusCheck(pushObj, i-1,
                                  false);

                assembler->bind(pushObj);
                assembler->mov(ctx, fiberPtr);
                assembler->add(Lfiber[fiber_sp], (Int) sizeof(StackElement));
                assembler->mov(value, Lfiber[fiber_sp]); // sp++

                assembler->mov(ctx, tmpInt);
                assembler->call((Int)_BaseAssembler::jitSetObject0);
                break;
            }
            case Opcode::NEWSTRING: {
                Label afterCheck = assembler->newLabel();
                Label end = assembler->newLabel();

                stackCheck(i, afterCheck);
                assembler->bind(afterCheck);

                assembler->mov(ctx, threadPtr);
                assembler->mov(value, GET_Da(Ir));
                assembler->call((Int)_BaseAssembler::jitNewString);

                threadStatusCheck(end, i,true);
                assembler->bind(end);
                break;
            }
            case Opcode::ADDL: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                assembler->mov(tmp, ctx);
                assembler->movsd(vec1, Lstack_element[stack_element_var]);

                movRegister(vec0, GET_Ca(Ir), false);
                assembler->addsd(vec0, vec1);
                assembler->mov(ctx, tmp);

                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::SUBL:
            case Opcode::MULL:
            case Opcode::DIVL: {
                assembler->mov(ctx, fiberPtr); // ctx->current
                assembler->mov(ctx, Lfiber[fiber_fp]); // ctx->current->fp
                if(GET_Cb(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                assembler->mov(tmp, ctx);
                assembler->movsd(vec0, Lstack_element[stack_element_var]);

                movRegister(vec1, GET_Ca(Ir), false);
                if(GET_OP(Ir) == Opcode::SUBL)
                    assembler->subsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::MULL)
                    assembler->mulsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::DIVL)
                    assembler->divsd(vec0, vec1);
                assembler->mov(ctx, tmp);

                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::MODL: {
                assembler->mov(ctx, fiberPtr); // ctx->current
                assembler->mov(ctx, Lfiber[fiber_fp]); // ctx->current->fp
                if(GET_Cb(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                assembler->mov(fnPtr, ctx);
                assembler->movsd(vec0, Lstack_element[stack_element_var]);
                assembler->cvttsd2si(assembler->zax(), vec0);

                movRegister(vec1, GET_Ca(Ir), false);
                assembler->cvttsd2si(assembler->zcx(), vec1);

                assembler->cqo();
                assembler->idiv(assembler->zcx());
                assembler->cvtsi2sd(vec0, assembler->zdx()); // int to Double

                assembler->mov(ctx, fnPtr);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::IADDL: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                assembler->movsd(vec0, Lstack_element[stack_element_var]);
                movConstToXmm(vec1, Ir2);
                assembler->addsd(vec0, vec1);

                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                assembler->movsd(vec0, Lstack_element[stack_element_var]);
                movConstToXmm(vec1, Ir2);
                if(GET_OP(Ir) == Opcode::ISUBL)
                    assembler->subsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::IMULL)
                    assembler->mulsd(vec0, vec1);
                else if(GET_OP(Ir) == Opcode::IDIVL)
                    assembler->divsd(vec0, vec1);

                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::IMODL: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                assembler->mov(fnPtr, ctx);
                assembler->movsd(vec0, Lstack_element[stack_element_var]);
                assembler->cvttsd2si(assembler->zax(), vec0);

                assembler->mov(assembler->zcx(), Ir2);

                assembler->cqo();
                assembler->idiv(assembler->zcx());
                assembler->cvtsi2sd(vec0, assembler->zdx()); // int to Double

                assembler->mov(ctx, fnPtr);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::LOADL: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                assembler->movsd(vec0, Lstack_element[stack_element_var]);
                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::IALOAD: {
                movRegister(vec0, GET_Cb(Ir), false);
                checkTmpPtrAsNumber(i);
                assembler->cvttsd2si(tmp, vec0);

                assembler->imul(tmp, (Int)sizeof(double));
                assembler->add(ctx, tmp);
                assembler->movsd(vec0, x86::qword_ptr(ctx));
                movRegister(vec0, GET_Ca(Ir), true);
                break;
            }
            case Opcode::POPOBJ: {
                checkTmpPtr(i, false);
                assembler->mov(arg3, ctx);

                assembler->mov(ctx, fiberPtr);
                assembler->mov(tmp, Lfiber[fiber_sp]); // sp--
                assembler->sub(Lfiber[fiber_sp], (Int) sizeof(StackElement));

                assembler->mov(ctx, tmp);
                assembler->lea(value, Lstack_element[stack_element_object]);
                assembler->mov(ctx, arg3);

                assembler->call((int64_t)_BaseAssembler::jitSetObject2);
                break;
            }
            case Opcode::SMOVR: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_sp]);
                if(GET_Cb(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                assembler->mov(tmp, ctx);
                movRegister(vec0, GET_Ca(Ir), false);
                assembler->mov(ctx, tmp);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ANDL:
            case Opcode::ORL:
            case Opcode::XORL: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                assembler->mov(value, ctx);
                assembler->movsd(vec0, Lstack_element[stack_element_var]);
                assembler->cvttsd2si(tmp, vec0); // double to int

                movRegister(vec0, GET_Ca(Ir), false);
                assembler->cvttsd2si(ctx, vec0); // double to int
                if(GET_OP(Ir) == Opcode::ANDL)
                    assembler->and_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::ORL)
                    assembler->or_(tmp, ctx);
                else if(GET_OP(Ir) == Opcode::XORL)
                    assembler->xor_(tmp, ctx);

                assembler->cvtsi2sd(vec0, tmp); // int to Double
                assembler->mov(ctx, value);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::RETURNVAL: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);

                assembler->mov(tmp, ctx);
                movRegister(vec0, GET_Da(Ir), false);
                assembler->mov(ctx, tmp);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ISTORE: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway
                Label afterCheck = assembler->newLabel();

                stackCheck(i, afterCheck);
                assembler->bind(afterCheck);

                movConstToXmm(vec0, Ir2);

                assembler->mov(ctx, fiberPtr);
                assembler->add(Lfiber[fiber_sp], (Int)sizeof(StackElement));
                assembler->mov(value, Lfiber[fiber_sp]); // sp++

                assembler->mov(ctx, value);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::SMOVR_2: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Cb(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Cb(Ir)));
                }

                assembler->mov(tmp, ctx);
                movRegister(vec0, GET_Ca(Ir), false);
                assembler->mov(ctx, tmp);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::ISTOREL: {
                i++;
                assembler->bind(labels[i]); // we wont use it but we need to bind it anyway

                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_fp]);
                if(GET_Da(Ir) != 0) {
                    assembler->add(ctx, (Int )(sizeof(StackElement) * GET_Da(Ir)));
                }

                movConstToXmm(vec0, Ir2);
                assembler->movsd(Lstack_element[stack_element_var], vec0);
                break;
            }
            case Opcode::TLS_MOVL: {
                assembler->mov(ctx, fiberPtr);
                assembler->mov(ctx, Lfiber[fiber_dataStack]);

                if (GET_Da(Ir) != 0) {
                    assembler->add(ctx, (Int) (sizeof(StackElement) * GET_Da(Ir)));
                }

                assembler->lea(value, Lstack_element[stack_element_object]);

                assembler->mov(arg, tmpPtr);  //  how to assign pointer
                assembler->mov(x86::ptr(arg), value);
                break;
            }
            case Opcode::VARCAST: {
                Label end = assembler->newLabel();
                assembler->mov(arg, i);
                updatePc();

                assembler->mov(ctx, tmpPtr);
                assembler->mov(ctx, x86::ptr(ctx));
                assembler->mov(value, (Int) GET_Ca(Ir));
                assembler->mov(arg3, (Int) GET_Cb(Ir));
                assembler->call((Int) _BaseAssembler::jitCastVar);
                threadStatusCheck(end, i, true);
                assembler->bind(end);
                break;
            }
            default: {
//                cout << "unknown opcode " << GET_OP(ir) << " please contact the developer immediately!" << endl;
                return jit_error_compile;
            }
        }
    }

    assembler->jmp(lendOfFunction);                  // if we reach the end of our function we dont want to set the labels again
    return jit_error_ok;
}

void x64Assembler::checkTmpPtr(Int irAddr, bool accessInnerObj) {
    assembler->mov(ctx, tmpPtr);
    assembler->mov(ctx, x86::ptr(ctx));
    assembler->cmp(ctx, 0);
    Label ifTrue = assembler->newLabel(), error = assembler->newLabel();
    assembler->jne(ifTrue);
    assembler->bind(error);
    assembler->mov(arg, irAddr);
    updatePc();
    assembler->mov(ctx, threadPtr);
    assembler->call((Int)_BaseAssembler::jitNullPtrException);
    threadStatusCheck(ifTrue, irAddr, true);
    assembler->bind(ifTrue);

    if(accessInnerObj) {
        Label ifFalse = assembler->newLabel();
        assembler->mov(ctx, qword_ptr(ctx));
        assembler->cmp(ctx, 0);
        assembler->jne(ifFalse);
        assembler->jmp(error);
        assembler->bind(ifFalse);
    }

}

void x64Assembler::checkTmpPtrAsObject(Int irAddr) {
    assembler->mov(ctx, tmpPtr);
    assembler->mov(ctx, x86::ptr(ctx));
    assembler->cmp(ctx, 0);
    Label ifTrue = assembler->newLabel(),
    error = assembler->newLabel(),
    ifFalse = assembler->newLabel(),
    nodeIsGood = assembler->newLabel();

    assembler->jne(ifTrue);
    assembler->bind(error);
    assembler->mov(arg, irAddr);
    updatePc();
    assembler->mov(ctx, threadPtr);
    assembler->call((Int)_BaseAssembler::jitNullPtrException);
    threadStatusCheck(ifTrue, irAddr, true);
    assembler->bind(ifTrue);
    assembler->mov(ctx, qword_ptr(ctx));
    assembler->cmp(ctx, 0);
    assembler->jne(ifFalse);
    assembler->jmp(error);
    assembler->bind(ifFalse);
    assembler->mov(ctx, Lsharp_object[sharp_object_node]);
    assembler->cmp(ctx, 0);
    assembler->jne(nodeIsGood);
    assembler->jmp(error);
    assembler->bind(nodeIsGood);
}

void x64Assembler::checkTmpPtrAsNumber(Int irAddr) {
    assembler->mov(ctx, tmpPtr);
    assembler->mov(ctx, x86::ptr(ctx));
    assembler->cmp(ctx, 0);
    Label ifTrue = assembler->newLabel(),
    error = assembler->newLabel(),
    ifFalse = assembler->newLabel(),
    headIsGood = assembler->newLabel();

    assembler->jne(ifTrue);
    assembler->bind(error);
    assembler->mov(arg, irAddr);
    updatePc();
    assembler->mov(ctx, threadPtr);
    assembler->call((Int)_BaseAssembler::jitNullPtrException);
    threadStatusCheck(ifTrue, irAddr, true);
    assembler->bind(ifTrue);
    assembler->mov(ctx, qword_ptr(ctx));
    assembler->cmp(ctx, 0);
    assembler->jne(ifFalse);
    assembler->jmp(error);
    assembler->bind(ifFalse);
    assembler->mov(ctx, Lsharp_object[sharp_object_HEAD]);
    assembler->cmp(ctx, 0);
    assembler->jne(headIsGood);
    assembler->jmp(error);
    assembler->bind(headIsGood);
}

/**
 * Requires:
 * tmp register to be set inotder to jump to address value dynamically
 */
void x64Assembler::jmpToLabel() {
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->mov(ctx, labelsPtr);      // were just using these registers because we can, makes life so much easier
    assembler->cmp(tmp, 0);
    Label ifTrue = assembler->newLabel();
    assembler->je(ifTrue);
    assembler->imul(tmp, (size_t)sizeof(int32_t));      // offset = labelAddr*sizeof(int64_t)
    assembler->add(ctx, tmp);
    assembler->bind(ifTrue);

    assembler->mov(ctx32, x86::ptr(ctx));
    assembler->jmp(ctx);
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
    Label hasSuspend = assembler->newLabel();
    Label exceptionCaught = assembler->newLabel();
    Label contextSwitchCheck = assembler->newLabel();
    Label signalCheckEnd = assembler->newLabel();
    Label isContextSwitchEnabled = assembler->newLabel();

    if(OS_id==win_os) {

        /* Thread Suspended Check */
        assembler->bind(lsignalCheck);
        assembler->mov(ctx, threadPtr);
        assembler->movzx(tmp32, Lthread[thread_context_switching]);
        assembler->movzx(tmp32, tmp8);
        assembler->cmp(tmp32, 0);
        assembler->je(hasSuspend);
        assembler->jmp(lfunctionEpilogue);
        assembler->bind(hasSuspend);

        updatePc();                                                      // before we call we need to set arg register to -1 if we already have the pc updated
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
        assembler->mov(value, tmpPc);
        assembler->call((Int)_BaseAssembler::jitTryContextSwitch);

        assembler->cmp(tmp32, 0);
        assembler->je(isContextSwitchEnabled);
        assembler->jmp(lfunctionEpilogue);
        assembler->bind(isContextSwitchEnabled);

        assembler->jmp(fnPtr);
        /* end of check */
    }
}


void x64Assembler::checkMasterShutdown(int64_t pc) {
    using namespace asmjit::x86;

    assembler->movzx(tmp32, word_ptr((Int)&vm.state));
    assembler->cmp(tmp16, VM_TERMINATED);
    Label ifFalse = assembler->newLabel();
    assembler->jne(ifFalse);
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
    assembler->mov(tmpPc, (incPc ? 1 : 0));

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
    assembler->sal(arg, 2);
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
    Label underflowErr = assembler->newLabel();

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
    assembler->sub(value, 1);
    assembler->cmp(value, 0);
    assembler->jl(underflowErr);
    assembler->jmp(returnAddress);
    assembler->bind(underflowErr);
    assembler->mov(ctx, threadPtr);
    assembler->call((Int) jitStackUnderflowException);
    assembler->mov(arg, -1);
    threadStatusCheck(end, false);
}

void x64Assembler::stackCheck(Int pc, Label &returnAddr) {
    assembler->mov(tmpInt, pc);
    assembler->lea(ctx, x86::ptr(returnAddr));
    assembler->mov(returnAddress, ctx);
    assembler->jmp(lstackCheck);
}

#endif
