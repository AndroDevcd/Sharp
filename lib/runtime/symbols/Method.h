//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H

#include "../../../stdimports.h"
#include "Object.h"
#include "string.h"
#include "../../grammar/Exception.h"
#include "../List.h"
#include "../Opcode.h"
#include "param.h"

struct LineData;

#ifdef BUILD_JIT
struct jit_context;
#endif

#define JIT_IR_LIMIT 25000

/**
 * The JIT will not waste time JIT'ing functions with only 5 instructions or less
 */
#define JIT_IR_MIN 1

typedef opcode_instr* Cache;
typedef void (*fptr)(void *);

/**
 * This is the representation of a method in its barest form
 * it will contain all the information to run correctly in the system
 *
 */
class Method : public Symbol {
public:
    int32_t* jit_labels;
    int32_t address;
    uint32_t* bytecode;
    DataType type;
    int stackSize;
    int paramSize;
    Symbol* utype;
    bool arrayUtype;
    long sourceFile;
    int32_t cacheSize;
    long delegateAddress;
    int spOffset;
    int fpOffset;
    Param* params;
    _List<TryCatchData> tryCatchTable;
    _List<LineData> lineTable;

    /**
     * Below are all the jit related fields
     *
     * This will contain the total number of calls to a single function.
     * It is called long calls because it holds the calls made to a function as well as
     * "back calls" as explained below
     *
     * def foo() {
     *
     * }
     *
     * def main() { // main is called 1 times
     *      for < 10:
     *          foo(); // foo is called 10 times
     *      // with backlogging calls main is actually "called" 11 times
     * }
     *
     * We also track how many branches are performed inside a function. branches in a function
     * will be tracked only at the interpreted level. They are treated as long calls as well because
     * they give further information that this particular function is doing quite a bit of work.
     *
     * Currently the limit for long calls will JIT any function that exceeds the max long call limit of
     * 25,000. This includes any branches included in this number
     */
    int16_t branches;
    int8_t jitAttempts; // we only allow 3 attempts to JIT a method
    int isjit;
    bool compiling; // are we compiling the function?
    fptr jit_func;


    void free() {
        Symbol::free();
        sourceFile=0;
        tryCatchTable.free();
        lineTable.free();
        isjit=0;
        branches=0;
        jit_func=0;
        compiling=false;
        jitAttempts=0;
        if(params != NULL) {
            std::free(params);
        }

        owner = NULL;
        if(bytecode != NULL) {
            std::free(bytecode);
        }

        if(jit_labels != NULL) {
            std::free(jit_labels);
        }
    }

    void init() {
        Symbol();
        name.init();
        fullName.init();
        isjit=false;
        branches=0;
        jit_func=0;
        compiling=false;
        jitAttempts=0;
        sourceFile=0;
        tryCatchTable.init();
        lineTable.init();
        params = NULL;
        owner = NULL;
        jit_labels = NULL;
        stackSize = 0;
        bytecode = NULL;
        address = 0;
        cacheSize = 0;
        spOffset = 0;
        fpOffset = 0;
        delegateAddress = -1;
    }
};

struct LineData {
public:
    LineData(Int pc, Int line)
    :
        pc(pc),
        line_number(line)
    {}

    Int pc;
    Int line_number;
};

struct StackElement;

struct Frame {
public:
    Frame(Method* current, Cache pc, StackElement* sp,
          StackElement* fp, bool jit)
    {
        this->current=current;
        this->pc=pc;
        this->sp=sp;
        this->fp=fp;
        this->isjit=jit;
    }

    void init(Method* current, Cache pc, StackElement* sp,
              StackElement* fp, bool jit)
    {
        this->current=current;
        this->pc=pc;
        this->sp=sp;
        this->fp=fp;
        this->isjit=jit;
    }

    Method *current;
    Cache pc;
    StackElement* sp;
    StackElement* fp;
    bool isjit;
};

#pragma optimize( "", off )
struct StackElement {
    double var;
    Object object;

    void modul(int64_t v) {
        var = (int64_t)var%v;
    }

    void andl(int64_t v) {
        var = (int64_t)var&v;
    }

    void orl(int64_t v) {
        var = (int64_t)var|v;
    }

    void xorl(int64_t v) {
        var = (int64_t)var^v;
    }
};
#pragma optimize( "", on )

#endif //SHARP_METHOD_H
