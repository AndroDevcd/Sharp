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

                create_new_error(GENERIC, trunk->line, trunk->col,
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
    sharp_function container;
    create_context(&container);
    compile_expression(typeDefinition, ast->getSubAst(ast_expression));
    delete_context();

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
                subComponent->dependencies.addAll(container.dependencies);
                if(rule == factory_type_definition)
                    subComponent->scheme = new operation_schema(typeDefinition.scheme);
                else {
                    stringstream ss;
                    ss << single_component_field_name_prefix << subComponent->id;
                    string fieldName = ss.str(); ss.str("");
                    ss << single_component_field_init_flag_prefix << subComponent->id;
                    string initFlagName = ss.str(); ss.str("");
                    ss << single_component_field_init_prefix << subComponent->id;
                    string initName = ss.str(); ss.str("");
                    sharp_field *single_field = create_field(
                            file,
                            currModule,
                            fieldName,
                            flag_static | flag_public,
                            *subComponent->type,
                            normal_field,
                            ast
                    );

                    sharp_type initFlagType(type_var);
                    sharp_field *single_init_flag = create_field(
                            file,
                            currModule,
                            initFlagName,
                            flag_static | flag_public,
                            initFlagType,
                            normal_field,
                            ast
                    );

                    List<sharp_field*> params;
                    sharp_function *init_func;
                    create_function(
                            resolve_class(currModule, global_class_name, false, false), flag_static | flag_public, normal_function,
                            initName, false, params,
                            *subComponent->type, ast, init_func);

                    if(single_init_flag && init_func && single_field) {
                        init_func->scheme = new operation_schema(scheme_master);
                        init_func->returnProtected = true;

                        create_context(init_func);
                        // code initialization block
                        APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
                           APPLY_TEMP_SCHEME_WITHOUT_INJECT(0,
                              create_static_field_access_operation(&scheme_0, single_init_flag);
                           )
                           APPLY_TEMP_SCHEME_WITHOUT_INJECT(5,
                              create_static_field_access_operation(&scheme_5, single_field);
                           )

                           ss.str("");
                           List<operation_schema*> emptyParams;
                           set_internal_label_name(ss, "if_end", uniqueId++)
                           auto endLabel = create_label(ss.str(), &current_context, ast, &scheme_1);
                           create_get_value_operation(&scheme_1, &scheme_0, false);
                           create_jump_if_false_operation(&scheme_1, endLabel);

                           if(init_func->returnType.type == type_nil) {
                               create_return_operation(&scheme_1);
                           } else if(is_numeric_type(init_func->returnType) && !get_real_type(init_func->returnType).isArray) {
                               create_numeric_return_operation(&scheme_1, &scheme_5);
                           } else {
                               create_object_return_operation(&scheme_1, &scheme_5);
                           }

                           create_set_label_operation(&scheme_1, endLabel);

                           APPLY_TEMP_SCHEME_WITHOUT_INJECT(4,
                               APPLY_TEMP_SCHEME_WITHOUT_INJECT(2,
                                   create_get_integer_constant_operation(&scheme_2, 1);
                               )

                               create_value_assignment_operation(&scheme_4, &scheme_0, &scheme_2);
                           )

                           add_scheme_operation(&scheme_1, &scheme_4);
                           create_unused_data_operation(&scheme_1);


                           APPLY_TEMP_SCHEME_WITHOUT_INJECT(7,
                               APPLY_TEMP_SCHEME_WITHOUT_INJECT(6,
                                   create_get_value_operation(&scheme_6, &typeDefinition.scheme, false);
                               )

                               create_value_assignment_operation(&scheme_7, &scheme_5, &scheme_6);
                           )

                           add_scheme_operation(&scheme_1, &scheme_7);
                           create_unused_data_operation(&scheme_1);

                           if(init_func->returnType.type == type_nil) {
                               create_return_operation(&scheme_1);
                           } else if(is_numeric_type(init_func->returnType) && !get_real_type(init_func->returnType).isArray) {
                               create_numeric_return_operation(&scheme_1, &scheme_5);
                           } else {
                               create_object_return_operation(&scheme_1, &scheme_5);
                           }

                           scheme_1.schemeType = scheme_process_steps;
                           create_unused_data_operation(&scheme_1);
                        )
                        delete_context();

                        init_func->scheme->steps.add(new operation_step(operation_step_scheme, &scheme_1));

                        subComponent->dependencies.add(dependency(single_field));
                        subComponent->dependencies.add(dependency(single_init_flag));
                        subComponent->dependencies.add(dependency(init_func));


                        List<operation_schema*> paramSchemea;
                        subComponent->scheme = new operation_schema();
                        create_static_function_call_operation(subComponent->scheme, paramSchemea, init_func);
                    } else {
                        // err
                    }
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
