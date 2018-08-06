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

struct SwitchTable { // for every value there will be a corresponding address
    List<int64_t> values;
    List<int64_t> addresses;

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

typedef int64_t* Cache;

/**
 * This is the representation of a method in its barest form
 * it will contain all the information to run correctly in the system
 *
 */
struct Method {
    unsigned long address;          /* refrence id to the the address space */

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
    List<ExceptionTable> exceptions;
    List<FinallyTable> finallyBlocks;
    List<line_table> lineNumbers;
    List<SwitchTable> switchTable;


    void free() {
        name.free();
        fullName.free();
        sourceFile=0;
        exceptions.free();
        lineNumbers.free();
        finallyBlocks.free();
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
    }

    void init() {
        name.init();
        fullName.init();
        sourceFile=0;
        exceptions.init();
        lineNumbers.init();
        finallyBlocks.init();
        switchTable.init();
        params = NULL;
        arrayFlag = NULL;
        paramSize = 0;
        owner = NULL;
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
    Frame(Method* last, Cache pc, StackElement* sp,
          StackElement* fp)
    {
        this->last=last;
        this->pc=pc;
        this->sp=sp;
        this->fp=fp;
    }

    void init(Method* last, Cache pc, StackElement* sp,
              StackElement* fp)
    {
        this->last=last;
        this->pc=pc;
        this->sp=sp;
        this->fp=fp;
    }

    Method *last;                   /* Last method */
    Cache pc;
    StackElement* sp;
    StackElement* fp;
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

    void notl(int64_t v) {
        var = (int64_t)var^v;
    }
};
#pragma optimize( "", on )

#endif //SHARP_METHOD_H
