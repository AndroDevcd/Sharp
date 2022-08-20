//
// Created by BNunnally on 9/9/2021.
//

#include "context.h"
#include "../access_flag.h"
#include "../types/sharp_class.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_function.h"
#include "../../compiler_info.h"
#include "../operation/operation.h"

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
    ctx->blockInfo.free();
}

void create_block(context *ctx, block_type type) {
    if(ctx->blockInfo.type != invalid_block) {
        store_block(&ctx->blockInfo);
    }

    if(ctx->blockInfo.lockScheme)
        int i = 0;
    delete ctx->blockInfo.lockScheme;
    ctx->blockInfo.lockScheme = NULL;
    ctx->blockInfo.type = type;
    if(ctx->blockInfo.storedItems.empty())
        ctx->blockInfo.id = 0;
    else ctx->blockInfo.id = ctx->blockInfo.storedItems.last()->id + 1;
}

void create_context(context *ctx, component *comp) {
    if(ctx->type != no_context) {
        store_context(ctx);
    }

    ctx->type = component_context;
    ctx->componentCtx = comp;
}

void delete_block() {
    delete_block(&currThread->currTask->file->context.blockInfo);
}

void delete_context() {
    delete_context(&currThread->currTask->file->context);
}

void delete_block(block_info *info) {
    if(info->storedItems.empty()) {
        info->type = invalid_block;
        info->id = -1;
        info->line = -1;
        info->reachable = true;
        delete info->lockScheme; info->lockScheme = NULL;
        deleteList(info->storedItems);
    } else {
        restore_block(info);
    }
}

void delete_context(context *ctx) {
    if(ctx->storedItems.empty()) {
        ctx->type = no_context;
        ctx->functionCxt = NULL;
        ctx->componentCtx = NULL;
        ctx->classCxt = NULL;
        ctx->isStatic = false;
        ctx->localFields.free();
        deleteList(ctx->blockInfo.storedItems);
    } else {
        restore_context(ctx);
    }
}

void store_block(block_info *info) {
    info->storedItems.add(new stored_block_info(*info));
}

void store_context(context *ctx) {
    ctx->storedItems.add(new stored_context_item(*ctx));
}

void retrieve_lock_schemes(block_info *info, List<operation_schema*> schemes) {
    if(info->lockScheme != NULL)
        schemes.add(new operation_schema(*info->lockScheme));

    for(Int i = 0; i < info->storedItems.size(); i++) {
        if(info->storedItems.get(i)->lockScheme != NULL)
            schemes.add(new operation_schema(*info->storedItems.get(i)->lockScheme));
    }
}

bool inside_block(block_info *info, block_type type) {
    if(info->type != type) {
        for(Int i = 0; i < info->storedItems.size(); i++) {
            if(info->storedItems.get(i)->type == type)
                return true;
        }

        return false;
    } else return true;
}

stored_block_info* retrieve_block(block_info *info, block_type type) {
    if(info->type != type) {
        for(Int i = (Int)info->storedItems.size() - 1; i >= 0; i--) {
            if(info->storedItems.get(i)->type == type)
                return info->storedItems.get(i);
        }

        return NULL;
    } else return info;
}

sharp_label* retrieve_next_finally_label(block_info *info) {
    if(info->finallyLabel != NULL)
        return info->finallyLabel;

    for(Int i = (Int)info->storedItems.size() - 1; i >= 0; i--) {
        if(info->storedItems.get(i)->finallyLabel != NULL)
            return info->storedItems.get(i)->finallyLabel;
    }
    return NULL;
}

void restore_block(block_info *info) {
    // we preserve any carry-over items from the block were exiting
    info->copy(*info->storedItems.last());
    info->reachable = info->storedItems.last()->reachable;

    free(info->storedItems.last());
    info->storedItems.pop_back();
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
    if(ctx->type == block_context)
        return ctx->functionCxt;

    sharp_function *fun = NULL;
    for(Int i = (Int)ctx->storedItems.size() - 1; i >= 0; i--) {
        stored_context_item *contextItem = ctx->storedItems.get(i);
        if(contextItem->type == block_context)
            fun = contextItem->functionCxt;
    }

    return fun;
}

void block_info::copy_all(const block_info &info)  {
    copy(*this);
    deleteList(storedItems);

    for(Int i = 0; i < info.storedItems.size(); i++) {
        storedItems.add(new stored_block_info(*info.storedItems.get(i)));
    }
}

void stored_block_info::free() {
    delete lockScheme; lockScheme = NULL;
}

void stored_block_info::copy(const stored_block_info &item)  {
    type = item.type;
    id = item.id;
    line = item.line;
    reachable = item.reachable;
    endLabel = item.endLabel;
    beginLabel = item.beginLabel;
    finallyLabel = item.finallyLabel;
    if(item.lockScheme)
        lockScheme = new operation_schema(*item.lockScheme);
    else lockScheme = NULL;
}

void stored_context_item::copy(const stored_context_item &item)  {

    type = item.type;
    classCxt = item.classCxt;
    functionCxt = item.functionCxt;
    componentCtx = item.componentCtx;
    localFields.addAll(item.localFields);
    localAliases.addAll(item.localAliases);
    labels.addAll(item.labels);
    isStatic = item.isStatic;
    blockInfo.copy_all(item.blockInfo);
}
