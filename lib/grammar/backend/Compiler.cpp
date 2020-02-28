//
// Created by BraxtonN on 10/18/2019.
//

#include <cmath>
#include "Compiler.h"
#include "../main.h"
#include "oo/ClassObject.h"
#include "ReferencePointer.h"
#include "data/Utype.h"
#include "../../runtime/Opcode.h"
#include "../../runtime/register.h"
#include "Expression.h"
#include "data/Literal.h"
#include "../../runtime/oo/Method.h"

string globalClass = "__srt_global";
long Compiler::guid = 0;
void Compiler::generate() {

}

void Compiler::cleanup() {

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
    if(ast->getEntityCount() > 4)
        this->errors->createNewError(GENERIC, ast->line, ast->col, "class objects only allows for access "
                                                                   "specifiers (public, private, protected, local, ext, and stable)");
    else {
        long errPos=0;
        AccessFlag lastFlag = flg_UNDEFINED;
        for(long i = 0; i < ast->getEntityCount(); i++) {
            AccessFlag flag = strToAccessFlag(ast->getEntity(i).getValue());
            flags.add(flag);

            if(flag == flg_CONST || flag == STATIC) {
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                             " `" + ast->getEntity(i).getValue() + "`");
            } else if(flag == lastFlag) {
                this->errors->createNewError(ILLEGAL_ACCESS_DECLARATION, ast->line, ast->col,
                                             " duplicate access flag `" + ast->getEntity(i).getValue() + "`");
            }

            if(flag==LOCAL && !globalScope()) {
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                             " `" + ast->getEntity(i).getValue() + "` can only be used at global scope");
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
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getEntity(errPos).getLine(),
                                             ast->getEntity(errPos).getColumn(),
                                             " `" + ast->getEntity(errPos).getValue() + "`. Make sure the order is correct and you're using the appropriate modifiers.");
            }

            if ((flags.find(PUBLIC) || flags.find(PRIVATE) || flags.find(PROTECTED)) && flags.find(LOCAL)) {
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col,
                                 "`local` access specifier trumps (public, private, and protected) flags");
            }
        }
    }
}

ClassObject* Compiler::addChildClassObject(string name, List<AccessFlag> &flags, ClassObject* owner, Ast* ast) {
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->sourcefile,
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    ClassObject *klass = new ClassObject(name,
                                         currModule, this->guid++, flags,
                                         meta), *prevClass;
    if((prevClass = owner->getChildClass(name)) != NULL) {
        this->errors->createNewError(DUPLICATE_CLASS, ast->line, ast->col, " '" + name + "'");

        klass->free();
        delete klass;
        printNote(owner->getChildClass(name)->meta, "class `" + name + "` previously defined here");
        return prevClass;
    } else {
        owner->addClass(klass);
        return klass;
    }
}


void Compiler::parseVariableAccessFlags(List<AccessFlag> &flags, Ast *ast) {
    long errPos=0;
    for(long i = 0; i < ast->getEntityCount(); i++) {
        flags.add(strToAccessFlag(ast->getEntity(i).getValue()));

        if(flags.last() == LOCAL && !globalScope()) {
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                         " `" + ast->getEntity(i).getValue() + "` can only be used at global scope");
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
        this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getEntity(errPos).getLine(),
                                     ast->getEntity(errPos).getColumn(), " `" + ast->getEntity(errPos).getValue() + "`");
    }

}

StorageLocality Compiler::strtostl(string locality) {
    return locality == "thread_local" ? stl_thread : stl_stack;
}



void Compiler::preProccessVarDeclHelper(List<AccessFlag>& flags, Ast* ast) {
    string name = ast->getEntity(0).getValue();
    StorageLocality locality = stl_stack;
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        locality = strtostl(name);
        name = ast->getEntity(1).getValue();
        if(flags.find(flg_CONST)) {
            this->errors->createNewError(GENERIC, ast->line, ast->col,
                                         "thread local field `" + name + "` does not allow `const` access modifier");
        }
        flags.addif(STATIC);
    }

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->sourcefile,
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
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
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

void Compiler::preProccessAliasDecl(Ast* ast) {
    List<AccessFlag> flags;

    if(ast->hasSubAst(ast_access_type)){
        parseVariableAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
    }

    // todo: validate flags certain ones are not allowed
    string name = ast->getEntity(0).getValue();
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->sourcefile,
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    Alias* prevAlias=NULL;
    if((prevAlias = currentScope()->klass->getAlias(name, false)) != NULL) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "alias `" + name + "` is already defined in the scope");
        printNote(prevAlias->meta, "alias `" + name + "` previously defined here");
    } else {
        currentScope()->klass->getAliases().add(new Alias(guid++, name, currentScope()->klass, flags, meta));
        if(currentScope()->klass->getField(name, false) != NULL) {
            createNewWarning(GENERIC, __WDECL, ast->line, ast->col,
                             "declared alias `" + name + "` conflicts with field `" + currentScope()->klass->getField(name, false)->toString() + "`");
        }

        currentScope()->klass->getAlias(name, false)->ast = ast;
        currentScope()->klass->getAlias(name, false)->fullName = currentScope()->klass->fullName + "." + name;
    }
}

void Compiler::parseIdentifierList(Ast *ast, List<string> &idList) {
    ast = ast->getSubAst(ast_identifier_list);

    for(long i = 0; i < ast->getEntityCount(); i++) {
        string Key = ast->getEntity(i).getValue();
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
    } else {
        flags.add(PUBLIC);
    }

    if(!flags.find(STABLE))
        flags.add(UNSTABLE);

    string className = ast->getEntity(0).getValue();
    List<string> identifierList;
    parseIdentifierList(ast, identifierList);

    stringstream fullName;
    if (currentScope()->klass == NULL || globalScope()) {
        // should never happen unless due to user error
        currentClass = addGlobalClassObject(className + "<>", flags, ast, generics);
        fullName << currModule << "#" << className;
    } else {
        currentClass = addChildClassObject(className + "<>", flags, currentScope()->klass, ast);
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
    currentClass->ast = ast;
    // the purpose of this is simply to create the existance of the generic class we will process it later
}


void Compiler::preProccessEnumVar(Ast *ast) {
    List<AccessFlag> flags;
    flags.add(PUBLIC);
    flags.add(STATIC);
    flags.add(flg_CONST);

    string name = ast->getEntity(0).getValue();
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->sourcefile,
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
    } else {
        flags.add(PUBLIC);
    }

    /**
     * We do not want other users messing with enums at compile time causing issues
     */
    flags.addif(STABLE);
    string className = ast->getEntity(0).getValue();

    if(currentScope()->klass == NULL || globalScope()) {
        // should never happen unless due to user error
        currentClass = addGlobalClassObject(className, flags, ast, classes);
        stringstream ss;
        ss << currModule << "#" << currentClass->name;
        currentClass->fullName = ss.str();
    }
    else {
        currentClass = addChildClassObject(className, flags, currentScope()->klass, ast);

        stringstream ss;
        ss << currentScope()->klass->fullName << "." << currentClass->name;
        currentClass->fullName = ss.str();
    }

    enums.addif(currentClass);
    currentClass->ast = ast;
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
            case ast_method_decl: /* invalid */
                break;
            case ast_operator_decl: /* invalid */
                break;
            case ast_construct_decl: /* invalid */
                break;
            case ast_delegate_impl: /* invalid */
                break;
            case ast_delegate_decl: /* invalid */
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

void Compiler::preProccessClassDecl(Ast* ast, bool isInterface, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

    if(currentClass == NULL) {
        if (ast->hasSubAst(ast_access_type)) {
            parseClassAccessFlags(flags, ast->getSubAst(ast_access_type));
        } else {
            flags.add(PUBLIC);
        }

        if(ast->getType() == ast_interface_decl || ast->getType() == ast_generic_interface_decl) {

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

        int startPos = 0;
        string className = ast->getEntity(0).getValue();
        if (currentScope()->klass == NULL || globalScope()) {
            currentClass = addGlobalClassObject(className, flags, ast, classes);

            stringstream ss;
            ss << currModule << "#" << currentClass->name;
            currentClass->fullName = ss.str();
        } else {
            currentClass = addChildClassObject(className, flags, currentScope()->klass, ast);

            stringstream ss;
            ss << currentScope()->klass->fullName << "." << currentClass->name;
            currentClass->fullName = ss.str();
        }

        currentClass->ast = ast;
        currentClass->setClassType(isInterface ? class_interface : class_normal);
    }

    currentClass->address = classSize++;
    currScope.add(new Scope(currentClass, CLASS_SCOPE));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        Ast* branch = block->getSubAst(i);
        CHECK_CMP_ERRORS(return;)

        switch(branch->getType()) {
            case ast_class_decl:
                preProccessClassDecl(branch, false);
                break;
            case ast_variable_decl:
                preProccessVarDecl(branch);
                break;
            case ast_method_decl: /* Will be parsed later */
                break;
            case ast_func_prototype:
                preProccessVarDecl(branch);
                break;
            case ast_alias_decl:
                preProccessAliasDecl(branch);
                break;
            case ast_operator_decl: /* Will be parsed later */
                break;
            case ast_construct_decl: /* Will be parsed later */
                break;
            case ast_delegate_impl: /* Will be parsed later */
                break;
            case ast_delegate_decl: /* Will be parsed later */
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
                if(processingStage >= POST_PROCESSING)
                    preProcessMutation(branch);
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
    for(long i = 0; i < branch->getEntityCount(); i++) {
        if(branch->getEntity(i).getValue() != "*") {
            ss << branch->getEntity(i).getValue();
        } else
            star = true;
    }

    string mod = ss.str();
    if(star) {
        bool found = false;
        for(long i = 0; i < modules.size(); i++) {
            if(startsWith(modules.get(i), mod)) {
                found = true;
                if(imports.find(modules.at(i))) {
                    createNewWarning(GENERIC, __WGENERAL, branch->line, branch->col, "module `" + modules.at(i) + "` has already been imported.");
                } else
                    imports.add(modules.at(i));
            }
        }

        if(!found) {
            errors->createNewError(GENERIC, branch->line, branch->col, "modules under prefix `" + mod +
                                                                       "*` could not be found");
        }
    } else {
        if(modules.find(mod)) {
            if(imports.find(mod)) {
                createNewWarning(GENERIC, __WGENERAL, branch->line, branch->col, "module `" + mod + "` has already been imported.");
            } else
                imports.add(mod);
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

    for(unsigned long x = 0; x < current->size(); x++)
    {
        Ast *branch = current->astAt(x);
        if(x == 0)
        {
            if(branch->getType() == ast_module_decl) {
                imports.push_back(currModule = parseModuleDecl(branch));
                // add class for global methods
                continue;
            } else {
                /* ignore */
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


    importMap.__new().key = current->sourcefile;
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
            if (importMap.get(i).key == current->sourcefile) {

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
            if (importMap.get(i).key == current->sourcefile) {

                List<string> &lst = importMap.get(i).value;
                for (unsigned int x = 0; x < lst.size(); x++) {
                    if ((klass = findClass(lst.get(x), name, classes, match)) != NULL) {
                        results.add(klass);
                    }
                }

                break;
            }
        }
    }

    return !results.empty();
}

void Compiler::inheritObjectClassHelper(Ast *ast, ClassObject *klass) {
    if(klass->module == "std" && klass->name == "_object_") { }
    else if(klass->getSuperClass() == NULL){
        ClassObject *base = findClass("std", "_object_", classes);

        if (base != NULL && (IS_CLASS_ENUM(base->getClassType()) || IS_CLASS_INTERFACE(base->getClassType()))) {
            stringstream err;
            err << "support class for objects must be of type class";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
        } else {
            if (base != NULL)
                klass->setSuperClass(base);
            else {
                stringstream err;
                err << "support class for objects not found";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
    }

    for(long long x = 0; x < klass->getChildClasses().size(); x++) {
        ClassObject* child = klass->getChildClasses().get(x);
        inheritObjectClassHelper(child->ast, child);
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
    } else {

        for(long long x = 0; x < enumClass->getChildClasses().size(); x++) {
            ClassObject *child = enumClass->getChildClasses().get(x);
            inheritEnumClassHelper(child->ast, child);
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
                expr->utype->setResolvedType(new Literal('\n'));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, '\n'), i64ebx);
                break;
            case 't':
                expr->utype->setResolvedType(new Literal('\t'));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, '\t'), i64ebx);
                break;
            case 'b':
                expr->utype->setResolvedType(new Literal('\b'));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, '\b'), i64ebx);
                break;
            case 'v':
                expr->utype->setResolvedType(new Literal('\v'));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, '\v'), i64ebx);
                break;
            case 'r':
                expr->utype->setResolvedType(new Literal('\r'));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, '\r'), i64ebx);
                break;
            case 'f':
                expr->utype->setResolvedType(new Literal('\f'));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, '\f'), i64ebx);
                break;
            case '\\':
                expr->utype->setResolvedType(new Literal('\\'));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, '\\'), i64ebx);
                break;
            default:
                expr->utype->setResolvedType(new Literal(token.getValue().at(1)));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, token.getValue().at(1)), i64ebx);
                break;
        }
    } else {
        expr->utype->setResolvedType(new Literal(token.getValue().at(0)));
        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, token.getValue().at(0)), i64ebx);
    }

    IrCode &inj = expr->utype->getCode().getInjector(stackInjector);
    inj.push_i64(SET_Di(i64, op_RSTORE, i64ebx));
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
        if(value > DA_MAX || value < DA_MIN) {
            stringstream ss;
            ss << "integral number too large: " + int_string;
            errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
        }
        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, value), i64ebx);

        expr->utype->getCode().getInjector(stackInjector)
                .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
    }else {
        value = std::strtod (int_string.c_str(), NULL);
        if((int64_t )value > DA_MAX || (int64_t )value < DA_MIN) {
            stringstream ss;
            ss << "integral number too large: " + int_string;
            errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
        }

        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVBI, ((int64_t)value)), abs(getLowBytes(value)));

        expr->utype->getCode().getInjector(stackInjector)
                .push_i64(SET_Di(i64, op_RSTORE, i64bmr));
    }

    expr->utype->setResolvedType(new Literal(value));
}

void Compiler::parseHexLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);

    double value;
    string hex_string = invalidateUnderscores(token.getValue());

    value = strtoll(hex_string.c_str(), NULL, 16);
    if(value > DA_MAX || value < DA_MIN) {
        stringstream ss;
        ss << "integral number too large: " + hex_string;
        errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
    }
    expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, value), i64ebx);
    expr->utype->getCode().getInjector(stackInjector)
            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));

    expr->utype->setResolvedType(new Literal(value));
}

void Compiler::parseStringLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(true);

    stringMap.addif(token.getValue());
    unsigned long long index =  stringMap.indexof(token.getValue());

    expr->utype->setResolvedType(new Literal(token.getValue(), index));
    expr->utype->getCode().push_i64(SET_Di(i64, op_NEWSTRING, index));

    expr->utype->getCode().getInjector(ptrInjector)
            .push_i64(SET_Ei(i64, op_POPOBJ_2));
}

void Compiler::parseBoolLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);
    expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, (token.getValue() == "true" ? 1 : 0)), i64ebx);

    expr->utype->getCode().getInjector(stackInjector)
            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
    expr->utype->setResolvedType(new Literal(token.getValue() == "true" ? 1 : 0));
}

void Compiler::compileLiteralExpression(Expression* expr, Ast* ast) {
    switch(ast->getEntity(0).getId()) {
        case CHAR_LITERAL:
            parseCharLiteral(expr, ast->getEntity(0));
            break;
        case INTEGER_LITERAL:
            parseIntegerLiteral(expr, ast->getEntity(0));
            break;
        case HEX_LITERAL:
            parseHexLiteral(expr, ast->getEntity(0));
            break;
        case STRING_LITERAL:
            parseStringLiteral(expr, ast->getEntity(0));
            break;
        default:
            if(ast->getEntity(0).getValue() == "true" ||
                    ast->getEntity(0).getValue() == "false") {
                parseBoolLiteral(expr, ast->getEntity(0));
            }
            break;
    }

    expr->ast = ast;
    compilePostAstExpressions(expr, ast);
}

void Compiler::compileUtypeClass(Expression* expr, Ast* ast) {
    Utype* utype = compileUtype(ast->getSubAst(ast_utype));
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);

    if(utype->getType() == utype_class) {
        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, utype->getResolvedType()->address), i64ebx);
        expr->utype->setResolvedType(new Literal(utype->getResolvedType()->address));
    } else {
        expr->utype->free();
        errors->createNewError(GENERIC, ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->line,
                               ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->col, "expected class");
    }

    expr->utype->getCode().getInjector(stackInjector)
            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
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
        params.get(i)->inlineCheck = true;

        if(e.type == exp_null) {
            params.get(i)->nullField = true;
            params.get(i)->utype = new Utype(OBJECT);
            params.get(i)->type = OBJECT;

            // yet another line of nasty dirty code...smh...
            params.get(i)->utype->getCode().inject(0, e.utype->getCode());
            params.get(i)->utype->getCode().getInjector(stackInjector).inject(0, e.utype->getCode().getInjector(stackInjector));
            freePtr(e.utype);
        }
        else if(e.type == exp_var && e.utype->getType() == utype_literal) {
            if(((Literal*) e.utype->getResolvedType())->literalType == string_literal) {
                params.get(i)->utype = new Utype(_INT8);
                params.get(i)->type = _INT8;
            } else {
                params.get(i)->utype = new Utype(VAR);
                params.get(i)->type = VAR;
            }

            // yet another line of nasty dirty code...smh...
            params.get(i)->utype->getCode().inject(0, e.utype->getCode());
            params.get(i)->utype->getCode().getInjector(stackInjector).inject(0, e.utype->getCode().getInjector(stackInjector));
            freePtr(e.utype);
        }
        else if(e.type == exp_var && e.utype->getType() == utype_method) {
            params.get(i)->utype = new Utype(VAR);
            params.get(i)->type = VAR;

            // yet another line of nasty dirty code...smh...
            params.get(i)->utype->getCode().inject(0, e.utype->getCode());
            params.get(i)->utype->getCode().getInjector(stackInjector).inject(0, e.utype->getCode().getInjector(stackInjector));
            freePtr(e.utype);
        }
        else {
            params.get(i)->isArray = e.utype->isArray();
            params.get(i)->utype = e.utype;
            params.get(i)->type = e.utype->getResolvedType() ? e.utype->getResolvedType()->type : UNDEFINED;
        }
        e.utype = NULL;
    }
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

Method* // TODO: Circle back around to this as it is in its primitive stae there might beissues with how it searches the mehtods with _int64 _int8 etc.
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
        List<ClassObject*> globalClasses;
        resolveClass(classes, globalClasses, "", globalClass, ast);

        for(long i = 0; i < globalClasses.size(); i++) {
            if(currentScope()->type == RESTRICTED_INSTANCE_BLOCK && !currentScope()->klass->isClassRelated(globalClasses.get(i)))
                continue;

            if((resolvedMethod = findFunction(globalClasses.get(i), name, params, ast, true)) != NULL)
                return resolvedMethod;
        }

        // TODO: check for overloads later i need to see how it comes in
        if((resolvedMethod = findGetterSetter(currentScope()->klass, name, params, ast)) != NULL) {
            return resolvedMethod;
        }
        else if((resolvedMethod = findFunction(currentScope()->klass, name, params, ast, true)) != NULL)
            return resolvedMethod;
        else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && currentScope()->getLocalField(name) != NULL) {
            Field* field = currentScope()->getLocalField(name)->field;
            if(field->utype && field->utype->getType() == utype_method_prototype) {
                resolvedMethod =  (Method*)field->utype->getResolvedType();


                expr->utype->getCode()
                        .push_i64(SET_Ci(i64, op_LOADL, i64ebx, 0, field->address))
                        .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                if(!complexParameterMatch(resolvedMethod->params, params)) {
                    errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` does not match provided parameter arguments");
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
            if(field->utype && field->utype->getType() == utype_method_prototype) {
                resolvedMethod =  (Method*)field->utype->getResolvedType();

                if(field->locality == stl_thread) {
                    expr->utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
                } else {
                    if(field->flags.find(STATIC) || field->owner->isGlobalClass())
                        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    else if(!expr->utype->getCode().instanceCaptured) {
                        expr->utype->getCode().instanceCaptured = true;
                        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                    }
                    expr->utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address));
                }

                expr->utype->getCode()
                        .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                        .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx))
                        .push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                if(!complexParameterMatch(resolvedMethod->params, params)) {
                    errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` does not match provided parameter arguments");
                }
                return resolvedMethod;
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` is not a function pointer");
            }
        } else if(currentScope()->klass->isClassRelated(findClass("std", "_enum_", classes))) {
            ClassObject *klass = currScope.get(currScope.size()-2)->klass;

            // TODO: function or field must be static as we dont own the instance
            if((resolvedMethod = findFunction(klass, name, params, ast, true)) != NULL)
                return resolvedMethod;
            else if(klass->getField(name, true) != NULL) {
                Field* field = klass->getField(name, true);
                compileFieldType(field);
                
                if(field->utype && field->utype->getType() == utype_method_prototype) {
                    resolvedMethod =  (Method*)field->utype->getResolvedType();

                    if(!complexParameterMatch(resolvedMethod->params, params)) {
                        errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` does not match provided parameter arguments");
                    }
                    expr->utype->getCode()
                            .push_i64(SET_Di(i64, op_MOVG, field->owner->address))
                            .push_i64(SET_Di(i64, op_MOVN, field->address))
                            .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                            .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx))
                            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                    if(!field->flags.find(STATIC))
                        errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` must be static to be used in static context");
                    return resolvedMethod;
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` is not a function pointer");
                }
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
            if(utype->getClass() && utype->getClass()->name == va_arg(ap, string)
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
                    utype->getCode().push_i64(SET_Ei(i64, op_PUSHOBJ));
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
                    utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address))
                            .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                            .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx))
                            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                    if(!complexParameterMatch(resolvedMethod->params, params)) {
                        errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->toString() + "` does not match provided parameter arguments");
                    }
                }
                else
                    errors->createNewError(GENERIC, ast->line, ast->col,
                                           " symbol `" + name + "` is not a function or pointer");
            } else
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       " symbol `" + name + "` is not a function or pointer");

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
        IrCode &code = expr->utype->getCode();

        validateAccess(resolvedMethod, ast);
        if(!resolvedMethod->flags.find(STATIC) && resolvedMethod->fnType != fn_prototype) {
            if(singularCall) {
                code.push_i64(SET_Di(i64, op_MOVL, 0));
                code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
        }

        for(long i = 0; i < params.size(); i++) {
            Field *param = resolvedMethod->params.get(i);
            if(isLambdaUtype(param->utype))
                fullyQualifyLambda(params.get(i)->utype, param->utype);

            if(isUtypeConvertableToNativeClass(param->utype, params.get(i)->utype)) {
                convertUtypeToNativeClass(param->utype, params.get(i)->utype, code, ast);
            } else {
                params.get(i)->utype->getCode().inject(stackInjector);
                code.inject(code.size(), params.get(i)->utype->getCode());
            }
        }

        if(resolvedMethod->fnType == fn_delegate) {
            if(resolvedMethod->flags.find(STATIC)) {
                code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE_STATIC, resolvedMethod->address, 0, params.size()), resolvedMethod->owner->address);
            } else
                code.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE, resolvedMethod->address, 0, params.size()));
        } else {
            if(resolvedMethod->fnType != fn_prototype)
                code.push_i64(SET_Di(i64, op_CALL, resolvedMethod->address));
            else {
                code.push_i64(SET_Di(i64, op_LOADVAL, i64ebx))
                    .push_i64(SET_Di(i64, op_CALLD, i64ebx));
            }
        }
    }

    ptr.free();
    freeListPtr(expressions);
    freeListPtr(params);
    return resolvedMethod;
}

/*
 * DEBUG: This is a debug function made to print the generated contents of a n expression's code
 */
void Compiler::printExpressionCode(Expression *expr) {
    cout << "\n\n\n==== Expression Code ===\n";
    cout << "line " << expr->ast->line << " file " << current->sourcefile << endl;
    cout << codeToString(expr->utype->getCode());
    cout << "\n=========\n" << std::flush;
}

/*
 * DEBUG: This is a debug function made to get the name of a register
 */
string Compiler::registerToString(int64_t r) {
    switch(r) {
        case i64adx:
            return "adx";
        case i64cx:
            return "cx";
        case i64cmt:
            return "cmt";
        case i64ebx:
            return "ebx";
        case i64ecx:
            return "ecx";
        case i64ecf:
            return "ecf";
        case i64edf:
            return "edf";
        case i64ehf:
            return "ehf";
        case i64bmr:
            return "bmr";
        case i64egx:
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
string Compiler::codeToString(IrCode &code) {
    stringstream ss;
    for(unsigned int x = 0; x < code.size(); x++) {
        int64_t x64=code.ir64.get(x);

        switch(GET_OP(x64)) {
            case op_NOP:
            {
                ss<<"nop";
                
                break;
            }
            case op_INT:
            {
                ss<<"int 0x" << std::hex << GET_Da(x64);
                
                break;
            }
            case op_MOVI:
            {
                ss<<"movi #" << GET_Da(x64) << ", ";
                ss<< registerToString(code.ir64.get(++x)) ;
                
                break;
            }
            case op_RET:
            {
                ss<<"ret";
                
                break;
            }
            case op_HLT:
            {
                ss<<"hlt";
                
                break;
            }
            case op_NEWARRAY:
            {
                ss<<"newarry ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_CAST:
            {
                ss<<"cast ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_VARCAST:
            {
                ss<<"vcast ";
                ss<< GET_Da(x64);
                
                break;
            }
            case op_MOV8:
            {
                ss<<"mov8 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_MOV16:
            {
                ss<<"mov16 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_MOV32:
            {
                ss<<"mov32 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_MOV64:
            {
                ss<<"mov64 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            } case op_MOVU8:
            {
                ss<<"movu8 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_MOVU16:
            {
                ss<<"movu16 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_MOVU32:
            {
                ss<<"movu32 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_MOVU64:
            {
                ss<<"movu64 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_RSTORE:
            {
                ss<<"rstore ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_ADD:
            {
                ss<<"add ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                ss<< " -> ";
                ss<< registerToString(code.ir64.get(++x));
                
                break;
            }
            case op_SUB:
            {
                ss<<"sub ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                ss<< " -> ";
                ss<< registerToString(code.ir64.get(++x));
                
                break;
            }
            case op_MUL:
            {
                ss<<"mul ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                ss<< " -> ";
                ss<< registerToString(code.ir64.get(++x));
                
                break;
            }
            case op_DIV:
            {
                ss<<"div ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                ss<< " -> ";
                ss<< registerToString(code.ir64.get(++x));
                
                break;
            }
            case op_MOD:
            {
                ss<<"mod ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                ss<< " -> ";
                ss<< registerToString(code.ir64.get(++x));
                
                break;
            }
            case op_IADD:
            {
                ss<<"iadd ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", #";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_ISUB:
            {
                ss<<"isub ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", #";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_IMUL:
            {
                ss<<"imul ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", #";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_IDIV:
            {
                ss<<"idiv ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", #";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_IMOD:
            {
                ss<<"imod ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", #";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_POP:
            {
                ss<<"pop";
                
                break;
            }
            case op_INC:
            {
                ss<<"inc ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_DEC:
            {
                ss<<"dec ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_MOVR:
            {
                ss<<"movr ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_IALOAD:
            {
                ss<<"iaload ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_BRH:
            {
                ss<<"brh";
                
                break;
            }
            case op_IFE:
            {
                ss<<"ife";
                
                break;
            }
            case op_IFNE:
            {
                ss<<"ifne";
                
                break;
            }
            case op_JE:
            {
                ss<<"je " << GET_Da(x64);
                
                break;
            }
            case op_JNE:
            {
                ss<<"jne " << GET_Da(x64);
                
                break;
            }
            case op_LT:
            {
                ss<<"lt ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_GT:
            {
                ss<<"gt ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_LTE:
            {
                ss<<"lte ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_GTE:
            {
                ss<<"gte ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_MOVL:
            {
                ss<<"movl " << GET_Da(x64);
                
                break;
            }
            case op_POPL:
            {
                ss<<"popl " << GET_Da(x64);
                
                break;
            }
            case op_MOVSL:
            {
                ss<<"movsl #";
                ss<< GET_Da(x64);
                
                break;
            }
            case op_MOVBI:
            {
                ss<<"movbi #" << GET_Da(x64) << ", #";
                ss<< code.ir64.get(++x);
                
                break;
            }
            case op_SIZEOF:
            {
                ss<<"sizeof ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_PUT:
            {
                ss<<"put ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_PUTC:
            {
                ss<<"_putc ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_CHECKLEN:
            {
                ss<<"chklen ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_GOTO:
            {
                ss<<"goto @" << GET_Da(x64);
                
                break;
            }
            case op_LOADPC:
            {
                ss<<"loadpc ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_PUSHOBJ:
            {
                ss<<"pushobj";
                
                break;
            }
            case op_DEL:
            {
                ss<<"del";
                
                break;
            }
            case op_CALL:
            {
                ss<<"call @" << GET_Da(x64) << " // <";
                //ss << find_method(GET_Da(x64)) << ">";
                
                break;
            }
            case op_CALLD:
            {
                ss<<"calld ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_NEWCLASS:
            {
                ss<<"new_class @" << GET_Da(x64);
                ss << " // "; ss << find_class(GET_Da(x64));
                
                break;
            }
            case op_MOVN:
            {
                ss<<"movn #" << GET_Da(x64);
                
                break;
            }
            case op_SLEEP:
            {
                ss<<"sleep ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_TEST:
            {
                ss<<"test ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_TNE:
            {
                ss<<"tne ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_LOCK:
            {
                ss<<"_lck ";
                
                break;
            }
            case op_ULOCK:
            {
                ss<<"_ulck";
                
                break;
            }
            case op_EXP:
            {
                ss<<"exp ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_MOVG:
            {
                ss<<"movg @"<< GET_Da(x64);
                ss << " // @"; ss << find_class(GET_Da(x64));
                
                break;
            }
            case op_MOVND:
            {
                ss<<"movnd ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_NEWOBJARRAY:
            {
                ss<<"newobj_arry ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_NOT: //c
            {
                ss<<"not ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_SKIP:// d
            {
                ss<<"skip @";
                ss<< GET_Da(x64);
                ss << " // pc = " << (x + GET_Da(x64));
                
                break;
            }
            case op_LOADVAL:
            {
                ss<<"loadval ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_SHL:
            {
                ss<<"shl ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                ss<< " -> ";
                ss<< registerToString(code.ir64.get(++x));
                
                break;
            }
            case op_SHR:
            {
                ss<<"shr ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                ss<< " -> ";
                ss<< registerToString(code.ir64.get(++x));
                
                break;
            }
            case op_SKPE:
            {
                ss<<"skpe ";
                ss<<GET_Da(x64);
                ss << " // pc = " << (x + GET_Da(x64));
                
                break;
            }
            case op_SKNE:
            {
                ss<<"skne ";
                ss<<GET_Da(x64);
                ss << " // pc = " << (x + GET_Da(x64));
                
                break;
            }
            case op_CMP:
            {
                ss<<"cmp ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_AND:
            {
                ss<<"and ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_UAND:
            {
                ss<<"uand ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_OR:
            {
                ss<<"or ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_XOR:
            {
                ss<<"xor ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_THROW:
            {
                ss<<"throw ";
                
                break;
            }
            case op_CHECKNULL:
            {
                ss<<"checknull";
                
                break;
            }
            case op_RETURNOBJ:
            {
                ss<<"returnobj";
                
                break;
            }
            case op_NEWCLASSARRAY:
            {
                ss<<"new_classarray ";
                ss<< registerToString(GET_Ca(x64));
                ss<< " ";
                ss << " // "; ss << find_class(GET_Cb(x64)) << "[]";
                
                break;
            }
            case op_NEWSTRING:
            {
                ss<<"newstr @" << GET_Da(x64) << " // ";
                //ss << getString(GET_Da(x64));
                
                break;
            }
            case op_ADDL:
            {
                ss<<"addl ";
                ss<< registerToString(GET_Ca(x64)) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_SUBL:
            {
                ss<<"subl ";
                ss<< registerToString(GET_Ca(x64)) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_MULL:
            {
                ss<<"mull ";
                ss<< registerToString(GET_Ca(x64)) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_DIVL:
            {
                ss<<"divl ";
                ss<< registerToString(GET_Ca(x64)) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_MODL:
            {
                ss<<"modl #";
                ss<< registerToString(GET_Ca(x64)) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_IADDL:
            {
                ss<<"iaddl ";
                ss<< GET_Ca(x64) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_ISUBL:
            {
                ss<<"isubl #";
                ss<< GET_Ca(x64) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_IMULL:
            {
                ss<<"imull #";
                ss<< GET_Ca(x64) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_IDIVL:
            {
                ss<<"idivl #";
                ss<< GET_Ca(x64) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_IMODL:
            {
                ss<<"imodl #";
                ss<< GET_Ca(x64) << ", @";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_LOADL:
            {
                ss<<"loadl ";
                ss<< registerToString(GET_Ca(x64)) << ", fp+";
                ss<<GET_Cb(x64);
                
                break;
            }
            case op_IALOAD_2:
            {
                ss<<"iaload_2 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_POPOBJ:
            {
                ss<<"popobj";
                
                break;
            }
            case op_SMOVR:
            {
                ss<<"smovr ";
                ss<< registerToString(GET_Ca(x64)) << ", sp+";
                if(GET_Cb(x64)<0) ss<<"[";
                ss<<GET_Cb(x64);
                if(GET_Cb(x64)<0) ss<<"]";
                
                break;
            }
            case op_SMOVR_2:
            {
                ss<<"smovr_2 ";
                ss<< registerToString(GET_Ca(x64)) << ", fp+";
                if(GET_Cb(x64)<0) ss<<"[";
                ss<<GET_Cb(x64);
                if(GET_Cb(x64)<0) ss<<"]";
                
                break;
            }
            case op_ANDL:
            {
                ss<<"andl ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_ORL:
            {
                ss<<"orl ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_XORL:
            {
                ss<<"xorl ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_RMOV:
            {
                ss<<"rmov ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_SMOV:
            {
                ss<<"smov ";
                ss<< registerToString(GET_Ca(x64)) << ", sp+";
                if(GET_Cb(x64)<0) ss<<"[";
                ss<<GET_Cb(x64);
                if(GET_Cb(x64)<0) ss<<"]";
                
                break;
            }
            case op_LOADPC_2:
            {
                ss<<"loadpc_2 ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", pc+";
                ss<< GET_Cb(x64);
                ss<< " // " << registerToString(GET_Ca(x64))
                  << " = " << (x + GET_Cb(x64));
                
                break;
            }
            case op_RETURNVAL:
            {
                ss<<"return_val ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_ISTORE:
            {
                ss<<"istore ";
                ss<< GET_Da(x64);
                
                break;
            }
            case op_ISTOREL:
            {
                ss<<"istorel ";
                ss<< code.ir64.get(++x) << ", fp+";
                ss<<GET_Da(x64);
                
                break;
            }
            case op_IPUSHL:
            {
                ss<<"ipushl #";
                ss<< GET_Da(x64);
                
                break;
            }
            case op_PUSHL:
            {
                ss<<"pushl ";
                ss<< GET_Da(x64);
                
                break;
            }
            case op_PUSHNIL:
            {
                ss<<"pushnil ";
                
                break;
            }
            case op_GET:
            {
                ss<<"get ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_ITEST:
            {
                ss<<"itest ";
                ss<< registerToString(GET_Da(x64));
                
                break;
            }
            case op_INVOKE_DELEGATE:
            {
                ss<<"invoke_delegate ";
                ss<< GET_Ca(x64);
                ss<< ", ";
                ss<< GET_Cb(x64);
                
                break;
            }
            case op_INVOKE_DELEGATE_STATIC:
            {
                ss<<"invoke_delegate_static ";
                ss<< registerToString(GET_Ca(x64));
                ss<< ", ";
                ss<< registerToString(GET_Cb(x64));
                
                break;
            }
            case op_ISADD:
            {
                ss<<"isadd ";
                ss<< GET_Ca(x64) << ", sp+";
                if(GET_Cb(x64)<0) ss<<"[";
                ss<<GET_Cb(x64);
                if(GET_Cb(x64)<0) ss<<"]";
                
                break;
            }
            case op_IPOPL:
            {
                ss<<"ipopl ";
                ss<< GET_Da(x64);
                
                break;
            }
            case op_SWITCH:
            {
                ss<<"switch ";
                ss<< GET_Da(x64);
                //ss<< " // " << getSwitchTable(method, GET_Da(x64));
                
                break;
            }
            case op_TLS_MOVL:
            {
                ss<<"tls_movl ";
                ss<< GET_Da(x64);
                
                break;
            }
            case op_DUP:
            {
                ss<<"dup ";
                
                break;
            }
            case op_POPOBJ_2:
            {
                ss<<"popobj2 ";
                
                break;
            }
            case op_SWAP:
            {
                ss<<"swap ";
                
                break;
            }
            default:
                ss << "? (" << GET_OP(x64) << ")";
                
                break;
        }

        ss << "\n";
    }

    return ss.str();
}

void Compiler::convertUtypeToNativeClass(Utype *clazz, Utype *paramUtype, IrCode &code, Ast *ast) {
    List<Field*> params;
    List<AccessFlag> flags;
    Method *constructor; // TODO: take note that I may not be doing pre equals validation on the param before I call this function

    flags.add(PUBLIC);
    Meta meta(current->getErrors()->getLine(ast->getEntity(0).getLine()), current->sourcefile,
              ast->getEntity(0).getLine(), ast->getEntity(0).getColumn());

    params.add(new Field(paramUtype->getResolvedType()->type, 0, "", currentScope()->klass, flags, meta, stl_stack, 0));
    params.get(0)->utype = paramUtype;

    if((constructor = clazz->getClass()->getConstructor(params, false)) != NULL) {
        validateAccess(constructor, ast);

        code.push_i64(SET_Di(i64, op_NEWCLASS, clazz->getClass()->address));

        paramUtype->getCode().inject(stackInjector);
        code.inject(paramUtype->getCode());
        code.push_i64(SET_Di(i64, op_CALL, constructor->address));
    } else {
        errors->createNewError(GENERIC, ast->line,  ast->col, "Support class `" + clazz->toString() + "` does not have constructor for type `"
                                                                          + paramUtype->toString() + "`");
    }

    params.get(0)->utype = NULL;
    freeListPtr(params);
}

bool Compiler::isUtypeConvertableToNativeClass(Utype *dest, Utype *src) {
    if(dest->getResolvedType() && src->getResolvedType()) {
        DataType type = src->getResolvedType()->type;

        if (isUtypeClass(dest, "std", 1, "string") && type == _INT8 && src->isArray()) {
            return true;
        } else if (isUtypeClass(dest, "std", 9, "int", "byte", "char", "bool", "short", "uchar", "ushort", "long", "ulong")) {
            return !src->isArray() && (type == VAR || (type >= _INT8 && type <= _UINT64));
        }
    }

    return false;
}

expression_type Compiler::utypeToExpressionType(Utype *utype) {
    if(utype->getType() == utype_field)
        return utypeToExpressionType(((Field*)utype->getResolvedType())->utype);
    else if(utype->getType() == utype_method)
        return exp_var;
    else if(utype->getType() == utype_method_prototype)
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
    }

    return exp_undefined;
}

void Compiler::compileVectorExpression(Expression* expr, Ast* ast, Utype *compareType) {
    List<Expression*> array;
    Ast* vectAst = ast->getSubAst(ast_vector_array);

    expr->utype->setArrayType(true);
    expr->ast = ast;
    compileExpressionList(array, vectAst);

    if(compareType && compareType->getType() != utype_class && compareType->getType() != utype_native)
        errors->createNewError(GENERIC, ast->line, ast->col, "arrayType `" + compareType->toString() + "` must be a class or a native type");

    if(array.size() > 1) {
        Utype *utype = compareType == NULL ? array.get(0)->utype : compareType, *elementUtype;
        expr->type = utypeToExpressionType(utype);
        expr->utype->copy(utype);
        if(expr->utype->getType() == utype_literal)
            expr->utype->setType(utype_native);
        expr->utype->setArrayType(true);

        // TODO: make this convert to object[] if elements dont match up correctly and dont allow single vars to be set in the array
        for (long i = 1; i < array.size(); i++) {
            elementUtype = array.get(i)->utype;
            if(elementUtype->getType() == utype_method_prototype) {
                errors->createNewError(GENERIC, ast->line, ast->col, "anonymous functions & lambdas are not allowed in array declarations");
            } else if (!utype->isRelated(elementUtype)) {
                errors->createNewError(GENERIC, ast->line, ast->col, "array element of type `" + elementUtype->toString() + "` is not equal to type `" + utype->toString() + "`");
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
        expr->utype->setArrayType(true);
    }

    if(array.size() >= 1) {
        expr->utype->getCode()
                .push_i64(SET_Di(i64, op_MOVI, array.size()), i64ebx);

        if(expr->type == exp_var)
            expr->utype->getCode().push_i64(SET_Di(i64, op_NEWARRAY, i64ebx));
        else if(expr->type == exp_class)
            expr->utype->getCode().push_i64(SET_Ci(i64, op_NEWCLASSARRAY, i64ebx, 0, expr->utype->getClass()->address));
        else if(expr->type == exp_object)
            expr->utype->getCode().push_i64(SET_Di(i64, op_NEWOBJARRAY, i64ebx));

        for(long i = 0; i < array.size(); i++) {
            if(expr->type == exp_var) {
                expr->utype->getCode().inject(array.get(i)->utype->getCode());
                expr->utype->getCode().inject(array.get(i)->utype->getCode().getInjector(ebxInjector));

                expr->utype->getCode()
                        .push_i64(SET_Di(i64, op_MOVSL, 0)) // get our array object
                        .push_i64(SET_Di(i64, op_MOVI, i), i64adx) // set element index
                        .push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx)); // ourArry[%adx] = %ebx
            }
            else if(expr->type == exp_class || expr->type == exp_object) {
                expr->utype->getCode().inject(array.get(i)->utype->getCode());
                expr->utype->getCode().inject(array.get(i)->utype->getCode().getInjector(stackInjector));

                expr->utype->getCode()
                   .push_i64(SET_Di(i64, op_MOVSL, -1)) // get our array object
                   .push_i64(SET_Di(i64, op_MOVN, i)) // select array element
                   .push_i64(SET_Ei(i64, op_POPOBJ)); // set object
            }
        }

        expr->utype->getCode().getInjector(ptrInjector)
                .push_i64(SET_Ei(i64, op_POPOBJ_2));
    } else {
        expr->utype->getCode()
                .push_i64(SET_Di(i64, op_MOVSL, 1))
                .push_i64(SET_Ei(i64, op_DEL));

        expr->utype->getCode().getInjector(stackInjector)
                .push_i64(SET_Ei(i64, op_PUSHOBJ));

    }

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
        expr->utype->getCode().push_i64(SET_Di(i64, op_NEWARRAY, i64ebx));
    else if(expr->type == exp_class)
        expr->utype->getCode().push_i64(SET_Ci(i64, op_NEWCLASSARRAY, i64ebx, 0, expr->utype->getClass()->address));
    else if(expr->type == exp_object)
        expr->utype->getCode().push_i64(SET_Di(i64, op_NEWOBJARRAY, i64ebx));
}

// usedfor assigning native vars with size constraints
__int64 Compiler::dataTypeToOpcode(DataType type) {
    switch (type) {
        case _INT8: return op_MOV8;
        case _INT16: return op_MOV16;
        case _INT32: return op_MOV32;
        case _INT64: return op_MOV64;
        case _UINT8: return op_MOVU8;
        case _UINT16: return op_MOVU16;
        case _UINT32: return op_MOVU32;
        case _UINT64: return op_MOVU64;
        default: return op_NOP;
    }
}

bool isFieldPair(KeyPair<Field*, bool> *pair, void *field) {
    return field == pair->key;
}

void Compiler::compileFieldInitialization(Expression* expr, List<KeyPair<Field*, bool>> &fields, Ast* ast) {
    if(ast->getType() == ast_expression_list && fields.size() < ast->getSubAstCount()) {
        stringstream ss;
        ss << "class `" << expr->utype->toString() << "` only contains a total of " << expr->utype->getClass()->totalFieldCount()
            << "fields but was supplied a total of `" << ast->getSubAstCount() << "` expressions";
        errors->createNewError(GENERIC, ast->line, ast->col, ss.str());
        return;
    }

    for(long i = 0; i < ast->getSubAstCount(); i++) {
        Ast *field_init = ast->getSubAst(i);
        bool baseField = false;

        if(field_init->hasEntity("base")) {
            baseField = true;
            if(expr->utype->getClass()->getSuperClass() == NULL) {
                baseField = false;
                errors->createNewError(GENERIC, ast->line, ast->col, "initializer class `" + expr->utype->getClass()->fullName + "` does not contain a base class");
            }
        }



        Expression assignExpression;
        compileExpression(&assignExpression, ast->getType() == ast_expression_list ? field_init : field_init->getSubAst(ast_expression));

        if(ast->getType() == ast_field_init_list) {
            Utype *utype;
            ClassObject* oldScope = currentScope()->klass;

            if(baseField)
                currentScope()->klass = expr->utype->getClass()->getSuperClass();
            else
                currentScope()->klass = expr->utype->getClass();

            RETAIN_BLOCK_TYPE(INSTANCE_BLOCK)

            utype = compileUtype(field_init->getSubAst(ast_utype));

            RESTORE_BLOCK_TYPE()
            currentScope()->klass = oldScope;

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

void Compiler::assignFieldInitExpressionValue(Field *field, Expression *assignExpr, IrCode *resultCode, Ast *ast) {
    if((field->utype->getClass() && assignExpr->utype->getClass() && assignExpr->utype->getClass()->isClassRelated(field->utype->getClass())
           && field->utype->isArray() == assignExpr->utype->isArray())
        || (field->utype->getClass() == NULL && assignExpr->utype->isRelated(field->utype))) {
        resultCode->inject(assignExpr->utype->getCode()); // TODO:...

        if(field->type >= _INT8 && field->type <= _UINT64) {
            if(field->isArray)
                goto object_assignment;
            resultCode->inject(assignExpr->utype->getCode().getInjector(ebxInjector));
            (*resultCode)
                    // first case ebx to appropriae value
                    .push_i64(SET_Ci(i64, dataTypeToOpcode(field->type), i64ebx, 0, i64ebx))
                    // get the created object
                    .push_i64(SET_Di(i64, op_MOVSL, 0))
                    //access the field
                    .push_i64(SET_Di(i64, op_MOVN, field->address))
                    // set the ebx vlue to the field
                    .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                    .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                    .push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx));
        } else if(field->type == VAR || field->type == FNPTR) {
            if(field->isArray)
                goto object_assignment;

            resultCode->inject(assignExpr->utype->getCode().getInjector(ebxInjector));
            (*resultCode)
                            // get the created object
                    .push_i64(SET_Di(i64, op_MOVSL, 0))
                            //access the field
                    .push_i64(SET_Di(i64, op_MOVN, field->address))
                            // set the ebx vlue to the field
                    .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                    .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                    .push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx));
        } else if(field->type == CLASS || field->type == OBJECT) {
            object_assignment:
            resultCode->inject(assignExpr->utype->getCode().getInjector(stackInjector));
            (*resultCode)
                            // get the created object
                    .push_i64(SET_Di(i64, op_MOVSL, -1))
                            //access the field
                    .push_i64(SET_Di(i64, op_MOVN, field->address))
                            // get value from stack & assign to field
                    .push_i64(SET_Ei(i64, op_POPOBJ));
        } else {
            errors->createNewError(GENERIC, ast->line, ast->col, " expression `" + assignExpr->utype->toString() + "` assigned to field `" + field->toString()
                                                                 + "` does not allow values to be assigned to it.");
        }
    } else {
        errors->createNewError(GENERIC, ast->line, ast->col, " expression `" + assignExpr->utype->toString() + "` assigned to field `" + field->toString()
                                                             + "` does not match th type of the field.");
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
    } else if(ast->hasSubAst(ast_expression_list) && ast->getSubAst(ast_expression_list)->hasEntity(LEFTPAREN)) {
        if(arrayType->getClass() && arrayType->getType() != utype_field) {
            if(!arrayType->getClass()->flags.find(EXTENSION)) {
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
                    expr->utype->getCode().push_i64(SET_Di(i64, op_NEWCLASS, expr->utype->getClass()->address));

                    for(long i = 0; i < params.size(); i++) {
                        Field *param = constr->params.get(i);
                        if(isUtypeConvertableToNativeClass(param->utype, params.get(i)->utype)) {
                            convertUtypeToNativeClass(param->utype, params.get(i)->utype, expr->utype->getCode(), ast);
                        } else {
                            params.get(i)->utype->getCode().inject(stackInjector);
                            expr->utype->getCode().inject(params.get(i)->utype->getCode());
                        }
                    }

                    expr->utype->getCode().push_i64(SET_Di(i64, op_CALL, constr->address));
                }

                freeListPtr(expressions);
                freeListPtr(params);
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col, "cannot instantiate extension class `" + arrayType->toString() + "`");
            }
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "arrayType `" + arrayType->toString() + "` must be a class");
    } else if(ast->hasSubAst(ast_field_init_list) || ast->hasSubAst(ast_expression_list)) {
        if(arrayType->getClass() && arrayType->getType() != utype_field) {
            if(!arrayType->getClass()->flags.find(EXTENSION)) {
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

                expr->utype->getCode().push_i64(SET_Di(i64, op_NEWCLASS, expr->utype->getClass()->address));
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

                compilePostAstExpressions(expr, ast, 2);
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col, "cannot instantiate extension class `" + arrayType->toString() + "`");
            }
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "arrayType `" + arrayType->toString() + "` must be a class");
    }

    compilePostAstExpressions(expr, ast, 2);
    RESTORE_TYPE_INFERENCE()
}

void Compiler::compileNullExpression(Expression* expr, Ast* ast) {
    DataEntity *de = new DataEntity();
    de->type = OBJECT;
    de->guid = guid++;
    expr->utype->setType(utype_native);
    expr->utype->setResolvedType(de);
    expr->utype->setNullType(true);

    expr->utype->getCode()
            .push_i64(SET_Di(i64, op_MOVSL, 1))
            .push_i64(SET_Ei(i64, op_DEL));

    expr->utype->getCode().getInjector(stackInjector)
            .push_i64(SET_Ei(i64, op_PUSHOBJ));
}

void Compiler::compileBaseExpression(Expression* expr, Ast* ast) {
    ClassObject* oldScope = currentScope()->klass;;
    currentScope()->klass = oldScope->getSuperClass();

    expr->ast = ast;
    if(currentScope()->klass != NULL) {
        if (ast->hasEntity(PTR)) {
            compileDotNotationCall(expr, ast->getSubAst(ast_dotnotation_call_expr));
        } else {
            expr->type = exp_class;
            expr->utype = new Utype(currentScope()->klass);
        }
    } else
        errors->createNewError(GENERIC, ast->line, ast->col, "scoped class `" + oldScope->fullName + "` does not have a base class to derive from");

    if(currentScope()->type == GLOBAL_SCOPE || currentScope()->type == STATIC_BLOCK)
        errors->createNewError(GENERIC, ast->line, ast->col, "illegal reference to self in static context");
    currentScope()->klass = oldScope;
}

void Compiler::compileSelfExpression(Expression* expr, Ast* ast) {

    expr->ast = ast;
    if(ast->hasEntity(PTR)) {
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
    expr->freeInjectors();
    if(ast->hasSubAst(ast_dot_fn_e)) {
        Ast* branch = ast->getSubAst(ast_dot_fn_e);
        Method* method = compileMethodUtype(expr, branch);

        if(method) {
            expr->type = utypeToExpressionType(method->utype);
            expr->utype->setResolvedType(method->utype->getResolvedType());
            expr->utype->setType(method->utype->getType());
            expr->utype->setArrayType(method->utype->isArray());

            if (method->utype->isArray() || method->utype->getResolvedType()->type == OBJECT
                || method->utype->getResolvedType()->type == CLASS) {
                expr->utype->getCode().getInjector(ptrInjector)
                        .push_i64(SET_Ei(i64, op_POPOBJ_2));
            } else if (method->utype->getResolvedType()->isVar()) {
                expr->utype->getCode().getInjector(ebxInjector)
                        .push_i64(SET_Di(i64, op_LOADVAL, i64ebx));
            }

        }
    } else {
        Utype *utype = compileUtype(ast->getSubAst(ast_utype), expr->utype->getCode().instanceCaptured);
        expr->utype->copy(utype);
        expr->copyInjectors(utype);
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
    RETAIN_BLOCK_TYPE(RESTRICTED_INSTANCE_BLOCK)
    if(ast->getSubAstCount() > 1) {
        for(long i = startPos; i < ast->getSubAstCount(); i++) {
            if(ast->getSubAst(i)->getType() == ast_post_inc_e)
                compilePostIncExpression(expr, false, ast->getSubAst(i));
            else if(ast->getSubAst(i)->getType() == ast_dotnotation_call_expr) {
                if(!expr->utype->getClass())
                    errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "expression of type `" + expr->utype->toString()
                                                                                                     + "` must resolve as a class");

                Expression bridgeExpr;
                bridgeExpr.utype->getCode().instanceCaptured = true;
                compileDotNotationCall(&bridgeExpr, ast->getSubAst(i));

                expr->copy(&bridgeExpr);
                expr->utype->getCode().inject(bridgeExpr.utype->getCode());
                expr->copyInjectors(bridgeExpr.utype);
            } else if(ast->getSubAst(i)->getType() == ast_arry_e)
                compileArrayExpression(expr, ast->getSubAst(i));
            else {
                errors->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "unexpected expression of type `" + Ast::astTypeToString(ast->getSubAst(i)->getType()) + "`");
            }

        }
    }
    RESTORE_BLOCK_TYPE()
}

void Compiler::compileNativeCast(Utype *utype, Expression *castExpr, Expression *outExpr) {

    if(utype->getResolvedType() != NULL && castExpr->type != exp_undefined) {
        if(utype->getResolvedType()->isVar()) {
            if (castExpr->utype->getClass() && IS_CLASS_ENUM(castExpr->utype->getClass()->getClassType())) {
                if(!utype->isArray()) {
                    Field *valueField = castExpr->utype->getClass()->getField("ordinal", true);
                    compileFieldType(valueField);

                    outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
                    outExpr->utype->getCode()
                             .push_i64(SET_Di(i64, op_MOVN, valueField->address))
                             .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                             .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx));

                    if(utype->getResolvedType()->type != VAR) {
                        outExpr->utype->getCode()
                                .push_i64(SET_Ci(i64, dataTypeToOpcode(utype->getResolvedType()->type), i64ebx, 0,
                                                 i64ebx));
                    }

                    outExpr->utype->getCode().getInjector(stackInjector)
                            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                } else goto castErr;
            } else if (castExpr->utype->getResolvedType()->type == OBJECT) {
                if(utype->isArray() && !castExpr->utype->isArray()) {
                    outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
                    outExpr->utype->getCode()
                            .push_i64(SET_Di(i64, op_VARCAST, 1));

                    outExpr->utype->getCode().getInjector(stackInjector)
                            .push_i64(SET_Ei(i64, op_PUSHOBJ));
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
                                outExpr->utype->getCode()
                                        .push_i64(
                                                SET_Ci(i64, dataTypeToOpcode(utype->getResolvedType()->type), i64ebx, 0,
                                                       i64ebx));
                            }
                        }

                        outExpr->utype->getCode().getInjector(stackInjector)
                                .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                    } else if (utype->equals(castExpr->utype)) {
                        createNewWarning(GENERIC, __WDECL, outExpr->ast->line, outExpr->ast->col, "redundant cast of type `" + castExpr->utype->toString()
                               + "` to `" + utype->toString() + "`");
                    } else goto castErr;
                } else goto castErr;
            } else goto castErr;
        } else if(utype->getResolvedType()->type == OBJECT) {
            if(!utype->isRelated(castExpr->utype))
                goto castErr;

            outExpr->utype->getCode().getInjector(stackInjector)
                    .inject(castExpr->utype->getCode().getInjector(stackInjector));
            outExpr->utype->getCode().getInjector(ptrInjector)
                    .inject(castExpr->utype->getCode().getInjector(ptrInjector));
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

                outExpr->utype->getCode().getInjector(stackInjector)
                        .inject(castExpr->utype->getCode().getInjector(stackInjector));
                outExpr->utype->getCode().getInjector(ptrInjector)
                        .inject(castExpr->utype->getCode().getInjector(ptrInjector));
                // we only assume its good if were casting downstream
                // any other casts need to be check explicitly by the VM at runtime
                // I.E. (_object_)new int(100)
                // we know that the int class obviously will have a superclass of type `_object_` so there's no need to do this check at runtime
                return;
            } else {
                // anything else MUST be checked at runtime!
                outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
                outExpr->utype->getCode()
                      .push_i64(SET_Di(i64, op_MOVI, utype->getClass()->address), i64cmt)
                      .push_i64(SET_Di(i64, op_CAST, i64cmt));

                outExpr->utype->getCode().getInjector(stackInjector)
                        .push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
        } else {
            goto castErr;
        }
    } else if(castExpr->utype->getResolvedType()->type == OBJECT) {
        outExpr->utype->getCode().inject(castExpr->utype->getCode().getInjector(ptrInjector));
        outExpr->utype->getCode()
              .push_i64(SET_Di(i64, op_MOVI, utype->getClass()->address), i64cmt)
              .push_i64(SET_Di(i64, op_CAST, i64cmt));

        outExpr->utype->getCode().getInjector(stackInjector)
                .push_i64(SET_Ei(i64, op_PUSHOBJ));
    } else {
        castErr:
        errors->createNewError(GENERIC, outExpr->ast->line, outExpr->ast->col, "unable to cast incompatible type of `" + castExpr->utype->toString()
                   + "` to type `" + utype->toString() + "`");
    }
}

void Compiler::compileCastExpression(Expression *expr, Ast *ast) {
    RETAIN_TYPE_INFERENCE(false)
    Utype *utype = compileUtype(ast->getSubAst(ast_utype));
    RESTORE_TYPE_INFERENCE()

    expr->ast = ast;
    expr->utype->copy(utype);
    expr->type = utypeToExpressionType(utype);

    Expression castExpr;
    compileExpression(&castExpr, ast->getSubAst(ast_expression));

    expr->utype->getCode().inject(castExpr.utype->getCode());
    if(expr->type != exp_undefined && utype->getType() != utype_unresolved) {
        if(utype->getType() == utype_class)
            compileClassCast(utype, &castExpr, expr);
        else if(utype->getType() == utype_native) {
            compileNativeCast(utype, &castExpr, expr);
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
                    .push_i64(SET_Ei(i64, op_POPOBJ_2))
                    .push_i64(SET_Ci(i64, op_MOVR, i64adx, 0, i64ebx))
                    .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                    .push_i64(SET_Di(i64, op_MOVND, i64adx));

            expr->freeInjectors();
            if(expr->utype->getResolvedType()->isVar()) {
                expr->utype->getCode().getInjector(ebxInjector)
                        .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                        .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx));

                expr->utype->getCode().getInjector(stackInjector)
                        .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                        .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx))
                        .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
            } else if(expr->utype->getResolvedType()->type == OBJECT || expr->utype->getResolvedType()->type == CLASS) {

                expr->utype->getCode().getInjector(stackInjector)
                        .push_i64(SET_Ei(i64, op_PUSHOBJ));
            } else
                errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + expr->utype->toString() + "` must be a var[], object[], or class[]");
        } else
            errors->createNewError(GENERIC, ast->line, ast->col, "index expression of type `" + arrayExpr.utype->toString() + "` was not found to be a var");
    } else {
            errors->createNewError(GENERIC, ast->line, ast->col, "expression of type `" + expr->utype->toString() + "` must be an array type");
    }

    compilePostAstExpressions(expr, ast);
}

void Compiler::compilePostIncExpression(Expression* expr, bool compileExpression, Ast* ast) {
    // use restricted insance for context expressions
    if(compileExpression)
        this->compileExpression(expr, ast->getSubAst(ast_expression));

    Token tok = ast->hasEntity(_INC) ? ast->getToken(_INC) : ast->getToken(_DEC);

    if(expr->utype->isArray()) {
        errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "expression of type `" + expr->utype->toString() + "` must return a var to use `" +
                                                                        tok.getValue() + "` operator");
    } else if(expr->utype->getType() != utype_unresolved){
        Method *overload;
        List<Field*> emptyParams;
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
                                                                                + tok.getValue() + "` operator try `((Type)<your-expression>)++` instead");
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
                if(tok == "++")
                    expr->utype->getCode().push_i64(SET_Di(i64, op_INC, i64ebx));
                else
                    expr->utype->getCode().push_i64(SET_Di(i64, op_DEC, i64ebx));

                if(expr->utype->getResolvedType()->type <= _UINT64) {
                    expr->utype->getCode()
                            .push_i64(SET_Ci(i64, dataTypeToOpcode(expr->utype->getResolvedType()->type), i64ebx, 0,
                                             i64ebx));
                }

                expr->freeInjectors();
                expr->utype->getCode().getInjector(stackInjector)
                        .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                break;

            case CLASS:
                if((overload = findFunction((ClassObject*)expr->utype->getResolvedType(), "operator" + tok.getValue(),
                                            emptyParams, ast, true, fn_op_overload)) != NULL) {
                    validateAccess(overload, ast);
                    expr->utype->copy(overload->utype);

                    expr->utype->getCode().push_i64(SET_Di(i64, op_CALL, overload->address));
                } else {
                    errors->createNewError(GENERIC, tok.getLine(), tok.getColumn(), "call to function `" + expr->utype->toString() + "` must return an var or class to use `" + tok.getValue() + "` operator");
                }

                expr->freeInjectors();
                if (expr->utype->isArray() || expr->utype->getResolvedType()->type == OBJECT
                    || expr->utype->getResolvedType()->type == CLASS) {
                    expr->utype->getCode().getInjector(ptrInjector)
                            .push_i64(SET_Ei(i64, op_POPOBJ_2));
                } else if (expr->utype->getResolvedType()->isVar()) {
                    expr->utype->getCode().getInjector(ebxInjector)
                            .push_i64(SET_Di(i64, op_LOADVAL, i64ebx));
                }
                break;
        }
    }

    compilePostAstExpressions(expr, ast);
}

Field* Compiler::compileLambdaArg(Ast *ast) {
    string name = ast->getEntity(0).getValue();
    Utype *utype = NULL;

    RETAIN_TYPE_INFERENCE(false)
    if(ast->hasSubAst(ast_utype))
        utype = compileUtype(ast->getSubAst(ast_utype));
    RESTORE_TYPE_INFERENCE()

    List<AccessFlag> fieldFlags;
    fieldFlags.add(PUBLIC);

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
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
        if(ast->getSubAst(i)->getType()==ast_func_prototype) {
            fields.add(compileFuncPrototypeArg(ast->getSubAst(i)));
        } else {
            fields.add(compileLambdaArg(ast->getSubAst(i)));
        }
    }
}

Method* Compiler::findLambdaByAst(Ast *ast) {
    for(long i = 0; i < lambdas.size(); i++) {
        if(lambdas.get(i)->ast == ast)
            return lambdas.get(i);
    }

    return NULL;
}

Utype* Compiler::compileLambdaReturnType(Ast* ast) {
    if(ast->hasEntity("nil")) {
        Utype *utype = new Utype();
        utype->setType(utype_native);

        Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
                  ast->line, ast->col);

        DataEntity *_void = new DataEntity();
        _void->owner = findClass(currModule, globalClass, classes);
        _void->type = NIL;
        _void->meta.copy(meta);

        utype->setResolvedType(_void);
        return utype;
    }

    return compileUtype(ast->getSubAst(ast_utype));
}

void Compiler::compileLambdaExpression(Expression* expr, Ast* ast) {
    Method *lambda = NULL;

    expr->ast = ast;
    if((lambda = findLambdaByAst(ast)) == NULL) {
        List<Field*> fields;
        compileLambdaArgList(fields, ast->getSubAst(ast_lambda_arg_list));

        List<AccessFlag> flags;
        flags.add(PRIVATE);
        flags.add(STATIC);

        Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
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
    }

    expr->type = exp_var;
    expr->utype->setType(utype_method_prototype);
    expr->utype->setArrayType(false);
    expr->utype->setNullType(false);
    expr->utype->setResolvedType(lambda);

    expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, lambda->address), i64ebx);
    expr->utype->getCode().getInjector(stackInjector)
            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));

    if(processingStage > POST_PROCESSING) {
        // Todo:...
    }
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
        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, ((Literal*)sizeExpr.utype->getResolvedType())->stringData.size()), i64ebx);
    } else if(sizeExpr.utype->getType() == utype_class || sizeExpr.utype->getType() == utype_field){
        expr->utype->getCode().inject(sizeExpr.utype->getCode().getInjector(ptrInjector));
        expr->utype->getCode().push_i64(SET_Di(i64, op_SIZEOF, i64ebx));
    } else {
        errors->createNewError(GENERIC, expr->ast, "cannot get sizeof from expression of type `" + sizeExpr.utype->toString() + "`");
    }

    expr->utype->getCode().getInjector(stackInjector)
            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));

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
            return compileCastExpression(expr, branch);
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
    Ast *branch = ast->getSubAst(0);

    switch(branch->getType()) {
        case ast_primary_expr:
            compilePrimaryExpression(expr, branch);
            break;
        case ast_vect_e:
            compileVectorExpression(expr, branch);
            break;
    }

    if(ast->getSubAstCount() > 1) {
        return this->errors->createNewError(GENERIC, ast->getSubAst(1)->line, ast->getSubAst(1)->col,
                                            "unexpected malformed expression found");
    }
}

bool Compiler::isLambdaFullyQualified(Method *lambda) {
    for(long i = 0; i < lambda->params.size(); i++) {
        Field *param = lambda->params.get(i);

        if(param->type == ANY || param->utype == NULL)
            return false;
    }

    return true;
}

void Compiler::resolvePrototypeField(Ast* ast) {
    string name = ast->getEntity(0).getValue();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getEntity(1).getValue();
    }

    Field *field = currentScope()->klass->getField(name, false);
    if(field->type == UNTYPED) {
        Method* prototype = new Method();
        prototype->name = field->name;
        prototype->fullName = field->fullName;
        prototype->owner = currentScope()->klass;
        prototype->fnType = fn_prototype;
        prototype->module = currModule;
        prototype->guid = guid++;
        prototype->ast = ast;
        prototype->flags.add(PUBLIC);
        prototype->flags.add(STATIC);
        field->utype = new Utype();
        field->utype->setResolvedType(prototype);
        field->utype->setType(utype_method_prototype);
        field->type = FNPTR;
        if (field->locality == stl_stack) {
            field->address = field->owner->getFieldAddress(field);
        }

        Meta meta(current->getErrors()->getLine(ast->getEntity(0).getLine()), current->sourcefile,
                  ast->getEntity(0).getLine(), ast->getEntity(0).getColumn());
        prototype->meta.copy(meta);

        if(ast->getSubAst(ast_utype_arg_list_opt)) {
            List<Field*> params;
            parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list_opt));
            validateMethodParams(params, ast->getSubAst(ast_utype_arg_list_opt));

            prototype->params.addAll(params);

            if(ast->hasSubAst(ast_method_return_type)) {
                if(ast->findEntity("nil"))
                    goto void_;

                Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type));
                if(utype->getType() != utype_native && utype->getType() != utype_class) {
                    this->errors->createNewError(GENERIC, ast->line, ast->col, " illegal use of symbol `" + utype->toString() + "` as a return type");
                }

                prototype->utype = utype;
            } else {
                void_:
                prototype->utype = new Utype();
                prototype->utype->setType(utype_native);

                DataEntity *_void = new DataEntity();
                _void->owner = prototype->owner;
                _void->type = NIL;
                _void->meta.copy(meta);

                prototype->utype->setResolvedType(_void);
            }
        } else {
            // we need to infer the type
            cout << ast->toString() << std::flush;
            Expression *expr = new Expression();
            compileExpression(expr, ast->getSubAst(ast_value)->getSubAst(ast_expression));
            Utype *utype = expr->utype;
            if(utype->getType() == utype_field)
                utype = ((Field*)utype->getResolvedType())->utype;

            if(utype->getType() == utype_method || (utype->getType() == utype_method_prototype && ((Method*)utype->getResolvedType())->fnType != fn_lambda)) {
                prototype->params.addAll(((Method*)utype->getResolvedType())->params);
                prototype->utype = ((Method*)utype->getResolvedType())->utype;
            } else if(utype->getType() == utype_method_prototype && ((Method*)utype->getResolvedType())->fnType != fn_lambda) {
                Method *lambda = (Method*)utype->getResolvedType();
                if(isLambdaFullyQualified(lambda)) {
                    compileMethodReturnType(lambda, lambda->ast);

                    prototype->params.addAll(lambda->params);
                    prototype->utype = lambda->utype;
                } else {
                    errors->createNewError(GENERIC, ast->line, ast->col, "expression being assigned to field `" + name + "` is not a fully qualified lambda expression. Try assigning types to all of the fields so that the compiler can correctly resolve the field.");
                }
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col, "expression being assigned to field `" + name + "` must be either a method or anonymous function");
                prototype->utype = new Utype();
                prototype->utype->setType(utype_native);

                DataEntity *_void = new DataEntity();
                _void->owner = prototype->owner;
                _void->type = NIL;
                _void->meta.copy(meta);

                prototype->utype->setResolvedType(_void);
            }
        }
    }
}

void Compiler::resolveFieldType(Field* field, Utype *utype, Ast* ast) {

    field->isArray = utype->isArray();
    if(utype->getType() == utype_class) {
        field->type = CLASS;
        field->utype = utype;
    } else if(utype->getType() == utype_method) {
        if(typeInference) {
            field->type = VAR;
            field->utype = new Utype(VAR);
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " illegal use of function `" + utype->toString() + "` as a type");
    } else if(utype->getType() == utype_native) {
        if(utype->getResolvedType()->type >= NIL)
            this->errors->createNewError(GENERIC, ast->line, ast->col, " field`" + field->fullName + "` cannot be assigned type `" + utype->toString() + "` due to invalid type assignment format");
        field->type = utype->getResolvedType()->type;
        field->utype = utype;
    } else if(utype->getType() == utype_literal) {
        if(typeInference) {
            field->type = utype->getResolvedType()->type;
            field->utype = new Utype(utype->getResolvedType()->type, utype->isArray());
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " field`" + field->fullName + "` cannot be assigned type `" + utype->toString() + "` due to invalid type assignment format");
    } else if(utype->getType() == utype_field) {
        if(typeInference) {
            if(((Field*)utype->getResolvedType()) == field) {
                this->errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->fullName + "` cannot be self-initialized");
            } else if(((Field*)utype->getResolvedType())->utype->getType() == utype_method_prototype) {
                field->type = VAR;
                field->utype = new Utype(VAR);
            } else {
                Field *fieldType = ((Field*)utype->getResolvedType());
                RETAIN_TYPE_INFERENCE(false)
                resolveFieldType(field,fieldType->utype, ast);
                RESTORE_TYPE_INFERENCE()
            }
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " illegal use of field `" + utype->toString() + "` as a type");
    } else
        this->errors->createNewError(GENERIC, ast->line, ast->col, " field `" + field->fullName + "` cannot be assigned type `" + utype->toString() + "` due to invalid type assignment format");

    if(field->utype == NULL) {
        field->utype = new Utype(UNDEFINED);
        field->utype->setType(utype_unresolved);
        field->type = UNDEFINED;
    }
}

void Compiler::resolveAlias(Ast* ast) {
    string name = ast->getEntity(0).getValue();

    if(resolveClass("", name, ast))
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col, "declared alias `" + name + "` shadows class at higher scope");

    Alias *alias = currentScope()->klass->getAlias(name, false);
    if(alias->type == UNTYPED) {
        // wee need to do this to prevent possible stack overflow errors

        RETAIN_BLOCK_TYPE(CLASS_SCOPE)
        RETAIN_TYPE_INFERENCE(false)
        Utype *utype = compileUtype(ast->getSubAst(ast_utype));
        RESTORE_TYPE_INFERENCE()
        RESTORE_BLOCK_TYPE()

        alias->type = utype->getResolvedType() ? utype->getResolvedType()->type : UNDEFINED;
        alias->utype = utype;
    }
}

void Compiler::resolveField(Ast* ast) {
    string name = ast->getEntity(0).getValue();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getEntity(1).getValue();
    }

    if(resolveClass("", name, ast))
        createNewWarning(GENERIC, __WDECL, ast->line, ast->col, "declared field `" + name + "` shadows class at higher scope");

   Field *field = currentScope()->klass->getField(name, false);
    if(field->type == UNTYPED) {
        // wee need to do this to prevent possible stack overflow errors
        field->type = UNDEFINED;
        field->utype = new Utype(UNDEFINED);

        if (field->locality == stl_stack) {
            field->address = field->owner->getFieldAddress(field);
        }

        if (ast->hasEntity(COLON)) {
            Utype *utype = compileUtype(ast->getSubAst(ast_utype));
            // very nasty thing we have to do to avoid seg fault
            freePtr(field->utype); field->utype = NULL;
            resolveFieldType(field, utype, ast);
        } else if (ast->hasEntity(INFER)) {
            Expression expr;
            BlockType oldScope = currentScope()->type;
            if(field->flags.find(STATIC))
                currentScope()->type = STATIC_BLOCK;
            RETAIN_TYPE_INFERENCE(true)
            cout << ast->toString() << std::flush;
            compileExpression(&expr, ast->getSubAst(ast_expression));
            if(field->flags.find(STATIC))
                currentScope()->type = oldScope;

            printExpressionCode(&expr);
            freePtr(field->utype); field->utype = NULL;
            resolveFieldType(field, expr.utype, ast);
           RESTORE_TYPE_INFERENCE()
        }

        if(field->type != UNTYPED) {
            if (ast->hasSubAst(ast_setter)) {
                resolveSetter(ast->getSubAst(ast_setter), field);
            }

            if (ast->hasSubAst(ast_getter)) {
                resolveGetter(ast->getSubAst(ast_getter), field);
            }
        }

        // TODO: make sure type is valid before exiting, you cant have a method as the type for the field etc.
        if(ast->hasSubAst(ast_variable_decl)) {
            for(long i = 0; i < ast->getSubAstCount(); i++) {
                Ast *branch = ast->getSubAst(i);

                name = branch->getEntity(0).getValue();
                Field *xtraField = currentScope()->klass->getField(name, false);

                // we cannot add thread local to secondary variables so me must match its locality with the first one
                xtraField->locality = field->locality;
                if(xtraField->locality == stl_stack)
                    xtraField->address = xtraField->owner->getFieldAddress(xtraField);

                if(xtraField->type == UNTYPED) {
                    resolveFieldType(xtraField, field->utype, ast);
                }
            }
        }
    }
}

void Compiler::resolveSetter(Ast *ast, Field *field) {
    List<Field*> params;
    List<AccessFlag> fieldFlags;
    params.add(field);

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
              ast->line, ast->col);

    Field *arg0 = new Field(CLASS, guid++, ast->getEntity(1).getValue(), field->owner, fieldFlags, meta, stl_stack, 0);
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
    else
        flags.add(PUBLIC);
    if(field->flags.find(STATIC))
        flags.add(STATIC);

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
              ast->line, ast->col);

    if(field->flags.find(flg_CONST)) {
        createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "field `" + field->name + "` has access specifier `const`. I would not recommend adding getters to constant fields, "
                                                                                                          "as the compiler is no longer able to inline the value.");
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
        string name = ast->getEntity(0).getValue();
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
            case ast_class_decl:
                resolveClassFields(branch);
                break;
            case ast_variable_decl:
                resolveField(branch);
                break;
            case ast_alias_decl:
                resolveAlias(branch);
                break;
            case ast_func_prototype:
                resolvePrototypeField(branch);
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

void Compiler::parseMethodAccessFlags(List<AccessFlag> &flags, Ast *ast) {
    long errPos=0;
    for(long i = 0; i < ast->getEntityCount(); i++) {
        flags.add(strToAccessFlag(ast->getEntity(i).getValue()));

        if(flags.last() == LOCAL && !globalScope()) {
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                         " `" + ast->getEntity(i).getValue() + "` can only be used at global scope");
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
        this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getEntity(errPos).getLine(),
                                     ast->getEntity(errPos).getColumn(), " `" + ast->getEntity(errPos).getValue() + "`");
    }
}

Field* Compiler::compileUtypeArg(Ast* ast) {
    Field *arg = new Field();
    Utype* utype = compileUtype(ast->getSubAst(ast_utype));

    if(utype && utype->getType() != utype_unresolved) {
        arg->isArray = utype->isArray();
        arg->type = utype->getResolvedType()->type;
    } else
        arg->type = UNDEFINED;

    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->sourcefile,
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);
    arg->meta.copy(meta);

    // for optional utype args
    if(ast->getEntityCount() != 0) {
        arg->name = ast->getEntity(0).getValue();
        arg->fullName = arg->name;
    } else {
        stringstream ss;
        ss << "arg" << guid++;
        arg->name = ss.str();
        arg->fullName = arg->name;
    }

    resolveFieldType(arg, utype, ast);
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
            errors->createNewError(SYMBOL_ALREADY_DEFINED, ast->line, ast->col, "symbol `" + params.get(i)->toString() + "` already defined in the scope");
        } else {
            if(params.get(i)->name == "" && ast->getType() == ast_utype_arg_list_opt) {
                stringstream ss;
                ss << "arg" << i;
                params.get(i)->name = ss.str();
                params.get(i)->fullName = ss.str();
            }
        }
    }
}

Field* Compiler::compileFuncPrototypeArg(Ast *ast) {
    Field* field = new Field();
    field->name = ast->getEntity(0).getValue();
    field->type = FNPTR;
    field->fullName = field->name;
    field->module = currModule;
    field->guid = guid++;
    field->ast = ast;
    field->flags.add(PUBLIC);
    field->utype = new Utype();
    field->utype->setType(utype_method_prototype);
    field->owner = currentScope()->klass;

    Meta meta(current->getErrors()->getLine(ast->getEntity(0).getLine()), current->sourcefile,
              ast->getEntity(0).getLine(), ast->getEntity(0).getColumn());
    field->meta.copy(meta);

    Method* prototype = new Method();
    prototype->name = field->name;
    prototype->fullName = field->fullName;
    prototype->type = METHOD;
    prototype->owner = currentScope()->klass;
    prototype->fnType = fn_prototype;
    prototype->module = currModule;
    prototype->guid = guid++;
    prototype->ast = ast;
    prototype->flags.add(PUBLIC);
    field->utype->setResolvedType(prototype);

    if(ast->getSubAst(ast_utype_arg_list_opt)) {
        List<Field*> params;
        parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list_opt));
        validateMethodParams(params, ast->getSubAst(ast_utype_arg_list_opt));

        prototype->params.addAll(params);
    }

    if(ast->hasSubAst(ast_method_return_type)) {
        compileMethodReturnType(prototype, ast);
    } else {
        prototype->utype = new Utype();
        prototype->utype->setType(utype_native);

        DataEntity *_void = new DataEntity();
        _void->owner = prototype->owner;
        _void->type = NIL;
        _void->meta.copy(meta);

        prototype->utype->setResolvedType(_void);
    }
    return field;
}

void Compiler::parseUtypeArgList(List<Field*> &params, Ast* ast) {
    for(unsigned int i = 0; i < ast->getSubAstCount(); i++) {
        if(ast->getSubAst(i)->getType()==ast_func_prototype) {
            params.add(compileFuncPrototypeArg(ast->getSubAst(i)));
        } else {
            params.add(compileUtypeArg(ast->getSubAst(i)));
        }
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
                    if(!hasMainMethod) {
                        mainMethod = method;
                        mainSignature = 0;
                    } else {
                        errors->createNewError(GENERIC, method->ast, "main method with the same or different signature already exists");
                        printNote(mainMethod->meta, "method `main` previously defined here");
                    }

                } else if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == NIL) { // fn main(string[]);
                    if(!hasMainMethod) {
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
                    if(!hasMainMethod) {
                        mainMethod = method;
                        mainSignature = 2;
                    } else {
                        errors->createNewError(GENERIC, method->ast, "main method with the same or different signature already exists");
                        printNote(mainMethod->meta, "method `main` previously defined here");
                    }
                } else if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == VAR) { // fn main() : var;
                    if(!hasMainMethod) {
                        mainMethod = method;
                        mainSignature = 3;
                    } else {
                        errors->createNewError(GENERIC, method->ast, "main method with the same or different signature already exists");
                        printNote(mainMethod->meta, "method `main` previously defined here");
                    }

                } else
                    createNewWarning(GENERIC, __WMAIN, method->ast->line, method->ast->col, "main method might not be executed");
            }
        }
    }
}

void Compiler::resolveOperatorOverload(Ast* ast) {
    List<AccessFlag> flags;

    if (ast->hasSubAst(ast_access_type)) {
        parseMethodAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
    }

    List<Field*> params;
    string name = ast->getEntity(0).getValue() + ast->getEntity(1).getValue();
    string op = ast->getEntity(1).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
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

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
    }

    List<Field*> params;
    string name = ast->getEntity(0).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_constructor;
    method->address = methodSize++;
    method->utype = new Utype(currentScope()->klass);

    if(name == currentScope()->klass->name) {
        if (!addFunction(currentScope()->klass, method, &simpleParameterMatch)) {
            this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                         "constructor `" + name + "` is already defined in the scope");
            printNote(findFunction(currentScope()->klass, method, &simpleParameterMatch)->meta,
                      "constructor `" + name + "` previously defined here");

            method->free();
            delete method;
        }
    } else
        errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "constructor `" + name + "` must be the same name as its parent class");
}

void Compiler::resolveDelegateImpl(Ast* ast) {
    List<AccessFlag> flags;

    if (ast->hasSubAst(ast_access_type)) {
        parseMethodAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
    }

    List<Field*> params;
    string name = ast->getEntity(0).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_delegate_impl;
    method->address = methodSize++;

    compileMethodReturnType(method, ast, true);
    checkMainMethodSignature(method);
    if(!addFunction(currentScope()->klass, method, &simpleParameterMatch)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "function `" + name + "` is already defined in the scope");
        printNote(findFunction(currentScope()->klass, method, &simpleParameterMatch)->meta, "function `" + name + "` previously defined here");

        method->free();
        delete method;
    }
}

void Compiler::resolveDelegate(Ast* ast) {
    List<AccessFlag> flags;

    if (ast->hasSubAst(ast_access_type)) {
        parseMethodAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
    }

    List<Field*> params;
    string name = ast->getEntity(0).getValue();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_delegate;
    method->address = delegateGUID++;

    if(ast->hasSubAst(ast_method_return_type)) {
        if(ast->findEntity("nil"))
            goto void_;

        Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
        method->utype = utype;
    } else {
        void_:
        method->utype = new Utype();
        method->utype->setType(utype_native);

        DataEntity *_void = new DataEntity();
        _void->owner = method->owner;
        _void->type = NIL;
        _void->meta.copy(meta);

        method->utype->setResolvedType(_void);
    }

    checkMainMethodSignature(method);
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
            if (ast->getSubAst(ast_method_return_type)->findEntity("nil"))
                goto void_;

            // TODO: dont just let anything be the return type
            Utype *utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
            fun->utype = utype;
        } else if (ast->findEntity(":=")) {
            if (!wait) {
                Expression e;
                compileExpression(&e, ast->getSubAst(ast_expression));
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
            fun->utype = new Utype();
            fun->utype->setType(utype_native);

            DataEntity *_void = new DataEntity();
            _void->owner = fun->owner;
            _void->type = NIL;
            _void->meta.copy(fun->meta);

            fun->utype->setResolvedType(_void);
        }
    }
}

ClassObject* Compiler::getExtensionFunctionClass(Ast* ast) {

    if(ast->getType() == ast_method_decl) {
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
        // our method is most likeley a normal global function
        resolveMethod(ast);
    } else { // looks like we have an extension function!!
        // god classes must update all its generic classes created under it
        if(IS_CLASS_GENERIC(resolvedClass->getClassType()) && resolvedClass->getGenericOwner() == NULL) {
            resolvedClass->getExtensionFunctionTree().add(ast);
        } else // its can be a generic class that was already created or a reg. class
            resolveMethod(ast);
    }
}

// TODO: make sure extension functions can process types properly like def List.last() : T { ... }
void Compiler::resolveMethod(Ast* ast, ClassObject* currentClass) {
    List<AccessFlag> flags;

    if (ast->hasSubAst(ast_access_type)) {
        parseMethodAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(globalScope()) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(globalScope()) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
    }

    List<Field*> params;
    ReferencePointer ptr;
    string name;
    bool extensionFunction = false;

    if(currentClass == NULL) {
        currentClass = currentScope()->klass;

        compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
        if (ptr.mod == "" && ptr.classes.singular()) {
            name = ptr.classes.last();
        } else {
            extensionFunction = true;
            if (!globalScope())
                errors->createNewError(GENERIC, ast->line, ast->col,
                                       "extension functions can only be created at global scope");

            if (ptr.mod != "" && ptr.classes.singular()) {
                currentClass = resolveClass(ptr.mod, globalClass, ast);
                name = ptr.classes.last();
                flags.addif(STATIC);
            } else {
                flags.removefirst(STATIC);
                name = ptr.classes.last();
                ptr.classes.pop_back();
                currentClass = resolveClassReference(ast->getSubAst(ast_refrence_pointer), ptr, true);
            }
        }
    } else {
        extensionFunction = true;
        compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
        name = ptr.classes.last();

        flags.removefirst(STATIC);
        flags.addif(PUBLIC);
        flags.removefirst(PRIVATE);
        flags.removefirst(PROTECTED);
    }

    if(currentClass != NULL) {
        if(extensionFunction && currentClass->flags.find(STABLE))
            errors->createNewError(GENERIC, ast->line, ast->col, "extension functions are not allowed on stable class `" + currentClass->fullName + "`");

        parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
        validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

        Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
                  ast->line, ast->col);

        Method *method = new Method(name, currModule, currentClass, params, flags, meta);
        method->fullName = currentClass->fullName + "." + name;
        method->ast = ast;
        method->fnType = fn_normal;
        method->address = methodSize++;

        compileMethodReturnType(method, ast, true);

        checkMainMethodSignature(method);
        if (!addFunction(currentClass, method, &simpleParameterMatch)) {
            this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                         "function `" + name + "` is already defined in the scope");
            printNote(findFunction(currentClass, method, &simpleParameterMatch)->meta,
                      "function `" + name + "` previously defined here");

            method->free();
            delete method;
        }
    } else
        errors->createNewError(GENERIC, ast->line, ast->col, "could not locate class for extension function `" + name + "`");
}

void Compiler::resolveClassMethods(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

    if(currentClass == NULL) {
        string name = ast->getEntity(0).getValue();
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
            case ast_class_decl:
                resolveClassMethods(branch);
                break;
            case ast_interface_decl:
                resolveClassMethods(branch);
                break;
            case ast_variable_decl:
                break;
            case ast_func_prototype:
                break;
            case ast_method_decl:
                resolveMethod(branch);
                break;
            case ast_operator_decl:
                resolveOperatorOverload(branch);
                break;
            case ast_construct_decl:
                resolveConstructor(branch);
                break;
            case ast_delegate_impl:
                resolveDelegateImpl(branch);
                break;
            case ast_delegate_decl:
                resolveDelegate(branch);
                break;
            case ast_enum_decl:
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

/**
 * As of the curent version delegates work as follows:
 * class Engine {
 *   def delegate::run();
 * }
 *
 * class 4Cylinder : Engine {
 *   def delegate::run() { ... }
 * }
 *
 * class 8Cylinder : Engine {
 *   def delegate::run() { ... }
 * }
 *
 * def main() {
 *    Engine v8 = new 8Cylinder();
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

    subscriber->getAllFunctionsByType(fn_delegate_impl, subscribedMethods);
    ClassObject* contracter;

    if(subscriber->guid == 20) {
        int i = 0;
    }
    // we want to add all the delegates we need to validate
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

    // we do this to ensure no bugs will come from searching methods
    for(long long i = 0;   i < contractedMethods.size(); i++) {
        compileMethodReturnType(contractedMethods.get(i), contractedMethods.get(i)->ast);
    }

    for(long long i = 0;   i < subscribedMethods.size(); i++) {
        compileMethodReturnType(subscribedMethods.get(i), subscribedMethods.get(i)->ast);
    }

    // First check that all subscribed methods have a contract attached to them
    for(long long i = 0; i < subscribedMethods.size(); i++) {
        Method* contract, *sub = subscribedMethods.get(i);
        if((contract = validateDelegatesHelper(sub, contractedMethods)) != NULL) {
            if((sub->utype != NULL && contract->utype != NULL) || (sub->utype == NULL && contract->utype == NULL)) {
                if(sub->utype != NULL) {
                    if(!sub->utype->equals(contract->utype)) {
                        goto return_type_err;
                    }
                }
            } else {
                return_type_err:
                stringstream err;
                err << "method `" << sub->toString() << "` return type does not match that of contract '"
                    << contract->toString() << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                printNote(contract->meta, "as defined here");
            }

            if(!(contract->flags.size() == sub->flags.size() && contract->flags.sameElements(sub->flags))) {
                stringstream err;
                err << "method `" << sub->toString() << "` provided different access privileges than contract method '"
                    << contract->toString() << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                printNote(contract->meta, "as defined here");
            }
        } else {
            stringstream err;
            err << "no contract found in class `" << subscriber->fullName << "` for method '"
                << subscribedMethods.get(i)->toString() << "'";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            printNote(contract->meta, "as defined here");
        }
    }

    // Lastly we need to verify that we have implemented all of our contract functions
    for(long long i = 0; i < contractedMethods.size(); i++) {
        Method* contract = contractedMethods.get(i), *sub;
        if((sub = validateDelegatesHelper(contract, subscribedMethods)) != NULL) {
            if((sub->utype != NULL && contract->utype != NULL) || (sub->utype == NULL && contract->utype == NULL)) {
                if(contract->utype != NULL) {
                    if(!contract->utype->equals(sub->utype)) {
                        goto subscribe_err;
                    }
                }
            } else {
                subscribe_err:
                stringstream err;
                err << "contract method `" << contract->toString() << "` does not have a subscribed method implemented in class '"
                    << subscriber->fullName << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                printNote(contract->meta, "as defined here");
            }

            if(!(contract->flags.size() == sub->flags.size() && contract->flags.sameElements(sub->flags))) {
                stringstream err;
                err << "method `" << sub->toString() << "` provided different access privileges than contract method '"
                    << contract->toString() << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                printNote(contract->meta, "as defined here");
            }
        } else {
            stringstream err;
            err << "contract method `" << contract->toString() << "` does not have a subscribed method implemented in class '"
                << subscriber->fullName << "'";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            printNote(contract->meta, "as defined here");
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
        string name = ast->getEntity(0).getValue();
        if(globalScope()) {
            currentClass = findClass(currModule, name, classes);
        }
        else {
            currentClass = currentScope()->klass->getChildClass(name);
        }
    }

    validateDelegates(currentClass, ast);
    currScope.add(new Scope(currentClass, CLASS_SCOPE));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        Ast* branch = block->getSubAst(i);

        switch(branch->getType()) {
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

void Compiler::resolveAllDelegates() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";

        sourceFiles.addif(current->sourcefile);
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    modules.addif(currModule = "__srt_undefined");
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    errors->createNewError(GENERIC, branch->line, branch->col, "module declaration must be "
                                                                               "first in every file");
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


        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void Compiler::resolveClassMutateMethods(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);
    if(IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
        // do nothing it's allready been processed
    } else {
        resolveClassMethods(ast, currentClass);
    }
}

void Compiler::resolveDelegateMutateMethods(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);
    if(IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
        // do nothing it's allready been processed
    } else {
        resolveAllDelegates(ast, currentClass);
    }
}

void Compiler::resolveAllMethods() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";

        sourceFiles.addif(current->sourcefile);
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
                    modules.addif(currModule = "__srt_undefined");
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    errors->createNewError(GENERIC, branch->line, branch->col, "module declaration must be "
                                                                               "first in every file");
                }
            }

            switch (branch->getType()) {
                case ast_class_decl:
                    resolveClassMethods(branch);
                    break;
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


        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void Compiler::resolveClassMutateFields(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);
    if(IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
        // do nothing it's allready been processed
    } else {
        resolveClassFields(ast, currentClass);
    }
}

void Compiler::resolveAllFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";

        sourceFiles.addif(current->sourcefile);
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    modules.addif(currModule = "__srt_undefined");
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    errors->createNewError(GENERIC, branch->line, branch->col, "module declaration must be "
                                                                               "first in every file");
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
                case ast_func_prototype:
                    resolvePrototypeField(branch);
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


        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void Compiler::inheritObjectClass() {
    for(long long i = 0; i < classes.size(); i++) {
        inheritObjectClassHelper(classes.at(i)->ast, classes.at(i));
    }
}


void Compiler::compileTypeIdentifier(ReferencePointer &ptr, Ast *ast) {
    if(ast->getSubAstCount() == 0) {
        ptr.classes.add(ast->getEntity(0).getValue());
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
    if(field == NULL || !field->flags.find(STATIC))
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
    return !((int64_t )var > DA_MAX || (int64_t )var < DA_MIN);
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

void Compiler::inlineVariableValue(IrCode &code, Field *field) {
    double value = getInlinedFieldValue(field);
    code.free();

    if(isDClassNumberEncodable(value)) {
        code.push_i64(SET_Di(i64, op_MOVI, value), i64ebx);
        code.getInjector(stackInjector)
                .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
    }
    else {
        code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)value)), abs(getLowBytes(value)));
        code.getInjector(stackInjector)
                .push_i64(SET_Di(i64, op_RSTORE, i64bmr));
        code.getInjector(ebxInjector)
                .push_i64(SET_Ci(i64, op_MOVR, i64ebx, 0, i64bmr));
    }
}

Field *Compiler::resolveEnum(string name) {
    Field* field;

    for (unsigned int i = 0; i < importMap.size(); i++) {
        if (importMap.get(i).key == current->sourcefile) {

            List<string> &lst = importMap.get(i).value;
            for (unsigned int x = 0; x < lst.size(); x++) {
                for(long j = 0; j < enums.size(); j++) {
                    if(enums.get(j)->module == lst.get(x)) {
                        if((field = enums.get(j)->getField(name, false)) != NULL)
                            return field;
                    }
                }
            }

            break;
        }
    }
    return nullptr;
}

void Compiler::resolveSingularUtype(ReferencePointer &ptr, Utype* utype, Ast *ast) {
    DataEntity* resolvedUtype = NULL;
    string name = ptr.classes.get(0);
    List<Method*> functions;
    List<ClassObject*> resolvedClasses;

    if(ptr.mod != "")
        goto globalCheck;

    if(currentScope()->type >= INSTANCE_BLOCK && currentScope()->getLocalField(name) != NULL) {
        Field* field = currentScope()->getLocalField(name)->field;

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);

        if(field->isVar()) {
            if(field->isArray || field->locality == stl_thread) {
                if(field->locality == stl_thread) {
                    utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));

                    // we only do it for stl_thread var if it is not an array field
                    if(!field->isArray) {
                        utype->getCode().getInjector(ebxInjector)
                            .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                            .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                            .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx));
                    }
                } else {
                    utype->getCode().push_i64(SET_Di(i64, op_MOVL, field->address));
                }

                utype->getCode().getInjector(stackInjector)
                        .push_i64(SET_Ei(i64, op_PUSHOBJ));
            } else {
                utype->getCode().push_i64(SET_Ci(i64, op_LOADL, i64ebx, 0, field->address));
                utype->getCode().getInjector(stackInjector)
                        .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
            }
        }
        else {
            if(field->locality == stl_thread) {
                utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
            } else {
                utype->getCode().push_i64(SET_Di(i64, op_MOVL, field->address));
            }

            utype->getCode().getInjector(stackInjector)
                    .push_i64(SET_Ei(i64, op_PUSHOBJ));
        }
    } else if((resolvedUtype = currentScope()->klass->getField(name, currentScope()->type != RESTRICTED_INSTANCE_BLOCK)) != NULL) {
        Field* field = (Field*)resolvedUtype;
        if(currentScope()->type == STATIC_BLOCK && !field->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->line, ast->col, "cannot get field `" + name + "` from self in static context");
        }

        compileFieldType(field);

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);
        validateAccess(field, ast);

        if(field->getter == NULL && isFieldInlined(field)) {
            inlineVariableValue(utype->getCode(), field);
        } else {
            if(field->getter != NULL) {
                if(field->locality == stl_stack && !field->flags.find(STATIC) && !utype->getCode().instanceCaptured) {
                    utype->getCode().instanceCaptured = true;
                    utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                }
                compileFieldGetterCode(utype->getCode(), field);
            } else {
                if(field->locality == stl_thread) {
                    utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
                } else {
                    if(field->flags.find(STATIC) || field->owner->isGlobalClass())
                        utype->getCode().push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    else if(!utype->getCode().instanceCaptured) {
                        utype->getCode().instanceCaptured = true;
                        utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                    }

                    utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address));
                }

                if (field->isVar() && !field->isArray) {
                    utype->getCode().getInjector(ebxInjector)
                            .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                            .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                            .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx, 0, i64adx));

                    utype->getCode().getInjector(stackInjector)
                            .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                            .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                            .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx, 0, i64adx))
                            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                } else
                    utype->getCode().getInjector(stackInjector)
                            .push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
        }
    }  else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && IS_CLASS_ENUM(currentScope()->klass->getClassType()) && (resolvedUtype = currScope.get(currScope.size() - 2)->klass->getField(name, true)) != NULL) {
        Field* field = (Field*)resolvedUtype;
        if(currentScope()->type == STATIC_BLOCK && !field->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->line, ast->col, "cannot get field `" + name + "` from self in static context");
        }

        compileFieldType(field); // TODO: this might pose a problem make sure gatting variable values at the low level is being genetrated properly might cause seg faults if not

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);
        validateAccess(field, ast);

        if(field->getter == NULL && isFieldInlined(field)) {
            inlineVariableValue(utype->getCode(), field);
        } else {
            if(field->getter != NULL) {
                compileFieldGetterCode(utype->getCode(), field);
            } else {
                if(field->locality == stl_thread) {
                    utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
                } else {
                    utype->getCode().push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address));
                }

                if (field->isVar() && !field->isArray) {
                    utype->getCode().getInjector(ebxInjector)
                            .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                            .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                            .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx, 0, i64adx));

                    utype->getCode().getInjector(stackInjector)
                            .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                            .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                            .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx, 0, i64adx))
                            .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                } else
                    utype->getCode().getInjector(stackInjector)
                            .push_i64(SET_Ei(i64, op_PUSHOBJ));
            }
        }
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveEnum(name)) != NULL) {
        validateAccess((Field*)resolvedUtype, ast);
        utype->setType(utype_field);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(((Field*)resolvedUtype)->isArray);

        utype->getCode().push_i64(SET_Di(i64, op_MOVG, resolvedUtype->owner->address));
        utype->getCode().push_i64(SET_Di(i64, op_MOVN, resolvedUtype->address));

        utype->getCode().getInjector(stackInjector)
                .push_i64(SET_Ei(i64, op_PUSHOBJ));
        return;
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveClass("", name, ast)) != NULL) {
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
        validateAccess((ClassObject*)resolvedUtype, ast);
        checkTypeInference(ast);
    } else if((resolvedUtype = currentScope()->klass->getChildClass(name)) != NULL && ((ClassObject*)resolvedUtype)->getGenericOwner() !=  NULL) {
        // global class ?
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
        checkTypeInference(ast);
    } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && resolveClass(generics, resolvedClasses, ptr.mod, name + "<>", ast, true)) {
        if(resolvedClasses.size() == 1) {
            stringstream helpfulMessage;
            helpfulMessage << "have you forgotten your type parameters? Were you possibly looking for class `"
                           << resolvedClasses.get(0)->fullName << "` it requires " << resolvedClasses.get(0)->getKeys().size()
                           << " generic types.";
            errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col,
                                   " `" + name + "` " +
                                   helpfulMessage.str());
        } else {
            stringstream helpfulMessage;
            helpfulMessage << "I found a few generic classes that match `" << name << "`. Were you possibly looking for any of these?\n";
            helpfulMessage << "\t{";

            for(int i = 0; i < resolvedClasses.size(); i++) {
                if(i >= 4) {
                    helpfulMessage << "...";
                    break;
                }

                ClassObject *klass = resolvedClasses.get(i);

                helpfulMessage << "\n\t\t" << klass->fullName << "<";
                for(long x = 0; x < klass->getKeys().size(); x++) {
                    if(x > 5) {
                        helpfulMessage << "...";
                        break;
                    }

                    helpfulMessage << klass->getKeys().get(x);
                    if((x+1) < klass->getKeys().size()) {
                        helpfulMessage << ", ";
                    }
                }
                helpfulMessage << ">";
                if((i+1) < resolvedClasses.size())
                    helpfulMessage << ", ";
            }

            helpfulMessage << "\n\t}\n";

            errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col,
                                   " `" + name + "` " +
                                   helpfulMessage.str());
        }
    }  else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && resolveClass(classes, resolvedClasses, ptr.mod, name, ast, true)) {
        stringstream helpfulMessage;
        helpfulMessage << "I found a few classes that match `" << name << "`. Were you possibly looking for any of these?\n";
        helpfulMessage << "\t{";

        for(int i = 0; i < resolvedClasses.size(); i++) {
            if(i >= 4) {
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
    } else if(resolveExtensionFunctions(currentScope()->klass) && currentScope()->klass->getFunctionByName(name, functions, currentScope()->type != RESTRICTED_INSTANCE_BLOCK)) {

        utype->setType(utype_method);
        utype->setResolvedType(functions.get(0));
        utype->setArrayType(false);
        if(!functions.get(0)->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->line, ast->col, " cannot get address from non static function `" + name + "` ");
        }
        utype->getCode().push_i64(SET_Di(i64, op_MOVI, functions.get(0)->address), i64ebx);
        utype->getCode().getInjector(stackInjector)
                .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
    } else if((resolvedUtype = currentScope()->klass->getAlias(name, currentScope()->type != RESTRICTED_INSTANCE_BLOCK)) != NULL) {
        Alias* alias = (Alias*)resolvedUtype;
        compileAliasType(alias);

        utype->copy(alias->utype);
        utype->getCode().free().inject(alias->utype->getCode());
        utype->getCode().getInjector(ptrInjector).free().inject(alias->utype->getCode().getInjector(ptrInjector));
        utype->getCode().getInjector(ebxInjector).free().inject(alias->utype->getCode().getInjector(ebxInjector));
        utype->getCode().getInjector(stackInjector).free().inject(alias->utype->getCode().getInjector(stackInjector));
        validateAccess(alias, ast);
        checkTypeInference(alias, ast);
    }  else {
        globalCheck:
        ClassObject* global = resolveClass(ptr.mod == "" ? currModule : ptr.mod, globalClass, ast);
        functions.free();

        if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = resolveClass(ptr.mod, name, ast)) != NULL) {
            utype->setType(utype_class);
            utype->setResolvedType(resolvedUtype);
            utype->setArrayType(false);
            validateAccess((ClassObject*)resolvedUtype, ast);
            checkTypeInference(ast);
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = global->getField(name, true)) != NULL) {
            Field* field = (Field*)resolvedUtype;
            compileFieldType(field);

            utype->setType(utype_field);
            utype->setResolvedType(field);
            utype->setArrayType(field->isArray);
            validateAccess(field, ast);

            if(field->getter == NULL && isFieldInlined(field)) {
                inlineVariableValue(utype->getCode(), field);
            } else {

                if(field->getter != NULL) {
                    compileFieldGetterCode(utype->getCode(), field);
                } else {
                    if(field->locality == stl_thread) {
                        utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
                    } else {
                        utype->getCode().push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                        utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address));
                    }

                    if (field->isVar() && !field->isArray) {
                        utype->getCode().getInjector(ebxInjector)
                                .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                                .push_i64(SET_Di(i64, op_CHECKLEN, i64adx))
                                .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx, 0, i64adx));

                        utype->getCode().getInjector(stackInjector)
                                .push_i64(SET_Di(i64, op_MOVI, 0), i64adx)
                                .push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx, 0, i64adx))
                                .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                    } else
                        utype->getCode().getInjector(stackInjector)
                                .push_i64(SET_Ei(i64, op_PUSHOBJ));
                }
            }
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && global->getFunctionByName(name, functions)) {
            if(functions.size() > 1)
                createNewWarning(GENERIC, __WAMBIG, ast->line, ast->col, "reference to function `" + functions.get(0)->name + "` is ambiguous");

            utype->setType(utype_method);
            utype->setResolvedType(functions.get(0));
            utype->setArrayType(false);
            if(!functions.get(0)->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast->line, ast->col, " cannot get address from non static function `" + name + "` ");
            }
            utype->getCode().push_i64(SET_Di(i64, op_MOVI, functions.get(0)->address), i64ebx);
            utype->getCode().getInjector(stackInjector)
                    .push_i64(SET_Di(i64, op_RSTORE, i64ebx));
            return;
        } else if(currentScope()->type != RESTRICTED_INSTANCE_BLOCK && (resolvedUtype = global->getAlias(name, true)) != NULL) {
            Alias* alias = (Alias*)resolvedUtype;
            compileAliasType(alias);

            utype->copy(alias->utype);
            utype->getCode().free().inject(alias->utype->getCode());
            utype->getCode().getInjector(ptrInjector).free().inject(alias->utype->getCode().getInjector(ptrInjector));
            utype->getCode().getInjector(ebxInjector).free().inject(alias->utype->getCode().getInjector(ebxInjector));
            utype->getCode().getInjector(stackInjector).free().inject(alias->utype->getCode().getInjector(stackInjector));
            validateAccess(alias, ast);
            checkTypeInference(alias, ast);
            return;
        }

        errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + name + "` ");
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
void Compiler::compileFieldGetterCode(IrCode &code, Field *field) {
    if(field != NULL && field->utype != NULL && field->getter) {
        if(field->isVar() && !field->isArray) {
            code.push_i64(SET_Di(i64, op_CALL, field->getter->address));

            code.getInjector(ebxInjector)
                    .push_i64(SET_Di(i64, op_LOADVAL, i64ebx));
        } else {
            code.push_i64(SET_Di(i64, op_CALL, field->getter->address));

            code.getInjector(ptrInjector)
                    .push_i64(SET_Ei(i64, op_POPOBJ_2));
        }
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
            if (field->ast->getType() == ast_func_prototype) {
                resolvePrototypeField(field->ast);
                field->inlineCheck = true;
            } else
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
        if (alias->type == UNTYPED) {
            currScope.add(new Scope(alias->owner, alias->owner->isGlobalClass() ? GLOBAL_SCOPE : CLASS_SCOPE));
            resolveAlias(alias->ast);
            removeScope();
        }
    }
}

void Compiler::resolveClassHeiarchy(DataEntity* data, bool fromClass, ReferencePointer& ptr, Utype* utype, Ast* ast) {
    bool lastReference;
    Utype *bridgeUtype = new Utype(); // we need this so we dont loose code when free is called

    ClassObject *oldScope = currentScope()->klass;
    RETAIN_BLOCK_TYPE(fromClass ? STATIC_BLOCK : RESTRICTED_INSTANCE_BLOCK)
    if(fromClass) currentScope()->klass = (ClassObject*)data;
    for(unsigned int i = 1; i < ptr.classes.size(); i++) {
        lastReference = (i+1) >= ptr.classes.size();

        string name = ptr.classes.at(i);
        ReferencePointer nextReference;
        nextReference.classes.add(name);

        bridgeUtype->getCode().free();
        bridgeUtype->getCode().instanceCaptured = true;
        bridgeUtype->setType(utype_unresolved);
        resolveSingularUtype(nextReference, bridgeUtype, ast);
        if(utype->getType() != utype_unresolved && utype->isArray()) {
            errors->createNewError(GENERIC, ast->line, ast->col, "dot notation access on array of type `" + utype->toString() + "`");
        } else if(utype->getType() != utype_unresolved && !utype->isArray() && utype->getResolvedType()->isVar()) {
            varErr:
            errors->createNewError(GENERIC, ast->line, ast->col, "dot notation access on field `" + utype->toString() + "` was not found to be a class");
            break;
        }

        if(bridgeUtype->getType() == utype_field) {
            if(fromClass)
                utype->getCode().push_i64(SET_Di(i64, op_MOVG, bridgeUtype->getResolvedType()->owner->address));

            utype->getCode().inject(bridgeUtype->getCode());
            if(!lastReference)
                utype->getCode().inject(bridgeUtype->getCode().getInjector(ptrInjector));
            else {
                utype->getCode().getInjector(ptrInjector).inject(bridgeUtype->getCode().getInjector(ptrInjector));
                utype->getCode().getInjector(ebxInjector).inject(bridgeUtype->getCode().getInjector(ebxInjector));
                utype->getCode().getInjector(stackInjector).inject(bridgeUtype->getCode().getInjector(stackInjector));
            }

            Field *field = (Field*)bridgeUtype->getResolvedType();
            compileFieldType(field);

            // if you had class K { static instance : K; } and you did { inst := K.instance.instance; }
            if(utype->getType() == utype_field && utype->getResolvedType()->guid == field->guid
                && (field->flags.find(STATIC) || field->locality == stl_thread)) {
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "redundant multiple dot notation access on field `" + field->toString() + "` which is static or a thread local");
            } else if(i == 1 && data->guid == field->guid
                                && (field->flags.find(STATIC) || field->locality == stl_thread))
                createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "redundant multiple dot notation access on field `" + field->toString() + "` which is static or a thread local");

            if(utype->getClass() != NULL)
                currentScope()->klass = utype->getClass();
            else if(!lastReference)
                errors->createNewError(GENERIC, ast->line, ast->col, "field `" + utype->toString() + "` was not found to be a class");
            fromClass = false;
            oldType = RESTRICTED_INSTANCE_BLOCK;
        } else if(bridgeUtype->getType() == utype_method) {
            utype->getCode().free(); // getting method address are just vars so we dont need to call all the other code
            utype->getCode().inject(bridgeUtype->getCode());
            if(!lastReference)
                utype->getCode().inject(bridgeUtype->getCode().getInjector(ebxInjector));
            else {
                utype->getCode().getInjector(ebxInjector).inject(bridgeUtype->getCode().getInjector(ebxInjector));
                utype->getCode().getInjector(stackInjector).inject(bridgeUtype->getCode().getInjector(stackInjector));
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
            }
        } else if(bridgeUtype->getType() == utype_class) {
            if(!fromClass)
                goto error;

            currentScope()->klass = (ClassObject*)bridgeUtype->getResolvedType();
            if(lastReference) checkTypeInference(ast);
        } else {
            error:
            errors->createNewError(GENERIC, ast->line, ast->col, " resolved type `" + bridgeUtype->toString() + "` must be a field or a method");
        }

        utype->copy(bridgeUtype);
    }

    currentScope()->klass = oldScope;
    bridgeUtype->free();
    delete bridgeUtype;
    RESTORE_BLOCK_TYPE()
}

void Compiler::resolveUtype(ReferencePointer &ptr, Utype* utype, Ast *ast) {

    if(ptr.classes.singular() && ptr.mod == "") {
        resolveSingularUtype(ptr, utype, ast);
    } else {
        string name = ptr.classes.at(0);
        ReferencePointer initialRefrence;
        initialRefrence.classes.add(name);
        initialRefrence.mod = ptr.mod;

        resolveSingularUtype(initialRefrence, utype, ast);
        if(utype->getType() == utype_method || utype->getType() == utype_method_prototype) {
            errors->createNewError(GENERIC, ast->line, ast->col, "symbol `" + utype->toString() +
                             "` is a pointer to the respective function and this returns a var and cannot be used as a class field.");
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

Utype* Compiler::compileUtype(Ast *ast, bool instanceCaptured) {
    if(ast->getType() != ast_utype)
        throw runtime_error("check parseUtype()");

    // we need this code for complex expressions
    Utype *utype = new Utype();
    utype->getCode().instanceCaptured = instanceCaptured;

    ReferencePointer ptr;
    compileTypeIdentifier(ptr, ast->getSubAst(ast_type_identifier));

    if(ptr.classes.singular() && parser::isNativeType(ptr.classes.get(0))) {
        utype->setType(utype_native);
        DataEntity *native = new DataEntity();
        utype->setResolvedType(native);

        native->type = strToNativeType(ptr.classes.get(0));

        if(ast->hasEntity(LEFTBRACE) && ast->hasEntity(RIGHTBRACE)) {
            utype->setArrayType(true);
        }

        checkTypeInference(ast);
        ptr.free();
        return utype;
    } else if(ptr.classes.singular() && ptr.mod == "" && IS_CLASS_GENERIC(currentScope()->klass->getClassType()) && currentScope()->klass->getKeys().find(ptr.classes.get(0))
            && currentScope()->klass->isProcessed()) {
        delete utype;
        utype = currentScope()->klass->getKeyTypes().get(currentScope()->klass->getKeys().indexof(ptr.classes.get(0)));
        bool isArray = false;
        if(ast->hasEntity(LEFTBRACE) && ast->hasEntity(RIGHTBRACE)) {
            isArray = true;
        }

        checkTypeInference(ast);
        if(utype->isArray() && isArray) {
            // error
            errors->createNewError(GENERIC, ast, "Array-arrays are not supported.");
        } else if(isArray) {
            utype->setArrayType(true);
        }
        ptr.free();
        return utype;
    }

    resolveUtype(ptr, utype, ast);

    if(ast->hasEntity(LEFTBRACE) && ast->hasEntity(RIGHTBRACE)) {
        utype->setArrayType(true);
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

    ClassObject *generic = parent != NULL ? parent->getChildClass(name + "<>") : findClass(mod, name + "<>", generics);
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
            newClass->setIsProcessed(true);
            newClass->owner = parent;
            newClass->addKeyTypes(utypes);
            newClass->setGenericOwner(generic);

            if(processingStage >= POST_PROCESSING) {
                errors->filename = newClass->getGenericOwner()->meta.file;

                string file = errors->filename;
                List<string> oldLines;
                parser* oldParser = current;
                long long currErrors = errors->getUnfilteredErrorCount() ,newErrors;

                oldLines.addAll(errors->lines);
                current = getParserBySourceFile(errors->filename);
                errors->lines.addAll(current->lines);

                // preprocess code
                preProccessClassDecl(newClass->ast, IS_CLASS_INTERFACE(newClass->getClassType()), newClass);
                resolveSuperClass(newClass->ast, newClass);

                inheritEnumClassHelper(newClass->ast, newClass);
                inheritObjectClassHelper(newClass->ast, newClass);
                resolveClassMethods(newClass->ast, newClass);
                resolveExtensionFunctions(newClass);
                resolveAllDelegates(newClass->ast, newClass);
                resolveUnprocessedClassMutations(newClass);

                if(processingStage > POST_PROCESSING) {
                    // post processing code
                    resolveClassFields(newClass->ast, newClass);
                    inlineClassFields(newClass->ast, newClass);
                    resolveGenericFieldMutations(newClass);

                    // TODO: add additional code
                } else // we need to make sure we don't double process he class
                    unProcessedClasses.add(newClass);

                current = oldParser;
                errors->lines.addAll(oldLines);
                errors->filename = file;
                newErrors = errors->getUnfilteredErrorCount()-currErrors;

                if(newErrors > 0) {
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
    bool hashFound = ast->findEntity("#"), hashProcessed = false;
    stringstream module;
    long genericListPos = 0;
    ClassObject *parent = NULL;
    bool failed = false;
    for(long i = 0; i < ast->getEntityCount(); i++) {
        if(hashFound && !hashProcessed) {
            module << ast->getEntity(i).getValue();
            if(ast->getEntity(i+1) == "#") {
                hashProcessed = true;
                ptr.mod = module.str();
                i++;
            }
        } else {
            string name = ast->getEntity(i).getValue();
            if(name != ".") {
                if (!failed && ((i + 1) < ast->getEntityCount()) && ast->getEntity(i + 1) == "<") {
                    RETAIN_TYPE_INFERENCE(false)
                    ClassObject *genericClass = compileGenericClassReference(
                            ptr.mod, name, parent, ast->getSubAst(genericListPos++));
                    ptr.classes.add(genericClass ? genericClass->name : "?");

                    parent = genericClass;
                    failed = parent == NULL;
                    i += 2;
                    RESTORE_TYPE_INFERENCE()
                } else {
                    ptr.classes.add(name);

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

ClassObject *Compiler::resolveClassReference(Ast *ast, ReferencePointer &ptr, bool allowGenerics) {
    if(ptr.classes.singular()) {
        ClassObject* klass = resolveClass(ptr.mod, ptr.classes.get(0), ast);
        if(klass == NULL) {
            if((klass = findClass(ptr.mod, ptr.classes.get(0) + "<>", generics))) {
                if(allowGenerics) // this is for extension functions
                    return klass;
                else
                    errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0)
                                                               + "`. It was found to be a generic class, have you missed the key parameters to the class perhaps?");
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
            } else {
                errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0) + "`");
            }
        } else {
            Utype utype;
            resolveClassHeiarchy(klass, true, ptr, &utype, ast);

            if(utype.getType() != utype_class) {
                errors->createNewError(GENERIC, ast, "symbol `" + utype.toString() + "` is not a class");
            } else
                return (ClassObject*)utype.getResolvedType();
        }
    }

    return NULL;
}

ClassObject *Compiler::resolveBaseClass(Ast *ast, ClassObject *currClass) {
    ClassObject* base=NULL;

    if(ast->hasSubAst(ast_refrence_pointer)) {

        ReferencePointer ptr;
        compileReferencePtr(ptr, ast->getSubAst(ast_refrence_pointer));
        base = resolveClassReference(ast, ptr);

        if (base != NULL) {
            if ((currentScope()->klass->isClassRelated(base) ||
                    base->isClassRelated(currClass))) {
                errors->createNewError(GENERIC, ast->getSubAst(0)->line, ast->getSubAst(0)->col,
                                       "cyclic dependency of class `" + currentScope()->klass->fullName + "` in parent class `" +
                                       base->fullName + "`");
                return NULL;
            }

            if(IS_CLASS_INTERFACE(base->getClassType())) {
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
        string name = ast->getEntity(0).getValue();
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


    currScope.add(new Scope(currentClass, CLASS_SCOPE));

    ClassObject* base = resolveBaseClass(ast, currentClass);
    if(base != NULL) {
        if (IS_CLASS_INTERFACE(currentClass->getClassType())) {
            if (base != NULL && !IS_CLASS_INTERFACE(base->getClassType())) {
                stringstream err;
                err << "interface '" << currentClass->fullName << "' must inherit another interface class";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                err.str("");
            } else
                currentClass->setSuperClass(base);
        } else {
            if (currentClass->getSuperClass()) {
                stringstream err;
                err << "class '" << currentClass->fullName << "' already has a base class of `"
                    << currentClass->getSuperClass()->fullName << "` and cannot Be overridden";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                err.str("");
            } else
                currentClass->setSuperClass(base);
        }
    }

    // we also want to resolve any interfaces as well
    if(ast->hasSubAst(ast_reference_pointer_list)) {
        List<ReferencePointer*> refPtrs;
        List<ClassObject*> interfaces;
        interfaces.addAll(currentClass->getInterfaces());

        parseReferencePointerList(refPtrs, ast->getSubAst(ast_reference_pointer_list));

        for(long i = 0; i < refPtrs.size(); i++) {
            branch = ast->getSubAst(ast_reference_pointer_list)->getSubAst(i);
            ClassObject* _interface = resolveClassReference(branch, *refPtrs.get(i));

            if(_interface != NULL && !IS_CLASS_INTERFACE(_interface->getClassType())) {
                stringstream err;
                err << "class `" + _interface->name + "` is not an interface";
                errors->createNewError(GENERIC, branch->line, branch->col, err.str());
            } else if(_interface != NULL && interfaces.count(_interface) > 1){
                stringstream err;
                err << "duplicate interface '" << _interface->name << "'";
                errors->createNewError(GENERIC, branch->line, branch->col, err.str());
            } else {
                interfaces.add(_interface);
            }
        }

        currentClass->getInterfaces().addAll(interfaces);
        interfaces.free();
        freeListPtr(refPtrs);
    }

    for(long i = 0; i < block->getSubAstCount(); i++) {
        branch = block->getSubAst(i);
        CHECK_CMP_ERRORS(return;)

        switch(branch->getType()) {
            case ast_class_decl: /* ignore */
                resolveSuperClass(branch);
                break;
            case ast_interface_decl: /* ignore */
                resolveSuperClass(branch);
                break;
            case ast_generic_class_decl: /* ignore */
                resolveSuperClass(branch);
                break;
            case ast_generic_interface_decl: /* ignore */
                resolveSuperClass(branch);
                break;
            default:
                break;
        }
    }
    removeScope();
}

void Compiler::inheritEnumClass() {
    for(long long i = 0; i < classes.size(); i++) {
        ClassObject *k = classes.get(i);
        inheritEnumClassHelper(k->ast, k);
    }
}

parser *Compiler::getParserBySourceFile(string name) {
    for(long long i = 0; i < parsers.size(); i++) {
        if(parsers.get(i)->sourcefile == name)
            return parsers.get(i);
    }
    return NULL;
}

void Compiler::preProcessUnprocessedClasses(long long unstableClasses) {
    string old = currModule;
    parser* oldParser = current;
    string oldFile = errors->filename;
    List<string> oldLines;
    oldLines.addAll(errors->lines);
    // we need this size just incase any classes get added after the fact to prevent double preprocessing
    long long size = unstableClasses;

    for(long long i = 0; i < size; i++) {
        ClassObject *unprocessedClass = unProcessedClasses.get(i);

        currModule = unprocessedClass->module;
        errors->filename = unprocessedClass->getGenericOwner()->meta.file;
        current = getParserBySourceFile(errors->filename);

        long long currErrors = errors->getUnfilteredErrorCount() ,newErrors;
        errors->lines.addAll(current->lines);

        // bring the classes up to speed
        preProccessClassDecl(unprocessedClass->ast, IS_CLASS_INTERFACE(unprocessedClass->getClassType()), unprocessedClass);
        resolveSuperClass(unprocessedClass->ast, unprocessedClass);
        resolveClassMethods(unprocessedClass->ast, unprocessedClass);
        resolveAllDelegates(unprocessedClass->ast, unprocessedClass);
        resolveUnprocessedClassMutations(unprocessedClass);

        newErrors = errors->getUnfilteredErrorCount()-currErrors;

        if(newErrors > 0) {
            // this helps the user find where they went wrong
            printNote(unprocessedClass->meta, "in generic `" + unprocessedClass->name + "`");
        }
    }

    currModule = old;
    current = oldParser;
    errors->lines.addAll(oldLines);
    errors->filename = oldFile;
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
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";

        sourceFiles.addif(current->sourcefile);
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
                    modules.addif(currModule = "__srt_undefined");
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    errors->createNewError(GENERIC, branch->line, branch->col, "module declaration must be "
                                                                               "first in every file");
                }
            }

            switch(branch->getType()) {
                case ast_class_decl:
                    resolveSuperClass(branch);
                    break;
                case ast_interface_decl:
                    resolveSuperClass(branch);
                    break;
                case ast_generic_class_decl:
                    resolveSuperClass(branch);
                    break;
                case ast_generic_interface_decl:
                    resolveSuperClass(branch);
                    break;
                default:
                    /* ignore */
                    break;
            }

            CHECK_CMP_ERRORS(return;)
        }


        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void Compiler::preProcessGenericClasses(long long unstableClasses) {

    errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

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

    if(errors->hasErrors()){
        report:

        errCount+= errors->getErrorCount();
        rawErrCount+= errors->getUnfilteredErrorCount();

        failedParsers.addif(current->sourcefile);
        succeededParsers.removefirst(current->sourcefile);
    } else {
        succeededParsers.addif(current->sourcefile);
        failedParsers.removefirst(current->sourcefile);
    }

    errors->free();
    delete (errors); this->errors = NULL;
}

void Compiler::inheritRespectiveClasses() {

    errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

    inheritEnumClass();
    inheritObjectClass();

    if(errors->hasErrors()){
        report:

        errCount+= errors->getErrorCount();
        rawErrCount+= errors->getUnfilteredErrorCount();

        failedParsers.addif(current->sourcefile);
        succeededParsers.removefirst(current->sourcefile);
    } else {
        succeededParsers.addif(current->sourcefile);
        failedParsers.removefirst(current->sourcefile);
    }

    errors->free();
    delete (errors); this->errors = NULL;
}

void Compiler::inlineFieldHelper(Ast* ast) {
    string name = ast->getEntity(0).getValue();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getEntity(1).getValue();
    }

    Field *field = currentScope()->klass->getField(name, false);
    if(!field->inlineCheck) {
        field->inlineCheck = true;

        if(ast->hasSubAst(ast_expression)) {
            Expression expr;
            compileExpression(&expr, ast->getSubAst(ast_expression));

            if(expr.type == exp_var && utypeToExpressionType(field->utype) == exp_var && !field->utype->isArray()) {
                if(expr.utype->getType() == utype_field && isFieldInlined((Field*)expr.utype->getResolvedType())) {
                    inlinedFields.add(KeyPair<Field*, double>(field, getInlinedFieldValue((Field*)expr.utype->getResolvedType())));
                } else if(expr.utype->getType() == utype_literal) {
                    Literal* literal = ((Literal*)expr.utype->getResolvedType());

                    if(literal->literalType == numeric_literal) {
                        inlinedFields.add(KeyPair<Field*, double>(field, literal->numericData));
                    }
                } else if(expr.utype->getType() == utype_method) {
                    Method* method = ((Method*)expr.utype->getResolvedType());
                    inlinedFields.add(KeyPair<Field*, double>(field, method->address));
                }
            }
        }
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
    string name = ast->getEntity(0).getValue();
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
        string name = ast->getEntity(0).getValue();
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
        string name = ast->getEntity(0).getValue();
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

void Compiler::inlineClassMutateFields(Ast *ast) {
    ReferencePointer ptr;
    compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
    ClassObject *currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);
    if(IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
        // do nothing it's allready been processed
    } else {
        inlineClassFields(ast, currentClass);
    }
}

void Compiler::inlineFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";

        sourceFiles.addif(current->sourcefile);
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for (int x = 0; x < current->size(); x++) {
            Ast *branch = current->astAt(x);

            if (x == 0) {
                if (branch->getType() == ast_module_decl) {
                    currModule = parseModuleDecl(branch);
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    modules.addif(currModule = "__srt_undefined");
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    errors->createNewError(GENERIC, branch->line, branch->col, "module declaration must be "
                                                                               "first in every file");
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


        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void Compiler::postProcessUnprocessedClasses() {
    string old = currModule;
    parser* oldParser = current;
    string oldFile = errors->filename;
    List<string> oldLines;
    oldLines.addAll(errors->lines);

    for(long long i = 0; i < unProcessedClasses.size(); i++) {
        ClassObject *unprocessedClass = unProcessedClasses.get(i);

        currModule = unprocessedClass->module;
        errors->filename = unprocessedClass->getGenericOwner()->meta.file;
        current = getParserBySourceFile(errors->filename);

        long long currErrors = errors->getUnfilteredErrorCount() ,newErrors;
        errors->lines.addAll(current->lines);

        // bring the classes up to speed
        resolveClassFields(unprocessedClass->ast, unprocessedClass);
        inlineClassFields(unprocessedClass->ast, unprocessedClass);
        resolveExtensionFunctions(unprocessedClass);
        resolveGenericFieldMutations(unprocessedClass);

        // class has graduated but still is unprocessed so we must babysit the class...
        newErrors = errors->getUnfilteredErrorCount()-currErrors;
        if(newErrors > 0) {
            // this helps the user find where they went wrong
            printNote(unprocessedClass->meta, "in generic `" + unprocessedClass->name + "`");
        }
    }

    currModule = old;
    current = oldParser;
    errors->lines.addAll(oldLines);
    errors->filename = oldFile;
}

void Compiler::postProcessGenericClasses() {

    errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

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

    if(errors->hasErrors()){
        report:

        errCount+= errors->getErrorCount();
        rawErrCount+= errors->getUnfilteredErrorCount();

        failedParsers.addif(current->sourcefile);
        succeededParsers.removefirst(current->sourcefile);
    } else {
        succeededParsers.addif(current->sourcefile);
        failedParsers.removefirst(current->sourcefile);
    }

    errors->free();
    delete (errors); this->errors = NULL;
}

bool Compiler::postProcess() {
    processingStage = POST_PROCESSING;
    long long unstableClasses = unProcessedClasses.size();

    preprocessMutations();
    preProcessGenericClasses(unstableClasses);
    inheritRespectiveClasses();
    resolveAllMethods();
    resolveAllDelegates();
    resolveAllFields();
    inlineFields();
    postProcessGenericClasses();
    // TODO: analyze generic classes first then everything else

    return rawErrCount == 0;
}

void Compiler::handleImports() {

    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";
        sourceFiles.addif(current->sourcefile);

        // Post-PreProcessing functions
        if(c_options.magic)
        { // import everything in magic mode
            importMap.__new().key = current->sourcefile;
            importMap.last().value.init();
            importMap.last().value.addAllUnique(modules);
        } else
            preproccessImports();

        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
    }
}

bool Compiler::preprocess() {
    bool success = true;
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";

        sourceFiles.addif(current->sourcefile);
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
                    modules.addif(currModule = "__srt_undefined");
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
                case ast_method_decl: /* ignore */
                    break;
                case ast_variable_decl:
                    preProccessVarDecl(branch);
                    break;
                case ast_alias_decl:
                    preProccessAliasDecl(branch);
                    break;
                case ast_func_prototype:
                    preProccessVarDecl(branch);
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

        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            success = false;
            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }

    handleImports();
    resolveBaseClasses();
    return success;
}

void Compiler::preProcessMutation(Ast *ast, ClassObject *currentClass) {
    Ast* block = ast->getLastSubAst();

    if(currentClass == NULL) {
        ReferencePointer ptr;
        compileReferencePtr(ptr, ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer));
        currentClass = resolveClassReference(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer), ptr, true);
    }

    if(currentClass) {
        if(currentClass->flags.find(STABLE))
            errors->createNewError(GENERIC, ast->line, ast->col, "cannot mutate stable class `" + currentClass->fullName + "`");

        if(IS_CLASS_GENERIC(currentClass->getClassType()) && currentClass->getGenericOwner() == NULL) {
            currentClass->getClassMutations().add(ast);
        } else {
            resolveSuperClass(ast, currentClass);
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
                    case ast_method_decl: /* Will be parsed later */
                        break;
                    case ast_func_prototype:
                        preProccessVarDecl(branch);
                        break;
                    case ast_operator_decl: /* Will be parsed later */
                        break;
                    case ast_construct_decl: /* Will be parsed later */
                        break;
                    case ast_delegate_impl: /* Will be parsed later */
                        break;
                    case ast_delegate_decl: /* Will be parsed later */
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
                        preProcessMutation(branch);
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
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";

        sourceFiles.addif(current->sourcefile);
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
                    modules.addif(currModule = "__srt_undefined");
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    errors->createNewError(GENERIC, branch->line, branch->col, "module declaration must be "
                                                                               "first in every file");
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

        if(errors->hasErrors()){
            report:

            errCount+= errors->getErrorCount();
            rawErrCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(current->sourcefile);
            succeededParsers.removefirst(current->sourcefile);
        } else {
            succeededParsers.addif(current->sourcefile);
            failedParsers.removefirst(current->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void Compiler::compile() {

    if(preprocess() && postProcess()) {
        processingStage = COMPILING;
        // TODO: write compileUnprocessedClasses() to be called for everything else and free unprocessedClasses list

        cout << "ready to compile!!!";
    }
}

string Compiler::parseModuleDecl(Ast *ast) {
    string module = ast->getEntity(0).getValue();
    for(long i = 1; i < ast->getEntityCount(); i++)
    {
        module += ast->getEntity(i).getValue();
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
    Meta meta(current->getErrors()->getLine(ast==NULL ? 0 : ast->line), current->sourcefile,
              ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);

    ClassObject* k = new ClassObject(name, currModule, this->guid++, flags, meta), *prevClass;
    if((prevClass = findClass(currModule, name, classList))){

        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                                                                                "` is already defined in module {" + currModule + "}");

        printNote(prevClass->meta, "class `" + name + "` previously defined here");
        k->free();
        delete k;
        return findClass(currModule, name, classList);
    } else {
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
            if(!params.get(i)->isRelated(*comparator.get(i)))
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
              current->sourcefile, ast==NULL ? 0 : ast->line, ast==NULL ? 0 : ast->col);
    if(klass->getConstructor(emptyParams, false) == NULL) {
        Method* method = new Method(klass->name, currModule, klass, emptyParams, flags, meta);

        method->fnType=fn_constructor;
        method->address = methodSize++;
        klass->addFunction(method);
    }
}


void Compiler::createNewWarning(error_type error, int type, int line, int col, string xcmnts) {
    if(c_options.warnings && !warnings.find(xcmnts)) {
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
    flags.add(UNSTABLE);
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
