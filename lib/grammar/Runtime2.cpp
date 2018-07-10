//
// Created by BraxtonN on 6/11/2018.
//

#include <dirent.h>
#include <sys/stat.h>
#include "../../stdimports.h"
#ifdef POSIX_
#include <cmath>
#endif
#include "Runtime.h"
#include "parser/ErrorManager.h"
#include "List.h"
#include "parser/Parser.h"
#include "../util/File.h"
#include "../runtime/Opcode.h"
#include "../runtime/register.h"
#include "Asm.h"
#include "../runtime/Exe.h"
#include "Optimizer.h"
#include "../util/zip/zlib.h"
#include "../util/time.h"
#include "Method.h"

extern string globalClass;

/**
 * This is the second filr to our compiler
 * It has become so big that is is no longer useful be the code analyzer dute to oversized file
 */

string RuntimeEngine::getSwitchTable(Method* func, long addr) {
    stringstream ss;
    ss << " table [";

    SwitchTable &st = func->switchTable.get(addr);
    ss << "default: " << st.defaultAddress << " ";
    for(long i = 0; i < st.values.size(); i++) {
        ss << "case " << st.values.get(i) << ": @";
        ss << st.addresses.get(i);

        if((i+1) < st.values.get(i)) {
            ss << ", ";
        } else
            ss << " ";
    }
    ss << "]";
    return ss.str();
}


void RuntimeEngine::parseCharLiteral(token_entity token, Expression& expression) {
    expression.type = expression_var;
    expression.charLiteral = true;

    int64_t  i64;
    if(token.getToken().size() > 1) {
        switch(token.getToken().at(1)) {
            case 'n':
                expression.code.push_i64(SET_Di(i64, op_MOVI, '\n'), ebx);
                expression.value = '\n';
                break;
            case 't':
                expression.code.push_i64(SET_Di(i64, op_MOVI, '\t'), ebx);
                expression.intValue = '\t';
                break;
            case 'b':
                expression.code.push_i64(SET_Di(i64, op_MOVI, '\b'), ebx);
                expression.intValue = '\b';
                break;
            case 'v':
                expression.code.push_i64(SET_Di(i64, op_MOVI, '\v'), ebx);
                expression.intValue = '\v';
                break;
            case 'r':
                expression.code.push_i64(SET_Di(i64, op_MOVI, '\r'), ebx);
                expression.intValue = '\r';
                break;
            case 'f':
                expression.code.push_i64(SET_Di(i64, op_MOVI, '\f'), ebx);
                expression.intValue = '\f';
                break;
            case '\\':
                expression.code.push_i64(SET_Di(i64, op_MOVI, '\\'), ebx);
                expression.intValue = '\\';
                break;
            default:
                expression.code.push_i64(SET_Di(i64, op_MOVI, token.getToken().at(1)), ebx);
                expression.intValue = token.getToken().at(1);
                break;
        }
    } else {
        expression.code.push_i64(SET_Di(i64, op_MOVI, token.getToken().at(0)), ebx);
        expression.intValue =token.getToken().at(0);
    }

    expression.literal = true;
}

string RuntimeEngine::invalidateUnderscores(string basic_string) {
    stringstream newstring;
    for(char c : basic_string) {
        if(c != '_')
            newstring << c;
    }
    return newstring.str();
}


void RuntimeEngine::parseIntegerLiteral(token_entity token, Expression& expression) {
    expression.type = expression_var;

    int64_t i64;
    double var;
    string int_string = invalidateUnderscores(token.getToken());

    if(all_integers(int_string)) {
        var = std::strtod (int_string.c_str(), NULL);
        if(var > DA_MAX || var < DA_MIN) {
            stringstream ss;
            ss << "integral number too large: " + int_string;
            errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
        }
        expression.code.push_i64(SET_Di(i64, op_MOVI, var), ebx);
    }else {
        var = std::strtod (int_string.c_str(), NULL);
        if((int64_t )var > DA_MAX || (int64_t )var < DA_MIN) {
            stringstream ss;
            ss << "integral number too large: " + int_string;
            errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
        }

        expression.code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)var)), abs(get_low_bytes(var)));
        expression.code.push_i64(SET_Ci(i64, op_MOVR, ebx,0, bmr));
    }

    expression.intValue = var;
    expression.literal = true;
}

void RuntimeEngine::parseHexLiteral(token_entity token, Expression& expression) {
    expression.type = expression_var;

    int64_t i64;
    double var;
    string hex_string = invalidateUnderscores(token.getToken());

    var = strtoll(hex_string.c_str(), NULL, 16);
    if(var > DA_MAX || var < DA_MIN) {
        stringstream ss;
        ss << "integral number too large: " + hex_string;
        errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
    }
    expression.code.push_i64(SET_Di(i64, op_MOVI, var), ebx);

    expression.intValue = var;
    expression.literal = true;
}

void RuntimeEngine::parseStringLiteral(token_entity token, Expression& expression) {
    expression.type = expression_string;

    string parsed_string = "";
    bool slash = false;
    long i = 0;
    for(char c : token.getToken()) {
        parsed_string += c;

        i++;
    }

    expression.value = parsed_string;
    stringMap.addif(parsed_string);
    expression.intValue = stringMap.indexof(parsed_string);
}

void RuntimeEngine::parseBoolLiteral(token_entity token, Expression& expression) {
    expression.type = expression_var;
    expression.code.push_i64(SET_Di(i64, op_MOVI, (token.getToken() == "true" ? 1 : 0)), ebx);

    expression.intValue = (token.getToken() == "true" ? 1 : 0);
    expression.literal = true;
}

Expression RuntimeEngine::parseLiteral(Ast* pAst) {
    Expression expression(pAst);

    switch(pAst->getEntity(0).getId()) {
        case CHAR_LITERAL:
            parseCharLiteral(pAst->getEntity(0), expression);
            break;
        case INTEGER_LITERAL:
            parseIntegerLiteral(pAst->getEntity(0), expression);
            break;
        case HEX_LITERAL:
            parseHexLiteral(pAst->getEntity(0), expression);
            break;
        case STRING_LITERAL:
            parseStringLiteral(pAst->getEntity(0), expression);
            break;
        default:
            if(pAst->getEntity(0).getToken() == "true" ||
               pAst->getEntity(0).getToken() == "false") {
                parseBoolLiteral(pAst->getEntity(0), expression);
            }
            break;
    }
    expression.link = pAst;
    return expression;
}

Expression RuntimeEngine::psrseUtypeClass(Ast* pAst) {
    Expression expression = parseUtype(pAst);

    if(pAst->hasEntity(DOT)) {
        expression.dot = true;
    }

    if(expression.type == expression_class) {
        expression.code.push_i64(SET_Di(i64, op_MOVI, expression.utype.klass->address), ebx);
        expression.type = expression_var;
        expression.literal = true;
        expression.intValue=expression.utype.klass->address;
    } else {
        errors->createNewError(GENERIC, pAst->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->line,
                               pAst->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->col, "expected class");
    }
    expression.link = pAst;
    return expression;
}

List<ClassObject*> RuntimeEngine::parseRefrenceIdentifierList(Ast *ast) {
    List<ClassObject*> classes;

    ClassObject *klass;
    Ast* vAst;
    for(unsigned int i = 0; i < ast->getSubAstCount(); i++) {
        vAst = ast->getSubAst(i);

        ReferencePointer ptr = parseReferencePtr(vAst, false);
        klass = resolveClassRefrence(vAst, ptr);

        if(klass != NULL)
            classes.push_back(klass);
    }

    return classes;
}

List<Expression> RuntimeEngine::parseValueList(Ast* pAst) {
    List<Expression> expressions;

    Ast* vAst;
    for(unsigned int i = 0; i < pAst->getSubAstCount(); i++) {
        vAst = pAst->getSubAst(i);
        expressions.add(parseValue(vAst));
    }

    return expressions;
}

bool RuntimeEngine::expressionListToParams(List<Param> &params, List<Expression>& expressions) {
    bool success = true;
    Expression* expression;
    List<AccessModifier> mods;
    RuntimeNote note;
    Field field;

    for(unsigned int i = 0; i < expressions.size(); i++) {
        expression = &expressions.get(i);
        if(expression->link != NULL) {
            note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(expression->link->line),
                               expression->link->line, expression->link->col);
        }

        if(expression->type == expression_var) {
            field = Field(VAR, 0, "", NULL, mods, note);
            field.isArray = expression->utype.array;

            params.add(Param(field));
        } else if(expression->type == expression_string) {
            field = Field(VAR, 0, "", NULL, mods, note);
            field.isArray = true;

            /* Native string is a char array */
            params.add(Param(field));
        }  else if(expression->type == expression_prototype) {
            field = Field(VAR, 0, "", NULL, mods, note);
            field.prototype = true;
            field.proto=expression->utype.method;

            /* Native string is a char array */
            params.add(Param(field));
        } else if(expression->type == expression_class) {
            success = false;
            errors->createNewError(INVALID_PARAM, expression->link->line, expression->link->col, " `class`, param must be lvalue");
        } else if(expression->type == expression_lclass) {
            field = Field(expression->utype.klass, 0, "", NULL, mods, note);
            field.type = CLASS;
            field.isArray = expression->isArray();

            params.add(Param(field));
        } else if(expression->type == expression_field) {
            params.add(*expression->utype.field);
        } else if(expression->type == expression_native) {
            success = false;
            errors->createNewError(GENERIC, expression->link->line, expression->link->col, " unexpected symbol `" +
                                                                                           expression->utype.referenceName + "`");
        } else if(expression->type == expression_objectclass) {
            field = Field(OBJECT, 0, "", NULL, mods, note);
            field.isArray = expression->isArray();

            params.add(field);
        } else if(expression->type == expression_void) {
            success = false;
            errors->createNewError(GENERIC, expression->link->line, expression->link->col, " expression of type `void` cannot be a param");
        }
        else if(expression->type == expression_null) {
            field = Field(OBJECT, 0, "", NULL, mods, note);
            field.nullType = true;

            params.add(Param(field));
        } else {
            /* Unknown expression */
            field = Field(UNDEFINED, 0, "", NULL, mods, note);

            params.add(Param(field));
            success = false;
        }
    }

    return success;
}

/**
 * This gets the name from the dotted refrence
 * Main.foo foo will be split from the dot notation to lookup the
 * classes before it
 * @param name
 * @param ptr
 * @return
 */
bool RuntimeEngine::splitMethodUtype(string& name, ReferencePointer& ptr) {
    if(ptr.singleRefrence() || ptr.singleRefrenceModule()) {
        return false;
    } else {
        name = ptr.referenceName;
        ptr.referenceName = ptr.classHeiarchy.last(); // assign the last field in the accessor to class
        ptr.classHeiarchy.remove(ptr.classHeiarchy.size()-1);
        return true;
    }
}

string RuntimeEngine::paramsToString(List<Param> &param) {
    string message = "(";
    for(unsigned int i = 0; i < param.size(); i++) {
        switch(param.get(i).field.type) {
            case VAR:
                if(param.get(i).field.prototype) {
                    if(param.get(i).field.proto != NULL) {
                        message += "fn*" + paramsToString(param.get(i).field.proto->getParams());
                    } else
                        message += "fn*" + paramsToString(param.get(i).field.params);
                } else
                    message += "var";
                break;
            case CLASS:
                if(param.get(i).field.nullType) {
                    if(param.get(i).field.klass == NULL)
                        message += "<class-type>";
                    else
                        message += param.get(i).field.klass->getName();
                } else message += param.get(i).field.klass->getFullName();
                break;
            case OBJECT:
                message += "object";
                break;
            default:
                message += "?";
                break;
        }

        if(param.get(i).field.isArray)
            message += "[]";
        if((i+1) < param.size()) {
            message += ",";
        }
    }

    message += ")";
    return message;
}

void RuntimeEngine::pushAuthenticExpressionToStackNoInject(Expression& expression, Expression& out) {

    switch(expression.type) {
        case expression_var:
            if(expression.newExpression) {
            } else {
                if (expression.func) {
                    // out.code.push_i64(SET_Di(i64, op_INC, sp));
                } else
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
            }
            break;
        case expression_field:
            if(expression.utype.field->isVar() && !expression.utype.field->isArray) {
                if(expression.utype.field->local) {
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
                } else {
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                    out.code.push_i64(SET_Ci(i64, op_IALOAD_2, ebx,0, adx));
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
                }
            } else if(expression.utype.field->isVar() && expression.utype.field->isArray) {
                if (expression.func) {
                } else
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            } else if(expression.utype.field->dynamicObject() || expression.utype.field->type == CLASS) {
                if (expression.func) {
                } else
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
            break;
        case expression_lclass:
            if(expression.newExpression) {
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                } else {
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
                }
            }
            break;
        case expression_string:
            out.code.push_i64(SET_Di(i64, op_NEWSTRING, expression.intValue));
            break;
        case expression_null:
            out.code.push_i64(SET_Ei(i64, op_PUSHNIL));
            break;
        case expression_objectclass:
            if(expression.newExpression) {
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                    // out.code.push_i64(SET_Di(i64, op_INC, sp));
                } else {
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
                }
            }
            break;
    }
}

void RuntimeEngine::pushExpressionToPtr(Expression& expression, Expression& out, bool check) {
    if(!expression.newExpression)
        out.code.inject(out.code.__asm64.size(), expression.code);

    if(check)
        isMemoryObject(expression, expression.link);

    switch(expression.type) {
        case expression_var:
            if(expression.newExpression) {
                out.code.inject(out.code.__asm64.size(), expression.code);
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            }

            if(expression.func) {
                out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            }
            break;
        case expression_field:
            break;
        case expression_lclass:
            if(expression.newExpression) {
                out.code.inject(out.code.__asm64.size(), expression.code);
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                    out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                }
            }
            break;
        case expression_string:
            out.code.push_i64(SET_Di(i64, op_MOVSL, 1));
            out.code.push_i64(SET_Di(i64, op_NEWSTRING, expression.intValue));
            break;
        case expression_null:
            out.code.push_i64(SET_Di(i64, op_MOVSL, 1));
            out.code.push_i64(SET_Ei(i64, op_DEL));
            break;
        case expression_objectclass:
            if(expression.newExpression) {
                out.code.inject(out.code.__asm64.size(), expression.code);
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                    out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                }
            }
            break;
    }
}

void RuntimeEngine::pushExpressionToStack(Expression& expression, Expression& out) {
    if(!expression.newExpression)
        out.code.inject(out.code.__asm64.size(), expression.code);

    switch(expression.type) {
        case expression_var:
            if(expression.newExpression) {
                out.code.inject(out.code.__asm64.size(), expression.code);
            } else {
                if (expression.func) {
                } else {
                    if(expression.isArray())
                        out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
                    else
                        out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
                }
            }
            break;
        case expression_prototype:
            out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
            break;
        case expression_field:
            if(expression.utype.field->isVar() && !expression.utype.field->isArray) {
                if(expression.utype.field->local) {
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
                } else {
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                    out.code.push_i64(SET_Ci(i64, op_IALOAD_2, ebx,0, adx));
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
                }
            } else if((expression.trueType() == VAR && expression.isArray()) ||
                      expression.trueType() == OBJECT || expression.trueType() == CLASS) {
                out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
            break;
        case expression_lclass:
            if(expression.newExpression) {
                out.code.inject(out.code.__asm64.size(), expression.code);
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                } else {
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
                }
            }
            break;
        case expression_string:
            out.code.push_i64(SET_Di(i64, op_NEWSTRING, expression.intValue));
            break;
        case expression_null:
            out.code.push_i64(SET_Ei(i64, op_PUSHNIL));
            break;
        case expression_objectclass:
            if(expression.newExpression) {
                out.code.inject(out.code.__asm64.size(), expression.code);
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                } else {
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
                }
            }
            break;
    }
}

Method* RuntimeEngine::resolveMethodUtype(Ast* utype, Ast* valueLst, Expression &out) {
    Method* fn = NULL;
    utype = utype->getSubAst(ast_utype);
    valueLst = valueLst->getSubAst(ast_value_list);

    /* This is a naked utype so we dont haave to worry about brackets */
    ReferencePointer ptr;
    string methodName;
    List<Param> params;
    List<Expression> expressions = parseValueList(valueLst);
    Expression expression(utype);
    bool access = false, staticCall = false;

    expressionListToParams(params, expressions);
    ptr = parseTypeIdentifier(utype);

    if(splitMethodUtype(methodName, ptr)) {
        access = true;
        // accessor
        resolveUtype(ptr, expression, utype);
        if(expression.type == expression_class || (expression.type == expression_field && expression.utype.field->type == CLASS)) {
            ClassObject* klass;
            if(expression.type == expression_class) {
                klass = expression.utype.klass;
                staticCall = true;
            } else {
                klass = expression.utype.field->klass;
            }

            if((fn = klass->getFunction(methodName, params, true, true)) != NULL){}
            else if((fn = klass->getOverload(stringToOp(methodName), params, true, true)) != NULL){}
            else if(methodName == klass->getName() && (fn = klass->getConstructor(params, false, true)) != NULL) {}
            else if(klass->getField(methodName, true) != NULL && !klass->getField(methodName, true)->prototype) {
                errors->createNewError(GENERIC, valueLst->line, valueLst->col, " symbol `" + methodName + "` is a field");
            }
            else {
                Field *f;
                if((f = klass->getField(methodName, true)) != NULL) {
                    if(f->prototype) {
                        Expression e, tmp;

                        if(staticCall || f->isStatic()) {
                            e.code.push_i64(SET_Di(i64, op_MOVG, klass->address));
                        } else {
                            e.inject(expression);
                        }
                        e.code.push_i64(SET_Di(i64, op_MOVN, f->address));
                        expression.code.free();

                        tmp = fieldToExpression(NULL, *f); tmp.code.free();
                        pushExpressionToRegister(tmp, e, ebx);
                        fn = fieldToFunction(f, e);
                        goto funcFound;
                    }
                }
                if(stringToOp(methodName) != oper_UNDEFINED) methodName = "operator" + methodName;

                errors->createNewError(COULD_NOT_RESOLVE, valueLst->line, valueLst->col, " `" + methodName + paramsToString(params) + "`");
            }
        } else if(expression.type == expression_field && expression.utype.field->type != CLASS) {
            errors->createNewError(GENERIC, expression.link->line, expression.link->col, " field `" +  expression.utype.field->name + "` is not a class");

        } else if(expression.utype.type == UNDEFINED) {
        }
        else {
            errors->createNewError(COULD_NOT_RESOLVE, valueLst->line, valueLst->col, " `" +  methodName + paramsToString(params) +  "`");
        }
    } else {
        // method or global macros
        if(ptr.singleRefrence()) {
            if(currentScope()->type == GLOBAL_SCOPE) {
                errors->createNewError(COULD_NOT_RESOLVE, valueLst->line, valueLst->col, " `" + ptr.referenceName +  paramsToString(params) + "`");
            } else {

                if((fn = currentScope()->klass->getFunction(ptr.referenceName, params, true, true)) != NULL){}
                else if((fn = currentScope()->klass->getOverload(stringToOp(ptr.referenceName), params, true, true)) != NULL){}
                else if(ptr.referenceName == currentScope()->klass->getName() && (fn = currentScope()->klass->getConstructor(params)) != NULL) {}
                else if(currentScope()->klass->getField(methodName, true) != NULL && !currentScope()->klass->getField(methodName, true)->prototype) {
                    errors->createNewError(GENERIC, valueLst->line, valueLst->col, " symbol `" + methodName + "` is a field");
                }
                else {
                    if(currentScope()->klass->hasBaseClass(getClass("std", "Enum", classes))) {
                        Scope *scope = &scopeMap.get(scopeMap.size()-2);
                        if((fn = scope->klass->getFunction(ptr.referenceName, params, true, true)) != NULL){}
                        else if((fn = scope->klass->getOverload(stringToOp(ptr.referenceName), params, true, true)) != NULL){}
                        else if(ptr.referenceName == scope->klass->getName() && (fn = scope->klass->getConstructor(params, false, true)) != NULL) {}
                        else if(scope->klass->getField(methodName, true) != NULL) {
                            errors->createNewError(GENERIC, valueLst->line, valueLst->col, " symbol `" + methodName + "` is a field");
                        }

                        if(fn != NULL) goto funcFound;
                    } else {
                        ClassObject *global = getClass("global", globalClass, classes);
                        KeyPair<int, Field> *field;
                        Field *f;
                        if((fn = global->getFunction(ptr.referenceName, params)) != NULL) {
                            goto funcFound;
                        } else if((field = currentScope()->getLocalField(ptr.referenceName)) != NULL) {
                            f = &field->value;
                            if(f->prototype) {
                                Expression e = fieldToExpression(NULL, *f);
                                fn = fieldToFunction(f, e);
                                goto funcFound;
                            }
                        } else if((f = currentScope()->klass->getField(ptr.referenceName, true)) != NULL) {
                            if(f->prototype) {
                                Expression e, tmp;
                                e.code.push_i64(SET_Di(i64, op_MOVL, 0));
                                tmp = fieldToExpression(NULL, *f); tmp.code.free();
                                pushExpressionToRegister(tmp, e, ebx);
                                fn = fieldToFunction(f, e);
                                goto funcFound;
                            }
                        }
                    }
                    if(stringToOp(methodName) != oper_UNDEFINED) methodName = "operator" + ptr.referenceName;
                    else methodName = ptr.referenceName;
                    errors->createNewError(COULD_NOT_RESOLVE, valueLst->line, valueLst->col, " `" + ptr.referenceName +  paramsToString(params) + "`");
                }
            }

            funcFound:
            if(fn != NULL && !fn->isStatic()) {
                expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
            }
        } else {
            errors->createNewError(COULD_NOT_RESOLVE, valueLst->line, valueLst->col, " `" + ptr.referenceName +  paramsToString(params) + "`");
        }
    }


    if(fn != NULL) {
        if(!access && currentScope()->type == STATIC_BLOCK && !fn->isStatic()) {
            errors->createNewError(GENERIC, valueLst->line, valueLst->col, " call to instance function `" + fn->getFullName() +  paramsToString(params) + "` inside static block");
        }

        if(staticCall && !fn->isStatic()) {
            errors->createNewError(GENERIC, valueLst->line, valueLst->col, " call to instance function `" + fn->getFullName() +  paramsToString(params) + "` via static context");
        }

        verifyMethodAccess(fn, valueLst);
        if(!fn->isStatic()) {
            if(access && currentScope()->last_statement == ast_return_stmnt) {
                out.inject(expression);
                pushAuthenticExpressionToStackNoInject(expression, out);
            } else {

                pushExpressionToPtr(expression, out);
                out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
        }

        for(unsigned int i = 0; i < expressions.size(); i++) {
            if(fn->getParam(i).field.dynamicObject() && expressions.get(i).type == expression_var) {
                pushExpressionToRegister(expressions.get(i), out, ebx);

                out.code.push_i64(SET_Di(i64, op_MOVI, 1), egx);
                out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                out.code.push_i64(SET_Di(i64, op_NEWARRAY, egx));
                out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                out.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
            } else if(isExpressionConvertableToNativeClass(&fn->getParam(i).field, expressions.get(i))) {
                ClassObject *k = fn->getParam(i).field.klass;
                Expression fExpr = fieldToExpression(utype, fn->getParam(i).field);
                constructNewNativeClass(k->getName(), k->getModuleName(), expressions.get(i), out, false);
            }
            else
                pushExpressionToStack(expressions.get(i), out);
        }

        if(fn->delegatePost) {
            if(fn->isStatic()) {
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE_STATIC, fn->address, 0, expressions.size()), fn->klass->address);
            } else
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE, fn->address, 0, expressions.size()));
        } else {
            if(!fn->dynamicPointer)
                out.code.push_i64(SET_Di(i64, op_CALL, fn->address));
            else {
                out.code.inject(out.code.size(), fn->code);
                out.code.push_i64(SET_Di(i64, op_CALLD, ebx));
                fn->free();
                free(fn);
            }
        }
    }

    freeList(params);
    freeList(expressions);
    ptr.free();
    return fn;
}

expression_type RuntimeEngine::methodReturntypeToExpressionType(Method* fn) {
    if(fn->type == CLASS)
        return expression_lclass;
    else if(fn->type == OBJECT) {
        return expression_objectclass;
    } else if(fn->type == VAR) {
        return expression_var;
    } else if(fn->type == TYPEVOID)
        return expression_void;
    else
        return expression_unknown;
}

void RuntimeEngine::resolveUtypeContext(ClassObject* classContext, ReferencePointer& refrence, Expression& expression, Ast* pAst) {
    int64_t i64;
    Field* field;

    if(refrence.singleRefrence()) {
        if((field = classContext->getField(refrence.referenceName)) != NULL) {

            expression.utype.type = CLASSFIELD;
            expression.utype.field = field;
            expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));
            expression.type = expression_field;
        }else {
            /* Un resolvable */
            errors->createNewError(COULD_NOT_RESOLVE, pAst->line, pAst->col, " `" + refrence.referenceName + "` " + (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));

            expression.utype.type = UNDEFINED;
            expression.utype.referenceName = refrence.referenceName;
            expression.type = expression_unresolved;
        }
    } else {
        resolveClassHeiarchy(classContext, refrence, expression, pAst);
    }
}

void RuntimeEngine::pushExpressionToStackNoInject(Expression& expression, Expression& out) {

    switch(expression.type) {
        case expression_var:
            if(expression.newExpression) {
            } else {
                if (expression.func) {
                } else
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
            }
            break;
        case expression_field:
            if(expression.utype.field->isVar() && !expression.utype.field->isArray) {
                if(expression.utype.field->local) {
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
                } else {
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                    out.code.push_i64(SET_Ci(i64, op_IALOAD_2, ebx,0, adx));
                    out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));
                }
            } else if(expression.utype.field->isVar() && expression.utype.field->isArray) {
                if (expression.func) {
                } else
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            } else if(expression.utype.field->dynamicObject() || expression.utype.field->type == CLASS) {
                if (expression.func) {
                } else
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
            break;
        case expression_lclass:
            if(expression.newExpression) {
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                } else {
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
                }
            }
            break;
        case expression_string:
            out.code.push_i64(SET_Di(i64, op_NEWSTRING, expression.intValue));
            break;
        case expression_null:
            out.code.push_i64(SET_Ei(i64, op_PUSHNIL));
            break;
        case expression_objectclass:
            if(expression.newExpression) {
            } else {
                if(expression.func) {
                    /* I think we do nothing? */
                } else {
                    out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
                }
            }
            break;
    }
}


Method* RuntimeEngine::resolveContextMethodUtype(ClassObject* classContext, Ast* pAst, Ast* pAst2, Expression& out, Expression& contextExpression) {
    Method* fn = NULL;

    /* This is a naked utype so we dont haave to worry about brackets */
    ReferencePointer ptr;
    string methodName = "";
    List<Param> params;
    List<Expression> expressions = parseValueList(pAst2);
    Expression expression(pAst);

    expressionListToParams(params, expressions);
    ptr = parseTypeIdentifier(pAst);

    if(ptr.module != "") {
        errors->createNewError(GENERIC, pAst->line, pAst->col, "module name not allowed in nested method call");
        return NULL;
    }

    if(splitMethodUtype(methodName, ptr)) {
        // accessor
        resolveUtypeContext(classContext, ptr, expression, pAst);
        if(expression.type == expression_class || (expression.type == expression_field && expression.utype.field->type == CLASS)) {
            ClassObject* klass;
            if(expression.type == expression_class) {
                klass = expression.utype.klass;
            } else {
                klass = expression.utype.field->klass;
            }

            if((fn = klass->getFunction(methodName, params, true, true)) != NULL){}
            else if((fn = klass->getOverload(stringToOp(methodName), params, true, true)) != NULL){}
            else if(classContext->getField(methodName, true) != NULL) {
                errors->createNewError(GENERIC, pAst2->line, pAst2->col, " symbol `" + classContext->getFullName() + methodName + "` is a field");
            }
            else {
                if(stringToOp(methodName) != oper_UNDEFINED) methodName = "operator" + methodName;

                errors->createNewError(COULD_NOT_RESOLVE, pAst2->line, pAst2->col, " `" + classContext->getFullName() + "."
                                                                                   + (klass == NULL ? ptr.referenceName : methodName) + paramsToString(params) + "`");
            }
        }
        else if(expression.type == expression_field && expression.utype.field->type != CLASS) {
            errors->createNewError(GENERIC, pAst2->line, pAst2->col, " field `" + (expression.utype.klass == NULL ? ptr.referenceName : methodName) + "` is not a class");

        }
        else if(expression.utype.type == UNDEFINED) {
        }
        else {
            errors->createNewError(COULD_NOT_RESOLVE, pAst2->line, pAst2->col, " `" + classContext->getFullName() + "." + (expression.utype.klass == NULL ? ptr.referenceName : methodName) + "`");
        }
    } else {
        // method or global macros
        if(ptr.singleRefrence()) {
            if((fn = classContext->getFunction(ptr.referenceName, params, true, true)) != NULL){}
            else if((fn = classContext->getOverload(stringToOp(ptr.referenceName), params, true, true)) != NULL){}
            else if(classContext->getField(ptr.referenceName, true) != NULL) {
                errors->createNewError(GENERIC, pAst2->line, pAst2->col, " symbol `" + ptr.referenceName + "` is a field");
            }
            else {
                if(stringToOp(ptr.referenceName) != oper_UNDEFINED) methodName = "operator" + ptr.referenceName;
                else methodName = ptr.referenceName;

                errors->createNewError(COULD_NOT_RESOLVE, pAst2->line, pAst2->col, " `" + classContext->getFullName() + "." + methodName +  paramsToString(params) + "`");
            }
        } else {
            // must be macros but it can be
            errors->createNewError(COULD_NOT_RESOLVE, pAst2->line, pAst2->col, " `" + ptr.referenceName + "`");
        }
    }


    if(fn != NULL) {
        if(fn->isStatic()) {
            createNewWarning(GENERIC, pAst->line, pAst->col, "instance access to static function");
        }
        verifyMethodAccess(fn, pAst);

        if(!fn->isStatic()) {
            pushAuthenticExpressionToStackNoInject(contextExpression, out);
        }

        for(unsigned int i = 0; i < expressions.size(); i++) {
            if(fn->getParam(i).field.dynamicObject() && expressions.get(i).type == expression_var) {
                pushExpressionToRegister(expressions.get(i), out, ebx);

                out.code.push_i64(SET_Di(i64, op_MOVI, 1), egx);
                out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                out.code.push_i64(SET_Di(i64, op_NEWARRAY, egx));
                out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                out.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
            } else if(isExpressionConvertableToNativeClass(&fn->getParam(i).field, expressions.get(i))) {
                ClassObject *k = fn->getParam(i).field.klass;
                Expression fExpr = fieldToExpression(pAst, fn->getParam(i).field);
                constructNewNativeClass(k->getName(), k->getModuleName(), expressions.get(i), out, false);
            } else
                pushExpressionToStack(expressions.get(i), out);
        }

        if(fn->delegatePost) {
            if(fn->isStatic()) {
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE_STATIC, fn->address, 0, expressions.size()), fn->klass->address);
            } else
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE, fn->address, 0, expressions.size()));
        } else
            out.code.push_i64(SET_Di(i64, op_CALL, fn->address));
    }

    freeList(params);
    freeList(expressions);
    ptr.free();
    return fn;
}

Expression RuntimeEngine::parseUtypeContext(ClassObject* classContext, Ast* pAst) {
    ReferencePointer ptr=parseTypeIdentifier(pAst);
    Expression expression(pAst);

    if(ptr.singleRefrence() && Parser::isnative_type(ptr.referenceName)) {
        errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + ptr.referenceName + "`");

        expression.type = expression_unresolved;
        expression.link = pAst;
        ptr.free();
        return expression;
    }

    resolveUtypeContext(classContext, ptr, expression, pAst);

    if(pAst->hasEntity(LEFTBRACE) && pAst->hasEntity(RIGHTBRACE)) {
        expression.utype.array = true;
    }

    expression.link = pAst;
    expression.utype.referenceName = ptr.toString();
    ptr.free();
    return expression;
}

Expression RuntimeEngine::parseDotNotationCallContext(Expression& contextExpression, Ast* pAst) {
    string method_name="";
    Expression expression(pAst), interm(pAst);
    Method* fn;

    if(contextExpression.type == expression_objectclass) {
        errors->createNewError(GENERIC, pAst->line, pAst->col, "cannot infer function from object, did you forget to add a cast? i.e. ((SomeClass)dynamic_class)");
        expression.type = expression_unresolved;
        return expression;
    } else if(contextExpression.type == expression_unresolved) {
        expression.type= expression_unresolved;
        return expression;
    } else if(contextExpression.type != expression_lclass) {
        errors->createNewError(GENERIC, pAst->line, pAst->col, "expression does not return a class");
        expression.type = expression_unresolved;
        return expression;
    }

    ClassObject* klass = contextExpression.utype.klass;

    if(pAst->getType() == ast_dot_fn_e) {
        fn = resolveContextMethodUtype(klass, pAst->getSubAst(ast_utype),
                                       pAst->getSubAst(ast_value_list), expression, contextExpression);
        if(fn != NULL) {
            expression.type = methodReturntypeToExpressionType(fn);
            if(expression.type == expression_lclass)
                expression.utype.klass = fn->klass;
            expression.utype.array = fn->array;

            // TODO: check for ++ and --
            // TODO: parse dot_notation_chain expression
        } else
            expression.type = expression_unresolved;
        expression.func=true;
    } else {

        /*
         * Nasty way to check which ast to pass
         * can we do this a better way?
         */
        if(pAst->getType() == ast_utype)
            expression = parseUtypeContext(klass, pAst);
        else
            expression = parseUtypeContext(klass, pAst->getSubAst(ast_utype));

        if(contextExpression.func)
            expression.code.injecti64(0, SET_Di(i64, op_MOVSL, 0));
        if(expression.trueType() == VAR && !expression.isArray()) {
            pushExpressionToRegisterNoInject(expression, expression, ebx);
        }
    }

    if(pAst->hasEntity(DOT)) {
        expression.dot = true;
    }

    expression.link = pAst;
    return expression;
}

bool RuntimeEngine::currentRefrenceAffected(Expression& expr) {
    int opcode;
    for(unsigned int i = 0; i < expr.code.size(); i++) {
        opcode = GET_OP(expr.code.__asm64.get(i));

        switch(opcode) {
            case op_MOVL:
            case op_MOVSL:
            case op_MOVN:
            case op_MOVG:
            case op_MOVND:
                return true;
            default:
                break;
        }
    }

    return false;
}

void RuntimeEngine::pushExpressionToRegister(Expression& expr, Expression& out, int reg) {
    out.code.inject(out.code.__asm64.size(), expr.code);
    pushExpressionToRegisterNoInject(expr, out, reg);
}

void RuntimeEngine::pushExpressionToRegisterNoInject(Expression& expr, Expression& out, int reg) {
    switch(expr.type) {
        case expression_var:
        case expression_prototype:
            if(expr.isArray()) {
                errors->createNewError(GENERIC, expr.link, "cannot get integer value from var[] expression");
            }

            if(expr.func) {
                out.code.push_i64(SET_Di(i64, op_LOADVAL, reg));
            } else if(reg != ebx) {
                if(reg == cmt) {
                    if(!expr.inCmtRegister)
                        out.code.push_i64(SET_Ci(i64, op_MOVR, reg,0, ebx));
                }
                else
                    out.code.push_i64(SET_Ci(i64, op_MOVR, reg,0, ebx));
            }
            break;
        case expression_field:
            if(expr.utype.field->isVar() && !expr.utype.field->isArray) {
                if(expr.utype.field->local) {
                    if(reg != ebx) {
                        out.code.push_i64(SET_Ci(i64, op_MOVR, reg,0, ebx));
                    }
                } else {
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                    out.code.push_i64(SET_Di(i64, op_CHECKLEN, adx));
                    out.code.push_i64(SET_Ci(i64, op_IALOAD_2, reg,0, adx));
                }
            } else if(expr.utype.field->isVar() && expr.utype.field->isArray) {
                errors->createNewError(GENERIC, expr.link, "cannot get integer value from " + expr.utype.field->name + "[] expression");
            } else if(expr.utype.field->dynamicObject() && !expr.utype.field->isArray) {
                out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                out.code.push_i64(SET_Di(i64, op_CHECKLEN, adx));
                out.code.push_i64(SET_Ci(i64, op_IALOAD_2, reg,0, adx));
            }else if(expr.utype.field->type == CLASS || expr.utype.field->dynamicObject()) {
                if(expr.utype.field->isEnum || expr.utype.field->klass->isEnum()) {
                    Field *valueField = expr.utype.field->klass->getField("value", true);
                    if(valueField != NULL) {
                        if(isFieldInlined(expr.utype.field)) {
                            inlineVariableValue(out, expr.utype.field);
                        } else {
                            out.code.push_i64(SET_Di(i64, op_MOVN, valueField->address));
                            out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                            out.code.push_i64(SET_Ci(i64, op_IALOAD_2, reg,0, adx));
                        }
                    }
                } else if(isNativeIntegerClass(expr.utype.field->klass)) {
                    out.code.push_i64(SET_Di(i64, op_MOVN, 0));
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                    out.code.push_i64(SET_Ci(i64, op_IALOAD_2, reg,0, adx));
                } else
                    errors->createNewError(GENERIC, expr.link, "cannot get integer value from non integer type `object`");
            }
            break;
        case expression_lclass:
            if(isNativeIntegerClass(expr.getClass())) {
                if(expr.func) {
                    out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                    out.code.push_i64(SET_Di(i64, op_MOVND, adx));
                } else {
                    out.code.push_i64(SET_Di(i64, op_MOVN, 0));
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                }
                out.code.push_i64(SET_Ci(i64, op_IALOAD_2, reg,0, adx));
                if(expr.func)
                    out.code.push_i64(SET_Ei(i64, op_POP));
            } else
                errors->createNewError(GENERIC, expr.link, "cannot get integer value from non integer type `" + expr.utype.typeToString() +"`");
            break;
        case expression_string:
            errors->createNewError(GENERIC, expr.link, "cannot get integer value from non integer type `var[]`");
            break;
        case expression_null:
            errors->createNewError(GENERIC, expr.link, "cannot get integer value from type of null");
            break;
        case expression_objectclass:
            errors->createNewError(GENERIC, expr.link, "cannot get integer value from non integer type `dynamic_object`");
            break;
        case expression_unresolved:
            break;
        default:
            errors->createNewError(GENERIC, expr.link, "cannot get integer value from non integer expression");
            break;
    }
}

Expression RuntimeEngine::parseArrayExpression(Expression& interm, Ast* pAst) {
    Expression expression(interm), indexExpr(pAst);

    indexExpr = parseExpression(pAst->getSubAst(ast_expression));

    expression.newExpression=false;
    expression.utype.array = false;
    expression.utype.array = false;
    expression.arrayElement=true;
    expression.func=false;
    expression.link = pAst;
    bool referenceAffected = currentRefrenceAffected(indexExpr);

    switch(interm.type) {
        case expression_field:
            if(!interm.utype.field->isArray) {
                // error not an array
                errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

            pushExpressionToRegister(indexExpr, expression, ebx);

            expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));

            if(interm.utype.field->type == CLASS) {
                expression.utype.klass = interm.utype.field->klass;
                expression.type = expression_lclass;

                expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));
            } else if(interm.utype.field->type == VAR) {
                expression.type = expression_var;
                expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));
            } else if(interm.utype.field->type == OBJECT) {
                expression.type = expression_objectclass;
                expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));
            }else {
                expression.type = expression_unknown;
            }
            break;
        case expression_string:
            errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            break;
        case expression_var:
            if(!interm.isArray()) {
                // error not an array
                errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            if(!interm.func) {
                expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }

            pushExpressionToRegister(indexExpr, expression, ebx);


            if(interm.func) {
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            } else {
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            }


            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));
            expression.code.push_i64(SET_Ci(i64, op_IALOAD_2, ebx,0, ebx));

            if(interm.func) {
                expression.code.push_i64(SET_Ei(i64, op_POP));
            } else {
                // do nothing??? no idea man
            }
            break;
        case expression_lclass:
            if(!interm.isArray()) {
                // error not an array
                errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

            pushExpressionToRegister(indexExpr, expression, ebx);

            expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));

            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));
            expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));
            break;
        case expression_null:
            errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "null cannot be used as an array");
            break;
        case expression_objectclass:
            if(!interm.isArray()) {
                // error not an array
                errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

            pushExpressionToRegister(indexExpr, expression, ebx);

            if(c_options.optimize) {
                if(referenceAffected)
                    expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            } else
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));

            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));
            expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));

            break;
        case expression_void:
            errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "void cannot be used as an array");
            break;
        case expression_unresolved:
            /* do nothing */
            break;
        default:
            errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "invalid array expression before `[`");
            break;
    }

    if(indexExpr.type == expression_var) {
        // we have an integer!
    } else if(indexExpr.type == expression_field) {
        if(!indexExpr.utype.field->isVar()) {
            errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "array index is not an integer");
        }
    } else if(indexExpr.type == expression_unresolved) {
    } else {
        errors->createNewError(GENERIC, indexExpr.link->line, indexExpr.link->col, "array index is not an integer");
    }

    return expression;
}


Expression &RuntimeEngine::parseDotNotationChain(Ast *pAst, Expression &expression, unsigned int startpos) {

    Ast* utype;
    Expression rightExpr(pAst);
    Expression result(pAst);
    rightExpr =expression;
    rightExpr.code.free();
    for(unsigned int i = startpos; i < pAst->getSubAstCount(); i++) {
        utype = pAst->getSubAst(i);

        if(rightExpr.isArray() && utype->getType() != ast_expression) {
            errors->createNewError(GENERIC, utype->line, utype->col, "expected array index `[]`");
        }

        if(utype->getType() == ast_dotnotation_call_expr) {
            result = parseDotNotationChain(utype, rightExpr, 0);
            rightExpr = result;
        }
        else if(utype->getType() == ast_expression){ /* array expression */
            result = parseArrayExpression(rightExpr, utype);
            rightExpr = result;
        }
        else {
            result = parseDotNotationCallContext(rightExpr, utype);
            rightExpr = result;
        }

        rightExpr.link = utype;
        if(rightExpr.type == expression_unresolved || rightExpr.type == expression_unknown)
            break;

        expression.inject(rightExpr);
        expression.type = rightExpr.type;
        expression.utype = rightExpr.utype;
        expression.utype.array = rightExpr.isArray();
        expression.func=rightExpr.func;
        rightExpr.code.free();
    }
    return expression;
}

Expression RuntimeEngine::parseDotNotationCall(Ast* pAst) {
    string method_name="";
    Expression expression(pAst), interm(pAst);
    Method* fn;
    Ast* pAst2;

    if(pAst->hasSubAst(ast_dot_fn_e)) {
        pAst2 = pAst->getSubAst(ast_dot_fn_e);
        fn = resolveMethodUtype(pAst2, pAst2, expression);
        if(fn != NULL) {

            expression.type = methodReturntypeToExpressionType(fn);
            if(expression.type == expression_lclass) {
                expression.utype.klass = fn->klass;
                expression.utype.type = CLASS;
            }

            expression.func = true;
            expression.utype.array = fn->array;

            if(pAst->hasEntity(_INC) || pAst->hasEntity(_DEC)) {
                token_entity entity = pAst->hasEntity(_INC) ? pAst->getEntity(_INC) : pAst->getEntity(_DEC);
                OperatorOverload* overload;
                List<Param> emptyParams;

                expression.type = expression_var;
                if(fn->array) {
                    errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "call to function `" + fn->getName() + paramsToString(fn->getParams()) + "` must return an var to use `" + entity.getToken() + "` operator");
                } else {
                    switch(fn->type) {
                        case TYPEVOID:
                            errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "cannot use `" + entity.getToken() + "` operator on function that returns void ");
                            break;
                        case VAR:
                            if(pAst->hasEntity(_INC))
                                expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                            else
                                expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                            break;
                        case OBJECT:
                            errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "function returning object must be casted before using `"
                                                                                                  + entity.getToken() + "` operator");
                            break;
                        case CLASS:
                            if((overload = fn->klass->getOverload(stringToOp(entity.getToken()), emptyParams)) != NULL) {
                                // add code to call overload
                                verifyMethodAccess(overload, pAst);
                                expression.type = methodReturntypeToExpressionType(overload);
                                if(expression.type == expression_lclass) {
                                    expression.utype.klass = overload->klass;
                                    expression.utype.type = CLASS;
                                }

                                expression.code.push_i64(SET_Di(i64, op_CALL, overload->address));
                            } else if(fn->klass->hasOverload(stringToOp(entity.getToken()))) {
                                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "call to function `" + fn->getName() + paramsToString(fn->getParams()) + "`; missing overload params for operator `"
                                                                                                      + fn->klass->getFullName() + ".operator" + entity.getToken() + "`");
                            } else {
                                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "call to function `" + fn->getName() + paramsToString(fn->getParams()) + "` must return an int to use `" + entity.getToken() + "` operator");
                            }
                            break;
                        case UNDEFINED:
                            // do nothing
                            break;
                    }
                }
            }
        } else
            expression.type = expression_unresolved;
    } else {
        expression = parseUtype(pAst);
    }

    if(pAst->getSubAstCount() > 1) {
        // chained calls
        if(expression.type == expression_void) {
            errors->createNewError(GENERIC, pAst->getSubAst(1)->line, pAst->getSubAst(1)->col, "illegal access to function of return type `void`");
        } else
            parseDotNotationChain(pAst, expression, 1);
    }

    if(pAst->hasEntity(DOT)) {
        expression.dot = true;
    }

    expression.link = pAst;
    return expression;
}

Expression RuntimeEngine::parsePrimaryExpression(Ast* ast) {
    if(ast->getType() == ast_primary_expr)
        ast = ast->getSubAst(0);

    switch(ast->getType()) {
        case ast_literal_e:
            return parseLiteral(ast->getSubAst(ast_literal));
        case ast_utype_class_e:
            return psrseUtypeClass(ast);
        case ast_dot_not_e:
            return parseDotNotationCall(ast->getSubAst(ast_dotnotation_call_expr));
        case ast_self_e:
            return parseSelfExpression(ast);
        case ast_base_e:
            return parseBaseExpression(ast);
        case ast_null_e:
            return parseNullExpression(ast);
        case ast_new_e:
            return parseNewExpression(ast);
        case ast_cast_e:
            return parseCastExpression(ast);
        case ast_sizeof_e:
            return parseSizeOfExpression(ast);
        case ast_paren_e:
            return parseParenExpression(ast);
        case ast_arry_e:
            return parseArrayExpression(ast);
        case ast_post_inc_e:
            return parsePostInc(ast);
        default:
            stringstream err;
            err << ": unknown ast type: " << ast->getType();
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            return Expression(ast); // not an expression!
    }
}

Method* RuntimeEngine::resolveSelfMethodUtype(Ast* utype, Ast* valueList, Expression& out) {
    Method* fn = NULL;
    utype = valueList->getSubAst(ast_utype);
    valueList = valueList->getSubAst(ast_value_list);

    /* This is a naked utype so we dont haave to worry about brackets */
    ReferencePointer ptr;
    string methodName = "";
    List<Param> params;
    List<Expression> expressions = parseValueList(valueList);
    Expression expression(utype);
    bool single = true;

    expressionListToParams(params, expressions);
    ptr = parseTypeIdentifier(utype);

    if(splitMethodUtype(methodName, ptr)) {
        single = false;
        // accessor
        if(currentScope()->type == GLOBAL_SCOPE) {

            errors->createNewError(GENERIC, valueList->line, valueList->col,
                                   "cannot get object `" + methodName + paramsToString(params) + "` from self at global scope");
        }

        currentScope()->self = true;
        resolveUtype(ptr, expression, utype);
        currentScope()->self = false;
        if(expression.type == expression_class || (expression.type == expression_field && expression.utype.field->type == CLASS)) {
            ClassObject* klass;
            if(expression.type == expression_class) {
                klass = expression.utype.klass;
            } else {
                klass = expression.utype.field->klass;
            }

            if((fn = klass->getFunction(methodName, params, true, true)) != NULL){}
            else if((fn = klass->getOverload(stringToOp(methodName), params, true, true)) != NULL){}
            else if(methodName == klass->getName() && (fn = klass->getConstructor(params, false, true)) != NULL) {}
            else {
                if(stringToOp(methodName) != oper_UNDEFINED) methodName = "operator" + methodName;

                errors->createNewError(COULD_NOT_RESOLVE, valueList->line, valueList->col, " `" + methodName + paramsToString(params) + "`");
            }
        } else if(expression.utype.type == UNDEFINED) {
        }
        else {
            errors->createNewError(COULD_NOT_RESOLVE, valueList->line, valueList->col, " `" + methodName + paramsToString(params) +  "`");
        }
    } else {
        // method or macros
        if(ptr.singleRefrence()) {
            if(currentScope()->type == GLOBAL_SCOPE) {
                // must be macros
                errors->createNewError(GENERIC, valueList->line, valueList->col,
                                       "cannot get object `" + ptr.referenceName + paramsToString(params) + "` from self at global scope");
            } else {

                if((fn = currentScope()->klass->getFunction(ptr.referenceName, params, true, true)) != NULL){}
                else if((fn = currentScope()->klass->getOverload(stringToOp(ptr.referenceName), params, true, true)) != NULL){}
                else if(ptr.referenceName == currentScope()->klass->getName() && (fn = currentScope()->klass->getConstructor(params, false, true)) != NULL) {}
                else {
                    if(stringToOp(methodName) != oper_UNDEFINED) methodName = "operator" + ptr.referenceName;
                    else methodName = ptr.referenceName;

                    errors->createNewError(COULD_NOT_RESOLVE, valueList->line, valueList->col, " `" + ptr.referenceName +  paramsToString(params) + "`");
                }
            }
        } else {
            // must be macros
            errors->createNewError(GENERIC, valueList->line, valueList->col, " use of module {" + ptr.referenceName + "} in expression signifies global access of object");

        }
    }

    if(fn != NULL) {
        if(!fn->isStatic() && currentScope()->type == STATIC_BLOCK) {
            errors->createNewError(GENERIC, utype->line, utype->col, "call to instance function in static context");
        }

        verifyMethodAccess(fn, utype);
        if(!fn->isStatic()) {
            if(single) {
                out.code.push_i64(SET_Di(i64, op_MOVL, 0));
                out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            } else {
                out.inject(expression);
                out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
        }

        for(unsigned int i = 0; i < expressions.size(); i++) {
            if(fn->getParam(i).field.dynamicObject() && expressions.get(i).type == expression_var) {
                pushExpressionToRegister(expressions.get(i), out, ebx);

                out.code.push_i64(SET_Di(i64, op_MOVI, 1), egx);
                out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                out.code.push_i64(SET_Di(i64, op_NEWARRAY, egx));
                out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                out.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
            } else if(isExpressionConvertableToNativeClass(&fn->getParam(i).field, expressions.get(i))) {
                ClassObject *k = fn->getParam(i).field.klass;
                Expression fExpr = fieldToExpression(utype, fn->getParam(i).field);
                constructNewNativeClass(k->getName(), k->getModuleName(), expressions.get(i), out, false);
            } else
                pushExpressionToStack(expressions.get(i), out);
        }

        if(fn->delegatePost) {
            if(fn->isStatic()) {
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE_STATIC, fn->address, 0, expressions.size()), fn->klass->address);
            } else
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE, fn->address, 0, expressions.size()));
        } else
            out.code.push_i64(SET_Di(i64, op_CALL, fn->address));
        out.func = true;
    }

    freeList(params);
    freeList(expressions);
    ptr.free();
    return fn;
}

Expression RuntimeEngine::parseSelfDotNotationCall(Ast* pAst) {
    string method_name="";
    Expression expression(pAst);
    Method* fn;
    Ast* pAst2;

    if(pAst->hasSubAst(ast_dot_fn_e)) {
        pAst2 = pAst->getSubAst(ast_dot_fn_e);
        fn = resolveSelfMethodUtype(pAst2, pAst2, expression);
        if(fn != NULL) {
            expression.type = methodReturntypeToExpressionType(fn);
            if(expression.type == expression_lclass) {
                expression.utype.klass = fn->klass;
                expression.utype.type = CLASS;
            }

            expression.utype.array = fn->array;
        } else
            expression.type = expression_unresolved;

    } else {
        currentScope()->self = true;
        expression = parseUtype(pAst);
        currentScope()->self = false;
    }

    if(pAst->hasEntity(DOT)) {
        expression.dot = true;
    }


    if(pAst->getSubAstCount() > 1) {
        // chained calls
        parseDotNotationChain(pAst, expression, 1);
    }

    expression.link = pAst;
    return expression;
}

Expression RuntimeEngine::parseSelfExpression(Ast* pAst) {
    Expression expression(pAst);

    if(pAst->hasEntity(PTR)) {
        // self-><expression>
        return parseSelfDotNotationCall(pAst->getSubAst(ast_dotnotation_call_expr));
    } else {
        // self
        expression.type = expression_lclass;
        expression.utype.type = CLASS;
        expression.utype.klass = currentScope()->klass;
        expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
    }

    if(currentScope()->type == GLOBAL_SCOPE || currentScope()->type == STATIC_BLOCK) {
        errors->createNewError(GENERIC, pAst->line, pAst->col, "illegal reference to self in static context");
        expression.type = expression_unknown;
    }

    expression.link = pAst;
    return expression;
}

Method* RuntimeEngine::resolveBaseMethodUtype(Ast* utype, Ast* valueList, Expression& out) {
    Method* fn = NULL;
    utype = valueList->getSubAst(ast_utype);
    valueList = valueList->getSubAst(ast_value_list);

    /* This is a naked utype so we dont haave to worry about brackets */
    ReferencePointer ptr;
    string methodName = "";
    List<Param> params;
    List<Expression> expressions = parseValueList(valueList);
    Expression expression(utype);

    expressionListToParams(params, expressions);
    ptr = parseTypeIdentifier(utype);
    bool single = true;

    if(splitMethodUtype(methodName, ptr)) {
        single = true;
        if(currentScope()->type == GLOBAL_SCOPE) {
            errors->createNewError(GENERIC, valueList->line, valueList->col,
                                   "cannot get object `" + methodName + paramsToString(params) + "` from self at global scope");
        }

        currentScope()->base = true;
        resolveUtype(ptr, expression, utype);
        currentScope()->base = false;
        if(expression.type == expression_class || (expression.type == expression_field && expression.utype.field->type == CLASS)) {
            ClassObject* klass;
            if(expression.type == expression_class) {
                klass = expression.utype.klass;
            } else {
                klass = expression.utype.field->klass;
            }

            if((fn = klass->getFunction(methodName, params, false, true)) != NULL){}
            else if((fn = klass->getOverload(stringToOp(methodName), params, false, true)) != NULL){}
            else if(methodName == klass->getName() && (fn = klass->getConstructor(params, false, true)) != NULL) {}
            else {
                if(stringToOp(methodName) != oper_UNDEFINED) methodName = "operator" + methodName;

                errors->createNewError(COULD_NOT_RESOLVE, valueList->line, valueList->col, " `" + (klass == NULL ? ptr.referenceName : methodName) + paramsToString(params) + "`");
            }
        } else if(expression.utype.type == UNDEFINED) {
        }
        else {
            errors->createNewError(COULD_NOT_RESOLVE, valueList->line, valueList->col, " `" + (expression.utype.klass == NULL ? ptr.referenceName : methodName) + paramsToString(params) +  "`");
        }
    } else {
        // method or macros
        if(ptr.singleRefrence()) {
            if(currentScope()->type == GLOBAL_SCOPE) {
                // must be macros
                errors->createNewError(GENERIC, valueList->line, valueList->col,
                                       "cannot get object `" + ptr.referenceName + paramsToString(params) + "` from self at global scope");
            } else {
                ClassObject* base, *start = currentScope()->klass->getBaseClass();

                if(start == NULL) {
                    errors->createNewError(GENERIC, utype->getSubAst(ast_type_identifier)->line, utype->getSubAst(ast_type_identifier)->col, "class `" + currentScope()->klass->getFullName() + "` does not inherit a base class");
                    return NULL;
                }

                for(;;) {
                    base = start;

                    if(base == NULL) {
                        // could not resolve
                        if(stringToOp(methodName) != oper_UNDEFINED) methodName = "operator" + ptr.referenceName;
                        else methodName = ptr.referenceName;

                        errors->createNewError(COULD_NOT_RESOLVE, valueList->line, valueList->col, " `" + ptr.referenceName
                                                                                                   +  paramsToString(params) + "`");
                        return NULL;
                    }

                    if((fn = base->getFunction(ptr.referenceName, params, false, true)) != NULL){ break; }
                    else if((fn = base->getOverload(stringToOp(ptr.referenceName), params, false, true)) != NULL){ break; }
                    else if(ptr.referenceName == base->getName() && (fn = base->getConstructor(params, false, true)) != NULL) { break; }
                    else {
                        start = base->getBaseClass(); // recursivley assign klass to new base
                    }
                }
            }
        } else {
            // must be macros
            errors->createNewError(GENERIC, valueList->line, valueList->col, " use of module {" + ptr.referenceName + "} in expression signifies global access of object");

        }
    }


    if(fn != NULL) {
        if(!fn->isStatic() && currentScope()->type == STATIC_BLOCK) {
            errors->createNewError(GENERIC, utype->line, utype->col, "call to instance function in static context");
        }

        verifyMethodAccess(fn, utype);
        if(!fn->isStatic()) {
            if(single) {
                out.code.push_i64(SET_Di(i64, op_MOVL, 0));
                out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            } else {
                out.inject(expression);
                out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
        }

        for(unsigned int i = 0; i < expressions.size(); i++) {
            if(fn->getParam(i).field.dynamicObject() && expressions.get(i).type == expression_var) {
                pushExpressionToRegister(expressions.get(i), out, ebx);

                out.code.push_i64(SET_Di(i64, op_MOVI, 1), egx);
                out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                out.code.push_i64(SET_Di(i64, op_NEWARRAY, egx));
                out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                out.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
            } else if(isExpressionConvertableToNativeClass(&fn->getParam(i).field, expressions.get(i))) {
                ClassObject *k = fn->getParam(i).field.klass;
                Expression fExpr = fieldToExpression(utype, fn->getParam(i).field);
                constructNewNativeClass(k->getName(), k->getModuleName(), expressions.get(i), out, false);
            } else
                pushExpressionToStack(expressions.get(i), out);
        }

        if(fn->delegatePost) {
            if(fn->isStatic()) {
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE_STATIC, fn->address, 0, expressions.size()), fn->klass->address);
            } else
                out.code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE, fn->address, 0, expressions.size()));
        } else
            out.code.push_i64(SET_Di(i64, op_CALL, fn->address));
        out.func = true;
    }

    freeList(params);
    freeList(expressions);
    ptr.free();
    return fn;
}

Expression RuntimeEngine::parseBaseDotNotationCall(Ast* pAst) {
    pAst = pAst->getSubAst(ast_dotnotation_call_expr);
    Expression expression(pAst);
    string method_name="";
    Method* fn;
    Ast* pAst2;

    if(pAst->hasSubAst(ast_dot_fn_e)) {
        pAst2 = pAst->getSubAst(ast_dot_fn_e);
        fn = resolveBaseMethodUtype(pAst2, pAst2, expression);
        if(fn != NULL) {
            expression.type = methodReturntypeToExpressionType(fn);
            if(expression.type == expression_lclass) {
                expression.utype.klass = fn->klass;
                expression.utype.type = CLASS;
            }

            expression.utype.array = fn->array;
        } else
            expression.type = expression_unresolved;

    } else {
        currentScope()->base = true;
        expression = parseUtype(pAst);
        currentScope()->base = false;
    }

    if(pAst->hasEntity(DOT)) {
        expression.dot = true;
    }

    if(pAst->getSubAstCount() > 1) {
        // chained calls
        parseDotNotationChain(pAst, expression, 1);
    }

    expression.link = pAst;
    return expression;
}

Expression RuntimeEngine::parseBaseExpression(Ast* pAst) {
    Expression expression(pAst);

    expression = parseBaseDotNotationCall(pAst);

    expression.link = pAst;
    return expression;
}

Expression RuntimeEngine::parseNullExpression(Ast* pAst) {
    Expression expression(pAst);

    expression.utype.referenceName = "null";
    expression.type = expression_null;
    return expression;
}

List<Expression> RuntimeEngine::parseVectorArray(Ast* pAst) {
    List<Expression> vecArry;
    pAst = pAst->getSubAst(ast_vector_array);

    for(unsigned int i = 0; i < pAst->getSubAstCount(); i++) {
        vecArry.add(parseExpression(pAst->getSubAst(i)));
    }

    return vecArry;
}

void RuntimeEngine::checkVectorArray(Expression& utype, List<Expression>& vecArry) {
    if(utype.utype.type == UNDEFINED) return;
    Expression right(utype.link);

    for(unsigned int i = 0; i < vecArry.size(); i++) {
        if(vecArry.get(i).type != expression_unresolved) {
            switch(vecArry.get(i).type) {
                case expression_native:
                    errors->createNewError(UNEXPECTED_SYMBOL, vecArry.get(i).link->line, vecArry.get(i).link->col, " `" + ResolvedReference::typeToString(utype.utype.type) + "`");
                    break;
                case expression_field:
                    equalsVectorArray(utype, vecArry.get(i));
                    break;
                case expression_lclass:
                    if(utype.utype.type == CLASS) {
                        if(!utype.utype.klass->match(vecArry.get(i).utype.klass)){
                            errors->createNewError(INCOMPATIBLE_TYPES, vecArry.get(i).link->line, vecArry.get(i).link->col, ": initalization of class `" +
                                                                                                                            utype.utype.klass->getName() + "` is not compatible with class `" + vecArry.get(i).utype.klass->getName() + "`");
                        }
                    } else if(utype.utype.type == OBJECT) {
                        /* Do nothing */
                    } else if(utype.trueType() == VAR) {
                        /* Do nothing */
                    } else {
                        errors->createNewError(INCOMPATIBLE_TYPES, vecArry.get(i).link->line, vecArry.get(i).link->col, ": type `" + utype.utype.typeToString() + "` is not compatible with type `"
                                                                                                                        + vecArry.get(i).utype.typeToString() + "`");
                    }
                    break;
                case expression_var:
                    if(utype.utype.isVar() && vecArry.get(i).trueType() == VAR) {}
                    else if(utype.utype.isVar() && vecArry.get(i).literal && vecArry.get(i).type == expression_var) {}
                    else if(utype.utype.dynamicObject() && vecArry.get(i).utype.dynamicObject()) {}
                    else if(utype.utype.dynamicObject() && vecArry.get(i).trueType() == VAR) {}
                    else {
                        errors->createNewError(INCOMPATIBLE_TYPES, vecArry.get(i).link->line, vecArry.get(i).link->col, ": type `" + utype.utype.typeToString() + "` is not compatible with type `"
                                                                                                                        + vecArry.get(i).utype.typeToString() + "`");
                    }
                    break;
                case expression_null:
                    if(utype.utype.type != CLASS && utype.utype.type != OBJECT) {
                        errors->createNewError(GENERIC, vecArry.get(i).link->line, vecArry.get(i).link->col, "cannot assign null to type `" + utype.utype.typeToString() + "`");
                    }
                    break;
                case expression_string:
                    errors->createNewError(GENERIC, vecArry.get(i).link->line, vecArry.get(i).link->col, "multi dimentional array are not supported yet, use string() class instead");
                    break;
                case expression_objectclass:
                    if(utype.utype.type != CLASS) {
                        errors->createNewError(GENERIC, vecArry.get(i).link->line, vecArry.get(i).link->col, "cannot assign null to type `" + utype.utype.typeToString() + "`");
                    }
                    break;
                default:
                    errors->createNewError(GENERIC, vecArry.get(i).link->line, vecArry.get(i).link->col, "error processing vector array");
                    break;
            }
        }
    }
}

void RuntimeEngine::parseNewArrayExpression(Expression& out, Expression& utype, Ast* pAst) {
    Expression sizeExpr = parseExpression(pAst->getSubAst(ast_expression));

    pushExpressionToRegister(sizeExpr, out, ebx);

    if(out.type == expression_var)
        out.code.push_i64(SET_Di(i64, op_NEWARRAY, ebx));
    else if(out.type == expression_lclass) {
        out.code.push_i64(SET_Ci(i64, op_NEWCLASSARRAY, ebx, 0, utype.utype.klass->address));
    }
    else
        out.code.push_i64(SET_Di(i64, op_NEWOBJARRAY, ebx));
}

Expression RuntimeEngine::parseNewExpression(Ast* pAst) {
    Expression expression(pAst), utype(pAst);
    List<Expression> expressions;
    List<Param> params;

    utype = parseUtype(pAst);
    if(pAst->hasSubAst(ast_vector_array)) {
        // vec array
        expressions = parseVectorArray(pAst);
        checkVectorArray(utype, expressions);

        expression.type = utype.type;
        expression.utype = utype.utype;
        if(expression.type == expression_class) {
            expression.type = expression_lclass;
        } else if(expression.type == expression_native) {
            if(utype.utype.isVar())
                expression.type = expression_var;
            else if(utype.utype.dynamicObject())
                expression.type = expression_objectclass;
        }
        expression.utype.array = true;

        expression.code.push_i64(SET_Di(i64, op_MOVI, expressions.size()), ebx);
        if(expression.type == expression_var)
            expression.code.push_i64(SET_Di(i64, op_NEWARRAY, ebx));
        else if(expression.type == expression_lclass) {
            expression.code.push_i64(SET_Ci(i64, op_NEWCLASSARRAY, ebx, 0, utype.utype.klass->address));
        }
        else
            expression.code.push_i64(SET_Di(i64, op_NEWOBJARRAY, ebx));

        // assign values

        /**
         * Mannually assign values
         */
        for(long i = 0; i < expressions.size(); i++) {
            Expression &right = expressions.get(i);


            switch(expression.type) {
                case expression_var:
                    if(!currentRefrenceAffected(right))
                        expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));

                    pushExpressionToRegister(right, expression, ebx);

                    if(currentRefrenceAffected(right)) {
                        expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                    }
                    expression.code.push_i64(SET_Di(i64, op_MOVI, i), adx);
                    expression.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
                    break;
                case expression_lclass:
                case expression_objectclass:
                    if(right.type != expression_null) {
                        if(right.trueType() == VAR) {
                            varToObject(right, expression);
                        } else
                            pushExpressionToStack(right, expression);

                        expression.code.push_i64(SET_Di(i64, op_MOVSL, -1)); // get our array object
                        expression.code.push_i64(SET_Di(i64, op_MOVN, i)); // select array element
                        expression.code.push_i64(SET_Ei(i64, op_POPOBJ)); // set object
                    }
                    break;
            }
        }
    }
    else if(pAst->hasSubAst(ast_array_expression)) {
        expression.type = utype.type;
        expression.utype = utype.utype;
        if(expression.type == expression_class) {
            expression.type = expression_lclass;
        } else if(expression.type == expression_native) {
            if(utype.utype.isVar())
                expression.type = expression_var;
            else if(utype.utype.dynamicObject())
                expression.type = expression_objectclass;
        }
        expression.utype.array = true;
        parseNewArrayExpression(expression, utype, pAst->getSubAst(ast_array_expression));
    }
    else {
        // ast_value_list
        if(pAst->hasSubAst(ast_value_list))
            expressions = parseValueList(pAst->getSubAst(ast_value_list));
        if(utype.type == expression_class) {
            Method* fn=NULL;
            if(!pAst->hasSubAst(ast_value_list)) {
                errors->createNewError(GENERIC, utype.link->line, utype.link->col, "expected '()' after class " + utype.utype.klass->getName());
            } else {
                expressionListToParams(params, expressions);
                if((fn=utype.utype.klass->getConstructor(params, true, true))==NULL) {
                    errors->createNewError(GENERIC, utype.link->line, utype.link->col, "class `" + utype.utype.klass->getFullName() +
                                                                                       "` does not contain constructor `" + utype.utype.klass->getName() + paramsToString(params) + "`");
                }
            }
            expression.type = expression_lclass;
            expression.utype = utype.utype;

            if(fn!= NULL) {

                verifyMethodAccess(fn, pAst);
                expression.code.push_i64(SET_Di(i64, op_NEWCLASS, utype.utype.klass->address));

                for(unsigned int i = 0; i < expressions.size(); i++) {
                    if(fn->getParam(i).field.dynamicObject() && expressions.get(i).type == expression_var) {
                        pushExpressionToRegister(expressions.get(i), expression, ebx);

                        expression.code.push_i64(SET_Di(i64, op_MOVI, 1), egx);
                        expression.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
                        expression.code.push_i64(SET_Di(i64, op_NEWARRAY, egx));
                        expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                        expression.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
                    } else if(isExpressionConvertableToNativeClass(&fn->getParam(i).field, expressions.get(i))) {
                        ClassObject *k = fn->getParam(i).field.klass;
                        Expression fExpr = fieldToExpression(pAst, fn->getParam(i).field);
                        constructNewNativeClass(k->getName(), k->getModuleName(), expressions.get(i), expression, false);
                    } else
                        pushExpressionToStack(expressions.get(i), expression);
                }

                expression.code.push_i64(SET_Di(i64, op_CALL, fn->address));


                if(pAst->hasSubAst(ast_dotnotation_call_expr)) {
                    Ast *context = pAst->getSubAst(ast_dotnotation_call_expr);
                    Expression contextExpression = parseDotNotationCallContext(expression, context->getSubAst(0));
                    Assembler code;
                    code.inject(0, expression.code);
                    expression = contextExpression;
                    expression.code.inject(0, code);
                }
            }

        } else if(utype.type == expression_native) {
            // native creation
            if(pAst->hasSubAst(ast_value_list)) {
                errors->createNewError(GENERIC, utype.link->line, utype.link->col, " native type `" + utype.utype.typeToString() + "` does not contain a constructor");
            }
            expression.type = expression_native;
            expression.utype = utype.utype;
        } else {
            if(utype.utype.type != UNDEFINED)
                errors->createNewError(COULD_NOT_RESOLVE, utype.link->line, utype.link->col, " `" + utype.utype.typeToString() + "`");
        }
    }

    freeList(params);
    freeList(expressions);
    expression.newExpression = true;
    expression.link = pAst;
    return expression;
}

Expression RuntimeEngine::parseSizeOfExpression(Ast* pAst) {
    Expression expression = parseExpression(pAst->getSubAst(ast_expression)), out(pAst);
    switch(expression.type) {
        case expression_void:
        case expression_unknown:
        case expression_class:
        case expression_native:
        case expression_null:
            errors->createNewError(GENERIC, expression.link, "cannot get sizeof from expression of type `" + expression.typeToString() + "`");
            break;
        case expression_string:
            out.code.push_i64(SET_Di(i64, op_MOVI, expression.value.size()), ebx); // This is silly but we will allow it
            break;
        default:
            if(expression.type == expression_var && expression.literal) {
                errors->createNewError(GENERIC, expression.link, "cannot get sizeof from literal value");
            }

            pushExpressionToPtr(expression, out);

            if(out.code.size() == 0)
                out.code.push_i64(SET_Di(i64, op_MOVI, 1), ebx); // just in case out object isnt an object
            else {
                if(expression.func) {
                    out.code.push_i64(SET_Di(i64, op_SIZEOF, ebx)); // just in case out object isnt an object
                    out.code.push_i64(SET_Ei(i64, op_POP));
                } else
                    out.code.push_i64(SET_Di(i64, op_SIZEOF, ebx)); // just in case out object isnt an object
            }
            break;
    }

    out.type = expression_var;
    return out;
}

Expression RuntimeEngine::parseIntermExpression(Ast* pAst) {
    Expression expression(pAst);

    if(pAst==NULL) return expression;
    switch(pAst->getType()) {
        case ast_primary_expr:
            return parsePrimaryExpression(pAst);
        case ast_self_e:
            return parseSelfExpression(pAst);
        case ast_base_e:
            return parseBaseExpression(pAst);
        case ast_null_e:
            return parseNullExpression(pAst);
        case ast_new_e:
            return parseNewExpression(pAst);
        case ast_sizeof_e:
            return parseSizeOfExpression(pAst);
        case ast_post_inc_e:
            return parsePostInc(pAst);
        case ast_arry_e:
            return parseArrayExpression(pAst);
        case ast_cast_e:
            return parseCastExpression(pAst);
        case ast_pre_inc_e:
            return parsePreInc(pAst);
        case ast_paren_e:
            return parseParenExpression(pAst);
        case ast_not_e:
            return parseNotExpression(pAst);
        case ast_vect_e:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "unexpected vector array expression. Did you mean 'new type { <data>, .. }'?");
            expression.link = pAst;
            return expression;
        case ast_add_e:
        case ast_mult_e:
        case ast_shift_e:
        case ast_less_e:
        case ast_equal_e:
        case ast_and_e:
            return parseBinaryExpression(pAst);
        case ast_ques_e:
            return parseQuesExpression(pAst);
        default:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "unexpected expression format");
            return Expression(pAst); // not an expression!
    }
}

void RuntimeEngine::postIncClass(Expression& out, token_entity op, ClassObject* klass) {
    OperatorOverload* overload;
    List<Param> emptyParams;

    if(op.getTokenType() == _INC) {
        overload = klass->getPostIncOverload();
    } else {
        overload = klass->getPostDecOverload();
    }

    if(overload != NULL) {
        // add code to call overload
        out.func = true;

        if(!overload->isStatic())
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

        verifyMethodAccess(overload, out.link);
        out.code.push_i64(SET_Di(i64, op_MOVI, 1), ebx);
        out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));


        out.code.push_i64(SET_Di(i64, op_CALL, overload->address));

        out.type = methodReturntypeToExpressionType(overload);
        if(out.type == expression_lclass) {
            out.utype.klass = overload->klass;
            out.utype.type = CLASS;
        } else {

        }
    } else if(klass->hasOverload(stringToOp(op.getToken()))) {
        errors->createNewError(GENERIC, op.getLine(), op.getColumn(), "use of class `" + klass->getName() + "`; missing overload params for operator `"
                                                                      + klass->getFullName() + ".operator" + op.getToken() + "`");
        out.type = expression_var;
    } else {
        errors->createNewError(GENERIC, op.getLine(), op.getColumn(), "use of class `" + klass->getName() + "` must return an int to use `" + op.getToken() + "` operator");
        out.type = expression_var;
    }
}

Expression RuntimeEngine::parsePostInc(Ast* pAst) {
    Expression interm(pAst);
    token_entity entity = pAst->hasEntity(_INC) ? pAst->getEntity(_INC) : pAst->getEntity(_DEC);

    interm = parsePrimaryExpression(pAst->getSubAst(0));
    Expression expression(interm);

    if(interm.isArray()){
        errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "expression must evaluate to an int to use `" + entity.getToken() + "` operator");
    } else {
        switch(interm.type) {
            case expression_var:
                if(interm.func) {
                    pushExpressionToRegister(interm, expression, ebx);
                    if(entity.getTokenType() == _INC)
                        expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                    else
                        expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                } else {
                    if(entity.getTokenType() == _INC)
                        expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                    else
                        expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                }
                break;
            case expression_field:
                if(interm.utype.field->type == CLASS) {
                    postIncClass(expression, entity, interm.utype.field->klass);
                    return expression;
                } else if(interm.utype.field->type == VAR || interm.utype.field->type == OBJECT) {
                    if(interm.utype.field->type == OBJECT) {
                        errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "use of `" + entity.getToken() + "` operator on field of type `dynamic_object` without a cast. Try ((SomeClass)dynamic_class)++");
                    } else if(interm.utype.field->isVar()) {
                        // increment the field
                        pushExpressionToRegisterNoInject(interm, expression, ebx);


                        if(entity.getTokenType() == _INC) {
                            if(expression.utype.field->local) {
                                if(c_options.optimize) {
                                    expression.code.free();
                                    expression.code.push_i64(SET_Ci(i64, op_LOADL, ebx, 0, interm.utype.field->address));
                                    expression.code.push_i64(
                                            SET_Ci(i64, op_IADDL, 1,0 , interm.utype.field->address));
                                } else {
                                    expression.code.push_i64(SET_Di(i64, op_MOVI, 1), ecx);
                                    expression.code.push_i64(
                                            SET_Ci(i64, op_ADDL, ecx,0 , interm.utype.field->address));
                                }
                            }
                            else {
                                expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                                expression.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
                                expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                            }
                        }
                        else {
                            if(expression.utype.field->local) {
                                if(c_options.optimize) {
                                    expression.code.free();
                                    expression.code.push_i64(SET_Ci(i64, op_LOADL, ebx, 0, interm.utype.field->address));
                                    expression.code.push_i64(
                                            SET_Ci(i64, op_ISUBL, 1,0 , interm.utype.field->address));
                                } else {
                                    expression.code.push_i64(SET_Di(i64, op_MOVI, 1), ecx);
                                    expression.code.push_i64(
                                            SET_Ci(i64, op_SUBL, ecx, 0, interm.utype.field->address));
                                }
                            }
                            else {
                                expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                                expression.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
                                expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                            }
                        }
                    }
                } else if(interm.utype.field->type != UNDEFINED){
                    // do nothing field is unresolved
                    errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "expression must evaluate to an int to use `" + entity.getToken() + "` operator");

                }
                break;
            case expression_lclass:
                if(interm.func)
                    expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                postIncClass(expression, entity, interm.utype.klass);
                return expression;
                break;
            case expression_objectclass:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "use of `" + entity.getToken() + "` operator on type `object` without a cast. Try ((SomeClass)object)++");
                break;
            case expression_null:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "value `null` cannot be used as var");
                break;
            case expression_native:
                errors->createNewError(UNEXPECTED_SYMBOL, entity.getLine(), entity.getColumn(), " `" + interm.utype.typeToString() + "`");
                break;
            case expression_string:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "increment on immutable string");
                break;
            case expression_unresolved:
                // do nothing
                break;
            default:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "expression must evaluate to an int to use `" + entity.getToken() + "` operator");
                break;
        }
    }

    expression.type = expression_var;
    return expression;
}

Expression RuntimeEngine::parseArrayExpression(Ast* pAst) {
    Expression expression(pAst), interm(pAst), indexExpr(pAst);
    Field* field;

    interm = parseDotNotationCall(pAst->getSubAst(0)->getSubAst(ast_dotnotation_call_expr));
    indexExpr = parseExpression(pAst->getSubAst(1));

    pushExpressionToPtr(interm, expression);
    expression.type = interm.type;
    expression.utype = interm.utype;
    expression.utype.array = false;
    expression.newExpression=false;
    expression.arrayElement=true;
    expression.func=false;
    expression.link = pAst;
    bool referenceAffected = currentRefrenceAffected(indexExpr);

    switch(interm.type) {
        case expression_field:
            if(!interm.utype.field->isArray) {
                // error not an array
                errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            if(referenceAffected)
                expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

            pushExpressionToRegister(indexExpr, expression, ebx);

            if(referenceAffected)
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));


            if(interm.utype.field->type == CLASS) {
                expression.utype.klass = interm.utype.field->klass;
                expression.type = expression_lclass;

                expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));
            } else if(interm.utype.field->type == VAR) {
                expression.type = expression_var;
                expression.code.push_i64(SET_Ci(i64, op_IALOAD_2, ebx,0, ebx));
            }
            else if(interm.utype.field->type == OBJECT) {
                expression.type = expression_objectclass;
                expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));
            } else {
                expression.type = expression_unknown;
            }

            if(referenceAffected)
                expression.code.push_i64(SET_Ei(i64, op_POP));

            break;
        case expression_string:
            errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            break;
        case expression_var:
            if(!interm.isArray()) {
                // error not an array
                errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            if(referenceAffected)
                expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

            pushExpressionToRegister(indexExpr, expression, ebx);

            if(referenceAffected)
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));

            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));
            expression.code.push_i64(SET_Ci(i64, op_IALOAD_2, ebx,0, ebx));

            break;
        case expression_lclass:
            if(!interm.isArray()) {
                // error not an array
                errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            if(interm.newExpression) {
                expression.code.inject(expression.code.__asm64.size(), interm.code);
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            }

            if(!interm.newExpression && referenceAffected) {
                expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }


            pushExpressionToRegister(indexExpr, expression, ebx);

            if(!interm.newExpression && referenceAffected) {
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            }

            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));
            expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));

            if(interm.newExpression && referenceAffected) {
                expression.code.push_i64(SET_Ei(i64, op_POP));
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "null cannot be used as an array");
            break;
        case expression_objectclass:
            if(!interm.isArray()) {
                // error not an array
                errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "expression of type `" + interm.typeToString() + "` must evaluate to array");
            }

            if(referenceAffected)
                expression.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

            pushExpressionToRegister(indexExpr, expression, ebx);

            if(referenceAffected)
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));

            expression.code.push_i64(SET_Di(i64, op_CHECKLEN, ebx));
            expression.code.push_i64(SET_Di(i64, op_MOVND, ebx));
            break;
        case expression_void:
            errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "void cannot be used as an array");
            break;
        case expression_unresolved:
            /* do nothing */
            break;
        default:
            errors->createNewError(GENERIC, pAst->getSubAst(0)->line, pAst->getSubAst(0)->col, "invalid array expression before `[`");
            break;
    }

    if(indexExpr.type == expression_var) {
        // we have an integer!
    } else if(indexExpr.type == expression_field) {
        if(!indexExpr.utype.field->isVar()) {
            errors->createNewError(GENERIC, pAst->getSubAst(1)->line, pAst->getSubAst(1)->col, "array index is not an integer");
        }
    } else if(indexExpr.type == expression_unresolved) {
    } else {
        errors->createNewError(GENERIC, pAst->getSubAst(1)->line, pAst->getSubAst(1)->col, "array index is not an integer");
    }

    if(pAst->getSubAstCount() > 2) {
        // expression after (we may not need this?)
        parseDotNotationChain(pAst->getSubAst(2), expression, 0);
    }

    return expression;
}

void RuntimeEngine::parseClassCast(Expression& utype, Expression& arg, Expression& out) {
    if(arg.type != expression_unresolved) {
        if(arg.utype.isArray() != utype.utype.isArray()) {
            errors->createNewError(INCOMPATIBLE_TYPES, utype.link->line, utype.link->col, "; cannot cast `" +
                                                                                          arg.utype.typeToString() + "` to `" + utype.utype.typeToString() + "`");
            out.type = expression_unresolved;
            return;
        }
    }

    if(utype.type == expression_class)
        utype.type = expression_lclass;

    switch(arg.type) {
        case expression_lclass:
            if(utype.utype.klass->assignable(arg.utype.klass)) {
                out.inject(arg);
                out.utype = utype.utype;
                out.type = expression_lclass;
                return;
            }
            break;
        case expression_objectclass:
            // TODO: put runtime code to evaluate at runtime
            pushExpressionToPtr(arg, out);
            out.code.push_i64(SET_Di(i64, op_MOVI, utype.utype.klass->address), cmt);
            out.code.push_i64(SET_Di(i64, op_CAST, cmt));
            out.type = utype.type;
            out.utype = utype.utype;
            return;
        case expression_field:
            if(arg.utype.field->type == CLASS) {
                if(utype.utype.klass->assignable(arg.utype.klass)) {
                    out.inject(arg);
                    out.type = expression_lclass;
                    out.utype = utype.utype;
                    return;
                }
            } else if(arg.utype.field->type == OBJECT) {
                pushExpressionToPtr(arg, out);
                out.code.push_i64(SET_Di(i64, op_MOVI, utype.utype.klass->address), cmt);
                out.code.push_i64(SET_Di(i64, op_CAST, cmt));
                out.type = expression_lclass;
                out.utype = utype.utype;
                return;
            } else if(arg.utype.field->type == VAR) {
                errors->createNewError(GENERIC, utype.link->line, utype.link->col, "field `" + arg.utype.field->name + "` is not a class; "
                        "cannot cast `" + arg.utype.typeToString() + "` to `" + utype.utype.typeToString() + "`");
                return;
            } else {
                // field is unresolved
                out.type = expression_unresolved;
                out.utype = utype.utype;
                return;
            }
            break;
    }

    if(arg.type != expression_unresolved)
        errors->createNewError(INCOMPATIBLE_TYPES, utype.link->line, utype.link->col, "; cannot cast `" + arg.utype.typeToString() + "` to `" + utype.utype.typeToString() + "`");
    return;
}

// TODO: add native support for casting
void RuntimeEngine::parseNativeCast(Expression& utype, Expression& expression, Expression& out) {

    if(expression.isArray() != utype.isArray() && expression.trueType() != OBJECT) {
        errors->createNewError(INCOMPATIBLE_TYPES, utype.link->line, utype.link->col, "; cannot cast `" + expression.typeToString() + "` to `" + utype.typeToString() + "`");
        out.type = expression_unresolved;
        return;
    }

    if(expression.utype.type != UNDEFINED && utype.utype.type == expression.utype.type)
        createNewWarning(GENERIC, utype.link->line, utype.link->col, "redundant cast of type `" + expression.typeToString() + "` to `" + utype.typeToString() + "`");

    if(Parser::isspecial_native_type(utype.utype.referenceName)) {
        utype.utype.type = VAR;
    }

    out.type = expression_var;
    out.utype = utype.utype;
    out.literal = expression.literal;
    out.intValue = expression.intValue;

    if(utype.utype.referenceName == "_int8") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOV8, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (int8_t )expression.intValue;
        return;
    } else if(utype.utype.referenceName == "_int16") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOV16, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (int16_t )expression.intValue;
        return;
    } else if(utype.utype.referenceName == "_int32") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOV32, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (int32_t )expression.intValue;
        return;
    } else if(utype.utype.referenceName == "_int64") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOV64, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (int64_t )expression.intValue;
        return;
    } else if(utype.utype.referenceName == "_uint8") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOVU8, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (uint8_t )expression.intValue;
        return;
    } else if(utype.utype.referenceName == "_uint16") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOVU16, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (uint16_t )expression.intValue;
        return;
    } else if(utype.utype.referenceName == "_uint32") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOVU32, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (uint32_t )expression.intValue;
        return;
    } else if(utype.utype.referenceName == "_uint64") {
        pushExpressionToRegisterNoInject(expression, out, ebx);
        out.code.push_i64(SET_Ci(i64, op_MOVU64, ebx, 0, ebx));
        if(expression.literal)
            out.intValue = (uint64_t )expression.intValue;
        return;
    } else if(utype.utype.isArray() && utype.utype.type == VAR) {
        if(expression.trueType() == OBJECT || expression.trueType() == VAR) {
            out.func=expression.func;
            return;
        }
    } else if(!utype.utype.isArray() && utype.utype.type == VAR) {
        if(expression.trueType() == OBJECT) {
            pushExpressionToRegisterNoInject(expression, out, ebx);
            return;
        } else if(expression.utype.field != NULL && expression.utype.field->isEnum) {
            pushExpressionToRegisterNoInject(expression, out, ebx);
            return;
        } else if(expression.trueType() == CLASS && expression.utype.field != NULL) {
            ClassObject *klass = expression.utype.field->klass;
            if(klass->isEnum()) {
                pushExpressionToRegisterNoInject(expression, out, ebx);
                return;
            }
        }
        else if(expression.trueType() == VAR) {
            createNewWarning(GENERIC, utype.link->line, utype.link->col, "redundant cast of type `var` to `var`");
            return;
        }
    } else if(utype.trueType() == OBJECT) {
        if(expression.trueType()== VAR || expression.trueType() == CLASS || expression.trueType() == OBJECT) {
            if(utype.isArray() == expression.isArray()) {

                if(expression.trueType()== VAR) {
                    varToObject(expression, out);
                }
                return;
            }
        }
    }
//    switch(utype.utype.type) {
//        case fi8:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOV8, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOV8, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case fi16:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOV16, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOV16, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case fi32:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOV32, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOV32, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case fi64:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOV64, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOV64, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case fui8:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOVU8, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOVU8, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case fui16:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOVU16, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOVU16, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case fui32:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOVU32, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOVU32, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case fui64:
//            if(arg.type == expression_var) {
//                out.code.push_i64(SET_Ci(i64, op_MOVU64, ebx,0, ebx));
//                return;
//            } else if(arg.type == expression_field) {
//                pushExpressionToRegisterNoInject(arg, out, ebx);
//                out.code.push_i64(SET_Ci(i64, op_MOVU64, ebx, 0, ebx));
//                out.code.push_i64(SET_Ci(i64, op_MOVR, adx, 0, fp));
//                out.code.push_i64(SET_Ci(i64, op_SMOVR, ebx, 0, arg.utype.field->vaddr));
//                return;
//            }
//            break;
//        case VAR:
//            if(expression.type == expression_var || expression.type==expression_field) {
//                return;
//            } // TODO: do we even have to do anything here?
//            break;
//        case TYPEVOID:
//            errors->createNewError(GENERIC, utype.link->line, utype.link->col, "type `void` cannot be used as a cast");
//            return;
//        case OBJECT:
//            out.type = expression_objectclass;
//            if(expression.type == expression_lclass) {
//                return;
//            }
//            break;
//    }

    if(expression.type != expression_unresolved)
        errors->createNewError(INCOMPATIBLE_TYPES, utype.link->line, utype.link->col, "; cannot cast `" + expression.utype.typeToString() + "` to `" + utype.utype.typeToString() + "`");
    return;
}

void RuntimeEngine::varToObject(Expression &expression, Expression &out) {
    if(expression.literal) {
        out.code.push_i64(SET_Di(i64, op_MOVI, expression.intValue), ebx);
    } else
        pushExpressionToRegisterNoInject(expression, out, ebx);
    out.code.push_i64(SET_Di(i64, op_MOVI, 1), egx);
    out.code.push_i64(SET_Di(i64, op_NEWARRAY, egx));
    out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
    out.code.push_i64(SET_Di(i64, op_MOVI, 0), adx);
    out.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
    out.func = true;
}

Expression RuntimeEngine::parseCastExpression(Ast* pAst) {
    Expression expression(pAst), utype(pAst), arg(pAst);

    utype = parseUtype(pAst);
    arg = parseExpression(pAst->getSubAst(ast_expression));

    if(utype.type != expression_unknown &&
       utype.type != expression_unresolved) {
        switch(utype.type) {
            case expression_native:
                expression.code.inject(expression.code.size(), arg.code);
                parseNativeCast(utype, arg, expression);
                break;
            case expression_class:
                parseClassCast(utype, arg, expression);
                expression.func=arg.func;
                break;
            case expression_field:
                errors->createNewError(GENERIC, utype.link->line, utype.link->col, "cast expression of type `field` not allowed, must be of type `class`");
                break;
            default:
                errors->createNewError(GENERIC, utype.link->line, utype.link->col, "cast expression of type `" + utype.typeToString() + "` not allowed, must be of type `class`");
                break;
        }
    }

    expression.newExpression=arg.newExpression;
    return expression;
}

void RuntimeEngine::preIncClass(Expression& out, token_entity op, ClassObject* klass) {
    OperatorOverload* overload;
    List<Param> emptyParams;

    if(op.getTokenType() == _INC) {
        overload = klass->getPreIncOverload();
    } else {
        overload = klass->getPreDecOverload();
    }

    if(overload != NULL) {
        // add code to call overload
        out.func = true;
        if(!overload->isStatic())
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

        verifyMethodAccess(overload, out.link);
        out.code.push_i64(SET_Di(i64, op_MOVI, 0), ebx);
        out.code.push_i64(SET_Di(i64, op_RSTORE, ebx));


        out.code.push_i64(SET_Di(i64, op_CALL, overload->address));

        out.type = methodReturntypeToExpressionType(overload);
        if(out.type == expression_lclass) {
            out.utype.klass = overload->klass;
            out.utype.type = CLASS;
        }
    } else if(klass->hasOverload(stringToOp(op.getToken()))) {
        errors->createNewError(GENERIC, op.getLine(), op.getColumn(), "use of class `" + klass->getName() + "`; missing overload params for operator `"
                                                                      + klass->getFullName() + ".operator" + op.getToken() + "`");
        out.type = expression_var;
    } else {
        errors->createNewError(GENERIC, op.getLine(), op.getColumn(), "use of class `" + klass->getName() + "` must return an int to use `" + op.getToken() + "` operator");
        out.type = expression_var;
    }
}

Expression RuntimeEngine::parsePreInc(Ast* pAst) {
    Expression interm(pAst);
    token_entity entity = pAst->hasEntity(_INC) ? pAst->getEntity(_INC) : pAst->getEntity(_DEC);

    interm = parseExpression(pAst->getSubAst(0));
    Expression expression(interm);

    if(expression.isArray()){
        errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "expression must evaluate to an int to use `" + entity.getToken() + "` operator");
    } else {
        switch(expression.type) {
            case expression_var:
                if(interm.func) {
                    pushExpressionToRegister(interm, expression, ebx);
                    if(entity.getTokenType() == _INC)
                        expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                    else
                        expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                } else {
                    if(entity.getTokenType() == _INC)
                        expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                    else
                        expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                }
                break;
            case expression_field:
                if(interm.utype.field->type == CLASS) {
                    preIncClass(expression, entity, expression.utype.field->klass);
                    return expression;
                } else if(interm.utype.field->type == VAR) {
                    // increment the field
                    if(entity.getTokenType() == _INC) {
                        if(expression.utype.field->local) {
                            if(c_options.optimize) {
                                expression.code.push_i64(
                                        SET_Ci(i64, op_IADDL, 1,0 , interm.utype.field->address));
                                expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                            } else {
                                expression.code.push_i64(SET_Di(i64, op_MOVI, 1), ecx);
                                expression.code.push_i64(
                                        SET_Ci(i64, op_ADDL, ecx,0 , interm.utype.field->address));
                                expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                            }
                        }
                        else {
                            pushExpressionToRegisterNoInject(interm, expression, ebx);
                            expression.code.push_i64(SET_Di(i64, op_INC, ebx));
                            expression.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
                        }
                    }
                    else {
                        if(expression.utype.field->local) {
                            if(c_options.optimize) {
                                expression.code.push_i64(
                                        SET_Ci(i64, op_ISUBL, 1,0 , interm.utype.field->address));
                                expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                            } else {
                                expression.code.push_i64(SET_Di(i64, op_MOVI, 1), ecx);
                                expression.code.push_i64(
                                        SET_Ci(i64, op_SUBL, ecx, 0, interm.utype.field->address));
                                expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                            }
                        }
                        else {
                            pushExpressionToRegisterNoInject(interm, expression, ebx);
                            expression.code.push_i64(SET_Di(i64, op_DEC, ebx));
                            expression.code.push_i64(SET_Ci(i64, op_RMOV, adx, 0, ebx));
                        }
                    }

                } else if(interm.utype.field->type == OBJECT) {
                    errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "use of `" + entity.getToken() +
                                                                                          "` operator on field of type `dynamic_object` without a cast. Try ((SomeClass)dynamic_class)++");

                } else if(interm.utype.field->type != UNDEFINED){
                    errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(),
                                           "expression must evaluate to an int to use `" + entity.getToken() +
                                           "` operator");
                }
                break;
            case expression_lclass:
                if(interm.func)
                    expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                preIncClass(expression, entity, expression.utype.klass);
                return expression;
                break;
            case expression_objectclass:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "use of `" + entity.getToken() + "` operator on type `dynamic_object` without a cast. Try ((SomeClass)dynamic_class)++");
                break;
            case expression_null:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "value `null` cannot be used as var");
                break;
            case expression_native:
                errors->createNewError(UNEXPECTED_SYMBOL, entity.getLine(), entity.getColumn(), " `" + expression.utype.typeToString() + "`");
                break;
            case expression_string:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "increment on immutable string");
                break;
            case expression_unresolved:
                // do nothing
                break;
            default:
                errors->createNewError(GENERIC, entity.getLine(), entity.getColumn(), "expression must evaluate to an int to use `" + entity.getToken() + "` operator");
                break;
        }
    }

    expression.type = expression_var;
    return expression;
}

Expression RuntimeEngine::parseParenExpression(Ast* pAst) {
    Expression expression(pAst);

    long recursiveAndExprsTmp = recursiveAndExprssions;
    List<long> skipAddressesTmp; skipAddressesTmp.addAll(skipAddress);
    recursiveAndExprssions = 0;
    skipAddress.free();
    expression = parseExpression(pAst->getSubAst(ast_expression));
    skipAddress.addAll(skipAddressesTmp);
    recursiveAndExprssions=recursiveAndExprsTmp;

    if(pAst->hasSubAst(ast_dotnotation_call_expr)) {
        return parseDotNotationChain(pAst->getSubAst(ast_dotnotation_call_expr), expression, 0);
    }

    return expression;
}

void RuntimeEngine::notClass(Expression& out, ClassObject* klass, Ast* pAst) {
    List<Param> emptyParams;
    Method* overload;
    if((overload = klass->getOverload(oper_NOT, emptyParams)) != NULL) {
        // call overload operator

        verifyMethodAccess(overload, pAst);
        if(!overload->isStatic())
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));

        out.code.push_i64(SET_Di(i64, op_CALL, overload->address));

        out.type = methodReturntypeToExpressionType(overload);
        if(out.type == expression_lclass) {
            out.utype.klass = overload->klass;
            out.utype.type = CLASS;
        }
    } else if(klass->hasOverload(oper_NOT)) {
        errors->createNewError(GENERIC, pAst->line, pAst->col, "use of unary operator '!' on class `" + klass->getName() + "`; missing overload params for operator `"
                                                               + klass->getFullName() + ".operator!`");
    } else {
        // error cannot apply not to expression of type class
        errors->createNewError(GENERIC, pAst->line, pAst->col, "unary operator '!' cannot be applied to expression of type `" + klass->getFullName() + "`");
    }
}

Expression RuntimeEngine::parseNotExpression(Ast* pAst) {
    Expression expression(pAst);

    expression = parseExpression(pAst->getSubAst(ast_expression));
    expression.inCmtRegister = false;
    switch(expression.type) {
        case expression_var:
            // negate value
            if(expression.func)
                pushExpressionToRegisterNoInject(expression, expression, ebx);

            expression.code.push_i64(SET_Ci(i64, op_NOT, ebx,0, ebx));
            break;
        case expression_lclass:
            // check for !operator
            if(expression.func)
                expression.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            notClass(expression, expression.utype.klass, pAst);
            break;
        case expression_unresolved:
            // do nothing
            break;
        case expression_native:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + expression.utype.typeToString() + "`");
            break;
        case expression_class:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + expression.utype.typeToString() + "`");
            break;
        case expression_objectclass:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "unary operator '!' cannot be applied to dynamic_object, did you forget to add a cast?  i.e !((SomeClass)dynamic_class)");
            break;
        case expression_field:
            expression.type = expression_var;
            if(expression.utype.field->isNative()) {
                if(expression.utype.field->dynamicObject()) {
                    errors->createNewError(GENERIC, pAst->line, pAst->col, "use of `!` operator on field of type `dynamic_object` without a cast. Try !((SomeClass)dynamic_class)");
                } else if(expression.utype.field->isVar()) {
                    pushExpressionToRegisterNoInject(expression, expression, ebx);
                    expression.code.push_i64(SET_Ci(i64, op_NOT, ebx,0, ebx));
                }
            } else if(expression.utype.field->type == CLASS) {
                if(expression.utype.field->local)
                    expression.code.push_i64(SET_Di(i64, op_MOVL, expression.utype.field->address));
                notClass(expression, expression.utype.field->klass, pAst);
            } else {
                errors->createNewError(GENERIC, pAst->line, pAst->col, "field must evaluate to an int to use `!` operator");
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "unary operator '!' cannot be applied to null");
            break;
        case expression_string:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "unary operator '!' cannot be applied to immutable string");
            break;
        default:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "unary operator '!' cannot be applied to given expression");
            break;

    }

    expression.func = false;
    return expression;
}

Expression RuntimeEngine::parseUnary(token_entity operand, Expression& right, Ast* pAst) {
    Expression expression(pAst);

    switch(right.type) {
        case expression_var:
            if(c_options.optimize && right.literal) {
                double var = 0;

                if(operand == "+")
                    var = +right.intValue;
                else
                    var = -right.intValue;

                double intpart;
                if( modf( right.intValue, &intpart) != 0 ) {
                    // movbi
                    if((int64_t )var > DA_MAX || (int64_t )var < DA_MIN) {
                        stringstream ss;
                        ss << "integral number too large: " << var;
                        errors->createNewError(GENERIC, operand.getLine(), operand.getColumn(), ss.str());
                    }

                    expression.code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)var)), abs(get_low_bytes(var)));
                    expression.code.push_i64(SET_Ci(i64, op_MOVR, ebx,0, bmr));
                } else {
                    // movi
                    if(var > DA_MAX || var < DA_MIN) {
                        stringstream ss;
                        ss << "integral number too large: " << var;
                        errors->createNewError(GENERIC, operand.getLine(), operand.getColumn(), ss.str());
                    }
                    expression.code.push_i64(SET_Di(i64, op_MOVI, var), ebx);
                }
            } else {
                pushExpressionToRegister(right, expression, ebx);

                if(operand == "+")
                    expression.code.push_i64(SET_Ci(i64, op_IMUL, ebx,0, 1));
                else
                    expression.code.push_i64(SET_Ci(i64, op_IMUL, ebx,0, -1));
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Unary operator `" + operand.getToken() +
                                                                    "` cannot be applied to expression of type `" + right.typeToString() + "`");
            break;
        case expression_field:
            if(right.utype.field->isNative()) {
                // add var
                if(right.utype.field->isVar()) {
                    pushExpressionToRegister(right, expression, ebx);

                    if(operand == "+") {
                        expression.code.push_i64(SET_Ci(i64, op_IMUL, ebx,0, 1));
                    }
                    else {
                        expression.code.push_i64(SET_Ci(i64, op_IMUL, ebx,0, -1));
                    }
                } else {
                    errors->createNewError(GENERIC, pAst->line,  pAst->col, "Unary operator `" + operand.getToken() +
                                                                            "` cannot be applied to expression of type `" + right.typeToString() + "`");
                }
            } else if(right.utype.field->type == CLASS) {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Expression of type `" + right.typeToString() + "` is non numeric");
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_native:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + right.typeToString() + "`");
            break;
        case expression_lclass:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Expression of type `" + right.typeToString() + "` is non numeric");
            break;
        case expression_class:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + right.typeToString() + "`");
            break;
        case expression_void:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Unary operator `" + operand.getToken() +
                                                                    "` cannot be applied to expression of type `" + right.typeToString() + "`");
            break;
        case expression_objectclass:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Unary operator `" + operand.getToken() +
                                                                    "` cannot be applied to expression of type `" + right.typeToString() + "`");
            break;
        case expression_string:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Expression of type `" + right.typeToString() + "` is non numeric");
            break;
        default:
            break;
    }

    expression.type = expression_var;
    expression.link = pAst;
    return expression;
}

bool RuntimeEngine::addExpressions(Expression &out, Expression &leftExpr, Expression &rightExpr, token_entity operand, double* varout) {
    if(operand == "+")
        *varout = leftExpr.intValue + rightExpr.intValue;
    else if(operand == "-")
        *varout = leftExpr.intValue - rightExpr.intValue;
    else if(operand == "*")
        *varout = leftExpr.intValue * rightExpr.intValue;
    else if(operand == "/")
        *varout = leftExpr.intValue / rightExpr.intValue;
    else if(operand == "%")
        *varout = (int64_t)leftExpr.intValue % (int64_t)rightExpr.intValue;

    if(!isDClassNumberEncodable(*varout)) {
        return false;
    } else {
        if(out.code.size() >= 2 && (GET_OP(out.code.__asm64.get(0)) == op_MOVI
                                    || GET_OP(out.code.__asm64.get(0)) == op_MOVBI)){
            if(GET_OP(out.code.__asm64.get(0)) == op_MOVBI)
                out.code.__asm64.pop_back();
            out.code.__asm64.pop_back();
            out.code.__asm64.pop_back();
        }

        double intpart;

        if( modf( *varout, &intpart) != 0 ) {
            // movbi
            out.code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)*varout)), abs(get_low_bytes(*varout)));
            out.code.push_i64(SET_Ci(i64, op_MOVR, ebx,0, bmr));
        } else {
            // movi
            out.code.push_i64(SET_Di(i64, op_MOVI, *varout), ebx);
        }

        leftExpr.literal = true;
        leftExpr.intValue = *varout;
        leftExpr.code.free();
    }

    out.literal = true;
    out.intValue = *varout;
    return true;
}

Opcode RuntimeEngine::operandToOp(token_entity operand)
{
    if(operand == "+")
        return op_ADD;
    else if(operand == "-")
        return op_SUB;
    else if(operand == "*")
        return op_MUL;
    else if(operand == "/")
        return op_DIV;
    else if(operand == "%")
        return op_MOD;
    else
        return op_MOD;
}

Opcode RuntimeEngine::assignOperandToOp(token_entity operand)
{
    if(operand == "+=")
        return op_ADD;
    else if(operand == "-=")
        return op_SUB;
    else if(operand == "*=")
        return op_MUL;
    else if(operand == "/=")
        return op_DIV;
    else if(operand == "%=")
        return op_MOD;
    else
        return op_MOD;
}

bool RuntimeEngine::equalsVectorArray(Expression& left, Expression& right) {

    if(right.type == expression_native) {
        errors->createNewError(UNEXPECTED_SYMBOL, right.link->line, right.link->col, " `" + right.typeToString() + "`");
        return false;
    }

    switch(left.type) {
        case expression_native:
            return left.utype.type==right.utype.type;
            break;
        case expression_var:
            if(right.trueType() == VAR) {
                // add 2 vars
                return true;
            }
            break;
        case expression_null:
            if(right.type == expression_lclass || right.type == expression_objectclass) {
                return true;
            } else if(right.type == expression_class) {
                errors->createNewError(GENERIC, right.link->line,  right.link->col, "Class `" + right.typeToString() + "` must be lvalue");
                return false;
            }
            break;
        case expression_field:
            if(left.utype.field->isNative()) {
                // add var
                if(right.trueType() == OBJECT) {
                    if(left.trueType() == OBJECT) {
                        return true;
                    }
                } else if(right.type == expression_string || right.type == expression_null) {
                    return left.utype.field->isArray;
                } else if(right.trueType() == VAR || (right.trueType() != CLASS)) {
                    if(left.trueType() == VAR) {
                        return left.isArray() == right.isArray();
                    }
                }
            } else if(left.utype.field->type == CLASS) {
                if(right.type == expression_lclass) {
                    if(left.utype.field->klass->match(right.utype.klass)) {
                        return true;
                    }
                } else if(right.type == expression_class) {
                    if(left.utype.field->klass->match(right.utype.klass)) {
                        errors->createNewError(GENERIC, right.link->line,  right.link->col, "Class `" + right.typeToString() + "` must be lvalue");
                        return false;
                    }
                } else if(right.type == expression_field && right.utype.field->type == CLASS) {
                    if(right.utype.field->klass->match(left.utype.field->klass)) {
                        return true;
                    }
                } else {
                    List<Param> params;
                    List<Expression> exprs;
                    exprs.push_back(right);

                    expressionListToParams(params, exprs);
                    return left.utype.field->klass->getOverload(oper_EQUALS, params) != NULL;
                }
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_lclass:
            if(right.type == expression_lclass) {
                if(left.utype.klass->match(right.utype.klass)) {
                    return true;
                }
            } else if(right.type == expression_field) {
                if(left.utype.klass->match(right.utype.field->klass)) {
                    return true;
                }
            } else if(right.type == expression_class) {
                if(left.utype.klass->match(right.utype.klass)) {
                    errors->createNewError(GENERIC, right.link->line,  right.link->col, "Class `" + right.typeToString() + "` must be lvalue");
                    return false;
                }
            }
            break;
        case expression_class:
            if(right.type == expression_class) {
                if(left.utype.klass->match(right.utype.klass)) {
                    return true;
                }
            }
            break;
        case expression_void:
            if(right.type == expression_void) {
                return true;
            }
            break;
        case expression_objectclass:
            if(right.trueType() == OBJECT || right.trueType() == CLASS) {
                return true;
            }
            break;
        case expression_string:
            if(right.trueType() == VAR) {
                if(right.isArray()) {
                    return true;
                }
            }
            else if(right.type == expression_string) {
                return true;
            }
            break;
        default:
            break;
    }

    errors->createNewError(GENERIC, right.link->line,  right.link->col, "Expressions of type `" + left.typeToString() + "` and `" + right.typeToString() + "` are not compatible");
    return false;
}

bool RuntimeEngine::prototypeEquals(Field *proto, List<Param> params, FieldType rtype) {
    if(Param::match(proto->params, params, true)) {
        if(proto->returnType==rtype)
            return true;

    }
    return false;
}

Method *RuntimeEngine::fieldToFunction(Field *field, Expression &code) {
    List<AccessModifier> modifiers;
    modifiers.add(PUBLIC);
    modifiers.add(STATIC);
    RuntimeNote note;
    Method *fn = new Method("", "", NULL, field->params, modifiers,
            field->returnType, note, 0, false, false);
    fn->code.__asm64.appendAll(code.code.__asm64);
    fn->dynamicPointer=true;
    return fn;
}
