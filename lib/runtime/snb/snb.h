//
// Created by BNunnally on 6/27/2020.
//

#ifndef SHARP_SNB_H
#define SHARP_SNB_H

#include "../../../stdimports.h"
#include "util.h"

#pragma once

// Sharp native bridge Api for 2 way communication back to the executable from shared lib
#ifdef __cplusplus
extern "C" {
#endif

    EXPORTED void inc_ref(void *object);
    EXPORTED void def_ref(void *object);
    EXPORTED double* getfpNumAt(int32_t fpOffset);
    EXPORTED object getField(object obj, const string& name);
    EXPORTED double* getVarPtr(object obj);
    EXPORTED object getfpLocalAt(int32_t fpOffset);
    EXPORTED int32_t getSize(object obj);
    EXPORTED int32_t setObject(object dest, object src);
    EXPORTED object staticClassInstance(const string& name);
    EXPORTED void incSp();
    EXPORTED double* getspNumAt(int32_t);
    EXPORTED object getspObjAt(int32_t);
    EXPORTED object newVarArray(int32_t);
    EXPORTED object newClass(const string &name);
    EXPORTED object newObjArray(int32_t);
    EXPORTED object newClassArray(const string& name, int32_t);
    EXPORTED void decSp(int32_t);
    EXPORTED void pushNum(double);
    EXPORTED void pushObj(object);
    EXPORTED void call(int32_t);


#ifdef __cplusplus
}
#endif


#endif //SHARP_SNB_H
