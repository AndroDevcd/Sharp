//
// Created by BNunnally on 9/9/2021.
//

#include "context.h"
#include "../access_flag.h"
#include "../types/sharp_class.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"

void create_context(sharp_class *sc, bool isStatic) {
    create_context(&currThread->currTask->file->context, sc, isStatic);
}

void create_context(sharp_function *fun, bool isStatic) {
    create_context(&currThread->currTask->file->context, fun, isStatic);
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
}

void delete_context() {
    delete_context(&currThread->currTask->file->context);
}

void delete_context(context *ctx) {
    if(ctx->storedItems.empty()) {
        ctx->type = no_context;
        ctx->functionCxt = NULL;
        ctx->classCxt = NULL;
        ctx->isStatic = false;
        ctx->localFields.free();
    } else {
        restore_context(ctx);
    }
}

void store_context(context *ctx) {
    ctx->storedItems.add(*ctx);
}

void restore_context(context *ctx) {
    stored_context_item item(ctx->storedItems.last());
    ctx->storedItems.pop_back();
    ctx->copy(item);
}

