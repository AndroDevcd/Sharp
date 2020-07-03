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

        int on_load() {
            inc_ref = (_inc_ref)load_func("inc_ref");
            dec_ref = (_dec_ref)load_func("def_ref");
            getfpNumAt = (_getfpNumAt)load_func("getfpNumAt");
            getField = (_getField)load_func("getField");
            getVarPtr = (_getVarPtr)load_func("getVarPtr");
            getfpLocalAt = (_getfpLocalAt)load_func("getfpLocalAt");
            getSize = (_getSize)load_func("getSize");
            setObject = (_setObject)load_func("setObject");
            staticClassInstance = (_staticClassInstance)load_func("staticClassInstance");
            inc_sp = (_inc_sp)load_func("incSp");
            getSpNumAt = (_getspNumAt)load_func("getspNumAt");
            getSpObjAt = (_getspObjAt)load_func("getspObjAt");
            newVarArray = (_newVarArray)load_func("newVarArray");
            newClass = (_newClass)load_func("newClass");
            newObjArray = (_newObjArray)load_func("newObjArray");
            newClassArray = (_newClassArray)load_func("newClassArray");
            decSp = (_decSp)load_func("decSp");
            pushNum = (_pushNum)load_func("pushNum");
            pushObj = (_pushObj)load_func("pushObj");
            call = (_call)load_func("call");

            return inc_ref && dec_ref && getfpNumAt && getField
                   && getVarPtr && getfpLocalAt && getSize && setObject
                   && staticClassInstance && inc_sp && getSpNumAt
                   && getSpObjAt && decSp && pushNum && pushObj
                   && call && newVarArray && newClass && newObjArray
                   && newClassArray;
        }
    }

    using namespace internal;

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
        object fieldObj = getField(obj, field);

        if(fieldObj) {
            T val(getVarPtr(fieldObj), fieldObj);
            return val;
        } else {
            T val(NULL);
            return val;
        }
    }

    template<> var_array get<var_array>(object obj, string field) {
        object fieldObj = getField(obj, field);

        if(fieldObj) {
            var_array val(getVarPtr(fieldObj), getSize(fieldObj), fieldObj);
            return val;
        } else {
            var_array val(NULL);
            return val;
        }
    }

    template<> object get<object>(object obj, string field) {
        return getField(obj, field);
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
        return staticClassInstance(name);
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
        object newObj = newVarArray(size);
        field.num = getVarPtr(newObj);
        field.size = size;
        set(field.handle, newObj);
    }

    void createClassArray(object field, const string &name, int32_t size) {
        set(field, newClassArray(name, size));
    }

    void createClass(object field, const string &name) {
        set(field, newClass(name));
    }

    void createObjectArray(object field, int32_t size) {
        set(field, newObjArray(size));
    }

    void unTrack(int32_t amount) {
        decSp(amount);
    }

    void set(var_array &arry, const char *str) {
        long len = strlen(str);
        if(str && arry.handle) {
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

#pragma optimize( "", off )
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
#pragma optimize( "", on )

#ifdef __cplusplus
extern "C" {
#endif

EXPORTED short snb_load_lib() {
    return snb_api::internal::on_load();
}

#ifdef __cplusplus
}
#endif
