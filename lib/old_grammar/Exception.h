//
// Created by BraxtonN on 2/5/2018.
//

#ifndef SHARP_EXCECPTION_H
#define SHARP_EXCECPTION_H


#include "../../stdimports.h"
#include <stdexcept>


struct CatchData {
    CatchData()
            :
            handler_pc(invalidAddr),
            localFieldAddress(invalidAddr),
            caughtException(NULL)
    {
    }

    void init() {
        CatchData();
    }

    void free() {
    }

    void operator=(CatchData* ct) {
        this->handler_pc=ct->handler_pc;
        this->handler_pc=ct->handler_pc;
        this->caughtException=ct->caughtException;
    }

    Int handler_pc;
    Int localFieldAddress;
    ClassObject* caughtException;
};

struct FinallyData {
    FinallyData()
            :
            start_pc(0),
            end_pc(0),
            exception_object_field_address(-1)
    {
    }

    void operator=(FinallyData *fd) {
        start_pc=fd->start_pc;
        end_pc=fd->end_pc;
        exception_object_field_address=fd->exception_object_field_address;
    }

    uInt exception_object_field_address;
    uInt start_pc, end_pc;
};

struct TryCatchData {
    TryCatchData()
            :
            try_start_pc(invalidAddr),
            try_end_pc(invalidAddr),
            finallyData(NULL),
            catchTable()
    {
    }

    void init() {
        try_start_pc = 0;
        try_end_pc = 0;
        catchTable.init();
        finallyData = NULL;
    }

    void operator=(TryCatchData *data) {
        this->try_start_pc=data->try_start_pc;
        this->try_end_pc=data->try_end_pc;
        this->block_start_pc=data->block_start_pc;
        this->block_end_pc=data->block_end_pc;
        this->catchTable.addAll(data->catchTable);
        this->finallyData = data->finallyData;
    }

    Int try_start_pc, try_end_pc;
    Int block_start_pc, block_end_pc;
    _List<CatchData> catchTable;
    FinallyData *finallyData;
};

#endif //SHARP_EXCECPTION_H
