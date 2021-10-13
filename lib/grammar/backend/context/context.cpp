//
// Created by BNunnally on 9/9/2021.
//

#include "context.h"
#include "../access_flag.h"
#include "../types/sharp_class.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_function.h"

void create_context(sharp_class *sc, bool isStatic) {
    create_context(&currThread->currTask->file->context, sc, isStatic);
}

void create_context(sharp_function *fun) {
    create_context(&currThread->currTask->file->context, fun, check_flag(fun->flags, flag_static));
}

void create_context(component *comp) {
    create_context(&currThread->currTask->file->context, comp);
}

void create_context(context *ctx, sharp_class *sc, bool isStatic) {
    if(ctx->type != no_context) {
        store_context(ctx);
    }

    if(check_flag(sc->flags, flag_global))
        ctx->type = global_context;
    else ctx->type = class_context;

    ctx->classCxt = sc;
    ctx->isStatic = isStatic;
}

void create_context(context *ctx, sharp_function *fun, bool isStatic) {
    if(ctx->type != no_context) {
        store_context(ctx);
    }

    ctx->type = block_context;
    ctx->functionCxt = fun;
    ctx->isStatic = isStatic;
    ctx->localFields.addAll(fun->parameters);
}

void create_context(context *ctx, component *comp) {
    if(ctx->type != no_context) {
        store_context(ctx);
    }

    ctx->type = component_context;
    ctx->componentCtx = comp;
}

void delete_context() {
    delete_context(&currThread->currTask->file->context);
}

void delete_context(context *ctx) {
    if(ctx->storedItems.empty()) {
        ctx->type = no_context;
        ctx->functionCxt = NULL;
        ctx->componentCtx = NULL;
        ctx->classCxt = NULL;
        ctx->isStatic = false;
        ctx->localFields.free();
    } else {
        restore_context(ctx);
    }
}

void store_context(context *ctx) {
    ctx->storedItems.add(new stored_context_item(*ctx));
}

void restore_context(context *ctx) {
    stored_context_item item(*ctx->storedItems.last());
    free(ctx->storedItems.last());
    ctx->storedItems.pop_back();
    ctx->copy(item);
}

sharp_class *get_primary_class(context *ctx) {
    if(ctx->type == class_context || ctx->type == global_context)
        return ctx->classCxt;

    for(Int i = (Int)ctx->storedItems.size() - 1; i >= 0; i--) {
        stored_context_item *contextItem = ctx->storedItems.get(i);
        if(contextItem->type == class_context
            || contextItem->type == global_context)
            return contextItem->classCxt;
    }

    return NULL;
}

component *get_primary_component(context *ctx) {
    if(ctx->type == component_context)
        return ctx->componentCtx;

    for(Int i = (Int)ctx->storedItems.size() - 1; i >= 0; i--) {
        stored_context_item *contextItem = ctx->storedItems.get(i);
        if(contextItem->type == component_context
            || contextItem->type == global_context)
            return contextItem->componentCtx;
    }

    return NULL;
}

sharp_function *get_primary_function(context *ctx) {

    sharp_function *fun = NULL;
    for(Int i = (Int)ctx->storedItems.size() - 1; i >= 0; i--) {
        stored_context_item *contextItem = ctx->storedItems.get(i);
        if(contextItem->type == block_context)
            fun = contextItem->functionCxt;
    }

    return fun;
}
