//
// Created by BNunnally on 10/3/2021.
//

#include "component_compiler.h"
#include "../../../sharp_file.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../astparser/ast_parser.h"
#include "../../types/types.h"
#include "../expressions/expression.h"
#include "../../postprocessor/field_processor.h"

List<Ast*> processedComponents;

void compile_components() {
    sharp_file *file = currThread->currTask->file;
    sharp_class *globalClass = NULL;

    for (Int i = 0; i < file->p->size(); i++) {
        if (panic) return;

        Ast *trunk = file->p->astAt(i);
        if (i == 0) {
            if (trunk->getType() == ast_module_decl) {
                string package = concat_tokens(trunk);
                currModule = create_module(package);
            } else {
                string package = "__$srt_undefined";
                currModule = create_module(package);

                currThread->currTask->file->errors->createNewError(GENERIC, trunk->line, trunk->col,
                                                                   "module declaration must be ""first in every file");
            }

            globalClass = resolve_class(currModule, global_class_name, false, false);
            create_context(globalClass, true);
            continue;
        }

        switch (trunk->getType()) {
            case ast_component_decl:
                compile_component(trunk);
                break;
            default:
                /* ignore */
                break;
        }
    }

    processedComponents.free();
    delete_context();
}


void compile_component(Ast *ast) {
    Ast* componentTypeList = ast->getSubAst(ast_component_type_list);

    string componentName = main_component_name;
    if(ast->getTokenCount() > 0) {
        componentName = ast->getToken(0).getValue();
    }

    component *comp = create_component(componentManager, componentName, ast);
    Int maxTries = componentTypeList->getSubAstCount() + 1;

    for(Int j = 0; j < maxTries; j++) {
        bool lastTry = j + 1 >= maxTries;

        for (Int i = 0; i < componentTypeList->getSubAstCount(); i++) {
            Ast *trunk = componentTypeList->getSubAst(i);

            switch (trunk->getType()) {
                case ast_single_component:
                    compile_sub_component(lastTry, comp, single_component, trunk);
                    break;
                case ast_factory_component:
                    compile_sub_component(lastTry, comp, factory_component, trunk);
                    break;
                default:
                    break;
            }
        }
    }
}


void compile_sub_component(
        bool lastTry,
        component *comp,
        component_representation representation,
        Ast *ast) {
    sharp_file *file = currThread->currTask->file;
    string componentName;

    if(processedComponents.find(ast))
        return;

    if(ast->hasSubAst(ast_component_name)) {
        componentName = ast->getSubAst(ast_component_name)->getToken(0).getValue();
    }

    if(!lastTry) {
        file->errors->enterProtectedMode();
    }

    expression typeDefinition;
    compile_expression(typeDefinition, ast->getSubAst(ast_expression));

    if(typeDefinition.type != type_undefined) {
        if(!lastTry) {
            file->errors->fail();
        }

        component_type *subComponent;
        if(typeDefinition.type == type_get_component_request) {
            get_component_request *request = typeDefinition.type.componentRequest;

            if(request->subComponentName.empty() && request->componentName.empty()) {
                file->errors->createNewError(GENERIC, ast,
                        "cannot get type definition from generic `get()` expression");
            } else {

                if(!request->subComponentName.empty()) {
                    if(!request->componentName.empty()) {
                        subComponent = get_sub_component(
                                componentManager,
                                request->subComponentName,
                                request->componentName
                        );
                    } else subComponent = get_sub_component(componentManager, request->subComponentName);
                } else if(!request->componentName.empty()) {
                    file->errors->createNewError(GENERIC, ast,
                           "cannot get type definition from generic `get(" + request->componentName + ")` expression.");

                }

                if(subComponent != NULL) {
                    typeDefinition.type.copy(*subComponent->type);
                    goto create_sub_component;
                } else {
                    file->errors->createNewError(GENERIC, ast,
                             "could not find type definition for given parameters.");

                }
            }
        } else {
            create_sub_component:
            processedComponents.add(ast);
            if(typeDefinition.type == type_integer
                || typeDefinition.type == type_decimal) {
                typeDefinition.type = type_var;
            } else if(typeDefinition.type == type_char
               || typeDefinition.type == type_bool) {
                typeDefinition.type = type_int8;
            } else if(typeDefinition.type == type_string) {
                typeDefinition.type = type_int8;
            } else if(typeDefinition.type == type_null) {
                typeDefinition.type = type_object;
            } else if(typeDefinition.type == type_field) {
                process_field(typeDefinition.type.field);
                typeDefinition.type.copy(typeDefinition.type.field->type);
            }

            subComponent = create_sub_component(
                    comp,
                    componentName,
                    representation,
                    typeDefinition.type,
                    ast
            );

            if(subComponent != NULL) {
                if(representation == factory_component)
                    subComponent->scheme = new operation_scheme(typeDefinition.scheme);
                else {
                    stringstream ss;
                    ss << single_component_field_name_prefix << subComponent->id;
                    string fieldName = ss.str();
                    sharp_field *single_field = create_field(
                            file,
                            currModule,
                            fieldName,
                            flag_static | flag_public,
                            *subComponent->type,
                            normal_field,
                            ast
                    );

                    single_field->scheme = new operation_scheme(typeDefinition.scheme);
                    subComponent->scheme = new operation_scheme();

                    create_static_field_access_operation(subComponent->scheme, single_field);
                }
            }
        }
    } else {
        if(!lastTry) {
            file->errors->pass();
        }
    }

}