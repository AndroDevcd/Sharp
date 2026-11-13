//
// Closures: how a lambda reaches the variables of the function that made it.
// The description is in closure_capture.h.
//

#include "closure_capture.h"
#include "dependancy.h"
#include "../types/types.h"
#include "../context/context.h"
#include "../operation/operation.h"
#include "../../compiler_info.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"

/** the class of the closure object of `fun` (made the first time it is asked for) */
static sharp_class* closure_class_of(sharp_function *fun, Ast *ast) {
    return create_closure_class(currThread->currTask->file, currModule, fun, ast);
}

/** a hidden local or parameter: nothing in the program can name it */
static sharp_field* create_hidden_local(sharp_function *fun, string name, sharp_class *type, Ast *ast) {
    impl_location location(currThread->currTask->file, ast != NULL ? ast->line : 0, ast != NULL ? ast->col : 0);
    sharp_type fieldType(type);
    sharp_field *field = new sharp_field(
            name, fun->owner, location,
            fieldType, flag_public, normal_field, ast
    );

    GUARD(globalLock)
    field->uid = uniqueId++;
    return field;
}

/**
 * `__closure`: the local of `fun` that holds its closure object. The generator
 * creates the object at the start of the function (generate_initial_closure_setup).
 */
static sharp_field* closure_local_of(sharp_function *fun, Ast *ast) {
    if(fun->closure == NULL) {
        fun->closure = create_hidden_local(fun, "__closure", closure_class_of(fun, ast), ast);
        fun->locals.add(fun->closure);
    }
    return fun->closure;
}

/**
 * `__captured`: the hidden last parameter of the lambda `fun`, the closure
 * object of the function around it (`parent`).
 *
 * It is NOT put in fun->parameters here: the parameters are the lambda's type,
 * and `{ a: int -> ... }` must keep matching `(int)()`. It takes the frame
 * slot right after the real parameters (frame slots follow the order of
 * fun->locals), and the generator adds it to the parameter list when all type
 * checking is over (generate_address in function_generator.cpp).
 */
static sharp_field* captured_parameter_of(sharp_function *fun, sharp_function *parent, Ast *ast) {
    if(fun->capturedParameter == NULL) {
        fun->capturedParameter = create_hidden_local(fun, "__captured", closure_class_of(parent, ast), ast);
        fun->locals.insert((uint32_t)fun->parameters.size(), fun->capturedParameter);
    }
    return fun->capturedParameter;
}

/**
 * Adds to `scheme` the steps that leave the closure object of `owner` as the
 * current value, seen from the function being compiled, and returns the class
 * of that object.
 *
 *      chain[0]            the lambda being compiled
 *      chain[1..n-2]       the lambdas around it, innermost first
 *      chain[n-1]          owner
 *
 *      __captured                          the closure object of chain[1]
 *      .__parent  (n-2 times)              ... of chain[2], and so on
 */
static sharp_class* reach_closure_of(
        sharp_function *owner,
        context &ctx,
        operation_schema *scheme,
        Ast *ast) {
    List<sharp_function*> chain;
    chain.add(ctx.functionCxt);

    for(Int i = (Int)ctx.storedItems.size() - 1; i >= 0 && chain.last() != owner; i--) {
        sharp_function *fun = ctx.storedItems.get(i)->functionCxt;
        if(fun != NULL && fun != chain.last())
            chain.add(fun);
    }

    if(chain.size() < 2 || chain.last() != owner || ctx.functionCxt == NULL) {
        create_new_error(INTERNAL_ERROR, ast,
                 " cannot find the function `" + owner->fullName + "` around the lambda that captures from it.");
        return NULL;
    }

    // every function that hands a closure object down has one, and every
    // lambda on the way receives the one of the function around it
    for(Int i = 1; i < (Int)chain.size(); i++) {
        closure_local_of(chain.get(i), ast);
        create_dependency(closure_class_of(chain.get(i), ast));
    }
    for(Int i = 0; i < (Int)chain.size() - 1; i++)
        captured_parameter_of(chain.get(i), chain.get(i + 1), ast);

    if(scheme) scheme->free();
    create_local_field_access_operation(scheme, ctx.functionCxt->capturedParameter);

    for(Int i = 1; i < (Int)chain.size() - 1; i++) {
        sharp_field *parent = create_closure_field(
                closure_class_of(chain.get(i), ast), closure_parent_field_name,
                sharp_type(closure_class_of(chain.get(i + 1), ast)), ast);

        create_dependency(parent);
        create_instance_field_access_operation(scheme, parent);
    }

    chain.free();
    return closure_class_of(owner, ast);
}

sharp_field* capture_enclosing_local(
        sharp_function *owner,
        sharp_field *local,
        context &ctx,
        operation_schema *scheme,
        Ast *ast) {
    sharp_class *closureClass = reach_closure_of(owner, ctx, scheme, ast);
    if(closureClass == NULL) return NULL;

    sharp_field *captured = create_closure_field(closureClass, local->name, local->type, ast);

    // the owner writes the variable to its closure object too: the generator
    // finds the object through these two (scheme_assign_value.cpp)
    local->closure = captured;
    local->closureRef = owner->closure;

    create_dependency(captured);
    create_instance_field_access_operation(scheme, captured);
    return captured;
}

sharp_field* capture_enclosing_self(
        sharp_function *owner,
        sharp_class *selfClass,
        context &ctx,
        operation_schema *scheme,
        Ast *ast) {
    sharp_class *closureClass = reach_closure_of(owner, ctx, scheme, ast);
    if(closureClass == NULL) return NULL;

    sharp_field *captured = create_closure_field(
            closureClass, closure_self_field_name, sharp_type(selfClass), ast);

    create_dependency(captured);
    create_instance_field_access_operation(scheme, captured);
    return captured;
}
