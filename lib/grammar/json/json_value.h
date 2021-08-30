//
// Created by bnunnally on 8/29/21.
//

#ifndef SHARP_JSON_VALUE_H
#define SHARP_JSON_VALUE_H

#include "../../../stdimports.h"
#include "json_type.h"

class json_array;
class json_object;

class json_value {
public:
    json_value()
    :
        type(jtype_invalid),
        intValue(0),
        stringValue(),
        arrayValue(NULL),
        objectValue(NULL),
        boolValue(false)
    {}

    json_value(const json_value &jv);

    json_type getType() {
        return type;
    }

    ~json_value()
    {
        free();
    }

    void free();

    void copy(const json_value &jv);

    bool& getBoolValue() {
        requireType(jtype_bool);

        return boolValue;
    }

    Int& getIntValue() {
        requireType(jtype_int);

        return intValue;
    }

    string& getStringValue() {
        requireType(jtype_string);

        return stringValue;
    }

    json_array* getArrayValue() {
        requireType(jtype_array);

        return arrayValue;
    }

    json_object* getJsonObject() {
        requireType(jtype_object);

        return objectValue;
    }

    void toString(stringstream &str, uInt &tabCount);

    void setStringValue(string &value) {
        type = jtype_string;
        stringValue = value;
    }

    void setIntValue(Int value) {
        type = jtype_int;
        intValue = value;
    }

    void setArrayValue(json_array *value);

    void setBoolValue(bool value) {
        type = jtype_bool;
        boolValue = value;
    }

    void setNullValue(string &value) {
        type = jtype_null;
    }

    void setObjectValue(json_object *value);

    void setNullValue() {
        type = jtype_null;
    }

    static string typeToString(json_type type) {
        switch(type) {
            case jtype_int: return "int";
            case jtype_string: return "string";
            case jtype_array: return "array";
            case jtype_bool: return "bool";
            case jtype_null: return "null";
            case jtype_object: return "object";
            case jtype_invalid: return "invalid";
            default: return "undefined";
        }
    }
private:
    void requireType(json_type requiredType) {
        if(requiredType != type) {
            stringstream ss;
            ss << "invalid call on getValue() on json type: "
               << typeToString(type) << ", expected type: " << requiredType;
            throw runtime_error(ss.str().c_str());
        }
    }

    json_type type;
    Int intValue;
    string stringValue;
    json_array* arrayValue;
    json_object* objectValue;
    bool boolValue;
};

void addTabs(stringstream &ss, uInt tabCount);

#endif //SHARP_JSON_VALUE_H
