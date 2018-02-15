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

/**
 * This is the representation of a method in its barest form
 * it will contain all the information to run correctly in the system
 *
 */
struct Method {
    unsigned long address;          /* refrence id to the the address space */

    int64_t* bytecode;
    int stackSize;                 /* inital stack space required for frame */
    ClassObject* owner;
    native_string name;
    int64_t* params;
    bool* arrayFlag;                /* array flag for each parameter */
    int paramSize;
    long sourceFile;                /* Link to source file in code */
    int64_t cacheSize;              /* Size of the bytecode cache */
    bool isStatic;
    List<ExceptionTable> exceptions;
    List<FinallyTable> finallyBlocks;
    List<line_table> lineNumbers;


    void free() {
        name.free();
        sourceFile=0;
        exceptions.free();
        lineNumbers.free();
        finallyBlocks.free();

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
    }

    void init() {
        name.init();
        sourceFile=0;
        exceptions.init();
        lineNumbers.init();
        finallyBlocks.init();
        params = NULL;
        arrayFlag = NULL;
        paramSize = 0;
        owner = NULL;
        stackSize = 0;
        bytecode = NULL;
        address = 0;
        cacheSize = 0;
        isStatic = false;
    }
};

struct line_table {
    int64_t pc;
    int64_t line_number;
};

struct Frame {
    Method *last;                   /* Last method */
    unsigned int pc;
    unsigned int sp;
    unsigned int fp;
};

struct StackElement {
    double var;
    Object object;

    void modul(int64_t v) {
        var = (int64_t)var&v;
    }

    void andl(int64_t v) {
        var = (int64_t)var&v;
    }

    void orl(int64_t v) {
        var = (int64_t)var|v;
    }

    void notl(int64_t v) {
        var = (int64_t)var^v;
    }
};

#define ret_frame(x) \
    if(curr_adsp == main->id) return; else { x }

#define ret_frame2(x, ret) \
    if(curr_adsp == main->id) return ret; else { x }

#define pc_offset 2
#define fp_offset 3
#define sp_offset 4

typedef int64_t* Cache;

#endif //SHARP_METHOD_H
