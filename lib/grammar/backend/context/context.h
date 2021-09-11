//
// Created by BNunnally on 9/9/2021.
//

#ifndef SHARP_CONTEXT_H
#define SHARP_CONTEXT_H

#include "../../List.h"

struct sharp_class;
struct sharp_function;
struct sharp_field;
struct sharp_file;

enum context_type {
    no_context,
    global_context,
    class_context,
    block_context
};

struct stored_context_item {
    stored_context_item()
    :
        type(no_context),
        classCxt(NULL),
        functionCxt(NULL),
        localFields(),
        isStatic(false)
    {}

    stored_context_item(const stored_context_item &item)
    :
            type(no_context),
            classCxt(NULL),
            functionCxt(NULL),
            localFields(),
            isStatic(false)
    {
        copy(item);
    }

    ~stored_context_item() {
        free();
    }

    void copy(const stored_context_item &item) {
        type = item.type;
        classCxt = item.classCxt;
        functionCxt = item.functionCxt;
        localFields.addAll(item.localFields);
        isStatic = item.isStatic;
    }

    void free() {
        localFields.free();
    }

    context_type type;
    sharp_class *classCxt;
    sharp_function *functionCxt;
    List<sharp_field*> localFields;
    bool isStatic;
};

struct context : public stored_context_item {
    context()
    :
        stored_context_item(),
        storedItems()
    {}

    List<stored_context_item> storedItems;
};

void create_context(sharp_class*, bool isStatic = false);
void create_context(sharp_function*, bool isStatic = false);

void create_context(context *ctx, sharp_class*, bool isStatic);
void create_context(context *ctx, sharp_function*, bool isStatic);

void delete_context();
void delete_context(context *ctx);
void store_context(context *ctx);
void restore_context(context *ctx);

sharp_class *get_primary_class(context*);

#endif //SHARP_CONTEXT_H
