//
// Created by BNunnally on 6/28/2020.
//

#ifndef SHARP_SNB_API_H
#define SHARP_SNB_API_H

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cstdio>
#include <cstdint>
#include <string>
#include <cstring>

#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define WIN32_
#else
#define POSIX_
#endif


#ifdef WIN32_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef __wtypes_h__
#include <wtypes.h>
#endif

#ifndef __WINDEF_
#include <windef.h>
#endif
#endif
#ifdef POSIX_
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#endif


using namespace std;

typedef void* object;
typedef object vararray;

#define CONCAT2(a, b) a ## b

// global scope
#define scope_2(module, code) \
    namespace CONCAT2(module, _global) { code }

// class scope
#define scope_3(module, name, code) \
    namespace CONCAT2(module, _ ## name) { code }

#define import_1(module) \
    using namespace CONCAT2(module, _global)

#define import_2(module, klass) \
    using namespace CONCAT2(module, _ ## klass)

#define import_3(a, b, c) static_assert(false, "Too many arguments provided, try import(std) or import(std, string) or import(std, someClass_subClass)");
#define import_4(a, b, c, d) static_assert(false, "Too many arguments provided, try import(std) or import(std, string) or import(std, someClass_subClass)");
#define import_5(a, b, c, d, e) static_assert(false, "Too many arguments provided, try import(std) or import(std, string) or import(std, someClass_subClass)");

#define GET_MACRO(_1,_2, _3, _4, _5, NAME,...) NAME
#define import(...) GET_MACRO(__VA_ARGS__, import_5, import_4, import_3, import_2, import_1)(__VA_ARGS__)

#define scope_4(a, b, c, d) static_assert(false, "Too many arguments provided, try scope(std, { .. }) or scope(std, string, { .. }) or scope(std, someClass_subClass, { .. })");
#define scope_5(a, b, c, d, e) static_assert(false, "Too many arguments provided, try scope(std, { .. }) or scope(std, string, { .. }) or scope(std, someClass_subClass, { .. })");
#define scope_1(a) static_assert(false, "Not enough arguments provided, try scope(std, { .. }) or scope(std, string, { .. }) or scope(std, someClass_subClass, { .. })");

#define scope(...) GET_MACRO(__VA_ARGS__, scope_5, scope_4, scope_3, scope_2, scope_1)(__VA_ARGS__)

#if defined _WIN32 || defined __CYGWIN__
    #define EXPORTED __attribute__ ((dllexport))
    #define NOT_EXPORTED
#elif __GNUC__ >= 4
    #define EXPORTED __attribute__ ((visibility ("default")))
    #define NOT_EXPORTED  __attribute__ ((visibility ("hidden")))
#else
    //  do nothing and hope for the best?
    #define EXPORTED
    #define NOT_EXPORTED
#endif

// standardized exported bridge functions
#ifdef __cplusplus
extern "C" {
#endif

EXPORTED short snb_link_proc(const char*, int32_t);
EXPORTED void snb_main(long);
EXPORTED short snb_handshake(void* lib_funcs[], int);

#ifdef __cplusplus
}
#endif


// exported virtual machine methods
typedef void (*_inc_ref)(void *object);
typedef void (*_dec_ref)(void *object);
typedef double* (*_getfpNumAt)(int32_t fpOffset);
typedef object (*_getField)(object obj, const string &name);
typedef double* (*_getVarPtr)(object obj);
typedef object (*_getfpLocalAt)(int32_t fpOffset);
typedef int32_t (*_getSize)(object obj);
typedef int (*_setObject)(object dest, object src);
typedef object (*_staticClassInstance)(const string &name);
typedef void (*_inc_sp)();
typedef double* (*_getspNumAt)(int32_t spOffset);
typedef object (*_getspObjAt)(int32_t spOffset);
typedef object (*_newVarArray)(int32_t);
typedef object (*_newClass)(const string &name);
typedef object (*_newObjArray)(int32_t);
typedef object (*_newClassArray)(const string& name, int32_t);
typedef void (*_decSp)(int32_t);
typedef void (*_pushNum)(double);
typedef void (*_pushObj)(object);
typedef void (*_call)(int32_t);


#define math_fun(op) \
    double operator op(double val) { if(num) return num[0] op val; else return 0; } \
    double operator op(var &val) { if(num) return num[0] op val.num[0]; else return 0; }

#define math_fun_cast(op, cast) \
    double operator op(double val) { if(num) return ((cast) num[0]) op (cast)val; else return 0; } \
    double operator op(var &val) { if(num) return ((cast) num[0]) op ((cast) val.num[0]); else return 0; }

#define math_fun_assign_cast(op, cast) \
    double operator op(double val) { if(num) return num[0] op (cast)val; else return 0; } \
    double operator op(var &val) { if(num) return num[0] op ((cast) val.num[0]); else return 0; }

namespace snb_api {

    namespace internal {
        extern _inc_ref inc_ref;
        extern _dec_ref dec_ref;
        extern _getfpNumAt getfpNumAt;
        extern _getField getField;
        extern _getVarPtr getVarPtr;
        extern _getfpLocalAt getfpLocalAt;
        extern _getSize getSize;
        extern _setObject setObject;
        extern _staticClassInstance staticClassInstance;
        extern _inc_sp inc_sp;
        extern _getspNumAt getSpNumAt;
        extern _getspObjAt getSpObjAt;
        extern _newVarArray newVarArray;
        extern _newClass newClass;
        extern _newObjArray newObjArray;
        extern _newClassArray newClassArray;
        extern _decSp decSp;
        extern _pushNum pushNum;
        extern _pushObj pushObj;
        extern _call call;
    }

    class var_array;

    typedef var_array _int8_array;
    typedef var_array _int16_array;
    typedef var_array _int32_array;
    typedef var_array _int64_array;
    typedef var_array _uint8_array;
    typedef var_array _uint16_array;
    typedef var_array _uint32_array;
    typedef var_array _uint64_array;

    string stringFrom(const var_array&);

    template <class T>
    T get(object obj, string field);

    template <class T>
    T cast_to(object obj);

    int set(object obj, var_array &arry);
    int set(object dest, object src);
    void set(var_array &arry, const char *);
    void set(var_array &arry, string&);

    void incRef(object obj);
    void decRef(object obj);

    object getStaticClassInstance(const string& name);

    template <class T>
    T createLocalField();

    void createVarArray(var_array &field, int32_t size);
    void createClassArray(object field, const string &name, int32_t size);
    void createClass(object field, const string &name);
    void createObjectArray(object field, int32_t size);
    void unTrack(int32_t amount);

    template<class T>
    class integer;

    class var {
    public:
        double value() {
            if (num)
                return *num;
            else return 0;
        }

        var(double *ref)
                :
                num(ref),
                handle(NULL) {
        }

        var(double *ref, object handle)
                :
                num(ref),
                handle(handle) {
            internal::inc_ref(handle);
        }

        var(const var &val)
                :
                num(val.num),
                handle(val.handle) {
            internal::inc_ref(handle);
        }

        virtual ~var() {
            internal::dec_ref(handle);
        }

        double operator=(double val) {
            if (num)
                *num = val;
            return val;
        }

        double operator=(const var &val) {
            if(num && val.num)
                *num = *val.num;
            return num ? num[0] : 0;
        }

        template<class T>
        double operator=(const integer<T> &val) {
            if(num)
                *num = val.value();
            return num ? num[0] : 0;
        }

        double operator++() {
            if (num)
                return num[0]++;
            else return 0;
        }

        double operator++(int i) {
            if (num)
                return ++num[0];
            else return 0;
        }

        double operator--() {
            if (num)
                return num[0]++;
            else return 0;
        }

        double operator--(int i) {
            if (num)
                return ++num[0];
            else return 0;
        }

        math_fun(+)

        math_fun(-)

        math_fun(*)

        math_fun(/)

        math_fun(+=)

        math_fun(-=)

        math_fun(/=)

        math_fun(*=)

        math_fun_cast(<<, int64_t)

        math_fun_cast(>>, int64_t)

        object handle;
        double *num;
    };

    template<class T>
    class integer {
    public:
        integer(double *ref)
                :
                num(ref),
                handle(NULL) {
        }

        integer(double *ref, object handle)
                :
                num(ref),
                handle(handle) {
            internal::inc_ref(handle);
        }

        integer(const var &val)
                :
                num(val.num),
                handle(val.handle) {
            internal::inc_ref(handle);
        }

        virtual ~integer() {
            internal::dec_ref(handle);
        }

        double operator++() {
            if (num)
                return num[0]++;
            else return 0;
        }

        double operator++(int i) {
            if (num)
                return ++num[0];
            else return 0;
        }

        double operator--() {
            if (num)
                return num[0]++;
            else return 0;
        }

        double operator--(int i) {
            if (num)
                return ++num[0];
            else return 0;
        }

        math_fun(+)

        math_fun(-)

        math_fun(*)

        math_fun(/)

        math_fun_assign_cast(+=, T)

        math_fun_assign_cast(-=, T)

        math_fun_assign_cast(/=, T)

        math_fun_assign_cast(*=, T)

        math_fun_cast(<<, int64_t)

        math_fun_cast(>>, int64_t)


        double operator=(double val) {
            if (num)
                *num = (int8_t) val;
            return val;
        }

        double operator=(const var &val) {
            if(num && val.num)
                *num = *val.num;
            return num ? num[0] : 0;
        }

        double operator=(const integer<T> &val) {
            if(num)
                *num = val.value();
            return num ? num[0] : 0;
        }

        double value() {
            if (num)
                return *num;
            else return 0;
        }

        object handle;
        double *num;
    protected:

        integer() {
        }
    };

    class _int8 : public integer<int8_t> {
    public:

        _int8(double *ref)
                :
                integer(ref) {
        }

        _int8(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        _int8(const var &val)
                :
                integer(val) {
        }

        template<class T>
        _int8(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_int8() {}

        double operator=(double val) {
            return this->integer<int8_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<int8_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<int8_t>::operator=(val);
        }
    };

    class _int16 : public integer<int16_t> {
    public:

        _int16(double *ref)
                :
                integer(ref) {
        }

        _int16(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        _int16(const var &val)
                :
                integer(val) {
        }

        template<class T>
        _int16(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_int16() {}

        double operator=(double val) {
            return this->integer<int16_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<int16_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<int16_t>::operator=(val);
        }
    };

    class _int32 : public integer<int32_t> {
    public:

        _int32(double *ref)
                :
                integer(ref) {
        }

        _int32(const var &val)
                :
                integer(val) {
        }

        _int32(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        template<class T>
        _int32(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_int32() {}

        double operator=(double val) {
            return this->integer<int32_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<int32_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<int32_t>::operator=(val);
        }
    };

    class _int64 : public integer<int64_t> {
    public:

        _int64(double *ref)
                :
                integer(ref) {
        }

        _int64(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        _int64(const var &val)
                :
                integer(val) {
        }

        template<class T>
        _int64(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_int64() {}

        double operator=(double val) {
            return this->integer<int64_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<int64_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<int64_t>::operator=(val);
        }
    };

    class _uint8 : public integer<uint8_t> {
    public:

        _uint8(double *ref)
                :
                integer(ref) {
        }

        _uint8(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        _uint8(const var &val)
                :
                integer(val) {
        }

        template<class T>
        _uint8(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_uint8() {}

        double operator=(double val) {
            return this->integer<uint8_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<uint8_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<uint8_t>::operator=(val);
        }
    };

    class _uint16 : public integer<uint16_t> {
    public:

        _uint16(double *ref)
                :
                integer(ref) {
        }

        _uint16(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        _uint16(const var &val)
                :
                integer(val) {
        }

        template<class T>
        _uint16(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_uint16() {}

        double operator=(double val) {
            return this->integer<uint16_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<uint16_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<uint16_t>::operator=(val);
        }
    };

    class _uint32 : public integer<uint32_t> {
    public:

        _uint32(double *ref)
                :
                integer(ref) {
        }

        _uint32(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        _uint32(const var &val)
                :
                integer(val) {
        }

        template<class T>
        _uint32(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_uint32() {}

        double operator=(double val) {
            return this->integer<uint32_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<uint32_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<uint32_t>::operator=(val);
        }
    };

    class _uint64 : public integer<uint64_t> {
    public:

        _uint64(double *ref)
                :
                integer(ref) {
        }

        _uint64(double *ref, object handle)
                :
                integer(ref, handle) {
        }

        _uint64(const var &val)
                :
                integer(val) {
        }

        template<class T>
        _uint64(const integer<T> &val)
                :
                integer(val) {
        }

        virtual ~_uint64() {}

        double operator=(double val) {
            return this->integer<uint64_t>::operator=(val);
        }

        double operator=(const var &val) {
            return this->integer<uint64_t>::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->integer<uint64_t>::operator=(val);
        }
    };

    class var_array : public var {
    public:

        var_array(double *ref, int32_t size)
                :
                var(ref),
                size(size) {
        }

        var_array(double *ref, int32_t size, object handle)
                :
                var(ref, handle),
                size(size) {
        }

        var_array(const var &val)
                :
                var(val),
                size(val.num ? 1 : 0) {
        }

        template<class T>
        var_array(const integer<T> &val)
                :
                var(val),
                size(val.num ? 1 : 0) {
        }

        virtual ~var_array() {}

        double &operator[](int32_t index) {
            if (num && index < size)
                return num[index];
            else {
                stringstream ss;
                ss << "index out of bounds array size: " << size
                   << " accessed at index: " << index;
                throw runtime_error(ss.str());
            }
        }


        double at(int32_t index) const {
            if (num && index < size)
                return num[index];
            else return 0;
        }

        double operator=(double val) {
            return this->var::operator=(val);
        }

        double operator=(const var &val) {
            return this->var::operator=(val);
        }

        template<class T>
        double operator=(const integer<T> &val) {
            return this->var::operator=(val);
        }

        bool operator==(const char* str) {
            long len = strlen(str);
            if(str && handle) {
                if(size == len) {
                    for(long i = 0; i < size; i++) {
                        if(num[i] != str[i])
                            return false;
                    }
                } else return false;

                return true;
            }
        }

        bool operator!=(const char* str) {
            return !operator==(str);
        }

        bool operator==(var_array &array) {
            if(handle && array.handle) {
                if(array.size == size) {
                    for(long i = 0; i < size; i++) {
                        if(num[i] != array.num[i])
                            return false;
                    }
                } else return false;

                return true;
            }
        }

        bool operator!=(var_array &array) {
            return !operator==(array);
        }

        bool operator==(string &str) {
            return operator==(str.c_str());
        }

        bool operator!=(string &str) {
            return !operator==(str);
        }

        void operator=(const char*val) {
            set(*this, val);
        }

        void operator=(string &val) {
            set(*this, val);
        }

        void operator=(var_array &val) {
            if(handle) {
                set(handle, val);
                num = val.num;
            }
        }

        int32_t size;
    };
};

#endif //SHARP_SNB_API_H
