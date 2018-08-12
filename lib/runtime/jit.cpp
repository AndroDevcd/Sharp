//
// Created by BraxtonN on 8/9/2018.
//

#include "jit.h"
#include "Opcode.h"
#include "Thread.h"
#include "../util/KeyPair.h"
#include <stdio.h>
#include <fstream>

using namespace asmjit;

/**
 * Global reference to our runtime engine
 */
JitRuntime rt;

thread_local jit_ctx jctx;
std::vector<jit_func> functions;
uint64_t jit_funcid = 0;
X86Mem jit_ctx_fields[5]; // memory map of jit_ctx
X86Mem thread_fields[9]; // sliced memory map of Thread
X86Mem stack_element_fields[2]; // memory map of StackElement

void setupJitContextFields(const X86Gp &ctx);
void setupThreadContextFields(const X86Gp &ctx);
void setupStackElementFields(const X86Gp &ctx);

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
        outfile << "";                          // clear it
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
         *     int64_t* labels;
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
        int labelsSize   = sizeof(int64_t*), laddr = paddr + labelsSize;
        int klassSize    = sizeof(ClassObject*), kaddr = laddr + klassSize;
        int objectSize   = sizeof(SharpObject*), oaddr = kaddr + objectSize;
        int o2Size       = sizeof(Object*), o2addr = oaddr + o2Size;
        int stackSize = ptrSize + /* tmpSize + valSize + delegateSize + argsSize */
                + labelsSize + klassSize + objectSize + o2Size;
        cc.sub(zsp, (stackSize));                  // allocate nessicary bytes on the stack for variables

        Label labels[sz];                       // Each opcode has its own labels
        for(int64_t i = 0; i < sz; i++) {       // Iterate through all the addresses to create labels for each address
            labels[i] = cc.newLabel();
        }

        // Emit all assembly code below

        X86Mem ctxPtr = x86::dword_ptr(zbp, -paddr); // store memory location of ctx pointer in the stack
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

        Constants lconsts;                              // local constant holder for function

        cc.mov(labelsPtr, ctx);                         // int64_t* labels = ctx->func->jit_labels;
        X86Mem tmp_jit_labels = x86::qword_ptr(ctx, 0); // labels[0]

        cc.mov(ctx, labelsPtr);                         // get int64_t* labels value
        cc.mov(ctx, tmp_jit_labels);                    // if(ctx->func->jit_labels[0]==0)
        cc.test(ctx, ctx);                              //      goto end;
        cc.jne(lbl_begin);
        cc.nop();
        cc.jmp(lbl_end);
        cout << "sizeof int " << sizeof(int) << endl;
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

            cc.bind(labels[i]);
            switch(GET_OP(x64)) {
                case op_NOP: {
                    cc.nop();                   // by far one of the easiest instructions yet
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
                    if(*bc == 0) {
                        cc.pxor(vec0, vec0);
                    } else {

                        idx = lconsts.createConstant(cc, (double)(*bc));
                        lconstMem = qword_ptr(lconsts.getConstant(idx).df[0]);        // load constant value to stack
                        cc.movsd(vec0, lconstMem);
                    }
                    cc.movsd(stack_element_fields[jit_field_id_stack_element_var], vec0);
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
        cc.mov(tmp, labelsPtr);                         // get a temporary ref to int64_t* labels
        X86Mem labelIndex = qword_ptr(tmp);
        X86Mem lbl;
        for(int64_t i = 0; i < sz; i++) {
            lbl = ptr(labels[i]);
            cc.mov(ctx, lbl);
            cc.mov(labelIndex, ctx);

            if((i + 1) < sz)                       // omit unessicary add instruction
                cc.add(labelIndex, (int64_t )sizeof(int64_t));
        }

        cc.xor_(tmp, tmp);                      // tmp = 0;
        cc.jmp(lbl_begin);                     // jump back to top to execute user code

        // end of function
        cc.bind(lbl_funcend);
        cc.nop();

//        X86Mem realmem = qword_ptr(l);        // load a double literal example
//        cc.movsd(vec0, realmem);

        FuncUtils::emitEpilog(&cc, layout);    // Emit function epilog and return.

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

void setupJitContextFields(const X86Gp &ctx) {
    int64_t sz = 0; // holds the growing size of the data
    jit_ctx_fields[jit_field_id_current] = x86::qword_ptr(ctx, 0); // Thread *current
    jit_ctx_fields[jit_field_id_registers] = x86::qword_ptr(ctx, SIZE(sizeof(Thread*))); // double *registers
    jit_ctx_fields[jit_field_id_vm] = x86::qword_ptr(ctx, SIZE(sz + sizeof(double*))); // VirtualMachine *vm
    jit_ctx_fields[jit_field_id_env] = x86::qword_ptr(ctx, SIZE(sz + sizeof(VirtualMachine*))); // Environment *env
    jit_ctx_fields[jit_field_id_func] = x86::qword_ptr(ctx, SIZE(sz + sizeof(Environment*))); // Method *func
}

void setupThreadContextFields(const X86Gp &ctx) {
    int64_t sz = 0; // holds the growing size of the data
    thread_fields[jit_field_id_thread_dataStack] = x86::qword_ptr(ctx, 0); // StackElement *dataStack;
    thread_fields[jit_field_id_thread_sp] = x86::qword_ptr(ctx, SIZE(sizeof(StackElement*))); // StackElement *sp
    thread_fields[jit_field_id_thread_fp] = x86::qword_ptr(ctx, SIZE(sz*2)); // StackElement *sp
    thread_fields[jit_field_id_thread_current] = x86::qword_ptr(ctx, SIZE(sz*3)); // Method *current
    thread_fields[jit_field_id_thread_callStack] = x86::qword_ptr(ctx, SIZE(sz + sizeof(Method*))); // Frame *callStack
    thread_fields[jit_field_id_thread_calls] = x86::qword_ptr(ctx, SIZE(sz + sizeof(Frame*))); // unsigned long calls
    thread_fields[jit_field_id_thread_stack_lmt] = x86::qword_ptr(ctx, SIZE(sz + sizeof(unsigned long))); // unsigned long stack_lmt
    thread_fields[jit_field_id_thread_cache] = x86::qword_ptr(ctx, SIZE(sz + sizeof(unsigned long))); // int64_t *cache
    thread_fields[jit_field_id_thread_pc] = x86::qword_ptr(ctx, SIZE(sz + sizeof(int64_t *))); // int64_t *pc
}

void setupStackElementFields(const X86Gp &ctx) {
    int64_t sz = 0; // holds the growing size of the data
    stack_element_fields[jit_field_id_stack_element_var] = x86::qword_ptr(ctx, 0); // double var;
    stack_element_fields[jit_field_id_stack_element_object] = x86::qword_ptr(ctx, SIZE(sizeof(double))); // Object object;
}

void releaseMem()
{
    for(std::vector<jit_func>::iterator it = functions.begin(); it != functions.end(); ++it) {
        rt.release((*it).func);
    }
}


void call(jit_ctx *ctx, int64_t serial)
{

}