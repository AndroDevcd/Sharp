//
// Created by braxtonn on 8/21/2019.
//

#ifndef SHARP_JITASSEMBLER_H
#define SHARP_JITASSEMBLER_H

#define ASMJIT_EMBED 1

#include "../util/jit/asmjit/src/asmjit/asmjit.h"
#include "List.h"

using namespace asmjit;


class Thread;
class Method;
struct Constants;
struct SharpObject;
struct StackElement;

struct jit_context {
    Thread* self;
    double *regs;
    Method* caller; // current method we are executing only used in initalization of the call
};

typedef void (*fptr)(jit_context *);

#ifdef ASMJIT_ARCH_X64
typedef int64_t x86int_t;
#else
#define ASMJIT_ARCH_X86
typedef int32_t x86int_t;
#endif

// Jit helper macros
#define offset_start(s) s
#define offset_end(e) e
#define relative_offset(obj, start, end) ((x86int_t)&obj->offset_end(end)-(x86int_t)&obj->offset_start(start))
#define is_op(ir, x) (GET_OP(ir)==(x))

class JitAssembler {
public:
    JitAssembler()
    :
        rt()
    {
        functions.init();
    }

    void shutdown();
    int performInitialCompile();
    int tryJit(Method*);
    static void jitSysInt(x86int_t signal);

protected:
    void initialize();

    X86Gp ctx, ctx32;                  // total registers used in jit
    X86Gp tmp, tmp32, value;
    X86Gp fnPtr, arg;
    X86Gp regPtr, threadPtr;
    X86Gp bp, sp;

    X86Xmm vec0, vec1;          // floating point registers

private:
    static int jitTryCatch(Method *method);
    static x86int_t jitGetPc(Thread *thread);
    static void __srt_cxx_prepare_throw(Exception &e);
    static SharpObject* jitNewObject(x86int_t size);
    static void jitSetObject0(SharpObject* o, StackElement *sp);
    static void test(x86int_t proc);

    virtual X86Mem getMemPtr(x86int_t addr) = 0;
    virtual X86Mem getMemPtr(X86Gp reg, x86int_t addr) = 0;
    virtual X86Mem getMemPtr(X86Gp reg) = 0;
    virtual x86int_t getRegisterSize() = 0;
    virtual void initializeRegisters() = 0;

    void setupContextFields();
    void setupThreadFields();
    void setupStackElementFields();
    void setupFrameFields();
    void setupMethodFields();
    void setupSharpObjectFields();
    int compile(Method*);
    void incPc(X86Assembler &assembler);
    void updatePc(X86Assembler &assembler);
    void threadStatusCheck(X86Assembler &assembler, Label &retLbl, Label &lbl_thread_sec, x86int_t irAddr);
    void checkMasterShutdown(X86Assembler &assembler, int64_t pc, const Label &lbl_funcend);
    void emitConstant(X86Assembler &assembler, Constants &cpool, double _const);
    void movRegister(X86Assembler &assembler, X86Xmm &vec, x86int_t addr, bool store = true);
    FILE* getLogFile();

    JitRuntime rt;
    _List<fptr> functions;
    X86Mem Ljit_context[3];     // memory layout of struct jit_context {}
    X86Mem Lthread[13];         // memory layout of class Thread {}
    X86Mem Lstack_element[2];   // memory layout of struct StackElement {}
    X86Mem Lframe[4];           // memory layout of struct Frame {}
    X86Mem Lmethod[1];          // memory layout of struct Method {}
    X86Mem Lsharp_object[4];    // memory layout of struct SharpObject {}
};

enum ConstKind {
    kConst64,
    kConstFloat
};

struct Constants {
    _List<Data64> constants;
    _List<ConstKind> constantKinds;
    _List<Label> constantLabels;

    Constants()
            :
            constants(),
            constantKinds(),
            constantLabels()
    {}

    ~Constants() {
        constants.free();
        constantLabels.free();
    }

    x86int_t createConstant(X86Assembler& cc, x86int_t const0) {
        x86int_t idx = _64ConstIndex(const0);

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

    x86int_t createConstant(X86Assembler& cc, double const0) {
        x86int_t idx = _floatConstIndex(const0);

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

    x86int_t constantSize() {
        return constants.size();
    }

    x86int_t _64ConstIndex(x86int_t const0) {
        for(x86int_t i = 0; i < constants.size(); i++) {
            Data64 &const_ = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConst64 && const_.sq[0] == const0) {
                return i;
            }
        }

        return -1;
    }

    x86int_t _floatConstIndex(double const0) {
        for(x86int_t i = 0; i < constants.size(); i++) {
            Data64 &const_ = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConstFloat && const_.df[0] == const0) {
                return i;
            }
        }

        return -1;
    }

    Data64& getConstant(x86int_t idx) {
        return constants.get(idx);
    }

    Label& getConstantLabel(x86int_t idx) {
        return constantLabels.get(idx);
    }

    void emitConstants(X86Assembler& cc) {

        for(x86int_t i = 0; i < constantLabels.size(); i++) {
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



// struct jit_context {} fields
#define jit_context_self   0
#define jit_context_regs   1
#define jit_context_caller 2

// class Thread {} fields
#define thread_current   0
#define thread_callStack 1
#define thread_calls     2
#define thread_dataStack 3
#define thread_sp        4
#define thread_fp        5
#define thread_stack_lmt 6
#define thread_cache     7
#define thread_pc        8
#define thread_state     9
#define thread_signal    10
#define thread_stbase    11
#define thread_stack     12

// struct StackElement {} fields
#define stack_element_var    0
#define stack_element_object 1

// struct Frame {} fields
#define frame_last 0
#define frame_pc   1
#define frame_sp   2
#define frame_fp   3

// struct Method {} fields
#define method_bytecode 0

// struct SharpObject {} fields
#define sharp_object_HEAD 0
#define sharp_object_node 1
#define sharp_object_k    2
#define sharp_object_size 3

// handy macros to use
#define JIT_MAX_ATTEMPTS 3

// version of out jit compiler
#define JIT_VERSION 2

#endif //SHARP_JITASSEMBLER_H
