//
// Created by BraxtonN on 10/18/2019.
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
#include "../../runtime/oo/Method.h"
#include "oo/Method.h"

string globalClass = "__srt_global";
string undefinedModule = "__$srt_undefined";
uInt Compiler::guid = 0;
void Compiler::generate() {

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
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
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

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
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
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
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

        fullName << currModule << "#" << className;
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
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
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
    flags.addif(STABLE);
    flags.addif(EXTENSION);
    string className = ast->getToken(0).getValue();

    if(globalScope()) {
        currentClass = addGlobalClassObject(className, flags, ast, classes);
        if(currentClass == NULL) return; // obviously the uer did something really dumb

        stringstream ss;
        ss << currModule << "#" << currentClass->name;
        currentClass->fullName = ss.str();
    }
    else {
        currentClass = addChildClassObject(className, flags, currentScope()->klass, ast);
        if(currentClass == NULL) return; // obviously the uer did something really dumb

        stringstream ss;
        ss << currentScope()->klass->fullName << "." << currentClass->name;
        currentClass->fullName = ss.str();
    }

    enums.addif(currentClass);
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
        constructor->data.locals.addAll(constructor->params);
        constructor->data.localVariablesSize = constructor->params.size() + 1; // +1 because we need space for the instance
    }

    constructors.free();
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
            ss << currModule << "#" << currentClass->name;
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
    currScope.pop_back();
}

void Compiler::preProccessImportDecl(Ast *branch, List<string> &imports) {
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
    if(star) {
        bool found = false;
        if(modules.find(baseMod))
            imports.addif(baseMod);
        for(long i = 0; i < modules.size(); i++) {
            if(startsWith(modules.get(i), mod)) {
                found = true;
                imports.addif(modules.at(i));
            }
        }

        if(!found) {
            errors->createNewError(GENERIC, branch->line, branch->col, "modules under prefix `" + mod +
                                                                       "*` could not be found");
        }

    } else {
        if(modules.find(mod)) {
            if(!imports.addif(mod)) {
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
    List<string> imports;
    imports.add("std"); // import the std lib by default

    for(unsigned long x = 0; x < current->size(); x++)
    {
        Ast *branch = current->astAt(x);
        if(x == 0)
        {
            if(branch->getType() == ast_module_decl) {
                imports.push_back(currModule = parseModuleDecl(branch));
                continue;
            } else {
                imports.push_back(currModule = undefinedModule);
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


    importMap.__new().key = current->getTokenizer()->file;
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
        if(scope->klass->meta.file == field->meta.file) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to localized field `" + field->fullName + "`");
        }
    } else if(field->flags.find(PRIVATE)) {
        if(field->owner == currentScope()->klass) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private field `" + field->fullName + "`");
        }
    } else if(field->flags.find(PROTECTED)) {
        if(currentScope()->klass->isClassRelated(field->owner) || field->owner == currentScope()->klass
                || (field->flags.find(STATIC) && field->owner->isGlobalClass() && field->module == currentScope()->klass->module)) {
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
        if(function->owner == scope->klass) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private method `" + function->toString() + "`");
        }
    } else if(function->flags.find(PROTECTED)) {
        Scope *scope = currentScope();
        if(scope->klass->isClassRelated(function->owner) || function->owner == scope->klass
           || (function->flags.find(STATIC) && function->owner->isGlobalClass() && function->module == currentScope()->klass->module)) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected method `" + function->toString() + "`");
        }
    } else {
        // access granted
    }
}

ClassObject* Compiler::resolveClass(string mod, string name, Ast* pAst) {
    ClassObject* klass = NULL;
    if(!mod.empty()) {
        if((klass = findClass(mod, name, classes)) != NULL) {
            validateAccess(klass, pAst);
            return klass;
        } else return NULL;
    } else {
        resolve:
        for (unsigned int i = 0; i < importMap.size(); i++) {
            if (importMap.get(i).key == current->getTokenizer()->file) {

                List<string> &lst = importMap.get(i).value;
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

bool Compiler::resolveClass(List<ClassObject*> &classes, List<ClassObject*> &results, string mod, string name, Ast* pAst, bool match) {
    ClassObject* klass = NULL;
    if(!mod.empty() && (klass = findClass(mod, name, classes, match)) != NULL) {
        validateAccess(klass, pAst);
        results.add(klass);
    } else {
        for (unsigned int i = 0; i < importMap.size(); i++) {
            if (importMap.get(i).key == current->getTokenizer()->file) {

                List<string> &lst = importMap.get(i).value;
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
        ClassObject *base = findClass("std", "_object_", classes);

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
        ClassObject *base = findClass("std", "_enum_", classes);

        if (base != NULL && (IS_CLASS_ENUM(base->getClassType()) || IS_CLASS_INTERFACE(base->getClassType()))) {
            stringstream err;
            err << "support class for enums must be of type class";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
        } else {
            if (base != NULL)
                enumClass->setSuperClass(base);
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

    double value;
    string int_string = invalidateUnderscores(token.getValue());

    if(isAllIntegers(int_string)) {
        value = std::strtod (int_string.c_str(), NULL);
        if(value > INT32_MAX) {
            long constantAddress = constantIntMap.addIfIndex(value);
            if(constantAddress >= CONSTANT_LIMIT) {
                stringstream err;
                err << "maximum large constant limit of (" << CONSTANT_LIMIT << ") reached.";
                errors->createNewError(INTERNAL_ERROR, token, err.str());
            }

            expr->utype->getCode().addIr(OpBuilder::ldc(EBX, constantAddress));
        } else
            expr->utype->getCode().addIr(OpBuilder::movi(value, EBX));
    }else {
        value = std::strtof (int_string.c_str(), NULL);
        long floatingPointAddress = floatingPointMap.addIfIndex(value);
        if(floatingPointAddress >= FLOATING_POINT_LIMIT) {
            stringstream err;
            err << "maximum floating point limit of (" << FLOATING_POINT_LIMIT << ") reached.";
            errors->createNewError(INTERNAL_ERROR, token, err.str());
        }

        expr->utype->getCode().addIr(OpBuilder::movf(EBX, floatingPointAddress));
    }

    expr->utype->getCode().getInjector(stackInjector)
            .addIr(OpBuilder::rstore(EBX));
    expr->utype->setResolvedType(new Literal(value));
}

void Compiler::parseHexLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);

    double value;
    string hex_string = invalidateUnderscores(token.getValue());

    value = strtoll(hex_string.c_str(), NULL, 16);
    if(value > INT32_MAX) {
        long constantAddress = constantIntMap.addIfIndex(value);
        if(constantAddress >= CONSTANT_LIMIT) {
            stringstream err;
            err << "maximum large constant limit of (" << CONSTANT_LIMIT << ") reached.";
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

    expr->ast = ast;
}

void Compiler::compileUtypeClass(Expression* expr, Ast* ast) {
    RETAIN_TYPE_INFERENCE(false)
    Utype* utype = compileUtype(ast->getSubAst(ast_utype));
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
        errors->createNewError(GENERIC, ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->line,
                               ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->col, "expected class");
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
        param->utype->getCode().inject(0, expression.utype->getCode());
        param->utype->getCode().getInjector(stackInjector).inject(0, expression.utype->getCode().getInjector(stackInjector));
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
        param->utype->getCode().inject(0, expression.utype->getCode());
        param->utype->getCode().getInjector(stackInjector).inject(0, expression.utype->getCode().getInjector(stackInjector));
        freePtr(expression.utype);
    }
    else if(expression.type == exp_var && expression.utype->getType() == utype_method) {
        param->utype = new Utype(VAR);
        param->type = VAR;

        // yet another line of nasty dirty code...smh...
        param->utype->getCode().inject(0, expression.utype->getCode());
        param->utype->getCode().getInjector(stackInjector).inject(0, expression.utype->getCode().getInjector(stackInjector));
        freePtr(expression.utype);
    }
    else if(expression.utype->getType() == utype_field) {
        auto *exprField = (Field*)expression.utype->getResolvedType();
        param->utype = new Utype();
        param->utype->copy(exprField->utype);
        param->type = exprField->type;

        // yet another line of nasty dirty code...smh...
        param->utype->getCode().inject(0, expression.utype->getCode());
        param->utype->getCode().getInjector(stackInjector).inject(0, expression.utype->getCode().getInjector(stackInjector));
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
Compiler::findFunction(ClassObject *k, string name, List<Field*> &params, Ast* ast, bool checkBase, function_type type) {
    List<Method*> funcs;
    List<Method*> matches;
    Method* resolvedFunction;

    if(k->getGenericOwner() && k->processedExtFunctions < k->getGenericOwner()->getExtensionFunctionTree().size())
        resolveExtensionFunctions(k);
    if(type == fn_undefined)
        k->getFunctionByName(name, funcs, checkBase);
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

        if(matches.empty()) {
            for (long long i = 0; i < funcs.size(); i++) {
                if (complexParameterMatch(funcs.get(i)->params, params)) {
                    matches.add(funcs.get(i));
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

Alias *Compiler::resolveAlias(string mod, string name, Ast *ast) {
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
    resolveClass(classes, globalClasses, "", globalClass, ast);
    Field *resolvedField;

    for(long i = 0; i < globalClasses.size(); i++) {

        if((resolvedField = globalClasses.get(i)->getField(name, false)) != NULL)
            return resolvedField;
    }

    return NULL;
}

Method *Compiler::resolveFunction(string name, List<Field*> &params, Ast *ast) {
    List<ClassObject*> globalClasses;
    resolveClass(classes, globalClasses, "", globalClass, ast);
    Method *resolvedMethod;

    for(long i = 0; i < globalClasses.size(); i++) {
        if((resolvedMethod = findFunction(globalClasses.get(i), name, params, ast, false)) != NULL)
            return resolvedMethod;
    }

    return NULL;
}

bool Compiler::resolveFunctionByName(string name, List<Method*> &functions, Ast *ast) {
    List<ClassObject*> globalClasses;
    resolveClass(classes, globalClasses, "", globalClass, ast);
    Method *resolvedMethod;

    for(long i = 0; i < globalClasses.size(); i++) {

        globalClasses.get(i)->getFunctionByName(name, functions);
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
        ClassObject *global = findClass(ptr.mod, globalClass, classes);
        if(currentScope()->type == RESTRICTED_INSTANCE_BLOCK && !currentScope()->klass->isClassRelated(global))
            global = NULL;

        if(global != NULL && (resolvedMethod = findFunction(global, name, params, ast)) != NULL) {
            return resolvedMethod;
        } else if(global != NULL && (resolvedMethod = findGetterSetter(global, name, params, ast)) != NULL) {
            return resolvedMethod;
        }
    } else {

        // TODO: check for overloads later i need to see how it comes in
        if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedMethod = resolveFunction(name, params, ast)) != NULL) {
            return resolvedMethod;
        } else if((resolvedMethod = findGetterSetter(currentScope()->klass, name, params, ast)) != NULL) {
            return resolvedMethod;
        }
        else if((resolvedMethod = findFunction(currentScope()->klass, name, params, ast, true)) != NULL)
            return resolvedMethod;
        else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && currentScope()->currentFunction != NULL
            && currentScope()->currentFunction->data.getLocalFieldHereOrHigher(name, currentScope()->scopeLevel) != NULL) {
            Field* field = currentScope()->currentFunction->data.getLocalFieldHereOrHigher(name, currentScope()->scopeLevel);
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

            // TODO: add getter sett here
            // TODO: replace with function resolveFieldUtype()
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
        }
    }

    errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + ptr.classes.get(0) +  Method::paramsToString(params) + "`");
    return NULL;
}

bool Compiler::isUtypeClass(Utype* utype, string mod, int names, ...) {
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

    // this seems weird but this check dosent check
    if(isLambdaFullyQualified(lambdaFn)) {
        if(lambdaFn->utype == NULL)
            lambdaFn->utype = qualifier->utype;
    } else {
        if(lambdaFn->utype == NULL)
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

    compileTypeIdentifier(ptr, ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier));
    compileExpressionList(expressions, ast->getSubAst(ast_expression_list));
    expressionsToParams(expressions, params);

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
            if((resolvedMethod = findGetterSetter(klass, name, params, ast)) != NULL) {}
            else if(klass != NULL && (resolvedMethod = findFunction(klass, name, params, ast, true)) != NULL) {
                compileMethodReturnType(resolvedMethod, resolvedMethod->ast);
                if(!resolvedMethod->flags.find(STATIC)) {
                    utype->getCode().inject(ptrInjector);
                    utype->getCode().addIr(OpBuilder::pushObject());
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
            } else
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " could not resolve function `" + name + "`");

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
        if(!resolvedMethod->flags.find(STATIC) && resolvedMethod->fnType != fn_ptr) {
            if(singularCall) {
                code.addIr(OpBuilder::movl(0));
                code.addIr(OpBuilder::pushObject());
            }
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
        if(isLambdaUtype(methodParam->utype))
            fullyQualifyLambda(params.get(i)->utype, methodParam->utype);

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
            code.addIr(OpBuilder::loadValue(EBX))
                    .addIr(OpBuilder::calld(EBX));
        }
    }
}

/*
 * DEBUG: This is a debug function made to print the generated contents of a n expression's code
 */
void Compiler::printExpressionCode(Expression &expr) {
    cout << "\n\n\n==== Expression Code ===\n";
    cout << "line " << expr.ast->line << " file " << current->getTokenizer()->file << endl;
    cout << codeToString(expr.utype->getCode());
    cout << "\n=========\n" << std::flush;
}

void Compiler::printMethodCode(Method &func, Ast *ast) {
    cout << "\n\n\n==== Expression Code ===\n";
    cout << "line " << ast->line << " file " << current->getTokenizer()->file << endl;
    cout << codeToString(func.data.code, &func.data);
    cout << "\n=========\n" << std::flush;
}

/*
 * DEBUG: This is a debug function made to get the name of a register
 */
string Compiler::registerToString(int64_t r) {
    switch(r) {
        case ADX:
            return "adx";
        case CX:
            return "cx";
        case CMT:
            return "cmt";
        case EBX:
            return "ebx";
        case ECX:
            return "ecx";
        case ECF:
            return "ecf";
        case EDF:
            return "edf";
        case EHF:
            return "ehf";
        case BMR:
            return "bmr";
        case EGX:
            return "egx";
        default: {
            stringstream ss;
            ss << "? (" << r << ")";
            return ss.str();
        }
    }
}

/*
 * DEBUG: This is a debug function made to find a class for better code readability
 */
string Compiler::find_class(int64_t id) {
    for(unsigned int i = 0; i < classes.size(); i++) {
        if(classes.get(i)->address == id)
            return classes.get(i)->fullName;
        else {
            ClassObject &klass = *classes.get(i);
            for(unsigned int x = 0; x < klass.getChildClasses().size(); x++) {
                if(klass.getChildClasses().get(x)->address == id)
                    return klass.getChildClasses().get(x)->fullName;
            }
        }
    }
    return "";
}

/*
 * DEBUG: This is a debug function made to allow early testing in compiler code generation
 */
string Compiler::codeToString(CodeHolder &code, CodeData *data) {
    stringstream ss, endData;
    for(unsigned int x = 0; x < code.size(); x++) {
        opcode_instr opcodeData=code.ir32.get(x);
        ss << x << ": ";

        if(data != NULL) {
            // TODO: put code info here
        }

        switch(GET_OP(opcodeData)) {
            case Opcode::ILL:
            {
                ss<<"ill ";

                break;
            }
            case Opcode::NOP:
            {
                ss<<"nop";
                
                break;
            }
            case Opcode::INT:
            {
                ss<<"int 0x" << std::hex << GET_Da(opcodeData);
                
                break;
            }
            case Opcode::MOVI:
            {
                ss << "movi #" << code.ir32.get(x+1)  << ", ";
                ss<< registerToString(GET_Da(opcodeData)) ;
                x++;
                break;
            }
            case Opcode::RET:
            {
                ss<<"ret";
                
                break;
            }
            case Opcode::HLT:
            {
                ss<<"hlt";
                
                break;
            }
            case Opcode::NEWARRAY:
            {
                ss<<"newarry ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::CAST:
            {
                ss<<"cast ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::VARCAST:
            {
                ss<<"vcast ";
                ss<< GET_Ca(opcodeData);
                ss << " -> " << (GET_Cb(opcodeData) == 1 ? "[]" : "");
                
                break;
            }
            case Opcode::MOV8:
            {
                ss<<"mov8 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::MOV16:
            {
                ss<<"mov16 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::MOV32:
            {
                ss<<"mov32 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::MOV64:
            {
                ss<<"mov64 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            } case Opcode::MOVU8:
            {
                ss<<"movu8 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::MOVU16:
            {
                ss<<"movu16 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::MOVU32:
            {
                ss<<"movu32 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::MOVU64:
            {
                ss<<"movu64 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::RSTORE:
            {
                ss<<"rstore ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::ADD:
            {
                ss<<"add ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));
                
                break;
            }
            case Opcode::SUB:
            {
                ss<<"sub ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));
                
                break;
            }
            case Opcode::MUL:
            {
                ss<<"mul ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));
                
                break;
            }
            case Opcode::DIV:
            {
                ss<<"div ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));
                
                break;
            }
            case Opcode::MOD:
            {
                ss<<"mod ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));
                
                break;
            }
            case Opcode::IADD:
            {
                ss<<"iadd ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));
                
                break;
            }
            case Opcode::ISUB:
            {
                ss<<"isub ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));
                
                break;
            }
            case Opcode::IMUL:
            {
                ss<<"imul ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));
                
                break;
            }
            case Opcode::IDIV:
            {
                ss<<"idiv ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));
                
                break;
            }
            case Opcode::IMOD:
            {
                ss<<"imod ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));
                
                break;
            }
            case Opcode::POP:
            {
                ss<<"pop";
                
                break;
            }
            case Opcode::INC:
            {
                ss<<"inc ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::DEC:
            {
                ss<<"dec ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::MOVR:
            {
                ss<<"movr ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::IALOAD:
            {
                ss<<"iaload ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::BRH:
            {
                ss<<"brh";
                
                break;
            }
            case Opcode::IFE:
            {
                ss<<"ife";
                
                break;
            }
            case Opcode::IFNE:
            {
                ss<<"ifne";
                
                break;
            }
            case Opcode::JE: // TODO: continue here
            {
                ss<<"je " << GET_Da(opcodeData);
                
                break;
            }
            case Opcode::JNE:
            {
                ss<<"jne " << GET_Da(opcodeData);
                
                break;
            }
            case Opcode::LT:
            {
                ss<<"lt ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::GT:
            {
                ss<<"gt ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::LTE:
            {
                ss<<"lte ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::GTE:
            {
                ss<<"gte ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::MOVL:
            {
                ss<<"movl " << GET_Da(opcodeData);
                
                break;
            }
            case Opcode::POPL:
            {
                ss<<"popl " << GET_Da(opcodeData);
                
                break;
            }
            case Opcode::MOVSL:
            {
                ss<<"movsl #";
                ss<< GET_Da(opcodeData);
                
                break;
            }
            case Opcode::MOVF:
            {
                ss << "movf #" << GET_Ca(opcodeData) << ", #";
                ss<< floatingPointMap.get(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::SIZEOF:
            {
                ss<<"sizeof ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::PUT:
            {
                ss<<"put ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::PUTC:
            {
                ss<<"_putc ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::CHECKLEN:
            {
                ss<<"chklen ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::JMP:
            {
                ss<<"jmp @" << GET_Da(opcodeData);
                
                break;
            }
            case Opcode::LOADPC:
            {
                ss<<"loadpc ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::PUSHOBJ:
            {
                ss<<"pushobj";
                
                break;
            }
            case Opcode::DEL:
            {
                ss<<"del";
                
                break;
            }
            case Opcode::CALL:
            {
                ss << "call @" << GET_Da(opcodeData) << " // <";
                //ss << find_method(GET_Da(x64)) << ">";
                
                break;
            }
            case Opcode::CALLD:
            {
                ss<<"calld ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::NEWCLASS:
            {
                ss<<"new_class @" << GET_Da(opcodeData);
                ss << " // "; ss << find_class(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::MOVN:
            {
                ss<<"movn #" << code.ir32.get(++x);
                
                break;
            }
            case Opcode::SLEEP:
            {
                ss<<"sleep ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::TEST:
            {
                ss<<"test ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::TNE:
            {
                ss<<"tne ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::LOCK:
            {
                ss<<"_lck ";
                
                break;
            }
            case Opcode::ULOCK:
            {
                ss<<"_ulck";
                
                break;
            }
            case Opcode::MOVG:
            {
                ss<<"movg @"<< GET_Da(opcodeData);
                ss << " // @"; ss << find_class(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::MOVND:
            {
                ss<<"movnd ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::NEWOBJARRAY:
            {
                ss<<"newobj_arry ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::NOT: //c
            {
                ss<<"not ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::SKIP:// d
            {
                ss<<"skip @";
                ss<< GET_Da(opcodeData);
                ss << " // pc = " << (x + GET_Da(opcodeData));
                
                break;
            }
            case Opcode::LOADVAL:
            {
                ss<<"loadval ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::SHL:
            {
                ss<<"shl ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));
                
                break;
            }
            case Opcode::SHR:
            {
                ss<<"shr ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));
                
                break;
            }
            case Opcode::SKPE:
            {
                ss<<"skpe ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss << " // pc = " << (x + GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::SKNE:
            {
                ss<<"skne ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss << " // pc = " << (x + GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::CMP:
            {
                ss<<"cmp ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", ";
                ss<< code.ir32.get(++x);
                
                break;
            }
            case Opcode::AND:
            {
                ss<<"and ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::UAND:
            {
                ss<<"uand ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::OR:
            {
                ss<<"or ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::XOR:
            {
                ss<<"xor ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::THROW:
            {
                ss<<"throw ";
                
                break;
            }
            case Opcode::CHECKNULL:
            {
                ss<<"checknull";
                
                break;
            }
            case Opcode::RETURNOBJ:
            {
                ss<<"returnobj";
                
                break;
            }
            case Opcode::NEWCLASSARRAY:
            {
                ss<<"new_classarray ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< " ";
                ss << " // "; ss << find_class(GET_Cb(opcodeData)) << "[]";
                
                break;
            }
            case Opcode::NEWSTRING:
            {
                ss << "newstr @" << GET_Da(opcodeData) << " // ";
                //ss << getString(GET_Da(x64));
                
                break;
            }
            case Opcode::ADDL:
            {
                ss<<"addl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::SUBL:
            {
                ss<<"subl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::MULL:
            {
                ss<<"mull ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::DIVL:
            {
                ss<<"divl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::MODL:
            {
                ss<<"modl #";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::IADDL:
            {
                ss<<"iaddl ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;
                
                break;
            }
            case Opcode::ISUBL:
            {
                ss<<"isubl ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;
                
                break;
            }
            case Opcode::IMULL:
            {
                ss<<"imull ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;
                
                break;
            }
            case Opcode::IDIVL:
            {
                ss<<"idivl ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;
                
                break;
            }
            case Opcode::IMODL:
            {
                ss<<"imodl ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;
                
                break;
            }
            case Opcode::LOADL:
            {
                ss<<"loadl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp+";
                ss<<GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::POPOBJ:
            {
                ss<<"popobj";
                
                break;
            }
            case Opcode::SMOVR:
            {
                ss<<"smovr ";
                ss << registerToString(GET_Ca(opcodeData)) << ", sp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";
                
                break;
            }
            case Opcode::SMOVR_2:
            {
                ss<<"smovr_2 ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";
                
                break;
            }
            case Opcode::ANDL:
            {
                ss<<"andl ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::ORL:
            {
                ss<<"orl ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::XORL:
            {
                ss<<"xorl ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);
                
                break;
            }
            case Opcode::RMOV:
            {
                ss<<"rmov ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                
                break;
            }
            case Opcode::SMOV:
            {
                ss<<"smov ";
                ss << registerToString(GET_Ca(opcodeData)) << ", sp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";
                
                break;
            }
            case Opcode::RETURNVAL:
            {
                ss<<"return_val ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::ISTORE:
            {
                ss<<"istore ";
                ss<< code.ir32.get(++x);
                
                break;
            }
            case Opcode::ISTOREL:
            {
                ss<<"istorel ";
                ss << code.ir32.get(++x) << ", fp+";
                ss<<GET_Da(opcodeData);
                
                break;
            }
            case Opcode::IPUSHL:
            {
                ss<<"ipushl #";
                ss<< GET_Da(opcodeData);
                
                break;
            }
            case Opcode::PUSHL:
            {
                ss<<"pushl ";
                ss<< GET_Da(opcodeData);
                
                break;
            }
            case Opcode::PUSHNULL:
            {
                ss<<"pushnull ";
                
                break;
            }
            case Opcode::GET:
            {
                ss<<"get ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::ITEST:
            {
                ss<<"itest ";
                ss<< registerToString(GET_Da(opcodeData));
                
                break;
            }
            case Opcode::INVOKE_DELEGATE:
            {
                ss<<"invoke_delegate ";
                ss<< GET_Da(opcodeData);
                ss<< " { static=";
                ss<< GET_Ca(code.ir32.get(x+1))
                    << ", args=" << GET_Cb(code.ir32.get(x+1)) <<  " }";
                x++;
                
                break;
            }
            case Opcode::ISADD:
            {
                ss<<"isadd ";
                ss << GET_Ca(opcodeData) << ", sp+";
                if(code.ir32.get(x+1) < 0) ss << "[";
                ss<<code.ir32.get(x+1);
                if(code.ir32.get(x+1) < 0) ss << "]";
                x++;

                break;
            }
            case Opcode::IPOPL:
            {
                ss<<"ipopl ";
                ss<< GET_Da(opcodeData);
                
                break;
            }
            case Opcode::SWITCH:
            {
                ss<<"switch ";
                ss<< code.ir32.get(x+1);
                //ss<< " // " << getSwitchTable(method, GET_Da(x64));
                
                break;
            }
            case Opcode::TLS_MOVL:
            {
                ss<<"tls_movl ";
                ss<< GET_Da(opcodeData);
                
                break;
            }
            case Opcode::DUP:
            {
                ss<<"dup ";
                
                break;
            }
            case Opcode::POPOBJ_2:
            {
                ss<<"popobj2 ";
                
                break;
            }
            case Opcode::SWAP:
            {
                ss<<"swap ";
                
                break;
            }
            case Opcode::SMOVR_3:
            {
                ss<<"smovr_3";
                ss << " fp+";
                if(GET_Da(opcodeData) < 0) ss << "[";
                ss<<GET_Da(opcodeData);
                if(GET_Da(opcodeData) < 0) ss << "]";

                break;
            }
            default:
                ss << "? (" << GET_OP(opcodeData) << ")";
                
                break;
        }


        ss << endData.str(); endData.str("");
        ss << "\n";
    }

    return ss.str();
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
    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
              ast->line, ast->col);

    params.add(new Field(paramUtype->getResolvedType()->type, 0, "", currentScope()->klass, flags, meta, stl_stack, 0));
    params.get(0)->utype = paramUtype;
    params.get(0)->isArray = paramUtype->isArray();

    if((constructor = clazz->getClass()->getConstructor(params, true)) != NULL) {
        validateAccess(constructor, ast);

        code.addIr(OpBuilder::newClass(clazz->getClass()->address));

        paramUtype->getCode().inject(stackInjector);
        code.inject(paramUtype->getCode());
        code.addIr(OpBuilder::call(constructor->address));

        code.freeInjectors();
        if (constructor->utype->isArray() || constructor->utype->getResolvedType()->type == OBJECT
            || constructor->utype->getResolvedType()->type == CLASS) {
            code.getInjector(ptrInjector)
                    .addIr(OpBuilder::popObject2());

            code.getInjector(removeFromStackInjector)
                    .addIr(OpBuilder::pop());
        } else if (constructor->utype->getResolvedType()->isVar()) {
            code.getInjector(ebxInjector)
                    .addIr(OpBuilder::loadValue(EBX));

            code.getInjector(removeFromStackInjector)
                    .addIr(OpBuilder::pop());
        }
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

        if (isUtypeClass(clazz, "std", 10, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong", "uint")) {
            return !dest->isArray() && (type <= VAR);
        }
    }

    return false;
}

bool Compiler::isUtypeConvertableToNativeClass(Utype *dest, Utype *src) {
    if(dest->getResolvedType() && src->getResolvedType()) {
        DataType type = src->getResolvedType()->type;

        if (isUtypeClass(dest, "std", 1, "string") && type == _INT8 && src->isArray()) {
            return true;
        } else if (isUtypeClass(dest, "std", 10, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong", "uint")) {
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
    } else { // utype_field
        Field *field = (Field*)leftExpr.utype->getResolvedType();

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
                    long constantAddress = constantIntMap.addIfIndex(result);
                    if(constantAddress >= CONSTANT_LIMIT) {
                        stringstream err;
                        err << "maximum large constant limit of (" << CONSTANT_LIMIT << ") reached.";
                        errors->createNewError(INTERNAL_ERROR, leftExpr.ast, err.str());
                    }

                    resultCode->addIr(OpBuilder::ldc(EBX, constantAddress));
                } else
                    resultCode->addIr(OpBuilder::movi(result, EBX));

            } else {
                long floatingPointAddress = floatingPointMap.addIfIndex(result);
                if(floatingPointAddress >= FLOATING_POINT_LIMIT) {
                    stringstream err;
                    err << "maximum floating point limit of (" << FLOATING_POINT_LIMIT << ") reached.";
                    errors->createNewError(INTERNAL_ERROR, leftExpr.ast, err.str());
                }
                resultCode->addIr(OpBuilder::movf(EBX, floatingPointMap.indexof(result)));
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

                Utype *stringClass = new Utype(resolveClass("std", "string", ast));
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
            if((rightLiteral != NULL && rightLiteral->literalType == string_literal) && rightExpr.type != exp_var
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
                expr->utype->setResolvedType(new DataEntity(VAR));
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
                expr->utype->setResolvedType(new DataEntity(VAR));

                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::rstore(EBX));
            }
        }
    } else if(leftExpr.utype->getType() == utype_field) {
        Field *field = (Field*)leftExpr.utype->getResolvedType();

        if(field->utype->getType() == utype_class) {
            goto processClassOverload;
        } else if(field->utype->getResolvedType()->type <= VAR) {
            goto processSimpleValue;
        }  else if(field->utype->getResolvedType()->type == OBJECT && (operand == "==" || operand == "!=")) {
            goto instanceTest;
        } else
            goto incompatibleExpressions;
    } else if(leftExpr.utype->getType() == utype_native) {
        if(leftExpr.utype->getResolvedType()->type <= VAR) {
            goto processSimpleValue;
        } else if(leftExpr.utype->getResolvedType()->type == OBJECT && (operand == "==" || operand == "!=")) {
            goto instanceTest;
        } else
            goto incompatibleExpressions;
    } else if((operand == "==" || operand == "!=") && leftExpr.utype->getResolvedType()->type == OBJECT) {
        instanceTest:
        if(rightExpr.utype->getResolvedType()->type == OBJECT || rightExpr.utype->getClass() != NULL) {
            resultCode->inject(leftExpr.utype->getCode());
            resultCode->inject(leftExpr.utype->getCode().getInjector(stackInjector));
            resultCode->inject(rightExpr.utype->getCode());
            resultCode->inject(rightExpr.utype->getCode().getInjector(ptrInjector));
            resultCode->addIr(OpBuilder::itest(EBX));

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
                    resultCode->inject(params.get(0)->utype->getCode().getInjector(ptrInjector));
                    resultCode->addIr(OpBuilder::itest(EBX));

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
                                       leftExpr.utype->toString()
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
}

void Compiler::assignValue(Expression* expr, Token &operand, Expression &leftExpr, Expression &rightExpr, Ast* ast, bool allowOverloading) {
    CodeHolder *resultCode = &expr->utype->getCode();

    if(leftExpr.utype->getType() == utype_field) {
        Field *field = (Field*)leftExpr.utype->getResolvedType();

        if(field->setter != NULL
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
        } else {
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

            if (rightExpr.utype->isRelated(leftExpr.utype)
                || ((field->type <= _UINT64 || field->type == VAR) && isUtypeClassConvertableToVar(field->utype, rightExpr.utype))
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
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " incompatible types, cannot convert `" + rightExpr.utype->toString() +
                                       "` to `" +
                                       leftExpr.utype->toString()
                                       + "`" + (!allowOverloading ? ", has the variable been initialized yet?" : "."));
            }
        }
    } else { // utype_class

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

void Compiler::compileBinaryExpression(Expression* expr, Ast* ast) {
    Expression leftExpr, rightExpr;

    RETAIN_TYPE_INFERENCE(true)
    compileExpression(&leftExpr, ast->getSubAst(0));
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

    RETAIN_TYPE_INFERENCE(true)
    compileExpressionAst(&leftExpr, ast->getSubAst(ast_primary_expr));
    compileExpression(&rightExpr, ast->getSubAst(ast_expression));
    RESTORE_TYPE_INFERENCE()

    expr->ast = ast;
    Token &operand = ast->getToken(0);

    switch(leftExpr.utype->getType()) {
        case utype_class:
        case utype_field: {
            if(operand == "=")
                assignValue(expr, operand, leftExpr, rightExpr, ast);
            else compoundAssignValue(expr, operand, leftExpr, rightExpr, ast);
            break;
        }
        default: {
            errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + leftExpr.utype->toString() + "` cannot be assigned a value");
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

            if((constr = findFunction(arrayType->getClass(), arrayType->getClass()->name, params, ast, false, fn_constructor)) == NULL) {
                errors->createNewError(GENERIC, ast->line, ast->col, "class `" + arrayType->toString() + "` does not contain constructor `"
                                                                     + arrayType->getClass()->name + Method::paramsToString(params) + "`");
            }

            if(constr != NULL) {
                validateAccess(constr, ast);
                expr->utype->getCode().addIr(OpBuilder::newClass(expr->utype->getClass()->address));

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

                expr->utype->getCode().getInjector(ptrInjector)
                        .addIr(OpBuilder::movsl(1));
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
    expr->utype = nullUtype;

    expr->utype->getCode()
            .addIr(OpBuilder::pushNull());

    expr->utype->getCode().getInjector(ptrInjector)
            .addIr(OpBuilder::popObject2());
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
        freePtr(utype);
    }

    compilePostAstExpressions(expr, ast);
}

/*
 * Proccess elements after an expression in the format of `[]`, `.<identifier>`, or `++`
 */
void Compiler::compilePostAstExpressions(Expression *expr, Ast *ast, long startPos) {

    expr->utype->getCode().instanceCaptured = true;
    RETAIN_SCOPE_CLASS(expr->utype->getClass() != NULL ? expr->utype->getClass() : currentScope()->klass)
    if(ast->getSubAstCount() > 1) {
        for(long i = startPos; i < ast->getSubAstCount(); i++) {
            if(ast->getSubAst(i)->getType() == ast_post_inc_e)
                compilePostIncExpression(expr, false, ast->getSubAst(i));
            else if(ast->getSubAst(i)->getType() == ast_cast_e)
                compileCastExpression(expr, false, ast->getSubAst(i));
            else if(ast->getSubAst(i)->getType() == ast_dot_not_e || ast->getSubAst(i)->getType() == ast_dotnotation_call_expr) {
                if(expr->utype->getClass()) {

                    RETAIN_BLOCK_TYPE(RESTRICTED_INSTANCE_BLOCK)
                    Ast *astToCompile = ast->getSubAst(i)->getType() == ast_dot_not_e ? ast->getSubAst(i)->getSubAst(0)
                                                                                      : ast->getSubAst(i);
                    Expression bridgeExpr;
                    bridgeExpr.utype->getCode().instanceCaptured = true;
                    bridgeExpr.utype->getCode().inject(expr->utype->getCode().getInjector(ptrInjector));
                    compileDotNotationCall(&bridgeExpr, astToCompile);

                    expr->copy(&bridgeExpr);
                    expr->utype->getCode().inject(bridgeExpr.utype->getCode());
                    expr->utype->getCode().copyInjectors(bridgeExpr.utype->getCode());
                    RESTORE_BLOCK_TYPE()
                } else
                    errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "expression of type `" + expr->utype->toString()
                                                                                                     + "` must resolve as a class");
            } else if(ast->getSubAst(i)->getType() == ast_arry_e)
                compileArrayExpression(expr, ast->getSubAst(i));
            else {
                errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "unexpected expression of type `" + Ast::astTypeToString(ast->getSubAst(i)->getType()) + "`");
            }

        }
    }
    RESTORE_SCOPE_CLASS()
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
            } else if (isUtypeClass(castExpr->utype, "std", 9, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong")) {
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
                            .addIr(OpBuilder::varCast(1, true));

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
        expr->utype->setArrayType(false);

        compileExpression(&arrayExpr, ast->getSubAst(ast_expression));

        expr->utype->getCode().inject(expr->utype->getCode().getInjector(stackInjector));
        expr->utype->getCode().inject(arrayExpr.utype->getCode());
        expr->utype->getCode().inject(arrayExpr.utype->getCode().getInjector(ebxInjector));

        if(arrayExpr.type == exp_var) {
            expr->utype->getCode()
                    .addIr(OpBuilder::popObject2())
                    .addIr(OpBuilder::movr(ADX, EBX))
                    .addIr(OpBuilder::checklen(ADX))
                    .addIr(OpBuilder::movnd(ADX));

            expr->utype->getCode().freeInjectors();
            if(expr->utype->getResolvedType()->isVar()) {
                expr->utype->getCode().getInjector(ebxInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::iaload(EBX, ADX));

                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::movi(0, ADX))
                        .addIr(OpBuilder::iaload(EBX, ADX))
                        .addIr(OpBuilder::rstore(EBX));
            } else if(expr->utype->getResolvedType()->type == OBJECT || expr->utype->getResolvedType()->type == CLASS) {

                expr->utype->getCode().getInjector(stackInjector)
                        .addIr(OpBuilder::pushObject());
            } else
                errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + expr->utype->toString() + "` must be a var[], object[], or class[]");
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "index expression of type `" + arrayExpr.utype->toString() + "` was not found to be a var");
    } else { // TODO: chech if utype is class and add support for operator[] overload
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
                expr->utype->getCode().inject(expr->utype->getCode().getInjector(ebxInjector));
                expr->utype->getCode().addIr(OpBuilder::neg(EBX, EBX));

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
                if (tok == "++")
                    expr->utype->getCode().addIr(OpBuilder::inc(EBX));
                else
                    expr->utype->getCode().addIr(OpBuilder::dec(EBX));

                if (expr->utype->getResolvedType()->type <= _UINT64) {
                    dataTypeToOpcode(expr->utype->getResolvedType()->type, EBX, EBX, expr->utype->getCode());
                }

                if(expr->utype->getType() == utype_field) {
                    Field *field = (Field*)expr->utype->getResolvedType();

                    if(field->local) {
                        expr->utype->getCode().addIr(OpBuilder::smovr2(EBX, field->address));
                    } else {
                        // we know that Ptr has to still be set because we had to pull the ebx value from it
                        // this is very dangerous but we have to assume or assumptions are correct
                        expr->utype->getCode()
                                .addIr(OpBuilder::movi(0, ADX)) // I'm pretty sure adx is already set to 0 but just in-case
                                .addIr(OpBuilder::rmov(ADX, EBX));
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

        Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
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

                if(expr->utype->getType() == utype_field) {
                    Field *field = (Field*)expr->utype->getResolvedType();
                    expr->utype->getCode().addIr(OpBuilder::movr(CMT, EBX));

                    if (tok == "++")
                        expr->utype->getCode().addIr(OpBuilder::inc(CMT));
                    else
                        expr->utype->getCode().addIr(OpBuilder::dec(CMT));

                    if (field->type <= _UINT64) {
                        dataTypeToOpcode(expr->utype->getResolvedType()->type, CMT, CMT, expr->utype->getCode());
                    }

                    if(field->local) {
                        expr->utype->getCode().addIr(OpBuilder::smovr2(CMT, field->address));
                    } else {
                        // we know that Ptr has to still be set because we had to pull the ebx value from it
                        // this is very dangerous but we have to assume or assumptions are correct
                        expr->utype->getCode()
                                .addIr(OpBuilder::movi(0, ADX)) // I'm pretty sure adx is already set to 0 but just in-case
                                .addIr(OpBuilder::rmov(ADX, CMT));
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

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
              ast->line, ast->col);

    Field *field = new Field(UNTYPED, guid++, name, findClass(currModule, globalClass, classes), fieldFlags, meta, stl_stack, 0);

    if(utype) {
        if(utype->getResolvedType() != NULL) {
            field->type = utype->getResolvedType()->type;
            field->utype = utype;
            field->isArray = utype->isArray();
        } else
            field->type = UNDEFINED;
    } else
        field->type = ANY;

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

        Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
                  ast->line, ast->col);

        stringstream ss;
        ss << "anon_func#" << methodSize;
        string name = ss.str();

        lambda = new Method(name, currModule, findClass(currModule, globalClass, classes), fields, flags, meta);
        lambda->fullName = findClass(currModule, globalClass, classes)->fullName + "." + name;
        lambda->ast = ast;
        lambda->fnType = fn_lambda;
        lambda->type = FNPTR; // lambda's are technically methos but we treat thm as pointers at high level
        lambda->address = methodSize++;

        compileMethodReturnType(lambda, ast);
        lambdas.add(lambda);

        if(processingStage > POST_PROCESSING) {
            // Todo:...
        } else {
            unProcessedMethods.add(lambda);
        }
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
    } else if(sizeExpr.utype->getType() == utype_class || sizeExpr.utype->getType() == utype_field){
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
    switch(branch->getType()) {
        case ast_primary_expr:
            return compilePrimaryExpression(expr, branch);
        case ast_pre_inc_e:
            return compilePreIncExpression(expr, branch);
        case ast_not_e:
            return compileNotExpression(expr, branch);
        case ast_vect_e:
            return compileVectorExpression(expr, branch);
        case ast_assign_e:
            return compileAssignExpression(expr, branch);
        case ast_minus_e:
            return compileMinusExpression(expr, branch);
        case ast_and_e:
        case ast_shift_e:
        case ast_equal_e:
        case ast_less_e:
        case ast_mult_e:
        case ast_add_e:
            return compileBinaryExpression(expr, branch);
    }

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
    } else {
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
    } else if(currentScope()->klass->getFunctionByName(name, functions)) {
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
            RETAIN_BLOCK_TYPE(field->local ? INSTANCE_BLOCK : RESTRICTED_INSTANCE_BLOCK)
            RETAIN_TYPE_INFERENCE(true)

            Token operand("=", SINGLE, ast->col, ast->line, EQUALS);
            Expression leftExpr, rightExpr, resultExpr;
            ReferencePointer fieldReference;
            resultExpr.ast = ast;
            leftExpr.ast = ast;

            fieldReference.classes.add(field->name);
            resolveUtype(fieldReference, leftExpr.utype, ast);

            leftExpr.type = utypeToExpressionType(leftExpr.utype);
            compileExpression(&rightExpr, ast->getSubAst(ast_expression));

            assignValue(&resultExpr, operand, leftExpr, rightExpr, ast, false);

            if(field->local) {
                currentScope()->currentFunction->data.code.inject(resultExpr.utype->getCode());
            } else {
                if (field->flags.find(STATIC)) {
                    staticMainInserts.inject(resultExpr.utype->getCode());
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
                if(xtraField->locality == stl_stack)
                    xtraField->address = xtraField->owner->getFieldAddress(xtraField);

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
    params.add(field);

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
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
    Method *method = new Method(fnName, currModule, currentScope()->klass, params, flags, meta);
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

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
              ast->line, ast->col);

    if(field->flags.find(flg_CONST)) {
        createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "field `" + field->name + "` has access specifier `const`. It is not recommended to add getters to constant fields, "
                                                                                                          "as the compiler is no longer able to inline the value for better performance.");
    }

    string fnName = "get_" + field->name;
    Method *method = new Method(fnName, currModule, currentScope()->klass, params, flags, meta);
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
    }

    flags.free();
    params.free();
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

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
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
        ClassObject* stringClass = findClass("std", "string", classes);

        if(stringClass != NULL) {
            List<Field*> params;
            List<AccessFlag> flags;
            Meta meta;
            Field *arg0 = new Field(CLASS, guid++, "args", method->owner, flags, meta, stl_stack, 0);
            arg0->isArray = true;
            arg0->utype = new Utype(stringClass, true);
            params.add(arg0);

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

    // TODO: when operator[] is supported only store `[` char so that we dont break this function
    List<Field*> params;
    string name = ast->getToken(0).getValue() + ast->getToken(1).getValue();
    string op = ast->getToken(1).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
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

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_constructor;
    method->address = methodSize++;
    method->utype = new Utype(currentScope()->klass);

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

void Compiler::resolveDelegateDecl(Ast* ast) {
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

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_delegate;
    method->address = delegateGUID++;

    if(ast->hasSubAst(ast_method_return_type)) {
        if(ast->hasToken("nil"))
            goto void_;

        Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
        method->utype = utype;
    } else {
        void_:
        method->utype = nilUtype;
    }

    if(!addFunction(currentScope()->klass, method, &simpleParameterMatch)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "function `" + name + "` is already defined in the scope");
        printNote(findFunction(currentScope()->klass, method, &simpleParameterMatch)->meta, "function `" + name + "` previously defined here");

        method->free();
        delete method;
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
                Utype *utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
                RESTORE_SCOPE_CLASS()
                RESTORE_TYPE_INFERENCE()

                if (utype->getType() == utype_class) {
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
                compileExpression(&e, ast->getSubAst(ast_expression));
                RESTORE_SCOPE_CLASS()
                RESTORE_TYPE_INFERENCE()

                if(e.utype->getType() == utype_field) {
                    fun->utype = ((Field*)e.utype->getResolvedType())->utype;
                } else if(e.utype->getType() == utype_class) {
                    fun->utype = e.utype;
                } else if(e.utype->getType() == utype_method) {
                    fun->utype = new Utype(VAR);
                } else if(e.utype->getType() == utype_native) {
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
                return resolveClass(ptr.mod, globalClass, ast);
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

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentClass, params, flags, meta);
    method->fullName = currentClass->fullName + "." + name;
    method->ast = ast;
    if(ast->getType() == ast_delegate_decl) {
        if(method->owner->isGlobalClass())
            this->errors->createNewError(GENERIC, ast->line, ast->col,
                                         "delegate functions are not allowed at global scope");

        method->fnType = fn_delegate;
        method->address = delegateGUID++;
    } else {
        method->fnType = fn_normal;
        method->address = methodSize++;
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
                        sub->delegateAddr = contract->address;
                    }
                }
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

        if(contracter->getSuperClass() != NULL && !(contracter->getSuperClass()->name == "_object_" && contracter->getSuperClass()->module =="std")) {
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

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if(branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if(branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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

    for(unsigned int i = 0; i < inlinedStringFields.size(); i++) {
        if(inlinedStringFields.get(i).key == field)
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

Int Compiler::getInlinedStringFieldAddress(Field* field) {
    for(unsigned int i = 0; i < inlinedStringFields.size(); i++) {
        if(inlinedStringFields.get(i).key==field)
            return inlinedStringFields.get(i).value;
    }

    return invalidAddr;
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

        if (isWholeNumber(value)) {
            if (value > INT32_MAX) {
                code.addIr(OpBuilder::ldc(EBX, constantIntMap.indexof(value)));
            } else
                code.addIr(OpBuilder::movi(value, EBX));
        } else {
            code.addIr(OpBuilder::movf(EBX, floatingPointMap.indexof(value)));
        }

        code.getInjector(stackInjector)
                .addIr(OpBuilder::rstore(EBX));
    } else {
        code.free();

        code.addIr(OpBuilder::newString(getInlinedStringFieldAddress(field)));

        code.getInjector(ptrInjector)
                .addIr(OpBuilder::popObject2());
        code.getInjector(removeFromStackInjector)
                .addIr(OpBuilder::pop());
    }
}

Field *Compiler::resolveEnum(string name) {
    Field* field;

    for (unsigned int i = 0; i < importMap.size(); i++) {
        if (importMap.get(i).key == current->getTokenizer()->file) {

            List<string> &lst = importMap.get(i).value;
            for (unsigned int x = 0; x < lst.size(); x++) {
                for(long j = 0; j < enums.size(); j++) {
                    ClassObject *enumClass = enums.get(j);
                    if(enumClass->module == lst.get(x)) {

                        if(enumClass->owner == NULL || (enumClass->owner != NULL && enumClass->owner->fullName == currentScope()->klass->fullName)) {
                            if ((field = enumClass->getField(name, false)) != NULL)
                                return field;
                        }
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
            } else if(resolveExtensionFunctions(currentClass) && currentClass->getFunctionByName(name, functions, true)) {
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

    if(parser::isOverrideOperator(name)) {
        name = "operator" + name;
    }

    if(ptr.mod != "")
        goto globalCheck;

    if(currentScope()->type >= INSTANCE_BLOCK && currentScope()->currentFunction != NULL
        && currentScope()->currentFunction->data.getLocalFieldHereOrHigher(name, currentScope()->scopeLevel) != NULL) {
        Field* field = currentScope()->currentFunction->data.getLocalFieldHereOrHigher(name, currentScope()->scopeLevel);

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);
        utype->getCode().instanceCaptured = true;

        if(field->getter == NULL && isFieldInlined(field)) {
            inlineVariableValue(utype->getCode(), field);
        } else {
            if (field->isVar()) {
                if (field->isArray || field->locality == stl_thread) {
                    if (field->locality == stl_thread) {
                        utype->getCode().addIr(OpBuilder::tlsMovl(field->address));

                        // we only do it for stl_thread var if it is not an array field
                        if (!field->isArray) {
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
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveClass("", name, ast)) != NULL) {
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
        validateAccess((ClassObject*)resolvedUtype, ast);
        checkTypeInference(ast);
    } else if((resolvedUtype = currentScope()->klass->getChildClass(name)) != NULL
        && (IS_CLASS_GENERIC(((ClassObject*)resolvedUtype)->getClassType())
            && ((ClassObject*)resolvedUtype)->getGenericOwner() !=  NULL)) {
        resolveClassUtype(utype, ast, resolvedUtype);
    }  else if((resolvedUtype = currentScope()->klass->getChildClass(name)) != NULL && !IS_CLASS_GENERIC(((ClassObject*)resolvedUtype)->getClassType())) {
        resolveClassUtype(utype, ast, resolvedUtype);
    } else if(resolveExtensionFunctions(currentScope()->klass) && currentScope()->klass->getFunctionByName(name, functions, true)) {
        resolveFunctionByNameUtype(utype, ast, name, functions); // TODO: add a "requiredSignature" flag in the scope to help searching for methods
    } else if((resolvedUtype = currentScope()->klass->getAlias(name, currentScope()->type != RESTRICTED_INSTANCE_BLOCK)) != NULL) {
        resolveAliasUtype(utype, ast, resolvedUtype);
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && resolveHigherScopeSingularUtype(ptr, utype, ast)) {}
    else {
        globalCheck:
        functions.free();

        if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveClass(ptr.mod, name, ast)) != NULL) {
            return resolveClassUtype(utype, ast, resolvedUtype);
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveField(name, ast)) != NULL) {
            return resolveFieldUtype(utype, ast, resolvedUtype, name);
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && resolveFunctionByName(name, functions, ast)) {
            return resolveFunctionByNameUtype(utype, ast, name, functions);
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveAlias("", name, ast)) != NULL) {
            return resolveAliasUtype(utype, ast, resolvedUtype);
        } else if((currentScope()->type != RESTRICTED_INSTANCE_BLOCK &&
            resolveClass(generics, resolvedClasses, ptr.mod, name + "<>", ast, true))
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
            resolveClass(classes, resolvedClasses, ptr.mod, name, ast, true))
                || currentScope()->klass->getChildClass(name, true)) {
            stringstream helpfulMessage;
            helpfulMessage << "I found a few classes that match `" << name << "`. Were you possibly looking for any of these?\n";
            helpfulMessage << "\t{";

            if(currentScope()->klass->getChildClass(name, true))
                resolvedClasses.add(currentScope()->klass->getChildClass(name + "<>", true));

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
                if (simpleParameterMatch(functions.get(i)->params, requiredSignature->params)) {
                    resolvedFunction = functions.get(i);
                    break;
                }
            }
        } else {
            resolvedFunction = functions.get(0);
            createNewWarning(GENERIC, __WAMBIG, ast->line, ast->col,
                             "reference to function `" + functions.get(0)->name + "` is ambiguous");
        }
    } else
        resolvedFunction = functions.get(0);

    utype->setType(utype_method);
    utype->setResolvedType(resolvedFunction);
    utype->setArrayType(false);
    if(!functions.get(0)->flags.find(STATIC)) {
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
void Compiler::compileAliasType(Alias *alias) {
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

            if(isFieldInlined((Field*)bridgeUtype->getResolvedType()) && lastReference) {
                bridgeUtype->setType(utype_literal);
                bridgeUtype->setArrayType(false);
                bridgeUtype->setResolvedType(new Literal(getInlinedFieldValue((Field*)bridgeUtype->getResolvedType())));
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

    Meta meta(current->getErrors()->getLine(ast->line), current->getTokenizer()->file,
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
        } else if (ptr.classes.singular() && ptr.mod == "" && IS_CLASS_GENERIC(currentScope()->klass->getClassType())
                   && currentScope()->klass->getKeys().find(ptr.classes.get(0)) &&
                   currentScope()->klass->isAtLeast(created)) {

            Utype *keyType = currentScope()->klass->getKeyTypes().get(
                    currentScope()->klass->getKeys().indexof(ptr.classes.get(0)));
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

ClassObject* Compiler::compileGenericClassReference(string &mod, string &name, ClassObject* parent, Ast *ast) {
    List<Utype*> utypes;
    compileUtypeList(ast, utypes);

    for(long i = 0; i < utypes.size(); i++) {
        if(utypes.get(i)->getType() == utype_unresolved) {
            return NULL;
        }
    }

    // TODO: add support for creating generic functions
    ClassObject *generic = parent != NULL ? parent->getChildClass(name + "<>") : findClass(mod, name + "<>", generics);
    if(generic == NULL && parent == NULL) {
        generic = currentScope()->klass->getChildClass(name + "<>");
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
                string old = currModule;
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
            fullName << newClass->module << "#" << name;

            newClass->meta.copy(tmp);
            newClass->fullName = fullName.str();
            newClass->name = name;
            newClass->setProcessStage(created);
            newClass->owner = parent;
            newClass->addKeyTypes(utypes);
            newClass->setGenericOwner(generic);

            if(processingStage >= POST_PROCESSING) {
                newClass->setProcessStage(preprocessed);

                parser* oldParser = current;
                long long totalErrors = errors->getUnfilteredErrorCount();

                current = getParserBySourceFile(errors->filename);
                updateErrorManagerInstance(current);

                // preprocess code
                preProccessClassDecl(newClass->ast, IS_CLASS_INTERFACE(newClass->getClassType()), newClass);
                resolveSuperClass(newClass->ast, newClass);

                inheritEnumClassHelper(newClass->ast, newClass);
                inheritObjectClassHelper(newClass->ast, newClass);
                resolveClassMethods(newClass->ast, newClass);
                resolveExtensionFunctions(newClass);
                resolveUnprocessedClassMutations(newClass);

                if(processingStage > POST_PROCESSING) {
                    // post processing code
                    newClass->setProcessStage(postprocessed);
                    resolveAllDelegates(newClass->ast, newClass);
                    resolveClassFields(newClass->ast, newClass);
                    inlineClassFields(newClass->ast, newClass);
                    resolveGenericFieldMutations(newClass);

                    // TODO: add additional code
                    newClass->setProcessStage(compiled);
                } else // we need to make sure we don't double process he class
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

            return newClass;
        } else
            return actualClass;
    }

    return NULL;
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
                    RETAIN_TYPE_INFERENCE(false) // TODO
                    ClassObject *genericClass = compileGenericClassReference(
                            ptr.mod, name, parent, ast->getSubAst(genericListPos++));
                    ptr.classes.add(genericClass ? genericClass->name : "?");

                    parent = genericClass;
                    failed = parent == NULL;
                    i += 2;
                    RESTORE_TYPE_INFERENCE()
                } else {
                    ptr.classes.add(name);

                    if(!failed) {
                        if (parent == NULL)
                            parent = resolveClass(ptr.mod, name, ast);
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
        ClassObject* klass = resolveClass(ptr.mod, ptr.classes.get(0), ast);
        if(klass == NULL) {
            if((klass = findClass(ptr.mod, ptr.classes.get(0) + "<>", generics))) {
                if(allowGenerics) // this is for extension functions
                    return klass;
                else
                    errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0)
                                                                   + "`. It was found to be a generic class, have you missed the key parameters to the class perhaps?");
            } else if((alias = currentScope()->klass->getAlias(ptr.classes.get(0), true)) != NULL
                || (alias = resolveAlias(ptr.mod, ptr.classes.get(0), ast)) != NULL) {
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
        ClassObject* klass = resolveClass(ptr.mod, ptr.classes.get(0), ast);
        if(klass == NULL) {
            if(findClass(ptr.mod, ptr.classes.get(0) + "<>", generics)) {
                errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0)
                                                               + "`. It was found to be a generic class, have you missed the key parameters to the class perhaps?");
            } else if((alias = currentScope()->klass->getAlias(ptr.classes.get(0), true)) != NULL
                || (alias = resolveAlias(ptr.mod, ptr.classes.get(0), ast)) != NULL) {
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

        for (long i = 0; i < block->getSubAstCount(); i++) {
            branch = block->getSubAst(i);
            CHECK_CMP_ERRORS(return;)

            switch (branch->getType()) {
                case ast_class_decl:
                    resolveSuperClass(branch);
                    break;
                case ast_interface_decl:
                    resolveSuperClass(branch);
                    break;
                default:
                    break;
            }
        }
        removeScope();
    }
}

parser *Compiler::getParserBySourceFile(string name) {
    for(long long i = 0; i < parsers.size(); i++) {
        if(parsers.get(i)->getTokenizer()->file == name)
            return parsers.get(i);
    }
    return NULL;
}

void Compiler::preProcessUnprocessedClasses(long long unstableClasses) {
    // we need this size just incase any classes get added after the fact to prevent double preprocessing
    long long size = unstableClasses;

    for(long long i = 0; i < size; i++) {
        ClassObject *unprocessedClass = unProcessedClasses.get(i);

        currModule = unprocessedClass->module;
        current = getParserBySourceFile(unprocessedClass->meta.file);

        long long totalErrors = errors->getUnfilteredErrorCount();
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

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for(unsigned long x = 0; x < current->size(); x++)
        {
            Ast *branch = current->astAt(x);
            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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

void Compiler::preProcessGenericClasses(long long unstableClasses) {
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
            Expression expr;
            compileExpression(&expr, ast->getSubAst(ast_expression));

            inlineField(field, expr);
        }
    }
}

void Compiler::inlineField(Field *field, Expression &expr) {
    if(expr.utype->isRelated(field->utype)) {
        if (expr.type == exp_var && utypeToExpressionType(field->utype) == exp_var && !field->utype->isArray()) {
            if (expr.utype->getType() == utype_field && isFieldInlined((Field *) expr.utype->getResolvedType())) {
                if (((Field *) expr.utype->getResolvedType())->isVar() &&
                    !((Field *) expr.utype->getResolvedType())->isArray)
                    inlinedFields.add(KeyPair<Field *, double>(field, getInlinedFieldValue(
                            (Field *) expr.utype->getResolvedType())));
                else
                    inlinedStringFields.add(KeyPair<Field *, Int>(field, getInlinedStringFieldAddress(
                            (Field *) expr.utype->getResolvedType())));
            } else if (expr.utype->getType() == utype_literal) {
                Literal *literal = ((Literal *) expr.utype->getResolvedType());

                if (literal->literalType == numeric_literal) {
                    if (!isWholeNumber(literal->numericData)) {
                        errors->createNewError(GENERIC, expr.ast,
                                               "constant value must evaluate to an integer, the constant's value appears to be a decimal");
                    }

                    inlinedFields.add(KeyPair<Field *, double>(field, literal->numericData));
                } else {
                    inlinedStringFields.add(KeyPair<Field *, Int>(field, literal->address));
                }
            } else if (expr.utype->getType() == utype_method) {
                Method *method = ((Method *) expr.utype->getResolvedType());
                inlinedFields.add(KeyPair<Field *, double>(field, method->address));
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
    } else {
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

void Compiler::inlineEnumField(Ast* ast) {
    string name = ast->getToken(0).getValue();
    Field *field = currentScope()->klass->getField(name, false);

    if(!field->inlineCheck) {
        field->inlineCheck = true;

        if(ast->hasSubAst(ast_expression)) {
            BlockType oldScope = currentScope()->type;
            if(field->flags.find(STATIC))
                currentScope()->type = STATIC_BLOCK;
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

                        inlinedFields.add(KeyPair<Field *, double>(field, inlinedValue));
                    } else
                        errors->createNewError(GENERIC, ast,
                                               " The expression being assigned to enum `" + field->name + "` was found to be of non constant value");
                } else if(expr.utype->getType() == utype_literal) {
                    Literal* literal = ((Literal*)expr.utype->getResolvedType());

                    if(literal->literalType == numeric_literal) {
                        if (!isWholeNumber(literal->numericData)) {
                            errors->createNewError(GENERIC, ast,
                                                   "enum value must evaluate to an integer, the constant variable's value you have derived resolves to a decimal");
                        }

                        inlinedFields.add(KeyPair<Field*, double>(field, literal->numericData));
                    } else
                        errors->createNewError(GENERIC, ast,
                                               " strings are not allowed to be assigned to enums");
                } else if(expr.utype->getType() == utype_method) {
                    Method* method = ((Method*)expr.utype->getResolvedType());
                    inlinedFields.add(KeyPair<Field*, double>(field, method->address));
                } else
                    errors->createNewError(GENERIC, ast,
                                           "enum value must evaluate to an integer, the expression assigned to enum `" + field->name + "` was found to be non-numeric");
            } else
                errors->createNewError(GENERIC, ast,
                                       "enum value must evaluate to an integer, the expression assigned to enum `" + field->name + "` was found to be non-numeric");

            if(field->flags.find(STATIC))
                currentScope()->type = oldScope;
        } else {
            inlinedFields.add(KeyPair<Field*, double>(field, guid++));
        }
    }
}

void Compiler::inlineEnumFields(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getSubAst(ast_enum_identifier_list);

    if(currentClass == NULL) {
        string name = ast->getToken(0).getValue();
        if(globalScope()) {
            currentClass = findClass(currModule, name, classes);
        }
        else {
            currentClass = currentScope()->klass->getChildClass(name);
        }
    }

    currScope.add(new Scope(currentClass, CLASS_SCOPE));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        Ast* branch = block->getSubAst(i);
        CHECK_CMP_ERRORS(return;)

        switch(branch->getType()) {
            case ast_enum_identifier:
                inlineEnumField(branch);
                break;
            default:
                /* ignore */
                break;
        }
    }
    removeScope();
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
                case ast_enum_decl:
                    inlineEnumFields(branch);
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

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if(branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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
                case ast_enum_decl:
                    inlineEnumFields(branch);
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
    for(long long i = 0; i < unProcessedClasses.size(); i++) {
        ClassObject *unprocessedClass = unProcessedClasses.get(i);

        currModule = unprocessedClass->module;
        current = getParserBySourceFile(unprocessedClass->meta.file);

        long long totalErrors = errors->getUnfilteredErrorCount();
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
    long long unstableClasses = unProcessedClasses.size();

    preprocessMutations();
    preProcessGenericClasses(unstableClasses);
    resolveAllMethods();
    resolveAllDelegates();
    resolveAllFields();
    inlineFields();
    postProcessGenericClasses();

    return !errors->hasErrors();
}

void Compiler::handleImports() {

    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        if(c_options.magic)
        { // import everything in magic mode
            importMap.__new().key = current->getTokenizer()->file;
            importMap.last().value.init();
            importMap.last().value.addAllUnique(modules);
        } else
            preproccessImports();


        if(NEW_ERRORS_FOUND()){
            failedParsers.addif(current);
        }
    }
}

bool Compiler::preprocess() {
    bool success = true;
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for(unsigned long x = 0; x < current->size(); x++)
        {
            Ast *branch = current->astAt(x);
            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    modules.addif(currModule = parseModuleDecl(branch));
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    modules.addif(currModule = undefinedModule);
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

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for(unsigned long x = 0; x < current->size(); x++)
        {
            Ast *branch = current->astAt(x);
            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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

void Compiler::setup() {
    modules.init();
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
    floatingPointMap.init();
    constantIntMap.init();
    unProcessedClasses.init();
    inlinedFields.init();
    staticMainInserts.init();
    nilUtype = new Utype(NIL);
    varUtype = new Utype(VAR);
    nullUtype = new Utype(OBJECT);
    undefUtype = new Utype(UNDEFINED);
    undefUtype->setType(utype_unresolved);
    nullUtype->setNullType(true);
}

bool Compiler::allControlPathsReturnAValue(bool *controlPaths) {
    return controlPaths[MAIN_CONTROL_PATH] || (controlPaths[IF_CONTROL_PATH] && controlPaths[ELSEIF_CONTROL_PATH] && controlPaths[ELSE_CONTROL_PATH]);
}

void Compiler::compileLabelDecl(Ast *ast, bool *controlPaths) {
    if(allControlPathsReturnAValue(controlPaths)) {
        for(Int i = 0; i < CONTROL_PATH_SIZE; i++) {
            controlPaths[i] = false;
        }
    }

    string name = ast->getToken(0).getValue();
    if(currentScope()->currentFunction->data.getLabelAddress(name) == invalidAddr) {
        currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(name, currentScope()->currentFunction->data.code.size()));
    } else {

        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "label `" + name + "` is already defined in the scope");
    }

    Ast *branch = ast->getSubAst(0);
    if(branch->getType() == ast_block) {
        controlPaths[MAIN_CONTROL_PATH] = compileBlock(branch);
    } else {
        branch = branch->getSubAst(0);
        compileStatement(branch, controlPaths);
    }
}

void Compiler::compileIfStatement(Ast *ast, bool *controlPaths) {
    Expression cond;
    string ifEndLabel, ifBlockEnd;
    CodeHolder &code = currentScope()->currentFunction->data.code;

    compileExpression(&cond, ast->getSubAst(ast_expression));

    stringstream labelId;
    labelId << INTERNAL_LABEL_NAME_PREFIX << "if_end" << guid++;
    ifEndLabel=labelId.str(); labelId.str("");

    code.inject(cond.utype->getCode());
    code.inject(cond.utype->getCode().getInjector(ebxInjector));
    code.addIr(OpBuilder::movr(CMT, EBX));

    if(ast->getSubAstCount() > 2) {
        labelId << INTERNAL_LABEL_NAME_PREFIX << "if_block_end" << guid++;
        ifBlockEnd=labelId.str(); labelId.str("");

        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifBlockEnd, 0));
        code.addIr(OpBuilder::jne(invalidAddr));

        controlPaths[IF_CONTROL_PATH] = compileBlock(ast->getSubAst(ast_block));
        if(!ast->hasSubAst(ast_elseif_statement)) {
            controlPaths[ELSEIF_CONTROL_PATH] = true;
        }

        currentScope()->isReachable = true;
        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifEndLabel, 0));
        code.addIr(OpBuilder::jmp(invalidAddr));

        currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(ifBlockEnd, code.size()));

        for(Int i = 2; i < ast->getSubAstCount(); i++) {
            Ast *branch = ast->getSubAst(i);

            switch(branch->getType()) {
                case ast_elseif_statement: {
                    Expression ifelseCond;
                    compileExpression(&ifelseCond, branch->getSubAst(ast_expression));

                    labelId << INTERNAL_LABEL_NAME_PREFIX << "if_block_end" << guid++;
                    ifBlockEnd=labelId.str(); labelId.str("");

                    code.inject(ifelseCond.utype->getCode());
                    code.inject(ifelseCond.utype->getCode().getInjector(ebxInjector));
                    code.addIr(OpBuilder::movr(CMT, EBX));

                    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifBlockEnd, 0));
                    code.addIr(OpBuilder::jne(invalidAddr));

                    controlPaths[ELSEIF_CONTROL_PATH] = compileBlock(branch->getSubAst(ast_block));

                    if((i+1) < ast->getSubAstCount()) {
                        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifEndLabel, 0));
                        code.addIr(OpBuilder::jmp(invalidAddr));
                    }

                    currentScope()->isReachable = true;
                    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(ifBlockEnd, code.size()));
                    break;
                }

                case ast_else_statement: {
                    controlPaths[ELSE_CONTROL_PATH] = compileBlock(branch->getSubAst(ast_block));
                    break;
                }
            }
        }

    } else {
        currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), ifEndLabel, 0));
        code.addIr(OpBuilder::jne(invalidAddr));
        compileBlock(ast->getSubAst(ast_block)); // we dont care about control paths because they are not compelete
        currentScope()->isReachable = true;
    }

    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(ifEndLabel, code.size()));
//    code.addIr(OpBuilder::nop()); // Theoretically we shouldn't need this, only for visual purposes
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

    if(!returnVal.utype->isRelated(currentScope()->currentFunction->utype)) {
        errors->createNewError(GENERIC, ast->line, ast->col, "returning `" + returnVal.utype->toString() + "` from a function returning `"
                                                             + currentScope()->currentFunction->utype->toString() + "`.");
    }

    bool shouldReturnValue = currentScope()->finallyNextBlockLabel.empty() && currentScope()->finallyStartLabel.empty();

    switch (returnVal.type) {
        case exp_var:
            code.inject(returnVal.utype->getCode());

            if(shouldReturnValue) {
                code.inject(returnVal.utype->getCode().getInjector(ebxInjector));
                code.addIr(OpBuilder::returnValue(EBX));
            } else {
                code.inject(returnVal.utype->getCode().getInjector(stackInjector));
            }
            break;

        case exp_class:
        case exp_object:
        case exp_null:
            code.inject(returnVal.utype->getCode());

            if(shouldReturnValue) {
                code.inject(returnVal.utype->getCode().getInjector(ptrInjector));
                code.addIr(OpBuilder::returnObject());
            } else {
                code.inject(returnVal.utype->getCode().getInjector(stackInjector));
            }
            break;

        case exp_nil:
            code.inject(returnVal.utype->getCode());
            break;
    }

    if(!shouldReturnValue) {
        Field *shouldReturn = currentScope()->currentFunction->data.getLocalField(INTERNAL_VARIABLE_NAME_PREFIX + "shouldReturn", 0);
        if(shouldReturn == NULL) {
            Utype *fieldType = new Utype(VAR);
            List<AccessFlag> flags;
            flags.add(PUBLIC);

            string name = INTERNAL_VARIABLE_NAME_PREFIX + "shouldReturn";
            shouldReturn = createLocalField(name, fieldType, false, stl_stack, flags, 0, ast);
        }

        code.addIr(OpBuilder::istorel(shouldReturn->address, 1));

        if(!currentScope()->finallyStartLabel.empty()) {
            currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), currentScope()->finallyStartLabel, 0));
        } else {
            currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), currentScope()->finallyNextBlockLabel, 0));
        }

        code.addIr(OpBuilder::jmp(invalidAddr));
    } else {
        code.addIr(OpBuilder::ret());
    }

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

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
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

    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(whileBeginLabel, currentScope()->currentFunction->data.code.size()));

    condExpr.utype->getCode().inject(ebxInjector);
    code.inject(condExpr.utype->getCode());
    code.addIr(OpBuilder::movr(CMT, EBX));

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), whileEndLabel, 0));
    code.addIr(OpBuilder::jne(invalidAddr));

    RETAIN_LOOP_LABELS(whileBeginLabel, whileEndLabel)
    compileBlock(ast->getSubAst(ast_block));
    RESTORE_LOOP_LABELS()

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), whileBeginLabel, 0));
    code.addIr(OpBuilder::jmp(invalidAddr));

    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(whileEndLabel, currentScope()->currentFunction->data.code.size()));
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
    initializeLocalVariable(indexField);
    code.addIr(OpBuilder::isubl(1, indexField->address));

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

    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(forBeginLabel, currentScope()->currentFunction->data.code.size()));

    code.addIr(OpBuilder::iaddl(1, indexField->address))
        .addIr(OpBuilder::movl(arrayResultField->address))
        .addIr(OpBuilder::_sizeof(ECX));

    code.addIr(OpBuilder::loadl(EBX, indexField->address))
        .addIr(OpBuilder::lt(EBX, ECX));

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forEndLabel, 0));
    code.addIr(OpBuilder::jne(invalidAddr));

    code.addIr(OpBuilder::movnd(EBX));

    if(iteratorField->isVar()) {
        code.addIr(OpBuilder::movi(0, ADX))
            .addIr(OpBuilder::iaload(EBX, ADX))
            .addIr(OpBuilder::smovr2(EBX, iteratorField->address));
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

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forBeginLabel, 0));
    code.addIr(OpBuilder::jmp(invalidAddr));

    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(forEndLabel, currentScope()->currentFunction->data.code.size()));
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

    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(forBeginLabel, currentScope()->currentFunction->data.code.size()));
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
            currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forEndLabel, 0));
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

    currentScope()->currentFunction->data.branchTable.add(BranchTable(code.size(), forBeginLabel, 0));
    code.addIr(OpBuilder::jmp(invalidAddr));

    currentScope()->isReachable = true;
    currentScope()->currentFunction->data.labelMap.add(KeyPair<string, Int>(forEndLabel, currentScope()->currentFunction->data.code.size()));
}

void Compiler::compileStatement(Ast *ast, bool *controlPaths) {
    if(!currentScope()->isReachable) {
        if(ast->getType() != ast_label_decl)
            errors->createNewError(GENERIC, ast->line, ast->col, "unreachable statement");
        currentScope()->isReachable = true;
    }

    switch(ast->getType()) {
        case ast_return_stmnt:
            return compileReturnStatement(ast, controlPaths);
        case ast_if_statement:
            return compileIfStatement(ast, controlPaths);
        case ast_expression: {
            Expression expr;
            compileExpression(&expr, ast);
            expr.utype->getCode().inject(removeFromStackInjector);
            currentScope()->currentFunction->data.code.inject(expr.utype->getCode());
            // TODO: add code for calling the base constructor
            break;
        }
        case ast_label_decl:
            return compileLabelDecl(ast, controlPaths);
        case ast_variable_decl:
            return compileLocalVariableDecl(ast); // TODO: fire out a way to prevent unititialized vars from being used as a result of a goto
        case ast_alias_decl:
            return compileLocalAlias(ast);
        case ast_for_statement:
            return compileForStatement(ast);
        case ast_foreach_statement:
            return compileForEachStatement(ast);
        case ast_while_statement:
            return compileWhileStatement(ast);
        default:
            stringstream err;
            err << ": unknown ast type: " << ast->getType();
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            break;
    }
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
    bool controlPaths[]
        = {
            false, // MAIN_CONTROL_PATH
            false, // IF_CONTROL_PATH
            false, // ELSEIF_CONTROL_PATH
            false  // ELSE_CONTROL_PATH
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

    invalidateLocalAliases();
    invalidateLocalVariables();
    currentScope()->scopeLevel--;
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

void Compiler::reconcileBranches(Ast *ast) {
    CodeHolder &code = currentScope()->currentFunction->data.code;
    for(Int i = 0; i < currentScope()->currentFunction->data.branchTable.size(); i++) {
        BranchTable &branch = currentScope()->currentFunction->data.branchTable.get(i);
        Int labelAddress = currentScope()->currentFunction->data.getLabelAddress(branch.labelName);

        if(branch.branch_pc < code.size()) {
            if(labelAddress != invalidAddr) {
                switch (GET_OP(code.ir32.get(branch.branch_pc))) {
                    case Opcode::JNE:
                        code.ir32.get(branch.branch_pc) = OpBuilder::jne(labelAddress + branch._offset);
                        break;
                    case Opcode::JE:
                        code.ir32.get(branch.branch_pc) = OpBuilder::je(labelAddress + branch._offset);
                        break;
                    case Opcode::JMP:
                        code.ir32.get(branch.branch_pc) = OpBuilder::jmp(labelAddress + branch._offset);
                        break;
                }
            } else {
                errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, ": attempt to jump to label `" + branch.labelName + "` that dosen't exist in the current function");
            }
        } else {
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, ": attempt to reconcile branch that is not in codebase ");
        }
    }

    currentScope()->currentFunction->data.branchTable.free();
    currentScope()->currentFunction->data.labelMap.free();
}

void Compiler::compileInitDecl(Ast *ast) {
    Ast *block = ast->getSubAst(ast_block);

    List<Method*> constructors;
    currentScope()->klass->getAllFunctionsByType(fn_constructor, constructors);
    for(uInt i = 0; i < constructors.size(); i++) {
        Method *constructor = constructors.get(i);
        uInt totalErrors = errors->getUnfilteredErrorCount();
        currentScope()->currentFunction = constructor;

        // we dont need to setup locals since it wont have any!
        RETAIN_BLOCK_TYPE(INSTANCE_BLOCK)
        if(!compileBlock(block)) {
            cout << "not all code paths return a value";
        }
        RESTORE_BLOCK_TYPE()

        reconcileBranches(ast);
        if(GET_OP(constructor->data.code.ir32.last()) != Opcode::RET) {
            constructor->data.code.addIr(OpBuilder::ret());
        }
//        cout << ast->toString() << endl;
        printMethodCode(*constructor, ast);
        if(NEW_ERRORS_FOUND()){
            break; // no need to waste processing power to compile a broken init decl
        }
    }

    constructors.free();
}

void Compiler::compileAllInitDecls() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        updateErrorManagerInstance(current);

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (unsigned long x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);
            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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

        currModule = "$unknown";
        long long totalErrors = errors->getUnfilteredErrorCount();
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (unsigned long x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);
            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
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
                case ast_enum_decl:
                    break;
                case ast_method_decl: /* ignore */
                    break;
                case ast_variable_decl:
                    compileVariableDecl(branch);
                    break;
                case ast_mutate_decl:
                    /* ignpre */
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

void Compiler::compile() {
    setup(); // TODO: talk about the changes maid to main.cpp in tutorial series for counting failed files commit is 17th pass on rewrite

    if(preprocess() && postProcess()) {
        processingStage = COMPILING;
        // TODO: write compileUnprocessedClasses() to be called for everything else and free unprocessedClasses list
        compileAllFields();
        compileAllInitDecls();
        initProcessed = true;


        // TODO: compile umprocessed methods last
        cout << "compiled!!!";
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

ClassObject* Compiler::findClass(string mod, string className, List<ClassObject*> &classes, bool match) {
    ClassObject* klass = NULL;
    bool found;
    for(unsigned int i = 0; i < classes.size(); i++) {
        klass = classes.get(i);
        if(match) found = klass->name.find(className) != string::npos;
        else
            found = klass->name == className;

        if(found) {
            if(mod != "" && klass->module == mod)
                return klass;
            else if(mod == "")
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
        note << meta.file << ":" << meta.ln << ":" << meta.col << ": note:  " << msg
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
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    ClassObject* k = new ClassObject(name, currModule, this->guid++, flags, meta), *prevClass;
    if((prevClass = findClass(currModule, name, classList))
        || (prevClass = findClass(currModule, name + "<>", classList)) != NULL){
        prevDefined:
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                                                                                "` is already defined in module {" + currModule + "}");

        printNote(prevClass->meta, "class `" + prevClass->fullName + "` previously defined here");
        k->free();
        delete k;
        return !ends_with(prevClass->name, "<>") ? prevClass : NULL;
    } else {
        if(ends_with(k->name, "<>") && (prevClass = findClass(currModule, k->name.substr(0, k->name.size() - 2), classes)) != NULL)
            goto prevDefined;

        k->owner = findClass(currModule, globalClass, classes);
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

bool Compiler::complexParameterMatch(List<Field*> &params, List<Field*> &comparator) {
    if(params.size() == comparator.size()) {
        for(long i = 0; i < params.size(); i++) {
            if(!params.get(i)->isRelated(*comparator.get(i)) && !isUtypeConvertableToNativeClass(params.get(i)->utype, comparator.get(i)->utype)
                && !isUtypeConvertableToNativeClass(comparator.get(i)->utype, params.get(i)->utype))
                return false;
        }

        return true;
    }

    return false;
}

void Compiler::addDefaultConstructor(ClassObject* klass, Ast* ast) {
    List<Field*> emptyParams;
    List<AccessFlag> flags;
    flags.add(PUBLIC);

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line),
              current->getTokenizer()->file, ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);
    if(klass->getConstructor(emptyParams, false) == NULL) {
        Method* method = new Method(klass->name, currModule, klass, emptyParams, flags, meta);

        method->fullName = klass->fullName + "." + klass->name;
        method->ast = ast;
        method->fnType = fn_constructor;
        method->address = methodSize++;
        method->utype = nilUtype;
        klass->addFunction(method);
    }
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
        ss << currModule << "#" << global->name;
        global->fullName = ss.str();
        addDefaultConstructor(global, NULL);
        classes.add(global); // global class ref
    }

    flags.free();
}

bool
Compiler::addFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &)) {
    List<Method*> funcs;
    k->getFunctionByName(method->name, funcs);

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
    k->getFunctionByName(method->name, funcs);

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
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->getTokenizer()->file,
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    Field* prevField=NULL;
    if((prevField = currentScope()->currentFunction->data.getLocalField(name, scopeLevel)) != NULL) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, prevField->ast->line, prevField->ast->col,
                                     "local field `" + name + "` is already defined in the scope");
        printNote(prevField->meta, "local field `" + name + "` previously defined here");
        return prevField;
    } else {
        if((prevField = currentScope()->currentFunction->data.getLocalFieldHereOrHigher(name, scopeLevel)) != NULL) {
            if(prevField->scopeLevel > 0 || initProcessed) {
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                                 "local field `" + name + "` shadows another field at higher scope");
            }
        }

        Field *local = new Field(type == NULL ? UNTYPED : type->getResolvedType()->type, guid++, name, NULL, flags, meta, locality, checkstl(locality));
        currentScope()->currentFunction->data.locals.add(local);
        local->ast = ast;
        local->local = true;
        local->utype = type;
        local->isArray = isArray;
        local->owner = currentScope()->klass;
        local->scopeLevel = scopeLevel;
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
