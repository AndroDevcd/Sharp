//
// Created by BNunnally on 8/31/2021.
//

#include "ast_parser.h"
#include "../access_flag.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_class.h"
#include "../types/sharp_function.h"
#include "../../settings/settings.h"
#include "../types/sharp_field.h"

string concat_tokens(Ast *ast) {
    string data;
    for(long i = 0; i < ast->getTokenCount(); i++)
        data += ast->getToken(i).getValue();

    return data;
}

Int get_access_flag_count(uInt allowedFlags) {
    Int flagCount = 0;

    if(check_flag(allowedFlags, flag_public)
        || check_flag(allowedFlags, flag_private)
        || check_flag(allowedFlags, flag_protected)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_excuse)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_local)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_const)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_static)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_stable)
       || check_flag(allowedFlags, flag_unstable)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_extension)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_native)) {
        flagCount++;
    }

    if(check_flag(allowedFlags, flag_global)) {
        flagCount++;
    }

    return flagCount;
}

void parse_function_return_type(sharp_type &type, Ast *ast) {
    if(ast->hasToken("nil")) {
        type.type = type_nil;
    } else {
        parse_utype(type, ast->getSubAst(ast_utype));
    }
}

void parse_function_pointer(sharp_type &type, Ast *ast) {
    type.type = type_untyped;

    type.unresolvedType.items.add(new unresolved_item());
    unresolved_item &item = *type.unresolvedType.items.last();
    item.type = function_ptr_reference;

    parse_utype_arg_list_opt(item.typeSpecifiers, ast->getSubAst(ast_utype_arg_list_opt));

    if(ast->hasSubAst(ast_method_return_type)) {
        item.returnType = new sharp_type();

        parse_function_return_type(*item.returnType,
                ast->getSubAst(ast_method_return_type));
    }
}

void parse_type_identifier(sharp_type &type, Ast *ast) {
    if(ast->hasSubAst(ast_native_type)) {
        type.type = str_to_native_type(ast->getSubAst(ast_native_type)->getToken(0).getValue());
    } else if(ast->hasSubAst(ast_func_ptr)) {
        parse_function_pointer(type, ast->getSubAst(ast_func_ptr));
    } else {
        parse_reference_pointer(type, ast->getSubAst(ast_refrence_pointer));
    }
}

void parse_utype(sharp_type &type, Ast *ast) {
    type.type = type_untyped;
    if(ast->hasToken(QUESMK))
        type.nullable = true;

    if(ast->hasToken(LEFTBRACE) && ast->hasToken(RIGHTBRACE))
        type.isArray = true;

    parse_type_identifier(type, ast->getSubAst(ast_type_identifier));
}

void parse_utype_arg_list_opt(List<sharp_type*> &types, Ast *ast) {
    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        types.add(new sharp_type());
        parse_utype(*types.last(), ast->getSubAst(i)->getSubAst(ast_utype));
    }
}

void parse_utype_arg_list(List<sharp_field*> &types, Ast *ast) {
    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        sharp_type emptyType;

        impl_location location(currThread->currTask->file, ast->getSubAst(i));
        types.add(new sharp_field(
                ast->getSubAst(i)->getToken(0).getValue(),
                NULL, location,
                emptyType, flag_public,
                normal_field, ast->getSubAst(i)));
        parse_utype(types.last()->type, ast->getSubAst(i)->getSubAst(ast_utype));

    }
}

void parse_utype_list(List<sharp_type*> &types, Ast *ast) {
    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        types.add(new sharp_type());
        parse_utype(*types.last(), ast->getSubAst(i));
    }
}

void parse_normal_reference_item(unresolved_item &item, Ast *ast) {
    item.type = normal_reference;
    for(Int i = 0; i < ast->getTokenCount(); i++) {
        if(ast->getToken(i).getId() == IDENTIFIER) {
            item.name = ast->getToken(i).getValue();
        }

        if(parser::isOverrideOperator(ast->getToken(i).getValue())) {
            item.type = operator_reference;
            break;
        }
    }

    if(ast->getToken(0).getType() == SAFEDOT) {
        item.accessType = access_safe;
    } else if(ast->getToken(0).getType() == FORCEDOT) {
        item.accessType = access_forced;
    } else {
        item.accessType = access_normal;
    }
}

void parse_reference_item(unresolved_item &item, Ast *ast) {
    item.ast = ast;

    if(ast->getSubAst(ast_generic_reference)) {
        ast = ast->getSubAst(ast_generic_reference);

        parse_normal_reference_item(item, ast);
        item.type = generic_reference;

        parse_utype_list(item.typeSpecifiers, ast->getSubAst(ast_utype_list));
    } else if(ast->getSubAst(ast_module_reference)) {
        ast = ast->getSubAst(ast_module_reference);

        stringstream module;
        Ast *child;
        Token *token;

        for(Int i = 0; i < ast->getSubAstCount(); i++) {
            child = ast->getSubAst(i);

            if((token = child->getToken(SAFEDOT)) != NULL || (token = child->getToken(FORCEDOT)) != NULL) {
                currThread->currTask->file->errors->createNewError(
                        GENERIC, *token, "invalid module accessor `" + token->getValue() + "` did you mean to type `.`?");
                token->getValue() = ".";
            }

            for(Int j = 0; j < child->getSubAstCount(); j++)
                module << child->getToken(j).getValue();
        }

        item.type = module_reference;
        item.name = module.str();
    } else {
        parse_normal_reference_item(item, ast);
    }
}

void parse_reference_pointer(sharp_type &type, Ast *ast) {
    Ast *child = NULL;

    type.type = type_untyped;
    if(ast->hasSubAst(ast_operator_reference)) {
        child = ast->getSubAst(ast_operator_reference);

        string name = "operator" + child->getToken(0).getValue();
        type.unresolvedType.items.add(new unresolved_item(
                name, operator_reference, access_normal));
    } else {
        for(Int i = 0; i < ast->getSubAstCount(); i++) {
            type.unresolvedType.items.add(new unresolved_item());

            parse_reference_item(
                    *type.unresolvedType.items.last(),
                    ast->getSubAst(i));
        }
    }
}

string access_flags_to_str(uInt accessFlags) {
    stringstream ss;
    bool firstFlag = true;

    if(check_flag(accessFlags, flag_public)) {
        firstFlag = false;
        ss << "public";
    }

    if(check_flag(accessFlags, flag_private)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "private";
    }

    if(check_flag(accessFlags, flag_protected)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "protected";
    }

    if(check_flag(accessFlags, flag_excuse)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "excuse";
    }

    if(check_flag(accessFlags, flag_local)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "local";
    }

    if(check_flag(accessFlags, flag_const)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "const";
    }

    if(check_flag(accessFlags, flag_static)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "static";
    }

    if(check_flag(accessFlags, flag_stable)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "stable";
    }

    if(check_flag(accessFlags, flag_unstable)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "unstable";
    }

    if(check_flag(accessFlags, flag_extension)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "ext";
    }

    if(check_flag(accessFlags, flag_native)) {
        if(!firstFlag) ss << ", ";
        else firstFlag = false;

        ss << "native";
    }

    if(check_flag(accessFlags, flag_global)) {
        if(!firstFlag) ss << ", ";

        ss << "global";
    }

    return ss.str();
}

bool matches_flag(access_flag flagOrder[], Int len, Int start, access_flag flag) {
    for(Int i = start; i < len; i++) {
        if(flagOrder[i] == flag)
            return true;
    }

    return false;
}

uInt parse_access_flags(uInt allowedFlags, string memberType, sharp_class *memberOwner, Ast *ast) {
    uInt flags = flag_none, errPos = 0, start = 0;
    List<access_flag> flagOrder;
    access_flag flag = flag_none, last_flag = flag_none;
    uInt maxFlags = get_access_flag_count(allowedFlags);

    if(ast->getTokenCount() > maxFlags) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                "member type `" + memberType + "` only allows access "
                                               "specifiers (" + access_flags_to_str(allowedFlags) + ")");
    }

    for(Int i = 0; i < ast->getTokenCount(); i++) {
        flag = str_to_access_flag(ast->getToken(i).getValue());
        flagOrder.add(flag);
        flags |= flag;

        if(!check_flag(allowedFlags, flag)) {
            currThread->currTask->file->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                         " `" + ast->getToken(i).getValue() + "`");
        } else if(flag == last_flag) {
            currThread->currTask->file->errors->createNewError(ILLEGAL_ACCESS_DECLARATION, ast->line, ast->col,
                         " duplicate access flag `" + ast->getToken(i).getValue() + "`");
        }

        if(flag == flag_local && !(memberOwner == NULL || check_flag(memberOwner->flags, flag_global))) {
            currThread->currTask->file->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                       " `" + ast->getToken(i).getValue() + "` can only be used at global scope");
        }

        last_flag = flag;
    }

    if(flagOrder.size() > 1) {
        const uInt flagCount = 11;
        access_flag order[flagCount] = {
                    flag_public, flag_private, flag_protected,
                    flag_excuse, flag_local,
                    flag_static, flag_const,
                    flag_stable, flag_unstable,
                    flag_extension,
                    flag_native
        };

        if (flagOrder.get(0) <= flag_protected) {
            start = 3;
        } else if (flagOrder.get(0) == flag_excuse) {
            start = 4;
        } else if (flagOrder.get(0) == flag_local) {
            start = 5;
        } else if (flagOrder.get(0) == flag_static) {
            start = 6;
        } else if (flagOrder.get(0) == flag_const) {
            start = 7;
        } else if (flagOrder.get(0) == flag_stable) {
            start = 9;
        } else if (flagOrder.get(0) == flag_unstable) {
            start = 9;
        } else if (flagOrder.get(0) == flag_extension) {
            start = 10;
        } else if (flagOrder.get(0) == flag_native) {
            start = 10;
        }

        if (flagOrder.size() == 2) {
            if(!matches_flag(order, flagCount, start, flagOrder.get(1))) {
                errPos = 1;
                goto error;
            }
        } else if (flagOrder.size() == 3) {
            errPos = 1;
            if(matches_flag(order, flagCount, start, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+1, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 4) {
            errPos = 1;
            if(matches_flag(order, flagCount, start, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+1, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+2, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 5) {
            errPos = 1;
            if(matches_flag(order, flagCount, start, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+1, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+2, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+3, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 6) {
            errPos = 1;
            if(matches_flag(order, flagCount, start, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+1, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+2, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+3, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+5, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 7) {
            errPos = 1;
            if(matches_flag(order, flagCount, start, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+1, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+2, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+3, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+5, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+6, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 8) {
            errPos = 1;
            if(matches_flag(order, flagCount, start, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+1, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+2, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+3, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+5, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+6, flagOrder.get(errPos++))
               && matches_flag(order, flagCount, start+7, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else {
            errPos = flagOrder.size() - 1;
            goto error;
        }

        if (false) {
            error:
            currThread->currTask->file->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getToken(errPos).getLine(),
                                         ast->getToken(errPos).getColumn(),
                                         " `" + ast->getToken(errPos).getValue() + "`. Make sure the order is correct and you're using the appropriate modifiers.");
        }

        if ((flagOrder.find(flag_public) || flagOrder.find(flag_private)
            || flagOrder.find(flag_protected)) && flagOrder.find(flag_local)) {
            create_new_warning(GENERIC, __w_access, ast->line, ast->col,
                             "`local` access specifier trumps (public, private, and protected) flags");
        }
    }

    return flags;
}

access_flag str_to_access_flag(string& flag) {
    if(flag == "public")
        return flag_public;
    else if(flag == "private")
        return flag_private;
    else if(flag == "protected")
        return flag_protected;
    else if(flag == "local")
        return flag_local;
    else if(flag == "const")
        return flag_const;
    else if(flag == "static")
        return flag_static;
    else if(flag == "stable")
        return flag_stable;
    else if(flag == "unstable")
        return flag_unstable;
    else if(flag == "ext")
        return flag_extension;
    else if(flag == "native")
        return flag_native;
    else if(flag == "global")
        return flag_global;
    else
        return flag_none;
}
