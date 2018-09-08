//
// Created by BraxtonN on 8/9/2018.
//

#include "jit.h"
#include "Opcode.h"
#include "Thread.h"
#include "../util/KeyPair.h"
#include "register.h"
#include "VirtualMachine.h"
#include "Environment.h"
#include "Manifest.h"
#include "init.h"
#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <conio.h>

using namespace asmjit;

/**
 * Global reference to our runtime engine
 */
JitRuntime rt;
// this will hold all the functions pending compile
List<jit_func> functions;
std::recursive_mutex jit_mut;

X86Mem jit_ctx_fields[6]; // memory map of jit_ctx
X86Mem thread_fields[12]; // sliced memory map of Thread
X86Mem frame_fields[4]; // memory map of Frame
X86Mem stack_element_fields[2]; // memory map of StackElement
X86Mem method_fields[1]; // memory map of Method
X86Mem sharp_object_fields[4]; // memory map of Method
#ifdef SHARP_PROF_
X86Mem profiler_fields[4]; // memory map of Profiler
#endif

// setup functions
void setupJitContextFields(const X86Gp &ctx);
void setupThreadContextFields(const X86Gp &ctx);
void setupStackElementFields(const X86Gp &ctx);
void setupFrameFields(const X86Gp &ctx);
void setupMethodFields(const X86Gp &ctx);
void setupSharpObjectFields(const X86Gp &ctx);

// state save functions
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
unsigned long global_jit_finallyBlocks(Method* fn);
int global_jit_executeFinally();
SharpObject* global_jit_newObject0(int64_t size);
void global_jit_newClass0(Thread* thread, int64_t classid);
void global_jit_newArray0(Thread* thread, int64_t reg);
void global_jit_pushNil(Thread* thread);
void global_jit_pushl(Thread* thread, int64_t offset);
void global_jit_newStr(Thread* thread, int64_t strid);
void global_jit_newClass1(Thread* thread, int64_t reg, int64_t classid);
void global_jit_setObject0(StackElement *sp, SharpObject* o);
void global_jit_setObject1(StackElement *sp, Object* o);
void global_jit_setObject2(StackElement *sp, Object* o);
void global_jit_castObject(Object *o2, int64_t klass);
void global_jit_imod(int64_t op0, int64_t op1);
void global_jit_put(int op0);
void global_jit_putc(int op0);
int64_t global_jit_getpc();
void global_jit_get(int op0);
void global_jit_popl(Object* o2, SharpObject* o);
void global_jit_del(Object* o);
void global_jit_lock(Object* o);
void global_jit_ulock(Object* o);
void global_jit_call0(Thread *thread, int64_t addr);
void global_jit_call1(Thread* thread, int64_t addr);

// global exception handling functions
void __srt_cxxa_prepare_throw(Exception &e);
int __srt_cxxa_try_catch(Method *method);
void __srt_cxxa_throw_nullptr();
void __srt_cxxa_throw_exception(Thread *thread);
void __srt_cxxa_throw_indexOutOfBounds(int reg, int64_t size);

// function calling helper methods
void passArg0(X86Assembler &cc, int64_t arg0);
void passArg0(X86Assembler &cc, X86Gp &arg);
void passArg0(X86Assembler &cc, X86Mem &arg);
void passArg1(X86Assembler &cc, int64_t arg0);
void passArg1(X86Assembler &cc, X86Gp &arg);
void passArg1(X86Assembler &cc, X86Mem &arg);
void passArg2(X86Assembler &cc, int64_t arg0);
void passArg2(X86Assembler &cc, X86Gp &arg);
void passArg2(X86Assembler &cc, X86Mem &arg);
void passReturnArg(X86Assembler &cc, X86Gp& arg);

#ifdef SHARP_PROF_
void setupProfilerFields(const X86Gp &ctx);
#endif

// quick pass to function args
#define JIT_TCSFIELDS cc,tmp,delegate,threadReg,ctx,argsReg,i,thread_check_section

void jit_safetycheck(X86Assembler &cc, X86Mem &ctxPtr, X86Gp &ctx, X86Gp &val, X86Gp &tmp, X86Gp &delegate, X86Xmm &vec0, X86Xmm &vec1,
                     X86Gp &threadPtr, X86Gp &argsReg, Label &lbl_funcend, X86Mem &labelsPtr);

void goto_threadSafetyCheck(X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp &argsReg, int64_t pc, Label &tsc, Label &retlbl);

// high level interfacing methods
void
getRegister(X86Assembler &cc, const X86Gp &tmp, const X86Gp &registersReg, int idx, X86Mem &tmpMem, const X86Xmm &vec);

void
setRegister(X86Assembler &cc, const X86Gp &ctx, const X86Gp &registersReg, const X86Xmm &vec0, const int64_t reg);

void
doubleToInt64(X86Assembler &cc, const X86Gp &dest, const X86Xmm &src);

void
int64ToDouble(X86Assembler &cc, const X86Xmm &dest, const X86Gp &src);

void
jmpToLabel(X86Assembler &cc, const X86Gp &idx, const X86Gp &dest, X86Mem &labelsPtr);

FILE *getLogFile();


void incrementPc(X86Assembler &cc, const X86Gp &ctx, const X86Gp &val, const X86Gp &threadReg);

void
emitReturnOp(X86Assembler &cc, X86Gp &tmp, X86Gp &val, X86Gp &threadReg, X86Gp &ctx, X86Gp &delegate, X86Gp &argsReg,
             X86Xmm &vec0, X86Xmm &vec1, const Label &lbl_funcend);

void checkMasterShutdown(X86Assembler &cc, X86Gp &argsReg, int64_t pc, const Label &lbl_funcend);

void updateThreadPc(X86Assembler &cc, const X86Gp &ctx, const X86Gp &val, const X86Gp &threadPtr, const X86Gp &argsReg);

void safeGuard_CheckNull(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack);

void safeGuard_CheckNull2(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack);

void safeGuard_CheckNullObj(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack);

void safeGuard_CheckINull(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack);

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

void jit_shutdown() {
    releaseMem(); // let go my little functions!
}

void jit_setup() {
    using namespace asmjit::x86;            // Easier access to x86/x64 registers.


    // Decide which registers will be mapped to function arguments.
    // registers zax for `ctx` is used to store the pointer to the jit context
    X86Gp ctx   = rax;

    // These values will never change so we setup once so we dont
    // perform any unsessicary work
    // Setup jit_ctx field offsets
    setupJitContextFields(ctx);

    // setup thread field offsets
    setupThreadContextFields(ctx);

    // setup stack element field offsets
    setupStackElementFields(ctx);

    // setup frame field offsets
    setupFrameFields(ctx);

    // setup method field offsets
    setupMethodFields(ctx);

    // setup SharpObject field offsets
    setupSharpObjectFields(ctx);

    /**
     * Initially compile all required JIT functions
     */
    performInitialCompile();
}

void jit_tls_setup() {
    Thread* thread = thread_self;
    thread->jctx->overflow = &overflow;
    thread->jctx->irCount = &irCount;
    thread->jctx->registers = registers;
    thread->jctx->current = thread_self;
}

int try_jit(Method* func) {
    int error;
    if(!func->isjit && func->jitAttempts < JIT_ATTEMPT_THRESHOLD)
    {
        if((error = compile(func)) != jit_error_ok) {
            func->jitAttempts++;
            return error;
        }
    } else if(!func->isjit)
        return jit_error_max_attm;

    return jit_error_ok;
}

/**
 * Initially compile all methods that need to be jit'ed
 */
void performInitialCompile()
{
    for(int64_t i = 0; i < manifest.methods; i++) {
        if(c_options.jit && (c_options.slowBoot || env->methods[i].isjit)) {
            // we want to compile it
            env->methods[i].isjit=false;
            try_jit(env->methods+i);
        } else
            env->methods[i].isjit=false;
    }
}

/**
 * JIT compiler function used to compile function from scratch
 * Currently and for the forseable future sharp will only support JIT of
 * x86/64 processors as well as x86/64 ARM for both windows and Linux
 *
 */
int compile(Method *method) {

    if(c_options.jit && method->bytecode != NULL && method->cacheSize >= JIT_IR_MIN)
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

        FileLogger logger(getLogFile());
        code.setLogger(&logger);                // Initialize logger temporarily to ensure quality of code

        X86Assembler cc(&code);                  // Create and attach X86Assembler to `code`.

        stringstream sstream;
        sstream << "; method " << method->fullName.str() << endl;
        string msg = sstream.str();
        cc.comment(msg.c_str(), msg.size());
        // Decide which registers will be mapped to function arguments.
        // registers zax for `ctx` is used to store the pointer to the jit context
        X86Gp ctx   = cc.zax();

        X86Gp tmp        = r10;       // setup register locations for local variables
        X86Gp val        = r11;       // virtual registers are r10, r11, r12, r13, and r14
        X86Gp delegate   = r12;       // each time a function is called that can mess with the state of these registers
        X86Gp argsReg    = r13;       // they must be pushed to the stack to be retained
        X86Gp registersReg = r14;     //
        X86Gp threadReg = r15;        //

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
//        ffi.enablePreservedFP();
        ffi.enableCalls();
        ffi.enableMmxCleanup();


        FuncArgsMapper args(&fd);               // Create function arguments mapper.
        args.assignAll(ctx);                    // Assign our registers to arguments.
        args.updateFrameInfo(ffi);              // Reflect our args in FuncFrameInfo.

        FuncFrameLayout layout;                 // Create the FuncFrameLayout, which
        layout.init(fd, ffi);                   // contains metadata of prolog/epilog.

        FuncUtils::emitProlog(&cc, layout);      // Emit function prolog.
        cc.push(zbp);
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
        int threadSize   = sizeof(int64_t);
        int labelsSize   = sizeof(int64_t*), laddr = paddr + labelsSize;
        int klassSize    = sizeof(ClassObject*), kaddr = laddr + klassSize;
        int objectSize   = sizeof(SharpObject*), oaddr = kaddr + objectSize;
        int o2Size       = sizeof(Object*), o2addr = oaddr + o2Size;
        int32_t stackSize = ptrSize + /* tmpSize + valSize + delegateSize + argsSize */
                            + labelsSize + klassSize + objectSize + o2Size;
        cc.sub(zsp, (stackSize));                  // allocate nessicary bytes on the stack for variables

        Label labels[sz];                       // Each opcode has its own label but not all labels will be used
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

#ifdef SHARP_PROF_
        setupProfilerFields(ctx);
#endif

        // we want fast access
        cc.mov(ctx, ctxPtr);        // move the context var into register
        cc.mov(registersReg, jit_ctx_fields[jit_field_id_registers]); // ctx->registers (quick "hack" for less instructions)

        cc.mov(ctx, ctxPtr);        // move the context var into register
        cc.mov(threadReg, jit_ctx_fields[jit_field_id_current]); // ctx->registers (quick "hack" for less instructions)

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
        Label thread_check_section = cc.newNamedLabel(".thread_check", 13);

        Constants lconsts;                              // local constant holder for function

        cc.mov(labelsPtr, ctx);                         // int64_t* labels = ctx->func->jit_labels;
        X86Mem tmp_jit_labels = x86::qword_ptr(ctx, 0); // labels[0]

        // get int64_t* labels value (ctx already has it)
        cc.mov(ctx, tmp_jit_labels);                    // if(ctx->func->jit_labels[0]==0)
        cc.test(ctx, ctx);                              //      goto end;
        cc.jne(lbl_begin);
        cc.jmp(lbl_end);
        cc.bind(lbl_begin);
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
            if(is_op(op_JNE) || is_op(op_GOTO) || is_op(op_BRH)
               || is_op(op_IFE) || is_op(op_IFNE)) {
                goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
            }

            cc.bind(labels[i]);
            switch(GET_OP(x64)) {
                case op_NOP: {
                    cc.nop();                   // by far one of the easiest instructions yet
                    break;
                }
                case op_INT: {                  // vm->sysInterrupt(GET_Da(*pc)); if(masterShutdown) return;

                    cc.comment("; int", 5);
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

                        cc.call((int64_t)Clock::realTimeInNSecs);
                        cc.mov(tmp, ctx); // pass returnVal()
                        cc.mov(ctx, argsReg);
                        cc.mov(profiler_fields[jit_field_id_profiler_endtm], tmp); // ctx->current->tprof->endtm = Clock::realTimeInNSecs();

                        passArg0(cc, argsReg);
                        cc.call((int64_t)global_jit_profile);

                        passArg0(cc, argsReg);
                        cc.call((int64_t)global_jit_dump);
                    }

#endif

                    passArg0(cc, GET_Da(x64));
                    cc.call((int64_t)global_jit_sysInterrupt);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);

                    // lets just make sure we are good
                    checkMasterShutdown(cc, argsReg, i, lbl_funcend);
                    break;
                }
                case op_MOVI: {                  // registers[*(pc+1)]=GET_Da(*pc);
                    cc.comment("; movi", 6);
                    int64_t num = GET_Da(x64);
                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway

                    SET_LCONST_DVAL(num);
                    setRegister(setRegisterParams(vec0, *bc));
                    break;
                }
                case op_RET: {
                    cc.comment("; ret", 5);
                    cc.mov(argsReg, i);
                    returnFunction()
                    break;
                }
                case op_HLT: {
                    cc.comment("; hlt", 5);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(thread_fields[jit_field_id_thread_state], THREAD_KILLED); // kill ourselves
                    cc.mov(argsReg, i);
                    returnFunction();
                    break;
                }
                case op_NEWARRAY: { // (++sp)->object = GarbageCollector::self->newObject(registers[GET_Da(*pc)]);
                    cc.comment("; newarray", 10);
                    getRegister(registerParams(vec0, GET_Da(x64)));
                    doubleToInt64(cc, tmp, vec0);

                    passArg0(cc, tmp);
                    cc.call((int64_t)global_jit_newObject0);
                    passReturnArg(cc, tmp);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);

                    cc.mov(ctx, threadReg);
                    cc.mov(val, thread_fields[jit_field_id_thread_sp]);
                    cc.lea(val, ptr(val, sizeof(StackElement)));
                    cc.mov(thread_fields[jit_field_id_thread_sp], val);

                    passArg0(cc, val);
                    passArg1(cc, tmp);
                    cc.call((int64_t)global_jit_setObject0);
                    break;
                } // TODO: after the exception system is in place support stack checks after instructions
                case op_CAST: { // CHECK_NULL(o2->castObject(registers[GET_Da(*pc)]);)
                    cc.comment("; cast", 6);
                    getRegister(registerParams(vec0, GET_Da(x64)));
                    cc.mov(ctx, o2Ptr);
                    doubleToInt64(cc, tmp, vec0);

                    passArg0(cc, ctx);
                    passArg1(cc, tmp);
                    cc.call((int64_t)global_jit_castObject);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }
                case op_VARCAST: {
                    cc.comment("; varcast", 6);
                    cc.nop(); // TODO: unsupported for now add exception system to update opcodes
                    break;
                }
                case op_MOV8: { // registers[GET_Ca(*pc)]=(int8_t)registers[GET_Cb(*pc)];
                    cc.comment("; mov8", 6);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, eax, vec0);
                    cc.movsx(eax, al);
                    int64ToDouble(cc, vec0, eax);

                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_MOV16: { //registers[GET_Ca(*pc)]=(int16_t)registers[GET_Cb(*pc)];
                    cc.comment("; mov16", 7);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, rax, vec0);
                    cc.cwde();
                    int64ToDouble(cc, vec0, ax);
                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }// 1215752192
                case op_MOV32:
                case op_MOV64: { // registers[GET_Ca(*pc)]=(int32_t)registers[GET_Cb(*pc)];
                    cc.comment("; mov32-64", 10);

                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, rax, vec0);

                    if(is_op(op_MOV32)) {
                        cc.mov(rax, eax); // set lower 32 bits
                    }

                    int64ToDouble(cc, vec0, rax);
                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_MOVU8: { // registers[GET_Ca(*pc)]=(uint8_t)registers[GET_Cb(*pc)];
                    cc.comment("; movu8", 7);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, eax, vec0);
                    cc.movzx(eax, al);
                    int64ToDouble(cc, vec0, eax);

                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_MOVU16: { // registers[GET_Ca(*pc)]=(uint16_t)registers[GET_Cb(*pc)];
                    cc.comment("; movu16", 8);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, eax, vec0);
                    cc.movzx(eax, ax);
                    int64ToDouble(cc, vec0, eax);

                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_MOVU32: { // registers[GET_Ca(*pc)]=(uint32_t)registers[GET_Cb(*pc)];
                    cc.comment("; movu32", 8);
                    cc.pxor(vec0, vec0);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, rax, vec0);
                    cc.mov(eax, eax);
                    int64ToDouble(cc, vec0, rax);

                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_MOVU64: {
                    cc.comment("; movu64", 8);
                    cc.nop(); // TODO: unsupported for now its too complicated
                    break;
                }
                case op_RSTORE: { // (++sp)->var = registers[GET_Da(*pc)];
                    cc.comment("; rstore", 8);
                    getRegister(registerParams(vec0, GET_Da(*bc)));
                    cc.mov(ctx, threadReg);
                    cc.add(thread_fields[jit_field_id_thread_sp], (int64_t)sizeof(StackElement));
                    cc.mov(val, thread_fields[jit_field_id_thread_sp]);
                    cc.movsd(qword_ptr(val), vec0);
                    break;
                }
                case op_ADD: { //registers[*(pc+1)]=registers[GET_Ca(*pc)]+registers[GET_Cb(*pc)];
                    cc.comment("; add", 5);
                    getRegister(registerParams(vec1, GET_Ca(*bc)));
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    cc.addsd(vec0, vec1);

                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    setRegister(setRegisterParams(vec0, *bc));
                    break;
                }
                case op_SUB: { //registers[*(pc+1)]=registers[GET_Ca(*pc)]-registers[GET_Cb(*pc)];
                    cc.comment("; sub", 5);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    getRegister(registerParams(vec1, GET_Cb(*bc)));
                    cc.subsd(vec0, vec1);

                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    setRegister(setRegisterParams(vec0, *bc));
                    break;
                }
                case op_MUL: { //registers[*(pc+1)]=registers[GET_Ca(*pc)]*registers[GET_Cb(*pc)];
                    cc.comment("; mul", 5);
                    getRegister(registerParams(vec1, GET_Ca(*bc)));
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    cc.mulsd(vec0, vec1);

                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    setRegister(setRegisterParams(vec0, *bc));
                    break;
                }
                case op_DIV: { //registers[*(pc+1)]=registers[GET_Ca(*pc)]/registers[GET_Cb(*pc)];
                    cc.comment("; div", 5);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    getRegister(registerParams(vec1, GET_Cb(*bc)));
                    cc.divsd(vec0, vec1);

                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    setRegister(setRegisterParams(vec0, *bc));
                    break;
                }
                case op_MOD: { // registers[*(pc+1)]=(int64_t)registers[GET_Ca(*pc)]%(int64_t)registers[GET_Cb(*pc)];
                    cc.comment("; mod", 5);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rax, vec0);

                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, rcx, vec0);

                    cc.cqo();
                    cc.idiv(rcx);
                    cc.mov(rax, rdx);

                    int64ToDouble(cc, vec0, rax);

                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    setRegister(setRegisterParams(vec0, *bc));
                    break;
                }
                case op_IADD: { // registers[GET_Ca(*pc)]+=GET_Cb(*pc);
                    cc.comment("; iadd", 6);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));

                    SET_LCONST_DVAL2(vec1, GET_Cb(*bc));
                    cc.addsd(vec1, vec0);
                    setRegister(setRegisterParams(vec1, GET_Ca(*bc)));
                    break;
                }
                case op_ISUB: { // registers[GET_Ca(*pc)]-=GET_Cb(*pc);
                    cc.comment("; isub", 6);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));

                    SET_LCONST_DVAL2(vec1, GET_Cb(*bc));
                    cc.subsd(vec1, vec0);
                    setRegister(setRegisterParams(vec1, GET_Ca(*bc)));
                    break;
                }
                case op_IMUL: { // registers[GET_Ca(*pc)]*=GET_Cb(*pc);
                    cc.comment("; imul", 6);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));

                    SET_LCONST_DVAL2(vec1, GET_Cb(*bc));
                    cc.mulsd(vec1, vec0);
                    setRegister(setRegisterParams(vec1, GET_Ca(*bc)));
                    break;
                }
                case op_IDIV: { // registers[GET_Ca(*pc)]/=GET_Cb(*pc);
                    cc.comment("; idiv", 6);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));

                    SET_LCONST_DVAL2(vec1, GET_Cb(*bc));
                    cc.divsd(vec1, vec0);
                    setRegister(setRegisterParams(vec1, GET_Ca(*bc)));
                    break;
                }
                case op_IMOD: { // registers[GET_Ca(*pc)]=(int64_t)registers[GET_Ca(*pc)]%(int64_t)GET_Cb(*pc);

                    cc.comment("; imod", 6);
                    passArg0(cc, GET_Ca(*bc));
                    passArg1(cc, GET_Cb(*bc));

                    cc.call((int64_t)global_jit_imod);
                    break;
                }
                case op_POP: {// --sp;
                    cc.comment("; pop", 5);
                    cc.mov(ctx, threadReg);
                    cc.mov(val, thread_fields[jit_field_id_thread_sp]);
                    cc.lea(val, ptr(val, -((int64_t)sizeof(StackElement))));
                    cc.mov(thread_fields[jit_field_id_thread_sp], val);

                    break;
                }
                case op_INC: { // registers[GET_Da(*pc)]++;
                    cc.comment("; inc", 5);
                    getRegister(registerParams(vec0, GET_Da(*bc)));

                    SET_LCONST_DVAL2(vec1, 1);
                    cc.addsd(vec1, vec0);

                    setRegister(setRegisterParams(vec1, GET_Da(*bc)));
                    break;
                }
                case op_DEC: { // registers[GET_Da(*pc)]--;
                    cc.comment("; dec", 5);
                    getRegister(registerParams(vec0, GET_Da(*bc)));

                    SET_LCONST_DVAL2(vec1, 1);
                    cc.subsd(vec0, vec1);

                    setRegister(setRegisterParams(vec0, GET_Da(*bc)));
                    break;
                }
                case op_MOVR: { // registers[GET_Ca(*pc)]=registers[GET_Cb(*pc)];
                    cc.comment("; movr", 6);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_IALOAD: {
                    cc.comment("; iaload", 8);
                    /*
                     *
                     * o = sp->object.object;
                       if(o != NULL && o->HEAD != NULL) {
                           registers[GET_Ca(*pc)] = o->HEAD[(int64_t)registers[GET_Cb(*pc)]];
                       } else throw Exception(Environment::NullptrException, "");
                     */
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_sp]); // ctx->current->sp
                    cc.mov(ctx, stack_element_fields[jit_field_id_stack_element_object]);

                    cc.cmp(ctx, 0); // tmp holds sp->object.object
                    Label ifTrue = cc.newLabel();
                    cc.je(ifTrue);

                    cc.mov(ctx, qword_ptr(ctx)); // o->HEAD != NULL
                    cc.test(ctx, ctx);
                    cc.je(ifTrue);

                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, val, vec0);
                    Label ifTrue2 = cc.newLabel();
                    cc.cmp(val, 0);
                    cc.je(ifTrue2);
                    cc.imul(val, (size_t)sizeof(double));
                    cc.add(ctx, val);
                    cc.bind(ifTrue2);

                    cc.movsd(vec0, qword_ptr(ctx));

                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    cc.bind(ifTrue); // were not doing exceptions right now
                    break;
                }
                case op_BRH: { //  pc=cache+(int64_t)registers[i64adx];
                    cc.comment("; brh", 5);
                    getRegister(registerParams(vec0, i64adx));
                    doubleToInt64(cc, val, vec0);

                    jmpToLabel(cc, val, tmp, labelsPtr);
                    break;
                }
                case op_IFE: {
                    cc.comment("; ife", 5);
                    Label ifTrue = cc.newLabel();
                    getRegister(registerParams(vec0, i64cmt));
                    doubleToInt64(cc, tmp, vec0);
                    cc.cmp(tmp, 0);
                    cc.je(ifTrue);

                    getRegister(registerParams(vec0, i64adx));
                    doubleToInt64(cc, val, vec0);

                    jmpToLabel(cc, val, tmp, labelsPtr);
                    cc.bind(ifTrue);
                    break;
                }
                case op_IFNE: {

                    cc.comment("; ifne", 6);
                    Label ifTrue = cc.newLabel();
                    getRegister(registerParams(vec0, i64cmt));
                    doubleToInt64(cc, tmp, vec0);
                    cc.cmp(tmp, 0);
                    cc.jne(ifTrue);

                    getRegister(registerParams(vec0, i64adx));
                    doubleToInt64(cc, val, vec0);

                    jmpToLabel(cc, val, tmp, labelsPtr);
                    cc.bind(ifTrue);
                    break;
                }
                case op_ISTOREL: {              // (fp+GET_Da(*pc))->var = *(pc+1);
                    cc.comment("; storel", 8);
                    cc.mov(ctx, threadReg); // ctx->current
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
                    cc.comment("; loadl", 7);
                    cc.mov(ctx, threadReg); // ctx->current
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
                case op_JNE: { // if(registers[i64cmt]==0) { pc=cache+GET_Da(*pc); _brh_NOINCREMENT }

                    cc.comment("; jne", 5);
                    tmpMem = qword_ptr(registersReg, (int64_t )(sizeof(double) * i64cmt));
                    cc.pxor(vec0, vec0);
                    cc.ucomisd(vec0, tmpMem);
                    Label ifEnd = cc.newLabel();
                    cc.jp(ifEnd);
                    cc.jne(ifEnd);
                    cc.jmp(labels[GET_Da(x64)]);

                    cc.bind(ifEnd);

                    break;
                }
                case op_IADDL: {                        // (fp+GET_Cb(*pc))->var+=GET_Ca(*pc);
                    cc.comment("; iaddl", 7);
                    cc.mov(ctx, threadReg); // ctx->current
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
                case op_ADDL: {                        // (fp+GET_Cb(*pc))->var+=registers[GET_Ca(*pc)];
                    cc.comment("; iaddl", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    cc.addsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_LT: {                     // registers[i64cmt]=registers[GET_Ca(*pc)]<registers[GET_Cb(*pc)];

                    cc.comment("; lt", 4);
                    cc.mov(ctx, registersReg);        // move the contex var into register
                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));

                    getRegister(registerParams(vec1, GET_Ca(x64)));
                    getRegister(registerParams(vec0, GET_Cb(x64)));

                    cc.comisd(vec0, vec1);

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
                case op_GT: {                     // registers[i64cmt]=registers[GET_Ca(*pc)]<registers[GET_Cb(*pc)];

                    cc.comment("; gt", 4);
                    cc.mov(ctx, registersReg);        // move the contex var into register
                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    getRegister(registerParams(vec1, GET_Cb(x64)));

                    cc.comisd(vec0, vec1);

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
                case op_LTE: {

                    cc.comment("; lte", 5);
                    cc.mov(ctx, registersReg);        // move the contex var into register
                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));

                    getRegister(registerParams(vec1, GET_Ca(x64)));
                    getRegister(registerParams(vec0, GET_Cb(x64)));

                    cc.comisd(vec0, vec1);

                    Label ifFalse = cc.newLabel();
                    Label ifEnd = cc.newLabel();
                    cc.jb(ifFalse);
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
                case op_GTE: {
                    cc.comment("; gte", 5);
                    cc.mov(ctx, registersReg);        // move the contex var into register
                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    getRegister(registerParams(vec1, GET_Cb(x64)));

                    cc.comisd(vec0, vec1);

                    Label ifFalse = cc.newLabel();
                    Label ifEnd = cc.newLabel();
                    cc.jb(ifFalse);
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
                case op_MOVL: { // o2 = &(fp+GET_Da(*pc))->object;
                    cc.comment("; movl", 6);

                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp

                    if(GET_Da(*bc) != 0) {
                        cc.add(ctx, (int64_t)(GET_Da(*bc)*sizeof(StackElement)));
                    }

                    cc.add(ctx, (size_t)sizeof(double));
                    cc.mov(o2Ptr, ctx);
                    break;
                }
                case op_POPL: { // (fp+GET_Da(*pc))->object = (sp--)->object.object;
                    cc.comment("; popl", 6);

                    cc.mov(ctx, threadReg); // ctx->current

                    cc.mov(val, thread_fields[jit_field_id_thread_sp]); // ctx->current->sp
                    cc.lea(delegate, ptr(val, (int32_t)-sizeof(StackElement)));
                    cc.mov(thread_fields[jit_field_id_thread_sp], delegate); // sp--

                    cc.mov(ctx, val);
                    cc.mov(val, stack_element_fields[jit_field_id_stack_element_object]);

                    cc.mov(ctx, threadReg);
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp

                    if(GET_Da(*bc) != 0) {
                        cc.add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(*bc))); // fp+GET_DA(*pc)
                    }

                    cc.lea(ctx, stack_element_fields[jit_field_id_stack_element_object]);

                    passArg0(cc, ctx);
                    passArg1(cc, val);
                    cc.call((int64_t)global_jit_popl);
                    break;
                }
                case op_IPOPL: { // (fp+GET_Da(*pc))->var = (sp--)->var;
                    cc.comment("; popl", 6);

                    cc.mov(ctx, threadReg); // ctx->current

                    cc.mov(val, thread_fields[jit_field_id_thread_sp]); // ctx->current->sp
                    cc.lea(delegate, ptr(val, (int32_t)-sizeof(StackElement)));
                    cc.mov(thread_fields[jit_field_id_thread_sp], delegate); // sp--

                    cc.mov(ctx, val);
                    cc.movsd(vec0, stack_element_fields[jit_field_id_stack_element_var]); // sp->val

                    cc.mov(ctx, threadReg);
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp

                    if(GET_Da(*bc) != 0) {
                        cc.add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(*bc))); // fp+GET_DA(*pc)
                    }

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_IPUSHL: { // (++sp)->var = (fp+GET_Da(*pc))->var;
                    cc.comment("; ipushl", 8);

                    cc.mov(ctx, threadReg);
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp

                    if(GET_Da(*bc) != 0) {
                        cc.add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(*bc))); // fp+GET_DA(*pc)
                    }

                    cc.movsd(vec0, stack_element_fields[jit_field_id_stack_element_var]);

                    cc.mov(ctx, threadReg); // ctx->current

                    cc.mov(val, thread_fields[jit_field_id_thread_sp]); // ctx->current->sp
                    cc.lea(val, ptr(val, (int32_t)sizeof(StackElement)));
                    cc.mov(thread_fields[jit_field_id_thread_sp], val); // sp++

                    cc.mov(ctx, val);
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0); // sp->val

                    break;
                }
                case op_MOVSL: { // o2 = &((sp+GET_Da(*pc))->object);
                    cc.comment("; movsl", 7);

                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_sp]); // ctx->current->sp

                    if(GET_Da(*bc) != 0) {
                        cc.add(ctx, (int64_t)(sizeof(StackElement) * GET_Da(*bc))); // sp+GET_DA(*pc)
                    }

                    cc.lea(ctx, stack_element_fields[jit_field_id_stack_element_object]);
                    cc.mov(o2Ptr, ctx);
                    break;
                }
                case op_MOVBI: { // registers[i64bmr]=GET_Da(*pc) + exponent(*(pc+1)); pc++;
                    cc.comment("; movbi", 7);
                    int64_t arg0 = GET_Da(*bc);
                    bc++;
                    i++; cc.bind(labels[i]); // we wont use it but we need to bind it anyway
                    passArg0(cc, *bc);
                    cc.call((int64_t)exponent);
                    SET_LCONST_DVAL2(vec1, arg0);

                    cc.addsd(vec0, vec1);
                    setRegister(setRegisterParams(vec0, i64bmr));
                    break;
                }
                case op_SIZEOF: {
                    cc.comment("; sizeof", 8);
                    /**
                     *
                        if(o2==NULL || o2->object == NULL)
                            registers[GET_Da(*pc)] = 0;
                        else
                            registers[GET_Da(*pc)]=o2->object->size;
                     */
                    cc.mov(ctx, o2Ptr);
                    cc.cmp(ctx, 0);
                    Label ifTrue = cc.newLabel(), end = cc.newLabel();
                    cc.jne(ifTrue);
                    SET_LCONST_DVAL(0);

                    setRegister(setRegisterParams(vec0, GET_Da(*bc)));
                    cc.jmp(end);
                    cc.bind(ifTrue);

                    cc.mov(ctx, qword_ptr(ctx));
                    cc.mov(ctx, sharp_object_fields[jit_field_id_shobj_size]);
                    int64ToDouble(cc, vec0, ctx);
                    setRegister(setRegisterParams(vec0, GET_Da(*bc)));

                    cc.bind(end);
                    break;
                }
                case op_PUT: {                            // cout << registers[GET_Da(*pc)];
                    cc.comment("; put", 5);
                    passArg0(cc, GET_Da(*bc));
                    cc.call((int64_t)global_jit_put);
                    break;
                }
                case op_PUTC: {                            // printf("%c", (char)registers[GET_Da(*pc)]);
                    cc.comment("; putc", 6);
                    passArg0(cc, GET_Da(*bc));
                    cc.call((int64_t)global_jit_putc);
                    break;
                }
                case op_GET: {
                    cc.comment("; get", 5);
                    passArg0(cc, GET_Da(*bc));
                    cc.call((int64_t)global_jit_get);
                    break;
                }
                case op_CHECKLEN: {
                    // unsupported for now
                    cc.comment("; checklen", 10);
                    safeGuard_CheckNull2(o2Ptr, JIT_TCSFIELDS, labels[i]);
                    getRegister(registerParams(vec0, GET_Da(*bc)));

                    // we control everything so we know ctx holds o2->object
                    cc.mov(val, sharp_object_fields[jit_field_id_shobj_size]);
                    int64ToDouble(cc, vec1, val);

                    cc.ucomisd(vec0, vec1);
                    Label fail = cc.newLabel(), success = cc.newLabel();
                    cc.jae(fail);

                    SET_LCONST_DVAL2(vec1, 0);
                    cc.ucomisd(vec0, vec1);
                    cc.jae(success);
                    cc.bind(fail);

                    passArg0(cc, GET_Da(*bc));
                    passArg1(cc, val);
                    cc.call((int64_t)__srt_cxxa_throw_indexOutOfBounds);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);

                    cc.bind(success);
                    break;
                }
                case op_GOTO: {// $                             // pc = cache+GET_Da(*pc);
                    cc.comment("; goto", 6);
                    cc.jmp(labels[GET_Da(x64)]);
                    break;
                }
                case op_LOADPC: {
                    cc.comment("; loadpc", 6);

                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(val, thread_fields[jit_field_id_thread_pc]); // ctx->current->pc
                    cc.mov(delegate, thread_fields[jit_field_id_thread_cache]); // ctx->current->pc

                    cc.sub(val, delegate);
                    int64ToDouble(cc, vec0, val);
                    setRegister(setRegisterParams(vec0, GET_Da(*bc)));
                    break;
                }
                case op_PUSHOBJ: {
                    cc.comment("; pushobj", 9);
                    cc.mov(ctx, threadReg);
                    cc.mov(val, thread_fields[jit_field_id_thread_sp]);
                    cc.lea(val, ptr(val, sizeof(StackElement)));
                    cc.mov(thread_fields[jit_field_id_thread_sp], val);

                    passArg0(cc, val);
                    passArg1(cc, o2Ptr);
                    cc.call((int64_t)global_jit_setObject1);
                    break;
                }
                case op_DEL: {
                    cc.comment("; del", 5);
                    passArg0(cc, o2Ptr);
                    cc.call((int64_t)global_jit_del);
                    break;
                }
                case op_NEWCLASS: {
                    cc.comment("; newclass", 10);
                    passArg0(cc, threadReg);
                    passArg1(cc, GET_Da(*bc));
                    cc.call((int64_t)global_jit_newClass0);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }
                case op_MOVN: {
                    cc.comment("; movn", 6);
                    safeGuard_CheckNullObj(o2Ptr, JIT_TCSFIELDS, labels[i]);

                    if(GET_Da(*bc) > 0)
                        cc.add(ctx, (int64_t)(GET_Da(*bc)*sizeof(Object)));
                    cc.mov(o2Ptr, ctx);
                    break;
                }

                case op_SLEEP: {
                    cc.comment("; sleep", 7);
                    getRegister(registerParams(vec0, GET_Da(*bc)));
                    doubleToInt64(cc, val, vec0);

                    passArg0(cc, val);
                    cc.call((int64_t)__os_sleep);
                    break;
                }
                case op_TEST: {
                    cc.comment("; test", 6);
                    cc.mov(ctx, registersReg);        // move the contex var into register
                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));

                    getRegister(registerParams(vec1, GET_Ca(x64)));
                    getRegister(registerParams(vec0, GET_Cb(x64)));

                    cc.ucomisd(vec0, vec1);

                    Label ifFalse = cc.newLabel();
                    Label ifEnd = cc.newLabel();
                    cc.jp(ifFalse);
                    cc.jne(ifFalse);
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
                case op_TNE: {
                    cc.comment("; tne", 5);
                    cc.mov(ctx, registersReg);        // move the contex var into register
                    cc.add(ctx, (int64_t )(sizeof(double) * i64cmt));

                    getRegister(registerParams(vec1, GET_Ca(x64)));
                    getRegister(registerParams(vec0, GET_Cb(x64)));

                    cc.ucomisd(vec0, vec1);

                    Label ifFalse = cc.newLabel();
                    Label ifEnd = cc.newLabel();
                    cc.jp(ifFalse);
                    cc.je(ifFalse);
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
                case op_LOCK: {
                    cc.comment("; tne", 5);
                    safeGuard_CheckNullObj(o2Ptr, JIT_TCSFIELDS, labels[i]);

                    passArg0(cc, o2Ptr);
                    cc.call((int64_t)global_jit_lock);
                    break;
                }
                case op_ULOCK: {
                    cc.comment("; tne", 5);
                    safeGuard_CheckNullObj(o2Ptr, JIT_TCSFIELDS, labels[i]);

                    passArg0(cc, o2Ptr);
                    cc.call((int64_t)global_jit_ulock);
                    break;
                }
                case op_EXP: {
                    cc.comment("; exp", 5);
                    getRegister(registerParams(vec0, GET_Da(x64)));
                    doubleToInt64(cc, val, vec0);

                    passArg0(cc, val);
                    cc.call((int64_t)exponent);

                    setRegister(setRegisterParams(vec0, i64bmr));
                    break;
                }

                case op_MOVG: {
                    cc.comment("; movg", 6);

                    Object* o = env->globalHeap+GET_Da(*bc);
                    cc.mov(ctxPtr, (int64_t)o);
                    break;
                }
                case op_MOVND: {
                    cc.comment("; movn", 6);
                    safeGuard_CheckNullObj(o2Ptr, JIT_TCSFIELDS, labels[i]);
                    cc.mov(val, ctx);

                    getRegister(registerParams(vec0, GET_Da(x64)));
                    doubleToInt64(cc, tmp, vec0);

                    cc.imul(tmp, (size_t)sizeof(Object));
                    cc.add(ctx, tmp);
                    cc.mov(o2Ptr, ctx);
                    break;
                }
                case op_NEWOBJARRAY: {
                    cc.comment("; newclass", 10);
                    passArg0(cc, threadReg);
                    passArg1(cc, GET_Da(*bc));
                    cc.call((int64_t) global_jit_newArray0);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }

                case op_NOT: {
                    cc.comment("; not", 5);
                    Label ifTrue = cc.newLabel(), end = cc.newLabel();
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, tmp, vec0);
                    cc.cmp(tmp, 0);
                    cc.ja(ifTrue);
                    cc.mov(tmp, 1);
                    cc.jmp(end);
                    cc.bind(ifTrue);
                    cc.mov(tmp, 0);
                    cc.bind(end);

                    int64ToDouble(cc, vec0, tmp);
                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }

                case op_SKIP: {
                    cc.comment("; skip", 6);

                    if(GET_Da(*bc) > 0) {
                        cc.mov(val, (int64_t)(i+GET_Da(*bc)+1));
                        jmpToLabel(cc, val, tmp, labelsPtr);
                    } else
                        cc.nop();
                    break;
                }
                case op_SKPE: {
                    cc.comment("; skpe", 6);
                    tmpMem = qword_ptr(registersReg, (int64_t )(sizeof(double) * i64cmt));
                    cc.pxor(vec0, vec0);
                    cc.ucomisd(vec0, tmpMem);
                    Label ifEnd = cc.newLabel();
                    cc.jp(ifEnd);
                    cc.je(ifEnd);
                    cc.jmp(labels[i+GET_Da(x64)]);

                    cc.bind(ifEnd);

                    break;
                }
                case op_SKNE: {
                    cc.comment("; skne", 6);
                    tmpMem = qword_ptr(registersReg, (int64_t )(sizeof(double) * i64cmt));
                    cc.pxor(vec0, vec0);
                    cc.ucomisd(vec0, tmpMem);
                    Label ifEnd = cc.newLabel();
                    cc.jp(ifEnd);
                    cc.jne(ifEnd);
                    cc.jmp(labels[i+GET_Da(x64)]);

                    cc.bind(ifEnd);

                    break;
                }
                case op_CMP: {
                    cc.comment("; cmp", 5);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));

                    SET_LCONST_DVAL2(vec1, GET_Cb(*bc));
                    cc.ucomisd(vec0, vec1);
                    Label ifEnd = cc.newLabel();
                    cc.jp(ifEnd);
                    cc.jne(ifEnd);
                    cc.pxor(vec0, vec0);
                    cc.jmp(ifEnd);
                    SET_LCONST_DVAL2(vec0, 1);
                    cc.bind(ifEnd);
                    setRegister(setRegisterParams(vec0, i64cmt));
                    break;
                }
                case op_AND: {
                    cc.comment("; and", 5);
                    Label isFalse = cc.newLabel(), isTrue = cc.newLabel(),
                            end = cc.newLabel();
                    getRegister(registerParams(vec0, GET_Ca(*bc)));

                    SET_LCONST_DVAL2(vec1, 0);
                    cc.ucomisd(vec0, vec1);
                    cc.jp(isFalse);
                    cc.je(isFalse);

                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    cc.ucomisd(vec0, vec1);
                    cc.jp(isTrue);
                    cc.jne(isTrue);

                    cc.bind(isFalse);
                    SET_LCONST_DVAL2(vec0, 0);
                    cc.jmp(end);
                    cc.bind(isTrue);
                    SET_LCONST_DVAL2(vec0, 1);
                    cc.bind(end);

                    setRegister(setRegisterParams(vec0, i64cmt));
                    break;
                }
                case op_UAND: {
                    cc.comment("; uand", 6);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rbx, vec0);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, rcx, vec0);
                    cc.and_(rbx, rcx);

                    int64ToDouble(cc, vec0, rbx);
                    setRegister(setRegisterParams(vec0, i64cmt));
                    break;
                }
                case op_OR: {
                    cc.comment("; or", 4);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rbx, vec0);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, rcx, vec0);
                    cc.or_(rbx, rcx);

                    int64ToDouble(cc, vec0, rbx);
                    setRegister(setRegisterParams(vec0, i64cmt));
                    break;
                }
                case op_XOR: {
                    cc.comment("; xor", 5);
                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rbx, vec0);
                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, rcx, vec0);
                    cc.xor_(rbx, rcx);

                    int64ToDouble(cc, vec0, rbx);
                    setRegister(setRegisterParams(vec0, i64cmt));
                    break;
                }
                case op_THROW: {
                    cc.comment("; throw", 7);
                    passArg0(cc, threadReg);
                    cc.call((int64_t)__srt_cxxa_throw_exception);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }
                case op_CHECKNULL: { // by far one of the dumbest opcodes......
                    cc.comment("; checknull", 11);
                    safeGuard_CheckNull2(o2Ptr, JIT_TCSFIELDS, labels[i]);
                    SET_LCONST_DVAL2(vec0, 1);
                    setRegister(setRegisterParams(vec0, i64cmt));
                    break;
                }
                case op_RETURNOBJ: {
                    cc.comment("; retobj", 8);
                    cc.mov(ctx, threadReg);
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]);

                    passArg0(cc, ctx);
                    passArg1(cc, o2Ptr);
                    cc.call((int64_t)global_jit_setObject1);
                    break;
                }
                case op_NEWCLASSARRAY: {
                    cc.comment("; newclassarry", 15);
                    passArg0(cc, threadReg);
                    passArg1(cc, GET_Ca(*bc));
                    passArg2(cc, GET_Cb(*bc));
                    cc.call((int64_t)global_jit_newClass1);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }
                case op_NEWSTRING: {
                    cc.comment("; newstr", 8);
                    passArg0(cc, threadReg);
                    passArg1(cc, GET_Da(*bc));
                    cc.call((int64_t)global_jit_newStr);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }
                case op_SUBL: {                        // (fp+GET_Cb(*pc))->var-=registers[GET_Ca(*pc)];
                    cc.comment("; subl", 6);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    cc.subsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_MULL: {                        // (fp+GET_Cb(*pc))->var*=registers[GET_Ca(*pc)];
                    cc.comment("; iaddl", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    cc.mulsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_DIVL: {                        // (fp+GET_Cb(*pc))->var/=registers[GET_Ca(*pc)];
                    cc.comment("; divl", 6);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    cc.divsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_MODL: {                        // (fp+GET_Cb(*pc))->modul(registers[GET_Ca(*pc)]);
                    cc.comment("; mod", 6);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);
                    doubleToInt64(cc, rax, vec1);

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    doubleToInt64(cc, rcx, vec0);

                    cc.cqo();
                    cc.idiv(rcx);
                    cc.mov(rax, rdx);
                    int64ToDouble(cc, vec0, rdx);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_ISUBL: {                        // (fp+GET_Cb(*pc))->var-=GET_Ca(*pc);
                    cc.comment("; isubl", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    SET_LCONST_DVAL(GET_Ca(x64));
                    cc.subsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_IMULL: {                        // (fp+GET_Cb(*pc))->var*=GET_Ca(*pc);
                    cc.comment("; imull", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    SET_LCONST_DVAL(GET_Ca(x64));
                    cc.mulsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_IDIVL: {                        // (fp+GET_Cb(*pc))->var*=GET_Ca(*pc);
                    cc.comment("; idivl", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    SET_LCONST_DVAL(GET_Ca(x64));
                    cc.divsd(vec0, vec1);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_IMODL: {                        // (fp+GET_Cb(*pc))->var*=GET_Ca(*pc);
                    cc.comment("; imodl", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);
                    doubleToInt64(cc, rax, vec1);

                    SET_LCONST_DVAL2(vec0, GET_Ca(x64));
                    doubleToInt64(cc, rcx, vec0);

                    cc.cqo();
                    cc.idiv(rcx);
                    cc.mov(rax, rdx);
                    int64ToDouble(cc, vec0, rdx);

                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_IALOAD_2: {
                    cc.comment("; iaload_2", 10);
                    safeGuard_CheckINull(o2Ptr, JIT_TCSFIELDS, labels[i]);

                    getRegister(registerParams(vec0, GET_Cb(*bc)));
                    doubleToInt64(cc, val, vec0);

                    cc.mul(val, sizeof(double));
                    cc.movsd(vec0, qword_ptr(ctx, val));
                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_POPOBJ: {
                    cc.comment("; popobj", 8);
                    safeGuard_CheckNull(o2Ptr, JIT_TCSFIELDS, labels[i]);
                    cc.mov(ctx, threadReg);
                    cc.mov(tmp, thread_fields[jit_field_id_thread_sp]);
                    cc.lea(val, ptr(tmp, ((int32_t)-sizeof(StackElement))));
                    cc.mov(thread_fields[jit_field_id_thread_sp], val);

                    cc.add(tmp, (size_t)sizeof(double));

                    passArg0(cc, tmp);
                    passArg1(cc, o2Ptr);
                    cc.call((int64_t)global_jit_setObject2);
                    break;
                }
                case op_SMOVR: {
                    cc.comment("; smovr", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_sp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_SMOVR_2: {
                    cc.comment("; smovr", 7);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    getRegister(registerParams(vec0, GET_Ca(x64)));
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_ANDL: {
                    cc.comment("; andL", 6);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);
                    doubleToInt64(cc, rax, vec1);

                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rcx, vec0);
                    cc.and_(rbx, rcx);

                    int64ToDouble(cc, vec0, rbx);
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_ORL: {
                    cc.comment("; orl", 5);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);
                    doubleToInt64(cc, rax, vec1);

                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rcx, vec0);
                    cc.or_(rbx, rcx);

                    int64ToDouble(cc, vec0, rbx);
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_XORL: {
                    cc.comment("; xorl", 6);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);
                    doubleToInt64(cc, rax, vec1);

                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rcx, vec0);
                    cc.xor_(rbx, rcx);

                    int64ToDouble(cc, vec0, rbx);
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_RMOV: {
                    cc.comment("; rmov", 6);
                    safeGuard_CheckINull(o2Ptr, JIT_TCSFIELDS, labels[i]);

                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    doubleToInt64(cc, rcx, vec0);
                    cc.mul(rcx, (int64_t)sizeof(double));
                    cc.add(ctx, rcx);

                    getRegister(registerParams(vec0, GET_Ca(*bc)));
                    cc.movsd(qword_ptr(ctx), vec0);
                    break;
                }
                case op_SMOV: {
                    cc.comment("; smov", 6);
                    cc.mov(ctx, threadReg); // ctx->current
                    cc.mov(ctx, thread_fields[jit_field_id_thread_sp]); // ctx->current->fp
                    if(GET_Cb(x64) != 0) {
                        cc.add(ctx, (int64_t )(sizeof(StackElement) * GET_Cb(x64)));
                    }

                    cc.mov(tmp, ctx);
                    tmpMem = qword_ptr(tmp);
                    cc.movsd(vec1, tmpMem);

                    setRegister(setRegisterParams(vec1, GET_Ca(*bc)));
                    break;
                }
                case op_LOADPC_2: {
                    cc.comment("; loadpc_2", 10);
                    cc.call((int64_t)global_jit_getpc);
                    passReturnArg(cc, val);

                    cc.add(val, GET_Cb(*bc));
                    int64ToDouble(cc, vec0, val);

                    setRegister(setRegisterParams(vec0, GET_Ca(*bc)));
                    break;
                }
                case op_PUSHNIL: {
                    cc.comment("; pushnil", 9);

                    passArg0(cc, threadReg);
                    cc.call((int64_t)global_jit_pushNil);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }
                case op_PUSHL: {
                    cc.comment("; pushl", 7);

                    passArg0(cc, threadReg);
                    passArg1(cc, GET_Da(*bc));
                    cc.call((int64_t)global_jit_pushl);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);
                    break;
                }
                case op_CALL: {
                    cc.comment("; call", 6);
                    passArg0(cc, threadReg);
                    passArg1(cc, GET_Da(*bc));
                    cc.call((int64_t) global_jit_call0);

                    // lets just make sure we are good
                    checkMasterShutdown(cc, argsReg, i, lbl_funcend);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);

                    break;
                }
                case op_CALLD: {
                    cc.comment("; calld", 7);
                    passArg0(cc, threadReg);
                    passArg1(cc, GET_Da(*bc));
                    cc.call((int64_t) global_jit_call1);

                    // lets just make sure we are good
                    checkMasterShutdown(cc, argsReg, i, lbl_funcend);
                    goto_threadSafetyCheck(JIT_TCSFIELDS, labels[i]);

                    break;
                }
                case op_RETURNVAL: { // (fp)->var=registers[GET_Da(*pc)];
                    cc.comment("; retval", 8);
                    getRegister(registerParams(vec0, GET_Da(*bc)));

                    cc.mov(ctx, threadReg);
                    cc.mov(ctx, thread_fields[jit_field_id_thread_fp]);
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_ISTORE: { // (++sp)->var = GET_Da(*pc);
                    cc.comment("; istore", 8);

                    SET_LCONST_DVAL(GET_Da(*bc));

                    cc.mov(ctx, threadReg);
                    cc.mov(val, thread_fields[jit_field_id_thread_sp]);
                    cc.lea(val, ptr(val, sizeof(StackElement)));
                    cc.mov(thread_fields[jit_field_id_thread_sp], val);

                    cc.mov(ctx, val);
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
                    break;
                }
                case op_LOADVAL: { // registers[GET_Da(*pc)]=(sp--)->var;
                    cc.mov(ctx, threadReg);
                    cc.mov(tmp, thread_fields[jit_field_id_thread_sp]);
                    cc.lea(val, ptr(tmp, ((int32_t)-sizeof(StackElement))));
                    cc.mov(thread_fields[jit_field_id_thread_sp], val);

                    cc.mov(ctx, tmp);
                    cc.movsd(vec0, stack_element_fields[jit_field_id_stack_element_var]);

                    setRegister(setRegisterParams(vec0, GET_Da(*bc)));
                    break;
                }
                default: {
                    error = jit_error_compile;
                    cout << "jit assembler:  error: unidentified opcode (" <<  GET_OP(x64) << ") in function {" << method->fullName.str() << "} @[" << i << "]." << endl;
                    cout << "\tPlease contact the developer with this information immediately." << endl;
                    return error;
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

        emitReturnOp(cc, tmp, val, threadReg, ctx, delegate, argsReg,
                     vec0, vec1, lbl_funcend);

        restoreCalleeRegisters(cc, stackSize);
        cc.pop(zbp);
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
        jit_safetycheck(cc, ctxPtr, ctx, val, tmp, delegate, vec0, vec1,
                        threadReg, argsReg, lbl_funcend, labelsPtr);

        cc.nop();
        cc.nop();
        cc.nop();
        cc.align(kAlignData, 64);              // Align 64
        cc.bind(data_section);                 // emit constants to be used
        cc.comment("; data section start", 21);
        lconsts.emitConstants(cc);

        if(!error) {

            jit_mut.lock();
            if(method->isjit) {
                // sanity check
                jit_mut.unlock();
                return jit_error_ok;
            }

            sjit_function jitfn;
            Error err = rt.add(&jitfn, &code);   // Add the generated code to the runtime.
            if (err) {
                cout << "code error " << err << endl;
                jit_mut.unlock();
                return jit_error_compile;
            }
            else {
                jit_func jfunc;
                jfunc.func = jitfn;
                jfunc.serial = method->address;
                method->jit_addr=functions.size();
                method->isjit = 1;
                functions.push_back(jfunc);
            }

            jit_mut.unlock();
            return jit_error_ok;
        }

        return error;
    }

    return jit_error_compile;
}

void safeGuard_CheckINull(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack) {
    cc.mov(ctx, o2Ptr);
    cc.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = cc.newLabel();
    Label nullCheckFailed = cc.newLabel();
    cc.je(nullCheckFailed);

    cc.mov(ctx, x86::qword_ptr(ctx));
    cc.cmp(ctx, 0);
    cc.je(nullCheckFailed);

    cc.mov(ctx, sharp_object_fields[jit_field_id_shobj_HEAD]);
    cc.cmp(ctx, 0);
    cc.jne(nullCheckPassed);

    cc.bind(nullCheckFailed);
    cc.call((int64_t)__srt_cxxa_throw_nullptr);
    goto_threadSafetyCheck(JIT_TCSFIELDS, traceBack);
    cc.bind(nullCheckPassed);
}

void safeGuard_CheckNullObj(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack) {
    cc.mov(ctx, o2Ptr);
    cc.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = cc.newLabel();
    Label nullCheckFailed = cc.newLabel();
    cc.je(nullCheckFailed);

    cc.mov(ctx, x86::qword_ptr(ctx));
    cc.cmp(ctx, 0);
    cc.je(nullCheckFailed);

    cc.mov(ctx, sharp_object_fields[jit_field_id_shobj_node]);
    cc.cmp(ctx, 0);
    cc.jne(nullCheckPassed);

    cc.bind(nullCheckFailed);
    cc.call((int64_t)__srt_cxxa_throw_nullptr);
    goto_threadSafetyCheck(JIT_TCSFIELDS, traceBack);
    cc.bind(nullCheckPassed);
}

void safeGuard_CheckNull2(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack) {
    cc.mov(ctx, o2Ptr);
    cc.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = cc.newLabel();
    Label nullCheckFailed = cc.newLabel();
    cc.je(nullCheckFailed);
    cc.mov(ctx, x86::qword_ptr(ctx));
    cc.cmp(ctx, 0);
    cc.jne(nullCheckPassed);

    cc.bind(nullCheckFailed);
    cc.call((int64_t)__srt_cxxa_throw_nullptr);
    goto_threadSafetyCheck(JIT_TCSFIELDS, traceBack);
    cc.bind(nullCheckPassed);
}

void safeGuard_CheckNull(const X86Mem &o2Ptr, X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t i, Label &thread_check_section, Label &traceBack) {
    cc.mov(ctx, o2Ptr);
    cc.cmp(ctx, 0); // 02==NULL
    Label nullCheckPassed = cc.newLabel();
    cc.jne(nullCheckPassed);
    cc.call((int64_t)__srt_cxxa_throw_nullptr);
    goto_threadSafetyCheck(JIT_TCSFIELDS, traceBack);
    cc.bind(nullCheckPassed);
}

void checkMasterShutdown(X86Assembler &cc, X86Gp &argsReg, int64_t pc, const Label &lbl_funcend) {
    using namespace asmjit::x86;

    cc.mov(x86::rbx, (int64_t)&masterShutdown);
    cc.movzx(x86::ebx, byte_ptr(x86::rbx));
    cc.test(x86::bl, x86::bl);
    Label ifFalse = cc.newLabel();
    cc.je(ifFalse);
    cc.mov(argsReg, pc);
    returnFunction();

    cc.bind(ifFalse);
}

void
emitReturnOp(X86Assembler &cc, X86Gp &tmp, X86Gp &val, X86Gp &threadReg, X86Gp &ctx, X86Gp &delegate, X86Gp &argsReg,
             X86Xmm &vec0, X86Xmm &vec1, const Label &lbl_funcend) {

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
    Label end = cc.newLabel();
    cc.mov(ctx, threadReg); // thread*
    cc.mov(val, thread_fields[jit_field_id_thread_calls]);
    cc.cmp(val, 1); // if(calls <= 1) {
    Label ifFalse = cc.newLabel();
    cc.ja(ifFalse);

#ifdef SHARP_PROF_
    cc.mov(ctx, tmp);        // move thread* var into register
    cc.mov(ctx, thread_fields[jit_field_id_thread_tprof]); // ctx->current->tprof
    cc.mov(argsReg, ctx);       // this register will be saved by the called function

    cc.call((int64_t)Clock::realTimeInNSecs);
    cc.mov(tmp, ctx); // passRetArg()
    cc.mov(ctx, argsReg);
    cc.mov(profiler_fields[jit_field_id_profiler_endtm], tmp); // ctx->current->tprof->endtm = Clock::realTimeInNSecs();

    passArg0(cc, argsReg);
    cc.call((int64_t)global_jit_profile);
#endif

    incrementPc(cc, ctx, val, threadReg);
    cc.jmp(end);
    cc.bind(ifFalse);

    cc.sub(val, 1);
    cc.mov(thread_fields[jit_field_id_thread_calls], val);

    cc.add(val, 1);

    cc.mov(tmp, (size_t)sizeof(Frame));
    cc.mov(ctx, thread_fields[jit_field_id_thread_callStack]);
    cc.imul(val, tmp);      // int64 offset = calls*sizeof(Frame)
    cc.add(ctx, val);      // shift pointer in ctx to address...i think??..
    cc.mov(val, ctx);      // swap values

//                    if(current->finallyBlocks.len > 0)
//                        vm->executeFinally(thread_self->current);

    cc.mov(ctx, threadReg); // ctx->current
    cc.mov(ctx, thread_fields[jit_field_id_thread_current]); // ctx->current->current

    passArg0(cc, ctx);
    cc.call((int64_t)global_jit_finallyBlocks);
    passReturnArg(cc, delegate);
    cc.cmp(delegate, 0);
    Label finallyBlockEnd = cc.newLabel();
    cc.jna(finallyBlockEnd);
    // we need to update the PC
    savePrivateRegisters(cc, vec0, vec1);
    updateThreadPc(cc, ctx, val, threadReg, argsReg);

    cc.call((int64_t) global_jit_executeFinally);
    passReturnArg(cc, tmp);
    restorePrivateRegisters(cc, vec0, vec1);
    cc.cmp(tmp, 1);
    cc.je(end);
    cc.bind(finallyBlockEnd);

    cc.mov(ctx, val); // fame->last
    cc.mov(delegate, frame_fields[jit_field_id_frame_last]);
    cc.mov(ctx, threadReg);

    cc.mov(thread_fields[jit_field_id_thread_current], delegate); // current = fame->last
    cc.mov(ctx, delegate);
    cc.mov(delegate, method_fields[jit_field_id_method_bytecode]);

    cc.mov(ctx, threadReg);
    cc.mov(thread_fields[jit_field_id_thread_cache], delegate); // cache = current->bytecode

    cc.mov(ctx, val);
    cc.mov(delegate, frame_fields[jit_field_id_frame_pc]); // pc = frame->pc;

    cc.mov(ctx, threadReg); // move thread*
    cc.mov(thread_fields[jit_field_id_thread_pc], delegate);

    cc.mov(ctx, val);
    cc.mov(delegate, frame_fields[jit_field_id_frame_sp]); // sp = frame->sp;

    cc.mov(ctx, threadReg); // move thread*
    cc.mov(thread_fields[jit_field_id_thread_sp], delegate);

    cc.mov(ctx, val);
    cc.mov(delegate, frame_fields[jit_field_id_frame_fp]); // fp = frame->fp;

    cc.mov(ctx, threadReg); // move thread*
    cc.mov(thread_fields[jit_field_id_thread_fp], delegate);

#ifdef SHARP_PROF_


    // in this case we dont give a crap because were returning anyway no no need to save regs :)
    cc.mov(ctx, tmp);        // move thread* var into register
    cc.mov(ctx, thread_fields[jit_field_id_thread_tprof]); // ctx->current->tprof

    passArg0(cc, ctx);
    cc.call((int64_t)global_jit_profile);
#endif
    incrementPc(cc, ctx, val, threadReg);
    cc.bind(end);
}

void incrementPc(X86Assembler &cc, const X86Gp &ctx, const X86Gp &val, const X86Gp &threadReg) {
    cc.mov(ctx, threadReg);
    cc.mov(val, thread_fields[jit_field_id_thread_pc]);
    cc.lea(val, x86::ptr(val, sizeof(int64_t)));
    cc.mov(thread_fields[jit_field_id_thread_pc], val);
}

void setRegister(X86Assembler &cc, const X86Gp &ctx, const X86Gp &registersReg, const X86Xmm &vec0, const int64_t reg) {
    cc.mov(ctx, registersReg);        // move the contex var into register
    if(reg != 0) {
        cc.add(ctx, (int64_t )(sizeof(double) * reg));
    }
    cc.movsd(x86::qword_ptr(ctx), vec0);
}

FILE *getLogFile() {
    ofstream outfile ("JIT.s");        // Quickly create file
    outfile.close();

    FILE * pFile;                           // out logging file
    pFile = fopen ("JIT.s" , "rw+");
    return pFile;
}

/**
 * high level equvilent of registers[i64ebx];
 *
 * This function abstracts retrieving register values to make core much easier to read
 *
 * @param cc
 * @param tmp
 * @param registersReg
 * @param idx
 * @param tmpMem
 * @param vec
 * @return
 */
void
getRegister(X86Assembler &cc, const X86Gp &tmp, const X86Gp &registersReg, int idx, X86Mem &tmpMem, const X86Xmm &vec) {
    cc.mov(tmp, registersReg);      // were just using these registers because we can, makes life so much easier
    if(idx != 0) {
        cc.add(tmp, (int64_t )(sizeof(double) * idx));
    }

    tmpMem = x86::qword_ptr(tmp);
    cc.movsd(vec, tmpMem);
}

void
doubleToInt64(X86Assembler &cc, const X86Gp &dest, const X86Xmm &src) {
    cc.cvttsd2si(dest, src);
}

void
int64ToDouble(X86Assembler &cc, const X86Xmm &dest, const X86Gp &src) {
    cc.cvtsi2sd(dest, src);
}

/**
 * Destination register must be different thansource register(idx)
 * otherwize you could jump to an invalid code base.
 * @param cc
 * @param idx
 * @param dest
 * @param labelsPtr
 */
void
jmpToLabel(X86Assembler &cc, const X86Gp &idx, const X86Gp &dest, X86Mem &labelsPtr) {
    using namespace asmjit::x86;

    cc.mov(dest, labelsPtr);      // were just using these registers because we can, makes life so much easier
    cc.cmp(idx, 0);
    Label ifTrue = cc.newLabel();
    cc.je(ifTrue);
    cc.imul(idx, (size_t)sizeof(int64_t));      // offset = labelAddr*sizeof(int64_t)
    cc.add(dest, idx);
    cc.bind(ifTrue);

    cc.mov(dest, x86::ptr(dest));
    cc.jmp(dest);
}

/**
 * Argument passing util functions
 * These functions allow you to pass up to 3 arguments to a function
 * through a register or numeric value
 * @param cc
 * @param arg
 */
void passArg0(X86Assembler &cc, int64_t arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rcx : x86::rdi;  // First argument (array ptr).
    }
    else {
        arg0 = cc.zax();                       // Use EDX to hold the array pointer.
    }
    cc.mov(arg0, arg); // set signal to send

}

void passArg0(X86Assembler &cc, X86Gp& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rcx : x86::rdi;  // First argument.
    }
    else {
        arg0 = cc.zax();                       // Use EaX to hold the second arg.
    }
    cc.mov(arg0, arg);

}

void passArg0(X86Assembler &cc, X86Mem& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rcx : x86::rdi;  // First argument.
    }
    else {
        arg0 = cc.zax();                       // Use EaX to hold the second arg.
    }
    cc.mov(arg0, arg);

}

void passArg1(X86Assembler &cc, X86Gp& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rdx : x86::rsi;  // Second argument.
    }
    else {
        arg0 = cc.zcx();                       // Use ECX to hold the second arg.
    }
    cc.mov(arg0, arg);

}

void passArg1(X86Assembler &cc, X86Mem& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rdx : x86::rsi;  // Second argument.
    }
    else {
        arg0 = cc.zcx();                       // Use ECX to hold the second arg.
    }
    cc.mov(arg0, arg);

}

void passArg1(X86Assembler &cc, int64_t arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::rdx : x86::rsi;  // Second argument.
    }
    else {
        arg0 = cc.zcx();                       // Use ECX to hold the second arg.
    }
    cc.mov(arg0, arg);

}

void passArg2(X86Assembler &cc, X86Gp& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::r8 : x86::rdx;  // Third argument.
    }
    else {
        arg0 = cc.zdx();                       // Use EdX to hold the third arg.
    }
    cc.mov(arg0, arg);

}

void passArg2(X86Assembler &cc, X86Mem& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::r8 : x86::rdx;  // Third argument.
    }
    else {
        arg0 = cc.zdx();                       // Use EdX to hold the third arg.
    }
    cc.mov(arg0, arg);

}

void passArg2(X86Assembler &cc, int64_t arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::r8 : x86::rdx;  // Third argument.
    }
    else {
        arg0 = cc.zdx();                       // Use EdX to hold the third arg.
    }
    cc.mov(arg0, arg);

}

/**
 * Get return argument from a function call
 * @param cc
 * @param arg
 */
void passReturnArg(X86Assembler &cc, X86Gp& arg) {
    X86Gp arg0;
    if (ASMJIT_ARCH_64BIT) {
        bool isWinOS = static_cast<bool>(ASMJIT_OS_WINDOWS);
        arg0 = isWinOS ? x86::eax : x86::rdi;  // First argument (array ptr).
    }
    else {
        arg0 = cc.zbx();                       // Use EDX to hold the array pointer.
    }
    cc.mov(arg, arg0); // set signal to send

}

void goto_threadSafetyCheck(X86Assembler &cc, X86Gp & tmp, X86Gp & delegate, X86Gp & threadReg, X86Gp & ctx, X86Gp & argsReg, int64_t pc, Label &tsc, Label &retlbl) {
    using namespace asmjit::x86;
    cc.lea(delegate, ptr(retlbl)); // set return addr

    cc.mov(argsReg, pc);
    cc.mov(ctx, threadReg);
    cc.cmp(thread_fields[jit_field_id_thread_signal], 0);
    cc.jne(tsc); // call thread_sec proc
}

void jit_safetycheck(X86Assembler &cc, X86Mem &ctxPtr, X86Gp &ctx, X86Gp &val, X86Gp & tmp, X86Gp & delegate, X86Xmm &vec0, X86Xmm &vec1,
                     X86Gp &threadPtr, X86Gp &argsReg, Label &lbl_funcend, X86Mem &labelsPtr) {
    using namespace asmjit::x86;
//#define SAFTEY_CHECK
//    if (suspendPending)
//        suspendSelf();
//    if (state == THREAD_KILLED)
//        return;

    cc.mov(ctx, threadPtr); // jctx->current
    cc.mov(eax, thread_fields[jit_field_id_thread_signal]);
    cc.sar(eax, 2); // tsig_suspend
    cc.and_(eax, 1);
    cc.test(eax, eax);
    Label ifFalse1 = cc.newLabel();
    cc.je(ifFalse1);

    savePrivateRegisters(cc, vec0, vec1);
    passArg0(cc, val);

    cc.call((int64_t)global_jit_suspendSelf);
    restorePrivateRegisters(cc, vec0, vec1);
    cc.bind(ifFalse1);

    cc.mov(ctx, threadPtr);
    cc.mov(val, thread_fields[jit_field_id_thread_state]);
    cc.cmp(val, THREAD_KILLED);
    Label ifFalse2 = cc.newLabel();
    cc.jne(ifFalse2);
    returnFunction();

    cc.bind(ifFalse2);
    cc.mov(ctx, threadPtr); // jctx->current
    cc.mov(eax, thread_fields[jit_field_id_thread_signal]);
    cc.sar(eax, 1); // tsig_except
    cc.and_(eax, 1);
    cc.test(eax, eax);
    Label ifFalse3 = cc.newLabel();
    cc.je(ifFalse3);

    // we need to update the PC
    cc.mov(rcx, argsReg);
    updateThreadPc(cc, ctx, val, threadPtr, argsReg);
    cc.mov(argsReg, rcx);

    cc.mov(ctx, threadPtr);
    cc.mov(val, thread_fields[jit_field_id_thread_current]);

    passArg0(cc, val);
    cc.call((int64_t) __srt_cxxa_try_catch);
    passReturnArg(cc, tmp);

    cc.cmp(tmp, 1);
    Label ifFalse4 = cc.newLabel();
    cc.je(ifFalse4);
    returnFunction();
    cc.bind(ifFalse4);

    cc.call((int64_t)global_jit_getpc);
    passReturnArg(cc, val);

    jmpToLabel(cc, val, tmp, labelsPtr);
    cc.bind(ifFalse3);
    cc.jmp(delegate); // dynamically jump to last called position
}

void updateThreadPc(X86Assembler &cc, const X86Gp &ctx, const X86Gp &val, const X86Gp &threadPtr, const X86Gp &argsReg) {
    cc.mov(ctx, threadPtr);
    cc.mov(val, thread_fields[jit_field_id_thread_cache]);
    cc.imul(argsReg, (size_t)sizeof(int64_t));
    cc.add(val, argsReg);
    cc.mov(thread_fields[jit_field_id_thread_pc], val);
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
    cc.pop(cc.zbx());                // restore stack back to its original state
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
    cc.push(r10);
    cc.push(r11);
    if(ASMJIT_ARCH_64BIT)
        cc.push(rax);
    else
        cc.push(rcx);
    cc.sub(cc.zsp(), STACK_ALIGN_OFFSET);
}

void restorePrivateRegisters(X86Assembler &cc, X86Xmm &vec0, X86Xmm &vec1) {
    using namespace asmjit::x86;

    cc.add(cc.zsp(), STACK_ALIGN_OFFSET);
    if(ASMJIT_ARCH_64BIT)
        cc.pop(rax);
    else
        cc.pop(rcx);
    cc.pop(r11);
    cc.pop(r10);
}

/**
 * Global helper functions for easier interfacing
 * because im lazy :)
 *
 * @param signal signal to Virtual Machine
 */
void global_jit_sysInterrupt(int32_t signal) {
    try {
        vm->sysInterrupt(signal);
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

#ifdef SHAR_PROF_
void global_jit_profile(Profiler* prof) {
    prof->profile();
}

void global_jit_dump(Profiler* prof) {
    prof->dump();
}
#endif

/**
 * C++ exception handeling binded with Sharp
 * @param thread
 *
 * Prepare an exception to be thrown at a low level
 */
 void __srt_cxxa_prepare_throw(Exception &e) {
    thread_self->throwable = e.getThrowable();
    Object *eobj = &thread_self->exceptionObject;

    VirtualMachine::fillStackTrace(eobj);
    thread_self->throwable.throwable = eobj->object->k;
    sendSignal(thread_self->jctx->current->signal, tsig_except, 1);
 }

 void __srt_cxxa_throw_nullptr() {
     Exception e(Environment::NullptrException, "");
     __srt_cxxa_prepare_throw(e);
 }

 void __srt_cxxa_throw_exception(Thread *thread) {
     thread->exceptionObject = thread->sp->object;
     Exception e("", false);
     __srt_cxxa_prepare_throw(e);
 }

 void __srt_cxxa_throw_indexOutOfBounds(int reg, int64_t size) {
     stringstream ss;
     ss << "Access to Object at: " << registers[reg] << " size is " << size;
     Exception e(Environment::IndexOutOfBoundsException, ss.str());
     __srt_cxxa_prepare_throw(e);
 }

 int __srt_cxxa_try_catch(Method *method) {
     return vm->TryCatch(method, &thread_self->exceptionObject) ? 1 : 0;
 }

void global_jit_suspendSelf(Thread* thread) {
    thread->suspendSelf();
}

unsigned long global_jit_finallyBlocks(Method* fn) {
    return fn->finallyBlocks.size();
}

int global_jit_executeFinally() {
    try {
        return vm->executeFinally(thread_self->current);
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
        return 0;
    }
}

SharpObject* global_jit_newObject0(int64_t size) {
    try {
        return GarbageCollector::self->newObject(size);
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
        return NULL;
    }
}

void global_jit_newClass0(Thread* thread, int64_t classid) {
    try {
        (++thread->sp)->object =
                GarbageCollector::self->newObject(&env->classes[classid]);
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_newClass1(Thread* thread, int64_t reg, int64_t classid) {
    try {
        (++thread->sp)->object = GarbageCollector::self->newObjectArray(registers[reg],
                env->findClassBySerial(classid));
        if(((thread->sp-thread->dataStack)+1) >= thread->stack_lmt) {
            Exception e(Environment::StackOverflowErr, "");
            __srt_cxxa_prepare_throw(e);
        }
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_newStr(Thread* thread, int64_t strid) {
    try {
        GarbageCollector::self->createStringArray(&(++thread->sp)->object,
                                                  env->getStringById(strid));
        if(((thread->sp-thread->dataStack)+1) >= thread->stack_lmt) {
            Exception e(Environment::StackOverflowErr, "");
            __srt_cxxa_prepare_throw(e);
        }
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_newArray0(Thread* thread, int64_t reg) {
    try {
        (++thread->sp)->object = GarbageCollector::self->newObjectArray(registers[reg]);
        if(((thread->sp-thread->dataStack)+1) >= thread->stack_lmt) {
            Exception e(Environment::StackOverflowErr, "");
            __srt_cxxa_prepare_throw(e);
        }
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_pushNil(Thread* thread) {
    try {
        GarbageCollector::self->releaseObject(&(++thread->sp)->object);
        if(((thread->sp-thread->dataStack)+1) >= thread->stack_lmt) {
            Exception e(Environment::StackOverflowErr, "");
            __srt_cxxa_prepare_throw(e);
        }
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_pushl(Thread* thread, int64_t offset) {
    try {
        (++thread->sp)->object = (thread->fp+offset)->object;
        if(((thread->sp-thread->dataStack)+1) >= thread->stack_lmt) {
            Exception e(Environment::StackOverflowErr, "");
            __srt_cxxa_prepare_throw(e);
        }
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_setObject0(StackElement *sp, SharpObject* o) {
    sp->object = o;
}

void global_jit_setObject1(StackElement *sp, Object* o) {
    sp->object = o;
}

void global_jit_setObject2(StackElement *sp, Object* o) {
    *o = sp->object;
}

void global_jit_castObject(Object *o2, int64_t klass) {
    try {
        o2->castObject(klass);
    }catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_imod(int64_t op0, int64_t op1) {
    registers[op0]=(int64_t)registers[op0]%op1;
}

void global_jit_put(int op0) {
    cout << registers[op0];
}

void global_jit_putc(int op0) {
    printf("%c", (char)registers[op0]);
}

int64_t global_jit_getpc() {
    return thread_self->pc-thread_self->cache;
}

void global_jit_get(int op0) {
    if(_64CMT)
        registers[op0] = getche();
    else
        registers[op0] = getch();
}

void global_jit_popl(Object* o2, SharpObject* o) {
    *o2 = o;
}

void global_jit_del(Object* o) {
    GarbageCollector::self->releaseObject(o);
}

void global_jit_lock(Object* o) {
    o->monitorLock();
}

void global_jit_ulock(Object* o) {
    o->monitorUnLock();
}

void global_jit_call0(Thread *thread, int64_t addr) {
#ifdef SHARP_PROF_
    tprof->hit(env->methods+GET_Da(*pc));
#endif
    try {
        if ((thread->calls + 1) >= thread->stack_lmt) throw Exception(Environment::StackOverflowErr, "");
        executeMethod(addr, thread, true);
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void global_jit_call1(Thread* thread, int64_t addr) {
    int64_t val;
#ifdef SHARP_PROF_
    tprof->hit(env->methods+GET_Da(*pc));
#endif
    try {
        if ((val = (int64_t) registers[addr]) <= 0 || val >= manifest.methods) {
            stringstream ss;
            ss << "invalid call to pointer of " << val;
            throw Exception(ss.str());
        }

        if ((thread->calls + 1) >= thread->stack_lmt) throw Exception(Environment::StackOverflowErr, "");
        executeMethod(addr, thread, true);
    } catch(Exception &e) {
        __srt_cxxa_prepare_throw(e);
    }
}

void setupJitContextFields(const X86Gp &ctx) {
    Thread* thread = thread_self;
    jit_ctx_fields[jit_field_id_current] = x86::qword_ptr(ctx, relative_offset((thread->jctx), current, current)); // Thread *current
    jit_ctx_fields[jit_field_id_registers] = x86::qword_ptr(ctx, relative_offset((thread->jctx), current, registers)); // double *registers
    jit_ctx_fields[jit_field_id_func] = x86::qword_ptr(ctx, relative_offset((thread->jctx), current, func)); // Method *func

#ifdef SHARP_PROF_
    jit_ctx_fields[jit_field_id_ir] = x86::qword_ptr(ctx, relative_offset((&jctx), current, irCount)); // unsigned long long *irCount
    jit_ctx_fields[jit_field_id_overflow] = x86::qword_ptr(ctx, relative_offset((&jctx), current, overflow)); // unsigned long long *overflow
#endif
}

void setupThreadContextFields(const X86Gp &ctx) {
    Thread* thread = thread_self;
    thread_fields[jit_field_id_thread_calls] = x86::qword_ptr(ctx, relative_offset(thread, calls, calls)); // long long calls
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
    thread_fields[jit_field_id_thread_state] = x86::dword_ptr(ctx, relative_offset(thread, calls, state)); // unsigned int state
    thread_fields[jit_field_id_thread_signal] = x86::dword_ptr(ctx, relative_offset(thread, calls, signal)); // unsigned int state
}

void setupStackElementFields(const X86Gp &ctx) {
    StackElement stack;
    stack_element_fields[jit_field_id_stack_element_var] = x86::qword_ptr(ctx, relative_offset((&stack), var, var)); // double var;
    stack_element_fields[jit_field_id_stack_element_object] = x86::qword_ptr(ctx, relative_offset((&stack), var, object)); // Object object;
}

void setupFrameFields(const X86Gp &ctx) {
    Frame frame(0,0,0,0,false);
    frame_fields[jit_field_id_frame_last] = x86::qword_ptr(ctx, relative_offset((&frame), last, last)); // Method* last;
    frame_fields[jit_field_id_frame_pc] = x86::qword_ptr(ctx, relative_offset((&frame), last, pc)); // int64_t* pc;
    frame_fields[jit_field_id_frame_sp] = x86::qword_ptr(ctx, relative_offset((&frame), last, sp)); // StackElement* sp;
    frame_fields[jit_field_id_frame_fp] = x86::qword_ptr(ctx, relative_offset((&frame), last, fp)); // StackElement* fp;
}

void setupMethodFields(const X86Gp &ctx) {
    Method m;
    method_fields[jit_field_id_frame_last] = x86::qword_ptr(ctx, relative_offset((&m), jit_labels, bytecode)); // int64_t bytecode;
}

void setupSharpObjectFields(const X86Gp &ctx) {
    SharpObject o;
    sharp_object_fields[jit_field_id_shobj_HEAD] = x86::qword_ptr(ctx, relative_offset((&o), HEAD, HEAD)); // double* HEAD;
    sharp_object_fields[jit_field_id_shobj_node] = x86::qword_ptr(ctx, relative_offset((&o), HEAD, node)); // Object* node;
    sharp_object_fields[jit_field_id_shobj_k] = x86::qword_ptr(ctx, relative_offset((&o), HEAD, k)); // ClassObject* k;
    sharp_object_fields[jit_field_id_shobj_size] = x86::qword_ptr(ctx, relative_offset((&o), HEAD, size)); // int64_t size;
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
    for(int64_t i = 0; i < functions.len; i++) {
        rt.release(functions.get(i).func);
    }
}

void jit_call(int64_t serial, Thread* thread)
{
    Method * func = (thread->jctx->func = env->methods+serial);
    if(func->isjit) {
        sjit_function f = functions.get(func->jit_addr).func;
        f(thread->jctx);
    }
}
