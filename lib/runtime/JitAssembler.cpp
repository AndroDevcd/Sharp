//
// Created by braxtonn on 8/21/2019.
//

#include <fstream>
#include "JitAssembler.h"
#include "Thread.h"
#include "Manifest.h"
#include "main.h"
#include "Environment.h"
#include "Jit.h"
#include "Opcode.h"
#include "VirtualMachine.h"

void JitAssembler::shutdown() {

    for(x86int_t i = 0; i < functions.len; i++) {
        rt.release(functions.get(i));
    }
    functions.free();
}

void JitAssembler::initialize() {
    initializeRegisters();
    setupContextFields();
    setupThreadFields();
    setupStackElementFields();
    setupFrameFields();
    setupMethodFields();
    setupSharpObjectFields();
}

void JitAssembler::setupContextFields() {
    Thread *self = Thread::getThread(main_threadid);
    Ljit_context[jit_context_self] = getMemPtr(relative_offset((self->jctx), self, self));
    Ljit_context[jit_context_regs] = getMemPtr(relative_offset((self->jctx), self, regs));
    Ljit_context[jit_context_caller] = getMemPtr(relative_offset((self->jctx), self, caller));
}

void JitAssembler::setupThreadFields() {
    Thread* thread = Thread::getThread(main_threadid);
    Lthread[thread_calls] = getMemPtr(relative_offset(thread, calls, calls));
    Lthread[thread_dataStack] = getMemPtr(relative_offset(thread, calls, dataStack));
    Lthread[thread_sp] = getMemPtr(relative_offset(thread, calls, sp));
    Lthread[thread_fp] = getMemPtr(relative_offset(thread, calls, fp));
    Lthread[thread_current] = getMemPtr(relative_offset(thread, calls, current));
    Lthread[thread_callStack] = getMemPtr(relative_offset(thread, calls, callStack));
    Lthread[thread_stack_lmt] = getMemPtr(relative_offset(thread, calls, stack_lmt));
    Lthread[thread_cache] = getMemPtr(relative_offset(thread, calls, cache));
    Lthread[thread_pc] = getMemPtr(relative_offset(thread, calls, pc));
    Lthread[thread_state] = getMemPtr(relative_offset(thread, calls, state));
    Lthread[thread_signal] = getMemPtr(relative_offset(thread, calls, signal));
    Lthread[thread_stbase] = getMemPtr(relative_offset(thread, calls, stbase));
    Lthread[thread_stack] = getMemPtr(relative_offset(thread, calls, stack));
}

void JitAssembler::setupStackElementFields() {
    StackElement stack;
    Lstack_element[stack_element_var] = getMemPtr(relative_offset((&stack), var, var));
    Lstack_element[stack_element_object] = getMemPtr(relative_offset((&stack), var, object));
}

void JitAssembler::setupFrameFields() {
    Frame frame(0,0,0,0,false);
    Lframe[frame_last] = getMemPtr(relative_offset((&frame), last, last));
    Lframe[frame_pc] = getMemPtr(relative_offset((&frame), last, pc));
    Lframe[frame_sp] = getMemPtr(relative_offset((&frame), last, sp));
    Lframe[frame_fp] = getMemPtr(relative_offset((&frame), last, fp));
}

void JitAssembler::setupMethodFields() {
    Method m;
    Lmethod[method_bytecode] = getMemPtr(relative_offset((&m), jit_labels, bytecode));
}

void JitAssembler::setupSharpObjectFields() {
    SharpObject o;
    Lsharp_object[sharp_object_HEAD] = getMemPtr(relative_offset((&o), HEAD, HEAD));
    Lsharp_object[sharp_object_node] = getMemPtr(relative_offset((&o), HEAD, node));
    Lsharp_object[sharp_object_k] = getMemPtr(relative_offset((&o), HEAD, k));
    Lsharp_object[sharp_object_size] = getMemPtr(relative_offset((&o), HEAD, size));
}

int JitAssembler::performInitialCompile() {
    int error;
    for(x86int_t i = 0; i < manifest.methods; i++) {
        if(c_options.jit && (c_options.slowBoot || env->methods[i].isjit)) {
            // we want to compile it
            env->methods[i].isjit=false;
            error = tryJit(env->methods+i);
            switch (error) {
                case jit_error_compile:
                    env->methods[i].jitAttempts = JIT_MAX_ATTEMPTS;
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

int JitAssembler::tryJit(Method* func) {
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

int JitAssembler::compile(Method *func) {
    if(func->bytecode != NULL && func->cacheSize >= JIT_IR_MIN)
    {
        int error = jit_error_ok;

        // make it easier for the JIT Compiler
        func->jit_labels = (x86int_t *)malloc(sizeof(x86int_t)*func->cacheSize);
        if(!func->jit_labels)
            return jit_error_mem;
        else
            std::memset(func->jit_labels, 0, sizeof(x86int_t)*func->cacheSize);

        CodeHolder code;
        code.init(rt.getCodeInfo());

        FileLogger logger(getLogFile());
        code.setLogger(&logger);                // Initialize logger temporarily to ensure quality of code

        X86Assembler assembler(&code);                  // Create and attach X86Assembler to `code`.
        Constants constant_pool;

        string msg = "; method " + func->fullName.str() + "\n";
        assembler.comment(msg.c_str(), msg.size());
        assembler.comment("; starting save state", 21);

        assembler.push(bp);
        assembler.mov(bp, sp);

        assembler.push(fnPtr);                          // Store used registers (windows x86 convention)
        assembler.push(arg);
        assembler.push(regPtr);
        assembler.push(threadPtr);

        // function body

        // allocate space for the stack
        x86int_t storedRegs = getRegisterSize() * 4;
        int ptrSize      = sizeof(jit_context*), paddr = storedRegs + ptrSize;
        int labelsSize   = sizeof(x86int_t*), laddr = paddr + labelsSize;
        int o2Size       = sizeof(Object*), o2addr = laddr + o2Size;
        x86int_t stackSize = ptrSize + labelsSize + o2Size;
        assembler.sub(sp, (stackSize));

        Label labels[func->cacheSize];                        // Each opcode has its own label but not all labels will be used
        for(x86int_t i = 0; i < func->cacheSize; i++) {       // Iterate through all the addresses to create labels for each address
            labels[i] = assembler.newLabel();
        }

        X86Mem ctxPtr = getMemPtr(bp, -(paddr));              // store memory location of ctx pointer in the stack
        X86Mem o2Ptr = getMemPtr(bp, -(o2addr));              // store memory location of o2 pointer in the stack
        X86Mem labelsPtr = getMemPtr(bp, -(laddr));           // store memory location of labels* pointer in the stack

        assembler.mov(ctxPtr, ctx);                           // send ctx to stack from ctx register via [ESP + paddr].

        // zero out registers & memory
        assembler.xor_(fnPtr, fnPtr);
        assembler.xor_(arg, arg);
        assembler.mov(o2Ptr, 0);
        assembler.mov(labelsPtr, 0);

        assembler.mov(regPtr, Ljit_context[jit_context_regs]);
        assembler.mov(threadPtr, Ljit_context[jit_context_self]);

        // context->caller->jit_lables
        assembler.mov(ctx, Ljit_context[jit_context_caller]);
        assembler.mov(ctx, getMemPtr(ctx, 0));

        Label lbl_code_start = assembler.newNamedLabel("code_start", 10);
        Label lbl_init_addr_tbl = assembler.newNamedLabel("init_addr_tbl", 13);
        Label lbl_func_end = assembler.newNamedLabel("func_end", 8);
        Label lbl_data_sec = assembler.newNamedLabel(".data", 5);
        Label lbl_thread_chk = assembler.newNamedLabel(".thread_check", 13);

        assembler.mov(labelsPtr, ctx);

        X86Mem tmp_ptr = getMemPtr(ctx, 0);
        assembler.mov(ctx, tmp_ptr);                    // if(ctx->func->jit_labels[0]==0)
        assembler.test(ctx, ctx);                       //      goto lbl_;
        assembler.jne(lbl_code_start);
        assembler.jmp(lbl_init_addr_tbl);
        assembler.bind(lbl_code_start);
        // user code start


        x86int_t ir, irTail;
        X86Mem lconstMem, tmpMem;
        for(x86int_t i = 0; i < func->cacheSize; i++) {
            if (error) break;
            ir = func->bytecode[i];
            if((i+1) < func->cacheSize)
                irTail = func->bytecode[i+1];
            if(i==0)
                threadStatusCheck(assembler, labels[i], lbl_thread_chk, i);

            assembler.bind(labels[i]);
            switch(GET_OP(ir)) {
                default: {
                    assembler.nop();                    // by far one of the easiest instructions yet
                    break;
                }
            }
        }

        assembler.mov(arg, (func->cacheSize-1));
        updatePc(assembler);
        assembler.jmp(lbl_func_end);                    // if we reach the end of our function we dont want to set the labels again
        assembler.bind(lbl_init_addr_tbl);
        assembler.nop();
        // labeles[] setting here
        assembler.comment("; setting label values", 22);
        assembler.mov(tmp, labelsPtr);
        X86Mem ptrIdx = getMemPtr(tmp);
        X86Mem lbl;
        for(int64_t i = 0; i < func->cacheSize; i++) {
            lbl = x86::ptr(labels[i]);
            assembler.lea(ctx, lbl);
            assembler.mov(ptrIdx, ctx);

            if((i + 1) < func->cacheSize)                       // omit unessicary add instruction
                assembler.add(tmp, (x86int_t )sizeof(x86int_t));
        }

        assembler.nop();
        assembler.jmp(lbl_code_start);                         // jump back to top to execute user code

        // end of function
        assembler.bind(lbl_func_end);
        assembler.mov(ctx, threadPtr);
        assembler.call((x86int_t)returnMethod);               // we need to update the PC just before this call
        incPc(assembler);

        assembler.add(sp, (stackSize));
        assembler.pop(threadPtr);
        assembler.pop(regPtr);
        assembler.pop(arg);
        assembler.pop(fnPtr);
        assembler.pop(bp);

        assembler.ret();

        assembler.bind(lbl_thread_chk);                                     // we need to update the PC just before this addr jump as well as save the return back addr in fnPtr
        Label isThreadKilled = assembler.newLabel();
        Label hasException = assembler.newLabel();
        Label thread_chk_end = assembler.newLabel();
        Label ifFalse = assembler.newLabel();

        assembler.mov(ctx, threadPtr);                                      // Suspend our thread?
        assembler.mov(tmp32, Lthread[thread_signal]);
        assembler.sar(tmp32, ((int)tsig_suspend));
        assembler.and_(tmp32, 1);
        assembler.test(tmp32, tmp32);
        assembler.je(isThreadKilled);

        assembler.call((x86int_t)Thread::suspendSelf);
        assembler.bind(isThreadKilled);

        assembler.mov(ctx, threadPtr);                                      // has it been shut down??
        assembler.mov(tmp32, Lthread[thread_state]);
        assembler.cmp(tmp32, THREAD_KILLED);
        assembler.jne(hasException);
        assembler.jmp(lbl_func_end); // verified

        assembler.bind(hasException);
        assembler.mov(ctx, threadPtr);                                    // Do we have an exception to catch?
        assembler.mov(tmp32, Lthread[thread_signal]);
        assembler.sar(tmp32, ((int)tsig_except));
        assembler.and_(tmp32, 1);
        assembler.test(tmp32, tmp32);
        assembler.je(thread_chk_end);                                    // at this point no need to check any more events

        updatePc(assembler);

        assembler.mov(ctx, Lthread[thread_current]);
        assembler.call((x86int_t)JitAssembler::jitTryCatch);

        assembler.cmp(tmp, 1);
        assembler.je(ifFalse);
        assembler.jmp(lbl_func_end);
        assembler.bind(ifFalse);

        assembler.mov(ctx, threadPtr);
        assembler.call((x86int_t)JitAssembler::jitGetPc);

        assembler.mov(value, labelsPtr);                              // reset pc to find location in function to jump to
        assembler.imul(tmp, (size_t)sizeof(x86int_t));
        assembler.add(value, tmp);
        assembler.mov(fnPtr, x86::ptr(value));
        assembler.jmp(fnPtr);

        assembler.bind(thread_chk_end);
        assembler.jmp(fnPtr);                                         // dynamically jump to last address in out function


        assembler.nop();
        assembler.nop();
        assembler.nop();
        assembler.align(kAlignData, 64);              // Align 64
        assembler.bind(lbl_data_sec);                 // emit constants to be used
        assembler.comment("; data section start", 20);
        constant_pool.emitConstants(assembler);


        fptr fn;
        Error err = rt.add(&fn, &code);   // Add the generated code to the runtime.
        if (err) {
            cout << "jit code error " << err << endl;
            error =  jit_error_compile;
        }
        else {
            func->isjit = true;
            func->jit_call = fn;
            functions.push_back(fn);
        }

        func->compiling = false;
        return error;
    }

    func->compiling = false;
    return jit_error_compile;
}

void
JitAssembler::threadStatusCheck(X86Assembler &assembler, Label &retLbl, Label &lbl_thread_sec, x86int_t irAddr) {
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
void JitAssembler::updatePc(X86Assembler &assembler) {
    assembler.mov(ctx, threadPtr);
    assembler.mov(tmp, Lthread[thread_cache]);
    assembler.imul(arg, (size_t)sizeof(x86int_t));
    assembler.add(tmp, arg);
    assembler.mov(Lthread[thread_pc], tmp);
}

x86int_t JitAssembler::jitGetPc(Thread *thread) {
    return thread->pc-thread->cache;
}

int JitAssembler::jitTryCatch(Method *method) {
    return vm->TryCatch(method, &thread_self->exceptionObject) ? 1 : 0;
}

void JitAssembler::incPc(X86Assembler &assembler) {
    assembler.mov(ctx, threadPtr);                       // increment PC from thread
    assembler.mov(value, Lthread[thread_pc]);
    assembler.lea(value, x86::ptr(value, sizeof(int64_t)));
    assembler.mov(Lthread[thread_pc], value);
}

FILE *JitAssembler::getLogFile() {
    ofstream outfile ("JIT.s");        // Quickly create file
    outfile.close();

    FILE * pFile;                           // out logging file
    pFile = fopen ("JIT.s" , "rw+");
    return pFile;
}
