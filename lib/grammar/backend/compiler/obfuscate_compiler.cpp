//
// Created by bknun on 9/1/2022.
//

#include "obfuscate_compiler.h"
#include "../types/types.h"

obfuscation_modifier process_modifier(Ast *ast) {
    if(ast->getTokenCount() > 0) {
        if(ast->hasToken("keep"))
            return modifier_keep;
        else if(ast->hasToken("keep_inclusive"))
            return modifier_keep_inclusive;
        else if(ast->hasToken("inclusive"))
            return modifier_obfuscate_inclusive;
        else {
            create_new_error(GENERIC, ast,
                             " unknown obfuscation modifier `" + ast->getToken(0).getValue() + "`.");
        }
    }

    return modifier_obfuscate;
}

void set_obfuscation_flag(obfuscation_modifier &obfuscate, obfuscation_modifier modifier) {
    obfuscate = modifier;
}

void obfuscate_class(sharp_class *sc, obfuscation_modifier modifier, Ast *ast) { // todo: if modifier is present for any type complain about overriding it
    set_obfuscation_flag(sc->obfuscateModifier, modifier);

    if(modifier == modifier_keep_inclusive || modifier == modifier_obfuscate_inclusive) {
        if(sc->blueprintClass) {
            GUARD(globalLock)

            for(Int i = 0; i < sc->genericClones.size(); i++) {
                obfuscate_class(sc->genericClones.get(i), modifier, ast);
            }
        } else {
            for (Int i = 0; i < sc->fields.size(); i++) {
                set_obfuscation_flag(sc->fields.get(i)->obfuscateModifier, modifier);
            }

            for (Int i = 0; i < sc->functions.size(); i++) {
                set_obfuscation_flag(sc->functions.get(i)->obfuscateModifier, modifier);
            }
        }
    } else {
        if(sc->blueprintClass) {
            // warn user
            create_new_warning(GENERIC, __w_general, ast, "attempt to obfuscate non-created generic class operation does nothing. Try "
                                                          "`obfuscate (-keep_inclusive | -inclusive) { " + sc->name + " }` this will add this setting to all classes created by the parent generic.");
        }
    }
}

void compile_obfuscation_element(obfuscation_modifier modifier, Ast *ast) {
    if(ast->getTokenCount() > 0) {
        sharp_module *module = get_module(ast->getToken(0).getValue());

        if(module != NULL) {
            set_obfuscation_flag(module->obfuscateModifier, modifier);
        } else {
            create_new_error(GENERIC, ast,
                             " cannot locate module `" + ast->getToken(0).getValue() + "` in obfuscation block.");
        }
    } else {
        sharp_type type;
        if(ast->hasSubAst(ast_function_signature)) {
            type = resolve(ast->getSubAst(ast_function_signature), resolve_unrestricted);
        } else {
            type = resolve(ast->getSubAst(ast_utype), resolve_unrestricted);
        }

        if(type == type_field) {
            sharp_field *field = type.field;
            set_obfuscation_flag(field->obfuscateModifier, modifier);
        } else if(type == type_function) {
            sharp_function *fun = type.fun;
            set_obfuscation_flag(fun->obfuscateModifier, modifier);
        } else if(type == type_class) {
            sharp_class *sc = type._class;
            obfuscate_class(sc, modifier, ast);
        } else if(type == type_function_ptr) {
            create_new_error(GENERIC, ast,
                             " invalid type found of `" + type_to_str(type) + "` in obfuscation block. Try explicitly declaring the function signature like so: `obfuscate { main(args: string[]) }`.");

        } else {
            create_new_error(GENERIC, ast,
                             " invalid type found of `" + type_to_str(type) + "` in obfuscation block. Only fields, functions, and classes are allowed.");
        }
    }
}

void compile_obfuscation_declaration(Ast *ast) {
    obfuscation_modifier modifier =
        process_modifier(ast);

    Ast *block = ast->getSubAst(ast_obfuscate_block);
    for(Int i = 0; i < block->getSubAstCount(); i++) {
        compile_obfuscation_element(modifier, block->getSubAst(i));
    }
}