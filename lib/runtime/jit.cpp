//
// Created by BraxtonN on 8/9/2018.
//

#include "jit.h"
#include "Opcode.h"
#include <stdio.h>

using namespace asmjit;

/**
 * Global reference to our runtime engine
 */
JitRuntime rt;

thread_local jit_ctx* jctx;
std::vector<jit_func> functions;
uint64_t jit_funcid = 0;

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

        CodeHolder code;                        // Holds code and relocation information.
        code.init(rt.getCodeInfo());            // Initialize to the same arch as JIT runtime.
        FileLogger logger(stdout);
        code.setLogger(&logger);                // Initialize logger temporarily to ensure quality of code

        X86Assembler cc(&code);                  // Create and attach X86Compiler to `code`.

        // Decide which registers will be mapped to function arguments.
        // registers zax for `ctx` is used to store the pointer to the jit context
        X86Gp ctx   = cc.zax();

        X86Gp tmp        = x86::gpq(0);       // setup register locations for local variables
        X86Gp val        = x86::gpq(1);       // virtual registers are r0, r1, r2, and r3
        X86Gp delegate   = x86::gpq(2);       // each time a function is called that can mess with the state of these registers
        X86Gp argsReg    = x86::gpq(3);       // they must be pushed to the stack to be retained

        // we need these for stack manip
        X86Gp zbp = cc.zbp();
        X86Gp zsp = cc.zsp();
        X86Gp zdi = cc.zdi();
        X86Gp zax = cc.zax();


        X86Xmm vec0 = xmm0;
        X86Xmm vec1 = xmm1;

        FuncDetail fd;
        fd.init(FuncSignature1<void, jit_ctx*>(CallConv::kIdHost));

        FuncFrameInfo ffi;
        ffi.setDirtyRegs(X86Reg::kKindVec,      // Make XMM0 and XMM1 dirty. VEC kind
                         Utils::mask(0, 1));    // describes XMM|YMM|ZMM registers.
        ffi.enablePreservedFP();
        ffi.enableCalls();


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

        int64_t malloc_ =                     // pointer address to malloc function
                (int64_t )std::malloc;

        cc.mov(edi, (uint64_t )(sizeof(uint64_t)*sz));
        cc.call(malloc_);          //   Function signature.
        cc.mov(labelsPtr, rax);

        /**
         * Solution for dynamic label problem
         *
         * void jit_function0(jit_ctx *ctx) {
         *     ...
         *     goto end;
         *     begin:
         *        x86 code starts here
         *
         *     end:
         *        new int64[labelsSz]; // sudocode to create a new array for the labels
         *        set int64 array to label values
         *        goto begin;
         */
        Error error = 0;
        int64_t x64;
        for(int64_t i = 0; i < sz; i++) {
            if(error) break;
            x64 = *bc;

            cc.bind(labels[i]);
            switch(GET_OP(x64)) {
                case op_NOP: {
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

        FuncUtils::emitEpilog(&cc, layout);    // Emit function epilog and return.

        if(!error) {

            sjit_function jitfn;
            Error err = rt.add(&jitfn, &code);   // Add the generated code to the runtime.
            if (err) return 1;
            else {
                jitfn(nullptr);
                jit_func jfunc;
                jfunc.func = jitfn;
                jfunc.serial = jit_funcid++;
                jfunc.rAddr = method->address;
                functions.push_back(jfunc);
            }
        }
    }

    releaseMem();
    return 0;
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