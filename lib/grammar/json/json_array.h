//
// Created by bnunnally on 8/29/21.
//

#ifndef SHARP_JSON_ARRAY_H
#define SHARP_JSON_ARRAY_H

#include "../List.h"
#include "json_value.h"

class json_array {
public:
    json_array()
    :
        values()
    {}

    json_array(const json_array &ja)
            :
            json_array()
    {
        copy(ja);
    }

    void copy(const json_array &ja) {
        free();

        for(Int i = 0; i < ja.values.size(); i++) {
            values.__new() = new json_value();
            values.last()->copy(*ja.values.get(i));
        }
    }

    ~json_array() {
        free();
    }

    void free() {
        for(Int i = 0; i < values.size(); i++) {
            values.get(i).free();
        }

        values.free();
    }

    json_value& operator[](uInt index) {
        return *values.get(index);
    }

    List<json_value*>& get_values() {
        return values;
    }

    uInt size() {
        return values.size();
    }

    void toString(stringstream &str, uInt &tabOffset) {
        str << "[" << endl;
        tabOffset++;

        for(uInt i = 0; i < size(); i++) {
            addTabs(str, tabOffset);
            values.get(i)->toString(str, tabOffset);

            if((i + 1) < size()) {
                str << ",";
            }

            str << endl;
        }

        str << "]";
        tabOffset--;
    }

    void addValue(json_value *value) {
        values.__new() = value;
    }

private:
    List<json_value*> values;
};


#endif //SHARP_JSON_ARRAY_H
