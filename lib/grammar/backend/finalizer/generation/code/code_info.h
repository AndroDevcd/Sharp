//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_CODE_INFO_H
#define SHARP_CODE_INFO_H

#include "../../../../../../stdimports.h"
#include "../../../../compiler_info.h"
#include "../../../../../runtime/Opcode.h"

struct line_info {
    line_info()
            :
            start_pc(0),
            line(0)
    {
    }

    line_info(uInt startPc, Int line)
            :
            start_pc(startPc),
            line(line)
    {
    }

    line_info(const line_info &li)
            :
            start_pc(0),
            line(0)
    {
        copy(li);
    }

    void copy(const line_info &li);

    uInt start_pc;
    Int line;
};

struct catch_data {
    catch_data()
            :
            handlerPc(-1),
            id(-1),
            exceptionFieldAddress(-1),
            classAddress(-1)
    {
    }

    catch_data(Int id)
            :
            handlerPc(-1),
            id(id),
            exceptionFieldAddress(-1),
            classAddress(-1)
    {
    }

    catch_data(const catch_data &cd)
            :
            handlerPc(cd.handlerPc),
            id(cd.id),
            exceptionFieldAddress(cd.exceptionFieldAddress),
            classAddress(cd.classAddress)
    {
    }

    ~catch_data()
    {
    }

    int id;
    Int handlerPc;
    Int exceptionFieldAddress;
    Int classAddress;
};

struct finally_data {
    finally_data()
            :
            startPc(-1),
            endPc(-1),
            id(-1),
            exceptionFieldAddress(-1)
    {
    }

    finally_data(Int id)
            :
            startPc(-1),
            endPc(-1),
            id(id),
            exceptionFieldAddress(-1)
    {
    }

    finally_data(const finally_data &fd)
            :
            startPc(fd.startPc),
            endPc(fd.endPc),
            id(fd.id),
            exceptionFieldAddress(fd.exceptionFieldAddress)
    {
    }

    ~finally_data()
    {
    }

    Int id;
    uInt exceptionFieldAddress;
    uInt startPc, endPc;
};

struct try_catch_data {
    try_catch_data()
            :
            tryStartPc(-1),
            tryEndPc(-1),
            id(-1),
            blockStartPc(-1),
            blockEndPc(-1),
            finallyData(NULL),
            catchTable()
    {
    }

    try_catch_data(Int id)
            :
            tryStartPc(-1),
            tryEndPc(-1),
            id(id),
            blockStartPc(-1),
            blockEndPc(-1),
            finallyData(NULL),
            catchTable()
    {
    }

    try_catch_data(const try_catch_data &tcd)
            :
            tryStartPc(tcd.tryStartPc),
            tryEndPc(tcd.tryEndPc),
            id(tcd.id),
            blockStartPc(tcd.blockStartPc),
            blockEndPc(tcd.blockEndPc),
            finallyData(NULL),
            catchTable()
    {
        if(tcd.finallyData)
            finallyData = new finally_data(*tcd.finallyData);

        for(Int i = 0; i < tcd.catchTable.size(); i++) {
            catchTable.add(new catch_data(*tcd.catchTable.get(i)));
        }
    }

    ~try_catch_data()
    {
        free();
    }

    void free();

    Int id;
    Int tryStartPc, tryEndPc;
    Int blockStartPc, blockEndPc;
    List<catch_data*> catchTable;
    finally_data *finallyData;
};

struct code_info {
    code_info()
            :
            address(-1),
            uuid(-1),
            stackSize(-1),
            code(),
            lineTable()
    {}

    code_info(const code_info &item)
            :
            address(-1),
            uuid(-1),
            stackSize(-1),
            code(),
            lineTable()
    {
        copy(item);
    }

    ~code_info() {
        free();
    }

    void free();

    void copy(const code_info &ci);

    int stackSize;
    List<line_info*> lineTable;
    List<try_catch_data*> tryCatchTable;
    List<opcode_instr> code;
    int address;
    Int uuid;
};

string code_to_string(code_info *info);

#endif //SHARP_CODE_INFO_H
