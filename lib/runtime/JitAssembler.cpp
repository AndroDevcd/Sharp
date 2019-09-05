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
#include "register.h"

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
    int error = jit_error_ok;
    if(func->bytecode != NULL)
    {
        if(func->cacheSize >= JIT_IR_MIN)
        {
            // make it easier for the JIT Compiler
            func->jit_labels = (x86int_t *)malloc(sizeof(x86int_t)*func->cacheSize);
            if(!func->jit_labels) {
                error = jit_error_mem;
                goto finish;
            }
            else
                std::memset(func->jit_labels, 0, sizeof(x86int_t)*func->cacheSize);

            CodeHolder code;
            code.init(rt.codeInfo());

            FileLogger logger(getLogFile());
            code.setLogger(&logger);                // Initialize logger temporarily to ensure quality of code

            x86::Assembler assembler(&code);                  // Create and attach x86::Assembler to `code`.
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
            int tmpIntSize       = sizeof(x86int_t), tmpIntaddr = o2addr + tmpIntSize;
            x86int_t stackSize = ptrSize + labelsSize + o2Size + tmpIntSize + 0x80; // 0x80 128 byte red zone for stack information
            assembler.sub(sp, (stackSize));

            Label labels[func->cacheSize];                        // Each opcode has its own label but not all labels will be used
            for(x86int_t i = 0; i < func->cacheSize; i++) {       // Iterate through all the addresses to create labels for each address
                labels[i] = assembler.newLabel();
            }

            x86::Mem ctxPtr = getMemPtr(bp, -(paddr));              // store memory location of ctx pointer in the stack
            x86::Mem o2Ptr = getMemPtr(bp, -(o2addr));              // store memory location of o2 pointer in the stack
            x86::Mem labelsPtr = getMemPtr(bp, -(laddr));           // store memory location of labels* pointer in the stack
            x86::Mem tmpInt = getMemPtr(bp, -(tmpIntaddr));           // store memory location of tmiInt for temporary stored integers in the stack

            assembler.mov(ctxPtr, ctx);                           // send ctx to stack from ctx register via [ESP + paddr].

            // zero out registers & memory
            assembler.xor_(fnPtr, fnPtr);
            assembler.xor_(arg, arg);
            assembler.mov(o2Ptr, 0);
            assembler.mov(labelsPtr, 0);
            assembler.mov(tmpInt, 0);

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

            x86::Mem tmp_ptr = getMemPtr(ctx, 0);
            assembler.mov(ctx, tmp_ptr);                    // if(ctx->func->jit_labels[0]==0)
            assembler.test(ctx, ctx);                       //      goto lbl_;
            assembler.jne(lbl_code_start);
            assembler.jmp(lbl_init_addr_tbl);
            assembler.bind(lbl_code_start);
            // user code start


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
                ss <<  "instr " << i;
                string s = ss.str();
                assembler.comment(s.c_str(), s.size());
                assembler.bind(labels[i]);
                switch(GET_OP(ir)) {
                    case op_NOP:
                        assembler.nop();
                        break;
                    case op_INT: {
                        assembler.mov(ctx32, GET_Da(ir));
                        assembler.call((x86int_t) JitAssembler::jitSysInt); // TODO: check stack address before and after call for allognment
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
                        assembler.call((x86int_t)JitAssembler::jitNewObject);
                        assembler.mov(tmpInt, tmp);

                        checkSystemState(lbl_func_end, i, assembler, lbl_thread_chk);

                        assembler.mov(ctx, threadPtr);
                        assembler.mov(value, Lthread[thread_sp]);
                        assembler.lea(value, x86::ptr(value, sizeof(StackElement)));
                        assembler.mov(Lthread[thread_sp], value);

                        assembler.mov(ctx, tmpInt);
                        assembler.call((x86int_t)JitAssembler::jitSetObject0);
                        break;
                    }
                    case op_CAST: {
                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.cvttsd2si(value, vec0); // double to int
                        assembler.mov(ctx, o2Ptr);

                        assembler.call((x86int_t)JitAssembler::jitCast);
                        checkSystemState(lbl_func_end, i, assembler, lbl_thread_chk);
                        break;
                    }
                    case op_VARCAST: {
                        assembler.mov(ctx, o2Ptr);
                        assembler.mov(value, (x86int_t )GET_Da(ir));

                        assembler.call((x86int_t)JitAssembler::jitCastVar);
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
                        assembler.call((int64_t)JitAssembler::jit64BitCast);
                        break;
                    }
                    case op_RSTORE: {
                        movRegister(assembler, vec0, GET_Da(ir), false);
                        assembler.mov(ctx, threadPtr);
                        assembler.add(Lthread[thread_sp], (int64_t)sizeof(StackElement));
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.movsd(getMemPtr(tmp), vec0);
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
                        assembler.mov(assembler.zax(), assembler.zdx());
                        assembler.cvtsi2sd(vec0, assembler.zax());

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
                        assembler.subsd(vec1, vec0);
                        movRegister(assembler, vec1, GET_Ca(ir));
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
                        assembler.divsd(vec1, vec0);
                        movRegister(assembler, vec1, GET_Ca(ir));
                        break;
                    }
                    case op_IMOD: {
                        movRegister(assembler, vec0, GET_Ca(ir), false);
                        assembler.cvttsd2si(assembler.zax(), vec0); // double to int

                        emitConstant(assembler, constant_pool, vec1, GET_Cb(ir));
                        assembler.cvttsd2si(assembler.zcx(), vec1); // double to int

                        assembler.cqo();
                        assembler.idiv(assembler.zcx());
                        assembler.mov(assembler.zax(), assembler.zdx());
                        assembler.cvtsi2sd(vec0, assembler.zax());

                        i++; assembler.bind(labels[i]); // we wont use it but we need to bind it anyway
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    case op_POP: {// --sp;
                        assembler.mov(ctx, threadPtr);
                        assembler.mov(tmp, Lthread[thread_sp]);
                        assembler.lea(tmp, x86::ptr(tmp, -((int64_t)sizeof(StackElement))));
                        assembler.mov(Lthread[thread_sp], tmp);
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
                        assembler.subsd(vec1, vec0);

                        movRegister(assembler, vec1, GET_Da(ir));
                        break;
                    }
                    case op_MOVR: { // registers[GET_Ca(*pc)]=registers[GET_Cb(*pc)];
                        movRegister(assembler, vec0, GET_Cb(ir), false);
                        movRegister(assembler, vec0, GET_Ca(ir));
                        break;
                    }
                    default: {
                        assembler.nop();                    // by far one of the easiest instructions yet
                        break;
                    }
                }

                assembler.nop(); // instruction differentation for now
                assembler.nop();
                assembler.nop();
                assembler.nop();
            }

            assembler.mov(arg, (func->cacheSize-1));
            updatePc(assembler);
            assembler.jmp(lbl_func_end);                    // if we reach the end of our function we dont want to set the labels again
            assembler.bind(lbl_init_addr_tbl);
            assembler.nop();
            // labeles[] setting here
            assembler.comment("; setting label values", 22);
            assembler.mov(tmp, labelsPtr);
            x86::Mem ptrIdx = getMemPtr(tmp);
            x86::Mem lbl;
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

            // Thread State Check
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
        } else {
            error = jit_error_size;
        }
    } else {
        error = jit_error_compile;
    }

    finish:
    func->compiling = false;
    return error;
}

/*
 * Very expensive procedure so use it sparingly
 */
void
JitAssembler::checkSystemState(const Label &lbl_func_end, x86int_t pc, x86::Assembler &assembler, Label &lbl_thread_chk) {
    Label thread_check_end = assembler.newLabel();
    threadStatusCheck(assembler, thread_check_end, lbl_thread_chk, pc);
    assembler.bind(thread_check_end);
    checkMasterShutdown(assembler, pc, lbl_func_end);
}

void JitAssembler::jitCast(Object *obj, x86int_t klass) {
    try {
        if(obj!=NULL) {
            obj->castObject(klass);
        } else {
            Exception nptr(Environment::NullptrException, "");
            __srt_cxx_prepare_throw(nptr);
        }
    }catch(Exception &e) {
        __srt_cxx_prepare_throw(e);
    }
}

void JitAssembler::jitCastVar(Object *obj, int array) {
    if(obj!=NULL && obj->object != NULL) {
        if(obj->object->HEAD == NULL) {
            stringstream ss;
            ss << "illegal cast to var" << (array ? "[]" : "");
            Exception err(Environment::ClassCastException, ss.str());
            __srt_cxx_prepare_throw(err);
        }
    } else {
        Exception nptr(Environment::NullptrException, "");
        __srt_cxx_prepare_throw(nptr);
    }
}

// REMEMBER!!! dont forget to check state of used registers befote and after this call as they might be different than what they werr before
void JitAssembler::test(x86int_t proc) {
    cout << "ebx " << registers[i64ebx] << endl << std::flush;
}

// had a hard time with this one so will do this for now
void JitAssembler::jit64BitCast(x86int_t dest, x86int_t src) {
    registers[dest] = (int64_t)registers[src];
}

void JitAssembler::emitConstant(x86::Assembler &assembler, Constants &cpool, x86::Xmm xmm, double _const) {
    if(_const == 0) {
        assembler.pxor(xmm, xmm);
    } else { \
        x86int_t idx = cpool.createConstant(assembler, _const);
        x86::Mem lconst = x86::ptr(cpool.getConstantLabel(idx));
        assembler.movsd(xmm, lconst);
    }
}

void JitAssembler::movRegister(x86::Assembler &assembler, x86::Xmm &vec, x86int_t addr, bool store) {
    assembler.mov(ctx, regPtr);        // move the contex var into register
    if(addr != 0) {
        assembler.add(ctx, (int64_t )(sizeof(double) * addr));
    }

    if(store)
        assembler.movsd(x86::qword_ptr(ctx), vec);  // store into register
    else
        assembler.movsd(vec, x86::qword_ptr(ctx)); // get value from register
}

void JitAssembler::checkMasterShutdown(x86::Assembler &assembler, int64_t pc, const Label &lbl_funcend) {
    using namespace asmjit::x86;

    assembler.movzx(ctx32, (x86int_t)&masterShutdown);
    assembler.cmp(ctx32, 0);
    Label ifFalse = assembler.newLabel();
    assembler.je(ifFalse);
    assembler.mov(arg, pc);
    assembler.jmp(lbl_funcend);

    assembler.bind(ifFalse);
}

void JitAssembler::jitSysInt(x86int_t signal) {
    try {
        VirtualMachine::sysInterrupt(signal);
    } catch(Exception &e) {
        __srt_cxx_prepare_throw(e);
    }
}

SharpObject* JitAssembler::jitNewObject(x86int_t size) {
    try {
        return GarbageCollector::self->newObject(size);
    } catch(Exception &e) {
        __srt_cxx_prepare_throw(e);
        return NULL;
    }
}

void JitAssembler::jitSetObject0(SharpObject* o, StackElement *sp) {
    sp->object = o;
}

void JitAssembler::__srt_cxx_prepare_throw(Exception &e) {
    Thread *self = thread_self;
    self->throwable = e.getThrowable();
    Object *eobj = &self->exceptionObject;

    VirtualMachine::fillStackTrace(eobj);
    self->throwable.throwable = eobj->object->k;
    sendSignal(self->signal, tsig_except, 1);
}

void
JitAssembler::threadStatusCheck(x86::Assembler &assembler, Label &retLbl, Label &lbl_thread_sec, x86int_t irAddr) {
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
void JitAssembler::updatePc(x86::Assembler &assembler) {
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

void JitAssembler::incPc(x86::Assembler &assembler) {
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
