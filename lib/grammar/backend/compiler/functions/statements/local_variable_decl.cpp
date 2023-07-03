//
// Created by bknun on 6/7/2022.
//

#include "local_variable_decl.h"
#include "../../../../../core/access_flag.h"
#include "../../../../frontend/parser/Ast.h"
#include "../../../astparser/ast_parser.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../../postprocessor/field_processor.h"
#include "../../field_compiler.h"

sharp_field* compile_local_variable_statement(sharp_function *parent, sharp_type *type, uInt flags, Ast *ast) {
    string name = ast->getToken(0).getValue();
    field_type ft = normal_field;
    if(name == "thread_local") {
        ft = tls_field;
        name = ast->getToken(1).getValue();
        flags |= flag_static;
    }


    sharp_field *field = create_local_field(
            current_file,
            &current_context,
            name,
            flags,
            sharp_type(type_untyped),
            ft,
            ast
    );

    field->scheme = new operation_schema();
    if(ast->hasSubAst(ast_setter)) {
        create_new_error(GENERIC, ast->line, ast->col,
                    "local field `" + field->name +
                    "` cannot have setter");
    }

    if(ast->hasSubAst(ast_getter)) {
        create_new_error(GENERIC, ast->line, ast->col,
                    "local field `" + field->name +
                    "` cannot have getter");
    }

    if(type == NULL) {
        if (field->ast->hasToken(COLON)) {
            sharp_type t = resolve(field->ast->getSubAst(ast_utype));
            validate_field_type(true, field, t, NULL, field->ast);
        }
    } else {
        field->type.copy(*type);
    }

    if(field->ast->hasSubAst(ast_inject_request)) {
        if(field->ast->hasSubAst(ast_expression)) {
            create_new_error(GENERIC, ast->line, ast->col,
                                                               "injected field `" + field->name +
                                                               "` cannot be assigned a value");
        } else {
            Ast *irAst = field->ast->getSubAst(ast_inject_request);
            if (irAst->getTokenCount() > 0) {
                field->request = new injection_request(irAst->getToken(0).getValue());
            } else {
                string anyComponent = any_component_name; // we must find the component to inject the field value
                field->request = new injection_request(anyComponent);
            }

            compile_field_injection_request(field, ast);
        }
    } else {
        if(ast->hasSubAst(ast_expression)) {
            expression e;
            sharp_function *initializer_function = nullptr;
            uInt field_match_result = 0;
            compile_expression(e, ast->getSubAst(ast_expression));

            if(field->type == type_untyped) {
                validate_field_type(false, field, e.type, &e.scheme,
                                    ast->getSubAst(ast_expression));
            }

            convert_expression_type_to_real_type(e);
            field_match_result = is_implicit_type_match(
                    field->type, e.type, match_operator_overload | match_constructor, initializer_function);

            if(field_match_result == no_match_found) {
                create_new_error(GENERIC, ast,
                                 "cannot assign field `" + field->name + ": " +
                                 type_to_str(field->type) + "` type `" + type_to_str(e.type) + "`, as types do not match.");
            } else if(field_match_result == indirect_match_w_nullability_mismatch) {
                create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                                 " expressions are not compatible, assigning nullable type of `" +
                                 type_to_str(field->type) + "` to non nullable type of `" + type_to_str(e.type) + "`.");
            }

            if(field_match_result == match_initializer) {
                APPLY_TEMP_SCHEME_WITHOUT_INJECT(5,
                    compile_initialization_call(ast, initializer_function, e, &scheme_5);
                )

                e.scheme.free();
                e.scheme.copy(scheme_5);
            }

            field->scheme->copy(e.scheme);
        } else if(field->type.type == type_class && pre_initialize_class(field->type._class)) {
            List<sharp_field*> params;
            List<operation_schema*> paramOps;
            auto constr = resolve_function(field->type._class->name, field->type._class, params,
                                           constructor_function, exclude_all, ast, false, false);

            if(constr != NULL) {
                compile_initialization_call(ast, field->type._class,
                                            constr, params, paramOps, field->scheme);
            } else {
                create_new_error(GENERIC, ast,
                                 "no default constructor found for class `" + field->type._class->fullName + "`:");
            }
        }
    }

    return field;
}

void compile_local_variable_statement(Ast *ast, operation_schema *scheme) {
    uInt flags = flag_none;
    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_excuse | flag_const,
                "field", current_context.functionCxt->owner,
                ast->getSubAst(ast_access_type)
        );
    }
    flags |= flag_public;

    operation_schema *subScheme = new operation_schema(scheme_master);
    sharp_field *field = compile_local_variable_statement(current_context.functionCxt, NULL, flags, ast);

    APPLY_TEMP_SCHEME(0, *subScheme,
        operation_schema *fieldScheme = new operation_schema();

        if(field->scheme->schemeType != scheme_none && !field->scheme->steps.empty()) {
            if(field->fieldType == tls_field)
                create_tls_field_access_operation(fieldScheme, field);
            else
                create_local_field_access_operation(fieldScheme, field);
            create_value_assignment_operation(&scheme_0, fieldScheme, field->scheme);
        } else {
            create_setup_local_field_operation(&scheme_0, field);
        }

        delete fieldScheme;
    )

    if(ast->hasSubAst(ast_variable_decl)) {
        long startAst = 0;
        for(Int i = 0; i < ast->getSubAstCount(); i++) {
            if(ast->getSubAst(i)->getType() == ast_variable_decl)
            {
                startAst = i;
                break;
            }
        }

        for(Int i = startAst; i < ast->getSubAstCount(); i++) {
            sharp_field *subField = compile_local_variable_statement(current_context.functionCxt, &field->type, flags, ast->getSubAst(i));

            APPLY_TEMP_SCHEME(1, *subScheme,
                operation_schema *fieldScheme = new operation_schema();

                if(field->scheme->schemeType != scheme_none && !subField->scheme->steps.empty()) {
                    if(field->fieldType == tls_field)
                        create_tls_field_access_operation(fieldScheme, field);
                    else
                        create_local_field_access_operation(fieldScheme, field);
                    create_value_assignment_operation(&scheme_1, fieldScheme, subField->scheme);
                } else {
                    create_setup_local_field_operation(&scheme_1, subField);
                }

                delete fieldScheme;
            )
        }
    }

    add_scheme_operation(scheme, subScheme);
    delete subScheme;
}