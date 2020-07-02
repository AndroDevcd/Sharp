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
            decSp = (_decSp)load_func("decSp");
            pushNum = (_pushNum)load_func("pushNum");
            pushObj = (_pushObj)load_func("pushObj");
            call = (_call)load_func("call");

            return inc_ref && dec_ref && getfpNumAt && getField
                   && getVarPtr && getfpLocalAt && getSize && setObject
                   && staticClassInstance && inc_sp && getSpNumAt
                   && getSpObjAt && decSp && pushNum && pushObj
                   && call;
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

    int set(object dest, var_array arry) {
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
        var_array arry(getVarPtr(newObj), size, newObj);
        field = arry;
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
};


#ifdef __cplusplus
extern "C" {
#endif

EXPORTED short snb_load_lib() {
    return snb_api::internal::on_load();
}

#ifdef __cplusplus
}
#endif
