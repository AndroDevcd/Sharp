//
// Created by braxtonn on 8/21/2019.
//

#ifndef SHARP_JITASSEMBLER_H
#define SHARP_JITASSEMBLER_H

#include "architecture.h"
#ifdef BUILD_JIT

#define ASMJIT_BUILD_X86 1
#define ASMJIT_STATIC   1
#define ASMJIT_BUILD_RELEASE 1

#include "../../../stdimports.h"
#include "../../util/jit/asmjit/src/asmjit/asmjit.h"
#include "../List.h"
#include "../symbols/Object.h"

using namespace asmjit;


class Thread;
class Method;
struct Constants;
struct SharpObject;
struct StackElement;

struct jit_context {
    Thread* self;
    fiber *this_fiber;
    Method* caller; // current method we are executing only used in initalization of the call
};

enum os {
    win_os=0x0ffa,
    linux_os=0x0ffb
};

#ifdef WIN32_
#define OS_id win_os
#endif
#ifdef POSIX_
#define OS_id linux_os
#endif

// Jit helper macros
#define offset_start(s) s
#define offset_end(e) e
#define relative_offset(obj, start, end) ((Int)&obj->offset_end(end)-(Int)&obj->offset_start(start))
#define is_op(ir, x) (GET_OP(ir)==(x))

class _BaseAssembler {
public:
    _BaseAssembler()
    :
        rt(),
        initialized(false)
    {
        functions.init();
    }

    void shutdown();
    int performInitialCompile();
    int tryJit(Method*);

protected:
    void initialize();
    bool initialized;
    FILE* getLogFile();
    _List<fptr> functions;

    x86::Gp ctx, ctx32;                  // total registers used in jit
    x86::Gp tmp, tmp32, tmp16, tmp8;
    x86::Gp arg2;
    x86::Gp fnPtr, fnPtr32, arg, arg3, arg4;
    x86::Gp regPtr, threadPtr, fiberPtr;

    x86::Xmm vec0, vec1;          // floating point registers

    x86::Mem Ljit_context[3];     // memory layout of struct jit_context {}
    x86::Mem Lthread[6];          // memory layout of class Thread {}
    x86::Mem Lfiber[12];           // memory layout of class fiber {}
    x86::Mem Lstack_element[2];   // memory layout of struct StackElement {}
    x86::Mem Lmethod[2];          // memory layout of struct Method {}
    x86::Mem Lsharp_object[4];    // memory layout of struct SharpObject {}

    JitRuntime rt;

private: // virtual functions
    virtual x86::Mem getMemPtr(Int addr) = 0;
    virtual x86::Mem getMemBytePtr(Int addr) = 0;
    virtual x86::Mem getMemPtr(x86::Gp reg, Int addr) = 0;
    virtual x86::Mem getMemPtr(x86::Gp reg) = 0;
    virtual Int getRegisterSize() = 0;
    virtual void createFunctionAndAllocateRegisters() = 0;
    virtual void createFunctionEpilogue() = 0;
    virtual void beginCompilation(Method*) = 0;
    virtual void endCompilation() = 0;
    virtual int addUserCode() = 0;
    virtual void addConstantsSection() = 0;

    // Method Logging fuctions
    virtual void logComment(std::string) = 0;

    // stack manipulation functions
    virtual void setupStackAndRegisterValues() = 0;

    // conrol flow functions
    virtual void setupGotoLabels() = 0;
    virtual void createFunctionLandmarks() = 0;
    virtual void setupAddressTable() = 0;
    virtual void storeLabelValues() = 0;
    virtual void validateVirtualStack() = 0;
    virtual void addThreadSignalCheck() = 0;
    virtual void addStackCheck() = 0;

    // setup functions
    void setupContextFields();
    void setupThreadFields();
    void setupFiberFields();
    void setupStackElementFields();
    void setupMethodFields();
    void setupSharpObjectFields();

    // helper functions
    virtual void incPc() = 0;
    virtual int createJitFunc() = 0;

    int compile(Method*);

protected:
    static void jitSysInt(Int signal);
    static int jitTryContextSwitch(Thread *thread, bool incPc);
    static void growStack(fiber *fib);
    static void jitStackOverflowException(Thread *);
    static void jitStackUnderflowException(Thread *);
    static SharpObject* jitNewObject(Int size, int ntype);
    static void jitCast(Object *o2, Int klass);
    static void jit64BitCast(Int,Int);
    static void jitPop(fiber *);
    static void jitPut(int reg, double *regs);
    static void jitPutC(int op0, double *regs);
    static void jitNullPtrException(Thread*);
    static void jitIndexOutOfBoundsException(Int size, Int index, Thread* thread);
    static void jitSetObject2(Object *dest, Object *src);
    static void jitSetObject0(SharpObject* obj, StackElement *sp);
    static void jitCastVar(Object *o2, int, int);
    static void jitDelete(Object* o);
    static fptr jitCall(Thread *thread, int64_t addr);
    static SharpObject* jitNewClass0(Int classid, Thread* thread);
    static SharpObject* jitNewObject2(Int size, Thread *thread);
    static SharpObject* jitNewClass1(Int size, Int classid, Thread *);
    static void jitNewString(Thread* thread, int64_t strid);
    static void jitPushNil(Thread* thread);


    static int jitTryCatch(Method *method);
    static Int jitGetPc(Thread *thread);
    static void jitSetObject1(StackElement*, StackElement*);
    static void jitSetObject3(Object *dest, SharpObject *src);
    static void jitInvokeDelegate(Int address, Int args, Thread* thread, Int staticAddr);

    static void test(Int proc, Int xtra);
    static fptr jitCallDynamic(Thread *thread, int64_t addr);
    static void jitGet(int op0);
    static void jitThrow(Thread *thread);
    static void jitIllegalStackSwapException(Thread*);

};

enum ConstKind {
    kConst64,
    kConstFloat
};

struct Constants {
    _List<double> constants;
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

    Int createConstant(x86::Compiler& cc, Int const0) {
        Int idx = _64ConstIndex(const0);

        if(idx == -1) {
            constants.add(const0);
            constantKinds.add(kConst64);
            constantLabels.add(cc.newLabel());
            return constants.size();
        }

        return idx;
    }

    Int createConstant(x86::Compiler& cc, double const0) {
        Int idx = _floatConstIndex(const0);

        if(idx == -1) {
            constants.add(const0);
            constantKinds.add(kConstFloat);
            constantLabels.add(cc.newLabel());
            return constants.size() - 1;
        }

        return idx;
    }

    Int constantSize() {
        return constants.size();
    }

    Int _64ConstIndex(Int const0) {
        for(Int i = 0; i < constants.size(); i++) {
            Int const_ = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConst64 && const_ == const0) {
                return i;
            }
        }

        return -1;
    }

    Int _floatConstIndex(double const0) {
        for(Int i = 0; i < constants.size(); i++) {
            double const_ = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConstFloat && const_ == const0) {
                return i;
            }
        }

        return -1;
    }

    Label& getConstantLabel(Int idx) {
        return constantLabels.get(idx);
    }

    void emitConstants(x86::Compiler& cc) {

        for(Int i = 0; i < constantLabels.size(); i++) {
            cc.bind(constantLabels.get(i));
            double const0 = constants.get(i);
            ConstKind kind = constantKinds.get(i);

            if(kind == kConst64) {
                cc.embedInt64(const0);
            } else if(kind == kConstFloat) {
                cc.embedDouble(const0);
            }
        }
    }
};



// struct jit_context {} fields
#define jit_context_self   0
#define jit_context_fiber  1
#define jit_context_caller 2

// class fiber {} fields
#define fiber_current   0
#define fiber_callStack 1
#define fiber_calls     2
#define fiber_dataStack 3
#define fiber_sp        4
#define fiber_fp        5
#define fiber_stack_lmt 6
#define fiber_cache     7
#define fiber_pc        8
#define fiber_regs      9
#define fiber_stack_sz 10
#define fiber_ptr      11

// class Thread {} fields
#define thread_state             0
#define thread_signal            1
#define thread_stbase            2
#define thread_stack             3
#define thread_stack_rebuild     4
#define thread_context_switching 5

// struct StackElement {} fields
#define stack_element_var    0
#define stack_element_object 1

// struct Frame {} fields
#define frame_current 0
#define frame_pc      1
#define frame_sp      2
#define frame_fp      3

// struct Method {} fields
#define method_bytecode 0
#define method_jit_labels 1

// struct SharpObject {} fields
#define sharp_object_HEAD 0
#define sharp_object_node 1
#define sharp_object_size 2
#define sharp_object_info 3

#endif

// handy macros to use
#define JIT_MAX_ATTEMPTS 3

// version of out jit compiler
#define JIT_VERSION 2
#endif //SHARP_JITASSEMBLER_H
