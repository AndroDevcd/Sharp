//
// Created by BNunnally on 9/17/2021.
//

#include "expression.h"
#include "unary/minus_expression.h"
#include "../../dependency/dependancy.h"
#include "../../types/types.h"
#include "../../../taskdelegator/task_delegator.h"
#include "unary/pre_increment_expression.h"
#include "primary/binary_expression.h"
#include "../../postprocessor/field_processor.h"
#include "dictionary_expression.h"
#include "vector_array_expression.h"
#include "elvis_expression.h"
#include "assign_expression.h"
#include "binary/binary_expression.h"
#include "../../postprocessor/function_processor.h"
#include "../../../compiler_info.h"
#include "inline_if_expression.h"
#include "primary/force_non_null_expression.h"

void compile_expression(expression &e, Ast *ast, Int endLabel) {
    if(ast->getType() == ast_expression)
        compile_expression(e, ast->getSubAst(0));
    else if(ast->getType() == ast_minus_e)
        compile_minus_expression(&e, ast);
    else if(ast->getType() == ast_pre_inc_e)
        compile_pre_increment_expression(&e, ast);
    else if(ast->getType() == ast_primary_expr)
        compile_primary_expression(&e, ast, endLabel);
    else if(ast->getType() == ast_dictionary_array)
        compile_dictionary_expression(&e, ast);
    else if(ast->getType() == ast_vect_e)
        compile_vector_array_expression(&e, ast->getSubAst(ast_vector_array));
    else if(ast->getType() == ast_force_non_null_e)
        compile_force_non_null_expression(&e, ast);
    else if(ast->getType() == ast_elvis_e)
        compile_elvis_expression(&e, ast);
    else if(ast->getType() == ast_ques_e)
        compile_inline_if_expression(&e, ast);
    else if(ast->getType() == ast_assign_e)
        compile_assign_expression(&e, ast);
    else if(ast->getType() == ast_and_e
        || ast->getType() == ast_equal_e
        || ast->getType() == ast_less_e
        || ast->getType() == ast_shift_e
        || ast->getType() == ast_add_e
        || ast->getType() == ast_mult_e
        || ast->getType() == ast_exponent_e)
        compile_binary_expression(&e, ast);
}

void compile_expression_for_type(sharp_type &type, Ast *ast) {
    expression e;
    compile_expression(e, ast);

    type.copy(e.type);
}

/**
 * Classes whose variables get an object without being given one: `n : int;`
 * holds an int object with the value 0.
 *
 * The callers ask this only for a plain variable of the class. A nullable one
 * (`n : int?;`) and an array (`n : int[];`) start as null like every other
 * nullable variable - they used to be given an object too, so `n == null` was
 * false for a variable nothing had been assigned to.
 */
bool pre_initialize_class(sharp_class *sc) {
    return sc->fullName == "std#bool" ||
       sc->fullName == "std#byte" ||
       sc->fullName == "std#short" ||
       sc->fullName == "std#char" ||
       sc->fullName == "std#int" ||
       sc->fullName == "std#long" ||
       sc->fullName == "std#ushort" ||
       sc->fullName == "std#uchar" ||
       sc->fullName == "std#uint" ||
       sc->fullName == "std#ulong" ||
       sc->fullName == "std#double" ||
       sc->fullName == "std#string";
}

void convert_expression_type_to_real_type(
        expression &typeDefinition) {
    if(typeDefinition.type == type_integer
       || typeDefinition.type == type_decimal) {
        // A whole-number literal is an _int32 - unless it does not fit in
        // one: 4294967296 is an _int64. (It used to be an _int32 always, and
        // `text + 4294967296` then called the _int32 function and printed 0.)
        Int whole = typeDefinition.type.integer;
        bool fitsInt32 = whole >= INT32_MIN && whole <= INT32_MAX;
        typeDefinition.type = typeDefinition.type == type_decimal ? type_var
                            : fitsInt32 ? type_int32 : type_int64;
    } else if(typeDefinition.type == type_char
              || typeDefinition.type == type_bool) {
        typeDefinition.type = type_int8;
    } else if(typeDefinition.type == type_string) {
        typeDefinition.type = type_int8;
        typeDefinition.type.isArray = true;
    } else if(typeDefinition.type == type_field) {
        process_field(typeDefinition.type.field);
        bool nullable = typeDefinition.type.nullable || typeDefinition.type.field->type.nullable;
        typeDefinition.type.copy(typeDefinition.type.field->type);
        typeDefinition.type.nullable = nullable;
    }
}

void recompile_cond_expression(expression &out, Ast *ast) {
    out.scheme.free();
    out.type.free();
    compile_cond_expression(out, ast);
}

void extract_value_field_from_expression(expression &e, string className, Ast *ast, bool forceError) {
    sharp_class *sc = get_class_type(e.type);

    if(sc != NULL && !e.type.nullable && !e.type.isArray &&
        (
            (!className.empty() && sc->fullName == className)
            || (className.empty() && sc->fullName == "std#bool")
            || (className.empty() && sc->fullName == "std#byte")
            || (className.empty() && sc->fullName == "std#short")
            || (className.empty() && sc->fullName == "std#char")
            || (className.empty() && sc->fullName == "std#int")
            || (className.empty() && sc->fullName == "std#long")
            || (className.empty() && sc->fullName == "std#ushort")
            || (className.empty() && sc->fullName == "std#uchar")
            || (className.empty() && sc->fullName == "std#uint")
            || (className.empty() && sc->fullName == "std#ulong")
            || (className.empty() && sc->fullName == "std#double")
        )
    ) {
        sharp_field *field = resolve_field("value", sc, true);

        if(field != NULL) {
            APPLY_TEMP_SCHEME(1, e.scheme,
                 create_get_value_operation(&scheme_1, &e.scheme);
                 create_instance_field_access_operation(&scheme_1, field);
                 e.scheme.free();
            )

            e.type.copy(field->type);
        } else {
            create_new_error(COULD_NOT_RESOLVE, ast,
                             " field `" + className + ".value`  was not found.");
        }
    } else if(forceError && !is_evaluable_type(e.type)) {
        create_new_error(GENERIC,  ast, "expression must evaluate to a numeric value, true, or false. But type `" + type_to_str(e.type) + "` was found.");
    }
}

void compile_cond_expression(expression &e, Ast *ast) {
    compile_expression(e, ast);
    extract_value_field_from_expression(e, "std#bool", ast);

    // a conditional jump tests an integer: a decimal condition (`if(total)`)
    // becomes 0 or 1 first
    convert_number_to_condition(e);
}

void compile_initialization_call(
        Ast *ast,
        sharp_function *constructor,
        expression &e,
        operation_schema *scheme) {
    string name = "";
    List<sharp_field *> params;
    List<operation_schema *> paramOperations;

    impl_location location;
    params.add(new sharp_field(
            name, get_primary_class(&currThread->currTask->file->ctx), location,
            e.type, flag_public, normal_field,
            ast
    ));
    paramOperations.add(new operation_schema(e.scheme));

    create_new_class_operation(scheme, constructor->owner);
    create_duplicate_operation(scheme);

    APPLY_TEMP_SCHEME(0, *scheme,
    compile_function_call(
            &scheme_0, params,
            paramOperations, constructor,
            false,
            false,
            false);
    )
}

void compile_initialization_call(
        Ast *ast,
        sharp_class *with_class,
        sharp_function *constructor,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        operation_schema *scheme) {
    create_new_class_operation(scheme, with_class);
    create_duplicate_operation(scheme);

    APPLY_TEMP_SCHEME(0, *scheme,
         compile_function_call(
                 &scheme_0, params,
                 paramOperations, constructor,
                 false,
                 false,
                 false);
    )
}

/**
 * Schemes whose steps are only meaningful to their own generator (they
 * allocate registers / labels in a fixed order, or leave a new object on the
 * stack) and therefore cannot simply get another step appended to them.
 */
bool scheme_has_own_layout(_operation_scheme type) {
    return type == scheme_binary_math || type == scheme_and || type == scheme_or
        || type == scheme_inline_if || type == scheme_compound_assign_value
        || type == scheme_null_fallback || type == scheme_assign_value
        || type == scheme_new_class;
}

/**
 * Turn the steps of `scheme` into ONE step, "get the value of <those steps>",
 * so that more steps (a cast, an operator call on the result) can follow.
 *
 *      before:  scheme = [ a ] [ b ] [ c ]            (layout of its own)
 *      after:   scheme = [ get value ( [a] [b] [c] ) ]
 */
void wrap_scheme_as_value(operation_schema &scheme) {
    operation_schema *value = new operation_schema(scheme);
    scheme.free();
    create_get_value_operation(&scheme, value, false, true);   // true: it is now a plain "get value" scheme
}

sharp_function* compile_class_function_overload(
        sharp_class *with_class,
        expression &e,
        List<sharp_field*> &params,
        List<operation_schema*> &paramOperations,
        string &op,
        Ast *ast,
        bool allowMissing) {
    sharp_function *fun;

    if((fun = resolve_function(
            "operator" + op,
            with_class,
            params,
            operator_function,
            match_initializer | match_operator_overload,
            ast,
            true,
            true)) != NULL) {
        data_type prevType = fun->returnType.type;
        process_function_return_type(fun);

        // The operator function is called ON the value `e`, by appending the
        // call to the steps that produce it. When those steps are more than a
        // plain access - a call, a `new`, arithmetic: `!new box().is_ok()` -
        // they are first made ONE "get value" step, so the call has a single
        // finished value to work on. (Without this the generator stopped with
        // "attempting to consume empty machine data".)
        if(e.scheme.steps.size() > 1 || scheme_has_own_layout(e.scheme.schemeType))
            wrap_scheme_as_value(e.scheme);

        compile_function_call(&e.scheme, params, paramOperations, fun, false, false,
                              false);

        create_dependency(fun);
        e.type.copy(fun->returnType);
        return fun;
    } else {
        if(!allowMissing) {
            create_new_error(GENERIC, ast,
                             "use of operator `" + op + "` does not have any qualified overloads with class `" +
                             with_class->fullName + "`");
        }

        return NULL;
    }
}

// ---------------------------------------------------------------------------------
// Direct call or run-time lookup?
// Which function does a call on an object run when a subclass has its own
// version?
//
//      interface named { def name(): string; }
//      class rectangle : named {
//          override def name(): string { return "rectangle"; }
//          def describe(): string { return "a " + name(); }
//      }
//      class labelled_rectangle base rectangle {
//          override def name(): string { return "label"; }
//      }
//
//      r : rectangle = new labelled_rectangle();
//      r.name();           // must be "label": the object is a labelled_rectangle
//      r.describe();       // must be "a label"
//
// A call written against the class `rectangle` used to be bound to
// rectangle.name when it was compiled, whatever the object really was. Only a
// call through the INTERFACE type looked the function up at run time.
//
// THE RULE
// A call to a function that implements an interface method goes through that
// interface method (a run-time lookup, the INVOKE_DELEGATE instruction) when
// some class DERIVED from the function's class implements the same method too.
// When no subclass does, the object can only run this very function and the
// call stays a direct one - nothing gets slower.
//
// NEVER through the interface: a call on `base` (`base->name()`). It names the
// base class's function on purpose; looking it up again would call the
// function it was made from, forever.
//
// C++ comparison: a virtual call. C++ makes every call of a virtual function a
// lookup and lets the optimizer "devirtualize" the ones it can prove; here the
// compiler sees the whole program and only emits the lookup where it matters.
//
// Only interface / delegate methods take part (`override` applies to those
// alone, see tests/suite/cases/inheritance.sharp): a plain base-class method
// is still bound when compiled.
//
// ---------------------------------------------------------------------------------

// the compiler compiles several files at once, one per worker thread
static thread_local sharp_class *baseCallClass = nullptr;

void set_base_call_class(sharp_class *baseClass) {
    baseCallClass = baseClass;
}

/** is `sc` the class `ancestor` or derived from it? */
static bool is_or_derives_from(sharp_class *sc, sharp_class *ancestor) {
    for(sharp_class *c = sc; c != nullptr; c = c->baseClass) {
        if(c == ancestor) return true;
    }
    return false;
}

static sharp_function* dispatch_target(sharp_function *callee) {
    if(callee == nullptr || callee->delegate == nullptr || callee->type == delegate_function)
        return callee;      // implements no interface method (or is one itself)

    // `base->f()`: the base class's own function, on purpose
    if(baseCallClass != nullptr && is_or_derives_from(baseCallClass, callee->owner))
        return callee;

    // does a class derived from the callee's class implement the method too?
    List<sharp_function*> &implementations = callee->delegate->impls;
    for(Int i = 0; i < implementations.size(); i++) {
        sharp_function *other = implementations.get(i);
        if(other != callee && other->owner != callee->owner
           && is_or_derives_from(other->owner, callee->owner))
            return callee->delegate;
    }

    return callee;
}

void compile_function_call(
        operation_schema *scheme,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        sharp_function *callee,
        bool isStaticCall,
        bool isPrimaryClass,
        bool isFunctionPointer) {
    if(scheme) {
        Int matchResult;
        List<operation_schema*> compiledParamOperations;

        for (Int i = 0; i < params.size(); i++) {
            sharp_type *asignee = &callee->parameters.get(i)->type;
            sharp_type *assigner = &params.get(i)->type;
            sharp_function *matchedConstructor = NULL;

            if(assigner->type == type_get_component_request) {
                paramOperations.get(i)->copy(
                        *assigner->componentRequest->resolvedTypeDefinition->scheme);
                matchResult = direct_match;
            } else {
                matchResult = is_explicit_type_match(*asignee, *assigner);
                if (matchResult == no_match_found) {
                    matchResult = is_implicit_type_match(
                            *asignee, *assigner, constructor_only,
                            matchedConstructor);

                    if(matchedConstructor != NULL)
                        create_dependency( matchedConstructor);
                } else if(*assigner == type_lambda_function
                   && !is_fully_qualified_function(assigner->fun)) {
                    fully_qualify_function(assigner->fun, asignee->fun);
                }
            }


            // an argument becomes the kind of number its parameter is:
            // area(count) for `def area(side: var)` passes a decimal
            if(is_match_normal(matchResult)) {
                convert_scheme_for(paramOperations.get(i), *assigner, *asignee);
                // ... and is cut to the parameter's size: f(200) for
                // `def f(x: _int8)` passes -56
                narrow_scheme_for(paramOperations.get(i), *assigner, *asignee);
            }
            // ... or of the constructor it is wrapped with: an _int32 given to a
            // parameter of a class that is built from a var
            else if(matchedConstructor != NULL && matchedConstructor->parameters.size() == 1)
                convert_scheme_for(paramOperations.get(i), *assigner,
                                   matchedConstructor->parameters.get(0)->type);

            compiledParamOperations.add(new operation_schema());
            create_function_parameter_push_operation(
                    asignee,
                    matchResult,
                    matchedConstructor,
                    paramOperations.get(i),
                    compiledParamOperations.last()
            );
        }

        create_dependency(callee);
        if(isFunctionPointer) {
            ALLOCATE_REGISTER_1X(1, scheme,
              create_dynamic_function_call_operation(scheme, compiledParamOperations, callee, register_1, false);
           )
        } else if (isStaticCall) {
            create_static_function_call_operation(scheme, compiledParamOperations, callee);
        } else {
            // a call on an object: direct, or through the interface method when
            // a subclass has its own version (dispatch_target above)
            sharp_function *target = dispatch_target(callee);
            if(target != callee)
                create_dependency(target);

            if (isPrimaryClass)
                create_primary_class_function_call_operation(scheme, compiledParamOperations, target);
            else {
                create_instance_function_call_operation(scheme, compiledParamOperations, target);
            }
        }
    }
}
