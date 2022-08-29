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
struct sharp_label;
struct sharp_alias;

enum context_type {
    no_context,
    global_context,
    class_context,
    field_context,
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
    lock_block,
    for_block,
    for_each_block,
    while_block,
    do_while_block,
    when_block
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
            lockExpression(NULL),
            beginLabel(NULL),
            endLabel(NULL),
            finallyLabel(NULL)
    {}

    stored_block_info(const stored_block_info &item)
            :
            type(invalid_block),
            reachable(true),
            id(-1),
            line(-1),
            lockExpression(NULL),
            beginLabel(NULL),
            finallyLabel(NULL),
            endLabel(NULL)
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
    sharp_label *beginLabel;
    sharp_label *endLabel;
    sharp_label *finallyLabel;
    Ast *lockExpression;
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
        fieldCxt(NULL),
        localFields(),
        localAliases(),
        labels(),
        isStatic(false)
    {}

    stored_context_item(const stored_context_item &item)
    :
            type(no_context),
            classCxt(NULL),
            functionCxt(NULL),
            componentCtx(NULL),
            fieldCxt(NULL),
            localFields(),
            localAliases(),
            labels(),
            isStatic(false),
            blockInfo()
    {
        copy(item);
    }

    ~stored_context_item() {
        localFields.free();
        localAliases.free();
        labels.free();
    }

    void copy(const stored_context_item &item);

    context_type type;
    sharp_class *classCxt;
    sharp_function *functionCxt;
    sharp_field *fieldCxt;
    component *componentCtx;
    List<sharp_field*> localFields;
    List<sharp_alias*> localAliases;
    List<sharp_label*> labels;
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
void create_context(sharp_field*);
void create_context(component*);

void create_context(context *ctx, sharp_field *, bool isStatic);
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
void retrieve_lock_expressions(block_info *info, List<Ast*> &expressions);
stored_block_info* retrieve_block(block_info *info, block_type type);
sharp_label* retrieve_next_finally_label(block_info *info);

sharp_class *get_primary_class(context*);
sharp_function *get_primary_function(context*);
sharp_field *get_primary_field(context*);
component *get_primary_component(context*);

#endif //SHARP_CONTEXT_H
