//
// Created by BNunnally on 9/9/2021.
//

#ifndef SHARP_CONTEXT_H
#define SHARP_CONTEXT_H

#include "../../List.h"
#include "../dependency/component.h"

struct sharp_class;
struct sharp_function;
struct sharp_field;
struct sharp_file;

enum context_type {
    no_context,
    global_context,
    class_context,
    block_context,
    component_context
};

enum block_type {
    invalid_block,
    normal_block,
    try_block,
    catch_block,
    finally_block,
    if_block,
    elseif_block,
    else_block,
    lock_block
};

/**
 * @struct stored_block_info
 * This struct represents a way for u to store block-specific info when compiling functions
 * we want to separate block info for the overall context because @struct context uses a "big picture"
 * method for telling the compiler where we are in the code. I.e. inside a class, global scope,
 * instance/static method, etc.
 *
 * This structure will provide us with finer details about a specific block that can be used by lower blocks in the function.
 * When traversing back up via restoring old block info structs, carry over field such as:
 *    - @field reachable
 * will carry over to the previous block when restored
 * to keep
 */
struct stored_block_info {
    stored_block_info()
            :
            type(invalid_block),
            reachable(true),
            id(-1),
            line(-1),
            lockScheme(NULL)
    {}

    stored_block_info(const stored_block_info &item)
            :
            type(invalid_block),
            reachable(true),
            id(-1),
            line(-1),
            lockScheme(NULL)
    {
        copy(item);
    }

    ~stored_block_info() {
        free();
    }

    void copy(const stored_block_info &item);

    void free();

    block_type type;
    Int id;
    Int line;
    bool reachable;
    operation_scheme *lockScheme;
};

struct block_info : public stored_block_info {
    block_info()
            :
            stored_block_info(),
            storedItems()
    {}

    void copy_all(const block_info &info);

    List<stored_block_info*> storedItems;
};

struct stored_context_item {
    stored_context_item()
    :
        type(no_context),
        classCxt(NULL),
        functionCxt(NULL),
        componentCtx(NULL),
        localFields(),
        isStatic(false)
    {}

    stored_context_item(const stored_context_item &item)
    :
            type(no_context),
            classCxt(NULL),
            functionCxt(NULL),
            componentCtx(NULL),
            localFields(),
            isStatic(false),
            blockInfo()
    {
        copy(item);
    }

    ~stored_context_item() {
        localFields.free();
    }

    void copy(const stored_context_item &item) {
        type = item.type;
        classCxt = item.classCxt;
        functionCxt = item.functionCxt;
        componentCtx = item.componentCtx;
        localFields.addAll(item.localFields);
        isStatic = item.isStatic;
        blockInfo.copy_all(item.blockInfo);
    }

    context_type type;
    sharp_class *classCxt;
    sharp_function *functionCxt;
    component *componentCtx;
    List<sharp_field*> localFields;
    block_info blockInfo;
    bool isStatic;
};

struct context : public stored_context_item {
    context()
    :
        stored_context_item(),
        storedItems()
    {}

    List<stored_context_item*> storedItems;
};

void create_context(sharp_class*, bool isStatic = false);
void create_context(sharp_function*);
void create_context(component*);

void create_context(context *ctx, sharp_class*, bool isStatic);
void create_context(context *ctx, sharp_function*, bool isStatic);
void create_context(context *ctx, component*);
void create_block(context *ctx, block_type type);

void delete_context();
void delete_context(context *ctx);
void store_context(context *ctx);
void restore_context(context *ctx);

void delete_block();
void delete_block(block_info *info);
void store_block(block_info *info);
void restore_block(block_info *info);
bool inside_block(block_info *info, block_type type);
void retrieve_lock_schemes(block_info *info, List<operation_scheme*> schemes);

sharp_class *get_primary_class(context*);
sharp_function *get_primary_function(context*);
component *get_primary_component(context*);

#endif //SHARP_CONTEXT_H
