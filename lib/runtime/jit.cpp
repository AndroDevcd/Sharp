//
// Created by BraxtonN on 8/9/2018.
//

#include "jit.h"
#include "Opcode.h"
#include "Thread.h"
#include "../util/KeyPair.h"
#include "register.h"
#include "VirtualMachine.h"
#include <stdio.h>
#include <fstream>
#include <cstdint>

using namespace asmjit;

/**
 * Global reference to our runtime engine
 */
JitRuntime rt;

thread_local jit_ctx jctx;
std::vector<jit_func> functions;
uint64_t jit_funcid = 0;
X86Mem jit_ctx_fields[5]; // memory map of jit_ctx
X86Mem thread_fields[12]; // sliced memory map of Thread
X86Mem stack_element_fields[2]; // memory map of StackElement
#ifdef SHARP_PROF_
X86Mem profiler_fields[4]; // memory map of Profiler
#endif

void setupJitContextFields(const X86Gp &ctx);
void setupThreadContextFields(const X86Gp &ctx);
void setupStackElementFields(const X86Gp &ctx);

void saveCalleeRegisters(X86Assembler &cc);
void restoreCalleeRegisters(X86Assembler &cc, int64_t stackSize);

void savePrivateRegisters(X86Assembler &cc, X86Xmm &vec0, X86Xmm &vec1);
void restorePrivateRegisters(X86Assembler &cc, X86Xmm &vec0, X86Xmm &vec1);

// global function helpers
void global_jit_sysInterrupt(int32_t signal);
#ifdef SHARP_PROF_
void global_jit_profile(Profiler* prof);
void global_jit_dump(Profiler* prof);
#endif
void global_jit_suspendSelf(Thread* thread);

void passArg0(X86Assembler &cc, int64_t arg0);
void passArg0(X86Assembler &cc, X86Gp &arg);

#ifdef SHARP_PROF_
void setupProfilerFields(const X86Gp &ctx);
#endif

// quick pass to function args
#define JIT_TSCFIELDS cc,tmp,delegate,thread_check_section

void jit_safetycheck(X86Assembler &cc, X86Mem &ctxPtr, X86Gp &ctx, X86Gp &val, X86Gp &tmp, X86Gp &delegate, X86Xmm &vec0, X86Xmm &vec1,
                     Label &lbl_funcend);

void goto_threadSafetyCheck(X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, Label &tsc, Label &retlbl, bool skipCtx);

enum ConstKind {
    kConst64,
    kConstFloat
};

// low level constant manager
struct Constants {
    List<Data64> constants;
    List<ConstKind> constantKinds;
    List<Label> constantLabels;

    ~Constants() {
        constants.free();
        constantLabels.free();
    }

    int64_t createConstant(X86Assembler& cc, int64_t const0) {
        int64_t idx = _64ConstIndex(const0);

        if(idx == -1) {
            Data64 const_;
            const_.setI64(const0);
            constants.push_back(const_);
            constantKinds.add(kConst64);
            constantLabels.add(cc.newLabel());
            return constants.size();
        }

        return idx;
    }

    int64_t createConstant(X86Assembler& cc, double const0) {
        int64_t idx = _floatConstIndex(const0);

        if(idx == -1) {

            Data64 const_;
            const_.setF64(const0);
            constants.push_back(const_);
            constantKinds.add(kConstFloat);
            constantLabels.add(cc.newLabel());
            return constants.size() - 1;
        }

        return idx;
    }

    int64_t constantSize() {
        return constants.size();
    }

    int64_t _64ConstIndex(int64_t const0) {
        for(int64_t i = 0; i < constants.size(); i++) {
            Data64 &const_ = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConst64 && const_.sq[0] == const0) {
                return i;
            }
        }

        return -1;
    }

    int64_t _floatConstIndex(double const0) {
        for(int64_t i = 0; i < constants.size(); i++) {
            Data64 &const_ = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConstFloat && const_.df[0] == const0) {
                return i;
            }
        }

        return -1;
    }

    Data64& getConstant(int64_t idx) {
        return constants.get(idx);
    }

    Label& getConstantLabel(int64_t idx) {
        return constantLabels.get(idx);
    }

    void emitConstants(X86Assembler& cc) {

        for(uint64_t i = 0; i < constantLabels.size(); i++) {
            cc.bind(constantLabels.get(i));
            Data64 &const0 = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConst64) {
                cc.dint64(const0.sq[0]);
            } else if(kind == kConstFloat) {
                cc.dmm(const0);
            }
        }
    }
};

/**
 * testing suite for Sharp
 */
int compile(Method *method) {
    cout << "===========================JIT\n\n";

    if(method->bytecode != NULL)
    {
        using namespace asmjit::x86;            // Easier access to x86/x64 registers.
        int64_t* bc = method->bytecode;
        int64_t sz = method->cacheSize;

        // make it easier for the JIT Compiler
        method->jit_labels = (int64_t *)malloc(sizeof(int64_t)*method->cacheSize);
        if(!method->jit_labels)
            return jit_error_mem; // "error processing exe: not enough memory"
        else
            std::memset(method->jit_labels, 0, sizeof(int64_t)*method->cacheSize);

        CodeHolder code;                        // Holds code and relocation information.
        code.init(rt.getCodeInfo());            // Initialize to the same arch as JIT runtime.
        std::ofstream outfile ("JIT.s");        // Quickly create file
//        outfile << "";                          // clear it
        outfile.close();

        FILE * pFile;                           // out logging file
        pFile = fopen ("JIT.s" , "rw+");

        FileLogger logger(pFile);
        code.setLogger(&logger);                // Initialize logger temporarily to ensure quality of code

        X86Assembler cc(&code);                  // Create and attach X86Compiler to `code`.

        // Decide which registers will be mapped to function arguments.
        // registers zax for `ctx` is used to store the pointer to the jit context
        X86Gp ctx   = cc.zax();

        X86Gp tmp        = r10;       // setup register locations for local variables
        X86Gp val        = r11;       // virtual registers are r10, r11, r12, and r13
        X86Gp delegate   = r12;       // each time a function is called that can mess with the state of these registers
        X86Gp argsReg    = r13;       // they must be pushed to the stack to be retained
        X86Gp registersReg = r14;     // they must be pushed to the stack to be retained

        // we need these for stack manip
        X86Gp zbp = cc.zbp();
        X86Gp zsp = cc.zsp();


        X86Xmm vec0 = xmm0;
        X86Xmm vec1 = xmm1;

        FuncDetail fd;
        fd.init(FuncSignature1<void, jit_ctx*>(CallConv::kIdHost));

        FuncFrameInfo ffi;
        ffi.setDirtyRegs(X86Reg::kKindVec,      // Make XMM0 and XMM1 dirty. VEC kind
                         Utils::mask(0, 1));    // describes XMM|YMM|ZMM registers.
        ffi.enablePreservedFP();
        ffi.enableCalls();
        ffi.enableMmxCleanup();


        FuncArgsMapper args(&fd);               // Create function arguments mapper.
        args.assignAll(ctx);                    // Assign our registers to arguments.
        args.updateFrameInfo(ffi);              // Reflect our args in FuncFrameInfo.

        FuncFrameLayout layout;                 // Create the FuncFrameLayout, which
        layout.init(fd, ffi);                   // contains metadata of prolog/epilog.

        FuncUtils::emitProlog(&cc, layout);      // Emit function prolog.
        FuncUtils::allocArgs(&cc, layout, args); // Allocate arguments to registers.
        saveCalleeRegisters(cc);

        /**
         * Layout of how our function will look (conceptually)
         *
         * void jit_function0(jit_ctx *ctx) {
         *
         *     int64_t tmp=0;
         *     int64_t val=0;
         *     int64_t delegate=0;
         *     int64_t args=0;
         *     ClassObject *klass;
         *     SharpObject* o;
         *     Object* o2=NULL;
         *     int64_t* labels;'
         *     void *registers;                     // we extrapolate out of the ctx pointer because of being widely used
         *
         *     user code...
         * }
         */
        // allocate space for the stack
        int ptrSize      = sizeof(jit_ctx*), paddr = ptrSize;
        int tmpSize      = sizeof(int64_t);           // yes I know this will be redundant but i want to be a verbose as possible
        int valSize      = sizeof(int64_t);           // to prevent any ambiguity or assumptions
        int delegateSize = sizeof(int64_t);           // the int64_t sizes are omitted for now as they are allocated into a register
        int argsSize     = sizeof(int64_t);
        int registersSize = sizeof(int64_t);
        int labelsSize   = sizeof(int64_t*), laddr = paddr + labelsSize;
        int klassSize    = sizeof(ClassObject*), kaddr = laddr + klassSize;
        int objectSize   = sizeof(SharpObject*), oaddr = kaddr + objectSize;
        int o2Size       = sizeof(Object*), o2addr = oaddr + o2Size;
        int32_t stackSize = ptrSize + /* tmpSize + valSize + delegateSize + argsSize */
                + labelsSize + klassSize + objectSize + o2Size;
        cc.sub(zsp, (stackSize));                  // allocate nessicary bytes on the stack for variables

        Label labels[sz];                       // Each opcode has its own labels but not all labels will be used
        for(int64_t i = 0; i < sz; i++) {       // Iterate through all the addresses to create labels for each address
            labels[i] = cc.newLabel();
        }

        // Emit all assembly code below

        X86Mem ctxPtr = x86::qword_ptr(zbp, -paddr); // store memory location of ctx pointer in the stack
        X86Mem klassPtr = x86::qword_ptr(zbp, -kaddr); // store memory location of klass pointer in the stack
        X86Mem objectPtr = x86::qword_ptr(zbp, -oaddr); // store memory location of o pointer in the stack
        X86Mem o2Ptr = x86::qword_ptr(zbp, -o2addr); // store memory location of o2 pointer in the stack
        X86Mem labelsPtr = x86::qword_ptr(zbp, -laddr); // store memory location of labels* pointer in the stack

        cc.mov(ctxPtr, ctx);                   // send ctx to stack from ctx register via [ESP + paddr].

        // zero out all memory
        cc.xor_(tmp, tmp);                      // technically this is unessicary but it is good practice
        cc.xor_(val, val);                      // really we mainly care about zeroing ou all the pointers
        cc.xor_(delegate, delegate);
        cc.xor_(argsReg, argsReg);
        cc.mov(klassPtr, 0);                    // very important!!
        cc.mov(objectPtr, 0);
        cc.mov(o2Ptr, 0);
        cc.mov(labelsPtr, 0);

        // Setup jit_ctx field offsets
        setupJitContextFields(ctx);

        // setup thread field offsets
        setupThreadContextFields(ctx);

        // setup stack element field offsets
        setupStackElementFields(ctx);

#ifdef SHARP_PROF_
        setupProfilerFields(ctx);
#endif

        // we want fast access
        cc.mov(ctx, ctxPtr);        // move the contex var into register
        cc.mov(registersReg, jit_ctx_fields[jit_field_id_registers]); // ctx->registers (quick "hack" for less instructions)

        // we already have ctx pointer no need to set
        cc.mov(ctx, jit_ctx_fields[jit_field_id_func]); // ctx->func

        // oh god now we have to get labels field
        X86Mem method_jit_fields = x86::qword_ptr(ctx, 0); // You know what im just super lazy so i slapped it straight to the top
        cc.mov(ctx, method_jit_fields); // ctx->func->jit_labels

        // setup some labels
        Label lbl_begin = cc.newNamedLabel("begin", 5);
        Label lbl_end = cc.newNamedLabel("end", 3);
        Label lbl_funcend = cc.newNamedLabel("func_end", 8);
        Label data_section = cc.newNamedLabel(".data", 5);
        Label thread_check_section = cc.newNamedLabel(".thread_ckeck", 13);

        Constants lconsts;                              // local constant holder for function

        cc.mov(labelsPtr, ctx);                         // int64_t* labels = ctx->func->jit_labels;
        X86Mem tmp_jit_labels = x86::qword_ptr(ctx, 0); // labels[0]

        cc.mov(ctx, labelsPtr);                         // get int64_t* labels value
        cc.mov(ctx, tmp_jit_labels);                    // if(ctx->func->jit_labels[0]==0)
        cc.test(ctx, ctx);                              //      goto end;
        cc.jne(lbl_begin);
        cc.nop();
        cc.jmp(lbl_end);
        cc.bind(lbl_begin);
        cc.nop();
        // user code start

        /**
         * Solution for dynamic label problem
         *
         * void jit_function0(jit_ctx *ctx) {
         *     ...
         *     if(ctx->func->jit_labels[0]==0)
         *        goto end;
         *     begin:
         *        user code starts here
         *
         *        goto real_end;
         *     end:
         *        set int64 array to label values
         *        goto begin;
         *
         *     real_end:
         *        return;
         */
        Error error = 0;
        int64_t x64, idx;
        X86Mem lconstMem, tmpMem;
        for(int64_t i = 0; i < sz; i++) {
            if(error) break;
            x64 = *bc;

#ifdef SHARP_PROF_
            cc.mov(ctx, ctxPtr);        // move the contex var into register
            cc.mov(tmp, jit_ctx_fields[jit_field_id_ir]);
            cc.mov(val, qword_ptr(tmp));
            cc.add(val, 1);
            cc.mov(qword_ptr(tmp), val);
            cc.cmp(val, 0);
            Label irNotOverflow = cc.newLabel();
            cc.jne(irNotOverflow);
            cc.mov(tmp, jit_ctx_fields[jit_field_id_overflow]);
            cc.mov(val, qword_ptr(tmp));
            cc.add(val, 1);
            cc.mov(qword_ptr(tmp), val);
            cc.bind(irNotOverflow);
#endif
#define is_op(x) (GET_OP(x64)==(x))
            if(is_op(op_INT) || is_op(op_JNE) || is_op(op_GOTO)) {
                goto_threadSafetyCheck(JIT_TSCFIELDS, labels[i], false);
            }

            cc.bind(labels[i]);
            switch(GET_OP(x64)) {
                case op_NOP: {
                    cc.nop();                   // by far one of the easiest instructions yet
                    break;
                }
                case op_INT: {                  // vm->sysInterrupt(GET_Da(*pc)); if(masterShutdown) return;
                    savePrivateRegisters(cc, vec0, vec1);
#ifdef SHARP_PROF_
//                    if(GET_Da(*pc) == 0xa9) {
//                        tprof.endtm=Clock::realTimeInNSecs();
//                        tprof.profile();
//                        tprof.dump();
//                    }

                    if(GET_Da(x64) == 0xa9) {
                        cc.mov(ctx, ctxPtr);        // move the contex var into register
                        cc.mov(ctx, jit_ctx_fields[jit_field_id_current]); // ctx->current
                        cc.mov(ctx, thread_fields[jit_field_id_thread_tprof]); // ctx->current->tprof
                        cc.mov(argsReg, ctx);       // this register will be saved by the called function

                        cout << "thread->tprof = " << thread_self->tprof << " tprof->endtm = " << &thread_self->tprof->endtm << endl;
                        cout << "thread_self " << thread_self << endl;

                        cc.call((int64_t)Clock::realTimeInNSecs);
                        cc.mov(tmp, ctx);
                        cc.mov(ctx, argsReg);
                        cc.mov(profiler_fields[jit_field_id_profiler_endtm], tmp); // ctx->current->tprof->endtm = Clock::realTimeInNSecs();

                        passArg0(cc, argsReg);
                        cc.call((int64_t)global_jit_profile);

                        passArg0(cc, argsReg);
                        cc.call((int64_t)global_jit_dump);
                    }

#endif

                    // rdi to send rax to get
                    passArg0(cc, GET_Da(x64));

                    cc.call((int64_t)global_jit_sysInterrupt);

                    cc.mov(rbx, (int64_t)&masterShutdown);
                    cc.movzx(ebx, byte_ptr(rbx));
                    cc.test(bl, bl);
                    Label ifFalse = cc.newLabel();
                    cc.je(ifFalse);
                    restorePrivateRegisters(cc, vec0, vec1); // we wnt to make sure the stack is in its correct position
                    returnFuntion();

                    cc.bind(ifFalse);
                    restorePrivateRegisters(cc, vec0, vec1);
                    break;
                }
                case op_MOVI: {                  // registers[*(pc+1)]=GET_Da(*pc);
                    cc.mov(ctx, registersReg);        // move the contex var into register

                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    if(*bc != 0) {
                        cc.add(ctx, (int64_t )(sizeof(double) * (*bc)));
                    }

                    SET_LCONST_DVAL(GET_Da(x64));

                    tmpMem = qword_ptr(ctx);
                    cc.movsd(tmpMem, vec0);
                    break;
                }
                case op_RET: {
                    /**
                     * if(calls <= 1) {
        #ifdef SHARP_PROF_
                        tprof->endtm=Clock::realTimeInNSecs();
                        tprof->profile();
        #endif
                            return;
                        }

                        Frame *frame = callStack+(calls--);

                        if(current->finallyBlocks.len > 0)
                            vm->executeFinally(thread_self->current);

                        current = frame->last;
                        cache = current->bytecode;

                        pc = frame->pc;
                        sp = frame->sp;
                        fp = frame->fp;

        #ifdef SHARP_PROF_
                    tprof->profile();
        #endif
                     */
                    cc.mov(ctx, ctxPtr);        // move the contex var into register
                    cc.mov(ctx, jit_ctx_fields[jit_field_id_current]); // ctx->current
                    cc.mov(tmp, ctx);           // save Thread*

                    cc.mov(ctx, thread_fields[jit_field_id_thread_calls]);
                    cc.cmp(ctx, 1); // if(calls <= 1) {
                    Label ifFalse = cc.newLabel();
                    cc.jnae(ifFalse);
#ifdef SHARP_PROF_

                    savePrivateRegisters(cc, vec0, vec1);
                    cc.mov(ctx, tmp);        // move thread* var into register
                    cc.mov(ctx, thread_fields[jit_field_id_thread_tprof]); // ctx->current->tprof
                    cc.mov(argsReg, ctx);       // this register will be saved by the called function

                    cc.call((int64_t)Clock::realTimeInNSecs);
                    cc.mov(tmp, ctx);
                    cc.mov(ctx, argsReg);
                    cc.mov(profiler_fields[jit_field_id_profiler_endtm], tmp); // ctx->current->tprof->endtm = Clock::realTimeInNSecs();

                    passArg0(cc, argsReg);
                    cc.call((int64_t)global_jit_profile);
                    restorePrivateRegisters(cc, vec0, vec1);

#endif
                    returnFuntion();
                    cc.bind(ifFalse);

                    break;
                }
                case op_ISTOREL: {              // (fp+GET_Da(*pc))->var = *(pc+1);
                    cc.mov(ctx, ctxPtr);        // move the contex var into register
                    cc.mov(ctx, jit_ctx_fields[jit_field_id_current]); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp

                    if(GET_Da(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Da(x64)));
                    }

                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    SET_LCONST_DVAL(*bc);
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_LOADL: {                   // registers[GET_Ca(*pc)]=(fp+GET_Cb(*pc))->var;
                    cc.mov(ctx, ctxPtr);        // move the contex var into register
                    cc.mov(ctx, jit_ctx_fields[jit_field_id_current]); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    cc.mov(tmp, registersReg); // ctx->registers (fast pointer)

                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    tmpMem = stack_element_fields[jit_field_id_stack_element_var];
                    cc.movsd(vec0, tmpMem);


                    if(GET_Ca(x64) != 0) {
                        cc.add(tmp, (int64_t )(sizeof(double) * GET_Ca(x64)));
                    }

                    tmpMem = qword_ptr(tmp);
                    cc.movsd(tmpMem, vec0);
                    break;
                }
                case op_LT: {                     // registers[i64cmt]=registers[GET_Ca(*pc)]<registers[GET_Cb(*pc)];
                    cc.mov(ctx, registersReg);        // move the contex var into register

                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));

                    cc.mov(tmp, registersReg);      // were just using these registers because we can, makes life so much easier
                    if(GET_Ca(x64) != 0) {
                        cc.add(tmp, (int64_t )(sizeof(double) * GET_Ca(x64)));
                    }
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    cc.mov(tmp, registersReg);      // were just using these registers because we can, makes life so much easier
                    if(GET_Cb(x64) != 0) {
                        cc.add(tmp, (int64_t )(sizeof(double) * GET_Cb(x64)));
                    }
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec0, tmpMem);

                    cc.ucomisd(vec0, vec1);

                    Label ifFalse = cc.newLabel();
                    Label ifEnd = cc.newLabel();
                    cc.jbe(ifFalse);
                    idx = lconsts.createConstant(cc, (double)(1));
                    lconstMem = ptr(lconsts.getConstantLabel(idx));

                    cc.movsd(vec0, lconstMem);
                    cc.jmp(ifEnd);
                    cc.bind(ifFalse);

                    cc.pxor(vec0, vec0);
                    cc.bind(ifEnd);

                    tmpMem = qword_ptr(ctx);
                    cc.movsd(tmpMem, vec0);

                    break;
                }
                case op_JNE: { // if(registers[i64cmt]==0) { pc=cache+GET_Da(*pc); _brh_NOINCREMENT }
                    cc.mov(ctx, registersReg);        // move the contex var into register
                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));
                    tmpMem = qword_ptr(ctx);
                    cc.movsd(vec0, tmpMem);
                    cc.pxor(vec1, vec1);
                    cc.ucomisd(vec0, vec1);
                    Label ifEnd = cc.newLabel();
                    cc.jp(ifEnd);
                    cc.pxor(vec1, vec1);
                    cc.ucomisd(vec0, vec1);
                    cc.jne(ifEnd);
                    cc.jmp(labels[GET_Da(x64)]);

                    cc.bind(ifEnd);

                    break;
                }
                case op_IADDL: {                        // (fp+GET_Cb(*pc))->var+=GET_Ca(*pc);
                    cc.mov(ctx, ctxPtr);        // move the contex var into register
                    cc.mov(ctx, jit_ctx_fields[jit_field_id_current]); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    SET_LCONST_DVAL(GET_Ca(x64));
                    cc.addsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_GOTO: {// $                             // pc = cache+GET_Da(*pc);
                    cc.jmp(labels[GET_Da(x64)]);
                    break;
                }
                case op_RETURNVAL: {
                    break;
                }
                default: {
//                    error = 1;
//                    cout << "jit assembler:  error: unidentified opcode (" <<  GET_OP(x64) << ") at [" << i << "]." << endl;
//                    cout << "\tPlease contact the developer with this information immediately." << endl;
                    break;
                }
            }

            bc++;
        }

        cc.jmp(lbl_funcend);                   // if we reach the end of our function we dont want to set the labels again
        cc.bind(lbl_end);
        cc.nop();
        // labeles[] setting here
        cc.comment("; setting label values", 22);
        cc.mov(tmp, labelsPtr);                         // get a temporary ref to int64_t* labels
        X86Mem labelIndex = qword_ptr(tmp);
        X86Mem lbl;
        for(int64_t i = 0; i < sz; i++) {
            lbl = ptr(labels[i]);
            cc.lea(ctx, lbl);
            cc.mov(labelIndex, ctx);

            if((i + 1) < sz)                       // omit unessicary add instruction
                cc.add(tmp, (int64_t )sizeof(int64_t));
        }

        cc.xor_(tmp, tmp);                      // tmp = 0;
        cc.jmp(lbl_begin);                     // jump back to top to execute user code

        // end of function
        cc.bind(lbl_funcend);
        cc.nop();

        restoreCalleeRegisters(cc, stackSize);
        FuncUtils::emitEpilog(&cc, layout);    // Emit function epilog and return.

        /**
         * Thread check section
         *
         * The thread check section is a section responsible for checking weather or
         * not the thread has been killed or put to sleep.
         *
         * The general contract of this code section holds a few rules to be used
         * correctly
         *
         *      %tmp - this register holds weather or not to fetch the jit_ctx
         *      %val - this register is used so if information requires persistance it is the programmers
         *             responsibility
         * %delegate - this register MUST hold the return address to jump back to executing the Jit'ed code
         *
         * we must use goto_threadSafetyCheck() for thread safety check calls
         */
        cc.bind(thread_check_section);
        jit_safetycheck(cc, ctxPtr, ctx, val, tmp, delegate, vec0, vec1, lbl_funcend);

        cc.nop();
        cc.nop();
        cc.nop();
        cc.align(kAlignData, 64);              // Align 64
        cc.bind(data_section);                 // emit constancts to be used
        cc.comment(";\t\tdata section start", 21);
        lconsts.emitConstants(cc);

        if(!error) {

            sjit_function jitfn;
            Error err = rt.add(&jitfn, &code);   // Add the generated code to the runtime.
            if (err) {
                cout << "code error " << err << endl;
                return jit_error_compile;
            }
            else {
                jit_ctx *ctx = &jctx;
                jitfn(ctx);
                jit_func jfunc;
                jfunc.func = jitfn;
                jfunc.serial = jit_funcid++;
                jfunc.rAddr = method->address;
                functions.push_back(jfunc);
            }
        }

        releaseMem(); // this will go away we just use it for now
    }

    return jit_error_ok;
}

void passArg0(X86Assembler &cc, int64_t arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rcx : x86::rdi;  // First argument (array ptr).
    }
    else {
        arg0 = cc.zdx();                       // Use EDX to hold the array pointer.
    }
    cc.mov(arg0, arg); // set signal to send

}

void passArg0(X86Assembler &cc, X86Gp& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rcx : x86::rdi;  // First argument (array ptr).
    }
    else {
        arg0 = cc.zdx();                       // Use EDX to hold the array pointer.
    }
    cc.mov(arg0, arg); // set signal to send

}

void goto_threadSafetyCheck(X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, Label &tsc, Label &retlbl, bool skipCtx) {
    using namespace asmjit::x86;
    cc.mov(tmp, skipCtx ? 1 : 0); // set jit_ctz switch
    cc.lea(delegate, ptr(retlbl)); // set return addr
    cc.jmp(tsc); // call thread_sec proc
}

void jit_safetycheck(X86Assembler &cc, X86Mem &ctxPtr, X86Gp &ctx, X86Gp &val, X86Gp & tmp, X86Gp & delegate, X86Xmm &vec0, X86Xmm &vec1,
                     Label &lbl_funcend) {
    using namespace asmjit::x86;
//#define SAFTEY_CHECK
//    if (suspendPending)
//        suspendSelf();
//    if (state == THREAD_KILLED)
//        return;
    Label skipCtx = cc.newLabel();
    cc.cmp(tmp, 1);
    cc.je(skipCtx);
    cc.mov(ctx, ctxPtr);
    cc.bind(skipCtx);

    cc.mov(ctx, jit_ctx_fields[jit_field_id_current]); // jctx->current
    cc.mov(val, ctx);                                  //  store thread*
    cc.movzx(ebx, thread_fields[jit_field_id_thread_suspendPending]);
    cc.test(bl, bl);
    Label ifFalse1 = cc.newLabel();
    cc.je(ifFalse1);

    savePrivateRegisters(cc, vec0, vec1);
    passArg0(cc, val);

    cc.call((int64_t)global_jit_suspendSelf);
    restorePrivateRegisters(cc, vec0, vec1);
    cc.bind(ifFalse1);

    cc.mov(ctx, val);
    cc.mov(val, thread_fields[jit_field_id_thread_state]);
    cc.cmp(val, THREAD_KILLED);
    Label ifFalse2 = cc.newLabel();
    cc.jne(ifFalse2);
    returnFuntion();

    cc.bind(ifFalse2);
    cc.jmp(delegate); // dynamically jump to last called position
}

void saveCalleeRegisters(X86Assembler &cc) {
    using namespace asmjit::x86;
    /**
      * The registers RAX, RCX, RDX, R8, R9, R10, R11 are considered volatile (caller-saved).[17]
      * The registers RBX, RBP, RDI, RSI, RSP, R12, R13, R14, and R15 are considered nonvolatile (callee-saved).[17]
      *
      * Save these register in out function
      */
    cc.push(cc.zbx());
    cc.push(cc.zdi());
    cc.push(cc.zsi());
    cc.push(r12);
    cc.push(r13);
    cc.push(r14);
    cc.push(r15);
    cc.mov(cc.zbp(), cc.zsp());
}

void restoreCalleeRegisters(X86Assembler &cc, int64_t stackSize) {
    using namespace asmjit::x86;

    cc.add(cc.zsp(), (stackSize));                  // restore stack back to its original state
    /**
      * The registers RAX, RCX, RDX, R8, R9, R10, R11 are considered volatile (caller-saved).[17]
      * The registers RBX, RBP, RDI, RSI, RSP, R12, R13, R14, and R15 are considered nonvolatile (callee-saved).[17]
      *
      * Save these register in out function
      */
    cc.pop(r15);
    cc.pop(r14);
    cc.pop(r13);
    cc.pop(r12);
    cc.pop(cc.zsi());
    cc.pop(cc.zdi());
    cc.pop(cc.zbx());
    cc.mov(cc.zbp(), cc.zsp());                  // restore stack back to its original state
}

/**
  * The registers below are considered volatile and must be preserved.[17]
  *
  * X86Gp ctx   = cc.zax();
  *
  * X86Gp tmp        = r10;       // setup register locations for local variables
  * X86Gp val        = r11;       // virtual registers are r10, r11, r12, and r13
  * X86Gp delegate   = r12;       // No need to save
  * X86Gp argsReg    = r13;       // No need to save
  * X86Gp registersReg = r14;     // No need to save
  *
  * X86Xmm vec0 = xmm0;
  * X86Xmm vec1 = xmm1;
  * Save these registers before function call
  */
void savePrivateRegisters(X86Assembler &cc, X86Xmm &vec0, X86Xmm &vec1) {
    using namespace asmjit::x86;
//    sub     esp, 16
//    movdqu  dqword [esp], xmm0
    cc.sub(cc.zsp(), 16);                           // do we even need to save these dirty registers?
    cc.movdqu(dqword_ptr(cc.zsp()), vec0);
    cc.sub(cc.zsp(), 16);
    cc.movdqu(dqword_ptr(cc.zsp()), vec1);

    cc.push(r10);
    cc.push(r11);
}

void restorePrivateRegisters(X86Assembler &cc, X86Xmm &vec0, X86Xmm &vec1) {
    using namespace asmjit::x86;

    cc.pop(r11);
    cc.pop(r10);

    cc.movdqu(vec1, dqword_ptr(cc.zsp()));
    cc.add(cc.zsp(), 16);
    cc.movdqu(vec0, dqword_ptr(cc.zsp()));
    cc.add(cc.zsp(), 16);
}

/**
 * Global helper functions for easier interfacing
 * because im lazy :)
 *
 * @param signal signal to Virtual Machine
 */
void global_jit_sysInterrupt(int32_t signal) {
    vm->sysInterrupt(signal);
}

#ifdef SHAR_PROF_
void global_jit_profile(Profiler* prof) {
    prof->profile();
}

void global_jit_dump(Profiler* prof) {
    prof->dump();
}
#endif

void global_jit_suspendSelf(Thread* thread) {
    thread->suspendSelf();
}

void setupJitContextFields(const X86Gp &ctx) {
    int64_t sz = 0; // holds the growing size of the data
    jit_ctx_fields[jit_field_id_current] = x86::qword_ptr(ctx, relative_offset((&jctx), current, current)); // Thread *current
    jit_ctx_fields[jit_field_id_registers] = x86::qword_ptr(ctx, relative_offset((&jctx), current, registers)); // double *registers
    jit_ctx_fields[jit_field_id_func] = x86::qword_ptr(ctx, relative_offset((&jctx), current, func)); // Method *func

#ifdef SHARP_PROF_
    jit_ctx_fields[jit_field_id_ir] = x86::qword_ptr(ctx, relative_offset((&jctx), current, irCount)); // unsigned long long *irCount
    jit_ctx_fields[jit_field_id_overflow] = x86::qword_ptr(ctx, relative_offset((&jctx), current, overflow)); // unsigned long long *overflow
#endif
}

void setupThreadContextFields(const X86Gp &ctx) {
    Thread* thread = thread_self;
    thread_fields[jit_field_id_thread_calls] = x86::qword_ptr(ctx, relative_offset(thread, calls, calls)); // unsigned long calls
    thread_fields[jit_field_id_thread_dataStack] = x86::qword_ptr(ctx, relative_offset(thread, calls, dataStack)); // StackElement *dataStack;
    thread_fields[jit_field_id_thread_sp] = x86::qword_ptr(ctx, relative_offset(thread, calls, sp));  // StackElement *sp
    thread_fields[jit_field_id_thread_fp] = x86::qword_ptr(ctx, relative_offset(thread, calls, fp)); // StackElement *fp
    thread_fields[jit_field_id_thread_current] = x86::qword_ptr(ctx, relative_offset(thread, calls, current)); // Method *current
    thread_fields[jit_field_id_thread_callStack] = x86::qword_ptr(ctx, relative_offset(thread, calls, callStack)); // Frame *callStack
    thread_fields[jit_field_id_thread_stack_lmt] = x86::qword_ptr(ctx, relative_offset(thread, calls, stack_lmt));  // unsigned long stack_lmt
    thread_fields[jit_field_id_thread_cache] = x86::qword_ptr(ctx, relative_offset(thread, calls, cache)); // int64_t *cache
    thread_fields[jit_field_id_thread_pc] = x86::qword_ptr(ctx, relative_offset(thread, calls, pc)); // int64_t *pc

#ifdef SHARP_PROF_
    thread_fields[jit_field_id_thread_tprof] = x86::qword_ptr(ctx, relative_offset(thread, calls, tprof)); // Profiler *tprof
#endif
    thread_fields[jit_field_id_thread_suspendPending] = x86::byte_ptr(ctx, relative_offset(thread, calls, suspendPending)); // bool suspendPending
    thread_fields[jit_field_id_thread_state] = x86::dword_ptr(ctx, relative_offset(thread, calls, state)); // unsigned int state
}

void setupStackElementFields(const X86Gp &ctx) {
    int64_t sz = 0; // holds the growing size of the data
    StackElement stack;
    stack_element_fields[jit_field_id_stack_element_var] = x86::qword_ptr(ctx, relative_offset((&stack), var, var)); // double var;
    stack_element_fields[jit_field_id_stack_element_object] = x86::qword_ptr(ctx, relative_offset((&stack), var, object)); // Object object;
}

#ifdef SHARP_PROF_
void setupProfilerFields(const X86Gp &ctx) {
    int64_t sz = 0; // holds the growing size of the data
    Profiler prof;
    profiler_fields[jit_field_id_profiler_totalHits] = x86::qword_ptr(ctx, relative_offset((&prof), totalHits, totalHits)); // size_t totalHits;
    profiler_fields[jit_field_id_profiler_starttm] = x86::qword_ptr(ctx, relative_offset((&prof), totalHits, starttm)); // int64_t starttm;
    profiler_fields[jit_field_id_profiler_endtm] = x86::qword_ptr(ctx, relative_offset((&prof), totalHits, endtm)); // int64_t endtm;
    profiler_fields[jit_field_id_profiler_lastHit] = x86::qword_ptr(ctx, relative_offset((&prof), totalHits, lastHit)); // size_t lastHit;
}
#endif

void releaseMem()
{
    for(std::vector<jit_func>::iterator it = functions.begin(); it != functions.end(); ++it) {
        rt.release((*it).func);
    }
}


void call(jit_ctx *ctx, int64_t serial)
{

}