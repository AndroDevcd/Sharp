//
// Created by bknun on 6/7/2022.
//

#include "local_variable_decl.h"
#include "../../../access_flag.h"
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
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                    "local field `" + field->name +
                    "` cannot have setter");
    }

    if(ast->hasSubAst(ast_getter)) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
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
            currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
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
            compile_expression(e, ast->getSubAst(ast_expression));

            if(field->type == type_untyped) {
                validate_field_type(false, field, e.type, &e.scheme,
                                    ast->getSubAst(ast_expression));
            } else {
                convert_expression_type_to_real_type(e);

                if(!is_match(is_implicit_type_match(
                        field->type, e.type, match_operator_overload | match_constructor))) {
                    currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                                       "cannot assign field `" + field->name + ": " +
                                                                       type_to_str(field->type) + "` type `" + type_to_str(e.type) + "`, as types do not match.");
                }
            }

            field->scheme->copy(e.scheme);
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
                    create_local_field_access_operation(fieldScheme, subField);
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