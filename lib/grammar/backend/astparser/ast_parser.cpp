//
// Created by BNunnally on 8/31/2021.
//

#include "ast_parser.h"
#include "../access_flag.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_class.h"
#include "../../settings/settings.h"

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
        last_flag = flag;

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
    }

    if(!flagOrder.empty()) {
        const uInt flagCount = 10;
        access_flag order[flagCount] = {
                    flag_public, flag_private, flag_protected,
                    flag_local,
                    flag_static, flag_const,
                    flag_stable, flag_unstable,
                    flag_extension,
                    flag_native
        };

        if (flagOrder.get(0) <= flag_protected) {
            start = 3;
        } else if (flagOrder.get(0) == flag_local) {
            start = 4;
        } else if (flagOrder.get(0) == flag_static) {
            start = 5;
        } else if (flagOrder.get(0) == flag_const) {
            start = 6;
        } else if (flagOrder.get(0) == flag_stable) {
            start = 8;
        } else if (flagOrder.get(0) == flag_unstable) {
            start = 8;
        } else if (flagOrder.get(0) == flag_extension) {
            start = 9;
        } else if (flagOrder.get(0) == flag_native) {
            start = 9;
        }

        if (flagOrder.size() == 2) {
            if(!matches_flag(order, 6, start, flagOrder.get(1))) {
                errPos = 1;
                goto error;
            }
        } else if (flagOrder.size() == 3) {
            errPos = 1;
            if(matches_flag(order, 6, start, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+1, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 4) {
            errPos = 1;
            if(matches_flag(order, 6, start, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+1, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+2, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 5) {
            errPos = 1;
            if(matches_flag(order, 6, start, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+1, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+2, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+3, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 6) {
            errPos = 1;
            if(matches_flag(order, 6, start, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+1, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+2, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+3, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+5, flagOrder.get(errPos++))) {
            } else {
                goto error;
            }
        } else if (flagOrder.size() == 7) {
            errPos = 1;
            if(matches_flag(order, 6, start, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+1, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+2, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+3, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+5, flagOrder.get(errPos++))
               && matches_flag(order, 6, start+6, flagOrder.get(errPos++))) {
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
