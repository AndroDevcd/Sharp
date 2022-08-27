//
// Created by bknun on 7/3/2022.
//

#include "enum_compiler.h"
#include "../types/types.h"
#include "expressions/expression.h"

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
                    enum_field->constValue = lastEnumValue;
                    enum_field->hasConstValue = true;
                } else if(e.type == type_char) {
                    lastEnumValue = e.type._char;
                    enum_field->constValue = lastEnumValue;
                    enum_field->hasConstValue = true;
                } else if(e.type == type_bool) {
                    lastEnumValue = e.type._bool;
                    enum_field->constValue = lastEnumValue;
                    enum_field->hasConstValue = true;
                } else if(e.type == type_decimal) {
                    lastEnumValue = e.type.decimal;
                    enum_field->constValue = lastEnumValue;
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
            compile_initialization_call(ast, enumClass, constructor, params, paramOperations, &valueScheme); // todo I think we may need to create a dup opcode here becuase the function will use the newed up class in the sack and the lower scope wold be able to pop it off after

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

                APPLY_TEMP_SCHEME(0, (*init_function->scheme),
                     create_get_value_operation(&scheme_0, &valueScheme);
                     create_static_field_access_operation(&scheme_0, enum_field);
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
    } else {
        create_new_error(INTERNAL_ERROR, ast, "could not locate enum class `" + className + "`.");
    }
}
