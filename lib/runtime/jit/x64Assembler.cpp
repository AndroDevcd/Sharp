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
    fiberPtr  = r8;        // acts as a temporary and 3rd argument for calling functions
                           // if a 4th argument is needed add a register of $r8

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

        assembler->push(fnPtr);                          // Store used registers (windows x86 convention)
        assembler->push(arg);
        assembler->push(regPtr);
        assembler->push(threadPtr);
        assembler->push(fiberPtr);
    } else {

    }
}

void x64Assembler::createFunctionEpilogue() {
    if(OS_id==win_os) {
        assembler->bind(lendOfFunction);
        assembler->mov(ctx, threadPtr);
        assembler->call((int64_t) returnMethod);               // we need to update the PC just before this call
        incPc();

        assembler->add(sp, (stackSize));
        assembler->pop(threadPtr);
        assembler->pop(regPtr);
        assembler->pop(arg);
        assembler->pop(fnPtr);
        assembler->pop(fiberPtr);
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
        int64_t storedRegs = getRegisterSize() * 5;
        int ptrSize = sizeof(jit_context *), paddr = storedRegs + ptrSize;
        int labelsSize = sizeof(int64_t *), laddr = paddr + labelsSize;
        int tmpPtrSize = sizeof(Object *), o2addr = laddr + tmpPtrSize;
        int tmpIntSize = sizeof(int64_t), tmpIntaddr = o2addr +
                                                       tmpIntSize; // NOTE: make sure the stack is alligned to 16 bits if I add or subtract a stack variable
        stackSize = ptrSize + labelsSize + tmpPtrSize + tmpIntSize;
        assembler->sub(sp, (stackSize));

        ctxPtr = getMemPtr(bp, -(paddr));              // store memory location of ctx pointer in the stack
        labelsPtr = getMemPtr(bp, -(laddr));           // store memory location of labels* pointer in the stack
        tmpPtr = getMemPtr(bp, -(o2addr));              // store memory location of o2 pointer in the stack
        tmpInt = getMemPtr(bp,
                           -(tmpIntaddr));           // store memory location of tmiInt for temporary stored integers in the stack
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
    lcontextSwitchCheck = assembler->newNamedLabel(".context_switch_check", 21);
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
//        assembler->mov(tmp, labelsPtr);
//
//        x86::Mem ptrIdx = getMemPtr(tmp);
//        x86::Mem lbl;
//        for (int64_t i = 0; i < compiledMethod->cacheSize; i++) {
//            lbl = x86::ptr(labels[i]);
//            assembler->lea(ctx, lbl);
//            assembler->mov(ptrIdx, ctx);
//
//            if ((i + 1) < compiledMethod->cacheSize)                       // omit unessicary add instruction
//                assembler->add(tmp, (int64_t) sizeof(int32_t));
//        }
//
//        assembler->nop();
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

    if(OS_id==win_os) {
        assembler->bind(lcodeStart);
        assembler->test(fnPtr, fnPtr);
        assembler->jle(opcodeStart);
        assembler->mov(ctx, labelsPtr);
        assembler->imul(fnPtr, (int64_t)sizeof(int32_t));
        assembler->add(ctx, fnPtr);
        assembler->mov(ctx, qword_ptr(ctx));
        assembler->jmp(ctx);
        assembler->bind(opcodeStart);

        // TODO: add for loop for all opcodes
        assembler->jmp(lendOfFunction);                  // if we reach the end of our function we dont want to set the labels again
    }
}

void x64Assembler::addThreadSignalCheck() {

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

#endif
