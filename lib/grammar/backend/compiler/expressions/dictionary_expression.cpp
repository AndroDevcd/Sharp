//
// Created by BNunnally on 11/23/2021.
//

#include "dictionary_expression.h"
#include "../../types/types.h"
#include "expression.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../compiler_info.h"

void compile_dictionary_expression(expression *e, Ast *ast) {
    sharp_type key, value;
    bool inferDictType = compile_dict_type(ast, &key, &value);
    List<KeyPair<expression*, expression*>> dictionaryItems;

    for(Int i = inferDictType ? 0 : 1; i < ast->getSubAstCount(); i++) {
        compile_dict_item(ast->getSubAst(i), &key, &value, inferDictType, dictionaryItems);
    }


    if(has_type(key) && has_type(value)) {
        for (Int i = 0; i < dictionaryItems.size(); i++) {
            if (!is_implicit_type_match(key, dictionaryItems.at(i).key->type,
                                        constructor_only)) {
                create_new_error(GENERIC, ast->line, ast->col,
                                                                   " expected dictionary key of type `" +
                                                                   type_to_str(key) +
                                                                   "` but found `" +
                                                                   type_to_str(dictionaryItems.at(i).key->type) + "`.");
            }

            if (!is_implicit_type_match(value, dictionaryItems.at(i).value->type,
                                        constructor_only)) {
                create_new_error(GENERIC, ast->line, ast->col,
                                                                   " expected dictionary value of type `" +
                                                                   type_to_str(value) +
                                                                   "` but found `" +
                                                                   type_to_str(dictionaryItems.at(i).value->type) +
                                                                   "`.");
            }
        }

        string name = "";
        string stdModule = "std";
        sharp_class *pairClassGeneric = resolve_class(get_module(stdModule), "pair", true, false), *pairClass = NULL;
        sharp_class *dictionaryClassGeneric = resolve_class(get_module(stdModule), "dictionary", true,
                                                            false), *dictionaryClass = NULL;

        if (pairClassGeneric != NULL && dictionaryClassGeneric != NULL) {
            List<sharp_type> pairTypes;
            pairTypes.__new().copy(key);
            pairTypes.__new().copy(value);

            pairClass = create_generic_class(pairTypes, pairClassGeneric, ast);
            create_dependency(pairClass);
            create_dependency(pairClass);

            sharp_type pairType(pairClass, false, false);
            dictionaryClass = create_generic_class(pairTypes, dictionaryClassGeneric, ast);
            create_dependency(dictionaryClass);
            create_dependency(dictionaryClass);

            if (pairClass != NULL && dictionaryClass != NULL) {
                operation_schema *arraySizeScheme = new operation_schema();
                operation_schema *arrayScheme = new operation_schema();
                arraySizeScheme->schemeType = scheme_get_constant;
                arraySizeScheme->steps.add(
                        new operation_step(operation_get_integer_constant, (Int) dictionaryItems.size()));

                create_new_class_array_operation(arrayScheme, arraySizeScheme, pairClass);

                // pairList[n] = new pain<k, v>(<key>, <value>) is what were doing below
                for (Int i = 0; i < dictionaryItems.size(); i++) {
                    operation_schema *pairItemScheme = new operation_schema();
                    operation_schema *resultScheme = new operation_schema();
                    expression *keyExpr = dictionaryItems.get(i).key;
                    expression *valExpr = dictionaryItems.get(i).value;
                    List<sharp_field *> params;
                    List<operation_schema *> paramOperations;
                    resultScheme->sc = pairClass;
                    pairItemScheme->schemeType = scheme_assign_array_value;

                    impl_location location;
                    params.add(new sharp_field(
                            name, get_primary_class(&currThread->currTask->file->context), location,
                            keyExpr->type, flag_public, normal_field,
                            ast
                    ));

                    params.add(new sharp_field(
                            name, get_primary_class(&currThread->currTask->file->context), location,
                            valExpr->type, flag_public, normal_field,
                            ast
                    ));

                    paramOperations.add(new operation_schema(keyExpr->scheme));
                    paramOperations.add(new operation_schema(valExpr->scheme));

                    sharp_function *constructor = resolve_function(get_simple_name(pairClass), pairClass,
                                                                   params, constructor_function,
                                                                   constructor_only,
                                                                   ast, false, true);

                    if (constructor != NULL) {
                        create_new_class_operation(resultScheme, pairClass);
                        create_duplicate_operation(resultScheme);

                        operation_schema *tmp = new operation_schema();
                        compile_function_call(tmp, params,
                                              paramOperations, constructor,
                                              false, false, false);

                        resultScheme->steps.add(
                                new operation_step(operation_step_scheme, tmp)
                        );
                        resultScheme->schemeType = scheme_new_class;
                        pairItemScheme->steps.add(new operation_step(operation_step_scheme, resultScheme));
                        create_dependency(constructor);
                    } else {
                        sharp_type returnType;
                        string mock = get_simple_name(pairClass);
                        sharp_function mock_function(mock, pairClass, impl_location(),
                                                     flag_none, NULL, params, returnType, undefined_function, true);
                        sharp_type unresolvedType(&mock_function);

                        create_new_error(GENERIC, ast->line, ast->col,
                                                                           "cannot find constructor for standard library class `" +
                                                                           pairClass->fullName + "`.");
                    }

                    create_push_to_stack_operation(pairItemScheme);
                    create_assign_array_element_operation(pairItemScheme, i, false);
                    arrayScheme->steps.add(new operation_step(operation_assign_array_value, pairItemScheme));

                    deleteList(params);
                    deleteList(paramOperations);
                }

                // new dictionary(pairList) represents below code
                List<sharp_field *> params;
                List<operation_schema *> paramOperations;
                sharp_type pairArrayType(pairClass, false, true);

                impl_location location;
                params.add(new sharp_field(
                        name, get_primary_class(&currThread->currTask->file->context), location,
                        pairArrayType, flag_public, normal_field,
                        ast
                ));

                paramOperations.add(new operation_schema(*arrayScheme));

                sharp_function *constructor = resolve_function(get_simple_name(dictionaryClass), dictionaryClass,
                                                               params, constructor_function,
                                                               constructor_only,
                                                               ast, false, true);

                if (constructor != NULL) {
                    create_new_class_operation(&e->scheme, dictionaryClass);
                    create_duplicate_operation(&e->scheme);

                    operation_schema *tmp = new operation_schema();
                    compile_function_call(tmp, params,
                                          paramOperations, constructor,
                                          false, false, false);

                    e->scheme.steps.add(
                            new operation_step(operation_step_scheme, tmp)
                    );
                    create_dependency(constructor);
                    e->type.type = type_class;
                    e->type._class = dictionaryClass;
                } else {
                    sharp_type returnType;
                    string mock = get_simple_name(dictionaryClass);
                    sharp_function mock_function(mock, dictionaryClass, impl_location(),
                                                 flag_none, NULL, params, returnType, undefined_function, true);
                    sharp_type unresolvedType(&mock_function);

                    create_new_error(GENERIC, ast->line, ast->col,
                                                                       "cannot find constructor `" +
                                                                       type_to_str(unresolvedType) + "` for class `" +
                                                                       dictionaryClass->fullName + "`.");
                }
            } else {
                create_new_error(INTERNAL_ERROR, ast->line, ast->col,
                                                                   " could not create generic classes `pair` and `dictionary`.");
            }

            freeList(pairTypes);
        } else {
            create_new_error(GENERIC, ast->line, ast->col,
                                                               " could not find standard library generic classes `pair` and `dictionary`.");

        }

        e->type.type = type_class;
        e->type._class = dictionaryClass;
    } else {
        create_new_error(GENERIC, ast->line, ast->col,
                      " could not determine the type of the dictionary, please ensure all key and values are typed.");
    }

    for (Int i = 0; i < dictionaryItems.size(); i++) {
        delete dictionaryItems.get(i).key;
        delete dictionaryItems.get(i).value;
    }
    dictionaryItems.free();
}

bool compile_dict_type(Ast *ast, sharp_type *key, sharp_type *value) {
    if(ast->hasSubAst(ast_dictionary_type)) {
        Ast *pairAst = ast->getSubAst(ast_dictionary_type);
        auto utype1 = resolve(pairAst->getSubAst(0));
        key->copy(utype1);

        auto utype2 = resolve(pairAst->getSubAst(1));
        value->copy(utype2);
        return false;
    } else {
        return true;
    }
}

void compile_dict_item(
        Ast *ast,
        sharp_type *key,
        sharp_type *value,
        bool inferDictType,
        List<KeyPair<expression*, expression*>> &items) {
    KeyPair<expression*, expression*> item;

    item.key = new expression();
    item.value = new expression();

    compile_expression(*item.key, ast->getSubAst(0));
    compile_dict_pair_item(item.key, inferDictType, key);

    compile_expression(*item.value, ast->getSubAst(1));
    compile_dict_pair_item(item.value, inferDictType, value);

    items.add(item);
}

void compile_dict_pair_item(expression *e, bool inferType, sharp_type *pairType) {
    convert_expression_type_to_real_type(*e);

    if(inferType) {
        if (!has_type(*pairType) && has_type(e->type)) {
            pairType->copy(e->type);
        } else if (has_type(e->type)) {
            if (!is_implicit_type_match(
                    *pairType, e->type,
                    constructor_only)) {
                if (is_implicit_type_match(
                        e->type, *pairType,
                        constructor_only)) {
                    pairType->copy(e->type);
                }
            }
        }
    }
}

