//
// Created by BraxtonN on 10/18/2019.
//

#include <random>
#include <cmath>
#include <chrono>
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
#include "ofuscation/Obfuscater.h"
#include "../generator/ExeBuilder.h"
#include "../../util/time.h"
#include "../optimizer/Optimizer.h"

string globalClass = "__srt_global";
ModuleData* undefinedModule = NULL;
string staticInitMethod = "static_init";
string tlsSetupMethod = "tls_init";
uInt Compiler::guid = 0;
void Compiler::generate() {
    try {
        Obfuscater obf(this);
        obf.obfuscate();

        ExeBuilder builder(this);
        builder.build();
    } catch(std::exception &e) {
        errors->createNewError(INTERNAL_ERROR, 0, 0, e.what());
    }
}

void Compiler::cleanup() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        parser* parser = parsers.get(i);
        parser->getTokenizer()->free();
        delete parser->getTokenizer();
        parser->free();
        delete(parser);
    }
    parsers.free();
}

AccessFlag Compiler::strToAccessFlag(string flag) {
    if(flag == "public")
        return PUBLIC;
    else if(flag == "private")
        return PRIVATE;
    else if(flag == "protected")
        return PROTECTED;
    else if(flag == "const")
        return flg_CONST;
    else if(flag == "static")
        return STATIC;
    else if(flag == "local")
        return LOCAL;
    else if(flag == "ext")
        return EXTENSION;
    else if(flag == "stable")
        return STABLE;
    else
        return flg_UNDEFINED;
}

bool Compiler::matchesFlag(AccessFlag flags[], int len, int startPos, AccessFlag flag) {
    for(int i = startPos; i < len; i++) {
        if(flags[i] == flag)
            return true;
    }
    return false;
}

void Compiler::parseClassAccessFlags(List<AccessFlag> &flags, Ast *ast) {
    if(ast->getTokenCount() > 4)
        this->errors->createNewError(GENERIC, ast->line, ast->col, "class objects only allows for access "
                                                                   "specifiers (public, private, protected, local, ext, and stable)");
    else {
        long errPos=0;
        AccessFlag lastFlag = flg_UNDEFINED;
        for(long i = 0; i < ast->getTokenCount(); i++) {
            AccessFlag flag = strToAccessFlag(ast->getToken(i).getValue());
            flags.add(flag);

            if(flag == flg_CONST || flag == STATIC) {
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                             " `" + ast->getToken(i).getValue() + "`");
            } else if(flag == lastFlag) {
                this->errors->createNewError(ILLEGAL_ACCESS_DECLARATION, ast->line, ast->col,
                                             " duplicate access flag `" + ast->getToken(i).getValue() + "`");
            }

            if(flag==LOCAL && !globalScope()) {
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                             " `" + ast->getToken(i).getValue() + "` can only be used at global scope");
            }
        }

        if(!flags.empty()) {            /* first part */               /* Second part */
            AccessFlag flagOrder[6] = { PUBLIC, PRIVATE, PROTECTED,    LOCAL, STABLE, EXTENSION };
            if (flags.get(0) <= PROTECTED) {
                if (flags.size() == 2) {
                    if(!matchesFlag(flagOrder, 6, 3, flags.get(1))) {
                        errPos = 1;
                        goto error;
                    }
                } else if (flags.size() == 3) {
                    errPos = 1;
                    if(matchesFlag(flagOrder, 6, 3, flags.get(errPos++))
                       && matchesFlag(flagOrder, 6, 4, flags.get(errPos++))) {
                    } else {
                        goto error;
                    }
                } else if (flags.size() == 4) {
                    errPos = 1;
                    if(matchesFlag(flagOrder, 6, 3, flags.get(errPos++))
                       && matchesFlag(flagOrder, 6, 4, flags.get(errPos++))
                          && matchesFlag(flagOrder, 6, 5, flags.get(errPos++))) {
                    } else {
                        goto error;
                    }
                } else if(flags.size() > 4) {
                    errPos = flags.size() - 1;
                    goto error;
                }
            } else if (flags.get(0) == LOCAL) {
                if (flags.size() == 2) {
                    if(!matchesFlag(flagOrder, 6, 4, flags.get(1))) {
                        errPos = 1;
                        goto error;
                    }
                } else if (flags.size() == 3) {
                    errPos = 1;
                    if(matchesFlag(flagOrder, 6, 4, flags.get(errPos++))
                       && matchesFlag(flagOrder, 6, 5, flags.get(errPos++))) {
                    } else {
                        goto error;
                    }
                } else if(flags.size() > 3) {
                    errPos = flags.size() - 1;
                    goto error;
                }
            } else if (flags.get(0) == STABLE) {
                if (flags.size() == 2) {
                    if (!matchesFlag(flagOrder, 6, 5, flags.get(1))) {
                        errPos = 1;
                        goto error;
                    }
                } else if(flags.size() > 2) {
                    errPos = flags.size() - 1;
                    goto error;
                }
            } else if (flags.get(0) == EXTENSION) {
                if (flags.size() > 1) {
                    errPos = 1;
                    goto error;
                }
            } else
                goto error;

            if (false) {
                error:
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getToken(errPos).getLine(),
                                             ast->getToken(errPos).getColumn(),
                                             " `" + ast->getToken(errPos).getValue() + "`. Make sure the order is correct and you're using the appropriate modifiers.");
            }

            if ((flags.find(PUBLIC) || flags.find(PRIVATE) || flags.find(PROTECTED)) && flags.find(LOCAL)) {
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                                 "`local` access specifier trumps (public, private, and protected) flags");
            }
        }
    }
}

ClassObject* Compiler::addChildClassObject(string name, List<AccessFlag> &flags, ClassObject* owner, Ast* ast) {
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    ClassObject *klass = new ClassObject(name,
                                         currModule, this->guid++, flags,
                                         meta), *prevClass;
    if((prevClass = owner->getChildClass(name)) != NULL || (prevClass = owner->getChildClass(name + "<>")) != NULL) {
        prevDefined:
        this->errors->createNewError(DUPLICATE_CLASS, ast->line, ast->col, " '" + name + "'");

        klass->free();
        delete klass;
        printNote(prevClass->meta, "class `" + prevClass->fullName + "` previously defined here");
        return owner->getChildClass(name + "<>") == NULL ? prevClass : NULL;
    } else {
        if(ends_with(klass->name, "<>") && (prevClass = owner->getChildClass(klass->name.substr(0, klass->name.size() - 2))) != NULL)
            goto prevDefined;

        klass->ast = ast;
        klass->owner = owner;
        owner->addClass(klass);
        return klass;
    }
}


void Compiler::parseVariableAccessFlags(List<AccessFlag> &flags, Ast *ast) {
    long errPos=0;
    for(long i = 0; i < ast->getTokenCount(); i++) {
        flags.add(strToAccessFlag(ast->getToken(i).getValue()));

        if(flags.last() == LOCAL && !globalScope()) {
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                         " `" + ast->getToken(i).getValue() + "` can only be used at global scope");
        } else if(flags.last() > STATIC) {
            errPos = i;
            goto error;
        }
    }

    if(flags.size() > 3) {
        errors->createNewError(ILLEGAL_ACCESS_DECLARATION, ast->line, ast->col,
                               "too many access specifiers found");
    }

    if(flags.get(0) <= PROTECTED || flags.get(0) == LOCAL) {
        if(flags.size() == 2) {
            if((flags.get(1) != flg_CONST
                && flags.get(1) != STATIC))
            { errPos = 1; goto error; }
        }
        else if(flags.size() == 3) {
            if(flags.get(1) != STATIC)
            { errPos = 1; goto error; }
            if(flags.get(2) != flg_CONST)
            { errPos = 2; goto error; }
        }
    }
    else if(flags.get(0) == STATIC) {
        if(flags.size() > 2)
        { errPos = flags.size() - 1; goto error; }
        else if(flags.size() == 2 && flags.get(1) != flg_CONST)
        { errPos = 1; goto error; }
    }
    else if(flags.get(0) == flg_CONST) {
        if(flags.size() != 1)
        { errPos = flags.size() - 1; goto error; }
    } else
        goto error;

    if(false) {
        error:
        this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getToken(errPos).getLine(),
                                     ast->getToken(errPos).getColumn(), " `" + ast->getToken(errPos).getValue() + "`");
    }

}

StorageLocality Compiler::strtostl(string locality) {
    return locality == "thread_local" ? stl_thread : stl_stack;
}



void Compiler::preProccessVarDeclHelper(List<AccessFlag>& flags, Ast* ast) {
    string name = ast->getToken(0).getValue();
    StorageLocality locality = stl_stack;
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        locality = strtostl(name);
        name = ast->getToken(1).getValue();
        flags.addif(STATIC);
    }

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    Field* prevField=NULL;
    if((prevField = currentScope()->klass->getField(name, false)) != NULL) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "field `" + name + "` is already defined in the scope");
        printNote(prevField->meta, "field `" + name + "` previously defined here");
    } else {
        currentScope()->klass->addField(new Field(UNTYPED, guid++, name, NULL, flags, meta, locality, checkstl(locality)));
        currentScope()->klass->getField(name, false)->ast = ast;
        currentScope()->klass->getField(name, false)->owner = currentScope()->klass;
        currentScope()->klass->getField(name, false)->fullName = currentScope()->klass->fullName + "." + name;
    }
}

void Compiler::preProccessVarDecl(Ast* ast) {
    List<AccessFlag> flags;

    if(ast->hasSubAst(ast_access_type)){
        parseVariableAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PUBLIC);
    }

    preProccessVarDeclHelper(flags, ast);

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
            preProccessVarDeclHelper(flags, ast->getSubAst(i));
        }
    }
}

Alias* Compiler::preProccessAliasDecl(Ast* ast) {
    List<AccessFlag> flags;

    if(ast->hasSubAst(ast_access_type)){
        parseVariableAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
        } else
            flags.add(PRIVATE);
    }

    if(flags.find(STATIC)) {
        flags.removeAt(STATIC);
        createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                         "access specifier `static` is ignored on aliases");
    }

    if(flags.find(flg_CONST)) {
        flags.removeAt(flg_CONST);
        errors->createNewError(GENERIC, ast->line, ast->col, "access specifier `const` is not allowed on aliases");
    }

    string name = ast->getToken(0).getValue();
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    Alias* prevAlias=NULL;
    if((prevAlias = currentScope()->klass->getAlias(name, false)) != NULL) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "alias `" + name + "` is already defined in the scope");
        printNote(prevAlias->meta, "alias `" + name + "` previously defined here");
        return NULL;
    } else {
        Alias *alias = new Alias(guid++, name, currentScope()->klass, flags, meta);
        currentScope()->klass->getAliases().add(alias);
        currentScope()->klass->getAlias(name, false)->ast = ast;
        currentScope()->klass->getAlias(name, false)->fullName = currentScope()->klass->fullName + "." + name;
        return alias;
    }
}

void Compiler::parseIdentifierList(Ast *ast, List<string> &idList) {
    ast = ast->getSubAst(ast_identifier_list);

    for(long i = 0; i < ast->getTokenCount(); i++) {
        string Key = ast->getToken(i).getValue();
        if(Key != ",") {
            if(!idList.addif(Key)) {
                stringstream err;
                err << "duplicate generic key `" << Key << "`";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
    }
}

void Compiler::preProccessGenericClassDecl(Ast* ast, bool isInterface) {
    List<AccessFlag> flags;
    ClassObject* currentClass;

    if (ast->hasSubAst(ast_access_type)) {
        parseClassAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
            flags.add(PUBLIC);
    } else {
        flags.add(PUBLIC);
    }

    if(ast->getType() == ast_generic_interface_decl) {
        // user has specified this interface as stable
        if(!flags.addif(STABLE)) {
            createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                             "`stable` access specifier is already applied to interfaces by default");
        }

        // user has specified that this interface is an ext class
        if(!flags.addif(EXTENSION)) {
            createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                             "`extension` access specifier is already applied to interfaces by default");
        }
    } else {
        if(!flags.find(STABLE))
            flags.addif(UNSTABLE);
    }

    string className = ast->getToken(0).getValue();
    List<string> identifierList;
    parseIdentifierList(ast, identifierList);

    stringstream fullName;
    if (globalScope()) {
        currentClass = addGlobalClassObject(className + "<>", flags, ast, generics);
        if(currentClass == NULL) return; // obviously the uer did something really dumb

        fullName << currModule->name << "#" << className;
    } else {
        currentClass = addChildClassObject(className + "<>", flags, currentScope()->klass, ast);
        if(currentClass == NULL) return; // obviously the uer did something really dumb

        fullName << currentScope()->klass->fullName << "." << className;
    }

    fullName << "<";
    for (long i = 0; i < identifierList.size(); i++) {
        fullName << identifierList.get(i);

        if ((i + 1) < identifierList.size()) {
            fullName << ", ";
        }
    }
    fullName << ">";
    currentClass->fullName = fullName.str();
    currentClass->addKeys(identifierList);
    identifierList.free();

    if (isInterface)
        currentClass->setClassType(class_interface | class_generic);
    else
        currentClass->setClassType(class_generic);
    // the purpose of this is simply to create the existance of the generic class we will process it later
}


void Compiler::preProccessEnumVar(Ast *ast) {
    List<AccessFlag> flags;
    flags.add(PUBLIC);
    flags.add(STATIC);
    flags.add(flg_CONST);

    string name = ast->getToken(0).getValue();
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);


    Field* prevField;
    if((prevField = currentScope()->klass->getField(name, false)) != NULL) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "enum `" + name + "` is already defined in the scope");
        printNote(prevField->meta, "field `" + name + "` previously defined here");
    } else {
        currentScope()->klass->addField(new Field(CLASS, guid++, name, currentScope()->klass, flags, meta, stl_stack, 0));
        currentScope()->klass->getField(name, false)->utype = new Utype(currentScope()->klass);
        currentScope()->klass->getField(name, false)->fullName = currentScope()->klass->fullName + "." +name;
        currentScope()->klass->getField(name, false)->ast = ast;
    }
}

void Compiler::preProccessEnumDecl(Ast *ast)
{
    Ast* block = ast->getSubAst(ast_enum_identifier_list);
    List<AccessFlag> flags;
    ClassObject* currentClass;

    if(ast->hasSubAst(ast_access_type)){
        parseClassAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
            flags.add(PUBLIC);
    } else {
        flags.add(PUBLIC);
    }

    /**
     * We do not want other users messing with enums at compile time causing issues
     */
    flags.addif(EXTENSION);
    string className = ast->getToken(0).getValue();

    if(IS_CLASS_GENERIC(currentScope()->klass->getClassType())) {
        ClassObject *enumClass = currentScope()->klass->getGenericOwner()->getChildClass(className);
        if(enumClass != NULL && enumClass->isAtLeast(preprocessed)) {
            currentScope()->klass->addClass(enumClass);
            return;
        }
    }

    if(globalScope()) {
        currentClass = addGlobalClassObject(className, flags, ast, classes);
        if(currentClass == NULL) return; // obviously the uer did something really dumb

        stringstream ss;
        ss << currModule->name << "#" << currentClass->name;
        currentClass->fullName = ss.str();
    }
    else {
        currentClass = addChildClassObject(className, flags, currentScope()->klass, ast);
        if(currentClass == NULL) return; // obviously the uer did something really dumb

        stringstream ss;
        ss << currentScope()->klass->fullName << "." << currentClass->name;
        currentClass->fullName = ss.str();
    }

    if(IS_CLASS_GENERIC(currentScope()->klass->getClassType())) {
        currentScope()->klass->getGenericOwner()->addClass(currentClass);
        currentClass->fullName = currentScope()->klass->getGenericOwner()->fullName + "." + currentClass->name;
        currentClass->owner = currentScope()->klass->getGenericOwner();
    }

    currentClass->setProcessStage(preprocessed);
    enums.addif(currentClass);
    if(currentClass->address == invalidAddr)
        currentClass->address = classSize++;
    currentClass->setClassType(class_enum);
    currScope.add(new Scope(currentClass, CLASS_SCOPE));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        Ast* branch = block->getSubAst(i);
        CHECK_CMP_ERRORS(return;)

        switch(branch->getType()) {
            case ast_class_decl: /* invalid */
                break;
            case ast_variable_decl: /* invalid */
                break;
            case ast_delegate_decl: /* invalid */
            case ast_method_decl: /* invalid */
                break;
            case ast_operator_decl: /* invalid */
                break;
            case ast_construct_decl: /* invalid */
                break;
            case ast_interface_decl: /* invalid */
                break;
            case ast_generic_class_decl: /* invalid */
                break;
            case ast_generic_interface_decl: /* invalid */
                break;
            case ast_enum_identifier:
                preProccessEnumVar(branch);
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << branch->getType();
                errors->createNewError(INTERNAL_ERROR, branch->line, branch->col, err.str());
                break;
        }
    }
    removeScope();
}

void Compiler::compileClassInitDecls(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

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
                    compileClassInitDecls(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateInitDecls(branch);
                    break;
                case ast_init_decl:
                    compileInitDecl(branch);
                    break;
                default:
                    break;
            }
        }

        removeScope();
    }
}

void Compiler::addLocalVariables() {
    List<Method*> constructors;
    currentScope()->klass->getAllFunctionsByType(fn_constructor, constructors);
    for(uInt i = 0; i < constructors.size(); i++) {
        Method *constructor = constructors.get(i);
        addLocalFields(constructor);
    }

    constructors.free();
}

void Compiler::addLocalFields(Method *func) {
    func->data.locals.addAll(func->params);
    func->data.localVariablesSize = func->params.size();

    for(Int i = 0; i < func->params.size(); i++) {
        func->params.get(i)->address = i + (func->flags.find(STATIC) ? 0 : 1);
    }

    if(!func->flags.find(STATIC))
        func->data.localVariablesSize++; // allocate 1 slot for the class instance
}

void Compiler::compileObfuscateDecl(Ast* ast) {
    if(c_options.obfuscate) {
        bool keepData = ast->hasToken("keep");
        Ast *block = ast->getSubAst(ast_obfuscate_block);
        obfuscateMode = true;

        for (Int i = 0; i < block->getSubAstCount(); i++) {
            Ast *branch = block->getSubAst(i);

            if (branch->getTokenCount() > 0) {
                ModuleData *package = Obfuscater::getModule(branch->getToken(0).getValue());
                if (package != NULL) {
                    if(!package->obfuscate && !keepData) {
                        errors->createNewError(GENERIC, ast->line, ast->col,
                                               "unable to obfuscate package `" + branch->getToken(0).getValue() + "`, that is already being preserved with `-keep`.");
                    } else
                        package->obfuscate = !keepData;
                } else {
                    FileData *file = Obfuscater::getFile(branch->getToken(0).getValue());
                    if (file != NULL) {
                        if(!file->obfuscate && !keepData) {
                            errors->createNewError(GENERIC, ast->line, ast->col,
                                                   "unable to obfuscate file `" + branch->getToken(0).getValue() + "`, that is already being preserved with `-keep`.");
                        } else
                            file->obfuscate = !keepData;
                    } else {
                        errors->createNewError(GENERIC, ast->line, ast->col,
                                               "could not locate package or file `" + branch->getToken(0).getValue() + "`.");
                    }
                }
            } else {
                Utype *utype;
                if(branch->hasSubAst(ast_utype_arg_list_opt)) {
                    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
                              ast->line, ast->col);

                    List<Field*> params;
                    List<AccessFlag> flags;
                    flags.add(PUBLIC);
                    parseUtypeArgList(params, branch->getSubAst(ast_utype_arg_list_opt));
                    Method tmp("tmp", currModule, currentScope()->klass, guid++, params, flags, meta);
                    tmp.ast = branch;

                    compileMethodReturnType(&tmp, tmp.ast);

                    RETAIN_REQUIRED_SIGNATURE(&tmp)
                    utype = compileUtype(branch->getSubAst(ast_utype));
                    RESTORE_REQUIRED_SIGNATURE()

                    flags.free();
                    freeListPtr(params);
                } else {
                    utype = compileUtype(branch->getSubAst(ast_utype));
                }

                if(utype->getResolvedType()) {
                    DataEntity *de = utype->getResolvedType();
                    if (!de->obfuscate && !keepData) {
                        errors->createNewError(GENERIC, ast->line, ast->col,
                                               "unable to obfuscate type `" + utype->toString() +
                                               "`, that is already being preserved with `-keep`.");
                    } else
                        de->obfuscate = !keepData;
                }
            }
        }

        obfuscateMode = false;
    }
}

void Compiler::compileClassObfuscations(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

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
                    compileClassObfuscations(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateObfuscations(branch);
                    break;
                case ast_obfuscate_decl:
                    compileObfuscateDecl(branch);
                    break;
                default:
                    break;
            }
        }

        addLocalVariables();
        removeScope();
    }
}

void Compiler::compileClassFields(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

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
                    compileClassFields(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateFields(branch);
                    break;
                case ast_variable_decl:
                    compileVariableDecl(branch);
                    break;
                default:
                    break;
            }
        }

        addLocalVariables();
        removeScope();
    }
}

void Compiler::compileClassMethods(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

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
                    compileClassMethods(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateFields(branch);
                    break;
                case ast_method_decl:
                    compileClassMethod(branch);
                    break;
                case ast_construct_decl:
                    compileConstructor(branch);
                    break;
                case ast_operator_decl:
                    compileOperatorOverload(branch);
                    break;

                default:
                    break;
            }
        }

        List<Method*> constructors;
        currentClass->getAllFunctionsByType(fn_constructor, constructors);
        for(Int i = 0; i < constructors.size(); i++) {
            Method* func = constructors.get(i);
            if(func->data.code.ir32.empty() && func->params.empty()) {
                currentScope()->currentFunction = func;
                RETAIN_BLOCK_TYPE(func->flags.find(STATIC) ? STATIC_BLOCK : INSTANCE_BLOCK)
                callBaseClassConstructor(ast, func);
                RESTORE_BLOCK_TYPE()
                func->data.code.addIr(OpBuilder::ret(NO_ERR));
            }

            if(!func->data.code.ir32.empty() && GET_OP(func->data.code.ir32.last()) != Opcode::RET) {
                func->data.code.addIr(OpBuilder::ret(NO_ERR));
            }
        }
        removeScope();
    }
}

void Compiler::preProccessClassDecl(Ast* ast, bool isInterface, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

    bool generatedClass = false;
    if(currentClass == NULL) {
        if (ast->hasSubAst(ast_access_type)) {
            parseClassAccessFlags(flags, ast->getSubAst(ast_access_type));

            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
        } else {
            flags.add(PUBLIC);
        }

        if(ast->getType() == ast_interface_decl) {
            // user has specified this interface as stable
            if(!flags.addif(STABLE)) {
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                                 "`stable` access specifier is already applied to interfaces by default");
            }

            // user has specified that this interface is an ext class
            if(!flags.addif(EXTENSION)) {
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                                 "`extension` access specifier is already applied to interfaces by default");
            }
        } else {
            if(!flags.find(STABLE))
                flags.addif(UNSTABLE);
        }

        string className = ast->getToken(0).getValue();
        if (globalScope()) {
            currentClass = addGlobalClassObject(className, flags, ast, classes);
            if(currentClass == NULL) return; // obviously the uer did something really dumb

            stringstream ss;
            ss << currModule->name << "#" << currentClass->name;
            currentClass->fullName = ss.str();
        } else {
            currentClass = addChildClassObject(className, flags, currentScope()->klass, ast);
            if(currentClass == NULL) return; // obviously the uer did something really dumb

            stringstream ss;
            ss << currentScope()->klass->fullName << "." << currentClass->name;
            currentClass->fullName = ss.str();
        }

        currentClass->setClassType(isInterface ? class_interface : class_normal);
    } else generatedClass = true;

    if(currentClass->address == invalidAddr)
        currentClass->address = classSize++;
    if(currentClass->address >= CLASS_LIMIT) {
        stringstream err;
        err << "maximum class limit of (" << CLASS_LIMIT << ") reached.";
        errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
    }

    currScope.add(new Scope(currentClass, CLASS_SCOPE));
    for (long i = 0; i < block->getSubAstCount(); i++) {
        Ast *branch = block->getSubAst(i);
        CHECK_CMP_ERRORS(return;)

        switch (branch->getType()) {
            case ast_class_decl:
                preProccessClassDecl(branch, false);
                break;
            case ast_variable_decl:
                preProccessVarDecl(branch);
                break;
            case ast_delegate_decl: /* Will be parsed later */
            case ast_method_decl: /* Will be parsed later */
                break;
            case ast_alias_decl:
                preProccessAliasDecl(branch);
                break;
            case ast_operator_decl: /* Will be parsed later */
                break;
            case ast_construct_decl: /* Will be parsed later */
                break;
            case ast_interface_decl:
                preProccessClassDecl(branch, true);
                break;
            case ast_generic_class_decl:
                preProccessGenericClassDecl(branch, false);
                break;
            case ast_generic_interface_decl:
                preProccessGenericClassDecl(branch, true);
                break;
            case ast_enum_decl:
                preProccessEnumDecl(branch);
                break;
            case ast_mutate_decl:
                if (processingStage >= POST_PROCESSING && !generatedClass)
                    preProcessMutation(branch);
                break;
            case ast_init_decl: /* Will be parsed later */
                break;
            case ast_obfuscate_decl:
                /* ignpre */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << branch->getType();
                errors->createNewError(INTERNAL_ERROR, branch->line, branch->col, err.str());
                break;
        }
    }
    removeScope();
}

int64_t Compiler::checkstl(StorageLocality locality) {
    if(locality == stl_thread)
        return threadLocals++;
    else
        return invalidAddr;
}

void Compiler::removeScope() {
    currScope.last()->resetLocalScopeFlags();
    delete currScope.last();
    currScope.pop_back();
}

void Compiler::preProccessImportDecl(Ast *branch, List<ModuleData*> &imports) {
    bool star = false;
    stringstream ss;
    for(long i = 0; i < branch->getTokenCount(); i++) {
        if(branch->getToken(i).getValue() != "*") {
            ss << branch->getToken(i).getValue();
        } else {
            star = true;
            break;
        }
    }

    string mod = ss.str();
    string baseMod = mod.substr(0, mod.size() - 1);
    ModuleData *package;

    if(star) {
        bool found = false;
        if((package = Obfuscater::getModule(baseMod)) != NULL) {
            imports.addif(package);
        }

        for(long i = 0; i < Obfuscater::modules.size(); i++) {
            if(startsWith(Obfuscater::modules.get(i)->name, mod)) {
                found = true;
                imports.addif(Obfuscater::modules.get(i));
            }
        }

        if(!found) {
            errors->createNewError(GENERIC, branch->line, branch->col, "modules under prefix `" + mod +
                                                                       "*` could not be found");
        }

    } else {
        if((package = Obfuscater::getModule(mod)) != NULL) {
            if(!imports.addif(package)) {
                createNewWarning(GENERIC, __WGENERAL, branch->line, branch->col, "module `" + mod + "` has already been imported.");
            }
        }
        else {
            errors->createNewError(GENERIC, branch->line, branch->col, "module `" + mod +
                                                                       "` could not be found");
        }
    }
}

void Compiler::preproccessImports() {
    KeyPair<string, List<string>> resolveMap;
    List<ModuleData*> imports;
    imports.add(Obfuscater::getModule("std")); // import the std lib by default

    for(unsigned long x = 0; x < current->size(); x++)
    {
        Ast *branch = current->astAt(x);
        if(x == 0)
        {
            if(branch->getType() == ast_module_decl) {
                string module = parseModuleDecl(branch);
                imports.add(Obfuscater::getModule(module));
                currModule = Obfuscater::getModule(module);
                continue;
            } else {
                currModule = undefinedModule;
                imports.add(currModule);
            }
        }
        switch(branch->getType()) {
            case ast_import_decl: /* ignore for now */
                preProccessImportDecl(branch, imports);
                break;
            default:
                /* ignore */
                break;
        }

        CHECK_CMP_ERRORS(return;)
    }


    importMap.__new().key = Obfuscater::getFile(current->getTokenizer()->file);
    importMap.last().value.init();
    importMap.last().value.addAll(imports);
}

void Compiler::validateAccess(Alias *alias, Ast* ast) {
    if(alias->utype->getType() == utype_field)
        validateAccess((Field*)alias->utype->getResolvedType(), ast);
    else if(alias->utype->getType() == utype_class)
        validateAccess((ClassObject*)alias->utype->getResolvedType(), ast);
    else if(alias->utype->getType() == utype_method)
        validateAccess((Method*)alias->utype->getResolvedType(), ast);
}

void Compiler::checkTypeInference(Alias *alias, Ast* ast) {
    if(alias->utype->getType() == utype_class)
        checkTypeInference(ast);
    else if(alias->utype->getType() == utype_native)
        checkTypeInference(ast);
}

void Compiler::validateAccess(Field *field, Ast* pAst) {
    if(field->flags.find(LOCAL)) {
        Scope *scope = currentScope();
        if(Obfuscater::getFile(current->getTokenizer()->file) == field->meta.file) {
        } else {
            FileData *fd = Obfuscater::getFile(current->getTokenizer()->file);
            FileData *ffd = field->meta.file;
            errors->createNewError(GENERIC, pAst, " invalid access to localized field `" + field->fullName + "`");
        }
    } else if(field->flags.find(PRIVATE)) {
        Scope *scope = currentScope();
        if(field->owner == currentScope()->klass || scope->klass->isClassRelated(field->owner)) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private field `" + field->fullName + "`");
        }
    } else if(field->flags.find(PROTECTED)) {
        if(currentScope()->klass->isClassRelated(field->owner) || field->owner == currentScope()->klass
                || field->module == currentScope()->klass->module) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected field `" + field->fullName + "`");
        }
    } else {
        // access granted
    }
}

void Compiler::validateAccess(ClassObject *klass, Ast* pAst) {
    if(klass->flags.find(LOCAL)) {
        Scope *scope = currentScope();
        if(scope->klass->meta.file == klass->meta.file) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to localized class `" + klass->fullName + "`");
        }
    } else if(klass->flags.find(PRIVATE)) {
        Scope *scope = currentScope();
        if(scope->klass->isClassRelated(klass)) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private class `" + klass->fullName + "`");
        }
    } else if(klass->flags.find(PROTECTED)) {
        Scope *scope = currentScope();
        if(scope->klass->isClassRelated(klass)
           || scope->klass->module == klass->module) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected class `" + klass->fullName + "`");
        }
    } else {
        // access granted
    }
}

void Compiler::validateAccess(Method *function, Ast* pAst) {
    if(function->flags.find(LOCAL)) {
        Scope *scope = currentScope();
        if(scope->klass->meta.file == function->meta.file) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to localized method `" + function->toString() + "`");
        }
    } else if(function->flags.find(PRIVATE)) {
        Scope *scope = currentScope();
        if(function->owner == scope->klass || scope->klass->isClassRelated(function->owner)) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private method `" + function->toString() + "`");
        }
    } else if(function->flags.find(PROTECTED)) {
        Scope *scope = currentScope();
        if(scope->klass->isClassRelated(function->owner) || function->owner == scope->klass
           || function->module == currentScope()->klass->module) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected method `" + function->toString() + "`");
        }
    } else {
        // access granted
    }
}

ClassObject* Compiler::resolveClass(ModuleData* mod, string name, Ast* pAst) {
    ClassObject* klass = NULL;
    if(mod != NULL) {
        if((klass = findClass(mod, name, classes)) != NULL) {
            validateAccess(klass, pAst);
            return klass;
        } else return NULL;
    } else {
        resolve:
        for (unsigned int i = 0; i < importMap.size(); i++) {
            if (importMap.get(i).key->name == current->getTokenizer()->file) {

                List<ModuleData*> &lst = importMap.get(i).value;
                for (unsigned int x = 0; x < lst.size(); x++) {
                    if ((klass = findClass(lst.get(x), name, classes)) != NULL) {
                        validateAccess(klass, pAst);
                        return klass;
                    }
                }

                break;
            }
        }

        return klass;
    }
}

bool Compiler::resolveClass(List<ClassObject*> &classes, List<ClassObject*> &results, ModuleData* mod, string name, Ast* pAst, bool match) {
    ClassObject* klass = NULL;
    if(mod != NULL && (klass = findClass(mod, name, classes, match)) != NULL) {
        validateAccess(klass, pAst);
        results.add(klass);
    } else {
        for (unsigned int i = 0; i < importMap.size(); i++) {
            if (importMap.get(i).key->name == current->getTokenizer()->file) {

                List<ModuleData*> &lst = importMap.get(i).value;
                for (unsigned int x = 0; x < lst.size(); x++) {
                    if ((klass = findClass(lst.get(x), name, classes, match)) != NULL) {
                        results.addif(klass);
                    }
                }

                break;
            }
        }
    }

    return !results.empty();
}

void Compiler::inheritObjectClassHelper(Ast *ast, ClassObject *klass) {
    if(klass->fullName != "std#_object_" && klass->getSuperClass() == NULL) {
        ClassObject *base = findClass(Obfuscater::getModule("std"), "_object_", classes);

        if (base != NULL && (IS_CLASS_ENUM(base->getClassType()) || IS_CLASS_INTERFACE(base->getClassType()))) {
            stringstream err;
            err << "support class for objects must be of type class";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
        } else {
            if (base != NULL) {
                if(!ends_with(klass->fullName, "#" + globalClass))
                    klass->setSuperClass(base);
            }
            else {
                stringstream err;
                err << "support class for objects not found";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
    }
}

void Compiler::inheritEnumClassHelper(Ast *ast, ClassObject *enumClass) {
    if(IS_CLASS_ENUM(enumClass->getClassType())) {
        ClassObject *base = findClass(Obfuscater::getModule("std"), "_enum_", classes);

        if (base != NULL && (IS_CLASS_ENUM(base->getClassType()) || IS_CLASS_INTERFACE(base->getClassType()))) {
            stringstream err;
            err << "support class for enums must be of type class";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
        } else {
            if (base != NULL) {
                enumClass->setSuperClass(base);
            }
            else {
                stringstream err;
                err << "support class for enums not found";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
    }
}

void Compiler::parseCharLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);

    if(token.getValue().size() > 1) {
        switch(token.getValue().at(1)) {
            case 'n':
                expr->utype->setResolvedType(new Literal('\n', _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi('\n', EBX));
                break;
            case 't':
                expr->utype->setResolvedType(new Literal('\t', _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi('\t', EBX));
                break;
            case 'b':
                expr->utype->setResolvedType(new Literal('\b', _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi('\b', EBX));
                break;
            case 'v':
                expr->utype->setResolvedType(new Literal('\v', _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi('\v', EBX));
                break;
            case 'r':
                expr->utype->setResolvedType(new Literal('\r', _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi('\r', EBX));
                break;
            case 'f':
                expr->utype->setResolvedType(new Literal('\f', _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi('\f', EBX));
                break;
            case '\\':
                expr->utype->setResolvedType(new Literal('\\', _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi('\\', EBX));
                break;
            default:
                expr->utype->setResolvedType(new Literal(token.getValue().at(1), _INT8));
                expr->utype->getCode().addIr(OpBuilder::movi(token.getValue().at(1), EBX));
                break;
        }
    } else {
        expr->utype->setResolvedType(new Literal(token.getValue().at(0), _INT8));
        expr->utype->getCode().addIr(OpBuilder::movi(token.getValue().at(0), EBX));
    }

    CodeHolder &inj = expr->utype->getCode().getInjector(stackInjector);
    inj.addIr(OpBuilder::rstore(EBX));
}

string Compiler::invalidateUnderscores(string str) {
    stringstream newstring;
    for(char c : str) {
        if(c != '_')
            newstring << c;
    }
    return newstring.str();
}

bool Compiler::isAllIntegers(string int_string) {
    for(char c : int_string) {
        if(!isdigit(c))
            return false;
    }
    return true;
}

void Compiler::parseIntegerLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);
    Int address = invalidAddr;

    double value;
    string int_string = invalidateUnderscores(token.getValue());

    if(isAllIntegers(int_string)) {
        value = std::strtod (int_string.c_str(), NULL);
        if(value > INT32_MAX) {
            long constantAddress = constantMap.addIfIndex(value);
            address = constantAddress;

            if(constantAddress >= CONSTANT_LIMIT) {
                stringstream err;
                err << "maximum constant limit of (" << CONSTANT_LIMIT << ") reached.";
                errors->createNewError(INTERNAL_ERROR, token, err.str());
            }

            expr->utype->getCode().addIr(OpBuilder::ldc(EBX, constantAddress));
        } else
            expr->utype->getCode().addIr(OpBuilder::movi(value, EBX));
    }else {
        value = std::strtod (int_string.c_str(), NULL);
        long constantAddress = constantMap.addIfIndex(value);
        address = constantAddress;

        if(constantAddress >= CONSTANT_LIMIT) {
            stringstream err;
            err << "maximum constant limit of (" << CONSTANT_LIMIT << ") reached.";
            errors->createNewError(INTERNAL_ERROR, token, err.str());
        }

        expr->utype->getCode().addIr(OpBuilder::ldc(EBX, constantAddress));
    }

    expr->utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));
    expr->utype->setResolvedType(new Literal(value));
    expr->utype->getResolvedType()->address = address;
}

void Compiler::parseHexLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);

    double value;
    string hex_string = invalidateUnderscores(token.getValue());

#if _ARCH_BITS == 64
    value = strtoll(hex_string.c_str(), NULL, 16);
#else
    value = strtol(hex_string.c_str(), NULL, 16);
#endif
    if(value > INT32_MAX) {
        long constantAddress = constantMap.addIfIndex(value);
        if(constantAddress >= CONSTANT_LIMIT) {
            stringstream err;
            err << "maximum constant limit of (" << CONSTANT_LIMIT << ") reached.";
            errors->createNewError(INTERNAL_ERROR, token, err.str());
        }

        expr->utype->getCode().addIr(OpBuilder::ldc(EBX, constantAddress));
    } else
        expr->utype->getCode().addIr(OpBuilder::movi(value, EBX));
    expr->utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));

    expr->utype->setResolvedType(new Literal(value));
}

void Compiler::parseStringLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(true);

    long long index = stringMap.addIfIndex(token.getValue());
    if(index >= STRING_LITERAL_LIMIT) {
        stringstream err;
        err << "maximum string literal limit of (" << STRING_LITERAL_LIMIT << ") reached.";
        errors->createNewError(INTERNAL_ERROR, token, err.str());
    }

    expr->utype->setResolvedType(new Literal(token.getValue(), index));
    expr->utype->getCode().addIr(OpBuilder::newString(index));

    expr->utype->getCode().getInjector(ptrInjector)
            .addIr(OpBuilder::popObject2());
}

void Compiler::parseBoolLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);
    expr->utype->getCode().addIr(OpBuilder::movi(token.getValue() == "true" ? 1 : 0, EBX));

    expr->utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));
    expr->utype->setResolvedType(new Literal(token.getValue() == "true" ? 1 : 0));
}

void Compiler::compileLiteralExpression(Expression* expr, Ast* ast) {
    expr->ast = ast;
    switch(ast->getToken(0).getId()) {
        case CHAR_LITERAL:
            parseCharLiteral(expr, ast->getToken(0));
            break;
        case INTEGER_LITERAL:
            parseIntegerLiteral(expr, ast->getToken(0));
            break;
        case HEX_LITERAL:
            parseHexLiteral(expr, ast->getToken(0));
            break;
        case STRING_LITERAL:
            parseStringLiteral(expr, ast->getToken(0));
            break;
        default:
            if(ast->getToken(0).getValue() == "true" ||
                    ast->getToken(0).getValue() == "false") {
                parseBoolLiteral(expr, ast->getToken(0));
            }
            break;
    }

}

void Compiler::compileUtypeClass(Expression* expr, Ast* ast) {
    RETAIN_TYPE_INFERENCE(false)
    RETAIN_BLOCK_TYPE(INSTANCE_BLOCK)
    Utype* utype = compileUtype(ast->getSubAst(ast_utype));
    RESTORE_BLOCK_TYPE()
    RESTORE_TYPE_INFERENCE()

    if(utype->getType() == utype_class) {
        expr->type = exp_var;
        expr->utype->setType(utype_literal);
        expr->utype->setArrayType(false);

        expr->utype->getCode().addIr(OpBuilder::movi(utype->getResolvedType()->address, EBX));
        expr->utype->setResolvedType(new Literal(utype->getResolvedType()->address));
    } else {
        expr->type = exp_undefined;
        expr->utype = new Utype();
        utype->free();
        errors->createNewError(GENERIC, ast, "expected class");
    }

    expr->utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));
    expr->ast = ast;
    compilePostAstExpressions(expr, ast);
}

void Compiler::compileExpressionList(List<Expression*>& lst, Ast* ast) {
    for(long i = 0; i < ast->getSubAstCount(); i++) {
        lst.__new() = new Expression();
        compileExpression(lst.get(i), ast->getSubAst(i));
    }
}

void Compiler::expressionsToParams(List<Expression*> &expressions, List<Field*> &params) {
    for(long i = 0; i < expressions.size(); i++) {
        Expression &e = *expressions.get(i);
        params.add(new Field());
        expressionToParam(e, params.get(i));
    }
}

void Compiler::expressionToParam(Expression &expression, Field *param) {
    param->inlineCheck = true;

    if(expression.type == exp_null) {
        param->nullField = true;
        param->utype = new Utype(OBJECT);
        param->type = OBJECT;

        // yet another line of nasty dirty code...smh...
        param->utype->getCode().copy(expression.utype->getCode());
        freePtr(expression.utype);
    }
    else if(expression.type == exp_var && expression.utype->getType() == utype_literal) {
        if(((Literal*) expression.utype->getResolvedType())->literalType == string_literal) {
            param->utype = new Utype(_INT8, true);
            param->isArray = true;
            param->type = _INT8;
        } else {
            param->utype = new Utype(VAR);
            param->type = VAR;
        }

        // yet another line of nasty dirty code...smh...
        param->utype->getCode().copy(expression.utype->getCode());
        freePtr(expression.utype);
    }
    else if(expression.type == exp_var && expression.utype->getType() == utype_method) {
        param->utype = new Utype(FNPTR);
        param->utype->copy(expression.utype);
        param->type = FNPTR;

        // yet another line of nasty dirty code...smh...
        param->utype->getCode().copy(expression.utype->getCode());
        freePtr(expression.utype);
    }
    else if(expression.utype->getType() == utype_field) {
        auto *exprField = (Field*)expression.utype->getResolvedType();
        param->utype = new Utype();
        param->utype->copy(exprField->utype);
        param->type = exprField->type;
        param->isArray = exprField->isArray;

        // yet another line of nasty dirty code...smh...
        param->utype->getCode().copy(expression.utype->getCode());
        freePtr(expression.utype);
    }
    else {
        param->isArray = expression.utype->isArray();
        param->utype = expression.utype;
        param->type = expression.utype->getResolvedType() ? expression.utype->getResolvedType()->type : UNDEFINED;
    }

    expression.utype = NULL;
}

bool Compiler::isLambdaUtype(Utype *utype) {
    if(utype && utype->getResolvedType()) {
        if (utype->getResolvedType()->type == FNPTR) {
            Method *lambda = (Method *) utype->getResolvedType();
            return lambda->fnType == fn_lambda;
        }
    }

    return false;
}

bool Compiler::paramsContainNonQualifiedLambda(List<Field*> &params) {
    for(long i = 0; i < params.size(); i++) {
        Field *param = params.get(i);
        if(isLambdaUtype(param->utype) && !isLambdaFullyQualified((Method*)param->utype->getResolvedType())) {
            return true;
        }
    }

    return false;
}

Method* // TODO: Circle back around to this as it is in its primitive stae there might be issues with how it searches the mehtods with _int64 _int8 etc.
Compiler::findFunction(ClassObject *k, string name, List<Field*> &params, Ast* ast, bool checkBase, function_type type, bool advancedSearch) {
    List<Method*> funcs;
    List<Method*> matches;
    Method* resolvedFunction; // TODO: go over function in tutorial later

    if(k->getGenericOwner() && k->processedExtFunctions < k->getGenericOwner()->getExtensionFunctionTree().size())
        resolveExtensionFunctions(k);
    if(type == fn_undefined)
        k->getAllFunctionsByName(name, funcs, checkBase);
    else
        k->getAllFunctionsByTypeAndName(type, name, checkBase, funcs);

    if(!funcs.empty()) {
        for(long long i = 0; i < funcs.size(); i++) {
            if(simpleParameterMatch(funcs.get(i)->params, params)) {
                resolvedFunction = funcs.get(i);

                if(!paramsContainNonQualifiedLambda(params)) {
                    funcs.free();
                    return resolvedFunction;
                } else
                    matches.add(funcs.get(i));
            }
        }

        if(advancedSearch && matches.empty()) {
            for (long long i = 0; i < funcs.size(); i++) {
                if (complexParameterMatch(funcs.get(i)->params, params, false)) {
                    matches.add(funcs.get(i));
                }
            }

            if(matches.empty()) {
                for (long long i = 0; i < funcs.size(); i++) {
                    if (complexParameterMatch(funcs.get(i)->params, params, true)) {
                        matches.add(funcs.get(i));
                    }
                }
            }
        }
    }

    funcs.free();
    if(!matches.empty()) {
        resolvedFunction = matches.get(0);
        if(matches.size() > 1) {
            errors->createNewError(GENERIC, ast->line, ast->col, "call to method `" + name + "` is ambiguous");
        }

        matches.free();
        return resolvedFunction;
    }

    return NULL;
}

Alias *Compiler::resolveAlias(ModuleData* mod, string name, Ast *ast) {
    List<ClassObject*> globalClasses;
    resolveClass(classes, globalClasses,  mod, globalClass, ast);
    Alias *resolvedAlias;

    for(long i = 0; i < globalClasses.size(); i++) {

        if((resolvedAlias = globalClasses.get(i)->getAlias(name, false)) != NULL) {
            return resolvedAlias;
        }
    }

    return NULL;
}

Field *Compiler::resolveField(string name, Ast *ast) {
    List<ClassObject*> globalClasses;
    resolveClass(classes, globalClasses, NULL, globalClass, ast);
    Field *resolvedField;

    for(long i = 0; i < globalClasses.size(); i++) {

        if((resolvedField = globalClasses.get(i)->getField(name, false)) != NULL)
            return resolvedField;
    }

    return NULL;
}

Method *Compiler::resolveFunction(string name, List<Field*> &params, Ast *ast) {
    List<ClassObject*> globalClasses;
    resolveClass(classes, globalClasses, NULL, globalClass, ast);
    Method *resolvedMethod;

    for(long i = 0; i < globalClasses.size(); i++) {
        if((resolvedMethod = findFunction(globalClasses.get(i), name, params, ast, false)) != NULL)
            return resolvedMethod;
    }

    return NULL;
}

bool Compiler::resolveFunctionByName(string name, List<Method*> &functions, Ast *ast) {
    List<ClassObject*> globalClasses;
    resolveClass(classes, globalClasses, NULL, globalClass, ast);
    Method *resolvedMethod;

    for(long i = 0; i < globalClasses.size(); i++) {

        globalClasses.get(i)->getAllFunctionsByName(name, functions);
    }

    return !functions.empty();
}

Method* Compiler::findGetterSetter(ClassObject *klass, string name, List<Field*> &params, Ast* ast) {
    Method *resolvedMethod = NULL;
    if((startsWith(name, "get_") || startsWith(name, "set_"))) {
        stringstream fieldName;
        for(long i = 4; i < name.size(); i++) {
            fieldName << name.at(i);
        }

        Field *field = klass->getField(fieldName.str(), true);
        compileFieldType(field);

        if((resolvedMethod = findFunction(klass, name, params, ast)) != NULL) {
            return resolvedMethod;
        }
    }
    return resolvedMethod;
}

Method* Compiler::compileSingularMethodUtype(ReferencePointer &ptr, Expression *expr, List<Field*> &params, Ast* ast) {
    Method *resolvedMethod = NULL;
    string name = ptr.classes.get(0);
    if(parser::isOverrideOperator(name)) {
        name = "operator" + name;
    }

    if(ptr.mod != "") {
        ModuleData *module = Obfuscater::getModule(ptr.mod) ? Obfuscater::getModule(ptr.mod) : undefinedModule;
        ClassObject *global = findClass(module, globalClass, classes);
        if(currentScope()->type == RESTRICTED_INSTANCE_BLOCK && !currentScope()->klass->isClassRelated(global))
            global = NULL;

        if(global != NULL && (resolvedMethod = findFunction(global, name, params, ast)) != NULL) {
            return resolvedMethod;
        } else if(global != NULL && (resolvedMethod = findGetterSetter(global, name, params, ast)) != NULL) {
            return resolvedMethod;
        }
    } else {


        if((resolvedMethod = findFunction(currentScope()->klass, name, params, ast, true)) != NULL)
            return resolvedMethod;
        else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && currentScope()->currentFunction != NULL
            && currentScope()->currentFunction->data.getLocalField(name) != NULL) {
            Field* field = currentScope()->currentFunction->data.getLocalField(name);
            if(field->utype && field->utype->getType() == utype_function_ptr) {
                resolvedMethod =  (Method*)field->utype->getResolvedType();

                if (!complexParameterMatch(resolvedMethod->params, params)) {
                    errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() +
                                                                         "` does not match provided parameter arguments");
                }

                if(isFieldInlined(field)) {
                    inlineVariableValue(expr->utype->getCode(), field);
                    expr->utype->getCode().inject(stackInjector);
                } else {
                    if (field->locality == stl_thread) {
                        expr->utype->getCode()
                                .addIr(OpBuilder::tlsMovl(field->address))
                                .addIr(OpBuilder::movi(0, ADX))
                                .addIr(OpBuilder::iaload(EBX, ADX))
                                .addIr(OpBuilder::rstore(EBX));
                    } else {
                        expr->utype->getCode()
                                .addIr(OpBuilder::loadl(EBX, field->address))
                                .addIr(OpBuilder::rstore(EBX));
                    }
                }
                return resolvedMethod;
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` is not a function pointer");
            }
        } else if(currentScope()->klass->getField(name, true) != NULL) {

            Field* field = currentScope()->klass->getField(name, true);
            compileFieldType(field);
            if(currentScope()->type == STATIC_BLOCK && !field->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "cannot get field `" + name + "` from self in static context");
            }

            if(field->utype && field->utype->getType() == utype_function_ptr) {
                resolvedMethod =  (Method*)field->utype->getResolvedType();

                if(!complexParameterMatch(resolvedMethod->params, params)) {
                    errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` does not match provided parameter arguments");
                }

                if(field->getter == NULL && isFieldInlined(field)) {
                    inlineVariableValue(expr->utype->getCode(), field);
                    expr->utype->getCode().inject(stackInjector);
                } else {
                    if(field->getter != NULL
                       && (currentScope()->currentFunction == NULL ||
                           !(currentScope()->currentFunction == field->setter || currentScope()->currentFunction == field->getter))) {
                        if(field->locality == stl_stack && !field->flags.find(STATIC) && !expr->utype->getCode().instanceCaptured) {
                            expr->utype->getCode().instanceCaptured = true;
                            expr->utype->getCode().addIr(OpBuilder::movl(0));
                        }

                        if(field->locality == stl_stack && !field->flags.find(STATIC))
                            expr->utype->getCode().addIr(OpBuilder::pushObject());
                        compileFieldGetterCode(expr->utype->getCode(), field);
                        // we dont need the getter code size stuff because we immediatley use the dynaic function address
                    } else {
                        if (field->locality == stl_thread) {
                            expr->utype->getCode().addIr(OpBuilder::tlsMovl(field->address));
                        } else {
                            if (field->flags.find(STATIC) || field->owner->isGlobalClass())
                                expr->utype->getCode().addIr(OpBuilder::movg(field->owner->address));
                            else if (!expr->utype->getCode().instanceCaptured) {
                                expr->utype->getCode().instanceCaptured = true;
                                expr->utype->getCode().addIr(OpBuilder::movl(0));
                            }
                            expr->utype->getCode().addIr(OpBuilder::movn(field->address));
                        }

                        expr->utype->getCode()
                                .addIr(OpBuilder::movi(0, ADX))
                                .addIr(OpBuilder::iaload(EBX, ADX))
                                .addIr(OpBuilder::rstore(EBX));
                    }
                }

                return resolvedMethod;
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` is not a function pointer");
            }
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedMethod = resolveFunction(name, params, ast)) != NULL) {
            return resolvedMethod;
        } else if((resolvedMethod = findGetterSetter(currentScope()->klass, name, params, ast)) != NULL) {
            return resolvedMethod;
        }
    }

    errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + ptr.classes.get(0) +  Method::paramsToString(params) + "`");
    return NULL;
}

bool Compiler::isUtypeClass(Utype* utype, ModuleData* mod, int names, ...) {
    if(utype) {
        va_list ap;
        bool found = false;

        va_start(ap, names);
        for (size_t loop = 0; loop < names; ++loop) {
            string name = va_arg(ap, char*);
            if(utype->getClass() && utype->getClass()->name == name
               && utype->getClass()->module == mod) {
                va_end(ap);
                return true;
            }
        }

        va_end(ap);
    }
    return false;
}
void Compiler::fullyQualifyLambda(Utype *lambdaQualifier, Utype *lambda) {
    Method *lambdaFn = (Method*)lambda->getResolvedType();
    Method *qualifier = (Method*)lambdaQualifier->getResolvedType();

    if(isLambdaFullyQualified(lambdaFn)) {
        lambdaFn->utype = qualifier->utype;
    } else {
        lambdaFn->utype = qualifier->utype;

        for(long i = 0; i < lambdaFn->params.size(); i++) {
            Field *param = lambdaFn->params.get(i);
            Field *qualifierParam = qualifier->params.get(i);

            if(param->type == ANY) {
                param->type = qualifierParam->type;
                param->isArray = qualifierParam->isArray;
                param->utype = qualifierParam->utype;
            }
        }
    }
}

Method* Compiler::compileMethodUtype(Expression* expr, Ast* ast) {
    Method *resolvedMethod = NULL;
    ReferencePointer ptr;
    List<Expression*> expressions;
    List<Field*> params;
    bool singularCall = false; //TODO: fully support restricted instance blocks

    RETAIN_BLOCK_TYPE(currentScope()->type == RESTRICTED_INSTANCE_BLOCK || currentScope()->type == INSTANCE_BLOCK
        ? INSTANCE_BLOCK : STATIC_BLOCK)
    compileTypeIdentifier(ptr, ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier));
    RETAIN_TYPE_INFERENCE(true)
    compileExpressionList(expressions, ast->getSubAst(ast_expression_list));
    RESTORE_TYPE_INFERENCE()

    expressionsToParams(expressions, params);
    RESTORE_BLOCK_TYPE()

    if(ptr.classes.singular()) {
        singularCall = true;
        resolvedMethod = compileSingularMethodUtype(ptr, expr, params, ast);

        if(resolvedMethod)
            compileMethodReturnType(resolvedMethod, resolvedMethod->ast);
        if(resolvedMethod && currentScope()->type == STATIC_BLOCK && !resolvedMethod->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->line, ast->col, " call to instance function `" + resolvedMethod->toString() + "` inside static context");
        }
    } else {
        string name = ptr.classes.get(ptr.classes.size() - 1);
        if(parser::isOverrideOperator(name)) {
            name = "operator" + name;
        }

        ptr.classes.pop_back();

        Utype *utype = expr->utype;
        RETAIN_TYPE_INFERENCE(false)
        resolveUtype(ptr, utype, ast);
        RESTORE_TYPE_INFERENCE()
        if(utype->isClass()) {
            ClassObject* klass = utype->getClass();

            // TODO: add overload need to see how it comes in later
            if(klass != NULL && (resolvedMethod = findFunction(klass, name, params, ast, true)) != NULL) {
                compileMethodReturnType(resolvedMethod, resolvedMethod->ast);
                if(!resolvedMethod->flags.find(STATIC)) {
                    utype->getCode().inject(stackInjector);
                } else {
                    // TODO: try to find out wether or not the last item processed is an instance field
                    // if so warn the user that what they are doing is innefficent
                }
            }
            else if(klass != NULL && klass->getField(name, true) != NULL) {
                Field *field = klass->getField(name, true);
                compileFieldType(field);

                if(field->type == FNPTR) {
                    resolvedMethod = (Method *) field->utype->getResolvedType();
                    compileMethodReturnType(resolvedMethod, resolvedMethod->ast); // probably unessicary but we do it anyway

                    utype->getCode().inject(ptrInjector);
                    utype->getCode().addIr(OpBuilder::movn(field->address))
                            .addIr(OpBuilder::movi(0, ADX))
                            .addIr(OpBuilder::iaload(EBX, ADX))
                            .addIr(OpBuilder::rstore(EBX));

                    if(!complexParameterMatch(resolvedMethod->params, params)) {
                        errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` does not match provided parameter arguments");
                    }
                }
                else
                    errors->createNewError(GENERIC, ast->line, ast->col,
                                           " could not resolve function `" + name + "`");
            } else if((resolvedMethod = findGetterSetter(klass, name, params, ast)) != NULL) {
                if(!resolvedMethod->flags.find(STATIC)) {
                    utype->getCode().inject(stackInjector);
                } else {
                    // TODO: try to find out wether or not the last item processed is an instance field
                    // if so warn the user that what they are doing is innefficent
                }
            }
            else
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " could not resolve function `" + name + "` with parameters `"
                                            + Method::paramsToString(params) + "`");

        } else if(utype->getType() == utype_field) {
            errors->createNewError(GENERIC, ast->line, ast->col, " field `" + utype->toString() + "` is not a data structure");
        } else {
            utype_unresolved:
            errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + ptr.classes.get(0) +  Method::paramsToString(params) + "`");
        }

        if(resolvedMethod && utype->getType() == utype_class && !resolvedMethod->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->line, ast->col, " call to instance function `" + resolvedMethod->toString() + "` via static context");
        }
    }

    if(resolvedMethod) {
        CodeHolder &code = expr->utype->getCode();

        validateAccess(resolvedMethod, ast);
        if(!resolvedMethod->flags.find(STATIC)
            && resolvedMethod->fnType != fn_ptr
            && !expr->utype->getCode().instanceCaptured
            && singularCall) {
            code.addIr(OpBuilder::movl(0));
            code.addIr(OpBuilder::pushObject());
        }

        pushParametersToStackAndCall(ast, resolvedMethod, params, code);
    }

    ptr.free();
    expressions.free();
    freeListPtr(params);
    return resolvedMethod;
}

void Compiler::pushParametersToStackAndCall(Ast *ast, Method *resolvedMethod, List<Field *> &params, CodeHolder &code) {
    for(long i = 0; i < params.size(); i++) {
        Field *methodParam = resolvedMethod->params.get(i);
        if(isLambdaUtype(params.get(i)->utype))
            fullyQualifyLambda(methodParam->utype, params.get(i)->utype);

        if(isUtypeConvertableToNativeClass(methodParam->utype, params.get(i)->utype)) {
            convertUtypeToNativeClass(methodParam->utype, params.get(i)->utype, code, ast);
            code.inject(code.getInjector(stackInjector));
        } else if(isUtypeClassConvertableToVar(methodParam->utype, params.get(i)->utype)) {
            convertNativeIntegerClassToVar(params.get(i)->utype, methodParam->utype, code, ast);
            code.inject(code.getInjector(stackInjector));
        } else {
            params.get(i)->utype->getCode().inject(stackInjector);
            code.inject(code.size(), params.get(i)->utype->getCode());
        }
    }

    if(resolvedMethod->fnType == fn_delegate) {
        code.addIr(OpBuilder::invokeDelegate(resolvedMethod->address, params.size(), resolvedMethod->flags.find(STATIC)));
    } else {
        if(resolvedMethod->fnType != fn_ptr)
            code.addIr(OpBuilder::call(resolvedMethod->address));
        else {
            if(params.size() == 0) {
                code.addIr(OpBuilder::loadValue(EBX))
                        .addIr(OpBuilder::calld(EBX));
            } else {
                code.addIr(OpBuilder::smov(EBX, -params.size()))
                        .addIr(OpBuilder::calld(EBX));
            }
        }
    }
}

void Compiler::convertNativeIntegerClassToVar(Utype *clazz, Utype *paramUtype, CodeHolder &code, Ast *ast) {
    Field *valueField = clazz->getClass()->getField("value", true);

    if(valueField) {
        compileFieldType(valueField);

        code.inject(clazz->getCode());
        code.inject(clazz->getCode().getInjector(ptrInjector));
        code
                .addIr(OpBuilder::movn(valueField->address))
                .addIr(OpBuilder::movi(0, ADX))
                .addIr(OpBuilder::iaload(EBX, ADX));

        if (paramUtype->getResolvedType()->type != VAR) {
            dataTypeToOpcode(paramUtype->getResolvedType()->type, EBX, EBX, code);
        }

        code.getInjector(stackInjector)
                .addIr(OpBuilder::rstore(EBX)); // todo: add all stack injectors here
    } else
        errors->createNewError(GENERIC, ast->line, ast->col, "could not locate field `value` inside of `" + clazz->toString() + "`");
}

void Compiler::convertUtypeToNativeClass(Utype *clazz, Utype *paramUtype, CodeHolder &code, Ast *ast) {
    List<Field*> params;
    List<AccessFlag> flags;
    Method *constructor; // TODO: take note that I may not be doing pre equals validation on the param before I call this function

    flags.add(PUBLIC);
    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    params.add(new Field(paramUtype->getResolvedType()->type, 0, "", currentScope()->klass, flags, meta, stl_stack, 0));
    params.get(0)->utype = paramUtype;
    params.get(0)->isArray = paramUtype->isArray();

    if((constructor = findFunction(clazz->getClass(), clazz->getClass()->name, params, ast, false, fn_constructor, true)) != NULL) {
        validateAccess(constructor, ast);

        code.addIr(OpBuilder::newClass(clazz->getClass()->address))
            .addIr(OpBuilder::dup());

        paramUtype->getCode().inject(stackInjector);
        code.inject(paramUtype->getCode());
        code.addIr(OpBuilder::call(constructor->address));

        code.freeInjectors();
        code.getInjector(ptrInjector)
                .addIr(OpBuilder::popObject2());

        code.getInjector(removeFromStackInjector)
                .addIr(OpBuilder::pop());
    } else {
        errors->createNewError(GENERIC, ast->line,  ast->col, "Support class `" + clazz->toString() + "` does not have constructor for type `"
                                                                          + paramUtype->toString() + "`");
    } // todo: add injectors for stack and ebx here

    params.get(0)->utype = NULL;
    freeListPtr(params);
}

bool Compiler::isUtypeClassConvertableToVar(Utype *dest, Utype *clazz) {
    if(dest->getResolvedType() && clazz->getResolvedType()) {
        DataType type = dest->getResolvedType()->type;

        if (isUtypeClass(clazz, Obfuscater::getModule("std"), 10, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong", "uint")) {
            return !dest->isArray() && (type <= VAR);
        }
    }

    return false;
}

bool Compiler::isUtypeConvertableToNativeClass(Utype *dest, Utype *src) {
    if(dest->getResolvedType() && src->getResolvedType()) {
        DataType type = src->getResolvedType()->type;

        if (isUtypeClass(dest, Obfuscater::getModule("std"), 1, "string") && type == _INT8 && src->isArray()) {
            return true;
        } else if (isUtypeClass(dest, Obfuscater::getModule("std"), 10, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong", "uint")) {
            return !src->isArray() && (type <= VAR);
        }
    }

    return false;
}

expression_type Compiler::utypeToExpressionType(Utype *utype) {
    if(utype->getType() == utype_field)
        return utypeToExpressionType(((Field*)utype->getResolvedType())->utype);
    else if(utype->getType() == utype_method)
        return exp_var;
    else if(utype->getType() == utype_function_ptr)
        return exp_var;
    else if(utype->getType() == utype_class)
        return exp_class;
    else if(utype->getType() == utype_literal)
        return exp_var;
    else if(utype->getType() == utype_native) {
        if(utype->getResolvedType()->type >= _INT8 && utype->getResolvedType()->type <= VAR)
            return exp_var;
        else if(utype->getResolvedType()->type == OBJECT)
            return exp_object;
        else if(utype->getResolvedType()->type == NIL)
            return exp_nil;
    }

    return exp_undefined;
}

void Compiler::compoundAssignValue(Expression* expr, Token &operand, Expression &leftExpr, Expression &rightExpr, Ast* ast) {
    CodeHolder *resultCode = &expr->utype->getCode();

    if(leftExpr.utype->getType() == utype_class) {
        Method *overload;
        List<Field *> params;

        params.add(new Field());
        expressionToParam(rightExpr, params.get(0));

        if ((overload = findFunction(leftExpr.utype->getClass(), "operator" + operand.getValue(),
                                     params, ast, true, fn_op_overload)) != NULL) {
            compileMethodReturnType(overload, overload->ast, false);

            validateAccess(overload, ast);
            expr->utype->copy(overload->utype);

            resultCode->inject(leftExpr.utype->getCode());
            resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
            pushParametersToStackAndCall(ast, overload, params, *resultCode);

            expr->utype->getCode().freeInjectors();
            if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                || expr->utype->getResolvedType()->type == CLASS) {
                expr->utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::popObject2());

                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
            } else if (expr->utype->getResolvedType()->isVar()) {
                expr->utype->getCode().getInjector(ebxInjector)
                        .addIr(OpBuilder::loadValue(EBX));

                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
            }
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col,
                                   " incompatible types, cannot convert `" + params.get(0)->utype->toString() + "` to `" +
                                   leftExpr.utype->toString()
                                   + "`.");
        }

        freeListPtr(params);
    } else if(leftExpr.utype->getType() == utype_native) {
        stringstream injector;
        injector << operand.getValue() << "{injector}";
        string inj = injector.str();
        if(!leftExpr.utype->getCode().getInjector(inj).ir32.empty()) {
            expr->utype->copy(leftExpr.utype);

            resultCode->inject(rightExpr.utype->getCode());
            resultCode->inject(rightExpr.utype->getCode().getInjector(stackInjector));
            resultCode->inject(leftExpr.utype->getCode());
            resultCode->inject(leftExpr.utype->getCode().getInjector(ebxInjector));
            resultCode->inject(leftExpr.utype->getCode().getInjector(inj));
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + leftExpr.utype->toString() + "` must be a variable, class, or indexer");
        }
    } else { // utype_field
        Field *field = (Field*)leftExpr.utype->getResolvedType();
        if(field->flags.find(flg_CONST)) {
            errors->createNewError(GENERIC, ast->line, ast->col, "operator `" + operand.getValue()
                        +  "` cannot be applied to constant field `" + leftExpr.utype->toString() + "`.");
        }

        if(field->getter != NULL
           && (currentScope()->currentFunction == NULL ||
               !(currentScope()->currentFunction == field->setter || currentScope()->currentFunction == field->getter))) {

            leftExpr.utype->getCode().getInjector(ptrInjector).free();
            leftExpr.utype->getCode().getInjector(removeFromStackInjector).free();
            leftExpr.utype->getCode().removeIrEnd(leftExpr.utype->getCode().getInjector(getterInjector).ir32.at(0));

            if(field->locality == stl_stack) {
                if(field->flags.find(STATIC))
                    leftExpr.utype->getCode().addIr(OpBuilder::movg(field->owner->address));
                leftExpr.utype->getCode().addIr(OpBuilder::movn(field->address));
            }
            else
                leftExpr.utype->getCode().addIr(OpBuilder::tlsMovl(field->address));

            leftExpr.utype->getCode().getInjector(stackInjector).free()
                    .addIr(OpBuilder::pushObject());
        }

        if(field->utype->getClass() != NULL) {
            Method *overload;
            List<Field *> params;

            params.add(new Field());
            expressionToParam(rightExpr, params.get(0));

            if ((overload = findFunction(leftExpr.utype->getClass(), "operator" + operand.getValue(),
                                         params, ast, true, fn_op_overload)) != NULL) {
                compileMethodReturnType(overload, overload->ast, false);

                validateAccess(overload, ast);
                expr->utype->copy(overload->utype);

                resultCode->inject(leftExpr.utype->getCode());
                resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
                pushParametersToStackAndCall(ast, overload, params, *resultCode);

                expr->utype->getCode().freeInjectors();
                if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                    || expr->utype->getResolvedType()->type == CLASS) {
                    expr->utype->getCode().getInjector(ptrInjector)
                            .addIr(OpBuilder::popObject2());

                    expr->utype->getCode().getInjector(removeFromStackInjector)
                            .addIr(OpBuilder::pop());
                } else if (expr->utype->getResolvedType()->isVar()) {
                    expr->utype->getCode().getInjector(ebxInjector)
                            .addIr(OpBuilder::loadValue(EBX));

                    expr->utype->getCode().getInjector(removeFromStackInjector)
                            .addIr(OpBuilder::pop());
                }
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " incompatible types, cannot convert `" + params.get(0)->utype->toString() + "` to `" +
                                       leftExpr.utype->toString()
                                       + "`.");
            }

            freeListPtr(params);
        } else {
            if(field->utype->getResolvedType()->type <= _UINT64 || field->utype->getResolvedType()->type == VAR) {
                if(!field->isArray) {
                    if(rightExpr.utype->isRelated(field->utype) || ((field->type <= _UINT64 || field->type == VAR) && isUtypeClassConvertableToVar(field->utype, rightExpr.utype))) {
                        bool integerField = (field->type >= _INT8 && field->type <= _UINT64);
                        expr->utype->copy(field->utype);

                        if(isUtypeClassConvertableToVar(field->utype, rightExpr.utype)) {
                            CodeHolder tmp;
                            convertNativeIntegerClassToVar(rightExpr.utype, field->utype, tmp, ast);

                            rightExpr.utype->getCode().free()
                                    .inject(tmp);

                            rightExpr.utype->getCode().getInjector(stackInjector)
                                    .inject(tmp.getInjector(stackInjector));
                            tmp.free();
                        }

                        if (field->local) {
                            resultCode->inject(rightExpr.utype->getCode());
                            resultCode->inject(rightExpr.utype->getCode().getInjector(ebxInjector));

                            if(integerField && operand != "&=")
                                resultCode->addIr(OpBuilder::loadl(ECX, field->address));

                            if(operand == "+=") {
                                if(integerField) {
                                    resultCode->addIr(OpBuilder::add(EBX, ECX, EBX));
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::smovr2(EBX, field->address));
                                } else
                                    resultCode->addIr(OpBuilder::addl(EBX, field->address));
                            } else if(operand == "-=") {
                                if(integerField) {
                                    resultCode->addIr(OpBuilder::sub(EBX, ECX, EBX));
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::smovr2(EBX, field->address));
                                } else
                                    resultCode->addIr(OpBuilder::subl(EBX, field->address));
                            } else if(operand == "*=") {
                                if(integerField) {
                                    resultCode->addIr(OpBuilder::mul(EBX, ECX, EBX));
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::smovr2(EBX, field->address));
                                } else
                                    resultCode->addIr(OpBuilder::mull(EBX, field->address));
                            } else if(operand == "/=") {
                                if(integerField) {
                                    resultCode->addIr(OpBuilder::div(EBX, ECX, EBX));
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::smovr2(EBX, field->address));
                                } else
                                    resultCode->addIr(OpBuilder::divl(EBX, field->address));
                            } else if(operand == "%=") {
                                if(integerField) {
                                    resultCode->addIr(OpBuilder::mod(EBX, ECX, EBX));
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::smovr2(EBX, field->address));
                                } else
                                    resultCode->addIr(OpBuilder::modl(EBX, field->address));
                            } else if(operand == "&=") {
                                resultCode->addIr(OpBuilder::andl(EBX, field->address));
                            } else if(operand == "|=") {
                                if(integerField) {
                                    resultCode->addIr(OpBuilder::_or(ECX, EBX));
                                    dataTypeToOpcode(field->type, CMT, CMT, *resultCode);
                                    resultCode->addIr(OpBuilder::smovr2(CMT, field->address));
                                } else
                                    resultCode->addIr(OpBuilder::orl(EBX, field->address));
                            } else if(operand == "^=") {
                                if(integerField) {
                                    resultCode->addIr(OpBuilder::_xor(ECX, EBX));
                                    dataTypeToOpcode(field->type, CMT, CMT, *resultCode);
                                    resultCode->addIr(OpBuilder::smovr2(CMT, field->address));
                                } else
                                    resultCode->addIr(OpBuilder::xorl(EBX, field->address));
                            } else {
                                errors->createNewError(GENERIC, ast->line, ast->col,
                                                       " illegal use of operator `"+ operand.getValue() + "` on field `" + field->toString() + "`.");
                            }

                            if(integerField && (operand == "|=" || operand == "^=")) {
                                resultCode->getInjector(ebxInjector)
                                        .addIr(OpBuilder::movr(EBX, CMT));
                                resultCode->getInjector(stackInjector)
                                        .addIr(OpBuilder::rstore(CMT));
                            } else {
                                resultCode->getInjector(stackInjector)
                                        .addIr(OpBuilder::rstore(EBX));
                            }
                        } else {
                            resultCode->inject(rightExpr.utype->getCode());
                            resultCode->inject(rightExpr.utype->getCode().getInjector(stackInjector));
                            resultCode->inject(leftExpr.utype->getCode());
                            resultCode->inject(leftExpr.utype->getCode().getInjector(ptrInjector));

                            resultCode->addIr(OpBuilder::movi(0, ADX));
                            resultCode->addIr(OpBuilder::iaload(ECX, ADX));
                            resultCode->addIr(OpBuilder::loadValue(EBX));

                            if(operand == "+=") {
                                resultCode->addIr(OpBuilder::add(EBX, ECX, EBX));

                                if(integerField) {
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                                } else
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                            } else if(operand == "-=") {
                                resultCode->addIr(OpBuilder::sub(EBX, ECX, EBX));

                                if(integerField) {
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                                } else
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                            } else if(operand == "*=") {
                                resultCode->addIr(OpBuilder::mul(EBX, ECX, EBX));

                                if(integerField) {
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                                } else
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                            } else if(operand == "/=") {
                                resultCode->addIr(OpBuilder::div(EBX, ECX, EBX));

                                if(integerField) {
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                                } else
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                            } else if(operand == "%=") {
                                resultCode->addIr(OpBuilder::mod(EBX, ECX, EBX));

                                if(integerField) {
                                    dataTypeToOpcode(field->type, EBX, EBX, *resultCode);
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                                } else
                                    resultCode->addIr(OpBuilder::rmov(ADX, EBX));
                            } else if(operand == "&=") {
                                resultCode->addIr(OpBuilder::_and(ECX, EBX));
                                resultCode->addIr(OpBuilder::rmov(ADX, CMT));
                            } else if(operand == "|=") {
                                resultCode->addIr(OpBuilder::_or(ECX, EBX));

                                if(integerField) {
                                    dataTypeToOpcode(field->type, CMT, CMT, *resultCode);
                                    resultCode->addIr(OpBuilder::rmov(ADX, CMT));
                                } else
                                    resultCode->addIr(OpBuilder::rmov(ADX, CMT));
                            } else if(operand == "^=") {
                                resultCode->addIr(OpBuilder::_xor(ECX, EBX));

                                if(integerField) {
                                    dataTypeToOpcode(field->type, CMT, CMT, *resultCode);
                                    resultCode->addIr(OpBuilder::rmov(ADX, CMT));
                                } else
                                    resultCode->addIr(OpBuilder::rmov(ADX, CMT));
                            } else {
                                errors->createNewError(GENERIC, ast->line, ast->col,
                                                       " illegal use of operator `"+ operand.getValue() + "` on field `" + field->toString() + "`.");
                            }

                            if(integerField && (operand == "&=" || operand == "|=" || operand == "^=")) {
                                resultCode->getInjector(ebxInjector)
                                        .addIr(OpBuilder::movr(EBX, CMT));
                                resultCode->getInjector(stackInjector)
                                        .addIr(OpBuilder::rstore(CMT));
                            } else {
                                resultCode->getInjector(stackInjector)
                                        .addIr(OpBuilder::rstore(EBX));
                            }
                        }
                    } else {
                        errors->createNewError(GENERIC, ast->line, ast->col,
                                               " field `" + field->toString() + "` is not equal to the expression of type `" + rightExpr.utype->toString() + "`.");
                    }
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col,
                                           " illegal use of operator `"+ operand.getValue() + "` on array field `" + field->toString() + "`.");
                }
            } else  {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " field `" + field->toString() + "` does not allow use of operator `" + operand.getValue()
                                       + "`.");
            }
        }
    }
}

void Compiler::compileBinaryExpression(Expression* expr, Token &operand, Expression &leftExpr, Expression &rightExpr, Ast* ast) {
    CodeHolder *resultCode = &expr->utype->getCode();
    Literal fieldLiteral;

    Literal *leftLiteral = leftExpr.utype->getType() == utype_literal ?
                            (Literal*)leftExpr.utype->getResolvedType() : NULL;
    if(leftLiteral == NULL && leftExpr.utype->getType() == utype_field && isFieldInlined((Field*)leftExpr.utype->getResolvedType())) {
        Field *field = (Field*)leftExpr.utype->getResolvedType();
        if(field->isVar()) {
            fieldLiteral.literalType = numeric_literal;
            fieldLiteral.numericData = getInlinedFieldValue(field);
        }
        else {
            fieldLiteral.literalType = string_literal;
            fieldLiteral.stringData = stringMap.get(getInlinedStringFieldAddress(field));
            fieldLiteral.address = getInlinedStringFieldAddress(field);
        }

        leftLiteral = &fieldLiteral;
    }

    Literal *rightLiteral = rightExpr.utype->getType() == utype_literal ?
                            (Literal*)rightExpr.utype->getResolvedType() : NULL;
    if(rightLiteral == NULL && rightExpr.utype->getType() == utype_field && isFieldInlined((Field*)rightExpr.utype->getResolvedType())) {
        Field *field = (Field*)rightExpr.utype->getResolvedType();
        if(field->isVar()) {
            fieldLiteral.literalType = numeric_literal;
            fieldLiteral.numericData = getInlinedFieldValue(field);
        }
        else {
            fieldLiteral.literalType = string_literal;
            fieldLiteral.stringData = stringMap.get(getInlinedStringFieldAddress(field));
            fieldLiteral.address = getInlinedStringFieldAddress(field);
        }

        rightLiteral = &fieldLiteral;
    }

    if(leftLiteral != NULL) {

        if(leftLiteral->literalType == numeric_literal && rightLiteral != NULL && rightLiteral->literalType == numeric_literal) {
            double result = 0;
            if (operand == "/") {
                result = leftLiteral->numericData / rightLiteral->numericData;
            } else if (operand == "*") {
                result = leftLiteral->numericData * rightLiteral->numericData;
            } else if (operand == "**") {
                result = pow(leftLiteral->numericData, rightLiteral->numericData);
            } else if (operand == "%") {
                result = (long)leftLiteral->numericData % (long)rightLiteral->numericData;
            } else if (operand == "+") {
                result = leftLiteral->numericData + rightLiteral->numericData;
            } else if (operand == "-") {
                result = leftLiteral->numericData - rightLiteral->numericData;
            } else if (operand == ">") {
                result = leftLiteral->numericData > rightLiteral->numericData;
            } else if (operand == "<") {
                result = leftLiteral->numericData < rightLiteral->numericData;
            } else if (operand == ">=") {
                result = leftLiteral->numericData >= rightLiteral->numericData;
            } else if (operand == "<=") {
                result = leftLiteral->numericData <= rightLiteral->numericData;
            } else if (operand == "==") {
                result = leftLiteral->numericData == rightLiteral->numericData;
            } else if (operand == "!=") {
                result = leftLiteral->numericData != rightLiteral->numericData;
            }else if (operand == "<<") {
                result = (long long)leftLiteral->numericData << (long)rightLiteral->numericData;
            } else if (operand == ">>") {
                result = (long long)leftLiteral->numericData >> (long)rightLiteral->numericData;
            } else if (operand == "&") {
                result = (long long)leftLiteral->numericData & (long)rightLiteral->numericData;
            } else if (operand == "|") {
                result = (long long)leftLiteral->numericData | (long)rightLiteral->numericData;
            } else if (operand == "^") {
                result = (long long)leftLiteral->numericData ^ (long)rightLiteral->numericData;
            } else if (operand == "&&") {
                result = leftLiteral->numericData && rightLiteral->numericData;
            } else if (operand == "||") {
                result = leftLiteral->numericData || rightLiteral->numericData;
            }

            if(isWholeNumber(result)) {
                if(result > INT32_MAX) {
                    long constantAddress = constantMap.addIfIndex(result);
                    if(constantAddress >= CONSTANT_LIMIT) {
                        stringstream err;
                        err << "maximum constant limit of (" << CONSTANT_LIMIT << ") reached.";
                        errors->createNewError(INTERNAL_ERROR, leftExpr.ast, err.str());
                    }

                    resultCode->addIr(OpBuilder::ldc(EBX, constantAddress));
                } else
                    resultCode->addIr(OpBuilder::movi(result, EBX));

            } else {
                long constantAddress = constantMap.addIfIndex(result);
                if(constantAddress >= CONSTANT_LIMIT) {
                    stringstream err;
                    err << "maximum constant limit of (" << CONSTANT_LIMIT << ") reached.";
                    errors->createNewError(INTERNAL_ERROR, leftExpr.ast, err.str());
                }
                resultCode->addIr(OpBuilder::ldc(EBX, constantAddress));
            }

            expr->utype->setType(utype_literal);
            expr->utype->setArrayType(false);
            expr->utype->setResolvedType(new Literal(result));

            expr->utype->getCode().getInjector(stackInjector)
                    .addIr(OpBuilder::rstore(EBX));
        } else if(leftLiteral->literalType == string_literal) {

            if(operand == "+" &&
                (rightLiteral != NULL ||
                    (rightExpr.utype->getType() == utype_field && isFieldInlined((Field*)rightExpr.utype->getResolvedType())))) {
                expr->type = exp_var;
                expr->utype->setType(utype_literal);
                expr->utype->setArrayType(true);

                stringstream newStr;
                if(rightExpr.utype->getType() == utype_field) {
                    newStr << leftLiteral->stringData << getInlinedFieldValue((Field*)rightExpr.utype->getResolvedType());
                } else if (rightLiteral->literalType == string_literal) {
                    newStr << leftLiteral->stringData << rightLiteral->stringData;
                } else {
                    newStr << leftLiteral->stringData << rightLiteral->numericData;
                }


                long index = stringMap.addIfIndex(newStr.str());
                if (index >= STRING_LITERAL_LIMIT) {
                    stringstream err;
                    err << "maximum string literal limit of (" << STRING_LITERAL_LIMIT << ") reached.";
                    errors->createNewError(INTERNAL_ERROR, leftExpr.ast, err.str());
                }

                expr->utype->setResolvedType(new Literal(stringMap.get(index), index));
                expr->utype->getCode().addIr(OpBuilder::newString(index));

                expr->utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::popObject2());
            } else if(operand != "&&" && operand != "||"){
                _string_literal:
                List<Expression*> exprs;
                List<Field*> params;
                List<Field*> stringLiteralParam;
                exprs.add(&leftExpr);

                Utype *stringClass = new Utype(resolveClass(Obfuscater::getModule("std"), "string", ast));
                if(stringClass->getResolvedType() != NULL) {
                    expressionsToParams(exprs, stringLiteralParam);
                    convertUtypeToNativeClass(stringClass, stringLiteralParam.get(0)->utype, *resultCode, ast);

                    Method *overload;

                    params.add(new Field());
                    expressionToParam(rightExpr, params.get(0));

                    if ((overload = findFunction(stringClass->getClass(), "operator" + operand.getValue(),
                                                 params, ast, true, fn_op_overload)) != NULL) {
                        compileMethodReturnType(overload, overload->ast, false);

                        validateAccess(overload, ast);
                        expr->utype->copy(overload->utype);

                        pushParametersToStackAndCall(ast, overload, params, *resultCode);

                        expr->utype->getCode().freeInjectors();
                        if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                            || expr->utype->getResolvedType()->type == CLASS) {
                            expr->utype->getCode().getInjector(ptrInjector)
                                    .addIr(OpBuilder::popObject2());

                            expr->utype->getCode().getInjector(removeFromStackInjector)
                                    .addIr(OpBuilder::pop());
                        } else if (expr->utype->getResolvedType()->isVar()) {
                            expr->utype->getCode().getInjector(ebxInjector)
                                    .addIr(OpBuilder::loadValue(EBX));

                            expr->utype->getCode().getInjector(removeFromStackInjector)
                                    .addIr(OpBuilder::pop());
                        }
                    } else {
                        errors->createNewError(GENERIC, ast->line, ast->col,
                                               " illegal use of operator `" + operand.getValue() +
                                               "` with expressions of `" +
                                               stringLiteralParam.get(0)->utype->toString() + "` and `" +
                                               params.get(0)->utype->toString() + "`.");
                    }

                    freeListPtr(params);
                    freeListPtr(stringLiteralParam);
                    freePtr(stringClass);
                }
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " illegal use of operator `" + operand.getValue() +
                                       "` with expression `" +
                                       leftExpr.utype->toString() + "`.");
            }
        } else {
            processSimpleValue:
            if((rightLiteral != NULL && rightLiteral->literalType == string_literal) && !(rightExpr.type == exp_var && rightExpr.utype->isArray())
                && !isUtypeClassConvertableToVar(leftExpr.utype, rightExpr.utype)) {
                incompatibleExpressions:
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " attempt to use operand `" + operand.getValue() + "` on differing literal values of `" + leftExpr.utype->toString()
                                       + "` and `" + rightExpr.utype->toString() + "`.");
            } else if(operand != "&&" && operand != "||") {
                if(isUtypeClassConvertableToVar(leftExpr.utype, rightExpr.utype)) {
                    CodeHolder tmp;
                    convertNativeIntegerClassToVar(rightExpr.utype, leftExpr.utype, tmp, ast);

                    rightExpr.utype->getCode().free()
                            .inject(tmp);

                    rightExpr.utype->getCode().getInjector(stackInjector)
                            .inject(tmp.getInjector(stackInjector));
                    tmp.free();
                }

                if(operand == "==") {
                    if(!rightExpr.utype->isRelated(leftExpr.utype)) {
                        errors->createNewError(GENERIC, ast->line, ast->col, "operator `" + operand.getValue()
                             +  "` cannot be applied to expressions of type `" + leftExpr.utype->toString() + "` and `" + rightExpr.utype->toString() + "`");
                    }
                } else {
                    if(rightExpr.type != exp_var) {
                        errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + rightExpr.utype->toString() +  "` must evaluate to a `var`");
                    }
                }

                resultCode->inject(leftExpr.utype->getCode());
                resultCode->inject(leftExpr.utype->getCode().getInjector(ebxInjector));
                resultCode->addIr(OpBuilder::rstore(EBX));
                resultCode->inject(rightExpr.utype->getCode());
                resultCode->inject(rightExpr.utype->getCode().getInjector(ebxInjector));
                resultCode->addIr(OpBuilder::loadValue(ECX));


                if (operand == "/") {
                    resultCode->addIr(OpBuilder::div(EBX, ECX, EBX));
                } else if (operand == "*") {
                    resultCode->addIr(OpBuilder::mul(EBX, ECX, EBX));
                } else if (operand == "**") {
                    resultCode->addIr(OpBuilder::exp(EBX, ECX, EBX));
                } else if (operand == "%") {
                    resultCode->addIr(OpBuilder::mod(EBX, ECX, EBX));
                } else if (operand == "+") {
                    resultCode->addIr(OpBuilder::add(EBX, ECX, EBX));
                } else if (operand == "-") {
                    resultCode->addIr(OpBuilder::sub(EBX, ECX, EBX));
                } else if (operand == ">") {
                    resultCode->addIr(OpBuilder::gt(ECX, EBX));
                } else if (operand == "<") {
                    resultCode->addIr(OpBuilder::lt(ECX, EBX));
                } else if (operand == ">=") {
                    resultCode->addIr(OpBuilder::ge(ECX, EBX));
                } else if (operand == "<=") {
                    resultCode->addIr(OpBuilder::le(ECX, EBX));
                } else if (operand == "==") {
                    resultCode->addIr(OpBuilder::te(ECX, EBX));
                } else if (operand == "!=") {
                    resultCode->addIr(OpBuilder::tne(ECX, EBX));
                }else if (operand == "<<") {
                    resultCode->addIr(OpBuilder::shl(EBX, ECX, EBX));
                } else if (operand == ">>") {
                    resultCode->addIr(OpBuilder::shr(EBX, ECX, EBX));
                } else if (operand == "&") {
                    resultCode->addIr(OpBuilder::uand(ECX, EBX));
                } else if (operand == "|") {
                    resultCode->addIr(OpBuilder::_or(ECX, EBX));
                } else if (operand == "^") {
                    resultCode->addIr(OpBuilder::_xor(ECX, EBX));
                }

                expr->utype->setType(utype_native);
                expr->utype->setArrayType(false);
                expr->utype->setResolvedType(varUtype->getResolvedType());
                if(operand == ">" || operand == "<" || operand == ">=" || operand == "<="
                    || operand == "==" || operand == "!=") {
                    expr->utype->getCode().getInjector(ebxInjector)
                            .addIr(OpBuilder::movr(EBX, CMT));
                    expr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(CMT));
                } else {
                    expr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(EBX));
                }
            } else {
                resultCode->inject(leftExpr.utype->getCode());
                resultCode->inject(leftExpr.utype->getCode().getInjector(ebxInjector));

                long rightExpresionSize = rightExpr.utype->getCode().size() +
                        rightExpr.utype->getCode().getInjector(ebxInjector).size();

                if(operand == "&&") {
                    resultCode->addIr(OpBuilder::skipifne(EBX, rightExpresionSize + 1));
                } else {
                    resultCode->addIr(OpBuilder::skipife(EBX, rightExpresionSize + 1));
                }


                resultCode->inject(rightExpr.utype->getCode());
                resultCode->inject(rightExpr.utype->getCode().getInjector(ebxInjector));

                expr->utype->setType(utype_native);
                expr->utype->setArrayType(false);
                expr->utype->setResolvedType(varUtype->getResolvedType());

                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::rstore(EBX));
            }
        }
    } else if(leftExpr.utype->getType() == utype_field) {
        Field *field = (Field*)leftExpr.utype->getResolvedType();

        if(field->utype->getType() == utype_class) {
            goto processClassOverload;
        } else if(field->utype->getResolvedType()->type <= VAR && !field->isArray) {
            goto processSimpleValue;
        }  else if((field->utype->getResolvedType()->type == OBJECT || field->isArray)
            && (operand == "==" || operand == "!=")) {
            goto instanceTest;
        } else
            goto incompatibleExpressions;
    } else if(leftExpr.utype->getType() == utype_native) {
        if(leftExpr.utype->getResolvedType()->type <= VAR && !leftExpr.utype->isArray()) {
            goto processSimpleValue;
        } else if((leftExpr.utype->getResolvedType()->type == OBJECT || leftExpr.utype->isArray())
            && (operand == "==" || operand == "!=")) {
            goto instanceTest;
        } else
            goto incompatibleExpressions;
    } else if((operand == "==" || operand == "!=") && leftExpr.utype->getResolvedType()->type == OBJECT) {
        instanceTest:
        if((rightExpr.utype->getResolvedType()->type == OBJECT || rightExpr.utype->isArray())
            || rightExpr.utype->getClass() != NULL) {
            resultCode->inject(leftExpr.utype->getCode());
            resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
            resultCode->inject(rightExpr.utype->getCode());
            resultCode->inject(rightExpr.utype->getCode().getInjector(stackInjector));
            resultCode->addIr(OpBuilder::itest(EBX));

            if(operand == "!=")
                resultCode->addIr(OpBuilder::_not(EBX, EBX));

            expr->utype->setType(utype_native);
            expr->utype->setArrayType(false);
            expr->utype->setResolvedType(varUtype->getResolvedType());

            expr->utype->getCode().getInjector(stackInjector)
                    .addIr(OpBuilder::rstore(EBX));
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col,
                                   " illegal use of operator `" + operand.getValue() +
                                   "` with expression `" +
                                   leftExpr.utype->toString() + "`.");
        }
    }
    else if(leftExpr.utype->getType() == utype_class){
        processClassOverload:
        Method *overload;
        List<Field *> params;

        params.add(new Field());
        expressionToParam(rightExpr, params.get(0));

        if ((overload = findFunction(leftExpr.utype->getClass(), "operator" + operand.getValue(),
                                     params, ast, true, fn_op_overload)) != NULL) {
            compileMethodReturnType(overload, overload->ast, false);

            validateAccess(overload, ast);
            expr->utype->copy(overload->utype);

            resultCode->inject(leftExpr.utype->getCode());
            resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
            pushParametersToStackAndCall(ast, overload, params, *resultCode);

            expr->utype->getCode().freeInjectors();
            if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                || expr->utype->getResolvedType()->type == CLASS) {
                expr->utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::popObject2());

                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
            } else if (expr->utype->getResolvedType()->isVar()) {
                expr->utype->getCode().getInjector(ebxInjector)
                        .addIr(OpBuilder::loadValue(EBX));

                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
            }
        } else {
            if(operand == "==" || operand == "!=") {
                if(params.get(0)->utype->getResolvedType()->type == OBJECT || params.get(0)->utype->getClass() != NULL) {
                    resultCode->inject(leftExpr.utype->getCode());
                    resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
                    resultCode->inject(params.get(0)->utype->getCode());
                    resultCode->inject(params.get(0)->utype->getCode().getInjector(stackInjector));
                    resultCode->addIr(OpBuilder::itest(EBX));

                    if(operand == "!=")
                        resultCode->addIr(OpBuilder::_not(EBX, EBX));

                    expr->utype->setType(utype_native);
                    expr->utype->setArrayType(false);
                    expr->utype->setResolvedType(varUtype->getResolvedType());

                    expr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(EBX));
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col,
                                           " illegal use of operator `" + operand.getValue() +
                                           "` with expression `" +
                                           leftExpr.utype->toString() + "`.");
                }
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " expression `" + leftExpr.utype->toString() +
                                       "` does not contain an override operator for expresion `" +
                                               params.get(0)->utype->toString()
                                       + "` with operand `" + operand.getValue() + "`.");
            }
        }

        freeListPtr(params);
    } else {
        errors->createNewError(GENERIC, ast->line, ast->col,
                               " illegal use of operator `" + operand.getValue() +
                               "` with expression `" +
                               leftExpr.utype->toString() + "`.");
    }

    expr->type = utypeToExpressionType(expr->utype);
}

void Compiler::assignValue(Expression* expr, Token &operand, Expression &leftExpr, Expression &rightExpr, Ast* ast, bool allowOverloading, bool allowSetter) {
    CodeHolder *resultCode = &expr->utype->getCode();

    if(leftExpr.utype->getType() == utype_field) {
        Field *field = (Field*)leftExpr.utype->getResolvedType();
        field->initialized = true;
        if(field->flags.find(flg_CONST)) {
            errors->createNewError(GENERIC, ast->line, ast->col, "operator `" + operand.getValue()
                   +  "` cannot be applied to constant field `" + leftExpr.utype->toString() + "`.");
        }

        if(allowSetter && field->setter != NULL
        && (currentScope()->currentFunction == NULL ||
            !(currentScope()->currentFunction == field->setter || currentScope()->currentFunction == field->getter))) {

            if(field->getter != NULL)
                leftExpr.utype->getCode().removeIrEnd(leftExpr.utype->getCode().getInjector(getterInjector).ir32.at(0));

            List<Field *> params;

            params.add(new Field());
            expressionToParam(rightExpr, params.get(0));

            if(leftExpr.utype->isRelated(params.get(0)->utype)
                || isUtypeConvertableToNativeClass(field->setter->params.get(0)->utype, params.get(0)->utype)
                   || isUtypeClassConvertableToVar(field->setter->params.get(0)->utype, params.get(0)->utype)) {

                if(field->locality != stl_thread && !field->flags.find(STATIC)) {
                    resultCode->inject(leftExpr.utype->getCode());
                    resultCode->addIr(OpBuilder::pushObject());
                }

                validateAccess(field->setter, ast);

                pushParametersToStackAndCall(ast, field->setter, params, *resultCode);

                expr->utype->getCode().freeInjectors();
                expr->utype->copy(field->setter->utype);
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " field `" + field->fullName + "` has a setter that requires type `" +
                                       field->utype->toString()
                                       + "` which differs from the provided type of `" + params.get(0)->utype->toString() + "`.");
            }
        }
        else {
            if(field->getter != NULL
            && (currentScope()->currentFunction == NULL ||
                !(currentScope()->currentFunction == field->setter || currentScope()->currentFunction == field->getter))) {

                leftExpr.utype->getCode().getInjector(ptrInjector).free();
                leftExpr.utype->getCode().getInjector(removeFromStackInjector).free();
                leftExpr.utype->getCode().removeIrEnd(leftExpr.utype->getCode().getInjector(getterInjector).ir32.at(0));

                if(field->locality == stl_stack) {
                    if(field->flags.find(STATIC))
                        leftExpr.utype->getCode().addIr(OpBuilder::movg(field->owner->address));
                    leftExpr.utype->getCode().addIr(OpBuilder::movn(field->address));
                }
                else
                    leftExpr.utype->getCode().addIr(OpBuilder::tlsMovl(field->address));

                leftExpr.utype->getCode().getInjector(stackInjector).free()
                    .addIr(OpBuilder::pushObject());
            }

            if ((field->type == CLASS && rightExpr.utype->getClass() && rightExpr.utype->isRelated(leftExpr.utype))
                || ((field->type == OBJECT || field->type == FNPTR) && leftExpr.utype->isRelated(rightExpr.utype))
                || ((field->type <= _UINT64 || field->type == VAR) && isUtypeClassConvertableToVar(field->utype, rightExpr.utype))
                || ((field->type <= _UINT64 || field->type == VAR) && leftExpr.utype->isRelated(rightExpr.utype))
                || (field->type == CLASS && rightExpr.utype->isNullType())
                || (!allowOverloading && isUtypeConvertableToNativeClass(field->utype, rightExpr.utype))) {

                expr->utype->copy(field->utype);
                bool integerField = (field->type >= _INT8 && field->type <= _UINT64);
                if (field->type <= VAR) {
                    if (field->isArray)
                        goto object_assignment;

                    if(field->utype->getResolvedType() != NULL && field->utype->getResolvedType()->type == FNPTR
                        && isLambdaUtype(rightExpr.utype))
                        fullyQualifyLambda(field->utype, rightExpr.utype);

                    if(isUtypeClassConvertableToVar(field->utype, rightExpr.utype)) {
                        CodeHolder tmp;
                        convertNativeIntegerClassToVar(rightExpr.utype, field->utype, tmp, ast);

                        rightExpr.utype->getCode().free()
                            .inject(tmp);

                        rightExpr.utype->getCode().getInjector(stackInjector)
                            .inject(tmp.getInjector(stackInjector));
                        tmp.free();
                    }

                    resultCode->inject(rightExpr.utype->getCode());

                    resultCode->getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(EBX));

                    if (field->local) {
                        resultCode->inject(rightExpr.utype->getCode().getInjector(ebxInjector));

                        if (integerField)
                            dataTypeToOpcode(field->type, EBX, EBX, *resultCode);

                        if(field->locality == stl_thread) {
                            resultCode->addIr(OpBuilder::tlsMovl(field->address))
                                    .addIr(OpBuilder::movi(0, ADX))
                                    .addIr(OpBuilder::rmov(ADX, EBX));

                            resultCode->getInjector(stackInjector).free();
                            resultCode->getInjector(ebxInjector)
                                    .addIr(OpBuilder::tlsMovl(field->address))
                                    .addIr(OpBuilder::movi(0, ADX))
                                    .addIr(OpBuilder::checklen(ADX))
                                    .addIr(OpBuilder::iaload(EBX, ADX));

                            resultCode->getInjector(stackInjector)
                                    .addIr(OpBuilder::tlsMovl(field->address))
                                    .addIr(OpBuilder::movi(0, ADX))
                                    .addIr(OpBuilder::checklen(ADX))
                                    .addIr(OpBuilder::iaload(EBX, ADX))
                                    .addIr(OpBuilder::rstore(EBX));

                        } else
                            resultCode->addIr(OpBuilder::smovr2(EBX, field->address));
                        return;
                    } else {
                        resultCode->inject(rightExpr.utype->getCode().getInjector(stackInjector));
                        resultCode->inject(leftExpr.utype->getCode());
                        resultCode->inject(leftExpr.utype->getCode().getInjector(ptrInjector));

                        resultCode->addIr(OpBuilder::loadValue(EBX));

                        if (integerField)
                            dataTypeToOpcode(field->type, EBX, EBX, *resultCode);

                        resultCode->addIr(OpBuilder::movi(0, ADX))
                                .addIr(OpBuilder::rmov(ADX, EBX));
                    }
                } else if (field->type == CLASS || field->type == OBJECT) {
                    object_assignment:

                    if(isUtypeConvertableToNativeClass(field->utype, rightExpr.utype)) {
                        CodeHolder tmp;
                        convertUtypeToNativeClass(field->utype, rightExpr.utype, tmp, ast);

                        rightExpr.utype->getCode().free()
                                .inject(tmp);

                        rightExpr.utype->getCode().getInjector(stackInjector)
                                .inject(tmp.getInjector(stackInjector));
                        rightExpr.utype->getCode().getInjector(ptrInjector)
                                .inject(tmp.getInjector(ptrInjector));
                        tmp.free();
                    }

                    if (field->local) {
                        resultCode->inject(rightExpr.utype->getCode());
                        resultCode->inject(rightExpr.utype->getCode().getInjector(ptrInjector));

                        if(field->locality == stl_thread) {
                            resultCode->addIr(OpBuilder::tlsMovl(field->address))
                                    .addIr(OpBuilder::popObject());

                            expr->utype->getCode().getInjector(stackInjector)
                                    .addIr(OpBuilder::tlsMovl(field->address))
                                    .addIr(OpBuilder::pushObject());
                            expr->utype->getCode().getInjector(ptrInjector)
                                    .addIr(OpBuilder::tlsMovl(field->address));
                        } else {
                            resultCode->addIr(OpBuilder::smovr3(field->address));

                            expr->utype->getCode().getInjector(stackInjector)
                                    .addIr(OpBuilder::pushl(field->address));
                            expr->utype->getCode().getInjector(ptrInjector)
                                    .addIr(OpBuilder::movl(field->address));
                        }
                        return;
                    } else {
                        if(rightExpr.utype->isNullType()) {
                            resultCode->inject(leftExpr.utype->getCode());
                            resultCode->inject(leftExpr.utype->getCode().getInjector(ptrInjector));
                            resultCode->addIr(OpBuilder::del());
                        } else {
                            resultCode->inject(rightExpr.utype->getCode());
                            resultCode->inject(rightExpr.utype->getCode().getInjector(stackInjector));
                            resultCode->inject(leftExpr.utype->getCode());
                            resultCode->inject(leftExpr.utype->getCode().getInjector(ptrInjector));
                            resultCode->addIr(OpBuilder::popObject());
                        }

                        expr->utype->getCode().getInjector(stackInjector)
                                .addIr(OpBuilder::pushObject());
                    }
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col,
                                           " expression `" + rightExpr.utype->toString() + "` assigned to field `" +
                                           field->toString()
                                           + "` does not allow values to be assigned to it.");
                }
            } else if (field->utype->getClass() && allowOverloading) {
                Method *overload;
                List<Field *> params;

                params.add(new Field());
                expressionToParam(rightExpr, params.get(0));

                if ((overload = findFunction(field->utype->getClass(), "operator=",
                                             params, ast, true, fn_op_overload)) != NULL) {
                    compileMethodReturnType(overload, overload->ast, false);

                    validateAccess(overload, ast);
                    expr->utype->copy(overload->utype);

                    resultCode->inject(leftExpr.utype->getCode());
                    resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
                    pushParametersToStackAndCall(ast, overload, params, *resultCode);

                    expr->utype->getCode().freeInjectors();
                    if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                        || expr->utype->getResolvedType()->type == CLASS) {
                        expr->utype->getCode().getInjector(ptrInjector)
                                .addIr(OpBuilder::popObject2());

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    } else if (expr->utype->getResolvedType()->isVar()) {
                        expr->utype->getCode().getInjector(ebxInjector)
                                .addIr(OpBuilder::loadValue(EBX));

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    }
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col,
                                           " incompatible types, cannot convert `" + params.get(0)->utype->toString() + "` to `" +
                                           leftExpr.utype->toString()
                                           + "`.");
                }

                freeListPtr(params);
            } else if(isUtypeClass(field->utype, Obfuscater::getModule("std"), 10, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong", "uint")){
                Method *constr;
                List<Field *> params;

                params.add(new Field());
                expressionToParam(rightExpr, params.get(0));

                if ((constr = findFunction(field->utype->getClass(), field->utype->getClass()->name, params, ast, false, fn_constructor, true)) != NULL) {
                    compileMethodReturnType(constr, constr->ast, false);

                    validateAccess(constr, ast);
                    expr->utype->copy(field->utype);

                    resultCode->addIr(OpBuilder::newClass(field->utype->getClass()->address))
                            .addIr(OpBuilder::dup());
                    pushParametersToStackAndCall(ast, constr, params, *resultCode);
                    resultCode->inject(leftExpr.utype->getCode());
                    resultCode->inject(leftExpr.utype->getCode().getInjector(ptrInjector));
                    resultCode->addIr(OpBuilder::popObject());

                    expr->utype->getCode().freeInjectors();
                    expr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::pushObject());
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col,
                                           " incompatible types, cannot convert `" + params.get(0)->utype->toString() + "` to `" +
                                           leftExpr.utype->toString() + "`, are you possibly missing a cast?");
                }

                freeListPtr(params);
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " incompatible types, cannot convert `" + rightExpr.utype->toString() +
                                       "` to `" +
                                       leftExpr.utype->toString()
                                       + "`" + (!allowOverloading && leftExpr.utype->getResolvedType()->type == CLASS ? ", is the variable initalized?" : "."));
            }
        }
    } else if(leftExpr.utype->getType() == utype_native) {
        if(!leftExpr.utype->getCode().getInjector("={injector}").ir32.empty()) {
            expr->utype->copy(leftExpr.utype);

            resultCode->inject(rightExpr.utype->getCode());
            resultCode->inject(rightExpr.utype->getCode().getInjector(stackInjector));
            resultCode->inject(leftExpr.utype->getCode());
            resultCode->inject(leftExpr.utype->getCode().getInjector("={injector}"));
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + leftExpr.utype->toString() + "` must be a variable, class, or indexer");
        }
    }
    else { // utype_class

        if(rightExpr.utype->isRelated(leftExpr.utype)) {
            if(rightExpr.utype->isNullType()) {
                resultCode->inject(leftExpr.utype->getCode());
                resultCode->inject(leftExpr.utype->getCode().getInjector(ptrInjector));
                resultCode->addIr(OpBuilder::del());
            } else {
                resultCode->inject(rightExpr.utype->getCode());
                resultCode->inject(rightExpr.utype->getCode().getInjector(stackInjector));
                resultCode->inject(leftExpr.utype->getCode());
                resultCode->inject(leftExpr.utype->getCode().getInjector(ptrInjector));
                resultCode->addIr(OpBuilder::popObject());
            }

            expr->utype->copy(leftExpr.utype);
            expr->utype->getCode().getInjector(stackInjector)
                    .addIr(OpBuilder::pushObject());
        } else if(allowOverloading) {
            Method *overload;
            List<Field *> params;

            params.add(new Field());
            expressionToParam(rightExpr, params.get(0));

            if ((overload = findFunction(leftExpr.utype->getClass(), "operator" + operand.getValue(),
                                         params, ast, true, fn_op_overload)) != NULL) {
                compileMethodReturnType(overload, overload->ast, false);

                validateAccess(overload, ast);
                expr->utype->copy(overload->utype);

                resultCode->inject(leftExpr.utype->getCode());
                resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
                pushParametersToStackAndCall(ast, overload, params, *resultCode);

                expr->utype->getCode().freeInjectors();
                if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                    || expr->utype->getResolvedType()->type == CLASS) {
                    expr->utype->getCode().getInjector(ptrInjector)
                            .addIr(OpBuilder::popObject2());

                    expr->utype->getCode().getInjector(removeFromStackInjector)
                            .addIr(OpBuilder::pop());
                } else if (expr->utype->getResolvedType()->isVar()) {
                    expr->utype->getCode().getInjector(ebxInjector)
                            .addIr(OpBuilder::loadValue(EBX));

                    expr->utype->getCode().getInjector(removeFromStackInjector)
                            .addIr(OpBuilder::pop());
                }
            } else {

                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " incompatible types, cannot convert `" + params.get(0)->utype->toString() + "` to `" +
                                       leftExpr.utype->toString()
                                       + "`.");
            }

            freeListPtr(params);
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col,
                                   " incompatible types, cannot convert `" + rightExpr.utype->toString() + "` to `" +
                                   leftExpr.utype->toString()
                                   + "`.");
        }
    }
}

void Compiler::compileInlineIfExpression(Expression* expr, Ast* ast) {
    Expression condExpr, trueExpr, falseExpr;

    compileExpression(&condExpr, ast->getSubAst(0));
    compileExpression(&trueExpr, ast->getSubAst(1));
    compileExpression(&falseExpr, ast->getSubAst(2));

    if(condExpr.utype->isRelated(varUtype) || isUtypeClassConvertableToVar(varUtype, condExpr.utype)) {
        if((trueExpr.utype->getResolvedType()->isVar() && trueExpr.utype->isRelated(falseExpr.utype))
            || (falseExpr.utype->getResolvedType()->isVar() && falseExpr.utype->isRelated(trueExpr.utype))
            || (trueExpr.type == exp_class && trueExpr.utype->isRelated(falseExpr.utype))
            || trueExpr.utype->equals(falseExpr.utype)) {
            if(isUtypeClassConvertableToVar(varUtype, condExpr.utype)) {
                CodeHolder tmp;
                convertNativeIntegerClassToVar(condExpr.utype, varUtype, tmp, ast);
                condExpr.utype->getCode().free()
                    .inject(tmp);
                condExpr.utype->getCode().copyInjectors(tmp);
            }

            expr->utype->copy(trueExpr.utype);
            CodeHolder &code = expr->utype->getCode();

            condExpr.utype->getCode().inject(ebxInjector);
            code.inject(condExpr.utype->getCode());

            code.addIr(OpBuilder::skipifne(EBX, trueExpr.utype->getCode().size() + 1));
            if(trueExpr.type == exp_var) {
                trueExpr.utype->getCode().inject(ebxInjector);
                code.inject(trueExpr.utype->getCode());
                code.addIr(OpBuilder::skip(falseExpr.utype->getCode().size()));
                falseExpr.utype->getCode().inject(ebxInjector);
                code.inject(falseExpr.utype->getCode());

                code.getInjector(stackInjector)
                        .addIr(OpBuilder::rstore(EBX));
            } else {
                trueExpr.utype->getCode().inject(ptrInjector);
                code.inject(trueExpr.utype->getCode());
                code.addIr(OpBuilder::skip(falseExpr.utype->getCode().size()));
                falseExpr.utype->getCode().inject(ptrInjector);
                code.inject(falseExpr.utype->getCode());

                code.getInjector(stackInjector)
                        .addIr(OpBuilder::pushObject());
            }
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, " expression of type `" + trueExpr.utype->toString() + "` is not equal to that of type `"
                + falseExpr.utype->toString() + "`, are you possibly missing a cast?");
        }
    } else {
        errors->createNewError(GENERIC, ast->line, ast->col, " cannot convert expression of type `" + condExpr.utype->toString() + "` to a var");
    }
}

void Compiler::compileBinaryExpression(Expression* expr, Ast* ast) {
    Expression leftExpr, rightExpr;

    RETAIN_TYPE_INFERENCE(true)
    compileExpression(&leftExpr, ast->getSubAst(0));

    if(leftExpr.utype->getType() == utype_function_ptr
       || (leftExpr.utype->getType() == utype_field && ((Field*)leftExpr.utype->getResolvedType())->type == FNPTR)) {
        if(leftExpr.utype->getType() == utype_field) {
            RETAIN_REQUIRED_SIGNATURE((Method*)((Field*)leftExpr.utype->getResolvedType())->utype->getResolvedType())
            compileExpression(&rightExpr, ast->getSubAst(1));
            RESTORE_REQUIRED_SIGNATURE()
        } else {
            RETAIN_REQUIRED_SIGNATURE((Method*)leftExpr.utype->getResolvedType())
            compileExpression(&rightExpr, ast->getSubAst(1));
            RESTORE_REQUIRED_SIGNATURE()
        }
    } else
        compileExpression(&rightExpr, ast->getSubAst(1));
    RESTORE_TYPE_INFERENCE()

    expr->ast = ast;
    Token &operand = ast->getToken(0);
    switch(leftExpr.utype->getType()) {
        case utype_class:
        case utype_field:
        case utype_native:
        case utype_literal: {
            compileBinaryExpression(expr, operand, leftExpr, rightExpr, ast);
            break;
        }
        default: {
            errors->createNewError(GENERIC, ast->line, ast->col, "unexpected expression of type `" + leftExpr.utype->toString() + "` to be used with operand `" + operand.getValue() + "`");
            break;
        }
    }

    expr->type = utypeToExpressionType(expr->utype);
}

void Compiler::compileAssignExpression(Expression* expr, Ast* ast) {
    Expression leftExpr, rightExpr;
    Token &operand = ast->getToken(0);

    RETAIN_TYPE_INFERENCE(true)
    RETAIN_SCOPE_INIT_CHECK(operand == "=" ? false : true)
    compileExpressionAst(&leftExpr, ast->getSubAst(ast_primary_expr));
    RESTORE_SCOPE_INIT_CHECK()

    if(leftExpr.utype->getType() == utype_function_ptr
        || (leftExpr.utype->getType() == utype_field && ((Field*)leftExpr.utype->getResolvedType())->type == FNPTR)) {
        if(leftExpr.utype->getType() == utype_field) {
            RETAIN_REQUIRED_SIGNATURE((Method*)((Field*)leftExpr.utype->getResolvedType())->utype->getResolvedType())
            compileExpression(&rightExpr, ast->getSubAst(ast_expression));
            RESTORE_REQUIRED_SIGNATURE()
        } else {
            RETAIN_REQUIRED_SIGNATURE((Method*)leftExpr.utype->getResolvedType())
            compileExpression(&rightExpr, ast->getSubAst(ast_expression));
            RESTORE_REQUIRED_SIGNATURE()
        }
    } else
        compileExpression(&rightExpr, ast->getSubAst(ast_expression));
    RESTORE_TYPE_INFERENCE()

    expr->ast = ast;
    switch(leftExpr.utype->getType()) {
        case utype_class:
        case utype_native:
        case utype_field: {
            if(operand == "=")
                assignValue(expr, operand, leftExpr, rightExpr, ast);
            else compoundAssignValue(expr, operand, leftExpr, rightExpr, ast);
            break;
        }
        default: {
            errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + leftExpr.utype->toString() + "` must be a variable, class, or indexer");
            break;
        }
    }

}

void Compiler::compileVectorExpression(Expression* expr, Ast* ast, Utype *compareType) {
    List<Expression*> array;
    Ast* vectAst = ast->getSubAst(ast_vector_array);

    expr->ast = ast;
    compileExpressionList(array, vectAst);

    if(compareType && compareType->getType() != utype_class && compareType->getType() != utype_native && compareType->getType() != utype_function_ptr)
        errors->createNewError(GENERIC, ast->line, ast->col, "arrayType `" + compareType->toString() + "` must be a class, funcion pointer, or a native type");

    if(array.size() >= 1) {
        if(compareType == NULL) compareType = array.get(0)->utype;
        Utype *elementUtype;
        expr->type = utypeToExpressionType(compareType);
        expr->utype->copy(compareType);
        if(expr->utype->getType() == utype_literal) {
            if(((Literal*)expr->utype->getResolvedType())->literalType == string_literal) {
                expr->utype->setResolvedType(new DataEntity(OBJECT));
                expr->type = exp_object;
            }

            expr->utype->setType(utype_native);
        }

        for (long i = 0; i < array.size(); i++) {
            elementUtype = array.get(i)->utype;
            if (!isUtypeClassConvertableToVar(compareType, elementUtype) && !isUtypeConvertableToNativeClass(compareType, elementUtype)
                     && !compareType->isRelated(elementUtype)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "array element of type `" + elementUtype->toString() + "` is not equal to type `" + compareType->toString() + "`");
            }
        }

        if(expr->type != exp_object) {
            for (int i = 0; i < array.size(); i++) {
                if (array.get(0)->utype->isArray()) {
                    errors->createNewError(GENERIC, ast->line, ast->col, "array arrays are not supported");
                }
            }
        }
    } else {
        expr->type = utypeToExpressionType(compareType);
        expr->utype->copy(compareType);
    }

    if(array.size() >= 1) {
        expr->utype->getCode()
                .addIr(OpBuilder::movi(array.size(), EBX));

        if(expr->type == exp_var)
            expr->utype->getCode().addIr(OpBuilder::newVarArray(EBX));
        else if(expr->type == exp_class)
            expr->utype->getCode().addIr(OpBuilder::newClassArray(EBX, expr->utype->getClass()->address));
        else if(expr->type == exp_object)
            expr->utype->getCode().addIr(OpBuilder::newObjectArray(EBX));

        for(long i = 0; i < array.size(); i++) {
            if(expr->type == exp_var) {
                if(isUtypeClassConvertableToVar(array.get(i)->utype, expr->utype)) {
                    convertNativeIntegerClassToVar(array.get(i)->utype, expr->utype, expr->utype->getCode(), ast);
                } else {
                    expr->utype->getCode().inject(array.get(i)->utype->getCode());
                    expr->utype->getCode().inject(array.get(i)->utype->getCode().getInjector(ebxInjector));
                }

                expr->utype->getCode()
                        .addIr(OpBuilder::movsl(0)) // get our array object
                        .addIr(OpBuilder::movi(i, ADX)) // set element index
                        .addIr(OpBuilder::rmov(ADX, EBX)); // ourArry[%adx] = %ebx
            }
            else if(expr->type == exp_class || expr->type == exp_object) {
                if(isUtypeConvertableToNativeClass(expr->utype, array.get(i)->utype)) {
                    List<Expression*> exprs;
                    List<Field*> params;
                    exprs.add(array.get(i));

                    expressionsToParams(exprs, params);
                    convertUtypeToNativeClass(expr->utype, params.get(0)->utype, expr->utype->getCode(), ast);
                } else {
                    expr->utype->getCode().inject(array.get(i)->utype->getCode());
                    expr->utype->getCode().inject(array.get(i)->utype->getCode().getInjector(stackInjector));
                }

                expr->utype->getCode()
                        .addIr(OpBuilder::movsl(-1)) // get our array object
                        .addIr(OpBuilder::movn(i)) // select array element
                        .addIr(OpBuilder::popObject()); // set object
            }
        }

        expr->utype->getCode().getInjector(removeFromStackInjector)
                .addIr(OpBuilder::pop());
        expr->utype->getCode().getInjector(ptrInjector)
                .addIr(OpBuilder::popObject2());
    } else {
        expr->utype->getCode()
                .addIr(OpBuilder::pushNull());

        expr->utype->getCode().getInjector(removeFromStackInjector)
                .addIr(OpBuilder::pop());
        expr->utype->getCode().getInjector(ptrInjector)
                .addIr(OpBuilder::popObject2());

    }

    expr->utype->setArrayType(true);
}

void Compiler::compileNewArrayExpression(Expression *expr, Ast *ast, Utype *arrayType) {
    expr->ast = ast;
    expr->utype->copy(arrayType);
    expr->utype->setArrayType(true);
    expr->type = utypeToExpressionType(arrayType);

    if(arrayType->getType() != utype_class && arrayType->getType() != utype_native)
        errors->createNewError(GENERIC, ast->line, ast->col, "arrayType `" + arrayType->toString() + "` must be a class or a native type");

    Expression index;
    compileExpression(&index, ast->getSubAst(ast_expression));

    expr->utype->getCode().inject(index.utype->getCode());
    expr->utype->getCode().inject(index.utype->getCode().getInjector(ebxInjector));

    if(expr->type == exp_var)
        expr->utype->getCode().addIr(OpBuilder::newVarArray(EBX));
    else if(expr->type == exp_class)
        expr->utype->getCode().addIr(OpBuilder::newClassArray(EBX, expr->utype->getClass()->address));
    else if(expr->type == exp_object)
        expr->utype->getCode().addIr(OpBuilder::newObjectArray(EBX));

    expr->utype->getCode().getInjector(ptrInjector)
            .addIr(OpBuilder::popObject2());

    expr->utype->getCode().getInjector(removeFromStackInjector)
            .addIr(OpBuilder::pop());
}

// used for assigning native vars with size constraints
CodeHolder& Compiler::dataTypeToOpcode(DataType type, _register outRegister, _register castRegister, CodeHolder &code) {
    switch (type) {
        case _INT8: return code.addIr(OpBuilder::mov8(outRegister, castRegister));
        case _INT16: return code.addIr(OpBuilder::mov16(outRegister, castRegister));
        case _INT32: return code.addIr(OpBuilder::mov32(outRegister, castRegister));
        case _INT64: return code.addIr(OpBuilder::mov64(outRegister, castRegister));
        case _UINT8: return code.addIr(OpBuilder::movu8(outRegister, castRegister));
        case _UINT16: return code.addIr(OpBuilder::movu16(outRegister, castRegister));
        case _UINT32: return code.addIr(OpBuilder::movu32(outRegister, castRegister));
        case _UINT64: return code.addIr(OpBuilder::movu64(outRegister, castRegister));
        default: return code.addIr(OpBuilder::ill());
    }
}

bool isFieldPair(KeyPair<Field*, bool> *pair, void *field) {
    return field == pair->key;
}

void Compiler::compileFieldInitialization(Expression* expr, List<KeyPair<Field*, bool>> &fields, Ast* ast) {
    if(ast->getType() == ast_expression_list && fields.size() < ast->getSubAstCount()) {
        stringstream ss;
        ss << "class `" << expr->utype->toString() << "` only contains a total of " << expr->utype->getClass()->totalFieldCount()
            << " fields but was supplied a total of `" << ast->getSubAstCount() << "` expressions";
        errors->createNewError(GENERIC, ast->line, ast->col, ss.str());
        return;
    }

    for(long i = 0; i < ast->getSubAstCount(); i++) {
        Ast *field_init = ast->getSubAst(i);
        ClassObject *scopedClass = NULL;

        if(field_init->hasToken("base")) {
            scopedClass = getBaseClassUtype(field_init);
        } else
            scopedClass = currentScope()->klass;



        Expression assignExpression;
        compileExpression(&assignExpression, ast->getType() == ast_expression_list ? field_init : field_init->getSubAst(ast_expression));

        if(ast->getType() == ast_field_init_list) {
            RETAIN_SCOPE_CLASS(scopedClass)
            RETAIN_BLOCK_TYPE(INSTANCE_BLOCK)

            Utype *utype = compileUtype(field_init->getSubAst(ast_utype));

            RESTORE_BLOCK_TYPE()
            RESTORE_SCOPE_CLASS()

            if (utype->getType() == utype_field) {
                Field *field = (Field *) utype->getResolvedType();

                if (expr->utype->getClass()->isClassRelated(field->owner, false)) {
                    fields.get(fields.indexof(&isFieldPair, (void *) field)).value = true;
                    if(field->flags.find(STATIC))
                        createNewWarning(GENERIC, __WGENERAL, ast->line, ast->col, "field `" + field->fullName + "` initialized  with class appears to be static");
                    if(isUtypeConvertableToNativeClass(field->utype, assignExpression.utype)) {
                        convertUtypeToNativeClass(field->utype, assignExpression.utype, expr->utype->getCode(), ast);

                        assignExpression.type = utypeToExpressionType(field->utype);
                        assignExpression.utype->copy(field->utype);
                        assignExpression.utype->getCode().free();

                        assignFieldInitExpressionValue(field, &assignExpression, &expr->utype->getCode(), ast);
                    } else
                        assignFieldInitExpressionValue(field, &assignExpression, &expr->utype->getCode(), ast);
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col, "owner of field `" + field->toString()
                                                                         + "` is not found in class `" +
                                                                         expr->utype->getClass()->fullName +
                                                                         "` or derived super classes.");
                }
            } else
                errors->createNewError(GENERIC, ast->line, ast->col, "initializer of type `" + utype->toString()
                                                                     + "` must be a field from derrived class `" +
                                                                     expr->utype->getClass()->fullName + "`");
        } else {
            fields.get(i).value = true;
            Field* field = fields.get(i).key;
            if(!field->flags.find(STATIC)) {
                if(isUtypeConvertableToNativeClass(field->utype, assignExpression.utype)) {
                    convertUtypeToNativeClass(field->utype, assignExpression.utype, expr->utype->getCode(), ast);

                    assignExpression.type = utypeToExpressionType(field->utype);
                    assignExpression.utype->copy(field->utype);
                    assignExpression.utype->getCode().free();

                    assignFieldInitExpressionValue(field, &assignExpression, &expr->utype->getCode(), ast);
                } else
                    assignFieldInitExpressionValue(field, &assignExpression, &expr->utype->getCode(), ast);
            }
        }
    }
}

void Compiler::assignFieldInitExpressionValue(Field *field, Expression *assignExpr, CodeHolder *resultCode, Ast *ast) {
    if((field->utype->getClass() && assignExpr->utype->getClass() && assignExpr->utype->getClass()->isClassRelated(field->utype->getClass())
           && field->utype->isArray() == assignExpr->utype->isArray())
        || (field->utype->getClass() && assignExpr->utype->isNullType())
        || (field->utype->getClass() == NULL && assignExpr->utype->isRelated(field->utype))) {
        resultCode->inject(assignExpr->utype->getCode()); // TODO:...

        if(field->type >= _INT8 && field->type <= _UINT64) {
            if(field->isArray)
                goto object_assignment;
            resultCode->inject(assignExpr->utype->getCode().getInjector(ebxInjector));

            dataTypeToOpcode(field->type, EBX, EBX, *resultCode)
                    .addIr(OpBuilder::movsl(0))
                            //access the field
                    .addIr(OpBuilder::movn(field->address))
                            // set the ebx value to the field
                    .addIr(OpBuilder::movi(0, ADX))
                    .addIr(OpBuilder::checklen(ADX))
                    .addIr(OpBuilder::rmov(ADX, EBX));
        } else if(field->type == VAR || field->type == FNPTR) {
            if(field->isArray)
                goto object_assignment;

            resultCode->inject(assignExpr->utype->getCode().getInjector(ebxInjector));
            (*resultCode)
                    // get the created object
                    .addIr(OpBuilder::movsl(0))
                            //access the field
                    .addIr(OpBuilder::movn(field->address))
                            // set the ebx vlue to the field
                    .addIr(OpBuilder::movi(0, ADX))
                    .addIr(OpBuilder::checklen(ADX))
                    .addIr(OpBuilder::rmov(ADX, EBX));
        } else if(field->type == CLASS || field->type == OBJECT) {
            object_assignment:
            resultCode->inject(assignExpr->utype->getCode().getInjector(stackInjector));
            (*resultCode)
                    // get the created object
                    .addIr(OpBuilder::movsl(-1))
                            //access the field
                    .addIr(OpBuilder::movn(field->address))
                            // get value from stack & assign to field
                    .addIr(OpBuilder::popObject());
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, " expression `" + assignExpr->utype->toString() + "` assigned to field `" + field->toString()
                                                                 + "` does not allow values to be assigned to it.");
        }
    } else {
        errors->createNewError(GENERIC, ast->line, ast->col, " expression `" + assignExpr->utype->toString() + "` assigned to field `" + field->toString()
                                                             + "` does not match the type of the field.");
    }
}

void Compiler::compileNewExpression(Expression* expr, Ast* ast) {

    RETAIN_TYPE_INFERENCE(false)
    expr->ast = ast;
    Utype *arrayType =compileUtype(ast->getSubAst(ast_utype));
    if(ast->hasSubAst(ast_vector_array)) {
        compileVectorExpression(expr, ast, arrayType);
    } else if(ast->hasSubAst(ast_array_expression)) {
        compileNewArrayExpression(expr, ast->getSubAst(ast_array_expression), arrayType);
    } else if(ast->hasSubAst(ast_expression_list) && ast->getSubAst(ast_expression_list)->hasToken(LEFTPAREN)) {
        if(arrayType->getClass() && arrayType->getType() != utype_field) {
            if(arrayType->getClass()->flags.find(EXTENSION)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "cannot instantiate extension class `" + arrayType->toString() + "`");
            }

            List<Expression*> expressions;
            List<Field*> params;
            Method *constr;
            compileExpressionList(expressions, ast->getSubAst(ast_expression_list));
            expressionsToParams(expressions, params);

            expr->ast = ast;
            expr->utype->copy(arrayType);
            expr->utype->setArrayType(false);
            expr->type = utypeToExpressionType(arrayType);
            string functionName = arrayType->getClass()->name;
            if(IS_CLASS_GENERIC(arrayType->getClass()->getClassType())) {
                functionName = arrayType->getClass()->getGenericOwner()->name
                        .substr(0, arrayType->getClass()->getGenericOwner()->name.size() - 2);
            }

            if((constr = findFunction(arrayType->getClass(), functionName, params, ast, false, fn_constructor, true)) == NULL) {
                errors->createNewError(GENERIC, ast->line, ast->col, "class `" + arrayType->toString() + "` does not contain constructor `"
                                                                     + arrayType->getClass()->name + Method::paramsToString(params) + "`");
            }

            if(constr != NULL) {
                validateAccess(constr, ast);
                expr->utype->getCode().addIr(OpBuilder::newClass(expr->utype->getClass()->address))
                        .addIr(OpBuilder::dup());

                for(long i = 0; i < params.size(); i++) {
                    Field *param = constr->params.get(i);
                    if(isUtypeConvertableToNativeClass(param->utype, params.get(i)->utype)) {
                        convertUtypeToNativeClass(param->utype, params.get(i)->utype, expr->utype->getCode(), ast);
                    } else {
                        params.get(i)->utype->getCode().inject(stackInjector);
                        expr->utype->getCode().inject(params.get(i)->utype->getCode());
                    }
                }

                expr->utype->getCode().addIr(OpBuilder::call(constr->address));

                expr->utype->getCode().freeInjectors();
                expr->utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::movsl(0));
                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
            }

            expressions.free();
            freeListPtr(params);
        } else if(arrayType->getType() == utype_field)
            errors->createNewError(GENERIC, ast->line, ast->col, "field `" + arrayType->toString() + "` cannot be used for instantiation.");
        else if(arrayType->getType() != utype_unresolved)
            errors->createNewError(GENERIC, ast->line, ast->col, "symbol `" + arrayType->toString() + "` must be of type `class` to be used for instantiation.");
    } else if(ast->hasSubAst(ast_field_init_list) || ast->hasSubAst(ast_expression_list)) {
        if(arrayType->getClass() && arrayType->getType() != utype_field) {
            if(arrayType->getClass()->flags.find(EXTENSION)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "cannot instantiate extension class `" + arrayType->toString() + "`");
            }

            List<KeyPair<Field*, bool>> fields;
            ClassObject *klass = arrayType->getClass();

            expr->ast = ast;
            expr->utype->copy(arrayType);
            expr->utype->setArrayType(false);
            expr->type = utypeToExpressionType(arrayType);

            addFields:
            for(long i = 0; i < klass->fieldCount(); i++) {
                fields.add(KeyPair<Field*, bool>(klass->getField(i), false));
                compileFieldType(klass->getField(i));
            }

            if(klass->getSuperClass() != NULL) {
                klass = klass->getSuperClass();
                goto addFields;
            }

            expr->utype->getCode().addIr(OpBuilder::newClass(expr->utype->getClass()->address));
            compileFieldInitialization(expr, fields,
                                       ast->getSubAst(ast_expression_list) == NULL ? ast->getSubAst(ast_field_init_list) : ast->getSubAst(ast_expression_list));

            // for the rest of the fields we only do the main class i n question for automatic initialization
            for(long i = 0; i < fields.size(); i++) {
                if(!fields.get(i).value) {
                    fields.get(i).value = true;
                    Field *field = fields.get(i).key;
                    if(field->ast->hasSubAst(ast_expression) && !field->flags.find(STATIC) && field->owner->guid == arrayType->getClass()->guid) {
                        if(field->ast->getSubAst(ast_expression)->hasSubAst(ast_primary_expr)
                           && field->ast->getSubAst(ast_expression)->getSubAst(ast_primary_expr)->hasSubAst(ast_new_e)
                           && field->ast->getSubAst(ast_expression)->getSubAst(ast_primary_expr)->getSubAst(ast_new_e) == ast) {
                            errors->createNewError(GENERIC, ast->line, ast->col, "cyclic expression found while attempting to compile new expression. Expression code being assigned to field `"
                                                                                 + field->toString() + "` will cause stack overflow error.\n\nTo fix this, I suggest you remove the default value of the field or assign the field directly in the new expression like so `new " + arrayType->getResolvedType()->name + " { ..., " + field->name + " = <your-expr> }`.");
                        } else {
                            Expression assignExpression;
                            compileExpression(&assignExpression, field->ast->getSubAst(ast_expression));

                            if (isUtypeConvertableToNativeClass(field->utype, assignExpression.utype)) {
                                convertUtypeToNativeClass(field->utype, assignExpression.utype,
                                                          expr->utype->getCode(), ast);

                                assignExpression.type = utypeToExpressionType(field->utype);
                                assignExpression.utype->copy(field->utype);
                                assignExpression.utype->getCode().free();

                                assignFieldInitExpressionValue(field, &assignExpression, &expr->utype->getCode(),
                                                               ast);
                            } else
                                assignFieldInitExpressionValue(field, &assignExpression, &expr->utype->getCode(),
                                                               ast);
                        }
                    }
                }
            }

            if(ast->getSubAstCount() > 2)
                compilePostAstExpressions(expr, ast, 2);
            else {
                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
                expr->utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::popObject2());
            }
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "arrayType `" + arrayType->toString() + "` must be a class");
    }

    compilePostAstExpressions(expr, ast, 2);
    RESTORE_TYPE_INFERENCE()
}

void Compiler::compileNullExpression(Expression* expr, Ast* ast) {
    expr->utype->copy(nullUtype);
    expr->type = utypeToExpressionType(nullUtype);

    expr->utype->getCode()
            .addIr(OpBuilder::pushNull());

    expr->utype->getCode().getInjector(ptrInjector)
            .addIr(OpBuilder::popObject2());
    expr->utype->getCode().getInjector(removeFromStackInjector)
            .addIr(OpBuilder::pop());
}

void Compiler::compileBaseExpression(Expression* expr, Ast* ast) {
    RETAIN_BLOCK_TYPE(RESTRICTED_INSTANCE_BLOCK)
    RETAIN_SCOPE_CLASS(getBaseClassUtype(ast))
    expr->ast = ast;

    if(currentScope()->klass != NULL) {
        if (ast->hasToken(PTR)) {
            compileDotNotationCall(expr, ast->getSubAst(ast_dotnotation_call_expr));
        } else {
            expr->type = exp_class;
            expr->utype = new Utype(currentScope()->klass);
            expr->utype->getCode()
                    .addIr(OpBuilder::movl(0));

            expr->utype->getCode().getInjector(stackInjector)
                    .addIr(OpBuilder::pushObject());
        }
    }

    if(currentScope()->type == GLOBAL_SCOPE || currentScope()->type == STATIC_BLOCK)
        errors->createNewError(GENERIC, ast->line, ast->col, "illegal reference to self in static context");
    RESTORE_SCOPE_CLASS()
    RESTORE_BLOCK_TYPE()
}

ClassObject *Compiler::getBaseClassUtype(Ast *ast) {
    ClassObject *scopedBaseClass = NULL;
    if(ast->hasSubAst(ast_base_utype)) {
        RETAIN_BLOCK_TYPE(CLASS_SCOPE)
        RETAIN_TYPE_INFERENCE(false)
        Utype *utype = compileUtype(ast->getSubAst(ast_base_utype)->getSubAst(ast_utype));
        RESTORE_BLOCK_TYPE()
        RESTORE_TYPE_INFERENCE()

        if(utype->getType() == utype_class) {
            scopedBaseClass = utype->getClass();
            if(!currentScope()->klass->isClassRelated(scopedBaseClass)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "class `" + currentScope()->klass->fullName + "` does not inherit base class `" + utype->toString() + "`");
            }
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "symbol `" + utype->toString() + "` does not represent a class");
        freePtr(utype);
    } else {
        scopedBaseClass = currentScope()->klass->getSuperClass();

        if(scopedBaseClass == NULL)
            errors->createNewError(GENERIC, ast->line, ast->col, "class `" + currentScope()->klass->fullName + "` does not contain a base class");
    }

    return scopedBaseClass;
}

void Compiler::compileSelfExpression(Expression* expr, Ast* ast) {

    expr->ast = ast;
    if(ast->hasToken(PTR)) {
        RETAIN_BLOCK_TYPE(RESTRICTED_INSTANCE_BLOCK)
        compileDotNotationCall(expr, ast->getSubAst(ast_dotnotation_call_expr));
        RESTORE_BLOCK_TYPE()
    } else {
        expr->type = exp_class;
        expr->utype = new Utype(currentScope()->klass);
        expr->utype->getCode()
            .addIr(OpBuilder::movl(0));
        expr->utype->getCode().getInjector(stackInjector)
                .addIr(OpBuilder::pushObject());
    }

    if(currentScope()->type == GLOBAL_SCOPE || currentScope()->type == STATIC_BLOCK)
        errors->createNewError(GENERIC, ast->line, ast->col, "illegal reference to self in static context");
}

void Compiler::compileDotNotationCall(Expression* expr, Ast* ast) {

    expr->ast = ast;
    expr->utype->getCode().freeInjectors();
    if(ast->hasSubAst(ast_dot_fn_e)) {
        Ast* branch = ast->getSubAst(ast_dot_fn_e);
        RETAIN_REQUIRED_SIGNATURE(NULL)
        Method* method = compileMethodUtype(expr, branch);
        RESTORE_REQUIRED_SIGNATURE()

        if(method) {
            expr->type = utypeToExpressionType(method->utype);
            expr->utype->setResolvedType(method->utype->getResolvedType());
            expr->utype->setType(method->utype->getType());
            expr->utype->setArrayType(method->utype->isArray());

            expr->utype->getCode().freeInjectors();
            if (method->utype->isArray() || method->utype->getResolvedType()->type == OBJECT
                || method->utype->getResolvedType()->type == CLASS) {
                expr->utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::popObject2());

                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
            } else if (method->utype->getResolvedType()->isVar()) {
                expr->utype->getCode().getInjector(ebxInjector)
                        .addIr(OpBuilder::loadValue(EBX));

                expr->utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());
            }

        }
    } else {
        Utype *utype = compileUtype(ast->getSubAst(ast_utype), expr->utype->getCode().instanceCaptured);
        expr->utype->copy(utype);
        expr->utype->getCode().copyInjectors(utype->getCode());
        expr->utype->getCode().inject(utype->getCode());

        expr->type = utypeToExpressionType(expr->utype);
    }

    compilePostAstExpressions(expr, ast);
}

/*
 * Proccess elements after an expression in the format of `[]`, `.<identifier>`, or `++`
 */
void Compiler::compilePostAstExpressions(Expression *expr, Ast *ast, long startPos) {
    expr->utype->getCode().instanceCaptured = true;
    if(ast->getSubAstCount() > 1) {
        for(long i = startPos; i < ast->getSubAstCount(); i++) {
            if(ast->getSubAst(i)->getType() == ast_post_inc_e)
                compilePostIncExpression(expr, false, ast->getSubAst(i));
            else if(ast->getSubAst(i)->getType() == ast_cast_e)
                compileCastExpression(expr, false, ast->getSubAst(i));
            else if(ast->getSubAst(i)->getType() == ast_dot_not_e || ast->getSubAst(i)->getType() == ast_dotnotation_call_expr) {
                if(expr->utype->getClass()) {

                    RETAIN_SCOPE_CLASS(expr->utype->getClass() != NULL ? expr->utype->getClass() : currentScope()->klass)
                    RETAIN_BLOCK_TYPE(RESTRICTED_INSTANCE_BLOCK)
                    Ast *astToCompile = ast->getSubAst(i)->getType() == ast_dot_not_e ? ast->getSubAst(i)->getSubAst(0)
                                                                                      : ast->getSubAst(i);
                    Expression bridgeExpr;
                    bridgeExpr.utype->getCode().instanceCaptured = true;

                    if(astToCompile->hasSubAst(ast_dot_fn_e))
                        bridgeExpr.utype->getCode().inject(expr->utype->getCode().getInjector(stackInjector));
                    else
                        bridgeExpr.utype->getCode().inject(expr->utype->getCode().getInjector(ptrInjector));
                    compileDotNotationCall(&bridgeExpr, astToCompile);

                    expr->copy(&bridgeExpr);
                    expr->utype->getCode().inject(bridgeExpr.utype->getCode());
                    expr->utype->getCode().copyInjectors(bridgeExpr.utype->getCode());
                    RESTORE_BLOCK_TYPE()
                    RESTORE_SCOPE_CLASS()
                } else
                    errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "expression of type `" + expr->utype->toString()
                                                                                                     + "` must resolve as a class");
            } else if(ast->getSubAst(i)->getType() == ast_dot_fn_e)  {
                if(expr->utype->getMethod() != NULL) {
                    Method *resolvedMethod = expr->utype->getMethod();
                    Ast *branch = ast->getSubAst(i);
                    List<Expression*> expressions;
                    List<Field*> params;

                    RETAIN_SCOPE_CLASS(expr->utype->getClass() != NULL ? expr->utype->getClass() : currentScope()->klass)
                    compileMethodReturnType(resolvedMethod, resolvedMethod->ast); // shouldn't need to but just in case
                    if(branch->getSubAst(ast_expression_list)->getSubAstCount() == resolvedMethod->params.size()) {
                        compileExpressionList(expressions, branch->getSubAst(ast_expression_list));
                        expressionsToParams(expressions, params);

                        if(simpleParameterMatch(resolvedMethod->params, params)
                            || complexParameterMatch(resolvedMethod->params, params)) {
                            CodeHolder &code = expr->utype->getCode();

                            validateAccess(resolvedMethod, ast);
                            code.inject(stackInjector);
                            pushParametersToStackAndCall(ast, resolvedMethod, params, code);
                            expr->type = utypeToExpressionType(resolvedMethod->utype);
                            expr->utype->setResolvedType(resolvedMethod->utype->getResolvedType());
                            expr->utype->setType(resolvedMethod->utype->getType());
                            expr->utype->setArrayType(resolvedMethod->utype->isArray());

                            code.freeInjectors();
                            if (resolvedMethod->utype->isArray() || resolvedMethod->utype->getResolvedType()->type == OBJECT
                                || resolvedMethod->utype->getResolvedType()->type == CLASS) {
                                code.getInjector(ptrInjector)
                                        .addIr(OpBuilder::popObject2());

                                code.getInjector(removeFromStackInjector)
                                        .addIr(OpBuilder::pop());
                            } else if (resolvedMethod->utype->getResolvedType()->isVar()) {
                                code.getInjector(ebxInjector)
                                        .addIr(OpBuilder::loadValue(EBX));

                                code.getInjector(removeFromStackInjector)
                                        .addIr(OpBuilder::pop());
                            }
                        } else
                            errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col,
                                                   "invalid arguments for expression of type `" + expr->utype->toString()
                                                   + "`");

                        expressions.free();
                        freeListPtr(params);
                        RESTORE_SCOPE_CLASS()
                    } else {
                        if(branch->getSubAst(ast_expression_list)->getSubAstCount() > resolvedMethod->params.size()) {
                            errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col,
                                                   "too many arguments for expression of type `" + expr->utype->toString()
                                                   + "`");
                        } else {
                            errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col,
                                                   "not enough arguments for expression of type `" + expr->utype->toString()
                                                   + "`");
                        }
                    }
                } else
                    errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "expression of type `" + expr->utype->toString()
                                                                                                     + "` must resolve as a function");
            } else if(ast->getSubAst(i)->getType() == ast_arry_e) {
                compileArrayExpression(expr, ast->getSubAst(i));
                continue;
            }
            else {
                errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "unexpected expression of type `" + Ast::astTypeToString(ast->getSubAst(i)->getType()) + "`");
            }
        }
    }
}

void Compiler::compileNativeCast(Utype *utype, Expression *castExpr, Expression *outExpr) {

    if(utype->getResolvedType() != NULL && castExpr->type != exp_undefined) {
        if(utype->getResolvedType()->isVar()) {
            if (castExpr->utype->getClass() && IS_CLASS_ENUM(castExpr->utype->getClass()->getClassType())) {
                if(!utype->isArray()) {
                    Field *valueField = castExpr->utype->getClass()->getField("ordinal", true);

                    if(valueField) {
                        compileFieldType(valueField);

                        outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
                        outExpr->utype->getCode()
                                .addIr(OpBuilder::movn(valueField->address))
                                .addIr(OpBuilder::movi(0, ADX))
                                .addIr(OpBuilder::iaload(EBX, ADX));

                        if (utype->getResolvedType()->type != VAR) {
                            dataTypeToOpcode(utype->getResolvedType()->type, EBX, EBX, outExpr->utype->getCode());
                        }

                        outExpr->utype->getCode().freeInjectors();
                        outExpr->utype->getCode().getInjector(stackInjector)
                                .addIr(OpBuilder::rstore(EBX));
                    } else goto castErr;
                } else goto castErr;
            } else if (isUtypeClass(castExpr->utype, Obfuscater::getModule("std"), 9, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong")) {
                if(!utype->isArray()) {
                    Field *valueField = castExpr->utype->getClass()->getField("value", true);

                    if(valueField) {
                        compileFieldType(valueField);

                        outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
                        outExpr->utype->getCode()
                                .addIr(OpBuilder::movn(valueField->address))
                                .addIr(OpBuilder::movi(0, ADX))
                                .addIr(OpBuilder::iaload(EBX, ADX));

                        if (utype->getResolvedType()->type != VAR) {
                            dataTypeToOpcode(utype->getResolvedType()->type, EBX, EBX, outExpr->utype->getCode());
                        }

                        outExpr->utype->getCode().freeInjectors();
                        outExpr->utype->getCode().getInjector(stackInjector)
                                .addIr(OpBuilder::rstore(EBX));
                    } else goto castErr;
                } else goto castErr;
            } else if (castExpr->utype->getResolvedType()->type == OBJECT) {
                if(utype->isArray() && !castExpr->utype->isArray()) {
                    outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
                    outExpr->utype->getCode()
                            .addIr(OpBuilder::varCast(utype->getResolvedType()->type, true));

                    outExpr->utype->getCode().freeInjectors();
                    outExpr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::pushObject());
                } else goto castErr;
            } else if (castExpr->type == exp_var) {
                if(utype->isArray() == castExpr->utype->isArray()) {
                    if (!utype->isArray()) {

                        outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ebxInjector));
                        if(utype->getResolvedType()->type == castExpr->utype->getResolvedType()->type) {
                            createNewWarning(GENERIC, __WDECL, outExpr->ast->line, outExpr->ast->col, "redundant cast of type `" + castExpr->utype->toString()
                                                                                                      + "` to `" + utype->toString() + "`");
                        } else {
                            if(utype->getResolvedType()->type != VAR) {
                                dataTypeToOpcode(utype->getResolvedType()->type, EBX, EBX, outExpr->utype->getCode());
                            }
                        }

                        outExpr->utype->getCode().freeInjectors();
                        outExpr->utype->getCode().getInjector(stackInjector)
                                .addIr(OpBuilder::rstore(EBX));
                    } else if (utype->equals(castExpr->utype)) {
                        createNewWarning(GENERIC, __WDECL, outExpr->ast->line, outExpr->ast->col, "redundant cast of type `" + castExpr->utype->toString()
                               + "` to `" + utype->toString() + "`");
                    } else goto castErr;
                } else goto castErr;
            } else goto castErr;
        } else if(utype->getResolvedType()->type == OBJECT) {
            if(!utype->isRelated(castExpr->utype))
                goto castErr;

            outExpr->utype->getCode().freeInjectors();
            outExpr->utype->getCode().copyInjectors(castExpr->utype->getCode());
        } else goto castErr; // should never happen but we need to make sure
    } else {
        castErr:
        errors->createNewError(GENERIC, outExpr->ast->line, outExpr->ast->col, "unable to cast incompatible type of `" + castExpr->utype->toString()
                                                                               + "` to type `" + utype->toString() + "`");
    }
}

void Compiler::compileFnPtrCast(Utype *utype, Expression *castExpr, Expression *outExpr) {
    if(utype->getResolvedType() != NULL && castExpr->type != exp_undefined) {
        if(utype->getType() == utype_function_ptr) {
            if(castExpr->utype->getType() == utype_method)
                compileMethodReturnType((Method*)castExpr->utype->getResolvedType(), castExpr->utype->getResolvedType()->ast);

            if(utype->equals(castExpr->utype)) {
                if(!utype->isArray()) {
                    outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ebxInjector));

                    outExpr->utype->getCode().freeInjectors();
                    outExpr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(EBX));
                } else {
                    outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));

                    outExpr->utype->getCode().freeInjectors();
                    outExpr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::pushObject());
                }
            } else goto castErr;
        } else goto castErr; // should never happen but we need to make sure
    } else {
        castErr:
        errors->createNewError(GENERIC, outExpr->ast->line, outExpr->ast->col, "unable to cast incompatible type of `" + castExpr->utype->toString()
                                                                               + "` to type `" + utype->toString() + "`");
    }
}

void Compiler::compileClassCast(Utype *utype, Expression *castExpr, Expression *outExpr) {
    if(castExpr->utype->getClass() != NULL) {
        if(utype->isArray() == castExpr->utype->isArray()) {
            if(castExpr->utype->isRelated(utype)) {
                if(castExpr->utype->getClass()->match(utype->getClass())) {
                    createNewWarning(GENERIC, __WDECL, outExpr->ast->line, outExpr->ast->col, "redundant cast of type `" + castExpr->utype->toString()
                               + "` to `" + utype->toString() + "`");
                }

                outExpr->utype->getCode().freeInjectors();
                outExpr->utype->getCode().copyInjectors(castExpr->utype->getCode());
                // we only assume its good if were casting downstream
                // any other casts need to be check explicitly by the VM at runtime
                // I.E. (_object_)new int(100)
                // we know that the int class obviously will have a superclass of type `_object_` so there's no need to do this check at runtime
                return;
            } else {
                // anything else MUST be checked at runtime!
                outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
                outExpr->utype->getCode()
                        .addIr(OpBuilder::movi(utype->getClass()->address, CMT))
                        .addIr(OpBuilder::cast(CMT));

                outExpr->utype->getCode().freeInjectors();
                outExpr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::pushObject());
            }
        } else {
            goto castErr;
        }
    } else if(castExpr->utype->getResolvedType()->type == OBJECT) {
        outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
        outExpr->utype->getCode()
                .addIr(OpBuilder::movi(utype->getClass()->address, CMT))
                .addIr(OpBuilder::cast(CMT));

        outExpr->utype->getCode().freeInjectors();
        outExpr->utype->getCode().getInjector(stackInjector)
                .addIr(OpBuilder::pushObject());
    } else if(isUtypeConvertableToNativeClass(utype, castExpr->utype)) {
        outExpr->utype->getCode().free();

        List<Expression*> exprs;
        List<Field*> params;
        exprs.add(castExpr);

        expressionsToParams(exprs, params);
        convertUtypeToNativeClass(utype, params.get(0)->utype, outExpr->utype->getCode(), castExpr->ast);
    }
    else {
        castErr:
        errors->createNewError(GENERIC, outExpr->ast->line, outExpr->ast->col, "unable to cast incompatible type of `" + castExpr->utype->toString()
                   + "` to type `" + utype->toString() + "`");
    }
}

void Compiler::compileCastExpression(Expression *expr, bool compileExpr, Ast *ast) {
    RETAIN_TYPE_INFERENCE(false)
    RETAIN_BLOCK_TYPE(CLASS_SCOPE)
    Utype *utype = compileUtype(ast->getSubAst(ast_utype));
    RESTORE_TYPE_INFERENCE()
    RESTORE_BLOCK_TYPE()

    Expression castExpr;
    if(compileExpr)
        compileExpression(&castExpr, ast->getSubAst(ast_expression));
    else {
        castExpr.ast = ast;
        castExpr.utype->copy(expr->utype);
        castExpr.type = utypeToExpressionType(expr->utype);

        // we need to transfer the left side of the expression to the castExpression
        castExpr.utype->getCode().copyInjectors(expr->utype->getCode());
        castExpr.utype->getCode().inject(expr->utype->getCode());
        expr->utype->getCode().free();
    }

    expr->ast = ast;
    expr->utype->copy(utype);
    expr->type = utypeToExpressionType(utype);

    expr->utype->getCode().inject(castExpr.utype->getCode());
    if(expr->type != exp_undefined && castExpr.utype->getType() != utype_unresolved) {
        if(utype->getType() == utype_class)
            compileClassCast(utype, &castExpr, expr);
        else if(utype->getType() == utype_native) {
            compileNativeCast(utype, &castExpr, expr);
        } else if(utype->getType() == utype_function_ptr) {
            compileFnPtrCast(utype, &castExpr, expr);
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "cast expression of type `" + utype->toString() + "` not allowed, must be of type `class` or a native type");
        }
    }
}

void Compiler::compileArrayExpression(Expression* expr, Ast* ast) {
    Expression arrayExpr;
    if(expr->utype->isArray()) {
        compileExpression(&arrayExpr, ast->getSubAst(ast_expression));

        expr->utype->getCode().inject(expr->utype->getCode().getInjector(stackInjector));
        expr->utype->getCode().inject(arrayExpr.utype->getCode());
        expr->utype->getCode().inject(arrayExpr.utype->getCode().getInjector(ebxInjector));

        if(arrayExpr.type == exp_var) {
            if(expr->utype->getType() == utype_field) {
                    expr->utype->copy(((Field*)expr->utype->getResolvedType())->utype);
            }

            expr->utype->setArrayType(false);
            expr->utype->getCode()
                    .addIr(OpBuilder::popObject2())
                    .addIr(OpBuilder::movr(ADX, EBX))
                    .addIr(OpBuilder::checklen(ADX));

            if(!expr->utype->getResolvedType()->isVar())
                expr->utype->getCode()
                    .addIr(OpBuilder::movnd(ADX));

            expr->utype->getCode().freeInjectors();
            if(expr->utype->getResolvedType()->isVar()) {
                expr->utype->getCode().getInjector(ebxInjector)
                        .addIr(OpBuilder::iaload(EBX, ADX));

                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::iaload(EBX, ADX))
                        .addIr(OpBuilder::rstore(EBX));

                expr->utype->getCode().getInjector(incInjector).addIr(OpBuilder::inc(EBX));
                expr->utype->getCode().getInjector(decInjector).addIr(OpBuilder::dec(EBX));

                if (expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, expr->utype->getCode().getInjector(incInjector));
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, expr->utype->getCode().getInjector(decInjector));
                }

                expr->utype->getCode().getInjector(incInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::rmov(ADX, EBX));
                expr->utype->getCode().getInjector(decInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::rmov(ADX, EBX));

                expr->utype->getCode().getInjector("={injector}")
                        .addIr(OpBuilder::loadValue(EBX))
                        .addIr(OpBuilder::rmov(ADX, EBX));


                CodeHolder *injector = &expr->utype->getCode().getInjector("+={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::add(EBX, EBX, ECX));
                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, *injector);
                    injector->addIr(OpBuilder::rmov(ADX, EBX));
                } else
                    injector->addIr(OpBuilder::rmov(ADX, EBX));

                injector = &expr->utype->getCode().getInjector("-={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::sub(EBX, EBX, ECX));

                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, *injector);
                    injector->addIr(OpBuilder::rmov(ADX, EBX));
                } else
                    injector->addIr(OpBuilder::rmov(ADX, EBX));

                injector = &expr->utype->getCode().getInjector("*={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::mul(EBX, EBX, ECX));

                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, *injector);
                    injector->addIr(OpBuilder::rmov(ADX, EBX));
                } else
                    injector->addIr(OpBuilder::rmov(ADX, EBX));

                injector = &expr->utype->getCode().getInjector("/={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::div(EBX, EBX, ECX));

                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, *injector);
                    injector->addIr(OpBuilder::rmov(ADX, EBX));
                } else
                    injector->addIr(OpBuilder::rmov(ADX, EBX));

                injector = &expr->utype->getCode().getInjector("%={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::mod(EBX, EBX, ECX));

                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, *injector);
                    injector->addIr(OpBuilder::rmov(ADX, EBX));
                } else
                    injector->addIr(OpBuilder::rmov(ADX, EBX));

                injector = &expr->utype->getCode().getInjector("&={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::_and(EBX, ECX));
                injector->addIr(OpBuilder::rmov(ADX, CMT));

                injector = &expr->utype->getCode().getInjector("|={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::_or(EBX, ECX));

                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, CMT, CMT, *injector);
                    injector->addIr(OpBuilder::rmov(ADX, CMT));
                } else
                    injector->addIr(OpBuilder::rmov(ADX, CMT));

                injector = &expr->utype->getCode().getInjector("^={injector}");
                injector->addIr(OpBuilder::loadValue(ECX))
                        .addIr(OpBuilder::_xor(EBX, ECX));

                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, CMT, CMT, *injector);
                    injector->addIr(OpBuilder::rmov(ADX, CMT));
                } else
                    injector->addIr(OpBuilder::rmov(ADX, CMT));

            } else if(expr->utype->getResolvedType()->type == OBJECT || expr->utype->getResolvedType()->type == CLASS) {
                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::pushObject());

                expr->utype->getCode().getInjector("={injector}")
                        .addIr(OpBuilder::popObject());
            } else
                errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + expr->utype->toString() + "` must be a var[], object[], or class[]");
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "index expression of type `" + arrayExpr.utype->toString() + "` was not found to be a var");
    } else {
        if(expr->utype->getClass()) {
            Method *overload;
            List<Field*> params; // def operator[](<Any>) { ... }
            List<Expression*> argumentExpr;
            argumentExpr.add(&arrayExpr);

            compileExpression(&arrayExpr, ast->getSubAst(ast_expression));
            expressionsToParams(argumentExpr, params);

            if((overload = findFunction(expr->utype->getClass(), "operator[",
                                        params, ast, true, fn_op_overload)) != NULL) {
                compileMethodReturnType(overload, overload->ast, false);

                validateAccess(overload, ast);
                expr->utype->copy(overload->utype);

                expr->utype->getCode().inject(stackInjector);
                pushParametersToStackAndCall(ast, overload, params, expr->utype->getCode());

                expr->utype->getCode().freeInjectors();
                if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                    || expr->utype->getResolvedType()->type == CLASS) {
                    expr->utype->getCode().getInjector(ptrInjector)
                            .addIr(OpBuilder::popObject2());
                    expr->utype->getCode().getInjector(removeFromStackInjector)
                            .addIr(OpBuilder::pop());
                } else if (expr->utype->getResolvedType()->isVar()) {
                    expr->utype->getCode().getInjector(ebxInjector)
                            .addIr(OpBuilder::loadValue(EBX));
                    expr->utype->getCode().getInjector(removeFromStackInjector)
                            .addIr(OpBuilder::pop());
                }
            } else {
                errors->createNewError(GENERIC, ast, "use  of operator `[]` does not have any qualified use with class `" + expr->utype->toString() + "`");
            }

            argumentExpr.free();
            freeListPtr(params);
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + expr->utype->toString() + "` must be an array type");
    }

    compilePostAstExpressions(expr, ast);
}


void Compiler::compileMinusExpression(Expression* expr, Ast* ast) {
    this->compileExpression(expr, ast->getSubAst(ast_expression));

    Token tok = ast->getToken(MINUS);
    if(expr->utype->isArray()) {
        errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expression of type `" + expr->utype->toString() + "` must return a var to use `" +
                                                                        tok.getValue() + "` operator");
    } else if(expr->utype->getType() != utype_unresolved){
        Method *overload;
        List<Field*> params; // def operator-() { ... }

        switch(expr->utype->getResolvedType()->type) {
            case NIL:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "cannot use `" + tok.getValue() + "` operator on expression that returns nil");
                break;
            case UNTYPED:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "the type of the expression that is returned from `" + expr->utype->toString() + "` is untyped. Mostly likely due to a compiler bug, and therefore cannot use operator `" + tok.getValue() + "`");
                break;
            case UNDEFINED:
                break;
            case OBJECT:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expressions of type object must be casted before using `!` operator, try `!((Type)<your-expression>)` instead");
                break;

            case _INT8:
            case _INT16:
            case _INT32:
            case _INT64:
            case _UINT8:
            case _UINT16:
            case _UINT32:
            case _UINT64:
            case VAR:
                if(expr->utype->getType() == utype_literal) {
                    expr->type = exp_var;
                    expr->utype->setType(utype_literal);
                    expr->utype->setArrayType(false);
                    expr->utype->getCode().free();
                    double value =((Literal*)expr->utype->getResolvedType())->numericData;
                    value = -value;

                    if(value > INT32_MAX || !isWholeNumber(value)) {
                        long constantAddress = constantMap.addIfIndex(value);

                        if(constantAddress >= CONSTANT_LIMIT) {
                            stringstream err;
                            err << "maximum constant limit of (" << CONSTANT_LIMIT << ") reached.";
                            errors->createNewError(INTERNAL_ERROR, ast, err.str());
                        }

                        expr->utype->getResolvedType()->address = constantAddress;
                        expr->utype->getCode().addIr(OpBuilder::ldc(EBX, constantAddress));
                    } else
                        expr->utype->getCode().addIr(OpBuilder::movi(value, EBX));

                    expr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(EBX));
                    expr->utype->setResolvedType(new Literal(value));
                } else {
                    expr->utype->copy(varUtype);
                    expr->utype->getCode().inject(expr->utype->getCode().getInjector(ebxInjector));
                    expr->utype->getCode().addIr(OpBuilder::neg(EBX, EBX));

                    expr->utype->getCode().freeInjectors();
                    expr->utype->getCode().getInjector(stackInjector)
                            .addIr(OpBuilder::rstore(EBX));
                }
                break;

            case CLASS:
                if((overload = findFunction(expr->utype->getClass(), "operator" + tok.getValue(),
                                            params, ast, true, fn_op_overload)) != NULL) {
                    compileMethodReturnType(overload, overload->ast, false);

                    validateAccess(overload, ast);
                    expr->utype->copy(overload->utype);

                    expr->utype->getCode().inject(stackInjector);
                    expr->utype->getCode().addIr(OpBuilder::call(overload->address));

                    expr->utype->getCode().freeInjectors();
                    if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                        || expr->utype->getResolvedType()->type == CLASS) {
                        expr->utype->getCode().getInjector(ptrInjector)
                                .addIr(OpBuilder::popObject2());
                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    } else if (expr->utype->getResolvedType()->isVar()) {
                        expr->utype->getCode().getInjector(ebxInjector)
                                .addIr(OpBuilder::loadValue(EBX));
                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    }
                } else {
                    errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "use  of operator `" + tok.getValue() + "` does not have any qualified use with class `" + expr->utype->toString() + "`");
                }

                break;
        }
    }

    expr->type = utypeToExpressionType(expr->utype);
    compilePostAstExpressions(expr, ast);
}

void Compiler::compileNotExpression(Expression* expr, Ast* ast) {
    this->compileExpression(expr, ast->getSubAst(ast_expression));

    Token tok = ast->getToken(NOT);
    if(expr->utype->isArray()) {
        errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expression of type `" + expr->utype->toString() + "` must return a var to use `" +
                                                                        tok.getValue() + "` operator");
    } else if(expr->utype->getType() != utype_unresolved){
        Method *overload;
        List<Field*> params; // def operator!() { ... }

        switch(expr->utype->getResolvedType()->type) {
            case NIL:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "cannot use `" + tok.getValue() + "` operator on expression that returns nil");
                break;
            case UNTYPED:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "the type of the expression that is returned from `" + expr->utype->toString() + "` is untyped. Mostly likely due to a compiler bug, and therefore cannot use operator `" + tok.getValue() + "`");
                break;
            case UNDEFINED:
                break;
            case OBJECT:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expressions of type object must be casted before using `!` operator, try `!((Type)<your-expression>)` instead");
                break;

            case _INT8:
            case _INT16:
            case _INT32:
            case _INT64:
            case _UINT8:
            case _UINT16:
            case _UINT32:
            case _UINT64:
            case VAR:
                expr->utype->getCode().inject(expr->utype->getCode().getInjector(ebxInjector));
                expr->utype->getCode().addIr(OpBuilder::_not(EBX, EBX));

                expr->utype->getCode().freeInjectors();
                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::rstore(EBX));
                break;

            case CLASS:
                if((overload = findFunction(expr->utype->getClass(), "operator" + tok.getValue(),
                                            params, ast, true, fn_op_overload)) != NULL) {
                    compileMethodReturnType(overload, overload->ast, false);

                    validateAccess(overload, ast);
                    expr->utype->copy(overload->utype);

                    expr->utype->getCode().inject(stackInjector);
                    expr->utype->getCode().addIr(OpBuilder::call(overload->address));

                    expr->utype->getCode().freeInjectors();
                    if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                        || expr->utype->getResolvedType()->type == CLASS) {
                        expr->utype->getCode().getInjector(ptrInjector)
                                .addIr(OpBuilder::popObject2());

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    } else if (expr->utype->getResolvedType()->isVar()) {
                        expr->utype->getCode().getInjector(ebxInjector)
                                .addIr(OpBuilder::loadValue(EBX));

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    }
                } else {
                    errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "call to function `" + expr->utype->toString() + "` must return an var or class to use `" + tok.getValue() + "` operator");
                }

                break;
        }
    }

    compilePostAstExpressions(expr, ast);
}

void Compiler::compilePreIncExpression(Expression* expr, Ast* ast) {
    // use restricted insance for context expressions
    this->compileExpression(expr, ast->getSubAst(ast_expression));

    Token tok = ast->hasToken(_INC) ? ast->getToken(_INC) : ast->getToken(_DEC);

    if(expr->utype->isArray()) {
        errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expression of type `" + expr->utype->toString() + "` must return a var to use `" +
                                                                        tok.getValue() + "` operator");
    } else if(expr->utype->getType() != utype_unresolved){
        Method *overload;
        List<Field*> params; // def operator++() { ... }

        switch(expr->utype->getResolvedType()->type) {
            case NIL:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "cannot use `" + tok.getValue() + "` operator on expression that returns nil");
                break;
            case UNTYPED:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "the type of the expression that is returned from `" + expr->utype->toString() + "` is untyped. Mostly likely due to a compiler bug, and therefore cannot use operator `" + tok.getValue() + "`");
                break;
            case UNDEFINED:
                break;
            case OBJECT:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expressions of type object must be casted before using `"
                                                                                + tok.getValue() + "` operator, try `++((Type)<your-expression>)` instead");
                break;

            case _INT8:
            case _INT16:
            case _INT32:
            case _INT64:
            case _UINT8:
            case _UINT16:
            case _UINT32:
            case _UINT64:
            case VAR:
                expr->utype->getCode().inject(expr->utype->getCode().getInjector(ebxInjector));

                if(!expr->utype->getCode().getInjector(incInjector).ir32.empty()) {
                    if (tok == "++")
                        expr->utype->getCode().inject(incInjector);
                    else
                        expr->utype->getCode().inject(decInjector);
                } else {

                    if (tok == "++")
                        expr->utype->getCode().addIr(OpBuilder::inc(EBX));
                    else
                        expr->utype->getCode().addIr(OpBuilder::dec(EBX));

                    if (expr->utype->getResolvedType()->type <= _UINT64) {
                        dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, expr->utype->getCode());
                    }
                }

                expr->utype->getCode().freeInjectors();
                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::rstore(EBX));
                break;

            case CLASS:
                if((overload = findFunction(expr->utype->getClass(), "operator" + tok.getValue(),
                                            params, ast, true, fn_op_overload)) != NULL) {
                    compileMethodReturnType(overload, overload->ast, false);

                    validateAccess(overload, ast);
                    expr->utype->copy(overload->utype);

                    expr->utype->getCode().inject(stackInjector);
                    expr->utype->getCode().addIr(OpBuilder::call(overload->address));

                    expr->utype->getCode().freeInjectors();
                    if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                        || expr->utype->getResolvedType()->type == CLASS) {
                        expr->utype->getCode().getInjector(ptrInjector)
                                .addIr(OpBuilder::popObject2());

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    } else if (expr->utype->getResolvedType()->isVar()) {
                        expr->utype->getCode().getInjector(ebxInjector)
                                .addIr(OpBuilder::loadValue(EBX));

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    }
                } else {
                    errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "call to function `" + overload->toString() + "` must return an var or class to use `" + tok.getValue() + "` operator");
                }

                break;
            default:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "cannot use `" + tok.getValue() + "` operator on expression of type `" + expr->utype->toString() + "`");
                break;
        }
    }

    compilePostAstExpressions(expr, ast);
}

void Compiler::compilePostIncExpression(Expression* expr, bool compileExpression, Ast* ast) {
    if(compileExpression)
        this->compileExpression(expr, ast->getSubAst(ast_expression));

    Token tok = ast->hasToken(_INC) ? ast->getToken(_INC) : ast->getToken(_DEC);

    if(expr->utype->isArray()) {
        errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expression of type `" + expr->utype->toString() + "` must return a var to use `" +
                                                                        tok.getValue() + "` operator");
    } else if(expr->utype->getType() != utype_unresolved){
        Method *overload;
        List<Field*> params;
        List<AccessFlag> flags;
        flags.add(PUBLIC);

        Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
                  ast->line, ast->col);

        params.add(new Field(VAR, guid++, "arg0", currentScope()->klass, flags, meta, stl_stack, 0));
        params.last()->utype = new Utype(VAR);
        params.last()->utype->getCode()
                .addIr(OpBuilder::istore(1));

        switch(expr->utype->getResolvedType()->type) {
            case NIL:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "cannot use `" + tok.getValue() + "` operator on expression that returns nil");
                break;
            case UNTYPED:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "the type of the expression that is returned from `" + expr->utype->toString() + "` is untyped. Mostly likely due to a compiler bug, and therefore cannot use operator `" + tok.getValue() + "`");
                break;
            case UNDEFINED:
                break;
            case OBJECT:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expressions of type object must be casted before using `"
                                                                                + tok.getValue() + "` operator, try `(<your-expression> as type)++` instead");
                break;

            case _INT8:
            case _INT16:
            case _INT32:
            case _INT64:
            case _UINT8:
            case _UINT16:
            case _UINT32:
            case _UINT64:
            case VAR:
                expr->utype->getCode().inject(ebxInjector);
                expr->utype->getCode().addIr(OpBuilder::movr(EGX, EBX));

                if (tok == "++")
                    expr->utype->getCode().inject(incInjector);
                else
                    expr->utype->getCode().inject(decInjector);

                expr->utype->getCode().freeInjectors();
                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::rstore(EGX));
                expr->utype->getCode().getInjector(ebxInjector)
                        .addIr(OpBuilder::movr(EBX, EGX));
                break;

            case CLASS:
                if((overload = findFunction(expr->utype->getClass(), "operator" + tok.getValue(),
                                            params, ast, true, fn_op_overload)) != NULL) {
                    compileMethodReturnType(overload, overload->ast, false);

                    validateAccess(overload, ast);
                    expr->utype->copy(overload->utype);

                    expr->utype->getCode().inject(stackInjector);
                    pushParametersToStackAndCall(ast, overload, params, expr->utype->getCode());

                    expr->utype->getCode().freeInjectors();
                    if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                        || expr->utype->getResolvedType()->type == CLASS) {
                        expr->utype->getCode().getInjector(ptrInjector)
                                .addIr(OpBuilder::popObject2());

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    } else if (expr->utype->getResolvedType()->isVar()) {
                        expr->utype->getCode().getInjector(ebxInjector)
                                .addIr(OpBuilder::loadValue(EBX));

                        expr->utype->getCode().getInjector(removeFromStackInjector)
                                .addIr(OpBuilder::pop());
                    }
                } else {
                    errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "class expression `" + expr->utype->toString() + "` must overload operator `" + tok.getValue() + "` to be used");
                }

                break;
            default:
                errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "cannot use `" + tok.getValue() + "` operator on expression of type `" + expr->utype->toString() + "`");
                break;
        }

        freeListPtr(params);
    }

    compilePostAstExpressions(expr, ast);
}

Field* Compiler::compileLambdaArg(Ast *ast) {
    string name = ast->getToken(0).getValue();
    Utype *utype = NULL;

    RETAIN_TYPE_INFERENCE(false)
    if(ast->hasSubAst(ast_utype))
        utype = compileUtype(ast->getSubAst(ast_utype));
    RESTORE_TYPE_INFERENCE()

    List<AccessFlag> fieldFlags;
    fieldFlags.add(PUBLIC);

    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    Field *field = new Field(UNTYPED, guid++, name, findClass(currModule, globalClass, classes), fieldFlags, meta, stl_stack, 0);

    if(utype) {
        if(utype->getResolvedType() != NULL) {
            field->type = utype->getResolvedType()->type;
            field->utype = utype;
            field->isArray = utype->isArray();
        } else
            field->type = UNDEFINED;
    } else {
        field->type = ANY;
        field->utype = anyUtype;
    }

    return field;
}

void Compiler::compileLambdaArgList(List<Field*> &fields, Ast* ast) {
    for(long i = 0; i < ast->getSubAstCount(); i++) {
        fields.add(compileLambdaArg(ast->getSubAst(i)));
    }
}

Method* Compiler::findLambdaByAst(Ast *ast) {
    for(long i = 0; i < lambdas.size(); i++) {
        if(lambdas.get(i)->ast == ast)
            return lambdas.get(i);
    }

    return NULL;
}

void Compiler::compileLambdaExpression(Expression* expr, Ast* ast) {
    Method *lambda = NULL;

    expr->ast = ast;
    if((lambda = findLambdaByAst(ast)) == NULL) {
        List<Field*> fields;

        if(ast->hasSubAst(ast_lambda_arg_list))
            compileLambdaArgList(fields, ast->getSubAst(ast_lambda_arg_list));

        List<AccessFlag> flags;
        flags.add(PRIVATE);
        flags.add(STATIC);

        Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
                  ast->line, ast->col);

        stringstream ss;
        ss << "anon_func#" << methodSize;
        string name = ss.str();
        ClassObject *lambdaOwner = findClass(currModule, globalClass, classes);

        lambda = new Method(name, currModule, lambdaOwner, guid++, fields, flags, meta);
        lambda->fullName = findClass(currModule, globalClass, classes)->fullName + "." + name;
        lambda->ast = ast;
        lambda->fnType = fn_lambda;
        lambda->type = FNPTR; // lambda's are technically methods but we treat thm as pointers at high level
        lambda->address = methodSize++;
        lambdaOwner->addFunction(lambda);

        compileMethodReturnType(lambda, ast);
        lambdas.add(lambda);
        unProcessedMethods.add(lambda);
    }

    expr->type = exp_var;
    expr->utype->setType(utype_function_ptr);
    expr->utype->setArrayType(false);
    expr->utype->setNullType(false);
    expr->utype->setResolvedType(lambda);

    expr->utype->getCode().addIr(OpBuilder::movi(lambda->address, EBX));
    expr->utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));
}

void Compiler::compileParenExpression(Expression* expr, Ast* ast) {
    compileExpression(expr, ast->getSubAst(ast_expression));
    compilePostAstExpressions(expr, ast);
}

void Compiler::compileSizeOfExpression(Expression* expr, Ast* ast) {
    Expression sizeExpr;
    expr->ast = ast;

    DataEntity *de = new DataEntity();
    de->type = VAR;
    de->guid = guid++;
    expr->utype->setType(utype_native);
    expr->utype->setArrayType(false);
    expr->utype->setResolvedType(de);
    expr->type = exp_var;

    RETAIN_TYPE_INFERENCE(true)
    compileExpression(&sizeExpr, ast->getSubAst(ast_expression));
    RESTORE_TYPE_INFERENCE()

    expr->utype->getCode().inject(sizeExpr.utype->getCode());
    if(sizeExpr.utype->getType() == utype_literal
        && ((Literal*)sizeExpr.utype->getResolvedType())->literalType == string_literal) {
        expr->utype->getCode().addIr(
                OpBuilder::movi(((Literal *) sizeExpr.utype->getResolvedType())->stringData.size(), EBX));
    } else if(sizeExpr.utype->getType() == utype_class || sizeExpr.utype->getType() == utype_field
        || sizeExpr.utype->isArray()){
        expr->utype->getCode().inject(sizeExpr.utype->getCode().getInjector(ptrInjector));
        expr->utype->getCode().addIr(OpBuilder::_sizeof(EBX));
    } else {
        errors->createNewError(GENERIC, expr->ast, "cannot get sizeof from expression of type `" + sizeExpr.utype->toString() + "`");
    }

    expr->utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));

    compilePostAstExpressions(expr, ast, 1);
}

void Compiler::compilePrimaryExpression(Expression* expr, Ast* ast) {
    Ast* branch = ast->getSubAst(0);

    switch(branch->getType()) {
        case ast_not_e:
            return compileNotExpression(expr, branch);
        case ast_literal_e:
            return compileLiteralExpression(expr, branch->getSubAst(ast_literal));
        case ast_utype_class_e:
            return compileUtypeClass(expr, branch);
        case ast_dot_not_e:
            return compileDotNotationCall(expr, branch->getSubAst(ast_dotnotation_call_expr));
        case ast_self_e:
            return compileSelfExpression(expr, branch);
        case ast_base_e:
            return compileBaseExpression(expr, branch);
        case ast_null_e:
            return compileNullExpression(expr, branch);
        case ast_new_e:
            return compileNewExpression(expr, branch);
        case ast_cast_e:
            return compileCastExpression(expr, true, branch);
        case ast_sizeof_e:
            return compileSizeOfExpression(expr, branch);
        case ast_paren_e:
            return compileParenExpression(expr, branch);
        case ast_post_inc_e:
            return compilePostIncExpression(expr, true, branch);
        case ast_arry_e:
            return this->errors->createNewError(GENERIC, ast->line, ast->col, " unsupported array access not attached to object");
        case ast_lambda_function:
            return compileLambdaExpression(expr, branch);
        default:
            return this->errors->createNewError(GENERIC, ast->line, ast->col,
                                                "unexpected expression of type `" + Ast::astTypeToString(branch->getType()) + "` found");
    }

}

void Compiler::compileExpression(Expression* expr, Ast* ast) {
    Ast *branch = ast->getType() == ast_expression ? ast->getSubAst(0) : ast;
    compileExpressionAst(expr, branch);
}

void Compiler::compileExpressionAst(Expression *expr, Ast *branch) {
    if(branch->line >= 3000) {
        int fkdd = 0;
    }
    switch(branch->getType()) {
        case ast_minus_e:
            return compileMinusExpression(expr, branch);
        case ast_primary_expr:
            return compilePrimaryExpression(expr, branch);
        case ast_pre_inc_e:
            return compilePreIncExpression(expr, branch);
        case ast_vect_e:
            return compileVectorExpression(expr, branch);
        case ast_assign_e:
            return compileAssignExpression(expr, branch);
        case ast_and_e:
        case ast_shift_e:
        case ast_equal_e:
        case ast_less_e:
        case ast_mult_e:
        case ast_exponent_e:
        case ast_add_e:
            return compileBinaryExpression(expr, branch);
        case ast_ques_e:
            return compileInlineIfExpression(expr, branch);
    }

    cout << branch->toString() << endl;
    return errors->createNewError(GENERIC, branch->getSubAst(0)->line, branch->getSubAst(0)->col,
                                  "unexpected malformed expression found");
}

bool Compiler::isLambdaFullyQualified(Method *lambda) {
    for(long i = 0; i < lambda->params.size(); i++) {
        Field *param = lambda->params.get(i);

        if(param->type == ANY || param->utype == NULL)
            return false;
    }

    return true;
}

void Compiler::resolveFieldType(Field* field, Utype *utype, Ast* ast) {

    field->isArray = utype->isArray();
    if(utype->getType() == utype_class) {
        field->type = CLASS;
        field->utype = utype;
    } else if(utype->getType() == utype_method) {
        if(typeInference) {
            field->type = FNPTR;
            field->utype = utype;
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " illegal use of function `" + utype->toString() + "` as a type");
    } else if(utype->getType() == utype_native) {
        if(utype->getResolvedType()->type >= NIL)
            goto invalidUtype;

        field->type = utype->getResolvedType()->type;
        field->utype = utype;
    } else if(utype->getType() == utype_literal) {
        if(typeInference) {
            field->type = utype->getResolvedType()->type;
            field->utype = new Utype(utype->getResolvedType()->type, utype->isArray());
        } else
            goto invalidUtype;
    } else if(utype->getType() == utype_field) {
        if(typeInference) {
            if(((Field*)utype->getResolvedType()) == field) {
                this->errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->fullName + "` cannot be self-initialized");
            } else {
                Field *fieldType = ((Field*)utype->getResolvedType());
                RETAIN_TYPE_INFERENCE(false)
                resolveFieldType(field,fieldType->utype, ast);
                RESTORE_TYPE_INFERENCE()
            }
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " illegal use of field `" + utype->toString() + "` as a type");
    } else if(utype->getType() == utype_function_ptr) {
        field->type = FNPTR;
        field->utype = utype;
    } else if(isLambdaUtype(utype)) {
        Method *lambda = (Method*)utype->getResolvedType();
        if(isLambdaFullyQualified(lambda)) {
            field->type = FNPTR;
            field->utype = utype;
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "expression being assigned to field `" + field->fullName + "` is not a fully qualified lambda expression. Try assigning types to all of the fields so that the compiler can correctly resolve the field.");
        }
    } else if(utype->getType() != utype_unresolved) {
        invalidUtype:
        this->errors->createNewError(GENERIC, ast->line, ast->col,
                                     " field `" + field->fullName + "` cannot be assigned type `" + utype->toString() +
                                     "` due to invalid type assignment format");
    }
    if(field->utype == NULL) {
        field->utype = undefUtype;
        field->type = UNDEFINED;
    }
}

void Compiler::resolveAlias(Ast* ast) {
    string name = ast->getToken(0).getValue();

    Alias *alias = currentScope()->klass->getAlias(name, false);
    // we need to do this in case the user tries to use an alias as a class inheritence and there was something wrong with processing it
    if(alias->type == UNTYPED || (alias->type == UNDEFINED && processingStage <= POST_PROCESSING)) {
        findConflicts(ast, "alias", name);

        RETAIN_BLOCK_TYPE(CLASS_SCOPE)
        RETAIN_TYPE_INFERENCE(false)
        Utype *utype = compileUtype(ast->getSubAst(ast_utype));
        RESTORE_TYPE_INFERENCE()
        RESTORE_BLOCK_TYPE()

        alias->type = utype->getResolvedType() ? utype->getResolvedType()->type : UNDEFINED;
        if(alias->utype != NULL) {
            alias->utype->free();
            delete alias->utype;
        }
        alias->utype = utype;
    }
}

void Compiler::findConflicts(Ast *ast, string type, string &name) {
    List<Method*> functions;
    if(type != "field" && currentScope()->klass->getField(name, false) != NULL) {
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with field `" + currentScope()->klass->getField(name, false)->toString() + "`");
    } else if(type != "alias" && currentScope()->klass->getAlias(name, false) != NULL) {
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with alias `" + currentScope()->klass->getField(name, false)->toString() + "`");
    } else if(currentScope()->klass->getAllFunctionsByName(name, functions)) {
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with function `" + functions.get(0)->toString() + "`");
    } else if(currentScope()->klass->getChildClass(name) != NULL) {
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with child class `" + currentScope()->klass->getChildClass(name)->name + "`");
    } else if(currentScope()->klass->getChildClass(name + "<>") != NULL) {
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with child class `" + currentScope()->klass->getChildClass(name + "<>")->name + "`");
    } else if(findClass(currModule, name, classes) != NULL) {
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with class `" + findClass(currModule, name, classes)->fullName + "`");
    } else if(findClass(currModule, name + "<>", generics) != NULL) {
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with generic class `" + findClass(currModule, name + "<>", generics)->fullName + "`");
    }
}

void Compiler::compileVariableDecl(Ast* ast) {
    string name = ast->getToken(0).getValue();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getToken(1).getValue();
    }

    Field *field = currentScope()->klass->getField(name, false);
    assignFieldExpressionValue(field, ast);

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
            Ast *branch = ast->getSubAst(i);

            name = branch->getToken(0).getValue();
            Field *xtraField = currentScope()->klass->getField(name, false);
            assignFieldExpressionValue(xtraField, branch);
        }
    }
}

void Compiler::assignFieldExpressionValue(Field *field, Ast *ast) {
    if(field->type <= CLASS && !isFieldInlined(field)) {
        if(ast->hasSubAst(ast_expression)) {
            BlockType bt;

            if(field->flags.find(STATIC))
                bt = STATIC_BLOCK;
            else
                bt = currentScope()->type;

            RETAIN_BLOCK_TYPE(bt)
            RETAIN_TYPE_INFERENCE(true)

            Token operand("=", SINGLE, ast->col, ast->line, EQUALS);
            Expression leftExpr, rightExpr, resultExpr;
            ReferencePointer fieldReference;
            resultExpr.ast = ast;
            leftExpr.ast = ast;

            fieldReference.classes.add(field->name);
            resolveUtype(fieldReference, leftExpr.utype, ast);
            leftExpr.type = utypeToExpressionType(leftExpr.utype);

            if(field->type == FNPTR) {
                RETAIN_REQUIRED_SIGNATURE((Method*)field->utype->getResolvedType())
                compileExpression(&rightExpr, ast->getSubAst(ast_expression));
                RESTORE_REQUIRED_SIGNATURE()
            } else
                compileExpression(&rightExpr, ast->getSubAst(ast_expression));

            assignValue(&resultExpr, operand, leftExpr, rightExpr, ast, false, false);

            if(field->local) {
                currentScope()->currentFunction->data.code.inject(resultExpr.utype->getCode());
            } else {
                if(field->locality == stl_thread) {
                    tlsMainInserts.inject(resultExpr.utype->getCode());
                } else if (field->flags.find(STATIC)) {
                    getStaticInitFunction()->data.code.inject(resultExpr.utype->getCode());
                } else {
                    List<Method *> constructors;
                    currentScope()->klass->getAllFunctionsByType(fn_constructor, constructors);
                    for (uInt i = 0; i < constructors.size(); i++) {
                        constructors.get(i)->data.code.inject(resultExpr.utype->getCode());
                    }

                    constructors.free();
                }
            }

            RESTORE_TYPE_INFERENCE()
            RESTORE_BLOCK_TYPE()
        } else if(field->local) {
            initializeLocalVariable(field);
        }
    }
}

void Compiler::initializeLocalVariable(Field *field) {
    if(field->isVar() && !field->isArray) {
        currentScope()->currentFunction->data.code
                .addIr(OpBuilder::istorel(field->address, 0));
    } else {
        currentScope()->currentFunction->data.code
                .addIr(OpBuilder::movl(field->address))
                .addIr(OpBuilder::del());
    }
}

void Compiler::resolveField(Ast* ast) {
    string name = ast->getToken(0).getValue();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getToken(1).getValue();
    }

    Field *field = currentScope()->klass->getField(name, false);
    if(field->type == UNTYPED) {
        findConflicts(ast, "field", name);
        // wee need to do this to prevent possible stack overflow errors
        field->type = UNDEFINED;
        field->utype = undefUtype;

        if (field->locality == stl_stack) {
            field->address = field->owner->getFieldAddress(field);
            if(field->address >= CLASS_FIELD_LIMIT) {
                stringstream err;
                err << "maximum class field limit of (" << CLASS_FIELD_LIMIT << ") reached.";
                errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            }
        } else {
            if(field->address >= THREAD_LOCAL_FIELD_LIMIT) {
                stringstream err;
                err << "maximum thread local field limit of (" << THREAD_LOCAL_FIELD_LIMIT << ") reached.";
                errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            }
        }

        if (ast->hasToken(COLON)) {
            Utype *utype = compileUtype(ast->getSubAst(ast_utype));
            field->utype = NULL;
            resolveFieldType(field, utype, ast);
        } else if (ast->hasToken(INFER)) {
            Expression expr;
            RETAIN_BLOCK_TYPE(field->flags.find(STATIC) ? STATIC_BLOCK : currentScope()->type)
            RETAIN_TYPE_INFERENCE(true)
            compileExpression(&expr, ast->getSubAst(ast_expression));
            field->utype = NULL;
            resolveFieldType(field, expr.utype, ast);
            RESTORE_TYPE_INFERENCE()
            RESTORE_BLOCK_TYPE()
        }

        if(field->type != UNTYPED) {
            if (ast->hasSubAst(ast_setter)) {
                resolveSetter(ast->getSubAst(ast_setter), field);
            }

            if (ast->hasSubAst(ast_getter)) {
                resolveGetter(ast->getSubAst(ast_getter), field);
            }
        }

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
                Ast *branch = ast->getSubAst(i);

                name = branch->getToken(0).getValue();
                Field *xtraField = currentScope()->klass->getField(name, false);

                // we cannot add thread local to secondary variables so me must match its locality with the first one
                xtraField->locality = field->locality;
                if(xtraField->locality == stl_stack) {
                    xtraField->address = xtraField->owner->getFieldAddress(xtraField);
                }

                if(xtraField->type == UNTYPED) {
                    resolveFieldType(xtraField, field->utype, branch);
                }
            }
        }
    }
}

void Compiler::resolveSetter(Ast *ast, Field *field) {
    List<Field*> params;
    List<AccessFlag> fieldFlags;

    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    Field *arg0 = new Field(CLASS, guid++, ast->getToken(1).getValue(), field->owner, fieldFlags, meta, stl_stack, 0);
    arg0->utype = field->utype;
    arg0->type = field->type;
    params.add(arg0);

    if(field->flags.find(flg_CONST)) {
        errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "cannot apply setter to constant field `" + field->name + "`");
    }

    List<AccessFlag> flags;
    if(field->flags.find(LOCAL))
        flags.add(LOCAL);

    if(field->flags.find(PUBLIC))
        flags.add(PUBLIC);
    else if(field->flags.find(PRIVATE))
        flags.add(PRIVATE);
    else if(field->flags.find(PROTECTED))
        flags.add(PROTECTED);

    if(field->flags.find(STATIC))
        flags.add(STATIC);

    string fnName = "set_" + field->name;
    Method *method = new Method(fnName, currModule, currentScope()->klass, guid++, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + fnName;
    method->ast = ast;
    method->fnType = fn_normal;
    method->address = methodSize++;

    method->utype = new Utype();
    method->utype->setType(utype_native);

    DataEntity *_void = new DataEntity();
    _void->owner = method->owner;
    _void->type = NIL;
    _void->meta.copy(meta);
    method->utype->setResolvedType(_void);

    if (!addFunction(currentScope()->klass, method, &simpleParameterMatch)) {
        errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "function `" + fnName + "` is already defined in the scope");
        printNote(findFunction(currentScope()->klass, method, &simpleParameterMatch)->meta,
                  "function `" + fnName + "` previously defined here");

        method->free();
        delete method;
    } else {
        field->setter = method;
        unProcessedMethods.add(method);
    }
}

void Compiler::resolveGetter(Ast *ast, Field *field) {
    List<Field*> params;
    List<AccessFlag> flags;
    if(field->flags.find(LOCAL))
        flags.add(LOCAL);

    if(field->flags.find(PUBLIC))
        flags.add(PUBLIC);
    else if(field->flags.find(PRIVATE))
        flags.add(PRIVATE);
    else if(field->flags.find(PROTECTED))
        flags.add(PROTECTED);

    if(field->flags.find(STATIC))
        flags.add(STATIC);

    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    if(field->flags.find(flg_CONST)) {
        createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "field `" + field->name + "` has access specifier `const`. It is not recommended to add getters to constant fields, "
                                                                                                          "as the compiler is no longer able to inline the value for better performance.");
    }

    string fnName = "get_" + field->name;
    Method *method = new Method(fnName, currModule, currentScope()->klass, guid++, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + fnName;
    method->ast = ast;
    method->fnType = fn_normal;
    method->address = methodSize++;
    method->utype = field->utype;

    if (!addFunction(currentScope()->klass, method, &simpleParameterMatch)) {
        errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "function `" + fnName + "` is already defined in the scope");
        printNote(findFunction(currentScope()->klass, method, &simpleParameterMatch)->meta,
                  "function `" + fnName + "` previously defined here");

        method->free();
        delete method;
    } else {
        field->getter = method;
        unProcessedMethods.add(method);
    }

    flags.free();
}

void Compiler::resolveClassFields(Ast* ast, ClassObject* currentClass) {
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
        }
        removeScope();
    }
}

void Compiler::parseMethodAccessFlags(List<AccessFlag> &flags, Ast *ast) {
    long errPos=0;
    for(long i = 0; i < ast->getTokenCount(); i++) {
        flags.add(strToAccessFlag(ast->getToken(i).getValue()));

        if(flags.last() == LOCAL && !globalScope()) {
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                         " `" + ast->getToken(i).getValue() + "` can only be used at global scope");
        } else if(flags.last() == flg_CONST || flags.last() > STATIC) {
            errPos = i;
            goto error;
        }
    }

    if(flags.size() > 2) {
        errors->createNewError(ILLEGAL_ACCESS_DECLARATION, ast->line, ast->col,
                               "too many access specifiers found");
    }

    if(flags.get(0) <= LOCAL) {
        if(flags.size() == 2) {
            if((flags.get(1) != STATIC))
            { errPos = 1; goto error; }
        }
    }
    else if(flags.get(0) == STATIC) {
        if(flags.size() >= 2)
        { errPos = flags.size() - 1; goto error; }
    } else
        goto error;

    if(false) {
        error:
        this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getToken(errPos).getLine(),
                                     ast->getToken(errPos).getColumn(), " `" + ast->getToken(errPos).getValue() + "`");
    }
}

Field* Compiler::compileUtypeArg(Ast* ast) {
    Field *arg = new Field();
    arg->guid = guid;
    arg->module = currModule;
    arg->ast = ast;

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);
    arg->meta.copy(meta);

    // for optional utype args
    if(ast->getTokenCount() != 0) {
        arg->name = ast->getToken(0).getValue();
        arg->fullName = arg->name;
    } else {
        stringstream ss;
        ss << "arg" << guid++;
        arg->name = ss.str();
        arg->fullName = arg->name;
    }

    RETAIN_TYPE_INFERENCE(false)
    resolveFieldType(arg, compileUtype(ast->getSubAst(ast_utype)), ast);
    RESTORE_TYPE_INFERENCE()
    return arg;
}

/**
 * It is inderstood that if you have a function
 * foo(x : _int8, y : int16)
 * if we check for x we will find it once, however if we find it again then
 * we noe have a duplicate definition of parameter x
 *
 * @param params
 * @param name
 * @return
 */
bool Compiler::containsParam(List<Field*> &params, string name) {
    long paramCount = 0;
    for(unsigned int i = 0; i < params.size(); i++) {
        if(params.get(i)->name == name) {
            paramCount++;
            if(paramCount > 1)
                return true;
        }
    }
    return false;
}

bool Compiler::containsParamType(List<Field*> &params, DataType type) {
    for(unsigned int i = 0; i < params.size(); i++) {
        if(params.get(i)->type == type)
            return true;
    }
    return false;
}

void Compiler::validateMethodParams(List<Field*>& params, Ast* ast) {
    for(unsigned int i = 0; i < params.size(); i++) {
        if(containsParam(params, params.get(i)->name)) {
            errors->createNewError(SYMBOL_ALREADY_DEFINED, ast->line, ast->col, "param `" + params.get(i)->toString() + "` already defined in the scope");
        } else {
            params.get(i)->address = i;
            if(params.get(i)->name == "" && ast->getType() == ast_utype_arg_list_opt) {
                stringstream ss;
                ss << "arg" << i;
                params.get(i)->name = ss.str();
                params.get(i)->fullName = ss.str();
            }
        }
    }
}

void Compiler::parseUtypeArgList(List<Field*> &params, Ast* ast) {
    for(unsigned int i = 0; i < ast->getSubAstCount(); i++) {
        params.add(compileUtypeArg(ast->getSubAst(i)));
    }
}

// signature candidates
// private static fn main(string[]) : var;
// private static fn main2(string[]);
// private static fn main3() : var;
// private static fn main4();
void Compiler::checkMainMethodSignature(Method* method) {
    if(globalScope() && method->name == "main") {
        ClassObject* stringClass = findClass(Obfuscater::getModule("std"), "string", classes);

        if(stringClass != NULL) {
            List<Field*> params;
            List<AccessFlag> flags;
            Meta meta;
            Field *arg0 = new Field(CLASS, guid++, "args", method->owner, flags, meta, stl_stack, 0);
            arg0->isArray = true;
            arg0->utype = new Utype(stringClass, true);
            params.add(arg0);
            compileMethodReturnType(method, method->ast, false);

            if(simpleParameterMatch(method->params, params)) {
                if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == VAR) { // fn main(string[]) : var;
                    if(mainMethod == NULL) {
                        mainMethod = method;
                        mainSignature = 0;
                    } else {
                        errors->createNewError(GENERIC, method->ast, "main method with the same or different signature already exists");
                        printNote(mainMethod->meta, "method `main` previously defined here");
                    }

                } else if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == NIL) { // fn main(string[]);
                    if(mainMethod == NULL) {
                        mainMethod = method;
                        mainSignature = 1;
                    } else {
                        errors->createNewError(GENERIC, method->ast, "main method with the same or different signature already exists");
                        printNote(mainMethod->meta, "method `main` previously defined here");
                    }
                } else
                    createNewWarning(GENERIC, __WMAIN, method->ast->line, method->ast->col, "main method might not be executed");
            }

            params.free();
            if(simpleParameterMatch(method->params, params)) {
                if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == NIL) { // fn main();
                    if(mainMethod == NULL) {
                        mainMethod = method;
                        mainSignature = 2;
                    } else {
                        errors->createNewError(GENERIC, method->ast, "main method with the same or different signature already exists");
                        printNote(mainMethod->meta, "method `main` previously defined here");
                    }
                } else if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == VAR) { // fn main() : var;
                    if(mainMethod == NULL) {
                        mainMethod = method;
                        mainSignature = 3;
                    } else {
                        errors->createNewError(GENERIC, method->ast, "main method with the same or different signature already exists");
                        printNote(mainMethod->meta, "method `main` previously defined here");
                    }

                } else
                    createNewWarning(GENERIC, __WMAIN, method->ast->line, method->ast->col, "main method might not be executed");
            }
        } else
            errors->createNewError(GENERIC, method->ast, "class `std#string` was not found when analyzing main method");
    }
}

void Compiler::resolveOperatorOverload(Ast* ast) {
    List<AccessFlag> flags;

    if (ast->hasSubAst(ast_access_type)) {
        parseMethodAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
            flags.add(PUBLIC);

    } else {
        flags.add(PUBLIC);
    }

    if(flags.find(STATIC)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "static operator overloads are not allowed.");
    }

    List<Field*> params;
    string name = ast->getToken(0).getValue() + ast->getToken(1).getValue();
    string op = ast->getToken(1).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, guid++, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_op_overload;
    method->address = methodSize++;
    method->overload = op.at(0);

    compileMethodReturnType(method, ast, true);
    if(!addFunction(currentScope()->klass, method, &simpleParameterMatch)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "function `" + name + "` is already defined in the scope");
        printNote(findFunction(currentScope()->klass, method, &simpleParameterMatch)->meta, "function `" + name + "` previously defined here");

        method->free();
        delete method;
    }
}

void Compiler::compileOperatorOverload(Ast* ast) {
    List<Field*> params;
    string name = ast->getToken(0).getValue() + ast->getToken(1).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    Method *func = findFunction(currentScope()->klass, name, params, ast, false, fn_op_overload, false);

    if(func != NULL) { // if null it must be a delegate func which we dont care about
        compileMethod(ast, func);
    }
}

void Compiler::resolveConstructor(Ast* ast) {
    List<AccessFlag> flags;

    if (ast->hasSubAst(ast_access_type)) {
        parseMethodAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
            flags.add(PUBLIC);
    } else {
        flags.add(PUBLIC);
    }

    List<Field*> params;
    string name = ast->getToken(0).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, guid++, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_constructor;
    method->address = methodSize++;
    method->utype = nilUtype;

    if(name != currentScope()->klass->name) {
        if(IS_CLASS_GENERIC(currentScope()->klass->getClassType())) {
            stringstream adjustedName;
            adjustedName << name << "<>";
            if(adjustedName.str() == currentScope()->klass->getGenericOwner()->name) {
                goto addFunc;
            }
        }

        errors->createNewError(GENERIC, ast->line, ast->col,
                               "constructor `" + name + "` must be the same name as its parent class");
    }

    addFunc:
    if (!addFunction(currentScope()->klass, method, &simpleParameterMatch)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "constructor `" + name + "` is already defined in the scope");
        printNote(findFunction(currentScope()->klass, method, &simpleParameterMatch)->meta,
                  "constructor `" + name + "` previously defined here");

        method->free();
        delete method;
    }
}

void Compiler::compileConstructor(Ast* ast) {
    List<Field*> params;
    string name = ast->getToken(0).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    Method *func = findFunction(currentScope()->klass, name, params, ast, false, fn_constructor, false);

    if(func != NULL) {
        compileMethod(ast, func);
    } else {
        this->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col,
                                     " could not resolve constructor `" + name + "` in `" +
                                             currentScope()->klass->fullName + "`.");
    }
}

void Compiler::compileMethodReturnType(Method* fun, Ast *ast, bool wait) {
    if(fun->utype == NULL) {
        if (ast->hasSubAst(ast_method_return_type)) {
            if (ast->getSubAst(ast_method_return_type)->hasToken("nil"))
                goto void_;

            if (!wait) {
                RETAIN_TYPE_INFERENCE(false)
                RETAIN_SCOPE_CLASS(fun->owner)
                RETAIN_BLOCK_TYPE(fun->flags.find(STATIC) ? STATIC_BLOCK : INSTANCE_BLOCK)
                Utype *utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
                RESTORE_BLOCK_TYPE()
                RESTORE_SCOPE_CLASS()
                RESTORE_TYPE_INFERENCE()

                if (utype->getType() == utype_class
                    || utype->getType() == utype_function_ptr) {
                    fun->utype = utype;
                } else if (utype->getType() == utype_method) {
                    this->errors->createNewError(GENERIC, ast->line, ast->col,
                                                 " illegal use of function `" + utype->toString() +
                                                 "` as a return type");
                } else if (utype->getType() == utype_native) {
                    if (utype->getResolvedType()->type > NIL)
                        this->errors->createNewError(GENERIC, ast->line, ast->col,
                                                     " function `" + fun->fullName + "` cannot use type `" +
                                                     utype->toString() + "` as a return type");
                    fun->utype = utype;
                } else if (utype->getType() == utype_field) {
                    this->errors->createNewError(GENERIC, ast->line, ast->col,
                                                 " illegal use of field `" + utype->toString() + "` as a return type");
                } else
                    this->errors->createNewError(GENERIC, ast->line, ast->col,
                                                 " function `" + fun->fullName + "` cannot use type `" +
                                                 utype->toString() + "` as a return type");

                if (fun->utype == NULL)
                    fun->utype = undefUtype;
            }
        } else if (ast->hasToken(":=")) {
            if (!wait) {

                Expression e;
                RETAIN_TYPE_INFERENCE(false)
                RETAIN_SCOPE_CLASS(fun->owner)
                RETAIN_BLOCK_TYPE(fun->flags.find(STATIC) ? STATIC_BLOCK : INSTANCE_BLOCK)
                RETAIN_CURRENT_FUNCTION(fun)
                addLocalFields(fun);
                compileExpression(&e, ast->getSubAst(ast_expression));
                freeListPtr(fun->data.locals);
                RESTORE_CURRENT_FUNCTION()
                RESTORE_BLOCK_TYPE()
                RESTORE_SCOPE_CLASS()
                RESTORE_TYPE_INFERENCE()

                if(e.utype->getType() == utype_field) {
                    fun->utype = ((Field*)e.utype->getResolvedType())->utype;
                } else if(e.utype->getType() == utype_class) {
                    fun->utype = e.utype;
                } else if(e.utype->getType() == utype_method) {
                    fun->utype = new Utype(VAR);
                } else if(e.utype->getType() == utype_native || e.utype->getType() == utype_function_ptr) {
                    fun->utype = e.utype;
                } else if(e.utype->getType() == utype_literal) {
                    fun->utype = new Utype(e.utype->getResolvedType()->type, e.utype->isArray());;
                } else {
                    this->errors->createNewError(GENERIC, ast->line, ast->col, " function `" + fun->fullName + "` cannot infer type `" + e.utype->toString() + "`");
                }
            }
        } else {
            void_:
            fun->utype = nilUtype;
        }
    }
}

ClassObject* Compiler::getExtensionFunctionClass(Ast* ast) {

    if(ast->getType() == ast_method_decl || ast->getType() == ast_delegate_decl) {
        ReferencePointer ptr;
        compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
        if (ptr.mod == "" && ptr.classes.singular()) {
            return NULL;
        } else {
            if (ptr.mod != "" && ptr.classes.singular()) {
                return resolveClass(Obfuscater::getModule(ptr.mod) ? Obfuscater::getModule(ptr.mod) : undefinedModule, globalClass, ast);
            } else {
                ptr.classes.pop_back();
                return resolveClassReference(ast->getSubAst(ast_refrence_pointer), ptr, true);
            }
        }
    }

    return NULL;
}

void Compiler::resolveGlobalMethod(Ast* ast) {
    ClassObject *resolvedClass = getExtensionFunctionClass(ast);

    if(resolvedClass == NULL) {
        // our method is most likely a normal global function
        resolveMethod(ast);
    } else { // looks like we have an extension function!!
        if(resolvedClass->flags.find(STABLE))
            errors->createNewError(GENERIC, ast->line, ast->col, "extension functions are not allowed on stable class `" + resolvedClass->fullName + "`");

        // god classes must update all its generic classes created under it
        if(IS_CLASS_GENERIC(resolvedClass->getClassType()) && resolvedClass->getGenericOwner() == NULL) {
            resolvedClass->getExtensionFunctionTree().add(ast);
        } else // its can be a generic class that was already created or a reg. class
            resolveMethod(ast, resolvedClass);
    }
}

void Compiler::compileGlobalMethod(Ast* ast) {
    ClassObject *resolvedClass = getExtensionFunctionClass(ast);

    if(resolvedClass == NULL) {
        // our method is most likely a normal global function
        compileMethodDecl(ast);
    } else { // looks like we have an extension function!!
        if(IS_CLASS_GENERIC(resolvedClass->getClassType()) && resolvedClass->getGenericOwner() == NULL) {
            /* ignore */
        } else { // it can be a generic class that was already created or a reg. class
            currScope.add(new Scope(resolvedClass, CLASS_SCOPE));
            compileMethodDecl(ast, resolvedClass);
            removeScope();
        }
    }
}

void Compiler::resolveClassMethod(Ast* ast) {
    ClassObject *resolvedClass = getExtensionFunctionClass(ast);

    if(resolvedClass == NULL) {
        // our method is most likely a normal global function
        resolveMethod(ast);
    } else { // looks like we have an extension function!!
        if(resolvedClass->flags.find(STABLE))
            errors->createNewError(GENERIC, ast->line, ast->col, "extension functions are not allowed on stable class `" + resolvedClass->fullName + "`");

        // god classes must update all its generic classes created under it
        if(IS_CLASS_GENERIC(resolvedClass->getClassType()) && resolvedClass->getGenericOwner() == NULL) {
            resolvedClass->getExtensionFunctionTree().add(ast);
        } else // its can be a generic class that was already created or a reg. class
            resolveMethod(ast, resolvedClass);
    }
}

void Compiler::compileClassMethod(Ast* ast) {
    ClassObject *resolvedClass = getExtensionFunctionClass(ast);

    if(resolvedClass == NULL) {
        // our method is most likely a normal global function
        compileMethodDecl(ast);
    } else { // looks like we have an extension function!!
        if(IS_CLASS_GENERIC(resolvedClass->getClassType()) && resolvedClass->getGenericOwner() == NULL) {
            /* ignore */
        } else // it can be a generic class that was already created or a reg. class
            compileMethodDecl(ast, resolvedClass);
    }
}

void Compiler::resolveMethod(Ast* ast, ClassObject* currentClass) {
    List<AccessFlag> flags;
    bool extensionFun = getExtensionFunctionClass(ast) != NULL;

    if (ast->hasSubAst(ast_access_type)) {
        parseMethodAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
            flags.add(PUBLIC);
        if(globalScope() && !extensionFun) flags.addif(STATIC);
    } else {
        if(globalScope() && !extensionFun) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PUBLIC);
    }

    List<Field*> params;
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
    string name = ptr.classes.last();

    if(currentClass == NULL)
        currentClass = currentScope()->klass;

    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentClass, guid++, params, flags, meta);
    method->fullName = currentClass->fullName + "." + name;
    method->ast = ast;
    method->address = methodSize++;
    if(ast->getType() == ast_delegate_decl) {
        if(method->owner->isGlobalClass())
            this->errors->createNewError(GENERIC, ast->line, ast->col,
                                         "delegate functions are not allowed at global scope");
        method->fnType = fn_delegate;
    } else {
        method->fnType = fn_normal;
    }

    if(method->address >= FUNCTION_LIMIT) {
        stringstream err;
        err << "maximum function limit of (" << FUNCTION_LIMIT << ") reached.";
        errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
    }
    method->extensionFun = extensionFun;

    compileMethodReturnType(method, ast, true);
    if (!addFunction(currentClass, method, &simpleParameterMatch)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "function `" + name + "` is already defined in the scope");
        printNote(findFunction(currentClass, method, &simpleParameterMatch)->meta,
                  "function `" + name + "` previously defined here");

        method->free();
        delete method;
    }
}

void Compiler::resolveClassMethods(Ast* ast, ClassObject* currentClass) {
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
                    resolveClassMethods(branch);
                    break;
                case ast_interface_decl:
                    resolveClassMethods(branch);
                    break;
                case ast_method_decl:
                case ast_delegate_decl:
                    resolveClassMethod(branch);
                    break;
                case ast_operator_decl:
                    resolveOperatorOverload(branch);
                    break;
                case ast_construct_decl:
                    resolveConstructor(branch);
                    break;
                case ast_mutate_decl:
                    resolveClassMutateMethods(branch);
                    break;
                default:
                    break;
            }
        }

        addDefaultConstructor(currentClass, ast);
        removeScope();
    }
}

string Compiler::accessFlagsToString(List<AccessFlag> &flags) {
    stringstream ss;
    ss << "{ ";

    for(long i = 0; i < flags.size(); i++) {
        ss << accessFlagToString(flags.get(i));

        if((i + 1) < flags.size())
            ss << ", ";
    }

    ss << " }";
    return ss.str();
}

string Compiler::accessFlagToString(AccessFlag flag) {
    switch(flag) {
        case PUBLIC:
            return "PUBLIC";
        case PRIVATE:
            return "PRIVATE";
        case PROTECTED:
            return "PROTECTED";
        case LOCAL:
            return "LOCAL";
        case flg_CONST:
            return "CONST";
        case STATIC:
            return "STATIC";
        case STABLE :
            return "STABLE";
        case UNSTABLE :
            return "UNSTABLE";
        case EXTENSION:
            return "EXTENSION";
        case flg_UNDEFINED:
            return "UNDEFINED";
    }

    return "UNDEFINED";
}

/**
 * As of the curent version delegates work as follows:
 * interface engine {
 *   def delegate::run();
 * }
 *
 * class 4cylinder : engine {
 *   def delegate::run() { ... }
 * }
 *
 * class 8cylinder : engine {
 *   def delegate::run() { ... }
 * }
 *
 * def main() {
 *    v8 : engine = new 8cylinder();
 *    v8.run();
 * }
 *
 * The above illustration shows that delegates work as a "subscriber" based system
 *  any class that wants to execute a method dynamically can do so by implementing the
 *  "contract" or signature of the function and can they be obvuscated behind its
 *  contracting class to execute the function.
 *
 * @param subscriber
 * @param contracter
 * @param ast
 */
void Compiler::validateDelegates(ClassObject *subscriber, Ast *ast) {
    List<Method*> subscribedMethods, contractedMethods;

    subscriber->getAllFunctionsByType(fn_normal, subscribedMethods);

    // we want to add all the delegates we need to validate
    ClassObject* currClass = subscriber;
    while(true) {
        if(currClass != NULL)
            getContractedMethods(currClass, contractedMethods);
        else
            break;

        currClass = currClass->getSuperClass();
    }

    // we do this to ensure no bugs will come from searching methods
    for(long long i = 0;   i < contractedMethods.size(); i++) {
        compileMethodReturnType(contractedMethods.get(i), contractedMethods.get(i)->ast);
    }

    for(long long i = 0;   i < subscribedMethods.size(); i++) {
        compileMethodReturnType(subscribedMethods.get(i), subscribedMethods.get(i)->ast);
    }

    for(long long i = 0; i < contractedMethods.size(); i++) {
        Method* contract = contractedMethods.get(i), *sub;
        if((sub = validateDelegatesHelper(contract, subscribedMethods)) != NULL) {
            if((sub->utype != NULL && contract->utype != NULL) || (sub->utype == NULL && contract->utype == NULL)) {
                if(contract->utype != NULL) {
                    if(!contract->utype->equals(sub->utype)) {
                        goto subscribe_err;
                    }
                }

                sub->delegateAddr = contract->address;
            } else {
                subscribe_err:
                stringstream err;
                err << "contract method `" << contract->toString() << "` does not have a subscribed method implemented in class '"
                    << subscriber->fullName << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                printNote(sub->meta, "as defined here");
            }

            if(!(contract->flags.size() == sub->flags.size() && contract->flags.sameElements(sub->flags))) {
                stringstream err;
                err << "method `" << sub->toString() << "` provided " + accessFlagsToString(sub->flags) + " different access privileges than contract method '"
                    << contract->toString() << "' provided " + accessFlagsToString(contract->flags);
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                printNote(contract->meta, "as defined here");
            }
        } else {
            // we only want to complain if the contract method is directly inside of the super class
            if(subscriber->getSuperClass()->match(contract->owner) || subscriber->hasInterface(contract->owner)) {

                stringstream err;
                err << "contract method `" << contract->toString() << "` does not have a subscribed method implemented in class '"
                    << subscriber->fullName << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                printNote(contract->meta, "as defined here");
            }
        }
    }
}

void Compiler::getContractedMethods(ClassObject *subscriber, List<Method *> &contractedMethods) {
    ClassObject* contracter;
    if((contracter = subscriber->getSuperClass()) != NULL) {
        contracter->getAllFunctionsByType(fn_delegate, contractedMethods);
    }

    for(long long i = 0; i < subscriber->getInterfaces().size(); i++) {
        contracter = subscriber->getInterfaces().get(i);
        contracter->getAllFunctionsByType(fn_delegate, contractedMethods);

        if(contracter->getSuperClass() != NULL && !(contracter->getSuperClass()->name == "_object_" && contracter->getSuperClass()->module == Obfuscater::getModule("std"))) {
            for(;;) {

                contracter = contracter->getSuperClass();
                contracter->getAllFunctionsByType(fn_delegate, contractedMethods);

                if(contracter->getSuperClass() == NULL)
                    break;
            }
        }
    }
}

Method* Compiler::validateDelegatesHelper(Method *method, List<Method*> &list) {
    for(long long i = 0; i < list.size(); i++) {
        if(method->name == list.get(i)->name) {
            if (simpleParameterMatch(method->params, list.get(i)->params)) {
                return list.get(i);
            }
        }
    }

    return NULL;
}

void Compiler::resolveAllDelegates(Ast *ast, ClassObject* currentClass) {
    Ast* block = ast->getSubAst(ast_block);

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
        validateDelegates(currentClass, ast);
        for (long i = 0; i < block->getSubAstCount(); i++) {
            Ast *branch = block->getSubAst(i);

            switch (branch->getType()) {
                case ast_class_decl:
                    resolveAllDelegates(branch);
                    break;
                case ast_mutate_decl:
                    resolveDelegateMutateMethods(branch);
                    break;
                default:
                    break;
            }

            CHECK_CMP_ERRORS(return;)
        }
        removeScope();
    }
}

void Compiler::resolveAllDelegates() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        long long totalErrors = errors->getUnfilteredErrorCount();
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
                    resolveAllDelegates(branch);
                    break;
                case ast_mutate_decl:
                    resolveDelegateMutateMethods(branch);
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

bool Compiler::preprocess() {
    processingStage = PRE_PROCESSING;

    bool success = true;
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);
        Obfuscater::addFile(current->getTokenizer()->file, guid++);

        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for(unsigned long x = 0; x < current->size(); x++)
        {
            Ast *branch = current->astAt(x);
            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    string package = parseModuleDecl(branch);
                    Obfuscater::addModule(package, guid++);
                    currModule = Obfuscater::getModule(package);
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    errors->createNewError(GENERIC, branch->line, branch->col, "module declaration must be "
                                                                               "first in every file");
                }
            }

            switch(branch->getType()) {
                case ast_class_decl:
                    preProccessClassDecl(branch, false);
                    break;
                case ast_import_decl: /* ignore for now */
                    break;
                case ast_module_decl: /* fail-safe */
                    errors->createNewError(GENERIC, branch->line, branch->col, "file module cannot be declared more than once");
                    break;
                case ast_interface_decl:
                    preProccessClassDecl(branch, true);
                    break;
                case ast_generic_class_decl:
                    preProccessGenericClassDecl(branch, false);
                    break;
                case ast_generic_interface_decl:
                    preProccessGenericClassDecl(branch, true);
                    break;
                case ast_enum_decl:
                    preProccessEnumDecl(branch);
                    break;
                case ast_delegate_decl: /* ignore */
                case ast_method_decl: /* ignore */
                    break;
                case ast_variable_decl:
                    preProccessVarDecl(branch);
                    break;
                case ast_alias_decl:
                    preProccessAliasDecl(branch);
                    break;
                case ast_mutate_decl:
                    /* ignpre */
                    break;
                case ast_obfuscate_decl:
                    /* ignpre */
                    break;
                default:
                    stringstream err;
                    err << ": unknown ast type: " << branch->getType();
                    errors->createNewError(INTERNAL_ERROR, branch->line, branch->col, err.str());
                    break;
            }

            CHECK_CMP_ERRORS(return false;)
        }

        if(NEW_ERRORS_FOUND()){
            success = false;
            failedParsers.addif(current);
        }
        removeScope();
    }

    handleImports();
    resolveBaseClasses();
    return success;
}

void Compiler::preProcessMutation(Ast *ast, ClassObject *currentClass) {
    if(currentClass == NULL) {
        ReferencePointer ptr;
        compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
        currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);
    }

    if(currentClass) {
        if(currentClass->flags.find(STABLE))
            errors->createNewError(GENERIC, ast->line, ast->col, "cannot mutate stable class `" + currentClass->fullName + "`");

        if(IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            currentClass->getClassMutations().addif(ast);
        } else {
            preProccessClassDecl(ast, IS_CLASS_INTERFACE(currentClass->getClassType()), currentClass);
            resolveSuperClass(ast, currentClass);
        }
    }
}


void Compiler::preProccessClassDeclForMutation(Ast* ast) {
    Ast* block = ast->getLastSubAst();

    for(long i = 0; i < block->getSubAstCount(); i++) {
        Ast* branch = block->getSubAst(i);
        CHECK_CMP_ERRORS(return;)

        switch(branch->getType()) {
            case ast_class_decl:
                preProccessClassDeclForMutation(branch);
                break;
            case ast_generic_class_decl:
                preProccessClassDeclForMutation(branch);
                break;
            case ast_mutate_decl:
                preProcessMutation(branch);
                break;
            default:
                /* ignore */
                break;
        }
    }
}

void Compiler::preprocessMutations() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for(unsigned long x = 0; x < current->size(); x++)
        {
            Ast *branch = current->astAt(x);
            if(x == 0)
            {
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

            switch(branch->getType()) {
                case ast_class_decl:
                    preProccessClassDeclForMutation(branch);
                    break;
                case ast_generic_class_decl:
                    preProccessClassDeclForMutation(branch);
                    break;
                case ast_mutate_decl:
                    preProcessMutation(branch);
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

void Compiler::randomizeGUID() {
    std::mt19937 generator((uInt)(NANO_TOMICRO(std::chrono::duration_cast<std::chrono::nanoseconds>
                   (std::chrono::high_resolution_clock::now().time_since_epoch()).count())>>4));
    std::uniform_int_distribution<Int> randomInt;
    decltype(randomInt.param()) new_range (1, 100000);
    randomInt.param(new_range);

    guid = randomInt(generator);
}

void Compiler::setup() {
    noteMessages.init();
    classes.init();
    enums.init();
    generics.init();
    warnings.init();
    currScope.init();
    importMap.init();
    stringMap.init();
    functionPtrs.init();
    failedParsers.init();
    constantMap.init();
    unProcessedClasses.init();
    inlinedFields.init();
    staticMainInserts.init();
    tlsMainInserts.init();
    initFuncs.init();
    nilUtype = new Utype(NIL);
    varUtype = new Utype(VAR);
    objectUtype = new Utype(OBJECT);
    nullUtype = new Utype(OBJECT);
    undefUtype = new Utype(UNDEFINED);
    anyUtype = new Utype(ANY);
    undefUtype->setType(utype_unresolved);
    nullUtype->setNullType(true);
    Obfuscater::addModule("__$srt_undefined", guid++);
    undefinedModule = Obfuscater::getModule("__$srt_undefined");
}

bool Compiler::allControlPathsReturnAValue(bool *controlPaths) {
    return controlPaths[MAIN_CONTROL_PATH]
        || (controlPaths[IF_CONTROL_PATH] && controlPaths[ELSEIF_CONTROL_PATH] && controlPaths[ELSE_CONTROL_PATH])
        || (controlPaths[TRY_CONTROL_PATH] && controlPaths[CATCH_CONTROL_PATH])
        || (controlPaths[WHEN_CONTROL_PATH] && controlPaths[WHEN_ELSE_CONTROL_PATH]);
}

void Compiler::deInitializeLocalVariables(string &name) {
    for(Int i = 0; i < currentScope()->currentFunction->data.branchTable.size(); i++) {
        BranchTable &branch = currentScope()->currentFunction->data.branchTable.get(i);

        if(branch.labelName == name && !branch.resolved) {
            for(Int j = 0; j < currentScope()->currentFunction->data.locals.size(); j++) {
                Field *local = currentScope()->currentFunction->data.locals.get(j);
                if(local->initialized_pc > branch.branch_pc) {
                    local->initialized = false;
                }
            }
        }
    }
}

void Compiler::compileLabelDecl(Ast *ast, bool *controlPaths) {
    if(allControlPathsReturnAValue(controlPaths)) {
        for(Int i = 0; i < CONTROL_PATH_SIZE; i++) {
            controlPaths[i] = false;
        }
    }

    Token &label = ast->getToken(0);

    if(currentScope()->currentFunction->data.getLabel(label.getValue()).start_pc == invalidAddr) {
        currentScope()->currentFunction->data.labelMap.add(
                LabelData(label.getValue(), currentScope()->currentFunction->data.code.size(), currentScope()->scopeLevel));
        deInitializeLocalVariables(label.getValue());
    } else {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "label `" + label.getValue() + "` is already defined in the scope");
    }

    Ast *branch = ast->getSubAst(0);
    if(branch->getType() == ast_block)
        controlPaths[MAIN_CONTROL_PATH] = compileBlock(branch);
    else
        compileStatement(branch->getSubAst(0), controlPaths);
}

void Compiler::compileIfStatement(Ast *ast, bool *controlPaths) {
    Expression cond;
    string ifEndLabel, ifBlockEnd;
    CodeHolder &code = currentScope()->currentFunction->data.code;

    compileExpression(&cond, ast->getSubAst(ast_expression));

    if(cond.type != exp_var) {
        errors->createNewError(GENERIC, ast->line, ast->col, "if statement condition expression must evaluate to a `var`");
    }

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "if_end" << guid++;
    ifEndLabel=labelId.str(); labelId.str("");

    code.inject(cond.utype->getCode());
    code.inject(cond.utype->getCode().getInjector(ebxInjector));
    code.addIr(OpBuilder::movr(CMT, EBX));

    if(ast->getSubAstCount() > 2) {
        labelId << INTERNAL_LABEL_NAME_PREFIX << "if_block_end" << guid++;
        ifBlockEnd=labelId.str(); labelId.str("");

        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifBlockEnd, 0, ast->line, ast->col, currentScope()->scopeLevel));
        code.addIr(OpBuilder::jne(invalidAddr));

        if(ast->line >= 3000) {
            int jkjk  = 3000;
        }
        controlPaths[IF_CONTROL_PATH] = compileBlock(ast->getSubAst(ast_block));
        controlPaths[ELSEIF_CONTROL_PATH] = true;

        currentScope()->isReachable = true;
        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
        code.addIr(OpBuilder::jmp(invalidAddr));

        currentScope()->currentFunction->data.labelMap.add(LabelData(ifBlockEnd, code.size(), 0));

        for(Int i = 2; i < ast->getSubAstCount(); i++) {
            Ast *branch = ast->getSubAst(i);

            currentScope()->scopeLevel++;
            switch(branch->getType()) {
                case ast_elseif_statement: {
                    Expression elseIfCond;
                    compileExpression(&elseIfCond, branch->getSubAst(ast_expression));

                    if(elseIfCond.type != exp_var) {
                        errors->createNewError(GENERIC, ast->line, ast->col, "else if condition expression must evaluate to a `var`");
                    }

                    labelId << INTERNAL_LABEL_NAME_PREFIX << "if_block_end" << guid++;
                    ifBlockEnd=labelId.str(); labelId.str("");

                    code.inject(elseIfCond.utype->getCode());
                    code.inject(elseIfCond.utype->getCode().getInjector(ebxInjector));
                    code.addIr(OpBuilder::movr(CMT, EBX));

                    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifBlockEnd, 0, ast->line, ast->col, currentScope()->scopeLevel));
                    code.addIr(OpBuilder::jne(invalidAddr));

                    if(!compileBlock(branch->getSubAst(ast_block))) {
                        controlPaths[ELSEIF_CONTROL_PATH] = false;
                    }

                    if((i+1) < ast->getSubAstCount()) {
                        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                        code.addIr(OpBuilder::jmp(invalidAddr));
                    }

                    currentScope()->currentFunction->data.labelMap.add(LabelData(ifBlockEnd, code.size(), 0));
                    break;
                }

                case ast_else_statement: {
                    controlPaths[ELSE_CONTROL_PATH] = compileBlock(branch->getSubAst(ast_block));
                    break;
                }
            }
            currentScope()->isReachable = true;
            currentScope()->scopeLevel--;
        }

    } else {
        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
        code.addIr(OpBuilder::jne(invalidAddr));
        compileBlock(ast->getSubAst(ast_block)); // we dont care about control paths because they are not compelete
        currentScope()->isReachable = true;
    }

    currentScope()->currentFunction->data.labelMap.add(LabelData(ifEndLabel, code.size(), 0));
    code.addIr(OpBuilder::nop()); // Theoretically we shouldn't need this, only for visual purposes
}

/**
 * This function MUST only be used with break; and continue; statements respectively
 *
 * It it intended to locate the total amount of possible finally or lock blocks skipped when jumping
 * to the beginning or end of a loop
 * @return
 */
bool Compiler::insideFinallyBlock() {
    for(Int i = currentScope()->astList.size() - 1; i >= 0; i--) {
        ast_type statement = currentScope()->astList.get(i)->getType();
        if(statement == ast_finally_block)
            return true;
    }

    return false;
}

void Compiler::compileReturnStatement(Ast *ast, bool *controlPaths) {
    currentScope()->isReachable = false;
    Expression returnVal;
    CodeHolder &code = currentScope()->currentFunction->data.code;

    if(ast->getSubAst(ast_expression)) {
        compileExpression(&returnVal, ast->getSubAst(ast_expression));
    }
    else {
        returnVal.ast = ast;
        returnVal.type = exp_nil;
        returnVal.utype->copy(nilUtype);
    }

    if(!returnVal.utype->isRelated(currentScope()->currentFunction->utype)
        && !(isUtypeClassConvertableToVar(currentScope()->currentFunction->utype, returnVal.utype))
        && !(isUtypeConvertableToNativeClass(currentScope()->currentFunction->utype, returnVal.utype))) {
        errors->createNewError(GENERIC, ast->line, ast->col, "returning `" + returnVal.utype->toString() + "` from a function returning `"
                                                             + currentScope()->currentFunction->utype->toString() + "`.");
    }

    if(insideFinallyBlock()) {
        errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }

    if(!currentScope()->finallyBlocks.empty()) {
        for (Int i = currentScope()->finallyBlocks.size() - 1; i >= 0; i--) {
            currentScope()->isReachable = true;
            if (currentScope()->finallyBlocks.get(i).value != NULL) {
                compileBlock(currentScope()->finallyBlocks.get(i).value);
            }
        }
    }

    if(!currentScope()->lockBlocks.empty()) {
        for (Int i = currentScope()->lockBlocks.size() - 1; i >= 0; i--) {
            Expression lockExpr;
            compileExpression(&lockExpr, currentScope()->lockBlocks.get(i));

            lockExpr.utype->getCode().inject(ptrInjector);
            code.inject(lockExpr.utype->getCode());
            code.addIr(OpBuilder::unlock());
        }
    }

    if(isUtypeClassConvertableToVar(currentScope()->currentFunction->utype, returnVal.utype)) {
        CodeHolder tmp;
        convertNativeIntegerClassToVar(returnVal.utype, currentScope()->currentFunction->utype, tmp, ast);
        returnVal.utype->copy(currentScope()->currentFunction->utype);
        returnVal.type = utypeToExpressionType(returnVal.utype);
        returnVal.utype->getCode().copy(tmp);
        tmp.free();
    } else if(isUtypeConvertableToNativeClass(currentScope()->currentFunction->utype, returnVal.utype)) {
        CodeHolder tmp;
        convertUtypeToNativeClass(currentScope()->currentFunction->utype, returnVal.utype, tmp, ast);
        returnVal.utype->copy(currentScope()->currentFunction->utype);
        returnVal.type = utypeToExpressionType(returnVal.utype);
        returnVal.utype->getCode().copy(tmp);
        tmp.free();
    }

    currentScope()->isReachable = false;
    switch (returnVal.type) {
        case exp_var:
            returnVal.utype->getCode().inject(ebxInjector);
            code.inject(returnVal.utype->getCode());
            code.addIr(OpBuilder::returnValue(EBX));
            break;

        case exp_class:
        case exp_object:
        case exp_null:
            code.inject(returnVal.utype->getCode());
            code.inject(returnVal.utype->getCode().getInjector(ptrInjector));
            code.addIr(OpBuilder::returnObject());
            break;

        case exp_nil:
            code.inject(returnVal.utype->getCode());
            break;
    }

    code.addIr(OpBuilder::ret(NO_ERR));
    controlPaths[MAIN_CONTROL_PATH] = true;
}

void Compiler::compileLocalVariableDecl(Ast *ast) {
    List<AccessFlag> flags;

    if(ast->hasSubAst(ast_access_type)){
        parseVariableAccessFlags(flags, ast->getSubAst(ast_access_type));
    }

    for(Int i = 0; i < flags.size(); i++) {
        if(flags.get(i) == LOCAL) {
            createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                             "`local` access specifier is not necessary on local variables");
        } else if(flags.get(i) != flg_CONST) {
            errors->createNewError(GENERIC, ast->line, ast->col, "illegal access declaration applied to local variable `" +
                    accessFlagToString(flags.get(i)) + "`");
        }
    }

    Field *field;
    Utype *fieldType;
    string name = ast->getToken(0).getValue();
    StorageLocality locality = stl_stack;
    Int threadLocalAddress = invalidAddr;
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        locality = strtostl(name);
        name = ast->getToken(1).getValue();
        flags.addif(STATIC);
    }

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    if (ast->hasSubAst(ast_setter) || ast->hasSubAst(ast_getter)) {
        errors->createNewError(GENERIC, ast->line, ast->col, "getters and setters are not allowed on local variables");
    }


    if (locality == stl_thread) {
        threadLocalAddress = checkstl(locality);
        if(threadLocalAddress >= THREAD_LOCAL_FIELD_LIMIT) {
            stringstream err;
            err << "maximum thread local field limit of (" << THREAD_LOCAL_FIELD_LIMIT << ") reached.";
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
        }
    }

    if (ast->hasToken(COLON)) {
        fieldType = compileUtype(ast->getSubAst(ast_utype));

        field = createLocalField(name, NULL, fieldType->isArray(), locality, flags, currentScope()->scopeLevel, ast);
        if(field->scopeLevel == currentScope()->scopeLevel) {
            field->initialized_pc = currentScope()->currentFunction->data.code.size();
            resolveFieldType(field, fieldType, ast);

            if(ast->hasSubAst(ast_expression)) {
                if(field->flags.find(flg_CONST)) {

                    Expression expr;
                    compileExpression(&expr, ast->getSubAst(ast_expression));
                    inlineField(field, expr);
                    currentScope()->currentFunction->data.localVariablesSize--;
                }
            } else if(field->flags.find(flg_CONST)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "constant field `" + field->name + "` requires a value to be assigned to it");
            }
        }
    } else if (ast->hasToken(INFER)) {
        RETAIN_TYPE_INFERENCE(true)
        Expression expr;
        compileExpression(&expr, ast->getSubAst(ast_expression));

        field = createLocalField(name, NULL, expr.utype->isArray(), locality, flags, currentScope()->scopeLevel, ast);
        if(field->scopeLevel == currentScope()->scopeLevel) {
            field->initialized_pc = currentScope()->currentFunction->data.code.size();
            resolveFieldType(field, expr.utype, ast);

            if(flags.find(flg_CONST)) {
                inlineField(field, expr);
                currentScope()->currentFunction->data.localVariablesSize--; // we do this because constant variables do not take up memory
            }
        }
        RESTORE_TYPE_INFERENCE()
    }

    if(field->locality == stl_thread)
        field->address = threadLocalAddress;

    assignFieldExpressionValue(field, ast);

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
            Ast *branch = ast->getSubAst(i);

            name = branch->getToken(0).getValue();
            Field *xtraField = createLocalField(name, field->utype, field->utype->isArray(), locality, flags, currentScope()->scopeLevel, branch);

            if(xtraField->scopeLevel == currentScope()->scopeLevel) {
                if (xtraField->locality == stl_thread)
                    xtraField->address = checkstl(xtraField->locality);

                if(flags.find(flg_CONST)) {
                    if(branch->hasSubAst(ast_expression)) {
                        Expression expr;
                        compileExpression(&expr, branch->getSubAst(ast_expression));
                        inlineField(field, expr);
                    } else {
                        errors->createNewError(GENERIC, branch->line, branch->col, "constant field `" + xtraField->name + "` requires a value to be assigned to it");
                    }
                }
                else
                    assignFieldExpressionValue(xtraField, branch);
            }
        }
    }
}

void Compiler::compileDoWhileStatement(Ast *ast) {
    string whileEndLabel, whileBeginLabel;
    CodeHolder &code = currentScope()->currentFunction->data.code;

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "do_while_begin" << guid++;
    whileBeginLabel=labelId.str(); labelId.str("");

    labelId << INTERNAL_LABEL_NAME_PREFIX << "do_while_end" << guid++;
    whileEndLabel=labelId.str(); labelId.str("");

    currentScope()->currentFunction->data.labelMap.add(LabelData(whileBeginLabel, currentScope()->currentFunction->data.code.size(), 0));

    RETAIN_LOOP_LABELS(whileBeginLabel, whileEndLabel)
    compileBlock(ast->getSubAst(ast_block));
    RESTORE_LOOP_LABELS()

    Expression condExpr;
    compileExpression(&condExpr, ast->getSubAst(ast_expression));

    if(condExpr.type != exp_var) {
        errors->createNewError(GENERIC, ast->line, ast->col, "do while loop condition expression must evaluate to a `var`");
    }

    condExpr.utype->getCode().inject(ebxInjector);
    code.inject(condExpr.utype->getCode());
    code.addIr(OpBuilder::movr(CMT, EBX));

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), whileBeginLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::je(invalidAddr));

    currentScope()->currentFunction->data.labelMap.add(LabelData(whileEndLabel, currentScope()->currentFunction->data.code.size(), 0));
    currentScope()->isReachable = true;
}

void Compiler::processScopeExitLockAndFinallys(string &label) {
    Int locksSkipped = 0, finallyBlocksSkipped = 0;
    CodeHolder &code = currentScope()->currentFunction->data.code;
    List<Ast*> finallyBlocksToIgnore;

    for(Int i = currentScope()->astList.size() - 1; i >= 0; i--) {
        Ast* branch = currentScope()->astList.get(i);
        if(branch->getType() == ast_block) {
            for(Int j = 0; j < branch->getSubAstCount(); j++) {
                Ast *statement = branch->getSubAst(j);
                if(statement->getType() == ast_statement)
                    statement = statement->getSubAst(0);

                if(statement->getType() == ast_label_decl) {
                    if(statement->getToken(0).getValue() == label) {
                        goto processFinally;
                    }

                    Ast *tmpBranch = statement;
                    while(true) {

                        if (tmpBranch->getSubAst(0)->getType() == ast_statement) {
                            if(tmpBranch->getSubAst(0)->getSubAst(0)->getType() == ast_label_decl) {
                                if(tmpBranch->getSubAst(0)->getSubAst(0)->getToken(0).getValue() == label) {
                                    goto processFinally;
                                }

                                tmpBranch = tmpBranch->getSubAst(0)->getSubAst(0);
                            } else break;
                        } else break;
                    }
                }
            }
        } else if(branch->getType() == ast_trycatch_statement)
            finallyBlocksSkipped++;
        else if(branch->getType() == ast_lock_statement)
            locksSkipped++;
        else if(branch->getType() == ast_label_decl) {
            if(branch->getToken(0).getValue() == label) {
                break;
            }

            Ast *tmpBranch = branch;
            while(true) {

                if (tmpBranch->getSubAst(0)->getType() == ast_statement) {
                    if(tmpBranch->getSubAst(0)->getSubAst(0)->getType() == ast_label_decl) {
                        if(tmpBranch->getSubAst(0)->getSubAst(0)->getToken(0).getValue() == label) {
                            goto processFinally;
                        }
                        tmpBranch = tmpBranch->getSubAst(0)->getSubAst(0);
                    } else break;
                } else break;
            }
        }
        else if(branch->getType() == ast_finally_block) {
            finallyBlocksToIgnore.add(branch);
        }
    }

    processFinally:
    if(finallyBlocksSkipped > 0) {
        for (Int i = currentScope()->finallyBlocks.size() - 1; i >= 0; i--) {
            currentScope()->isReachable = true;
            if (currentScope()->finallyBlocks.get(i).value != NULL
                && !finallyBlocksToIgnore.find(currentScope()->finallyBlocks.get(i).value)) {
                compileBlock(currentScope()->finallyBlocks.get(i).value);
            }

            if(--finallyBlocksSkipped == 0)
                break;
        }
    }

    if(locksSkipped > 0) {
        for(Int i = currentScope()->lockBlocks.size() - 1; i >= 0; i--) {
            Expression lockExpr;
            compileExpression(&lockExpr, currentScope()->lockBlocks.get(i));

            lockExpr.utype->getCode().inject(ptrInjector);
            code.inject(lockExpr.utype->getCode());
            code.addIr(OpBuilder::unlock());

            if(--locksSkipped == 0)
                break;
        }
    }

}

void Compiler::compileGotoStatement(Ast *ast) {
    Token &label = ast->getToken(0);
    CodeHolder &code = currentScope()->currentFunction->data.code;

    processScopeExitLockAndFinallys(label.getValue());
    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), label.getValue(), 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jmp(invalidAddr));
    currentScope()->isReachable = false;
}

ClassObject* Compiler::compileCatchClause(Ast *ast, TryCatchData &tryCatchData, bool *controlPaths) {
    CatchData catchData;
    ClassObject *exceptionClass = NULL;

    if(ast->getSubAst(ast_utype_arg)->getTokenCount() > 0) {
        Field *catchField;
        List<AccessFlag> flags;
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();
        Utype *handlingClass = compileUtype(ast->getSubAst(ast_utype_arg)->getSubAst(ast_utype));

        if(handlingClass->isArray()) {
            errors->createNewError(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` cannot evaluate to an array");
        }

        if(handlingClass->getType() != utype_class) {
            errors->createNewError(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` must be of type class");
        }

        catchField = createLocalField(name, handlingClass, false, stl_stack, flags, currentScope()->scopeLevel+1, ast);
        catchData.localFieldAddress = catchField->address;
        if(handlingClass->getClass() != NULL)
            catchData.classAddress = handlingClass->getClass()->address;
        exceptionClass = handlingClass->getClass();
    } else {
        Utype *handlingClass = compileUtype(ast->getSubAst(ast_utype_arg)->getSubAst(ast_utype));

        if(handlingClass->isArray()) {
            errors->createNewError(GENERIC, ast->line, ast->col, "handling class  `" + handlingClass->toString() + "` cannot evaluate to an array");
        }

        if(handlingClass->getType() != utype_class) {
            errors->createNewError(GENERIC, ast->line, ast->col, "type `" + handlingClass->toString() + "` must be of type class");
        }

        if(handlingClass->getClass() != NULL)
            catchData.classAddress = handlingClass->getClass()->address;
        catchData.localFieldAddress = -1;
        exceptionClass = handlingClass->getClass();
    }

    catchData.handler_pc = currentScope()->currentFunction->data.code.size();
    if(!compileBlock(ast->getSubAst(ast_block))) {
        controlPaths[CATCH_CONTROL_PATH] = false;
    }

    tryCatchData.catchTable.add(catchData);
    return exceptionClass;
}

_register Compiler::compileAsmRegister(Ast *ast) {
    if(to_lower(ast->getToken(0).getValue()) == "adx") {
        return ADX;
    } else if(to_lower(ast->getToken(0).getValue()) == "cx") {
        return CX;
    } else if(to_lower(ast->getToken(0).getValue()) == "cmt") {
        return CMT;
    } else if(to_lower(ast->getToken(0).getValue()) == "ebx") {
        return EBX;
    } else if(to_lower(ast->getToken(0).getValue()) == "ecx") {
        return ECX;
    } else if(to_lower(ast->getToken(0).getValue()) == "ecf") {
        return ECF;
    } else if(to_lower(ast->getToken(0).getValue()) == "edf") {
        return EDF;
    } else if(to_lower(ast->getToken(0).getValue()) == "ehf") {
        return EHF;
    } else if(to_lower(ast->getToken(0).getValue()) == "bmr") {
        return BMR;
    } else if(to_lower(ast->getToken(0).getValue()) == "egx") {
        return EGX;
    } else {
        return ADX;
    }
}

opcode_arg Compiler::compileAsmLiteral(Ast *ast) {
    CodeHolder &code = currentScope()->currentFunction->data.code;

    Int multiplier = ast->hasSubAst(ast_pre_inc_e) ? -1 : 1;
    if(ast->getTokenCount() == 1) {
        Expression tmp;
        compileLiteralExpression(&tmp, ast);

        Literal *literal = (Literal*)tmp.utype->getResolvedType();
        return literal->address != invalidAddr ? multiplier * literal->address : multiplier * literal->numericData;
    } else if(ast->getToken(0) == "@") {
        opcode_arg offset = getAsmOffset(ast);

        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ast->getToken(1).getValue(), offset, ast->line, ast->col, currentScope()->scopeLevel));
    } else if(ast->getToken(0) == "[") {
        List<Method*> functions;
        currentScope()->klass->getAllFunctionsByName(ast->getToken(1).getValue(), functions, true);
        opcode_arg offset = getAsmOffset(ast);

        if(!functions.empty()) {
            if(offset < functions.size()) {
                return functions.get(offset)->address;
            } else {
                stringstream err;
                err << "cannot locate function with offset of (" << offset << ")";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        } else {

            errors->createNewError(GENERIC, ast->line, ast->col, "attempt to get address of a function.");
        }
    } else if(ast->getToken(0) == "{") {
        RETAIN_BLOCK_TYPE(INSTANCE_BLOCK)
        obfuscateMode = true;
        Utype *utype = compileUtype(ast->getSubAst(ast_utype));
        obfuscateMode = false;
        RESTORE_BLOCK_TYPE()

        opcode_arg offset = getAsmOffset(ast);

        if(utype->getType() != utype_unresolved) {
            if(utype->getType() == utype_class
                || utype->getType() == utype_method
                || utype->getType() == utype_field) {
                return utype->getResolvedType()->address + offset;
            } else {
                stringstream err;
                err << "cannot get address of type `" << utype->toString() << "`.";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
    }

    return invalidAddr;
}

opcode_arg Compiler::getAsmOffset(Ast *ast) {
    opcode_arg offset = 0;
    if(ast->hasToken(PLUS) || ast->hasToken(MINUS)) {
        if(ast->getToken(3).getId() == INTEGER_LITERAL) {
            string hex_string = invalidateUnderscores(ast->getToken(3).getValue());

#if _ARCH_BITS == 64
            offset = strtoll(hex_string.c_str(), NULL, 16);
#else
            offset = strtol(hex_string.c_str(), NULL, 16);
#endif
        } else {
            string hex_string = invalidateUnderscores(ast->getToken(3).getValue());

#if _ARCH_BITS == 64
            offset = strtod(hex_string.c_str(), NULL);
#else
            offset = strtod(hex_string.c_str(), NULL);
#endif
        }
    }
    return offset;
}

void Compiler::compileWhenStatement(Ast *ast, bool *controlPaths) {
    CodeHolder &code = currentScope()->currentFunction->data.code;
    Expression condExpr, outExpr;
    string whenEndLabel, nextClauseLabel, clauseStartLabel;
    Ast *whenBlock = ast->getSubAst(ast_when_block);
    Token operand("==", SINGLE, ast->col, ast->line, EQEQ);

    controlPaths[WHEN_CONTROL_PATH] = true;
    if(ast->hasSubAst(ast_expression))
        compileExpression(&condExpr, ast->getSubAst(ast_expression));

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "when_end" << guid++;
    whenEndLabel=labelId.str(); labelId.str("");

    for(Int i = 0; i < whenBlock->getSubAstCount(); i++) {
        Ast *branch = whenBlock->getSubAst(i);

        currentScope()->scopeLevel++;
        switch(branch->getType()) {
            case ast_when_clause: {
                labelId << INTERNAL_LABEL_NAME_PREFIX << "when_next_case" << guid++;
                nextClauseLabel=labelId.str(); labelId.str("");
                labelId << INTERNAL_LABEL_NAME_PREFIX << "when_case_start" << guid++;
                clauseStartLabel=labelId.str(); labelId.str("");

                for(Int j = 0; j < branch->getSubAstCount() - 1; j++) {
                    Ast *expressionAst = branch->getSubAst(j);
                    Expression compareExpr;

                    outExpr.utype->free();
                    outExpr.ast = expressionAst;

                    if((ast->hasSubAst(ast_expression) && condExpr.utype->getType() == utype_function_ptr)
                       || (condExpr.utype->getType() == utype_field && ((Field*)condExpr.utype->getResolvedType())->type == FNPTR)) {
                        if(condExpr.utype->getType() == utype_field) {
                            RETAIN_REQUIRED_SIGNATURE((Method*)((Field*)condExpr.utype->getResolvedType())->utype->getResolvedType())
                            compileExpression(&compareExpr, expressionAst);
                            RESTORE_REQUIRED_SIGNATURE()
                        } else {
                            RETAIN_REQUIRED_SIGNATURE((Method*)condExpr.utype->getResolvedType())
                            compileExpression(&compareExpr, expressionAst);
                            RESTORE_REQUIRED_SIGNATURE()
                        }
                    } else
                        compileExpression(&compareExpr, expressionAst);

                    if(ast->hasSubAst(ast_expression)) {
                        compileBinaryExpression(&outExpr, operand, condExpr, compareExpr, branch);
                        if(outExpr.type != exp_var) {
                            errors->createNewError(GENERIC, branch->line, branch->col, "comparing `" + outExpr.utype->toString()
                                                   + "` to `" + outExpr.utype->toString() + "` must evaluate to a `var`");
                        }

                    } else {
                        if(compareExpr.type != exp_var) {
                            errors->createNewError(GENERIC, branch->line, branch->col, "comparing `" + compareExpr.utype->toString()
                                                   + "` to `" + compareExpr.utype->toString() + "` must evaluate to a `var`");
                        }
                    }

                    if(ast->hasSubAst(ast_expression)) {
                        outExpr.utype->getCode().inject(ebxInjector);
                        code.inject(outExpr.utype->getCode());
                        code.addIr(OpBuilder::movr(CMT, EBX));
                    } else {
                        compareExpr.utype->getCode().inject(ebxInjector);
                        code.inject(compareExpr.utype->getCode());
                        code.addIr(OpBuilder::movr(CMT, EBX));
                    }

                    if((j + 1) < (branch->getSubAstCount() -1)) {
                        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), clauseStartLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                        code.addIr(OpBuilder::je(invalidAddr));
                    } else {
                        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), nextClauseLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                        code.addIr(OpBuilder::jne(invalidAddr));
                    }
                }


                currentScope()->currentFunction->data.labelMap.add(LabelData(clauseStartLabel, currentScope()->currentFunction->data.code.size(), 0));
                if(!compileBlock(branch->getSubAst(ast_block))) {
                    controlPaths[WHEN_CONTROL_PATH] = false;
                }

                if((i+1) < whenBlock->getSubAstCount()) {
                    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), whenEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                    code.addIr(OpBuilder::jmp(invalidAddr));
                }

                currentScope()->isReachable = true;
                currentScope()->currentFunction->data.labelMap.add(LabelData(nextClauseLabel, currentScope()->currentFunction->data.code.size(), 0));
                break;
            }

            case ast_when_else_clause: {
                controlPaths[WHEN_ELSE_CONTROL_PATH] = compileBlock(branch->getSubAst(ast_block));
                break;
            }
        }
        currentScope()->scopeLevel--;
    }

    currentScope()->currentFunction->data.labelMap.add(LabelData(whenEndLabel, currentScope()->currentFunction->data.code.size(), 0));
    code.addIr(OpBuilder::nop());
}

void Compiler::compileTryCatchStatement(Ast *ast, bool *controlPaths) {
    string tryEndLabel, finallyStartLabel, finallyEndLabel;
    TryCatchData tryCatchData;
    List<ClassObject*> catchedClasses;
    Field *exceptionObjectField = NULL;

    ClassObject *throwable = resolveClass(Obfuscater::getModule("std"), "throwable", ast);
    bool hasFinallyBlock = ast->hasSubAst(ast_finally_block);
    CodeHolder &code = currentScope()->currentFunction->data.code;

    if(hasFinallyBlock) {
        List<AccessFlag> flags;
        flags.add(PUBLIC);
        stringstream errFieldName;
        errFieldName << INTERNAL_VARIABLE_NAME_PREFIX << "exception_object0";
        if((exceptionObjectField = currentScope()->currentFunction->data.getLocalField(errFieldName.str())) == NULL) {
            exceptionObjectField = createLocalField(errFieldName.str(), objectUtype, true, stl_stack, flags, 0,
                                                    ast);
        }
    }

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "try_end" << guid++;
    tryEndLabel=labelId.str(); labelId.str("");

    labelId << INTERNAL_LABEL_NAME_PREFIX << "finally_start" << guid++;
    finallyStartLabel=labelId.str(); labelId.str("");

    labelId << INTERNAL_LABEL_NAME_PREFIX << "finally_end" << guid++;
    finallyEndLabel=labelId.str(); labelId.str("");

    tryCatchData.block_start_pc = code.size();
    tryCatchData.try_start_pc = code.size();
    currentScope()->finallyBlocks.add(KeyPair<string, Ast*>(
            hasFinallyBlock ? finallyStartLabel : "",ast->getSubAst(ast_finally_block)));
    controlPaths[TRY_CONTROL_PATH] = compileBlock(ast->getSubAst(ast_block));
    tryCatchData.block_end_pc = code.size();

    currentScope()->currentFunction->data.branchTable.add(
            BranchTable(code.size(), tryEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jmp(invalidAddr));

    controlPaths[CATCH_CONTROL_PATH] = true;
    for(Int i = 1; i < ast->getSubAstCount(); i++) {
        Ast *branch = ast->getSubAst(i);

        currentScope()->scopeLevel++;
        currentScope()->astList.add(branch);
        currentScope()->isReachable = true;
        switch (branch->getType()) {
            case ast_catch_clause: {
                ClassObject *klass = compileCatchClause(branch, tryCatchData, controlPaths);

                if(klass != NULL && !catchedClasses.addif(klass)) {
                    errors->createNewError(GENERIC, ast->line, ast->col, "class `" + klass->fullName + "` has already been caught.");
                }

                if(!klass->isClassRelated(throwable)) {
                    errors->createNewError(GENERIC, ast->line, ast->col, "handling class `" + klass->fullName + "` must inherit base level exception class `std#throwable`");
                }

                currentScope()->currentFunction->data.branchTable.add(
                        BranchTable(code.size(), tryEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                code.addIr(OpBuilder::jmp(invalidAddr));
                currentScope()->isReachable = true;
                break;
            }

            case ast_finally_block: {
                tryCatchData.try_end_pc = code.size();

                string nextFinallyLabel;
                if(!currentScope()->finallyBlocks.empty()) {
                    bool currentBranchFound = false;
                    for(Int j = currentScope()->finallyBlocks.size() - 1; j >= 0; j--) {
                        if(currentScope()->finallyBlocks.get(j).value == branch) {
                            currentBranchFound = true;
                        } else if(currentScope()->finallyBlocks.get(j).value != NULL
                                  && currentBranchFound) {
                            nextFinallyLabel = currentScope()->finallyBlocks.get(j).key;
                            break;
                        }
                    }
                }

                currentScope()->finallyBlocks.last().value = NULL;
                currentScope()->currentFunction->data.labelMap.add(LabelData(tryEndLabel, currentScope()->currentFunction->data.code.size(), 0));
                initializeLocalVariable(exceptionObjectField);

                currentScope()->currentFunction->data.labelMap.add(LabelData(finallyStartLabel, currentScope()->currentFunction->data.code.size(), 0));
                tryCatchData.finallyData = new FinallyData();
                tryCatchData.finallyData->start_pc = code.size();
                tryCatchData.finallyData->exception_object_field_address = exceptionObjectField->address;
                compileBlock(branch->getSubAst(ast_block));

                if(nextFinallyLabel.empty()) {
                    code.addIr(OpBuilder::movl(exceptionObjectField->address))
                            .addIr(OpBuilder::checkNull(CMT));

                    currentScope()->currentFunction->data.branchTable.add(
                            BranchTable(code.size(), finallyEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                    code.addIr(OpBuilder::je(invalidAddr))
                            .addIr(OpBuilder::pushObject())
                            .addIr(OpBuilder::ret(ERR_STATE)); // it will only return of exception object still has not been handled
                } else {
                    code.addIr(OpBuilder::movl(exceptionObjectField->address))
                        .addIr(OpBuilder::checkNull(CMT));

                    currentScope()->currentFunction->data.branchTable.add(
                            BranchTable(code.size(), finallyEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                    code.addIr(OpBuilder::je(invalidAddr));

                    currentScope()->currentFunction->data.branchTable.add(
                            BranchTable(code.size(), nextFinallyLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
                    code.addIr(OpBuilder::jmp(invalidAddr)); // it will only return of exception object still has not been handled

                }

                currentScope()->currentFunction->data.labelMap.add(LabelData(finallyEndLabel, currentScope()->currentFunction->data.code.size(), 0));
                tryCatchData.finallyData->end_pc = code.size();
                break;
            }
        }

        currentScope()->scopeLevel--;
        currentScope()->astList.pop_back();
    }

    if(!hasFinallyBlock) {
        currentScope()->currentFunction->data.labelMap.add(
                LabelData(tryEndLabel, currentScope()->currentFunction->data.code.size(), 0));
        tryCatchData.try_end_pc = code.size();
    }
    code.addIr(OpBuilder::nop());

    catchedClasses.free();
    currentScope()->finallyBlocks.pop_back();
    currentScope()->currentFunction->data.tryCatchTable.add(tryCatchData);
}

void Compiler::compileLockStatement(Ast *ast, bool *controlPaths) {
    CodeHolder &code = currentScope()->currentFunction->data.code;
    Expression lockExpr;

    compileExpression(&lockExpr, ast->getSubAst(ast_expression));

    if(lockExpr.type == exp_object
        || lockExpr.type == exp_class
        || (lockExpr.type == exp_var && lockExpr.utype->isArray())) {

        lockExpr.utype->getCode().inject(ptrInjector);
        code.inject(lockExpr.utype->getCode());
        code.addIr(OpBuilder::lock());

        currentScope()->lockBlocks.add(ast->getSubAst(ast_expression));
        if(compileBlock(ast->getSubAst(ast_block))) {
            controlPaths[MAIN_CONTROL_PATH] = true;
        }
        currentScope()->lockBlocks.pop_back();

        lockExpr.utype->getCode().inject(ptrInjector);
        code.inject(lockExpr.utype->getCode());
        code.addIr(OpBuilder::unlock());
    } else {
        errors->createNewError(GENERIC, ast->line, ast->col, "attempt to lock non-lockable object of type `" + lockExpr.utype->toString() + "`.");
    }
}

/**
 * This function MUST only be used with break; and continue; statements respectively
 *
 * It it intended to locate the total amount of possible finally or lock blocks skipped when jumping
 * to the beginning or end of a loop
 * @return
 */
Int Compiler::getSkippedBlockCount(ast_type triggerStatement) {
    Int blocksSkipped = 0;
    for(Int i = currentScope()->astList.size() - 1; i >= 0; i--) {
        ast_type statement = currentScope()->astList.get(i)->getType();
        if(statement == triggerStatement)
            blocksSkipped++;
        else if(statement == ast_while_statement
            || statement == ast_foreach_statement
            || statement == ast_do_while_statement
            || statement == ast_for_statement)
            break;
    }

    return blocksSkipped;
}

void Compiler::compileContinueStatement(Ast *ast) {
    CodeHolder &code = currentScope()->currentFunction->data.code;
    Int finallyBlocksSkipped = getSkippedBlockCount(ast_finally_block);
    Int lockBlocksSkipped = getSkippedBlockCount(ast_lock_statement);

    if(finallyBlocksSkipped > 0) {
        errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }

    if(lockBlocksSkipped > 0) {
        for(Int i = currentScope()->lockBlocks.size() - 1; i >= 0; i--) {
            Expression lockExpr;
            compileExpression(&lockExpr, currentScope()->lockBlocks.get(i));

            lockExpr.utype->getCode().inject(ptrInjector);
            code.inject(lockExpr.utype->getCode());
            code.addIr(OpBuilder::unlock());

            if(--lockBlocksSkipped == 0)
                break;
        }
    }

    if(currentScope()->loopStartLabel.empty()) {
        errors->createNewError(GENERIC, ast->line, ast->col, "attempt to call `continue` outside of an enclosing loop");
    }

    currentScope()->currentFunction->data.branchTable.add(
            BranchTable(code.size(), currentScope()->loopStartLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jmp(invalidAddr));
    currentScope()->isReachable = false;
}

void Compiler::compileBreakStatement(Ast *ast) {
    CodeHolder &code = currentScope()->currentFunction->data.code;
    Int finallyBlocksSkipped = getSkippedBlockCount(ast_finally_block);
    Int lockBlocksSkipped = getSkippedBlockCount(ast_lock_statement);

    if(finallyBlocksSkipped > 0) {
        errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }

    if(lockBlocksSkipped > 0) {
        for(Int i = currentScope()->lockBlocks.size() - 1; i >= 0; i--) {
            Expression lockExpr;
            compileExpression(&lockExpr, currentScope()->lockBlocks.get(i));

            lockExpr.utype->getCode().inject(ptrInjector);
            code.inject(lockExpr.utype->getCode());
            code.addIr(OpBuilder::unlock());

            if(--lockBlocksSkipped == 0)
                break;
        }
    }

    if(currentScope()->loopStartLabel.empty()) {
        errors->createNewError(GENERIC, ast->line, ast->col, "attempt to call `break` outside of an enclosing loop");
    }

    currentScope()->currentFunction->data.branchTable.add(
            BranchTable(code.size(), currentScope()->loopEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jmp(invalidAddr));
    currentScope()->isReachable = false;
}

void Compiler::compileThrowStatement(Ast *ast, bool *controlPaths) {
    Expression exceptionExpr;
    CodeHolder &code = currentScope()->currentFunction->data.code;
    currentScope()->isReachable = false;
    controlPaths[MAIN_CONTROL_PATH] = true;

    compileExpression(&exceptionExpr, ast->getSubAst(ast_expression));

    if(exceptionExpr.type == exp_class) {
        ClassObject *throwable = resolveClass(Obfuscater::getModule("std"), "throwable", ast);
        if(exceptionExpr.utype->getClass()->isClassRelated(throwable)) {
            exceptionExpr.utype->getCode().inject(stackInjector);
            code.inject(exceptionExpr.utype->getCode());
            code.addIr(OpBuilder::_throw());
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + exceptionExpr.utype->toString() + "` must inherit base level exception class `std#throwable`");
        }
    } else {
        errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + exceptionExpr.utype->toString() + "` must evaluate to a class");
    }
}

void Compiler::compileWhileStatement(Ast *ast) {
    string whileEndLabel, whileBeginLabel;
    CodeHolder &code = currentScope()->currentFunction->data.code;

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "while_begin" << guid++;
    whileBeginLabel=labelId.str(); labelId.str("");

    labelId << INTERNAL_LABEL_NAME_PREFIX << "while_end" << guid++;
    whileEndLabel=labelId.str(); labelId.str("");

    Expression condExpr;
    compileExpression(&condExpr, ast->getSubAst(ast_expression));

    if(condExpr.type != exp_var) {
        errors->createNewError(GENERIC, ast->line, ast->col, "while loop condition expression must evaluate to a `var`");
    }

    currentScope()->currentFunction->data.labelMap.add(LabelData(whileBeginLabel, currentScope()->currentFunction->data.code.size(), 0));

    condExpr.utype->getCode().inject(ebxInjector);
    code.inject(condExpr.utype->getCode());
    code.addIr(OpBuilder::movr(CMT, EBX));

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), whileEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jne(invalidAddr));

    RETAIN_LOOP_LABELS(whileBeginLabel, whileEndLabel)
    compileBlock(ast->getSubAst(ast_block));
    RESTORE_LOOP_LABELS()

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), whileBeginLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jmp(invalidAddr));

    currentScope()->currentFunction->data.labelMap.add(LabelData(whileEndLabel, currentScope()->currentFunction->data.code.size(), 0));
    currentScope()->isReachable = true;
}

void Compiler::compileForEachStatement(Ast *ast) {
    string forEndLabel, forBeginLabel;
    CodeHolder &code = currentScope()->currentFunction->data.code;

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "foreach_begin" << guid++;
    forBeginLabel=labelId.str(); labelId.str("");

    labelId << INTERNAL_LABEL_NAME_PREFIX << "foreach_end" << guid++;
    forEndLabel=labelId.str(); labelId.str("");

    Expression arrayExpr;
    compileExpression(&arrayExpr, ast->getSubAst(ast_expression));
    if(!arrayExpr.utype->isArray()
        && !(arrayExpr.utype->getClass()
        && arrayExpr.utype->getClass()->getLoopableClass())) {
        errors->createNewError(GENERIC, ast->line, ast->col,
                               " foreach expression`" + arrayExpr.utype->toString() + "` must be an array type.");
    }

    if(!arrayExpr.utype->isArray() && arrayExpr.utype->getClass() && arrayExpr.utype->getClass()->getLoopableClass()) {
        ClassObject *loopable = arrayExpr.utype->getClass()->getLoopableClass();

        List<Field*> emptyParams;
        Method *get_elements = findFunction(loopable, "get_elements", emptyParams, ast, true, fn_delegate);
        if(get_elements != NULL) {
            if(get_elements->utype->isArray()) {
                arrayExpr.utype->getCode().inject(stackInjector);
                pushParametersToStackAndCall(ast, get_elements, emptyParams, arrayExpr.utype->getCode());

                arrayExpr.utype->getCode().freeInjectors();
                arrayExpr.utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::popObject2());

                arrayExpr.utype->getCode().getInjector(removeFromStackInjector)
                        .addIr(OpBuilder::pop());

                arrayExpr.type = utypeToExpressionType(get_elements->utype);
                arrayExpr.utype->copy(get_elements->utype);
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " support function `" + get_elements->toString() + "` must return an array.");
            } // we must make sure the user didnt srew with this function
        } // we dont need to error out the user will recieve an error elsewhere
    }

    currentScope()->scopeLevel++;
    Field *iteratorField, *indexField, *arrayResultField;
    List<AccessFlag> flags;
    if(ast->getSubAst(ast_utype_arg)->hasSubAst(ast_utype)) {
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();
        Utype *fieldType = compileUtype(ast->getSubAst(ast_utype_arg)->getSubAst(ast_utype));

        if(fieldType->isArray()) {
            errors->createNewError(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` cannot evaluate to an array");
        }

        iteratorField = createLocalField(name, fieldType, false, stl_stack, flags, currentScope()->scopeLevel, ast);

        fieldType->setArrayType(true);
        if(!arrayExpr.utype->isRelated(fieldType)) {
            errors->createNewError(GENERIC, ast->line, ast->col,
                                   " incompatible types, cannot convert `" + arrayExpr.utype->toString() + "` to `" +
                                   fieldType->toString() + "`.");
        }
        fieldType->setArrayType(false);
    } else {
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();
        Utype *fieldType = new Utype();
        if(arrayExpr.utype->getType() == utype_field)
            fieldType->copy(((Field*)arrayExpr.utype->getResolvedType())->utype);
        else
            fieldType->copy(arrayExpr.utype);
        fieldType->setArrayType(false);

        iteratorField = createLocalField(name, fieldType, false, stl_stack, flags, currentScope()->scopeLevel, ast);
    }

    if(iteratorField->utype->isRelated(nilUtype)) {
        errors->createNewError(GENERIC, ast->line, ast->col, "illegal use of field `" + iteratorField->name + "` in foreach with type of `nil`");
    }

    stringstream indexFieldName;
    indexFieldName << INTERNAL_VARIABLE_NAME_PREFIX << "foreach_index" << currentScope()->scopeLevel;
    indexField = createLocalField(indexFieldName.str(), varUtype, false, stl_stack, flags, currentScope()->scopeLevel, ast);
    code.addIr(OpBuilder::istorel(indexField->address, -1));

    Utype *arrayFieldType = new Utype();
    arrayFieldType->copy(arrayExpr.utype);
    stringstream arrayFieldName;
    arrayFieldName << INTERNAL_VARIABLE_NAME_PREFIX << "foreach_array_result" << currentScope()->scopeLevel;
    arrayResultField = createLocalField(arrayFieldName.str(), arrayFieldType, true, stl_stack, flags, currentScope()->scopeLevel, ast);
    currentScope()->scopeLevel--;

    arrayExpr.utype->getCode().inject(stackInjector);
    code.inject(arrayExpr.utype->getCode());
    code.addIr(OpBuilder::movl(arrayResultField->address))
        .addIr(OpBuilder::popObject());

    currentScope()->currentFunction->data.labelMap.add(LabelData(forBeginLabel, currentScope()->currentFunction->data.code.size(), 0));

    code.addIr(OpBuilder::iaddl(1, indexField->address))
        .addIr(OpBuilder::movl(arrayResultField->address))
        .addIr(OpBuilder::_sizeof(ECX));

    code.addIr(OpBuilder::loadl(EBX, indexField->address))
        .addIr(OpBuilder::lt(EBX, ECX));

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jne(invalidAddr));

    if(arrayExpr.utype->getResolvedType()->isVar()) {
        code.addIr(OpBuilder::iaload(ECX, EBX));
    } else
        code.addIr(OpBuilder::movnd(EBX));

    if(iteratorField->isVar()) {
        if((arrayExpr.utype->getResolvedType()->type <= _INT64 && iteratorField->type >= _INT8 && iteratorField->type <= _INT64
            && arrayExpr.utype->getResolvedType()->type > iteratorField->type)
            || (arrayExpr.utype->getResolvedType()->type >= _UINT8 && iteratorField->type >= _UINT8 && iteratorField->type <= _UINT64
                 && arrayExpr.utype->getResolvedType()->type > iteratorField->type))
        dataTypeToOpcode(iteratorField->type, ECX, ECX, code);
        code.addIr(OpBuilder::smovr2(ECX, iteratorField->address));
    } else {
        if(arrayResultField->type == OBJECT && iteratorField->type == CLASS) {
            code.addIr(OpBuilder::pushObject())
                    .addIr(OpBuilder::movl(iteratorField->address))
                    .addIr(OpBuilder::popObject())
                    .addIr(OpBuilder::movi(iteratorField->utype->getClass()->address, EBX))
                    .addIr(OpBuilder::cast(EBX));
        } else {
            code.addIr(OpBuilder::pushObject())
                    .addIr(OpBuilder::movl(iteratorField->address))
                    .addIr(OpBuilder::popObject());
        }
    }

    RETAIN_LOOP_LABELS(forBeginLabel, forEndLabel)
    compileBlock(ast->getSubAst(ast_block));
    RESTORE_LOOP_LABELS()

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forBeginLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jmp(invalidAddr));

    currentScope()->currentFunction->data.labelMap.add(LabelData(forEndLabel, currentScope()->currentFunction->data.code.size(), 0));
    currentScope()->isReachable = true;
}

void Compiler::compileForStatement(Ast *ast) {
    string forEndLabel, forBeginLabel;
    CodeHolder &code = currentScope()->currentFunction->data.code;

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "for_begin" << guid++;
    forBeginLabel=labelId.str(); labelId.str("");

    labelId << INTERNAL_LABEL_NAME_PREFIX << "for_end" << guid++;
    forEndLabel=labelId.str(); labelId.str("");

    currentScope()->scopeLevel++; // we need to do this because it will represent the inner scope of the for loop
    if(ast->hasSubAst(ast_variable_decl)) {
        compileLocalVariableDecl(ast->getSubAst(ast_variable_decl));
    }

    currentScope()->currentFunction->data.labelMap.add(LabelData(forBeginLabel, currentScope()->currentFunction->data.code.size(), 0));
    if(ast->hasSubAst(ast_for_expresion_cond)) {
        Expression expr;
        compileExpression(&expr, ast->getSubAst(ast_for_expresion_cond)->getSubAst(ast_expression));

        if(expr.type == exp_var || isUtypeClassConvertableToVar(varUtype, expr.utype)) {
            if(isUtypeClassConvertableToVar(varUtype, expr.utype)) {
                CodeHolder tmp;
                convertNativeIntegerClassToVar(expr.utype, varUtype, tmp, ast);
                code.inject(tmp);
                tmp.free();
            } else {
                expr.utype->getCode().inject(ebxInjector);
                code.inject(expr.utype->getCode());
            }

            code.addIr(OpBuilder::movr(CMT, EBX));
            currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forEndLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
            code.addIr(OpBuilder::jne(invalidAddr));

        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "for loop condition expression must evaluate to a `var`");
        }
    }

    Expression iterExpr; // the block will delete the local iterator field so we need to process the iter code beforehand
    if(ast->hasSubAst(ast_for_expresion_iter)) {
        compileExpression(&iterExpr, ast->getSubAst(ast_for_expresion_iter)->getSubAst(ast_expression));

        iterExpr.utype->getCode().inject(removeFromStackInjector);
    }
    currentScope()->scopeLevel--;

    RETAIN_LOOP_LABELS(forBeginLabel, forEndLabel)
    compileBlock(ast->getSubAst(ast_block));
    RESTORE_LOOP_LABELS()

    if(ast->hasSubAst(ast_for_expresion_iter)) {
        code.inject(iterExpr.utype->getCode());
    }

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forBeginLabel, 0, ast->line, ast->col, currentScope()->scopeLevel));
    code.addIr(OpBuilder::jmp(invalidAddr));

    currentScope()->isReachable = true;
    currentScope()->currentFunction->data.labelMap.add(LabelData(forEndLabel, currentScope()->currentFunction->data.code.size(), 0));
}

void Compiler::compileStatement(Ast *ast, bool *controlPaths) {
    if(currentScope()->currentFunction->data.lineTable.empty() || currentScope()->currentFunction->data.lineTable.last().line < ast->line) {
        currentScope()->currentFunction->data.lineTable.add(
                LineData(currentScope()->currentFunction->data.code.size(), ast->line));
    }

    if(!currentScope()->isReachable) {
        if(ast->getType() != ast_label_decl)
            createNewWarning(GENERIC, __WGENERAL, ast, "unreachable statement");
        currentScope()->isReachable = true;
    }

    currentScope()->astList.add(ast);
    switch(ast->getType()) {
        case ast_return_stmnt:
            compileReturnStatement(ast, controlPaths);
            break;
        case ast_if_statement:
            compileIfStatement(ast, controlPaths);
            break;
        case ast_expression: {
            Expression expr;
            compileExpression(&expr, ast);
            expr.utype->getCode().inject(removeFromStackInjector);
            currentScope()->currentFunction->data.code.inject(expr.utype->getCode());
            // TODO: add code for calling the base constructor
            break;
        }
        case ast_label_decl:
            compileLabelDecl(ast, controlPaths);
            break;
        case ast_variable_decl:
            compileLocalVariableDecl(ast);
            break;
        case ast_alias_decl:
            compileLocalAlias(ast);
            break;
        case ast_for_statement:
            compileForStatement(ast);
            break;
        case ast_foreach_statement:
            compileForEachStatement(ast);
            break;
        case ast_while_statement:
            compileWhileStatement(ast);
            break;
        case ast_do_while_statement:
            compileDoWhileStatement(ast);
            break;
        case ast_throw_statement:
            compileThrowStatement(ast, controlPaths);
            break;
        case ast_goto_statement:
            compileGotoStatement(ast);
            break;
        case ast_continue_statement:
            compileContinueStatement(ast);
            break;
        case ast_break_statement:
            compileBreakStatement(ast);
            break;
        case ast_lock_statement:
            compileLockStatement(ast, controlPaths);
            break;
        case ast_trycatch_statement:
            compileTryCatchStatement(ast, controlPaths);
            break;
        case ast_when_statement:
            compileWhenStatement(ast, controlPaths);
            break;
        case ast_assembly_statement:
            compileAsmStatement(ast);
            break;
        default:
            stringstream err;
            err << ": unknown ast type: " << ast->getType();
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            break;
    }
    currentScope()->astList.pop_back();
}

void Compiler::compileLocalAlias(Ast *ast) {
    Alias *alias = preProccessAliasDecl(ast);
    if(alias != NULL) {
        resolveAlias(ast);
        currentScope()->currentFunction->data.localAliases.add(
                KeyPair<Int, string>(currentScope()->scopeLevel, alias->name));
    }
}

void Compiler::invalidateLocalVariables() {
    checkFields:
    for(Int i = 0; i < currentScope()->currentFunction->data.locals.size(); i++) {
        Field *localField = currentScope()->currentFunction->data.locals.get(i);
        if(localField->scopeLevel == currentScope()->scopeLevel) {
            freePtr(localField);
            currentScope()->currentFunction->data.locals.remove(localField);
            goto checkFields;
        }
    }
}

bool Compiler::compileBlock(Ast *ast) {
    currentScope()->scopeLevel++;
    currentScope()->astList.add(ast);
    bool controlPaths[]
        = {
            false, // MAIN_CONTROL_PATH
            false, // IF_CONTROL_PATH
            false, // ELSEIF_CONTROL_PATH
            false,  // ELSE_CONTROL_PATH
            false,  // TRY_CONTROL_PATH
            false,  // CATCH_CONTROL_PATH
            false,  // WHEN_CONTROL_PATH
            false   // WHEN_ELSE_CONTROL_PATH
        };

    for(unsigned int i = 0; i < ast->getSubAstCount(); i++) {
        Ast *branch = ast->getSubAst(i);

        if(branch->getType() == ast_block) {
            compileBlock(branch);
            continue;
        } else {
            branch = branch->getSubAst(0);
            compileStatement(branch, controlPaths);
        }

        if(currentScope()->currentFunction->data.code.size() >= FUNCTION_OPCODE_LIMIT) {
            stringstream err;
            err << "maximum function opcode limit reached (" << LOCAL_FIELD_LIMIT << ").";
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
        }
    }

    reconcileBranches(false);
    invalidateLocalAliases();
    invalidateLocalVariables();
    currentScope()->scopeLevel--;
    currentScope()->astList.pop_back();
    return allControlPathsReturnAValue(controlPaths);
}

void Compiler::invalidateLocalAliases() {
    checkAliases:
    for(Int i = 0; i < currentScope()->currentFunction->data.localAliases.size(); i++) {
        if(currentScope()->currentFunction->data.localAliases.get(i).key == currentScope()->scopeLevel) {
            Alias *localAlias = currentScope()->klass->getAlias(
                    currentScope()->currentFunction->data.localAliases.get(i).value, false);
            freePtr(localAlias);
            currentScope()->klass->getAliases().remove(localAlias);
            goto checkAliases;
        }
    }
}

void Compiler::reconcileBranches(bool finalTry) {
    CodeHolder &code = currentScope()->currentFunction->data.code;
    for(Int i = 0; i < currentScope()->currentFunction->data.branchTable.size(); i++) {
        BranchTable &branch = currentScope()->currentFunction->data.branchTable.get(i);
        if(!branch.resolved) {
            LabelData label = currentScope()->currentFunction->data.getLabel(branch.labelName);

            if (branch.branch_pc < code.size()) {
                if (label.start_pc != invalidAddr && label.scopeLevel <= branch.scopeLevel) {
                    branch.resolved = true;

                    switch (GET_OP(code.ir32.get(branch.branch_pc))) {
                        case Opcode::MOVI:
                        case Opcode::IADD:
                        case Opcode::ISUB:
                        case Opcode::IMUL:
                        case Opcode::IDIV:
                        case Opcode::IMOD:
                        case Opcode::IADDL:
                        case Opcode::ISUBL:
                        case Opcode::IMULL:
                        case Opcode::IDIVL:
                        case Opcode::IMODL:
                        case Opcode::ISTORE:
                        case Opcode::ISTOREL:
                        case Opcode::ISADD:
                        case Opcode::CMP:
                            code.ir32.get(branch.branch_pc + 1) = label.start_pc + branch._offset;
                            break;
                        case Opcode::JNE:
                            code.ir32.get(branch.branch_pc) = OpBuilder::jne(label.start_pc + branch._offset);
                            break;
                        case Opcode::JE:
                            code.ir32.get(branch.branch_pc) = OpBuilder::je(label.start_pc + branch._offset);
                            break;
                        case Opcode::JMP:
                            code.ir32.get(branch.branch_pc) = OpBuilder::jmp(label.start_pc + branch._offset);
                            break;
                    }
                } else if(finalTry) {
                    errors->createNewError(GENERIC, branch.line, branch.col,
                                           "attempt to jump to label `" + branch.labelName +
                                           "` that dosen't exist in the current context.");
                }
            } else {
                branch.resolved = true;
                errors->createNewError(INTERNAL_ERROR, branch.line, branch.col,
                                       ": attempt to reconcile branch that is not in codebase.");
            }
        }
    }

    checkLabels:
    for(Int i = 0; i < currentScope()->currentFunction->data.labelMap.size(); i++) {
        if(currentScope()->currentFunction->data.labelMap.get(i).scopeLevel == currentScope()->scopeLevel) {
            LabelData ld = currentScope()->currentFunction->data.labelMap.get(i);
            currentScope()->currentFunction->data.labelMap.removeAt(i);
            goto checkLabels;
        }
    }
}

Method* Compiler::getStaticInitFunction() {
    List<Method *> functions;
    currentScope()->klass->getAllFunctionsByTypeAndName(fn_normal, INTERNAL_STATIC_INIT_FUNCTION, false, functions);

    if(functions.empty()) {
        List<Field*> emptyParams;
        List<AccessFlag> flags;
        flags.add(PRIVATE);
        flags.add(STATIC);
        Ast *ast = currentScope()->klass->ast;

        Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line),
                  Obfuscater::getFile(current->getTokenizer()->file), ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

        Method* initFun = new Method(INTERNAL_STATIC_INIT_FUNCTION, currModule, currentScope()->klass, guid++, emptyParams, flags, meta);

        initFun->fullName = currentScope()->klass->fullName + "." + INTERNAL_STATIC_INIT_FUNCTION;
        initFun->ast = ast;
        initFun->fnType = fn_normal;
        initFun->address = methodSize++;
        initFun->utype = nilUtype;
        currentScope()->klass->addFunction(initFun);
        initFuncs.add(initFun);
        return initFun;
    } else return functions.get(0);
}

void Compiler::compileInitDecl(Ast *ast) {
    Ast *block = ast->getSubAst(ast_block);

    List<AccessFlag> flags;
    bool staticInit = false;
    if (ast->hasSubAst(ast_access_type)) {
        Ast *accessFlagBranch = ast->getSubAst(ast_access_type);
        if(accessFlagBranch->getTokenCount() > 1) {
            this->errors->createNewError(GENERIC, ast->line, ast->col, "init declarations only allows access specifier (static)");
        } else {
            if(strToAccessFlag(accessFlagBranch->getToken(0).getValue()) == STATIC) {
                staticInit = true;
            } else {
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col, "`" + accessFlagBranch->getToken(0).getValue() + "`");
            }
        }
    }

    if(staticInit) {
        currentScope()->currentFunction = getStaticInitFunction();

        RETAIN_BLOCK_TYPE(STATIC_BLOCK)
        compileBlock(block);
        RESTORE_BLOCK_TYPE()

        reconcileBranches(true);
        currentScope()->resetLocalScopeFlags();
    } else {
        List<Method *> constructors;
        currentScope()->klass->getAllFunctionsByType(fn_constructor, constructors);
        for (uInt i = 0; i < constructors.size(); i++) {
            Method *constructor = constructors.get(i);
            uInt totalErrors = errors->getUnfilteredErrorCount();
            currentScope()->currentFunction = constructor;

            RETAIN_BLOCK_TYPE(INSTANCE_BLOCK)
            compileBlock(block);
            RESTORE_BLOCK_TYPE()

            reconcileBranches(true);
            currentScope()->resetLocalScopeFlags();
            if (NEW_ERRORS_FOUND()) {
                break; // no need to waste processing power to compile a broken init decl
            }
        }

        constructors.free();
    }
}

void Compiler::compileAllInitDecls() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (unsigned long x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);
            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    string module = parseModuleDecl(branch);
                    currModule = Obfuscater::getModule(module);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch(branch->getType()) {
                case ast_class_decl:
                    compileClassInitDecls(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateInitDecls(branch);
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

void Compiler::compileAllObfuscationRules() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (unsigned long x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);
            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    string module = parseModuleDecl(branch);
                    currModule = Obfuscater::getModule(module);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch(branch->getType()) {
                case ast_class_decl:
                    compileClassObfuscations(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateObfuscations(branch);
                    break;
                case ast_obfuscate_decl:
                    compileObfuscateDecl(branch);
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

void Compiler::compileAllFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (unsigned long x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);
            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    string module = parseModuleDecl(branch);
                    currModule = Obfuscater::getModule(module);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch(branch->getType()) {
                case ast_class_decl:
                    compileClassFields(branch);
                    break;
                case ast_variable_decl:
                    compileVariableDecl(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateFields(branch);
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

void Compiler::compileMethodDecl(Ast *ast, ClassObject* currentClass) {
    bool extensionFun = getExtensionFunctionClass(ast) != NULL;

    List<Field*> params;
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
    string name = ptr.classes.last();

    if(currentClass == NULL)
        currentClass = currentScope()->klass;

    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    Method *func = findFunction(currentClass, name, params, ast, false, fn_normal, false);

   if(func != NULL) { // if null it must be a delegate func which we dont care about
       checkMainMethodSignature(func);

       if((func->name == staticInitMethod || func->name == tlsSetupMethod)
            && func->owner->fullName == "platform.kernel#platform") {
            if(ast->hasSubAst(ast_expression) || ast->getSubAst(ast_block)->sub_asts.size() > 0) {
                errors->createNewError(GENERIC, ast, "platform level function `" + func->toString() + "` does not allow statements or expressions inside.");
            }
           return;
       }
       compileMethod(ast, func);
   }
}

void Compiler::compileMethod(Ast *ast, Method *func) {

    func->compiled = true;
    if(ast->hasSubAst(ast_block)) {
        Ast *block = ast->getSubAst(ast_block);
        currentScope()->currentFunction = func;
        bool allCodePathsReturnValue = false;
        addLocalFields(func);
        compileMethodReturnType(func, ast, false);
        RETAIN_BLOCK_TYPE(func->flags.find(STATIC) ? STATIC_BLOCK : INSTANCE_BLOCK)

        if(func->fnType == fn_constructor) {
            callBaseClassConstructor(ast, func);
        }

        allCodePathsReturnValue = compileBlock(block);
        RESTORE_BLOCK_TYPE()

        reconcileBranches(true);
        if(func->utype->isRelated(nilUtype)) {
            if(!func->data.code.ir32.empty() && GET_OP(func->data.code.ir32.last()) != Opcode::RET) {
                func->data.code.addIr(OpBuilder::ret(NO_ERR));
            } else
                func->data.code.addIr(OpBuilder::ret(NO_ERR));
        } else if(!func->utype->isRelated(undefUtype)){
            if(!allCodePathsReturnValue) {
                errors->createNewError(GENERIC, block, "not all code paths return a value");
            }
        }
    } else if(ast->hasSubAst(ast_expression)) {
        compileMethodReturnType(func, ast);
        currentScope()->currentFunction = func;

        RETAIN_BLOCK_TYPE(func->flags.find(STATIC) ? STATIC_BLOCK : INSTANCE_BLOCK)
        currentScope()->currentFunction->data.lineTable.add(
                LineData(currentScope()->currentFunction->data.code.size(), ast->getSubAst(ast_expression)->line));
        Expression expr;
        addLocalFields(func);
        compileExpression(&expr, ast->getSubAst(ast_expression));
        freeListPtr(func->data.locals);
        RESTORE_BLOCK_TYPE()

        if(!expr.utype->isRelated(func->utype)) {
            errors->createNewError(GENERIC, ast->line, ast->col, "return value of type `" + expr.utype->toString() + "` is not compatible with that of type `"
                + func->utype->toString() + "`");
        }

        if(expr.type == exp_nil) {
            func->data.code.inject(expr.utype->getCode());
            func->data.code.addIr(OpBuilder::ret(NO_ERR));
        } else {
            if(expr.type == exp_var && !expr.utype->isArray()) {
                expr.utype->getCode().inject(ebxInjector);
                func->data.code.inject(expr.utype->getCode());
                func->data.code.addIr(OpBuilder::returnValue(EBX))
                    .addIr(OpBuilder::ret(NO_ERR));
            } else {
                expr.utype->getCode().inject(ptrInjector);
                func->data.code.inject(expr.utype->getCode());
                func->data.code.addIr(OpBuilder::returnObject())
                        .addIr(OpBuilder::ret(NO_ERR));
            }
        }
    }

//        printMethodCode(*constructor, ast);
    currentScope()->resetLocalScopeFlags();
}

void Compiler::callBaseClassConstructor(Ast *ast, Method *func) {
    if(ast->hasSubAst(ast_base_class_constructor)) {
        Ast *baseClassConstr = ast->getSubAst(ast_base_class_constructor);

        if(func->owner->getSuperClass() != NULL) {
            List<Expression *> expressions;
            List<Field *> constructorParams;
            string basicName = func->owner->getSuperClass()->name;
            if(basicName.find("<") != string::npos) {
                stringstream ss;
                for(int i = 0; i < basicName.size(); i++) {
                    if(basicName[i] == '<')
                        break;
                    ss << basicName[i];
                }

                basicName = ss.str();
            }

            compileExpressionList(expressions, baseClassConstr->getSubAst(ast_expression_list));
            expressionsToParams(expressions, constructorParams);
            Method *constructor = findFunction(func->owner->getSuperClass(), basicName,
                                               constructorParams, baseClassConstr, false, fn_constructor);

            if (constructor != NULL) {
                func->data.code.addIr(OpBuilder::pushl(0));
                pushParametersToStackAndCall(baseClassConstr, constructor, constructorParams, func->data.code);
            }
            else {
                errors->createNewError(GENERIC, baseClassConstr->line, baseClassConstr->col,
                                             " could not resolve base class constructor in class `" +
                                             func->owner->getSuperClass()->name + "`.");
            }
        } else {
            errors->createNewError(GENERIC, baseClassConstr->line, baseClassConstr->col,
                                         " class `" +
                                         func->owner->fullName + "` does not inherit a base class.");
        }
    } else {
        if(func->owner->getSuperClass() != NULL) {
            string basicName = func->owner->getSuperClass()->name;
            if(basicName.find("<") != string::npos) {
                stringstream ss;
                for(int i = 0; i < basicName.size(); i++) {
                    if(basicName[i] == '<')
                        break;
                    ss << basicName[i];
                }

                basicName = ss.str();
            }

            List<Field*> emptyParams;
            Method *constructor = findFunction(func->owner->getSuperClass(), basicName, emptyParams, ast, false, fn_constructor);

            if(constructor != NULL) {
                func->data.code.addIr(OpBuilder::pushl(0));
                pushParametersToStackAndCall(ast, constructor, emptyParams, func->data.code);
            }
            else {
                errors->createNewError(INTERNAL_ERROR, ast->line, ast->col,
                                             " could not resolve base class constructor `" + func->owner->getSuperClass()->name + "` in `" +
                                             currentScope()->klass->fullName + "`.");
            }
        }
    }
}

void Compiler::compileAllMethods() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (unsigned long x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);
            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    string module = parseModuleDecl(branch);
                    currModule = Obfuscater::getModule(module);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    currModule = undefinedModule;
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                }
            }

            switch(branch->getType()) {
                case ast_class_decl:
                    compileClassMethods(branch);
                    break;
                case ast_method_decl:
                    compileGlobalMethod(branch);
                    break;
                case ast_mutate_decl:
                    compileClassMutateMethods(branch);
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

void Compiler::assignEnumFieldName(Field *enumField) {
    Field *nameField = ((ClassObject*)enumField->utype->getResolvedType())->getField("name", true);

    if(nameField != NULL) {
        Int index = stringMap.addIfIndex(enumField->name);
        staticMainInserts.addIr(OpBuilder::newString(stringMap.indexof(enumField->name)))
            .addIr(OpBuilder::movg(enumField->owner->address))
            .addIr(OpBuilder::movn(enumField->address))
            .addIr(OpBuilder::movn(nameField->address))
            .addIr(OpBuilder::popObject());
    } else
        errors->createNewError(INTERNAL_ERROR, enumField->ast,
                         " enum class field `name` could not be located");
}


void Compiler::getEnumValue(CodeHolder &code, Field *enumField) {
    Ast *ast = enumField->ast;

    if(ast->hasSubAst(ast_expression)) {
        RETAIN_BLOCK_TYPE(STATIC_BLOCK)
        Expression expr;
        compileExpression(&expr, ast->getSubAst(ast_expression));

        if(expr.type == exp_var) {
            if(expr.utype->getType() == utype_field) {
                if(isFieldInlined((Field*)expr.utype->getResolvedType())) {
                    double inlinedValue = getInlinedFieldValue((Field *) expr.utype->getResolvedType());
                    if (!isWholeNumber(inlinedValue)) {
                        errors->createNewError(GENERIC, ast,
                                               "enum value must evaluate to an integer, the constant variable's value you have derived resolves to a decimal");
                    }

                    enumValue = (long)inlinedValue + 1;
                } else
                    errors->createNewError(GENERIC, ast,
                                           " The expression being assigned to enum `" + enumField->name + "` was found to be of non constant value");
            } else if(expr.utype->getType() == utype_literal) {
                Literal* literal = ((Literal*)expr.utype->getResolvedType());

                if(literal->literalType == numeric_literal) {
                    if (!isWholeNumber(literal->numericData)) {
                        errors->createNewError(GENERIC, ast,
                                               "enum value must evaluate to an integer, the constant variable's value you have derived resolves to a decimal");
                    }

                    enumValue = (long)literal->numericData + 1;
                } else
                    errors->createNewError(GENERIC, ast,
                                           " strings are not allowed to be assigned to enums");
            } else if(expr.utype->getType() == utype_method) {
                Method* method = ((Method*)expr.utype->getResolvedType());
                enumValue = (long)method->address + 1;
            } else
                errors->createNewError(GENERIC, ast,
                                       "enum value must evaluate to an integer, the expression assigned to enum `" + enumField->name + "` was found to be non-numeric");
        } else
            errors->createNewError(GENERIC, ast,
                                   "enum value must evaluate to an integer, the expression assigned to enum `" + enumField->name + "` was found to be non-numeric");

        RESTORE_BLOCK_TYPE()
    } else {
        enumValue++;
    }

    double value = enumValue - 1;
    if(value > INT32_MAX) {
        long constantAddress = constantMap.addIfIndex(value);

        if(constantAddress >= CONSTANT_LIMIT) {
            stringstream err;
            err << "maximum constant limit of (" << CONSTANT_LIMIT << ") reached.";
            errors->createNewError(INTERNAL_ERROR, ast, err.str());
        }

        code.addIr(OpBuilder::ldc(EBX, constantAddress));
    } else
        code.addIr(OpBuilder::movi(value, EBX));
}

void Compiler::assignEnumFieldValue(Field *enumField) {
    Field *valueField = ((ClassObject*)enumField->utype->getResolvedType())->getField("ordinal", true);

    if(valueField != NULL) {
        getEnumValue(staticMainInserts, enumField);
        staticMainInserts.addIr(OpBuilder::movg(enumField->owner->address))
            .addIr(OpBuilder::movn(enumField->address))
            .addIr(OpBuilder::movn(valueField->address))
            .addIr(OpBuilder::movi(0, ADX))
            .addIr(OpBuilder::rmov(ADX, EBX));
    } else
        errors->createNewError(INTERNAL_ERROR, enumField->ast,
                         " enum class field `ordinal` could not be located");
}

void Compiler::compileEnumField(Field *enumField) {

    // instantiate field
    staticMainInserts.addIr(OpBuilder::newClass(enumField->utype->getClass() ? enumField->utype->getClass()->address : invalidAddr))
            .addIr(OpBuilder::movg(enumField->owner->address))
            .addIr(OpBuilder::movn(enumField->address))
            .addIr(OpBuilder::popObject());
    assignEnumFieldName(enumField);
    assignEnumFieldValue(enumField);
}

void Compiler::postProcessEnumFields() {
    for(Int i = 0; i < enums.size(); i++) {
        ClassObject *enumClass = enums.get(i);
        for(Int j = 0; j < enumClass->fieldCount(); j++) {
            enumClass->getField(j)->address = enumClass->getFieldAddress(enumClass->getField(j));
        }
    }
}

void Compiler::compileEnumFields() {
    for(Int i = 0; i < enums.size(); i++) {
        ClassObject *enumClass = enums.get(i);
        current = getParserBySourceFile(enumClass->meta.file->name);
        updateErrorManagerInstance(current);
        enumValue = guid++;
        currScope.add(new Scope(enumClass, CLASS_SCOPE));
        for(Int j = 0; j < enumClass->fieldCount(); j++) {
            compileEnumField(enumClass->getField(j));
        }

        removeScope();
    }
}

void Compiler::initStaticClassInstance(CodeHolder &code, ClassObject *klass) {

    List<Method *> functions;
    klass->getAllFunctionsByTypeAndName(fn_normal, INTERNAL_STATIC_INIT_FUNCTION, false, functions);

    if(!functions.empty()) {
        functions.get(0)->data.code.addIr(OpBuilder::ret(NO_ERR));
    }


    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        initStaticClassInstance(code, childClasses.get(i));
    }
}

void Compiler::setupMainMethod(Ast *ast) {
    string starterClass = "platform";
    string starterMethod = "srt_init";

    ClassObject* StarterClass = resolveClass(Obfuscater::getModule("platform.kernel"), starterClass, ast);
    if(StarterClass != NULL && mainMethod != NULL) {
        List<Field*> params;
        List<Method*> resolvedMethods;
        List<AccessFlag> flags;
        Meta meta;
        Field *arg0 = new Field(OBJECT, guid++, "args", StarterClass, flags, meta, stl_stack, 0);
        arg0->isArray = true;
        arg0->utype = new Utype();
        arg0->utype->copy(objectUtype);
        params.add(arg0);

        StarterClass->getAllFunctionsByTypeAndName(fn_normal, starterMethod, false, resolvedMethods);

        if(resolvedMethods.empty()) {
            errors->createNewError(GENERIC, ast, "could not locate main method '" + starterMethod + "(object[])' in starter class");
        }else if(resolvedMethods.size() > 1) { // shouldn't happen
            errors->createNewError(GENERIC, ast, "attempting to find main method '" + starterMethod + "(object[])' in starter class, but it was found to be ambiguous");
        } else {
            Method *main = resolvedMethods.last(), *StaticInit, *TlsSetup;
            if(!main->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast, "main method '" + starterMethod + "(object[])' must be static");
            }

            if(!main->flags.find(PRIVATE)) {
                errors->createNewError(GENERIC, ast, "main method '" + starterMethod + "(object[])' must be private");
            }

            if(!main->utype->equals(varUtype)) {
                errors->createNewError(GENERIC, ast, "main method '" + starterMethod + "(object[])' must  return a var");
            }

            delete params.last()->utype;
            delete params.last();
            params.free();

            StarterClass->getAllFunctionsByTypeAndName(fn_normal, staticInitMethod, false, resolvedMethods);
            if(resolvedMethods.empty()) {
                errors->createNewError(GENERIC, ast, "could not locate runtime environment setup method '" + staticInitMethod + "()' in starter class");
                return;
            } else StaticInit = resolvedMethods.last();

            StarterClass->getAllFunctionsByTypeAndName(fn_normal, tlsSetupMethod, false, resolvedMethods);
            if(resolvedMethods.empty()) {
                errors->createNewError(GENERIC, ast, "could not locate thread local storage init method '" + starterMethod + "()' in starter class");
                return;
            } else TlsSetup = resolvedMethods.last();

            if(!StaticInit->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast, "runtime environment setup method '" + staticInitMethod + "()' must be static");
            }
            if(!TlsSetup->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast, "thread local storage init method '" + tlsSetupMethod + "()' must be static");
            }

            if(!StaticInit->utype->equals(nilUtype)) {
                errors->createNewError(GENERIC, ast, "runtime environment setup method '" + staticInitMethod + "()' must return `nil`");
            }
            if(!TlsSetup->utype->equals(nilUtype)) {
                errors->createNewError(GENERIC, ast, "thread local storage init method '" + tlsSetupMethod + "()' must return `nil`");
            }

            if(!StaticInit->flags.find(PRIVATE)) {
                errors->createNewError(GENERIC, ast, "runtime environment setup method '" + staticInitMethod + "()' must be private");
            }

            if(!TlsSetup->flags.find(PUBLIC)) {
                errors->createNewError(GENERIC, ast, "thread local storage init method '" + tlsSetupMethod + "()' must be public");
            }

            Field *userMain;
            switch(mainSignature) {
                case 0: { // fn main(string[]) : var;
                    userMain = StarterClass->getField("main", false);
                    break;
                }
                case 1: { // fn main2(string[]);
                    userMain = StarterClass->getField("main2", false);
                    break;
                }
                case 2: { // fn main3();
                    userMain = StarterClass->getField("main3", false);
                    break;
                }
                case 3: { // fn main4() var;
                    userMain = StarterClass->getField("main4", false);
                    break;
                }
            }

            if(userMain != NULL) {
                if(!userMain->flags.find(STATIC)) {
                    errors->createNewError(GENERIC, ast, "main method function pointer field '" + userMain->toString() + "' must be static");
                }

                if(!userMain->flags.find(PRIVATE)) {
                    errors->createNewError(GENERIC, ast, "main method function pointer field '" + userMain->toString() + "' must be ptivate");
                }

                if(userMain->type != FNPTR) {
                    errors->createNewError(GENERIC, ast, "main method function pointer field '" + userMain->toString() + "' must be a function pointer");
                }

                staticMainInserts.addIr(OpBuilder::movg(StarterClass->address))
                     .addIr(OpBuilder::movn(userMain->address))
                     .addIr(OpBuilder::movi(0, ADX))
                     .addIr(OpBuilder::movi(mainMethod->address, EBX)) // set main address
                     .addIr(OpBuilder::rmov(ADX, EBX));
            } else
                errors->createNewError(INTERNAL_ERROR, ast, "user main method function pointer was not found");

            for(Int i = 0; i < classes.size(); i++) {
                initStaticClassInstance(StaticInit->data.code, classes.get(i));
            }

            StaticInit->data.lineTable.add(
                    LineData(0, StaticInit->ast->line));
            TlsSetup->data.lineTable.add(
                    LineData(0, TlsSetup->ast->line));

            for(Int i = 0; i < initFuncs.size(); i++) {
                staticMainInserts.addIr(OpBuilder::call(initFuncs.get(i)->address));
            }

            StaticInit->data.code.inject(staticMainInserts);
            StaticInit->data.code.addIr(OpBuilder::ret(NO_ERR));

            TlsSetup->data.code.inject(tlsMainInserts);
            TlsSetup->data.code.addIr(OpBuilder::ret(NO_ERR));

            mainMethod = main; // reset main method to __srt_init()
            staticMainInserts.free();
            tlsMainInserts.free();
        }
    } else if(StarterClass == NULL) {
        errors->createNewError(GENERIC, ast, "Could not find starter class '" + starterClass + "' for application entry point.");
    } else if(mainMethod == NULL) {
        errors->createNewError(GENERIC, ast, "could not locate main method 'main(string[])'");
    }
}

void Compiler::validateCoreClasses(Ast *ast) {
    if(resolveClass(Obfuscater::getModule("std"), "_enum_", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate enum support class `_enum_`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "string", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate string support class `string`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "throwable", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `throwable`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "runtime_exception", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `runtime_exception`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "stack_overflow_exception", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `stack_overflow_exception`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "thread_stack_exception", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `thread_stack_exception`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "out_of_bounds_exception", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `out_of_bounds_exception`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "nullptr_exception", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `nullptr_exception`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "class_cast_exception", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `class_cast_exception`.");
    }

    if(resolveClass(Obfuscater::getModule("std"), "out_of_memory_exception", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `out_of_memory_exception`.");
    }

    if(resolveClass(Obfuscater::getModule("platform.kernel"), "stack_state", ast) == NULL) {
        errors->createNewError(GENERIC, ast, "Could not locate exception support class `stack_state`.");
    }
}

void Compiler::compileUnprocessedClasses() {
    for(long long i = 0; i < unProcessedClasses.size(); i++) {
        ClassObject *unprocessedClass = unProcessedClasses.get(i);

        if(unprocessedClass->isNot(compiled)) {
            currModule = unprocessedClass->module;
            current = getParserBySourceFile(unprocessedClass->getGenericOwner()->meta.file->name);

            long long totalErrors = errors->getUnfilteredErrorCount();
            updateErrorManagerInstance(current);

            // bring the classes up to speed
            unprocessedClass->setProcessStage(compiled);
            compileClassObfuscations(unprocessedClass->ast, unprocessedClass);
            compileClassFields(unprocessedClass->ast, unprocessedClass);
            compileClassInitDecls(unprocessedClass->ast, unprocessedClass);
            compileClassMethods(unprocessedClass->ast, unprocessedClass);

            if (NEW_ERRORS_FOUND()) {
                failedParsers.addif(current);
                printNote(unprocessedClass->meta, "in generic `" + unprocessedClass->name + "`");
            }
        }
    }

    unProcessedClasses.free();
}

void Compiler::compileAllUnprocessedMethods() {
    for(Int i = 0; i < unProcessedMethods.size(); i++) {
        Method *method = unProcessedMethods.get(i);
        current = getParserBySourceFile(method->meta.file->name);
        updateErrorManagerInstance(current);

        currModule = method->module;
        currScope.add(new Scope(method->owner, GLOBAL_SCOPE));
        if(method->name == "long") {
            int itt = 3000;
        }
        compileMethod(method->ast, method);
        removeScope();
    }
    unProcessedMethods.free();
}

void Compiler::compile() {
    setup();
    randomizeGUID();

    if(preprocess() && postProcess()) {
        processingStage = COMPILING;
        compileAllObfuscationRules();
        compileAllFields();
        compileAllInitDecls();
        compileAllMethods();
        compileAllUnprocessedMethods();
        compileEnumFields();
        compileUnprocessedClasses();

        // after processing procedures
        updateErrorManagerInstance(parsers.get(0));
        setupMainMethod(parsers.get(0)->astAt(0));
        validateCoreClasses(parsers.get(0)->astAt(0));
    }
}

string Compiler::parseModuleDecl(Ast *ast) {
    string module;
    for(long i = 0; i < ast->getTokenCount(); i++)
    {
        module += ast->getToken(i).getValue();
    }
    return module;
}

ClassObject* Compiler::findClass(ModuleData* mod, string className, List<ClassObject*> &classes, bool match) {
    ClassObject* klass = NULL;
    bool found;
    for(unsigned int i = 0; i < classes.size(); i++) {
        klass = classes.get(i);
        if(match) found = klass->name.find(className) != string::npos;
        else
            found = klass->name == className;

        if(found) {
            found = false;
            if(mod != NULL && klass->module == mod)
                return klass;
            else if(mod == NULL)
                return klass;
        }
    }

    return NULL;
}

void Compiler::printNote(Meta& meta, string msg) {
    if(lastNoteMsg != msg && lastNote.ln != meta.ln
       && !noteMessages.find(msg))
    {
        stringstream note;
        note << "in file: ";
        note << meta.file->name << ":" << meta.ln << ":" << meta.col << ": note:  " << msg
             << endl;

        note << '\t' << '\t' << meta.line << endl << '\t' << '\t';

        for(int i = 0; i < meta.col-1; i++)
            note << " ";
        note << "^" << endl;

        cout << note.str();
        lastNoteMsg = msg;
        noteMessages.push_back(msg);
    }
}

ClassObject* Compiler::addGlobalClassObject(string name, List<AccessFlag>& flags, Ast *ast, List<ClassObject*> &classList) {
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    ClassObject* k = new ClassObject(name, currModule, guid++, flags, meta), *prevClass;
    if((prevClass = findClass(currModule, name, classList))
        || (prevClass = findClass(currModule, name + "<>", classList)) != NULL){
        prevDefined:
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                                                                                "` is already defined in module {" + currModule->name + "}");

        printNote(prevClass->meta, "class `" + prevClass->fullName + "` previously defined here");
        k->free();
        delete k;
        return !ends_with(prevClass->name, "<>") ? prevClass : NULL;
    } else {
        if(ends_with(k->name, "<>") && (prevClass = findClass(currModule, k->name.substr(0, k->name.size() - 2), classes)) != NULL)
            goto prevDefined;

        k->owner = findClass(currModule, globalClass, classes);
        if(k->owner != NULL)
            k->owner->addClass(k);
        k->ast = ast;
        classList.add(k);
        return k;
    }
}

bool Compiler::simpleParameterMatch(List<Field*> &params, List<Field*> &comparator) {
    if(params.size() == comparator.size()) {
        for(long i = 0; i < params.size(); i++) {
            if(!params.get(i)->equals(*comparator.get(i)))
                return false;
        }

        return true;
    }

    return false;
}

bool Compiler::complexParameterMatch(List<Field*> &params, List<Field*> &comparator, bool nativeClassSupport) {
    if(params.size() == comparator.size()) {
        for(long i = 0; i < params.size(); i++) {
            if(!params.get(i)->isRelated(*comparator.get(i))) {
                if(nativeClassSupport) {
                    if(!isUtypeConvertableToNativeClass(params.get(i)->utype, comparator.get(i)->utype)
                       && !isUtypeConvertableToNativeClass(comparator.get(i)->utype, params.get(i)->utype))
                    return false;
                } else
                    return false;
            }
        }

        return true;
    }

    return false;
}

void Compiler::addDefaultConstructor(ClassObject* klass, Ast* ast) {
    List<Field*> emptyParams;
    List<AccessFlag> flags;
    flags.add(PUBLIC);
    string functionName = klass->name;

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line),
              Obfuscater::getFile(current->getTokenizer()->file), ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);
    if(IS_CLASS_GENERIC(klass->getClassType())) {
        functionName = klass->getGenericOwner()->name.substr(0, klass->getGenericOwner()->name.size() - 2); // k<> -> k
    }

    if(findFunction(klass, functionName, emptyParams, ast, false, fn_constructor, false) == NULL) {
        Method* method = new Method(functionName, currModule, klass, guid++, emptyParams, flags, meta);
        string basicName = klass->name;
        if(basicName.find("<") != string::npos) {
            stringstream ss;
            for(int i = 0; i < basicName.size(); i++) {
                if(basicName[i] == '<')
                    break;
                ss << basicName[i];
            }

            basicName = ss.str();
        }

        method->fullName = klass->fullName + "." + basicName;
        method->ast = ast;
        method->fnType = fn_constructor;
        method->address = methodSize++;
        method->utype = nilUtype;
        klass->addFunction(method);
    }
}

void Compiler::createNewWarning(error_type error, int type, Ast* ast, string xcmnts) {
    createNewWarning(error, type, ast->line, ast->col, xcmnts);
}

void Compiler::createNewWarning(error_type error, int type, int line, int col, string xcmnts) {
    if(warning_map[__WGENERAL] && !warnings.find(xcmnts)) {
        if(warning_map[type]) {
            if(c_options.werrors){
                errors->createNewError(error, line, col, xcmnts);
            } else {
                warnings.add(xcmnts);
                errors->createNewWarning(error, line, col, xcmnts);
            }
        }
    }
}

void Compiler::createGlobalClass() {
    List<AccessFlag > flags;
    flags.add(PUBLIC);
    flags.add(EXTENSION);
    ClassObject * global = findClass(currModule, globalClass, classes);
    if(global == NULL) {
        global = addGlobalClassObject(globalClass, flags, NULL, classes);
        global->address = classSize++;
        global->setGlobalClass(true);
        stringstream ss;
        ss << currModule->name << "#" << global->name;
        global->fullName = ss.str();
        addDefaultConstructor(global, NULL);
    }

    flags.free();
}

bool
Compiler::addFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &)) {
    List<Method*> funcs;
    k->getAllFunctionsByName(method->name, funcs);

    if(!funcs.empty()) {
        for(long long i = 0; i < funcs.size(); i++) {
            if(paramaterMatchFun(funcs.get(i)->params, method->params)) {
                funcs.free();
                return false;
            }
        }
    }

    k->addFunction(method);
    funcs.free();
    return true;
}

bool Compiler::resolveExtensionFunctions(ClassObject *unprocessedClass) {
    if(unprocessedClass->getGenericOwner()) {

        currScope.add(new Scope(unprocessedClass, CLASS_SCOPE));
        List<Ast *> &extFunTree = unprocessedClass->getGenericOwner()->getExtensionFunctionTree();
        RETAIN_TYPE_INFERENCE(false)
        for (long i = unprocessedClass->processedExtFunctions; i < extFunTree.size(); i++) {
            resolveMethod(extFunTree.get(i), unprocessedClass);
            unprocessedClass->processedExtFunctions++;
        }
        RESTORE_TYPE_INFERENCE()
        removeScope();
    }
    return true;
}

void Compiler::resolveGenericFieldMutations(ClassObject *unprocessedClass) {
    if(unprocessedClass->getGenericOwner())
    {
        currScope.add(new Scope(unprocessedClass, CLASS_SCOPE));
        List<Ast*> &mutationTree = unprocessedClass->getGenericOwner()->getClassMutations();
        RETAIN_TYPE_INFERENCE(false)
        // we mainly used processedMutations counter for methods because they aren't as magical as fields are
        for(long z = 0; z < mutationTree.size(); z++) {
            resolveClassFields(mutationTree.get(z), unprocessedClass);
            inlineClassFields(mutationTree.get(z), unprocessedClass);
        }
        RESTORE_TYPE_INFERENCE()
        removeScope();
    }
}

Method*
Compiler::findFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &)) {
    List<Method*> funcs;
    if(k->getGenericOwner() && k->processedExtFunctions < k->getGenericOwner()->getExtensionFunctionTree().size())
        resolveExtensionFunctions(k);
    k->getAllFunctionsByName(method->name, funcs);

    if(!funcs.empty()) {
        for(long long i = 0; i < funcs.size(); i++) {
            if(paramaterMatchFun(funcs.get(i)->params, method->params)) {
                Method *fn = funcs.get(i);
                funcs.free();
                return fn;
            }
        }
    }

    funcs.free();
    return NULL;
}

void Compiler::updateErrorManagerInstance(parser *parser) {
    if(errors == NULL)
        errors = new ErrorManager(&parser->getTokenizer()->getLines(), parser->getTokenizer()->file, true, c_options.aggressive_errors);
    else errors->update(parser, true, c_options.aggressive_errors);
}

Field* Compiler::createLocalField(string name, Utype *type, bool isArray, StorageLocality locality, List<AccessFlag> flags,
                           Int scopeLevel, Ast *ast) {
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), Obfuscater::getFile(current->getTokenizer()->file),
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    Field* prevField=NULL;
    if((prevField = currentScope()->currentFunction->data.getLocalField(name)) != NULL) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, prevField->ast->line, prevField->ast->col,
                                     "local field `" + name + "` is already defined in the scope");
        printNote(prevField->meta, "local field `" + name + "` previously defined here");
        return prevField;
    } else {
        if((prevField = currentScope()->currentFunction->data.getLocalField(name)) != NULL) {
            createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                             "local field `" + name + "` shadows another field at higher scope");
        }

        Field *local = new Field(type == NULL ? UNTYPED : type->getResolvedType()->type, guid++, name, NULL, flags, meta, locality, checkstl(locality));
        currentScope()->currentFunction->data.locals.add(local);
        local->ast = ast;
        local->local = true;
        local->utype = type;
        local->isArray = isArray;
        local->owner = currentScope()->klass;
        local->scopeLevel = scopeLevel;
        if(!(local->flags.find(flg_CONST) && local->type <= VAR))
            local->address = currentScope()->currentFunction->data.localVariablesSize++;
        local->fullName = "[local: " + name + "]";

        if(local->address >= LOCAL_FIELD_LIMIT) {
            stringstream err;
            err << "maximum amount of local fields reached (" << LOCAL_FIELD_LIMIT << ").";
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
        }
        return local;
    }
}

/*
 * hash code
 *
 * long long compute_hash(string const& s) {
    const int p = 31;
    const int m = 1e9 + 9;
    long long hash_value = 0;
    long long p_pow = 1;
    for (char c : s) {
        hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash_value;
}
 */