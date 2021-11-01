//
// Created by BNunnally on 10/8/2021.
//

#include "field_compiler.h"
#include "../../sharp_file.h"
#include "../astparser/ast_parser.h"
#include "../types/types.h"
#include "expressions/expression.h"
#include "../postprocessor/field_processor.h"
#include "../../compiler_info.h"
#include "../postprocessor/function_processor.h"

void compile_fields() {
    sharp_file *file = current_file;
    sharp_class *globalClass = NULL;

    for(Int i = 0; i < file->p->size(); i++)
    {
        if(panic) return;

        Ast *trunk = file->p->astAt(i);
        if(i == 0) {
            if(trunk->getType() == ast_module_decl) {
                string package = concat_tokens(trunk);
                currModule = create_module(package);
            } else {
                string package = "__$srt_undefined";
                currModule = create_module(package);

                currThread->currTask->file->errors->createNewError(GENERIC, trunk->line, trunk->col, "module declaration must be ""first in every file");
            }

            globalClass = resolve_class(currModule, global_class_name, false, false);
            create_context(globalClass, true);
            continue;
        }

        switch(trunk->getType()) {
            case ast_class_decl:
                compile_class_fields(globalClass, NULL, trunk);
                break;
            case ast_variable_decl:
                compile_field(globalClass, trunk);
                break;
            default:
                /* */
                break;
        }
    }

    delete_context();
}

void compile_class_fields(sharp_class* parentClass, sharp_class *with_class, Ast *ast) {
    Ast* block = ast->getSubAst(ast_block);

    if(with_class == NULL) {
        string name = ast->getToken(0).getValue();
        with_class = resolve_class(parentClass, name, false, false);
    }

    create_context(with_class, true);
    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_class_decl:
                compile_class_fields(with_class, NULL, trunk);
                break;
            case ast_variable_decl:
                compile_field(with_class, trunk);
                break;
            default:
                /* ignore */
                break;
        }
    }

    delete_context();
}

void compile_field(sharp_class *with_class, Ast *ast) {
    string name;
    if(parser::isStorageType(ast->getToken(0))) {
        name = ast->getToken(1).getValue();
    } else name = ast->getToken(0).getValue();

    sharp_field *field = resolve_field(name, with_class);

    compile_field(field, ast);
}

void compile_field(sharp_field *field, Ast *ast) {
    GUARD(globalLock)

    if(field->name == "foo" && field->owner->name == "_object_") {
        // todo: remove here for testing only
        string fname = "%test";
        List<sharp_field*> params;
        sharp_type returnType(type_nil);
        create_function(field->owner, flag_public, normal_function, fname, false,
                params, returnType, ast);

        sharp_function *fun = field->owner->functions.last();
        create_context(fun);
        expression e;
        compile_expression(e, ast->getSubAst(ast_expression));

        validate_function_type(false, fun, e.type, &e.scheme, fun->ast);
        delete_context();
    }

    if(field->request == NULL
        && field->scheme == NULL) {

        if(ast->hasSubAst(ast_expression)) {
            expression e;
            field->scheme = new operation_scheme();
            create_context(field->owner, check_flag(field->flags, flag_static));
            compile_expression(e, ast->getSubAst(ast_expression));
            delete_context();

            if(field->type == type_untyped) {
                validate_field_type(false, field, e.type, &e.scheme,
                                    ast->getSubAst(ast_expression));
            } else {
                convert_expression_type_to_real_type(ast, current_file, e);
                if(!is_implicit_type_match(
                        field->type, e.type, match_operator_overload | match_constructor)) {
                    currThread->currTask->file->errors->createNewError(GENERIC, ast,
                               "cannot assign field `" + field->name + ": " +
                                    type_to_str(field->type) + "` type `" + type_to_str(e.type) + "`, as types do not match.");
                }
            }
            field->scheme->copy(e.scheme);
        }

        if(ast->hasSubAst(ast_getter) && field->getter == NULL) {
            process_getter(field, ast->getSubAst(ast_getter));
        }

        if(ast->hasSubAst(ast_setter) && field->setter == NULL) {
            process_setter(field, ast->getSubAst(ast_setter));
        }

        if(ast->hasSubAst(ast_variable_decl)) {
            long startAst = 0;
            for(long i = 0; i < ast->getSubAstCount(); i++) {
                if(ast->getSubAst(i)->getType() == ast_variable_decl)
                {
                    startAst = i;
                    break;
                }
            }

            for(long i = startAst; i < ast->getSubAstCount(); i++) {
                Ast *trunk = ast->getSubAst(i);

                string name = trunk->getToken(0).getValue();
                sharp_field *xtraField = resolve_field(name, field->owner);

                if(xtraField) {
                    xtraField->fieldType = field->fieldType;
                    xtraField->type.copy(field->type);

                    if(trunk->hasSubAst(ast_expression)) {
                        expression e;
                        xtraField->scheme = new operation_scheme();
                        compile_expression(e, trunk->getSubAst(ast_expression));

                        if(xtraField->type == type_untyped) {
                            validate_field_type(false, xtraField, e.type, &e.scheme,
                                                ast->getSubAst(ast_expression));
                        } else {
                            convert_expression_type_to_real_type(ast, current_file, e);
                            if(!is_implicit_type_match(
                                    xtraField->type, e.type, match_operator_overload | match_constructor)) {
                                currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                            "cannot assign field `" + xtraField->name + ": " +
                                                  type_to_str(xtraField->type) + "` type `" + type_to_str(e.type) + "`, as types do not match.");
                            }
                        }
                        xtraField->scheme->copy(e.scheme);
                    }
                }
            }
        }
    }
}