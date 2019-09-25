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

struct line_table;
struct jit_context;

struct SwitchTable { // for every value there will be a corresponding address
    _List<int64_t> values;
    _List<int64_t> addresses;

    int64_t defaultAddress; // -1 if not present

    void init() {
        values.init();
        addresses.init();
        defaultAddress = -1;
    }

    SwitchTable()
    {
        init();
    }
};

#define JIT_IR_LIMIT 25000

/**
 * The JIT will not waste time JIT'ing functions with only 5 instructions or less
 */
#define JIT_IR_MIN 1

typedef int64_t* Cache;
typedef void (*fptr)(jit_context *);

/**
 * This is the representation of a method in its barest form
 * it will contain all the information to run correctly in the system
 *
 */
struct Method {                     /* WARNING:  DO NOT!!!!!!! CHANGE THIS STRUCT DATA USED BY THE JIT */
    int64_t* jit_labels;
    int64_t address;          /* refrence id to the the address space */

    int64_t* bytecode;
    int stackSize;                 /* inital stack space required for frame */
    short int returnVal;           /* Simple binary flag indicating the function returns a value */
    ClassObject* owner;
    native_string name, fullName;
    int64_t* params;
    bool* arrayFlag;                /* array flag for each parameter */
    int paramSize;
    long sourceFile;                /* Link to source file in code */
    int64_t cacheSize;              /* Size of the bytecode cache */
    int isStatic;
    long delegateAddress;
    int stackEqulizer;
    _List<ExceptionTable> exceptions;
    _List<FinallyTable> finallyBlocks;
    _List<line_table> lineNumbers;
    _List<SwitchTable> switchTable;
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
    int16_t longCalls;
    int8_t jitAttempts; // we only allow 3 attempts to JIT a method
    int isjit;
    bool compiling; // are we compiling the function?
    fptr jit_call;


    void free() {
        name.free();
        fullName.free();
        sourceFile=0;
        exceptions.free();
        lineNumbers.free();
        finallyBlocks.free();
        isjit=0;
        longCalls=0;
        jit_call=0;
        compiling=false;
        jitAttempts=0;
        for(long i = 0; i < switchTable.size(); i++) {
            SwitchTable &st = switchTable.get(i);
            st.values.free();
            st.addresses.free();
        }

        switchTable.free();

        if(paramSize != 0) {
            if(arrayFlag != NULL)
                std::free(arrayFlag);
            if(params != NULL)
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
        name.init();
        fullName.init();
        isjit=false;
        longCalls=0;
        jit_call=0;
        compiling=false;
        jitAttempts=0;
        sourceFile=0;
        exceptions.init();
        lineNumbers.init();
        finallyBlocks.init();
        switchTable.init();
        params = NULL;
        arrayFlag = NULL;
        paramSize = 0;
        owner = NULL;
        jit_labels = NULL;
        stackSize = 0;
        bytecode = NULL;
        address = 0;
        cacheSize = 0;
        isStatic = 0;
        returnVal = 0;
        stackEqulizer = 0;
        delegateAddress = -1;
    }
};

struct line_table {
    int64_t pc;
    int64_t line_number;
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
