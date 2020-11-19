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
#include "../../grammar/backend/oo/FunctionType.h"

struct LineData;

#ifdef BUILD_JIT
struct jit_context;
#endif

#define JIT_IR_LIMIT 15000

/**
 * The JIT will not waste time JIT'ing functions with only 5 instructions or less
 */
#define JIT_IR_MIN 5

typedef opcode_instr* Cache;
typedef void (*bridgeFun)(long handle);
typedef long (*linkProc)(const char* funcName);
typedef short (*lib_handshake)(void *lib_funcs[], int);

/**
 * This is the representation of a method in its barest form
 * it will contain all the information to run correctly in the system
 *
 */
class Method : public Symbol {
public:
    uint32_t* bytecode;
    function_type fnType;
    int stackSize;
    int paramSize;
    Symbol* utype;
    bool arrayUtype;
    long sourceFile;
    int32_t cacheSize;
    long delegateAddress;
    int spOffset;
    int fpOffset;
    int frameStackOffset;
    bool nativeFunc;
    uint32_t linkAddr;
    bridgeFun bridge;
    Param* params;
    _List<TryCatchData> tryCatchTable;
    _List<LineData> lineTable;

    void free() {
        Symbol::free();
        sourceFile=0;
        tryCatchTable.free();
        lineTable.free();
        linkAddr=0;
        if(params != NULL) {
            std::free(params);
        }

        owner = NULL;
        if(bytecode != NULL) {
            std::free(bytecode);
        }
    }

    void init() {
        Symbol::init();
        name.init();
        fullName.init();
        sourceFile=0;
        linkAddr=0;
        tryCatchTable.init();
        lineTable.init();
        params = NULL;
        owner = NULL;
        utype = NULL;
        stackSize = 0;
        bytecode = NULL;
        address = 0;
        cacheSize = 0;
        spOffset = 0;
        fpOffset = 0;
        frameStackOffset = 0;
        delegateAddress = -1;
        type = METHOD;
        nativeFunc = false;
        bridge = NULL;
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
    Frame(Method* returnAddress, int32_t pc, int32_t sp,
          int32_t fp, bool nativeCall)
    {
        this->returnAddress=returnAddress ? returnAddress->address : 0;
        this->pc=pc;
        this->sp=sp;
        this->fp=fp;
        this->isNative= nativeCall ? 1 : 0;
    }

    void init(Method* returnAddress, int32_t pc, int32_t sp,
              int32_t fp, bool nativeCall)
    {
        this->returnAddress=returnAddress ? returnAddress->address : 0;
        this->pc=pc;
        this->sp=sp;
        this->fp=fp;
        this->isNative= nativeCall ? 1 : 0;
    }

    int32_t returnAddress;
    int32_t pc;
    int32_t sp;
    int32_t fp;
    unsigned short isNative: 1;
};

#pragma optimize( "", off )
struct StackElement {
    double var;
    Object object;

    void modul(Int v) {
        var = (Int)var%v;
    }

    void andl(Int v) {
        var = (Int)var&v;
    }

    void orl(Int v) {
        var = (Int)var|v;
    }

    void xorl(Int v) {
        var = (Int)var^v;
    }
};
#pragma optimize( "", on )

#endif //SHARP_METHOD_H
