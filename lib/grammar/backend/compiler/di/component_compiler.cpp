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
#include "../../../compiler_info.h"

thread_local List<Ast*> processedComponents;

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

    component *comp = NULL;

    if(componentName == main_component_name) {
        comp = get_component(componentManager, componentName);
    }

    if(comp == NULL)
        comp = create_component(componentManager, componentName, ast);
    Int maxTries = componentTypeList->getSubAstCount() + 1;

    create_context(comp);
    for(Int j = 0; j < maxTries; j++) {
        bool lastTry = j + 1 >= maxTries;

        for (Int i = 0; i < componentTypeList->getSubAstCount(); i++) {
            Ast *trunk = componentTypeList->getSubAst(i);

            switch (trunk->getType()) {
                case ast_single_definition:
                    compile_type_definition(lastTry, comp, single_type_definition, trunk);
                    break;
                case ast_factory_definition:
                    compile_type_definition(lastTry, comp, factory_type_definition, trunk);
                    break;
                default:
                    break;
            }
        }
    }
    delete_context();
}


void compile_type_definition(
        bool lastTry,
        component *comp,
        type_definition_rule rule,
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

        type_definition *subComponent;
        if(typeDefinition.type == type_get_component_request) {
            get_component_request *request = typeDefinition.type.componentRequest;

            if(request->typeDefinitionName.empty() && request->componentName.empty()) {
                file->errors->createNewError(GENERIC, ast,
                        "cannot get type definition from generic `get()` expression");
            } else {
                if(!request->typeDefinitionName.empty()) {
                    if(!request->componentName.empty()) {
                        subComponent = get_type_definition(
                                componentManager,
                                request->typeDefinitionName,
                                request->componentName
                        );
                    } else subComponent = get_type_definition(componentManager, request->typeDefinitionName);
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
            convert_expression_type_to_real_type(typeDefinition);
            if(typeDefinition.type == type_nil) {
                file->errors->createNewError(GENERIC, ast,
                                             "type of `nil` cannot be defined in components.");

            } else if(typeDefinition.type == type_lambda_function) {
                if(!is_fully_qualified_function(typeDefinition.type.fun)) {
                    file->errors->createNewError(GENERIC, ast,
                                                 "lambda must be fully qualified to be injected.");
                }

                typeDefinition.type = type_function_ptr;
            }

            subComponent = create_type_definition(
                    comp,
                    componentName,
                    rule,
                    typeDefinition.type,
                    ast
            );

            if(subComponent != NULL) {
                if(rule == factory_type_definition)
                    subComponent->scheme = new operation_schema(typeDefinition.scheme);
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

                    single_field->scheme = new operation_schema(typeDefinition.scheme);
                    subComponent->scheme = new operation_schema();

                    create_static_field_access_operation(subComponent->scheme, single_field);
                }
            }
        }
    } else {
        if(!lastTry) {
            file->errors->pass();
        } else {
            file->errors->createNewError(GENERIC, ast,
                    "cannot set type of `" + type_to_str(typeDefinition.type) + "` as a type definition.");
        }
    }

}
