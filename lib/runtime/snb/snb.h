//
// Created by BNunnally on 6/27/2020.
//

#ifndef SHARP_SNB_H
#define SHARP_SNB_H

#include "../../../stdimports.h"
#include "util.h"

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

#ifdef _WIN32
#define load_func(handle, name) GetProcAddress(handle, name)
#else
#define load_func(handle, name) dlsym(handle, name)
#endif

#ifdef _WIN32
#define load_lib(lib) LoadLibrary((lib.str() + ".dll").c_str())
#else
#define load_lib(lib) dlopen((lib.str() + ".so").c_str(), RTLD_LAZY)
#endif

#ifdef _WIN32
#define free_lib(handle) FreeLibrary(handle)
#else
#define free_lib(handle) dlclose(handle)
#endif


#endif //SHARP_SNB_H
