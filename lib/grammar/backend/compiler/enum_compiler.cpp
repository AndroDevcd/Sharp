//
// Created by bknun on 7/3/2022.
//

#include "enum_compiler.h"
#include "../types/types.h"
#include "expressions/expression.h"

/*
 * The value the next enum member gets when it has none of its own.
 *
 *   - An enum starts at a number that depends on when it is compiled
 *     (uniqueId, see compile_enum_fields) - on purpose: members without
 *     explicit values are only meant to be told apart, not to be 0, 1, 2.
 *   - A member with an explicit value sets it, and the members after it count
 *     on from there:   enum { first = 5, second, third }   is 5, 6, 7.
 *     (The explicit value was not stepped past, so `second` was 5 again.)
 */
Int lastEnumValue = 0;
void compile_enum_field(sharp_class *with_class, Ast *ast) {
    string name = ast->getToken(0).getValue();
    sharp_field *enum_field = resolve_field(name, with_class, false);

    if(enum_field->fullName == "std.io#thread_priority.min_priority") {
        int r = 0;
    }

    if(enum_field != NULL) {
        if(ast->hasSubAst(ast_expression)) {
            expression e;
            compile_expression(e, ast->getSubAst(ast_expression));

            if(!is_evaluable_type(e.type)) {
                create_new_error(GENERIC, ast, "expected numeric type for enum field but type `" +
                        type_to_str(e.type) + "` was found.");
            } else {
                if(e.type == type_integer) {
                    lastEnumValue = e.type.integer;
                    enum_field->constValue = lastEnumValue++;   // ++: the next member without a value continues from here
                    enum_field->hasConstValue = true;
                } else if(e.type == type_char) {
                    lastEnumValue = e.type._char;
                    enum_field->constValue = lastEnumValue++;   // ++: the next member without a value continues from here
                    enum_field->hasConstValue = true;
                } else if(e.type == type_bool) {
                    lastEnumValue = e.type._bool;
                    enum_field->constValue = lastEnumValue++;   // ++: the next member without a value continues from here
                    enum_field->hasConstValue = true;
                } else if(e.type == type_decimal) {
                    lastEnumValue = e.type.decimal;
                    enum_field->constValue = lastEnumValue++;   // ++: the next member without a value continues from here
                    enum_field->hasConstValue = true;
                }

                enum_field->scheme = new operation_schema();
                enum_field->scheme->copy(e.scheme);
            }
        }


        List<operation_schema*> paramOperations;
        List<sharp_field*> params;

        sharp_type type(type_int8, false, true);
        params.add(new sharp_field(name, NULL, enum_field->implLocation, type, flag_public, normal_field, enum_field->ast));
        paramOperations.add(new operation_schema());
        create_get_string_constant_operation(paramOperations.last(), enum_field->name);

        if(enum_field->scheme != NULL) {
            paramOperations.add(enum_field->scheme);
        } else {
            paramOperations.add(new operation_schema());
            create_get_integer_constant_operation(paramOperations.last(), lastEnumValue++);
        }

        // The ordinal is a whole number, and _enum_'s constructor takes it as a
        // `var`: it is passed as a decimal. (This call is put together by hand,
        // so the conversion compile_function_call does for arguments is done here.)
        {
            sharp_type ordinalType(type_int64), parameterType(type_var);
            convert_scheme_for(paramOperations.last(), ordinalType, parameterType);
        }

        name = "value";
        type = sharp_type(type_var);
        params.add(new sharp_field(name, NULL, enum_field->implLocation, type, flag_public, normal_field, enum_field->ast));

        string std = "std";
        string platform_kernel = "platform.kernel";
        sharp_class *platformClass = resolve_class(get_module(platform_kernel), "platform", false, false);
        sharp_class *enumClass = resolve_class(get_module(std), "_enum_", false, false);
        sharp_function *constructor = resolve_function("_enum_", enumClass, params, constructor_function, exclude_all, ast, false, false);

        if(constructor != NULL) {
            operation_schema valueScheme;
            compile_initialization_call(ast, enumClass, constructor, params, paramOperations, &valueScheme);

            deleteList(params);
            sharp_function *init_function = resolve_function(
                    static_init_function_name, platformClass,
                    params, normal_function, exclude_all,
                    ast, false, false
            );

            if (init_function != NULL) {
                GUARD(globalLock)

                if(init_function->scheme == NULL)
                    init_function->scheme = new operation_schema(scheme_master);

                create_dependency(init_function, enum_field);
                APPLY_TEMP_SCHEME(0, (*init_function->scheme),
                     create_get_value_operation(&scheme_0, &valueScheme);
                     create_static_field_access_operation(&scheme_0, enum_field, false);
                     create_pop_value_from_stack_operation(&scheme_0);
                )

                delete enum_field->scheme;
                enum_field->scheme = NULL;
            } else {
                create_new_error(INTERNAL_ERROR, ast,
                               "cannot locate internal function `static_init` in platform class for closure.");
            }
        } else {
            create_new_error(INTERNAL_ERROR, ast,
                       "cannot locate internal constructor for `_enum_` class field initialization.");
        }
    } else {
        create_new_error(INTERNAL_ERROR, ast,
                     "cannot locate enum field `" + name + "`.");
    }
}

void compile_enum_fields(sharp_class *with_class, Ast *ast) {
    string className = ast->getToken(0).getValue();
    sharp_class *enumClass = resolve_class(with_class, className, false, false);

    if(enumClass != NULL) {
        lastEnumValue = uniqueId++;
        Ast* block = ast->getSubAst(ast_enum_identifier_list);
        create_context(enumClass, true);

        for(Int i = 0; i < block->getSubAstCount(); i++) {
            Ast *trunk = block->getSubAst(i);

            switch(trunk->getType()) {
                case ast_enum_identifier:
                    compile_enum_field(enumClass, trunk);
                    break;
                default:
                    break;
            }
        }

        delete_context();
    } else {
        create_new_error(INTERNAL_ERROR, ast, "could not locate enum class `" + className + "`.");
    }
}
