//
// Created by bknun on 9/19/2022.
//

#ifndef SHARP_TRY_CATCH_DATA_H
#define SHARP_TRY_CATCH_DATA_H

#include "../../../stdimports.h"

struct sharp_class;

struct catch_data {
    catch_data()
            :
            handlerPc(-1),
            id(-1),
            exceptionFieldAddress(-1),
            exception(nullptr)
    {
    }

    catch_data(Int id)
            :
            handlerPc(-1),
            id(id),
            exceptionFieldAddress(-1),
            exception(nullptr)
    {
    }

    catch_data(const catch_data &cd)
            :
            handlerPc(cd.handlerPc),
            id(cd.id),
            exceptionFieldAddress(cd.exceptionFieldAddress),
            exception(cd.exception)
    {
    }

    ~catch_data()
    {
    }

    int id;
    Int handlerPc;
    Int exceptionFieldAddress;
    sharp_class* exception;
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

        auto item = tcd.catchTable.begin();
        for(Int i = 0; i < tcd.catchTable.size(); i++) {
            catchTable.emplace_back(catch_data(*item));
            item++;
        }
    }

    Int id;
    Int tryStartPc, tryEndPc;
    Int blockStartPc, blockEndPc;
    list<catch_data> catchTable;
    finally_data *finallyData;
};

#endif //SHARP_TRY_CATCH_DATA_H
