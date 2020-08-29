#include "snb_api.h"

namespace snb_api {

    // private fields
    namespace internal {
        _inc_ref inc_ref;
        _dec_ref dec_ref;
        _getfpNumAt getfpNumAt;
        _getField getField;
        _getVarPtr getVarPtr;
        _getfpLocalAt getfpLocalAt;
        _getSize getSize;
        _setObject setObject;
        _staticClassInstance staticClassInstance;
        _inc_sp inc_sp;
        _getspNumAt getSpNumAt;
        _getspObjAt getSpObjAt;
        _newVarArray newVarArray;
        _newClass newClass;
        _newObjArray newObjArray;
        _newClassArray newClassArray;
        _decSp decSp;
        _pushNum pushNum;
        _pushObj pushObj;
        _call call;
        _exceptionCheck exceptionCheck;
        _getExceptionObject getExceptionObject;
        _className className;
        _prepareException prepareException;
        _clearException clearExcept;
        _getItem getDataItem;

        int handshake(void *lib_funcs[], int size) {

            if (size == 26) {
                inc_ref = (_inc_ref) lib_funcs[0];
                dec_ref = (_dec_ref) lib_funcs[1];
                getfpNumAt = (_getfpNumAt) lib_funcs[2];
                getField = (_getField) lib_funcs[3];
                getVarPtr = (_getVarPtr) lib_funcs[4];
                getfpLocalAt = (_getfpLocalAt) lib_funcs[5];
                getSize = (_getSize) lib_funcs[6];
                setObject = (_setObject) lib_funcs[7];
                staticClassInstance = (_staticClassInstance) lib_funcs[8];
                inc_sp = (_inc_sp) lib_funcs[9];
                getSpNumAt = (_getspNumAt) lib_funcs[10];
                getSpObjAt = (_getspObjAt) lib_funcs[11];
                newVarArray = (_newVarArray) lib_funcs[12];
                newClass = (_newClass) lib_funcs[13];
                newObjArray = (_newObjArray) lib_funcs[14];
                newClassArray = (_newClassArray) lib_funcs[15];
                decSp = (_decSp) lib_funcs[16];
                pushNum = (_pushNum) lib_funcs[17];
                pushObj = (_pushObj) lib_funcs[18];
                call = (_call) lib_funcs[19];
                exceptionCheck = (_exceptionCheck) lib_funcs[20];
                getExceptionObject = (_getExceptionObject) lib_funcs[21];
                className = (_className) lib_funcs[22];
                prepareException = (_prepareException) lib_funcs[23];
                clearExcept = (_clearException) lib_funcs[24];
                getDataItem = (_getItem) lib_funcs[25];

                return inc_ref && dec_ref && getfpNumAt && getField
                       && getVarPtr && getfpLocalAt && getSize && setObject
                       && staticClassInstance && inc_sp && getSpNumAt
                       && getSpObjAt && decSp && pushNum && pushObj
                       && call && newVarArray && newClass && newObjArray
                       && newClassArray && exceptionCheck && getExceptionObject
                       && className && prepareException && clearExcept
                       && getDataItem;
            } else return false;

        }

    }

    using namespace internal;

    object getItem(object obj, int32_t index) {
        return getDataItem(obj, index);
    }

    void clearException() {
        clearExcept();
    }

    const char* getClassName(object klazz) {
        return className(klazz);
    }

    void throwException(object exceptionClass) {
        throw Exception(exceptionClass, "");
    }

    template<class T> T cast_to(object obj) {
        if(obj) {
            T val(getVarPtr(obj), obj);
            return val;
        } else {
            T val(NULL);
            return val;
        }
    }

    template<class T> T get(object obj, string field) {
        object fieldObj = getField(obj, field.c_str());

        if(fieldObj) {
            T val(getVarPtr(fieldObj), fieldObj);
            return val;
        } else {
            T val(NULL);
            return val;
        }
    }

    template<> var_array get<var_array>(object obj, string field) {
        object fieldObj = getField(obj, field.c_str());

        if(fieldObj) {
            var_array val(getVarPtr(fieldObj), getSize(fieldObj), fieldObj);
            return val;
        } else {
            var_array val(NULL);
            return val;
        }
    }

    template<> object get<object>(object obj, string field) {
        return getField(obj, field.c_str());
    }


    template<> var_array cast_to<var_array>(object obj) {
        if(obj) {
            var_array val(getVarPtr(obj), getSize(obj), obj);
            return val;
        } else {
            var_array val(NULL);
            return val;
        }
    }

    int set(object dest, var_array &arry) {
        return set(dest, arry.handle);
    }

    int set(object dest, object src) {
        return setObject(dest, src);
    }

    void incRef(object obj){
        inc_ref(obj);
    }

    void decRef(object obj){
        dec_ref(obj);
    }

    object getStaticClassInstance(const string& name) {
        return staticClassInstance(name.c_str());
    }

    string stringFrom(const var_array &arry) {
        stringstream ss;
        for(int32_t i = 0; i < arry.size; i++) {
            ss << (char)arry.at(i);
        }
        return ss.str();
    }

    template <class T>
    T createLocalField() {
        inc_sp();

        T val(getSpNumAt(0));
        val = 0;
        return val;
    }

    template<> object createLocalField<object>() {
        inc_sp();
        object field = getSpObjAt(0);
        set(field, NULL);

        return field;
    }

    template<> var_array createLocalField<var_array>() {
        inc_sp();
        object handle = getSpObjAt(0);
        set(handle, NULL);
        var_array field(NULL, 0, handle);

        return field;
    }

    void createVarArray(var_array &field, int32_t size) {
        object newObj = newVarArray(size, VAR_TYPE);

        if(exceptionCheck()) {
            throw Exception(getExceptionObject(), "");
        }

        field.num = getVarPtr(newObj);
        field.size = size;
        set(field.handle, newObj);
    }

    void createClassArray(object field, const string &name, int32_t size) {
        set(field, newClassArray(name.c_str(), size));

        if(exceptionCheck()) {
            throw Exception(getExceptionObject(), "");
        }
    }

    void createClass(object field, const string &name) {
        set(field, newClass(name.c_str()));

        if(exceptionCheck()) {
            throw Exception(getExceptionObject(), "");
        }
    }

    void createObjectArray(object field, int32_t size) {
        set(field, newObjArray(size));

        if(exceptionCheck()) {
            throw Exception(getExceptionObject(), "");
        }
    }

    void unTrack(int32_t amount) {
        decSp(amount);

        if(exceptionCheck()) {
            throw Exception(getExceptionObject(), "");
        }
    }

    void set(var_array &arry, const char *str) {
        if(arry.handle) {
            if(str == NULL) {
                createVarArray(arry, 0);
                return;
            }

            long len = strlen(str);
            if(arry.size != len)
                createVarArray(arry, len);

            for(long i = 0; i < len; i++) {
                arry[i] = str[i];
            }
        }
    }

    void set(var_array &arry, string& str) {
        set(arry, str.c_str());
    }
};

#pragma GCC push_options
#pragma GCC optimize ("O0")
/*
 * This allow the compiler to properly link each template function
 */
void dead_func() {
    using namespace snb_api;
    createLocalField<var>();
    createLocalField<_int8>();
    createLocalField<_int16>();
    createLocalField<_int16>();
    createLocalField<_int32>();
    createLocalField<_int64>();
    createLocalField<_uint8>();
    createLocalField<_uint16>();
    createLocalField<_uint32>();
    createLocalField<_uint64>();
    createLocalField<var_array>();

    object obj;
    cast_to<var>(obj);
    cast_to<_int8>(obj);
    cast_to<_int16>(obj);
    cast_to<_int16>(obj);
    cast_to<_int32>(obj);
    cast_to<_int64>(obj);
    cast_to<_uint8>(obj);
    cast_to<_uint16>(obj);
    cast_to<_uint32>(obj);
    cast_to<_uint64>(obj);
    cast_to<var_array>(obj);


    get<var>(obj, "");
    get<_int8>(obj, "");
    get<_int16>(obj, "");
    get<_int16>(obj, "");
    get<_int32>(obj, "");
    get<_int64>(obj, "");
    get<_uint8>(obj, "");
    get<_uint16>(obj, "");
    get<_uint32>(obj, "");
    get<_uint64>(obj, "");
    get<var_array>(obj, "");
}
#pragma pop_options

#ifdef __cplusplus
extern "C" {
#endif

EXPORTED short snb_handshake(void* lib_funcs[], int size) {
    return snb_api::internal::handshake(lib_funcs, size);
}

#ifdef __cplusplus
}
#endif
