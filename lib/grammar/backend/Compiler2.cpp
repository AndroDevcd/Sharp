//
// Created by BNunnally on 5/9/2020.
//

#include <cmath>
#include "Compiler.h"
#include "../main.h"
#include "oo/ClassObject.h"
#include "ReferencePointer.h"
#include "data/Utype.h"
#include "../../runtime/register.h"
#include "Expression.h"
#include "data/Literal.h"
#include "../../runtime/symbols/Method.h"
#include "oo/Method.h"


void Compiler::resolveClassMutateMethods(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            resolveClassMethods(ast, currentClass);
        }
    }
}

void Compiler::compileClassMutateFields(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            compileClassFields(ast, currentClass);
        }
    }
}

void Compiler::compileClassMutateObfuscations(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            compileClassObfuscations(ast, currentClass);
        }
    }
}

void Compiler::compileClassMutateMethods(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            compileClassMethods(ast, currentClass);
        }
    }
}

void Compiler::compileClassMutateInitDecls(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            compileClassInitDecls(ast, currentClass);
        }
    }
}

void Compiler::resolveDelegateMutateMethods(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            resolveAllDelegates(ast, currentClass);
        }
    }
}

void Compiler::resolveAllMethods() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        Int totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    currModule = Obfuscater::getModule(parseModuleDecl(branch));
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch (branch->getType()) {
                case ast_class_decl:
                    resolveClassMethods(branch);
                    break;
                case ast_delegate_decl:
                case ast_method_decl:
                    resolveGlobalMethod(branch);
                    break;
                case ast_interface_decl:
                    resolveClassMethods(branch);
                    break;
                case ast_mutate_decl:
                    resolveClassMutateMethods(branch);
                    break;
                default:
                    /* ignore */
                    break;
            }

            CHECK_CMP_ERRORS(return;)
        }


        if(NEW_ERRORS_FOUND()){
            failedParsers.addif(current);
        }
        removeScope();
    }
}

void Compiler::resolveClassMutateFields(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            resolveClassFields(ast, currentClass);
        }
    }
}

void Compiler::resolveAllFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        Int totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if(branch->getType() == ast_module_decl) {
                    string module = parseModuleDecl(branch);
                    currModule = Obfuscater::getModule(module);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch (branch->getType()) {
                case ast_class_decl:
                    resolveClassFields(branch);
                    break;
                case ast_variable_decl:
                    resolveField(branch);
                    break;
                case ast_alias_decl:
                    resolveAlias(branch);
                    break;
                case ast_mutate_decl:
                    resolveClassMutateFields(branch);
                    break;
                default:
                    /* ignore */
                    break;
            }

            CHECK_CMP_ERRORS(return;)
        }


        if(NEW_ERRORS_FOUND()){
            failedParsers.addif(current);
        }
        removeScope();
    }
}

void Compiler::compileTypeIdentifier(ReferencePointer &ptr, Ast *ast) {
    if(ast->getSubAstCount() == 0) {
        ptr.classes.add(ast->getToken(0).getValue());
    } else
        compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
}

DataType Compiler::strToNativeType(string &str) {
    if(str == "var")
        return VAR;
    else if(str == "object")
        return OBJECT;
    else if(str == "_int8")
        return _INT8;
    else if(str == "_int16")
        return _INT16;
    else if(str == "_int32")
        return _INT32;
    else if(str == "_int64")
        return _INT64;
    else if(str == "_uint8")
        return _UINT8;
    else if(str == "_uint16")
        return _UINT16;
    else if(str == "_uint32")
        return _UINT32;
    else if(str == "_uint64")
        return _UINT64;
    else
        return UNDEFINED;
}

bool Compiler::isFieldInlined(Field* field) {
    if(field == NULL || !field->flags.find(flg_CONST))
        return false;

    for(unsigned int i = 0; i < inlinedFields.size(); i++) {
        if(inlinedFields.get(i).key == field)
            return true;
    }

    return false;
}

double Compiler::getInlinedFieldValue(Field* field) {
    for(unsigned int i = 0; i < inlinedFields.size(); i++) {
        if(inlinedFields.get(i).key==field)
            return inlinedFields.get(i).value;
    }

    return 0;
}

bool Compiler::isDClassNumberEncodable(double var) {
    return !((int64_t )var > DA_MAX);
}

int64_t Compiler::getLowBytes(double var) {
    stringstream ss;
    ss.precision(16);
    ss << var;
    string num = ss.str(), result = "";
    int iter=0;
    for(unsigned int i = 0; i < num.size(); i++) {
        if(num.at(i) == '.') {
            for(unsigned int x = i+1; x < num.size(); x++) {
                if(iter++ > 16)
                    break;
                result += num.at(x);
            }
            return strtoll(result.c_str(), NULL, 0);
        }
    }
    return 0;
}

void Compiler::inlineVariableValue(CodeHolder &code, Field *field) {
    if(field->isVar() && !field->isArray) {
        double value = getInlinedFieldValue(field);
        code.free();

        if (constantMap.indexof(value) != -1) {
            code.addIr(OpBuilder::ldc(EBX, constantMap.indexof(value)));
        } else
            code.addIr(OpBuilder::movi(value, EBX));

        code.getInjector(stackInjector)
                .addIr(OpBuilder::rstore(EBX));
    } else {
        errors->createNewError(INTERNAL_ERROR, field->ast->line, field->ast->col,
                               " could not get inlined field value."); // should never happen
    }
}

Field *Compiler::resolveEnum(string name) {
    Field* field;

    for (unsigned int i = 0; i < importMap.size(); i++) {
        if (importMap.get(i).key->name == current->getTokenizer()->file) {

            for(long j = 0; j < enums.size(); j++) {
                ClassObject *enumClass = enums.get(j);
                if(enumClass->owner != NULL && (enumClass->owner->isGlobalClass() || enumClass->owner->fullName == currentScope()->klass->fullName)) {
                    if ((field = enumClass->getField(name, false)) != NULL && importMap.get(i).value.find(enumClass->module)) {
                        return field;
                    }
                }
            }

            break;
        }
    }

    return nullptr;
}

bool Compiler::resolveHigherScopeSingularUtype(ReferencePointer &ptr, Utype* utype, Ast *ast) {
    bool utypeFound = false;

    if(currScope.size() >= 2) {
        DataEntity *resolvedUtype;
        string name = ptr.classes.get(0);
        List<Method*> functions;

        // we leave global processing to the code in resolveSingularUtype()
        RETAIN_BLOCK_TYPE(STATIC_BLOCK)
        for (long i = currScope.size() - 2; i > 0; i--) {
            ClassObject *currentClass = currScope.get(i)->klass;

            if((resolvedUtype = currentClass->getField(name, true)) != NULL) {
                resolveFieldUtype(utype, ast, resolvedUtype, name);
                utypeFound = true;
                break;
            } else if((resolvedUtype = currentClass->getChildClass(name)) != NULL
                      && (IS_CLASS_GENERIC(((ClassObject*)resolvedUtype)->getClassType())
                          && ((ClassObject*)resolvedUtype)->getGenericOwner() !=  NULL)) {
                resolveClassUtype(utype, ast, resolvedUtype);
                utypeFound = true;
                break;
            }  else if((resolvedUtype = currentClass->getChildClass(name)) != NULL && !IS_CLASS_GENERIC(((ClassObject*)resolvedUtype)->getClassType())) {
                resolveClassUtype(utype, ast, resolvedUtype);
                utypeFound = true;
                break;
            } else if(resolveExtensionFunctions(currentClass) &&
                    currentClass->getAllFunctionsByName(name, functions, true)) {
                resolveFunctionByNameUtype(utype, ast, name, functions);
                utypeFound = true;
                break;
            } else if((resolvedUtype = currentClass->getAlias(name, currentScope()->type != RESTRICTED_INSTANCE_BLOCK)) != NULL) {
                resolveAliasUtype(utype, ast, resolvedUtype);
                utypeFound = true;
                break;
            }
        }
        RESTORE_BLOCK_TYPE()
    }

    return utypeFound;
}

void Compiler::resolveSingularUtype(ReferencePointer &ptr, Utype* utype, Ast *ast) {
    DataEntity* resolvedUtype = NULL;
    string name = ptr.classes.get(0);
    List<Method*> functions;
    List<ClassObject*> resolvedClasses;
    ModuleData *module;

    if(ptr.mod != "") {
        module = Obfuscater::getModule(ptr.mod) == NULL
                 ? undefinedModule : Obfuscater::getModule(ptr.mod);

        if(module == undefinedModule) {
            errors->createNewError(GENERIC, ast->line, ast->col,
                                   " could not resolve module `" + ptr.mod + "`.");
        }
    } else module = NULL;

    if(parser::isOverrideOperator(name)) {
        name = "operator" + name;
    }

    if(ptr.mod != "")
        goto globalCheck;

    if(currentScope()->type >= INSTANCE_BLOCK && currentScope()->currentFunction != NULL
       && currentScope()->currentFunction->data.getLocalField(name) != NULL) {
        Field* field = currentScope()->currentFunction->data.getLocalField(name);

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);
        utype->getCode().instanceCaptured = true;

        if(!field->initialized && currentScope()->initializationCheck) {
            errors->createNewError(GENERIC, ast->line, ast->col,
                                   " use of unitialized local field `" + name + "`.");
        }

        if(field->getter == NULL && isFieldInlined(field)) {
            inlineVariableValue(utype->getCode(), field);
        } else {
            if (field->isVar()) {
                if (field->isArray || field->locality == stl_thread) {
                    if (field->locality == stl_thread) {
                        utype->getCode().addIr(OpBuilder::tlsMovl(field->address));

                        // we only do it for stl_thread var if it is not an array field
                        if (!field->isArray) {
                            utype->getCode().getInjector(incInjector).addIr(OpBuilder::inc(EBX));
                            utype->getCode().getInjector(decInjector).addIr(OpBuilder::dec(EBX));

                            if (field->type <= _UINT64) {
                                dataTypeToOpcode(field->type, EBX, EBX, utype->getCode().getInjector(incInjector));
                                dataTypeToOpcode(field->type, EBX, EBX, utype->getCode().getInjector(decInjector));
                            }

                            utype->getCode().getInjector(incInjector)
                              .addIr(OpBuilder::movi(0, ADX))
                              .addIr(OpBuilder::rmov(ADX, EBX));
                            utype->getCode().getInjector(decInjector)
                                    .addIr(OpBuilder::movi(0, ADX))
                                    .addIr(OpBuilder::rmov(ADX, EBX));

                            utype->getCode().getInjector(ebxInjector)
                                    .addIr(OpBuilder::movi(0, ADX))
                                    .addIr(OpBuilder::iaload(EBX, ADX));

                            utype->getCode().getInjector(stackInjector)
                                    .addIr(OpBuilder::movi(0, ADX))
                                    .addIr(OpBuilder::iaload(EBX, ADX))
                                    .addIr(OpBuilder::rstore(EBX));
                            return;
                        }
                    } else {
                        utype->getCode().addIr(OpBuilder::movl(field->address));
                    }

                    utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::pushObject());
                } else {

                    utype->getCode().getInjector(incInjector).addIr(OpBuilder::inc(EBX));
                    utype->getCode().getInjector(decInjector).addIr(OpBuilder::dec(EBX));

                    if (field->type <= _UINT64) {
                        dataTypeToOpcode(field->type, EBX, EBX, utype->getCode().getInjector(incInjector));
                        dataTypeToOpcode(field->type, EBX, EBX, utype->getCode().getInjector(decInjector));
                    }

                    utype->getCode().getInjector(incInjector)
                      .addIr(OpBuilder::smovr2(EBX, field->address));
                    utype->getCode().getInjector(decInjector)
                            .addIr(OpBuilder::smovr2(EBX, field->address));

                    utype->getCode().addIr(OpBuilder::loadl(EBX, field->address));
                    utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(EBX));
                }
            } else {
                if (field->locality == stl_thread) {
                    utype->getCode().addIr(OpBuilder::tlsMovl(field->address));
                } else {
                    utype->getCode().addIr(OpBuilder::movl(field->address));
                }

                utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::pushObject());
            }
        }
    } else if((resolvedUtype = currentScope()->klass->getField(name, true)) != NULL) {
        resolveFieldUtype(utype, ast, resolvedUtype, name);
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && primaryClass && (resolvedUtype = primaryClass->getField(name, true)) != NULL) {
        resolveFieldUtype(utype, ast, resolvedUtype, name);
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveEnum(name)) != NULL) {
        validateAccess((Field*)resolvedUtype, ast);
        utype->setType(utype_field);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(((Field*)resolvedUtype)->isArray);

        utype->getCode().addIr(OpBuilder::movg(resolvedUtype->owner->address));
        utype->getCode().addIr(OpBuilder::movn(resolvedUtype->address));

        utype->getCode().getInjector(stackInjector)
                .addIr(OpBuilder::pushObject());
        return;
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveClass(NULL, name, ast)) != NULL) {
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
        validateAccess((ClassObject*)resolvedUtype, ast);
        checkTypeInference(ast);
    } else if(((resolvedUtype = currentScope()->klass->getChildClass(name)) != NULL ||
    (primaryClass && (resolvedUtype = primaryClass->getChildClass(name)) != NULL))
              && (IS_CLASS_GENERIC(((ClassObject*)resolvedUtype)->getClassType())
                  && ((ClassObject*)resolvedUtype)->getGenericOwner() !=  NULL)) {
        resolveClassUtype(utype, ast, resolvedUtype);
    }  else if(((resolvedUtype = currentScope()->klass->getChildClass(name)) != NULL  ||
               (primaryClass && (resolvedUtype = primaryClass->getChildClass(name)) != NULL))
        && !(IS_CLASS_GENERIC(((ClassObject*)resolvedUtype)->getClassType()) && ((ClassObject*)resolvedUtype)->getGenericOwner() == NULL)) {
        resolveClassUtype(utype, ast, resolvedUtype);
    } else if(resolveExtensionFunctions(currentScope()->klass) &&
            currentScope()->klass->getAllFunctionsByName(name, functions, true) ||
               (primaryClass && resolveExtensionFunctions(primaryClass) &&
                    primaryClass->getAllFunctionsByName(name, functions, true))) {
        resolveFunctionByNameUtype(utype, ast, name, functions);
    } else if((resolvedUtype = currentScope()->klass->getAlias(name, currentScope()->type != RESTRICTED_INSTANCE_BLOCK)) != NULL
        || (primaryClass && (resolvedUtype = primaryClass->getAlias(name, currentScope()->type != RESTRICTED_INSTANCE_BLOCK)) != NULL)) {
        resolveAliasUtype(utype, ast, resolvedUtype);
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && resolveHigherScopeSingularUtype(ptr, utype, ast)) {}
    else if(currentScope()->klass->name == name) {
        resolvedUtype = currentScope()->klass;
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
        validateAccess((ClassObject*)resolvedUtype, ast);
        checkTypeInference(ast);
    } else if(primaryClass && primaryClass->name == name) {
        resolvedUtype = primaryClass;
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
        validateAccess((ClassObject*)resolvedUtype, ast);
        checkTypeInference(ast);
    }
    else {
        globalCheck:
        functions.free();

        if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveClass(module, name, ast)) != NULL) {
            return resolveClassUtype(utype, ast, resolvedUtype);
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveField(name, ast)) != NULL) {
            return resolveFieldUtype(utype, ast, resolvedUtype, name);
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && resolveFunctionByName(name, functions, ast)) {
            return resolveFunctionByNameUtype(utype, ast, name, functions);
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveAlias(module, name, ast)) != NULL) {
            return resolveAliasUtype(utype, ast, resolvedUtype);
        } else if((currentScope()->type != RESTRICTED_INSTANCE_BLOCK &&
                   resolveClass(generics, resolvedClasses, module, name + "<>", ast, true))
                  || currentScope()->klass->getChildClass(name + "<>", true)) {
            if(currentScope()->klass->getChildClass(name + "<>", true))
                resolvedClasses.add(currentScope()->klass->getChildClass(name + "<>", true));
            if(resolvedClasses.size() == 1) {
                stringstream helpfulMessage;
                helpfulMessage << "have you forgotten your type parameters? Were you possibly looking for class `"
                               << resolvedClasses.get(0)->fullName << "`?";
                errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col,
                                       " `" + name + "` " +
                                       helpfulMessage.str());
            } else {
                stringstream helpfulMessage;
                helpfulMessage << "I found a few generic classes that match `" << name << "`. Were you possibly looking for any of these?\n";
                helpfulMessage << "\t{";

                for(int i = 0; i < resolvedClasses.size(); i++) {
                    if(i >= 5) {
                        helpfulMessage << "...";
                        break;
                    }

                    ClassObject *klass = resolvedClasses.get(i);

                    helpfulMessage << "\n\t\t" << klass->fullName;
                    if((i+1) < resolvedClasses.size())
                        helpfulMessage << ", ";
                }

                helpfulMessage << "\n\t}\n";

                errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col,
                                       " `" + name + "` " +
                                       helpfulMessage.str());
            }
            return;
        } else if((currentScope()->type != RESTRICTED_INSTANCE_BLOCK &&
                   resolveClass(classes, resolvedClasses, module, name, ast, true))
                  || currentScope()->klass->getChildClass(name, true)) {
            stringstream helpfulMessage;
            helpfulMessage << "I found a few classes that match `" << name << "`. Were you possibly looking for any of these?\n";
            helpfulMessage << "\t{";

            if(currentScope()->klass->getChildClass(name, true))
                resolvedClasses.add(currentScope()->klass->getChildClass(name, true));

            for(int i = 0; i < resolvedClasses.size(); i++) {
                if(i >= 5) {
                    helpfulMessage << "...";
                    break;
                }

                ClassObject *klass = resolvedClasses.get(i);

                helpfulMessage << "\n\t\t" << klass->fullName ;
                if((i+1) < resolvedClasses.size())
                    helpfulMessage << ", ";
            }

            helpfulMessage << "\n\t}\n";

            errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col,
                                   " `" + name + "` " +
                                   helpfulMessage.str());
            return;
        }

        errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + name + "` ");
    }
}

void Compiler::resolveAliasUtype(Utype *utype, Ast *ast, DataEntity *resolvedAlias) {
    Alias* alias = (Alias*)resolvedAlias;
    compileAliasType(alias);

    if(alias->utype != NULL && alias->utype->getType() == utype_field) {
        Field *field = (Field*)alias->utype->getResolvedType();
        if(currentScope()->type == STATIC_BLOCK && !field->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->line, ast->col, "cannot access field `" + field->name + "` in static context");
        }
    }

    utype->copy(alias->utype);
    utype->getCode().free().inject(alias->utype->getCode());
    utype->getCode().copyInjectors(alias->utype->getCode());
    validateAccess(alias, ast);
    checkTypeInference(alias, ast);
}

void Compiler::resolveFunctionByNameUtype(Utype *utype, Ast *ast, string &name, List<Method *> &functions) {
    // we use this when we need a scpeific function signature to be returned
    Method *resolvedFunction = NULL;
    if(functions.size() > 1) {
        if (requiredSignature != NULL) {
            for (long i = 0; i < functions.size(); i++) {
                if (simpleParameterMatch(functions.get(i)->params, requiredSignature->params)
                    && functions.get(i)->utype->equals(requiredSignature->utype)) {
                    resolvedFunction = functions.get(i);
                    break;
                }
            }

            if(resolvedFunction == NULL) {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       "could not find a function with signature `" + requiredSignature->toString() + "` in the current scope");
                utype->copy(undefUtype);
                return;
            }
        } else {
            ambiguous:
            resolvedFunction = functions.get(0);
            if(!obfuscateMode) {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       "reference to function `" + functions.get(0)->name + "` is ambiguous");
            }
        }
    } else
        resolvedFunction = functions.get(0);

    utype->setType(utype_method);
    utype->setResolvedType(resolvedFunction);
    utype->setArrayType(false);
    if(!obfuscateMode && !functions.get(0)->flags.find(STATIC)) {
        errors->createNewError(GENERIC, ast->line, ast->col, " cannot get address from non static function `" + name + "` ");
    }

    utype->getCode().addIr(OpBuilder::movi(resolvedFunction->address, EBX));
    utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));
}

void Compiler::resolveClassUtype(Utype *utype, Ast *ast, DataEntity *resolvedClass) {
    utype->setType(utype_class);
    utype->setResolvedType(resolvedClass);
    utype->setArrayType(false);
    checkTypeInference(ast);
}

void Compiler::resolveFieldUtype(Utype *utype, Ast *ast, DataEntity *resolvedField, string &name) {
    Field* field = (Field*)resolvedField;
    if(currentScope()->type == STATIC_BLOCK && !field->flags.find(STATIC)) {
        errors->createNewError(GENERIC, ast->line, ast->col, "cannot access field `" + name + "` in static context");
    }

    compileFieldType(field);

    utype->setType(utype_field);
    utype->setResolvedType(field);
    utype->setArrayType(field->isArray);
    validateAccess(field, ast);

    if(field->getter == NULL && isFieldInlined(field)) {
        inlineVariableValue(utype->getCode(), field);
    } else {
        if(field->getter != NULL
           && (currentScope()->currentFunction == NULL ||
               !(currentScope()->currentFunction == field->setter || currentScope()->currentFunction == field->getter))) {
            if(field->locality == stl_stack && !field->flags.find(STATIC) && !utype->getCode().instanceCaptured) {
                utype->getCode().instanceCaptured = true;
                utype->getCode().addIr(OpBuilder::movl(0));
            }

            long codeSize = utype->getCode().size();
            if(field->locality == stl_stack && !field->flags.find(STATIC))
                utype->getCode().addIr(OpBuilder::pushObject());
            compileFieldGetterCode(utype->getCode(), field);

            codeSize = utype->getCode().size() - codeSize;
            utype->getCode().getInjector(getterInjector).addIr(codeSize);
        } else {
            if(field->locality == stl_thread) {
                utype->getCode().addIr(OpBuilder::tlsMovl(field->address));
            } else {
                if(field->flags.find(STATIC) || field->owner->isGlobalClass())
                    utype->getCode().addIr(OpBuilder::movg( field->owner->address));
                else if(!utype->getCode().instanceCaptured) {
                    utype->getCode().instanceCaptured = true;
                    utype->getCode().addIr(OpBuilder::movl(0));
                }

                long codeSize = utype->getCode().size();
                utype->getCode().addIr(OpBuilder::movn(field->address));

                codeSize = utype->getCode().size() - codeSize;
                utype->getCode().getInjector(getterInjector).addIr(codeSize);
            }

            if (field->isVar() && !field->isArray) {
                utype->getCode().getInjector(incInjector).addIr(OpBuilder::inc(EBX));
                utype->getCode().getInjector(decInjector).addIr(OpBuilder::dec(EBX));

                if (field->type <= _UINT64) {
                    dataTypeToOpcode(field->type, EBX, EBX, utype->getCode().getInjector(incInjector));
                    dataTypeToOpcode(field->type, EBX, EBX, utype->getCode().getInjector(decInjector));
                }

                utype->getCode().getInjector(incInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::rmov(ADX, EBX));
                utype->getCode().getInjector(decInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::rmov(ADX, EBX));

                utype->getCode().getInjector(ebxInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::checklen(ADX))
                        .addIr(OpBuilder::iaload(EBX, ADX));

                utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::checklen(ADX))
                        .addIr(OpBuilder::iaload(EBX, ADX))
                        .addIr(OpBuilder::rstore(EBX));
            } else
                utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::pushObject());
        }
    }
}

/**
 * This function does a simple compile of your code
 * It expects 2 things to already be taken care of
 *
 * 1. That the caller has cleaned up the code for getting the variable
 * 2. that the caller has properly pushed the appropriate instance onto the stack for the getter to be called
 * @param code
 * @param field
 */
void Compiler::compileFieldGetterCode(CodeHolder &code, Field *field) { // TODO: dont call this if we are in the getter or setter code
    if(field != NULL && field->utype != NULL && field->getter) {
        if(field->isVar() && !field->isArray) {
            code.addIr(OpBuilder::call(field->getter->address));

            code.getInjector(ebxInjector)
                    .addIr(OpBuilder::loadValue(EBX));
        } else {
            code.addIr(OpBuilder::call(field->getter->address));

            code.getInjector(ptrInjector)
                    .addIr(OpBuilder::popObject2());
        }

        code.getInjector(removeFromStackInjector)
                .addIr(OpBuilder::pop());
    }
}

/*
 * The purposes of this function is to "forward" process a field's type and value if it hasn't
 * been processed yet to prevent weird incorrect undefined errors from the compiler
 */
void Compiler::compileFieldType(Field *field) {
    if(field != NULL) {
        RETAIN_TYPE_INFERENCE(false)
        if (field->type == UNTYPED) {
            currScope.add(new Scope(field->owner, field->owner->isGlobalClass() ? GLOBAL_SCOPE : CLASS_SCOPE));
            resolveField(field->ast);
            removeScope();
        }

        if (!field->inlineCheck) {
            currScope.add(new Scope(field->owner, field->owner->isGlobalClass() ? GLOBAL_SCOPE : CLASS_SCOPE));
            inlineField(field->ast);
            removeScope();
        }
        RESTORE_TYPE_INFERENCE()
    }
}

/*
 * The purposes of this function is to "forward" process a field's type and value if it hasn't
 * been processed yet to prevent weird incorrect undefined errors from the compiler
 */
void Compiler::compileAliasType(Alias *alias) { // TODO: [REVISIT: talk about in the tutorial
    if(alias != NULL) {
        currScope.add(new Scope(alias->owner, alias->owner->isGlobalClass() ? GLOBAL_SCOPE : CLASS_SCOPE));
        resolveAlias(alias->ast);
        removeScope();
    }
}

void Compiler::resolveClassHeiarchy(DataEntity* data, bool fromClass, ReferencePointer& ptr, Utype* utype, Ast* ast) {
    bool lastReference;
    Utype *bridgeUtype = new Utype(); // we need this so we dont loose code when free is called

    RETAIN_BLOCK_TYPE(RESTRICTED_INSTANCE_BLOCK)
    RETAIN_SCOPE_CLASS(fromClass ? (ClassObject*)data : currentScope()->klass)
    if(!fromClass)
        utype->getCode().inject(ptrInjector);
    for(unsigned int i = 1; i < ptr.classes.size(); i++) {
        lastReference = (i+1) >= ptr.classes.size();

        string name = ptr.classes.at(i);
        ReferencePointer nextReference;
        nextReference.classes.add(name);

        bridgeUtype->getCode().free();
        bridgeUtype->getCode().instanceCaptured = true;
        bridgeUtype->setType(utype_unresolved);
        RETAIN_TYPE_INFERENCE(false)
        resolveSingularUtype(nextReference, bridgeUtype, ast);
        RESTORE_TYPE_INFERENCE()

        if(utype->getType() != utype_unresolved && utype->isArray()) {
            errors->createNewError(GENERIC, ast->line, ast->col, "dot notation access on array of type `" + utype->toString() + "`");
        } else if(utype->getType() != utype_unresolved && !utype->isArray() && utype->getResolvedType()->isVar()) {
            varErr:
            errors->createNewError(GENERIC, ast->line, ast->col, "dot notation access on field `" + utype->toString() + "` was not found to be a class");
            break;
        }

        if(bridgeUtype->getType() == utype_field) {
            Field *field = (Field*)bridgeUtype->getResolvedType();
            if(fromClass)
                utype->getCode().addIr(OpBuilder::movg( field->owner->address));

            if(isFieldInlined(field) || field->flags.find(STATIC)) {
                utype->getCode().free();
            }

            utype->getCode().inject(bridgeUtype->getCode());
            if(!lastReference)
                utype->getCode().inject(bridgeUtype->getCode().getInjector(ptrInjector));
            else {
                utype->getCode().copyInjectors(bridgeUtype->getCode());
            }

            compileFieldType(field);

            // if you had class K { static instance : K; } and you did { inst := K.instance.instance; }
            if(utype->getType() == utype_field && utype->getResolvedType()->guid == field->guid
               && (field->flags.find(STATIC) || field->locality == stl_thread)) {
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "redundant multiple dot notation access on field `" + field->toString() + "` which is static or a thread local");
            } else if(i == 1 && data->guid == field->guid
                      && (field->flags.find(STATIC) || field->locality == stl_thread))
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "redundant multiple dot notation access on field `" + field->toString() + "` which is static or a thread local");

            if(bridgeUtype->getClass() != NULL)
                currentScope()->klass = bridgeUtype->getClass();
            else if(!lastReference)
                errors->createNewError(GENERIC, ast->line, ast->col, "field `" + bridgeUtype->toString() + "` was not found to be a class");

            if(!obfuscateMode && fromClass && !field->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "cannot get field `" + field->name + "` from self in static context");
            }

            if(isFieldInlined(field) && lastReference) {
                bridgeUtype->setType(utype_literal);
                bridgeUtype->setArrayType(false);

                bridgeUtype->setResolvedType(new Literal(getInlinedFieldValue(field)));
            }
            fromClass = false;
        } else if(bridgeUtype->getType() == utype_method) {
            utype->getCode().free(); // getting method address are just vars so we dont need to call all the other code
            utype->getCode().inject(bridgeUtype->getCode());
            if(!lastReference)
                utype->getCode().inject(bridgeUtype->getCode().getInjector(ebxInjector));
            else {
                utype->getCode().copyInjectors(bridgeUtype->getCode());
            }

            Method *method = (Method*)bridgeUtype->getResolvedType();
            compileMethodReturnType(method, method->ast, false);

            if(!method->flags.find(STATIC))
                errors->createNewError(GENERIC, ast->line, ast->col, "cannot get address from not static function `" + bridgeUtype->toString() + "`");

            if(!fromClass)
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "strange way to get address of function `" + method->toString() +
                                                                          "` found. I would suggest doing `" + method->fullName + "` instead." );

            if(!lastReference) {
                errors->createNewError(GENERIC, ast->line, ast->col, "symbol `" + bridgeUtype->toString() +
                                                                     "` is a pointer to the respective function and this returns a var and cannot be used as a class field.");
                break;
            } else {
                utype->getCode().copyInjectors(bridgeUtype->getCode());
            }
        } else if(bridgeUtype->getType() == utype_class) {
            if(!fromClass)
                goto error;

            currentScope()->klass = (ClassObject*)bridgeUtype->getResolvedType();
            if(lastReference) {
                checkTypeInference(ast);
                utype->getCode().copyInjectors(bridgeUtype->getCode());
            }
        } else {
            error:
            errors->createNewError(GENERIC, ast->line, ast->col, " resolved type `" + bridgeUtype->toString() + "` must be a field or a method");
            break;
        }

        utype->copy(bridgeUtype);
    }

    bridgeUtype->free();
    delete bridgeUtype;
    RESTORE_BLOCK_TYPE()
    RESTORE_SCOPE_CLASS()
}

void Compiler::resolveUtype(ReferencePointer &ptr, Utype* utype, Ast *ast) {

    if(ptr.classes.singular()) {
        resolveSingularUtype(ptr, utype, ast);
    } else {
        string name = ptr.classes.at(0);
        ReferencePointer initialRefrence;
        initialRefrence.classes.add(name);
        initialRefrence.mod = ptr.mod;
        if(parser::isOverrideOperator(name)) {
            name = "operator" + name;
        }

        RETAIN_TYPE_INFERENCE(false)
        resolveSingularUtype(initialRefrence, utype, ast);
        RESTORE_TYPE_INFERENCE()
        if(utype->getType() == utype_method || utype->getType() == utype_function_ptr) {
            errors->createNewError(GENERIC, ast->line, ast->col, "symbol `" + utype->toString() +
                                                                 "` is a function pointer and cant be used as an instance field.");
        } else if(utype->getType() == utype_class) {
            resolveClassHeiarchy(utype->getResolvedType(), true, ptr, utype, ast);
        } else if(utype->getType() == utype_field) {
            if(((Field*)utype->getResolvedType())->utype && ((Field*)utype->getResolvedType())->utype->getClass()) {
                RETAIN_SCOPE_CLASS(utype->getClass())
                resolveClassHeiarchy(utype->getClass(), false, ptr, utype, ast);
                RESTORE_SCOPE_CLASS()
            }
            else
                resolveClassHeiarchy(utype->getResolvedType(), false, ptr, utype, ast);
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "symbol `" + utype->toString() +
                                                                 "` is not a class or field and cannot be used as such.");
        }
    }
}

void Compiler::compileFuncPtr(Utype *utype, Ast *ast) {
    Method* funPtr = new Method();
    List<Field*> params;

    compileMethodReturnType(funPtr, ast);
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list_opt));

    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    stringstream name;
    name << "funcPtr#" << guid;
    funPtr->name = name.str();
    funPtr->fullName = name.str();
    funPtr->type = FNPTR;
    funPtr->owner = currentScope()->klass; // we just need this so the compiler dosen't crash
    funPtr->fnType = fn_ptr;
    funPtr->module = currModule;
    funPtr->guid = guid++;
    funPtr->ast = ast;
    funPtr->flags.add(PUBLIC);
    funPtr->flags.add(STATIC);
    funPtr->params.addAll(params);
    funPtr->meta.copy(meta);

    utype->setType(utype_function_ptr);
    utype->setResolvedType(funPtr);
    checkTypeInference(ast);
}

Utype* Compiler::compileUtype(Ast *ast, bool instanceCaptured) {
    if(ast->getType() != ast_utype)
        throw runtime_error("check parseUtype()");

    // we need this code for complex expressions
    Utype *utype = new Utype();
    utype->getCode().instanceCaptured = instanceCaptured;
    ReferencePointer ptr;

    if(ast->getSubAst(ast_type_identifier)->hasSubAst(ast_func_ptr)) {
        compileFuncPtr(utype, ast->getSubAst(ast_type_identifier)->getSubAst(ast_func_ptr));
    } else {
        compileTypeIdentifier(ptr, ast->getSubAst(ast_type_identifier));
        if (ptr.classes.singular() && parser::isNativeType(ptr.classes.get(0))) {
            utype->setType(utype_native);
            DataEntity *native = new DataEntity();
            utype->setResolvedType(native);

            native->type = strToNativeType(ptr.classes.get(0));

            if (ast->hasToken(LEFTBRACE) && ast->hasToken(RIGHTBRACE)) {
                utype->setArrayType(true);
            }

            checkTypeInference(ast);
            ptr.free();
            return utype;
        } else if (ptr.classes.singular() && ptr.mod == "" && (IS_CLASS_GENERIC(currentScope()->klass->getClassType())
                   && currentScope()->klass->getKeys().find(ptr.classes.get(0)) &&
                   currentScope()->klass->isAtLeast(created)) || (primaryClass && IS_CLASS_GENERIC(primaryClass->getClassType())
                        && primaryClass->getKeys().find(ptr.classes.get(0)) && primaryClass->isAtLeast(created))) {

            Utype *keyType;
            if(currentScope()->klass->getKeys().indexof(ptr.classes.get(0)) != -1) {
                keyType = currentScope()->klass->getKeyTypes().get(
                        currentScope()->klass->getKeys().indexof(ptr.classes.get(0)));
            } else {
                keyType = primaryClass->getKeyTypes().get(
                        primaryClass->getKeys().indexof(ptr.classes.get(0)));
            }

            utype->copy(keyType);

            bool isArray = false;
            if (ast->hasToken(LEFTBRACE) && ast->hasToken(RIGHTBRACE)) {
                isArray = true;
            }

            checkTypeInference(ast);
            if (utype->isArray() && isArray) {
                errors->createNewError(GENERIC, ast, "Array-arrays are not supported.");
            } else if (isArray) {
                utype->setArrayType(true);
            }
            ptr.free();
            return utype;
        }

        resolveUtype(ptr, utype, ast);
    }

    if(ast->hasToken(LEFTBRACE) && ast->hasToken(RIGHTBRACE)) {
        utype->setArrayType(true);
    }

    if(utype->getType() == utype_unresolved) {
        utype->free();
        delete utype;
        utype = undefUtype;
    }

    ptr.free();
    return utype;
}

void Compiler::checkTypeInference(Ast *ast) {
    if(typeInference)
        errors->createNewError(GENERIC, ast, "Illegal assignment via use of type to be inferred as a value");
}

void Compiler::compileUtypeList(Ast *ast, List<Utype *> &types) {
    for(long i = 0; i < ast->getSubAstCount(); i++) {
        types.add(compileUtype(ast->getSubAst(i)));
    }
}

ClassObject* Compiler::compileGenericClassReference(ModuleData *mod, string &name, ClassObject* parent, List<Utype*> utypes, Ast *ast) {

    ClassObject *generic;
    if(parent != NULL) {
        generic = parent->getChildClass(name + "<>");
    } else {
        if(currentScope()->klass->getChildClass(name + "<>"))
            generic = currentScope()->klass->getChildClass(name + "<>");
        else generic = findClass(mod, name + "<>", generics);

        if(generic == NULL) {
            generic = currentScope()->klass->getGenericOwner();
        }

        if(generic != NULL && !generic->owner->isGlobalClass()) {
            parent = generic->owner;
        }
    }

    if(generic && utypes.size() == generic->getKeys().size()) {
        stringstream ss;

        // build unique class name
        string trimmedName = generic->name;
        trimmedName.erase(trimmedName.end()-2, trimmedName.end());
        ss << trimmedName << "<";
        for(long i = 0; i < utypes.size(); i++) {
            ss << utypes.get(i)->toString();

            if((i+1) < utypes.size()) {
                ss << ",";
            }
        }
        ss << ">";
        name = ss.str();

        ClassObject *actualClass = parent != NULL ? parent->getChildClass(name) : findClass(mod, name, classes);
        if(actualClass == NULL)
        {
            List<AccessFlag> flags;
            flags.addAll(generic->flags);
            ClassObject *newClass = NULL;
            if(parent == NULL) {
                ModuleData* old = currModule;
                currModule = mod;
                newClass = addGlobalClassObject(name, flags, ast, classes);
                currModule = old;
            }
            else {
                newClass = addChildClassObject(name, flags, parent, ast);
            }

            Meta tmp;
            tmp.copy(newClass->meta);
            newClass->transfer(generic);

            stringstream fullName;
            fullName << newClass->module->name << "#" << name;

            newClass->meta.copy(tmp);
            newClass->fullName = fullName.str();
            newClass->name = name;
            newClass->setProcessStage(created);
            newClass->owner = parent ? parent : findClass(currModule, globalClass, classes);
            newClass->addKeyTypes(utypes);
            newClass->setGenericOwner(generic);

            RETAIN_SCOPE_CLASS(newClass)
            if(processingStage >= POST_PROCESSING) {
                newClass->setProcessStage(preprocessed);

                parser* oldParser = current;
                Int totalErrors = errors->getUnfilteredErrorCount();

                current = getParserBySourceFile(newClass->getGenericOwner()->meta.file->name);
                updateErrorManagerInstance(current);

                // preprocess code
                preProccessClassDecl(newClass->ast, IS_CLASS_INTERFACE(newClass->getClassType()), newClass);
                resolveSuperClass(newClass->ast, newClass);

                inheritEnumClassHelper(newClass->ast, newClass);
                inheritObjectClassHelper(newClass->ast, newClass);
                resolveClassMethods(newClass->ast, newClass);
                resolveExtensionFunctions(newClass);
                resolveUnprocessedClassMutations(newClass);

                if(processingStage == COMPILING) {
                    // post processing code
                    newClass->setProcessStage(postprocessed);
                    resolveAllDelegates(newClass->ast, newClass);
                    resolveClassFields(newClass->ast, newClass);
                    inlineClassFields(newClass->ast, newClass);
                    resolveGenericFieldMutations(newClass);

                    newClass->setProcessStage(compiled);
                    compileClassObfuscations(newClass->ast, newClass);
                    compileClassFields(newClass->ast, newClass);
                    compileClassInitDecls(newClass->ast, newClass);
                    compileClassMethods(newClass->ast, newClass);
                } else // we need to make sure we don't double process the class
                    unProcessedClasses.add(newClass);

                current = oldParser;
                updateErrorManagerInstance(current);

                if(NEW_ERRORS_FOUND()) {
                    // this helps the user find where they went wrong
                    printNote(newClass->meta, "in generic `" + newClass->name + "`");
                }
            } else {
                unProcessedClasses.add(newClass);
            }

            RESTORE_SCOPE_CLASS()
            return newClass;
        } else
            return actualClass;
    }

    return NULL;
}

ClassObject* Compiler::compileGenericClassReference(ModuleData *mod, string &name, ClassObject* parent, Ast *ast) {
    List<Utype*> utypes;
    compileUtypeList(ast, utypes);

    for(long i = 0; i < utypes.size(); i++) {
        if(utypes.get(i)->getType() == utype_unresolved) {
            return NULL;
        }
    }


    return compileGenericClassReference(mod, name, parent, utypes, ast);
}

void Compiler::compileReferencePtr(ReferencePointer &ptr, Ast* ast) {
    bool hashFound = ast->hasToken("#"), hashProcessed = false;
    stringstream module;
    long genericListPos = 0;
    ClassObject *parent = NULL;
    bool failed = false;
    for(long i = 0; i < ast->getTokenCount(); i++) {
        if(hashFound && !hashProcessed) {
            module << ast->getToken(i).getValue();
            if(ast->getToken(i + 1) == "#") {
                hashProcessed = true;
                ptr.mod = module.str();
                i++;
            }
        } else {
            string name = ast->getToken(i).getValue();
            if(name != ".") {
                if (!failed && ((i + 1) < ast->getTokenCount()) && ast->getToken(i + 1) == "<") {
                    RETAIN_TYPE_INFERENCE(false)
                    ClassObject *genericClass = compileGenericClassReference(
                            Obfuscater::getModule(ptr.mod), name, parent, ast->getSubAst(genericListPos++));
                    ptr.classes.add(genericClass ? genericClass->name : "?");

                    parent = genericClass;
                    failed = parent == NULL;
                    i += 2;
                    RESTORE_TYPE_INFERENCE()
                } else {
                    ptr.classes.add(name);

                    if(!failed) {
                        if (parent == NULL)
                            parent = resolveClass(Obfuscater::getModule(ptr.mod), name, ast);
                        else {
                            parent = parent->getChildClass(name);
                            failed = parent == NULL;
                        }
                    }
                }
            }
        }
    }
}

ClassObject *Compiler::resolveClassReference(Ast *ast, ReferencePointer &ptr, bool allowGenerics) {
    Alias *alias;
    if(ptr.classes.singular()) {
        ClassObject* klass = resolveClass(Obfuscater::getModule(ptr.mod), ptr.classes.get(0), ast);
        if(klass == NULL) { // class foo { k : list<>; class list<t> { class fooClass {}} }
            if((klass = findClass(Obfuscater::getModule(ptr.mod), ptr.classes.get(0) + "<>", generics))) {
                if(allowGenerics) // this is for extension functions
                    return klass;
                else
                    errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0)
                                                                   + "`. It was found to be a generic class, have you missed the key parameters to the class perhaps?");
            } else if((alias = currentScope()->klass->getAlias(ptr.classes.get(0), true)) != NULL
                      || (alias = resolveAlias(Obfuscater::getModule(ptr.mod), ptr.classes.get(0), ast)) != NULL) {
                compileAliasType(alias); // TODO: support local alises inside functions
                validateAccess(alias, ast);

                if(alias->utype->getType() == utype_class) {
                    return alias->utype->getClass();
                } else
                    errors->createNewError(GENERIC, ast, "symbol `" + alias->utype->toString() + "` is not a class");
            } else {
                errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0) + "`");
            }
        } else {
            return klass;
        }
    } else {
        ClassObject* klass = resolveClass(Obfuscater::getModule(ptr.mod), ptr.classes.get(0), ast);
        if(klass == NULL) {
            if(findClass(Obfuscater::getModule(ptr.mod), ptr.classes.get(0) + "<>", generics)) {
                errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0)
                                                               + "`. It was found to be a generic class, have you missed the key parameters to the class perhaps?");
            } else if((alias = currentScope()->klass->getAlias(ptr.classes.get(0), true)) != NULL
                      || (alias = resolveAlias(Obfuscater::getModule(ptr.mod), ptr.classes.get(0), ast)) != NULL) {
                compileAliasType(alias);
                validateAccess(alias, ast);

                if(alias->utype->getType() == utype_class) {
                    klass = alias->utype->getClass();
                    goto resolveHeiarchy;
                } else
                    errors->createNewError(GENERIC, ast, "symbol `" + alias->utype->toString() + "` is not a class");
            } else {
                errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0) + "`");
            }
        } else {
            resolveHeiarchy:
            Utype utype;
            string name = ptr.classes.last();
            ptr.classes.pop_back();

            if(ptr.classes.size() > 1) {
                RETAIN_SCOPE_CLASS(klass)
                resolveClassHeiarchy(klass, true, ptr, &utype, ast);
                RESTORE_SCOPE_CLASS()
            } else {
                utype.setType(utype_class);
                utype.setResolvedType(klass);
            }

            if(utype.getType() == utype_class) {
                klass = utype.getClass();
                ClassObject *subClass;

                if((subClass = klass->getChildClass(name)) != NULL) {
                    return subClass;
                } else if((subClass = klass->getChildClass(name + "<>")) != NULL) {
                    if(allowGenerics) // this is for extension functions
                        return subClass;
                    else
                        errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0)
                                                                       + "`. It was found to be a generic class, have you missed the key parameters to the class perhaps?");
                } else {
                    errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + name + "`");
                }
            } else {
                errors->createNewError(GENERIC, ast, "symbol `" + utype.toString() + "` is not a class");
            }
        }
    }

    return NULL;
}

ClassObject *Compiler::resolveBaseClass(Ast *ast) {
    ClassObject* base=NULL;
    if(ast->hasSubAst(ast_refrence_pointer)) {

        ReferencePointer ptr;
        compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
        base = resolveClassReference(ast, ptr);

        if (base != NULL) {
            if ((currentScope()->klass->isClassRelated(base) ||
                 base->isClassRelated(currentScope()->klass))) {
                errors->createNewError(GENERIC, ast->getSubAst(0)->line, ast->getSubAst(0)->col,
                                       "cyclic dependency of class `" + currentScope()->klass->fullName + "` in parent class `" +
                                       base->fullName + "`");
                return NULL;
            }

            if(IS_CLASS_INTERFACE(base->getClassType()) && !IS_CLASS_INTERFACE(currentScope()->klass->getClassType())) {
                errors->createNewError(GENERIC, ast->getSubAst(0)->line, ast->getSubAst(0)->col,
                                       "class `" + currentScope()->klass->fullName + "` cannot inherit interface `" +
                                       base->fullName + "` as a base class. Try this instead: `class " + currentScope()->klass->name + " base YourBaseClass : " + base->name + " { .. }`");
                return NULL;
            }
        }
    }

    return base;
}

void Compiler::parseReferencePointerList(List<ReferencePointer*> &refPtrs, Ast *ast) {
    for(long i = 0; i < ast->getSubAstCount(); i++) {
        refPtrs.add(new ReferencePointer());
        compileReferencePtr(*refPtrs.last(), ast->getSubAst(i));
    }
}


void Compiler::resolveSuperClass(Ast *ast, ClassObject* currentClass) {
    Ast* block = ast->getSubAst(ast_block), *branch;

    if(currentClass == NULL) {
        string name = ast->getToken(0).getValue();
        if(ast->getType() == ast_generic_class_decl || ast->getType() == ast_generic_interface_decl)
            name += "<>";

        if (globalScope()) {
            currentClass = findClass(currModule, name, ast->getType() == ast_generic_class_decl ||
                                                       ast->getType() == ast_generic_interface_decl ?
                                                       generics : classes);
        } else {
            currentClass = currentScope()->klass->getChildClass(name);
        }
    }

    if(currentClass != NULL) {
        currScope.add(new Scope(currentClass, CLASS_SCOPE));
        ClassObject *base = resolveBaseClass(ast);

        if (base != NULL) {
            if (IS_CLASS_INTERFACE(currentClass->getClassType())) {
                if (!IS_CLASS_INTERFACE(base->getClassType())) {
                    stringstream err;
                    err << "interface '" << currentClass->fullName << "' must inherit another interface class";
                    errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                    err.str("");
                    goto inheritInterfaces;
                }
            } else if(ends_with(base->fullName, "#" + globalClass)) {
                stringstream err;
                err << "class '" << currentClass->fullName << "' cannot inherit god level class `"
                    << base->fullName << "` as a base class";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                err.str("");
                goto inheritInterfaces;
            }

            if (currentClass->getSuperClass()) { // TODO: do we want to allow inheriting an enum???
                stringstream err;
                err << "class '" << currentClass->fullName << "' already has a base class of `"
                    << currentClass->getSuperClass()->fullName << "` and cannot Be overridden";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                err.str("");
            } else
                currentClass->setSuperClass(base);
        } else {
            if(IS_CLASS_ENUM(currentClass->getClassType())) {
                inheritEnumClassHelper(ast, currentClass);
            } else
                inheritObjectClassHelper(ast, currentClass);
        }

        inheritInterfaces:
        // we also want to resolve any interfaces as well
        if (ast->hasSubAst(ast_reference_pointer_list)) {
            List<ReferencePointer *> refPtrs;
            List<ClassObject *> interfaces;
            interfaces.addAll(currentClass->getInterfaces());

            parseReferencePointerList(refPtrs, ast->getSubAst(ast_reference_pointer_list));

            for (long i = 0; i < refPtrs.size(); i++) {
                branch = ast->getSubAst(ast_reference_pointer_list)->getSubAst(i);
                ClassObject *_interface = resolveClassReference(branch, *refPtrs.get(i));

                if (_interface != NULL) {
                    if (!IS_CLASS_INTERFACE(_interface->getClassType())) {
                        stringstream err;
                        err << "class `" + _interface->name + "` is not an interface";
                        errors->createNewError(GENERIC, branch->line, branch->col, err.str());
                    } else if (!interfaces.addif(_interface)) {
                        stringstream err;
                        err << "duplicate interface '" << _interface->name << "'";
                        errors->createNewError(GENERIC, branch->line, branch->col, err.str());
                    }
                }
            }

            currentClass->getInterfaces().addAll(interfaces);
            interfaces.free();
            freeListPtr(refPtrs);
        }

        if(block != NULL) {
            for (long i = 0; i < block->getSubAstCount(); i++) {
                branch = block->getSubAst(i);
                CHECK_CMP_ERRORS(return;)

                switch (branch->getType()) {
                    case ast_class_decl:
                        resolveSuperClass(branch);
                        break;
                    case ast_enum_decl:
                        resolveSuperClass(branch);
                        break;
                    case ast_interface_decl:
                        resolveSuperClass(branch);
                        break;
                    default:
                        break;
                }
            }
        }
        removeScope();
    }
}

parser *Compiler::getParserBySourceFile(string name) {
    for(Int i = 0; i < parsers.size(); i++) {
        if(parsers.get(i)->getTokenizer()->file == name)
            return parsers.get(i);
    }
    return NULL;
}

void Compiler::preProcessUnprocessedClasses(Int unstableClasses) {
    // we need this size just incase any classes get added after the fact to prevent double preprocessing
    Int size = unstableClasses;

    for(Int i = 0; i < size; i++) {
        ClassObject *unprocessedClass = unProcessedClasses.get(i);

        currModule = unprocessedClass->module;
        current = getParserBySourceFile(unprocessedClass->meta.file->name);

        Int totalErrors = errors->getUnfilteredErrorCount();
        updateErrorManagerInstance(current);

        // bring the classes up to speed
        unprocessedClass->setProcessStage(preprocessed);
        preProccessClassDecl(unprocessedClass->ast, IS_CLASS_INTERFACE(unprocessedClass->getClassType()), unprocessedClass);
        resolveSuperClass(unprocessedClass->ast, unprocessedClass);
        inheritEnumClassHelper(unprocessedClass->ast, unprocessedClass);
        inheritObjectClassHelper(unprocessedClass->ast, unprocessedClass);
        resolveClassMethods(unprocessedClass->ast, unprocessedClass);
        resolveUnprocessedClassMutations(unprocessedClass);

        if(NEW_ERRORS_FOUND()) {
            failedParsers.addif(current);
        }
    }
}

/**
 * This function is for generic classes only,
 * we only care about the vital processing of mutated data
 * everything else magically gets resolved
 * @param unprocessedClass
 */
void Compiler::resolveUnprocessedClassMutations(ClassObject *unprocessedClass) {
    if(IS_CLASS_GENERIC(unprocessedClass->getClassType()) && unprocessedClass->getGenericOwner() != NULL) {
        List<Ast *> &mutationTree = unprocessedClass->getGenericOwner()->getClassMutations();
        for (long z = unprocessedClass->processedMutations; z < mutationTree.size(); z++) {
            preProcessMutation(mutationTree.get(z), unprocessedClass);
            resolveClassMethods(mutationTree.get(z), unprocessedClass);
            resolveAllDelegates(mutationTree.get(z), unprocessedClass);
            unprocessedClass->processedMutations++;
        }
    }
}

void Compiler::resolveBaseClasses() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        Int totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for(unsigned long x = 0; x < current->size(); x++)
        {
            Ast *branch = current->astAt(x);
            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    currModule = Obfuscater::getModule(parseModuleDecl(branch));
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch(branch->getType()) {
                case ast_class_decl:
                    resolveSuperClass(branch);
                    break;
                case ast_interface_decl:
                    resolveSuperClass(branch);
                    break;
                case ast_enum_decl:
                    resolveSuperClass(branch);
                    break;
                default:
                    /* ignore */
                    break;
            }

            CHECK_CMP_ERRORS(return;)
        }


        if(NEW_ERRORS_FOUND()){
            failedParsers.addif(current);
        }
        removeScope();
    }
}

void Compiler::preProcessGenericClasses(Int unstableClasses) {
    /**
     * There is a bit of a tug of war going on with the order in how the compiler processes elements.
     * In the pre-processing stage we process the base classes of every single class in our code that we found, subclasses and all.
     * This means that In that time frame from pre-processing to now we could have prematurely tried to create a generic class due to it
     * being used as a base class.
     *
     * This then requires us to "protect" and ensure that all classes before post processing that were created be processed along with every other
     * class. Doing this however also creates another conundrum where any classes created after pre-processing will successfully pre-process themselves
     * without me needing to hold their hand. Meaning that we can only do this dance with the select few classes created during the pre-processing stage
     * and every other class will either work its-self out or be post-processed later.
     */
    preProcessUnprocessedClasses(unstableClasses);
}

void Compiler::inlineFieldHelper(Ast* ast) {
    string name = ast->getToken(0).getValue();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getToken(1).getValue();
    }

    Field *field = currentScope()->klass->getField(name, false);
    if(!field->inlineCheck) {
        field->inlineCheck = true;

        if(field->flags.find(flg_CONST) && ast->hasSubAst(ast_expression)) {
            if(field->utype != NULL && field->utype->getClass() && IS_CLASS_ENUM(field->utype->getClass()->getClassType()))
                return;

            Expression expr;
            compileExpression(&expr, ast->getSubAst(ast_expression));

            inlineField(field, expr);
        }
    }
}

void Compiler::inlineField(Field *field, Expression &expr) {
    if(expr.utype->isRelated(field->utype)) {
        if (expr.type == exp_var && utypeToExpressionType(field->utype) == exp_var) {
            if (expr.utype->getType() == utype_field && isFieldInlined((Field *) expr.utype->getResolvedType())) {
                if (((Field *) expr.utype->getResolvedType())->isVar() &&
                    !((Field *) expr.utype->getResolvedType())->isArray)
                    inlinedFields.add(KeyPair<Field *, double>(field, getInlinedFieldValue(
                            (Field *) expr.utype->getResolvedType())));
            } else if (expr.utype->getType() == utype_literal) {
                Literal *literal = ((Literal *) expr.utype->getResolvedType());

                if (literal->literalType == numeric_literal) {
                    inlinedFields.add(KeyPair<Field *, double>(field, literal->numericData));
                }
            } else if (expr.utype->getType() == utype_method) {
                Method *method = ((Method *) expr.utype->getResolvedType());
                inlinedFields.add(KeyPair<Field *, double>(field, method->address));
            } else if (expr.utype->getType() == utype_class) {
                // just ignore it so we support class inlined fields we mainly use this function just to validate
                // inlining values but we only want to add numeric const fields to the list considering thats not a value
                // that need to be allocated on the stack
            } else {
                errors->createNewError(GENERIC, expr.ast,
                                       " non constant value of type `" + expr.utype->toString() +
                                       "` assigned to constant field `" + field->name + "`");
            }
        } else {
            errors->createNewError(GENERIC, expr.ast,
                                   " non constant value of type `" + expr.utype->toString() +
                                   "` assigned to constant field `" + field->name + "`");
        }
    } else if(isUtypeConvertableToNativeClass(field->utype, expr.utype)) { /* ignore */ }
    else {
        errors->createNewError(GENERIC, expr.ast->line, expr.ast->col,
                               " incompatible types, cannot convert `" + expr.utype->toString() + "` to `" +
                               field->utype->toString()
                               + "`.");
    }
}

void Compiler::inlineField(Ast* ast) {
    inlineFieldHelper(ast);

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
            inlineFieldHelper(ast->getSubAst(i));
        }
    }
}

bool Compiler::isWholeNumber(double value) {
    if (floor(value) == value)
        return true;
    else
        return false;
}


void Compiler::inlineClassFields(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();

    if(currentClass == NULL) {
        string name = ast->getToken(0).getValue();
        if(globalScope()) {
            currentClass = findClass(currModule, name, classes);
        }
        else {
            currentClass = currentScope()->klass->getChildClass(name);
        }
    }

    if(currentClass != NULL) {
        currScope.add(new Scope(currentClass, CLASS_SCOPE));
        for (long i = 0; i < block->getSubAstCount(); i++) {
            Ast *branch = block->getSubAst(i);
            CHECK_CMP_ERRORS(return;)

            switch (branch->getType()) {
                case ast_class_decl:
                    inlineClassFields(branch);
                    break;
                case ast_variable_decl:
                    inlineField(branch);
                    break;
                case ast_mutate_decl:
                    inlineClassMutateFields(branch);
                    break;
                default:
                    /* ignore */
                    break;
            }
        }
        removeScope();
    }
}

void Compiler::inlineClassMutateFields(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);

    if(currentClass != NULL) {
        if (IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            // do nothing it's allready been processed
        } else {
            inlineClassFields(ast, currentClass);
        }
    }
}

void Compiler::inlineFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        Int totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if(branch->getType() == ast_module_decl) {
                    currModule = Obfuscater::getModule(parseModuleDecl(branch));
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch (branch->getType()) {
                case ast_class_decl:
                    inlineClassFields(branch);
                    break;
                case ast_variable_decl:
                    inlineField(branch);
                    break;
                case ast_mutate_decl:
                    inlineClassMutateFields(branch);
                    break;
                default:
                    /* ignore */
                    break;
            }

            CHECK_CMP_ERRORS(return;)
        }


        if(NEW_ERRORS_FOUND()){
            failedParsers.addif(current);
        }
        removeScope();
    }
}

void Compiler::postProcessUnprocessedClasses() {
    for(Int i = 0; i < unProcessedClasses.size(); i++) {
        ClassObject *unprocessedClass = unProcessedClasses.get(i);

        currModule = unprocessedClass->module;
        current = getParserBySourceFile(unprocessedClass->meta.file->name);

        Int totalErrors = errors->getUnfilteredErrorCount();
        updateErrorManagerInstance(current);

        // bring the classes up to speed
        unprocessedClass->setProcessStage(postprocessed);
        resolveAllDelegates(unprocessedClass->ast, unprocessedClass);
        resolveClassFields(unprocessedClass->ast, unprocessedClass);
        inlineClassFields(unprocessedClass->ast, unprocessedClass);
        resolveExtensionFunctions(unprocessedClass);
        resolveGenericFieldMutations(unprocessedClass);

        // class has graduated but still is unprocessed so we must babysit the class...
        if(NEW_ERRORS_FOUND()) {
            failedParsers.addif(current);
        }
    }
}

void Compiler::postProcessGenericClasses() {
    /**
     * There is a bit of a tug of war going on with the order in how the compiler processes elements.
     * In the pre-processing stage we process the base classes of every single class in our code that we found, subclasses and all.
     * This means that In that time frame from pre-processing to now we could have prematurely tried to create a generic class due to it
     * being used as a base class.
     *
     * This then requires us to "protect" and ensure that all classes before post processing that were created be processed along with every other
     * class. Doing this however also creates another conundrum where any classes created after pre-processing will successfully pre-process themselves
     * without me needing to hold their hand. Meaning that we can only do this dance with the select few classes created during the pre-processing stage
     * and every other class will either work its-self out or be post-processed later.
     */
    postProcessUnprocessedClasses();
}

bool Compiler::postProcess() {
    processingStage = POST_PROCESSING;
    Int unstableClasses = unProcessedClasses.size();

    preprocessMutations();
    preProcessGenericClasses(unstableClasses);
    resolveAllMethods();
    resolveAllDelegates();
    resolveAllFields();
    inlineFields();
    postProcessEnumFields();
    postProcessGenericClasses();

    return !errors->hasErrors();
}

void Compiler::handleImports() {

    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        Int totalErrors = errors->getUnfilteredErrorCount();
        if(c_options.magic)
        { // import everything in magic mode
            importMap.__new().key = Obfuscater::getFile(current->getTokenizer()->file);
            importMap.last().value.init();
            importMap.last().value.addAll(Obfuscater::modules);
        } else
            preproccessImports();


        if(NEW_ERRORS_FOUND()){
            failedParsers.addif(current);
        }
    }
}



void Compiler::compileAsmStatement(Ast *ast) {
    CodeHolder &code = currentScope()->currentFunction->data.code;
    Ast *asmBlock = ast->getSubAst(ast_assembly_block);
    AsmData asmData;
    asmData.start_pc = code.size();

    for(Int i = 0; i < asmBlock->getSubAstCount(); i++) {
        Ast *branch = asmBlock->getSubAst(i);
        string opcode = branch->getToken(0).getValue();
        compileAssemblyInstruction(code, branch, opcode);
    }

    asmData.end_pc = code.size();
    if(!ast->hasToken("volatile")) {
        currentScope()->currentFunction->data.protectedCodeTable.add(asmData);
    }
}

void Compiler::compileAssemblyInstruction(CodeHolder &code, Ast *branch, string &opcode) {
    if(opcode == "nop") {
        code.addIr(OpBuilder::nop());
    } else if(opcode == "int") {
        code.addIr(OpBuilder::_int((interruptFlag) compileAsmLiteral(branch->getSubAst(ast_literal))));
    } else if(opcode == "movi") {
        code.addIr(OpBuilder::movi(compileAsmLiteral(branch->getSubAst(ast_assembly_literal)),
                                   compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "ret") {
        code.addIr(OpBuilder::ret(compileAsmLiteral(branch->getSubAst(ast_assembly_literal))));
    } else if(opcode == "hlt") {
        code.addIr(OpBuilder::hlt());
    } else if(opcode == "newVarArray") {
        code.addIr(OpBuilder::newVarArray(compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "cast") {
        code.addIr(OpBuilder::cast(compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "mov8") {
        code.addIr(OpBuilder::mov8(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "mov16") {
        code.addIr(OpBuilder::mov16(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "mov32") {
        code.addIr(OpBuilder::mov32(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "mov64") {
        code.addIr(OpBuilder::mov64(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "movu8") {
        code.addIr(OpBuilder::movu8(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "movu16") {
        code.addIr(OpBuilder::movu16(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "movu32") {
        code.addIr(OpBuilder::movu32(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "movu64") {
        code.addIr(OpBuilder::movu64(
                compileAsmRegister(branch->getSubAst(ast_assembly_register)),
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "rstore") {
        code.addIr(OpBuilder::rstore(
                compileAsmRegister(branch->getSubAst(ast_assembly_register))));
    } else if(opcode == "add") {
        code.addIr(OpBuilder::add(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmRegister(branch->getSubAst(2))));
    } else if(opcode == "sub") {
        code.addIr(OpBuilder::sub(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmRegister(branch->getSubAst(2))));
    } else if(opcode == "mul") {
        code.addIr(OpBuilder::mul(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmRegister(branch->getSubAst(2))));
    } else if(opcode == "div") {
        code.addIr(OpBuilder::div(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmRegister(branch->getSubAst(2))));
    } else if(opcode == "mod") {
        code.addIr(OpBuilder::mod(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmRegister(branch->getSubAst(2))));
    } else if(opcode == "iadd") {
        code.addIr(OpBuilder::iadd(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "isub") {
        code.addIr(OpBuilder::isub(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "imul") {
        code.addIr(OpBuilder::imul(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "idiv") {
        code.addIr(OpBuilder::idiv(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "imod") {
        code.addIr(OpBuilder::imod(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "pop") {
        code.addIr(OpBuilder::pop());
    } else if(opcode == "inc") {
        code.addIr(OpBuilder::inc(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "dec") {
        code.addIr(OpBuilder::dec(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "movr") {
        code.addIr(OpBuilder::movr(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "iaload") {
        code.addIr(OpBuilder::iaload(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "brh") {
        code.addIr(OpBuilder::brh());
    } else if(opcode == "ife") {
        code.addIr(OpBuilder::ife());
    } else if(opcode == "ifne") {
        code.addIr(OpBuilder::ifne());
    } else if(opcode == "lt") {
        code.addIr(OpBuilder::lt(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "gt") {
        code.addIr(OpBuilder::gt(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "le") {
        code.addIr(OpBuilder::le(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "ge") {
        code.addIr(OpBuilder::ge(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "movl") {
        code.addIr(OpBuilder::movl(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "movsl") {
        code.addIr(OpBuilder::movsl(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "sizeof") {
        code.addIr(OpBuilder::_sizeof(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "put") {
        code.addIr(OpBuilder::put(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "putc") {
        code.addIr(OpBuilder::putc(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "checklen") {
        code.addIr(OpBuilder::checklen(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "jmp") {
        code.addIr(OpBuilder::jmp(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "loadpc") {
        code.addIr(OpBuilder::loadpc(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "pushObj") {
        code.addIr(OpBuilder::pushObject());
    } else if(opcode == "del") {
        code.addIr(OpBuilder::del());
    } else if(opcode == "call") {
        code.addIr(OpBuilder::call(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "newClass") {
        code.addIr(OpBuilder::newClass(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "movn") {
        code.addIr(OpBuilder::movn(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "sleep") {
        code.addIr(OpBuilder::sleep(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "te") {
        code.addIr(OpBuilder::te(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "te") {
        code.addIr(OpBuilder::tne(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "lock") {
        code.addIr(OpBuilder::lock());
    } else if(opcode == "ulock") {
        code.addIr(OpBuilder::unlock());
    } else if(opcode == "movg") {
        code.addIr(OpBuilder::movg(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "movnd") {
        code.addIr(OpBuilder::movnd(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "movnd") {
        code.addIr(OpBuilder::movnd(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "newObjArray") {
        code.addIr(OpBuilder::newObjectArray(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "not") {
        code.addIr(OpBuilder::_not(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "skip") {
        code.addIr(OpBuilder::skip(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "loadVal") {
        code.addIr(OpBuilder::loadValue(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "shl") {
        code.addIr(OpBuilder::shl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmRegister(branch->getSubAst(2))));
    } else if(opcode == "shr") {
        code.addIr(OpBuilder::shr(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmRegister(branch->getSubAst(2))));
    } else if(opcode == "skipife") {
        code.addIr(OpBuilder::skipife(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "skipifne") {
        code.addIr(OpBuilder::skipifne(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "and") {
        code.addIr(OpBuilder::_and(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "uand") {
        code.addIr(OpBuilder::uand(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "or") {
        code.addIr(OpBuilder::_or(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "xor") {
        code.addIr(OpBuilder::_xor(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "throw") {
        code.addIr(OpBuilder::_throw());
    } else if(opcode == "checknull") {
        code.addIr(OpBuilder::checkNull(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "returnObj") {
        code.addIr(OpBuilder::returnObject());
    } else if(opcode == "newClassArray") {
        code.addIr(OpBuilder::newClassArray(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "newString") {
        code.addIr(OpBuilder::newString(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "addl") {
        code.addIr(OpBuilder::addl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "subl") {
        code.addIr(OpBuilder::subl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "mull") {
        code.addIr(OpBuilder::mull(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "divl") {
        code.addIr(OpBuilder::divl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "modl") {
        code.addIr(OpBuilder::modl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "iaddl") {
        code.addIr(OpBuilder::iaddl(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "isubl") {
        code.addIr(OpBuilder::isubl(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "imull") {
        code.addIr(OpBuilder::imull(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "idivl") {
        code.addIr(OpBuilder::idivl(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "imodl") {
        code.addIr(OpBuilder::imodl(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "loadl") {
        code.addIr(OpBuilder::loadl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "popObj") {
        code.addIr(OpBuilder::popObject());
    } else if(opcode == "smovr") {
        code.addIr(OpBuilder::smovr(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "andl") {
        code.addIr(OpBuilder::andl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "orl") {
        code.addIr(OpBuilder::orl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "xorl") {
        code.addIr(OpBuilder::xorl(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "rmov") {
        code.addIr(OpBuilder::rmov(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "smov") {
        code.addIr(OpBuilder::smov(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "returnVal") {
        code.addIr(OpBuilder::returnValue(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "xor") {
        code.addIr(OpBuilder::_xor(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == "istore") {
        code.addIr(OpBuilder::istore(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "smovr2") {
        code.addIr(OpBuilder::smovr2(
                compileAsmRegister(branch->getSubAst(1)),
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "smovr3") {
        code.addIr(OpBuilder::smovr3(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "istorel") {
        code.addIr(OpBuilder::istorel(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "popl") {
        code.addIr(OpBuilder::popl(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "pushNull") {
        code.addIr(OpBuilder::pushNull());
    } else if(opcode == "ipushl") {
        code.addIr(OpBuilder::ipushl(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "pushl") {
        code.addIr(OpBuilder::pushl(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "itest") {
        code.addIr(OpBuilder::itest(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "invokeDelegate") {
        code.addIr(OpBuilder::invokeDelegate(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1)),
                (bool) compileAsmLiteral(branch->getSubAst(2))));
    } else if(opcode == "get") {
        code.addIr(OpBuilder::get(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "isadd") {
        code.addIr(OpBuilder::isadd(
                compileAsmLiteral(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "je") {
        code.addIr(OpBuilder::je(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "jne") {
        code.addIr(OpBuilder::jne(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "ipopl") {
        code.addIr(OpBuilder::ipopl(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "cmp") {
        code.addIr(OpBuilder::cmp(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "calld") {
        code.addIr(OpBuilder::calld(
                compileAsmRegister(branch->getSubAst(0))));
    } else if(opcode == "varCast") {
        code.addIr(OpBuilder::varCast(
                compileAsmLiteral(branch->getSubAst(0)),
                (bool) compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "tlsMovl") {
        code.addIr(OpBuilder::tlsMovl(
                compileAsmLiteral(branch->getSubAst(0))));
    } else if(opcode == "dup") {
        code.addIr(OpBuilder::dup());
    } else if(opcode == "popObj2") {
        code.addIr(OpBuilder::popObject2());
    } else if(opcode == "swap") {
        code.addIr(OpBuilder::swap());
    } else if(opcode == "ldc") {
        code.addIr(OpBuilder::ldc(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmLiteral(branch->getSubAst(1))));
    } else if(opcode == "neg") {
        code.addIr(OpBuilder::neg(
                compileAsmRegister(branch->getSubAst(0)),
                compileAsmRegister(branch->getSubAst(1))));
    } else if(opcode == ".") {
        string name = branch->getToken(1).getValue();
        if(currentScope()->currentFunction->data.getLabel(name).start_pc == invalidAddr) {
            currentScope()->currentFunction->data.labelMap.add(LabelData(name, currentScope()->currentFunction->data.code.size(), currentScope()->scopeLevel));
        } else {

            this->errors->createNewError(PREVIOUSLY_DEFINED, branch->line, branch->col,
                                         "label `" + name + "` is already defined in the scope");
        }

        Ast *instrBranch = branch->getSubAst(ast_assembly_instruction);
        string instrOpcode = instrBranch->getToken(0).getValue();
        compileAssemblyInstruction(code, instrBranch, instrOpcode);
    }
}
