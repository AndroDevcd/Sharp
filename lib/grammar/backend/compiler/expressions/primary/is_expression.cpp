//
// Created by BNunnally on 10/22/2021.
//

#include "is_expression.h"
#include "../../../types/types.h"
#include "../expression.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"

void compile_is_expression(expression *e, Ast *ast) {
    sharp_type castType = resolve(ast->getSubAst(ast_utype));
    compile_expression(*e, ast->getSubAst(ast_expression));

    // Two (non array) classes: `x is T` is known while compiling only when every
    // value x can hold is a T, i.e. x's class is T or derives from it
    // (`a_dog is animal`). Anything else depends on the object that is actually
    // in x - x declared as the base class and T derived from it
    // (`an_animal is cat`), or an interface - and is asked at run time (IS).
    //
    // The general test below treats two classes as matching when EITHER derives
    // from the other, so `an_animal is cat` was answered `true` right here.
    sharp_class *valueClass = get_class_type(e->type), *testClass = get_class_type(castType);
    if(valueClass != NULL && testClass != NULL
        && castType.type == type_class
        && !e->type.isArray && !castType.isArray) {
        if(is_class_related_to(valueClass, testClass)) {
            e->type.type = type_bool;
            e->type._bool = true;
            create_get_bool_constant_operation(&e->scheme, e->type._bool);
        } else {
            e->type.type = type_int8;
            create_is_operation(&e->scheme, &castType);
        }
        return;
    }

    if(is_explicit_type_match(castType, e->type)
        || is_implicit_type_match(castType, e->type, exclude_all)
        || (get_class_type(e->type) != NULL && get_class_type(castType) != NULL
            && is_implicit_type_match(e->type, castType, exclude_all))) {
        e->type.type = type_bool;
        e->type._bool = true;
        create_get_bool_constant_operation(&e->scheme, e->type._bool);
    } else {
        if(is_implicit_type_match(e->type, castType, exclude_all)) {
            e->type.type = type_int8;
            create_is_operation(&e->scheme, &castType);
        } else {
            e->type.type = type_bool;
            e->type._bool = false;
            create_get_bool_constant_operation(&e->scheme, e->type._bool);
        }
    }
}
