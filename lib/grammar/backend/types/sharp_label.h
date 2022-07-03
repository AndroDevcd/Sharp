//
// Created by bknun on 6/4/2022.
//

#ifndef SHARP_SHARP_LABEL_H
#define SHARP_SHARP_LABEL_H

#include "../../../../stdimports.h"
#include "../meta_data.h"

struct context;
struct operation_schema;

struct sharp_label {
    sharp_label()
            :
            name(""),
            id(-1),
            location()
    {}

    sharp_label(
            string name,
            Int id,
            impl_location location)
    :
            name(name),
            id(id),
            location(location)
    {
    }

    sharp_label(const sharp_label &label)
            :
            name(label.name),
            id(label.id),
            location(label.location)
    {}

    ~sharp_label()
    {
        free();
    }

    void free() {
    }

    string name;
    Int id;
    impl_location location;
};

sharp_label* create_label(
        string name,
        context *context,
        Ast *createLocation,
        operation_schema *scheme);

#endif //SHARP_SHARP_LABEL_H
