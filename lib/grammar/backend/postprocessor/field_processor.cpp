//
// Created by BNunnally on 9/10/2021.
//

#include "field_processor.h"
#include "../../frontend/parser/Parser.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "../../taskdelegator/task_delegator.h"
#include "../compiler/expression_compiler.h"
#include "../operation/operation.h"
#include "../types/sharp_function.h"

void process_field(sharp_class *with_class, Ast *ast) {
    string name;
    if(parser::isStorageType(ast->getToken(0))) {
        name = ast->getToken(1).getValue();
    } else name = ast->getToken(0).getValue();

    sharp_field *field = resolve_field(name, with_class);

    process_field(field);
}

void process_field(sharp_field *field) {
    GUARD(field->owner->mut)

    Ast *ast = field->ast;
    if(field->type.type == type_untyped) {
        field->type.type = type_undefined;

        if(field->ast->hasToken(COLON)) {
            expression e(resolve(field->ast->getSubAst(ast_utype)));
            validate_field_type(true, field, e.type, NULL, field->ast);
        } else if(currThread->currTask->file->stage >= pre_compilation_finished_state){
            expression e = compile_expression(field->ast->getSubAst(ast_expression));
            validate_field_type(false, field, e.type, &e.scheme, field->ast);
        } else {
            field->type.type = type_untyped;
            return;
        }

        if(field->type.type != type_undefined) {
            if(ast->hasSubAst(ast_setter)) {
                process_setter(field, ast->getSubAst(ast_setter));
            }

            if(ast->hasSubAst(ast_getter)) {
                process_getter(field, ast->getSubAst(ast_getter));
            }
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
                xtraField->fieldType = field->fieldType;
                xtraField->type = field->type;
            }
        }
    }
}

void process_setter(sharp_field *field, Ast *ast) {
    uInt flags = flag_none;

    if(check_flag(field->flags, flag_local))
        flags |= flag_local;

    if(check_flag(field->flags, flag_public))
        flags |= flag_public;
    else if(check_flag(field->flags, flag_private))
        flags |= flag_private;
    else if(check_flag(field->flags, flag_protected))
        flags |= flag_protected;

    if(check_flag(field->flags, flag_const)) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                               "cannot apply setter to constant field `" + field->name + "`");
    }

    List<sharp_field*> fields;
    impl_location location(currThread->currTask->file, ast);
    fields.add(new sharp_field(
            ast->getToken(1).getValue(),
            field->owner,
            location,
            field->type,
            flags,
            normal_field,
            ast));

    sharp_type returnType(type_nil);
    string name = "set_" + field->name;

    if(!create_function(
            field->owner,
            flags,
            normal_function,
            name,
            false,
            fields,
            returnType,
            ast
            )) {
        deleteList(fields);
    }
}

void process_getter(sharp_field *field, Ast *ast) {
    uInt flags = flag_none;

    if(check_flag(field->flags, flag_local))
        flags |= flag_local;

    if(check_flag(field->flags, flag_public))
        flags |= flag_public;
    else if(check_flag(field->flags, flag_private))
        flags |= flag_private;
    else if(check_flag(field->flags, flag_protected))
        flags |= flag_protected;

    List<sharp_field*> fields;
    impl_location location(currThread->currTask->file, ast);

    sharp_type returnType(type_nil);
    string name = "get_" + field->name;

    create_function(
            field->owner,
            flags,
            normal_function,
            name,
            false,
            fields,
            returnType,
            ast
    );
}


void validate_field_type(
        bool hardType,
        sharp_field *field,
        sharp_type &type,
        operation_scheme *scheme,
        Ast *ast) {
    if(type.type == type_class
        || (type.type >= type_int8 && type.type <= type_object)) {
        if(type.type == type_class)
            create_dependency(field, type._class);

        if(!hardType && scheme->steps.empty()) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast, " cannot assign hard type as value for field `" + field->fullName + "`");
            return;
        }
    } else if(type.type == type_field) {
        create_dependency(field, type.field);

        if(hardType) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast, " illegal use of field `" + type.field->fullName + "` as a type");
            return;
        } else {
            if(type.field == field) {
                currThread->currTask->file->errors->createNewError(GENERIC, ast, " cannot assign field `" + field->fullName + "` to its-self");
                return;
            }
        }

        field->type = type.field->type;
        return;
    } else if(type.type == type_lambda_function) {
        create_dependency(field, type.fun);

        if(!is_fully_qualified_function(type.fun)) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, "expression being assigned to field `" + field->fullName + "` is not a fully qualified lambda expression. Try assigning types to all of the fields in your lambda.");
        } else {
            type.type = type_function_ptr;
        }
    } else if(type.type == type_function_ptr) {
        create_dependency(field, type.fun);

        if(hardType && type.fun->type != blueprint_function) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, " field `" + field->fullName + "` cannot have a method representing the type.");
        } else if(!hardType && type.fun->type == blueprint_function) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, "expression being assigned to field `" + field->fullName + "` must resolve to a class, object, lambda, or number value.");
        }
    } else if(type.type != type_undefined) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                                     " field `" + field->fullName + "` cannot be assigned type `" + type_to_str(type) +
                                     "` due to invalid type assignment format");
    }

    field->type = type;
}
