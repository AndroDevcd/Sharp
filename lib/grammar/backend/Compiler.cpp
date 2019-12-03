//
// Created by BraxtonN on 10/18/2019.
//

#include <cmath>
#include "Compiler.h"
#include "../main.h"
#include "oo/ClassObject.h"
#include "ReferencePointer.h"
#include "Utype.h"
#include "../../runtime/Opcode.h"
#include "../../runtime/register.h"
#include "Expression.h"
#include "Literal.h"

string globalClass = "__SRT_Global";
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
    else
        return flg_UNDEFINED;
}

void Compiler::parseClassAccessFlags(List<AccessFlag> &flags, Ast *ast) {
    if(ast->getEntityCount() > 2)
        this->errors->createNewError(GENERIC, ast->line, ast->col, "class objects only allows for access "
                                                                   "specifiers (public, private, protected, and local)");
    else {
        AccessFlag lastFlag = flg_UNDEFINED;
        for(int i = 0; i < ast->getEntityCount(); i++) {
            AccessFlag flag = strToAccessFlag(ast->getEntity(i).getToken());
            flags.add(flag);

            if (flag > LOCAL)
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                             " `" + ast->getEntity(i).getToken() + "`");
            if(flag == lastFlag) {
                this->errors->createNewError(ILLEGAL_ACCESS_DECLARATION, ast->line, ast->col,
                                             " duplicate access flag `" + ast->getEntity(i).getToken() + "`");
            }

            if(flag==LOCAL && !globalScope()) {
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                             " `" + ast->getEntity(i).getToken() + "` can only be used at global scope");
            }

            lastFlag = flag;
        }

        if(flags.size() > 1 && flags.find(LOCAL)) {
            createNewWarning(GENERIC, __WACCESS, ast->line, ast->col, "`local` access specifier trumps others");
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
    for(long i = 0; i < ast->getEntityCount(); i++) {
        flags.add(strToAccessFlag(ast->getEntity(i).getToken()));

        if(flags.last() == LOCAL && !globalScope()) {
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                         " `" + ast->getEntity(i).getToken() + "` can only be used at global scope");
        }
    }

    if(flags.size() > 3) {
        errors->createNewError(ILLEGAL_ACCESS_DECLARATION, ast->line, ast->col,
                               "too many access specifiers found");
    }

    long errPos=0;
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
                                     ast->getEntity(errPos).getColumn(), " `" + ast->getEntity(errPos).getToken() + "`");
    }

}

StorageLocality Compiler::strtostl(string locality) {
    return locality == "thread_local" ? stl_thread : stl_stack;
}



void Compiler::preProccessVarDeclHelper(List<AccessFlag>& flags, Ast* ast) {
    string name =  ast->getEntity(0).getToken();
    StorageLocality locality = stl_stack;
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        locality = strtostl(name);
        name = ast->getEntity(1).getToken();
        if(flags.find(flg_CONST)) {
            this->errors->createNewError(GENERIC, ast->line, ast->col,
                                         "thread local field `" + name + "` does not allow `const` access modifier");
        }
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

void Compiler::parseIdentifierList(Ast *ast, List<string> &idList) {
    ast = ast->getSubAst(ast_identifier_list);

    for(long i = 0; i < ast->getEntityCount(); i++) {
        string Key = ast->getEntity(i).getToken();
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

    string className = ast->getEntity(0).getToken();
    List<string> identifierList;
    parseIdentifierList(ast, identifierList);

    stringstream fullName;
    if (currentScope()->klass == NULL || globalScope()) {
        // should never happen unless due to user error
        currentClass = addGlobalClassObject(className, flags, ast, generics);
        fullName << currModule << "#" << currentClass->name;
    } else {
        currentClass = addChildClassObject(className, flags, currentScope()->klass, ast);
        fullName << currentScope()->klass->fullName << "." << currentClass->name;
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

    string name =  ast->getEntity(0).getToken();
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

    string className =  ast->getEntity(0).getToken();

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

        string className = ast->getEntity(0).getToken();
        if (currentScope()->klass == NULL || globalScope()) {
            // should never happen unless due to user error
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
        return 0;
}

void Compiler::removeScope() {
    currScope.pop_back();
}

void Compiler::preProccessImportDecl(Ast *branch, List<string> &imports) {
    bool star = false;
    stringstream ss;
    for(long i = 0; i < branch->getEntityCount(); i++) {
        if(branch->getEntity(i).getToken() != "*") {
            ss << branch->getEntity(i).getToken();
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

void Compiler::validateAccess(Field *field, Ast* pAst) {
    if(field->flags.find(LOCAL)) {
        Scope *scope = currentScope();
        if(scope->klass->meta.file == field->meta.file) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to localized field `" + field->fullName + "`");
        }
    } else if(field->flags.find(PRIVATE)) {
        if(field->owner == currentScope()->klass || field->owner == currentScope()->klass->getSuperClass()) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private field `" + field->fullName + "`");
        }
    } else if(field->flags.find(PROTECTED)) {
        if(currentScope()->klass->isClassRelated(field->owner) || field->owner == currentScope()->klass) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected method `" + field->fullName + "`");
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

ClassObject* Compiler::resolveClass(string mod, string name, Ast* pAst) {
    ClassObject* klass = NULL;
    if(!mod.empty() && (klass = findClass(mod, name, classes)) != NULL) {
        validateAccess(klass, pAst);
        return klass;
    } else {
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

    int64_t  i64;
    if(token.getToken().size() > 1) {
        switch(token.getToken().at(1)) {
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
                expr->utype->setResolvedType(new Literal(token.getToken().at(1)));
                expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, token.getToken().at(1)), i64ebx);
                break;
        }
    } else {
        expr->utype->setResolvedType(new Literal(token.getToken().at(0)));
        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, token.getToken().at(0)), i64ebx);
    }
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
    string int_string = invalidateUnderscores(token.getToken());

    if(isAllIntegers(int_string)) {
        value = std::strtod (int_string.c_str(), NULL);
        if(value > DA_MAX || value < DA_MIN) {
            stringstream ss;
            ss << "integral number too large: " + int_string;
            errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
        }
        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, value), i64ebx);
    }else {
        value = std::strtod (int_string.c_str(), NULL);
        if((int64_t )value > DA_MAX || (int64_t )value < DA_MIN) {
            stringstream ss;
            ss << "integral number too large: " + int_string;
            errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
        }

        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVBI, ((int64_t)value)), abs(getLowBytes(value)));
        expr->utype->getCode().push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64bmr));
    }

    expr->utype->setResolvedType(new Literal(value));
}

void Compiler::parseHexLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);

    double value;
    string hex_string = invalidateUnderscores(token.getToken());

    value = strtoll(hex_string.c_str(), NULL, 16);
    if(value > DA_MAX || value < DA_MIN) {
        stringstream ss;
        ss << "integral number too large: " + hex_string;
        errors->createNewError(GENERIC, token.getLine(), token.getColumn(), ss.str());
    }
    expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, value), i64ebx);

    expr->utype->setResolvedType(new Literal(value));
}

void Compiler::parseStringLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(true);

    stringMap.addif(token.getToken());
    expr->utype->setResolvedType(new Literal(token.getToken(), stringMap.indexof(token.getToken())));
}

void Compiler::parseBoolLiteral(Expression* expr, Token &token) {
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);
    expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, (token.getToken() == "true" ? 1 : 0)), i64ebx);

    expr->utype->setResolvedType(new Literal(token.getToken() == "true" ? 1 : 0));
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
            if(ast->getEntity(0).getToken() == "true" ||
               ast->getEntity(0).getToken() == "false") {
                parseBoolLiteral(expr, ast->getEntity(0));
            }
            break;
    }

    expr->ast = ast;
}

void Compiler::compileUtypeClass(Expression* expr, Ast* ast) {
    Utype* utype = compileUtype(ast->getSubAst(ast_utype));
    expr->type = exp_var;
    expr->utype->setType(utype_literal);
    expr->utype->setArrayType(false);

    if(utype->isClass()) {
        expr->utype->getCode().push_i64(SET_Di(i64, op_MOVI, utype->getResolvedType()->address), i64ebx);
        expr->utype->setResolvedType(new Literal(utype->getResolvedType()->address));
    } else {
        expr->utype->free();
        errors->createNewError(GENERIC, ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->line,
                               ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier)->col, "expected class");
    }

    expr->ast = ast;
}

void Compiler::compileExpressionList(List<Expression>& lst, Ast* ast) {
    for(long i = 0; i < ast->getSubAstCount(); i++) {
        lst.__new().init();
        compileExpression(&lst.get(i), ast->getSubAst(i));
    }
}

void Compiler::expressionsToParams(List<Expression> &expressions, List<Field*> &params) {
    for(long i = 0; i < expressions.size(); i++) {
        Expression &e = expressions.get(i);
        params.add(new Field());
        if(e.type == exp_null)
            params.get(i)->nullField = true;
        else {
            params.get(i)->isArray = e.utype->isArray();
            params.get(i)->utype = e.utype;
        }
        e.utype = NULL;
    }
}

Method* // TODO: Circle back around to this as it is in its primitive stae there might beissues with how it searches the mehtods with _int64 _int8 etc.
Compiler::findFunction(ClassObject *k, string name, List<Field*> &params, Ast* ast, bool checkBase) {
    List<Method*> funcs;
    List<Method*> matches;
    Method* resolvedFunction;
    k->getFunctionByName(name, funcs, checkBase);

    if(!funcs.empty()) {
        for(long long i = 0; i < funcs.size(); i++) {
            if(simpleParameterMatch(funcs.get(i)->params, params)) {
                return funcs.get(i);
            }
        }

        for(long long i = 0; i < funcs.size(); i++) {
            if(complexParameterMatch(funcs.get(i)->params, params)) {
                matches.add(funcs.get(i));
            }
        }
    }

    if(!matches.empty()) {
        if(matches.size() > 1 && containsParamType(params, VAR)) {
            createNewWarning(GENERIC, __WAMBIG, ast->line, ast->col, "call to method `" + name + "` may be ambiguous");
        }

        return matches.get(0);
    }

    funcs.free();
    return NULL;
}

Method* Compiler::compileSingularMethodUtype(ReferencePointer &ptr, List<Field*> &params, Ast* ast) {
    Method *resolvedMethod = NULL;
    string name = ptr.classes.get(0);

    if(ptr.mod != "") {
        ClassObject *global = findClass(ptr.mod, globalClass, classes);
        if(global != NULL && (resolvedMethod = findFunction(global, name, params, ast)) != NULL) {
            return resolvedMethod;
        }
    } else {
        ClassObject *global = findClass(currModule, globalClass, classes);
        // TODO: check for overloads later i need to see how it comes in
        if((resolvedMethod = findFunction(currentScope()->klass, name, params, ast, true)) != NULL)
            return resolvedMethod;
        else if(global != NULL && (resolvedMethod = findFunction(global, name, params, ast)) != NULL) {
            return resolvedMethod;
        } else if(currentScope()->getLocalField(name) != NULL) {
            Field* field = currentScope()->getLocalField(name)->field;
            if(field->utype && field->utype->getType() == utype_method_prototype) {
                return (Method*)field->utype->getResolvedType();
            }
        } else if(currentScope()->klass->getField(name, true) != NULL) {

            Field* field = currentScope()->klass->getField(name, true);
            if(field->utype && field->utype->getType() == utype_method_prototype) {
                return (Method*)field->utype->getResolvedType();
            }
        } else if(currentScope()->klass->isClassRelated(findClass("std", "_enum_", classes))) {
            ClassObject *klass = currScope.get(currScope.size()-2)->klass;

            // TODO: function or field must be static as we dont own the instance
            if((resolvedMethod = findFunction(klass, name, params, ast, true)) != NULL)
                return resolvedMethod;
            else if(klass->getField(name, true) != NULL) {
                Field* field = klass->getField(name, true);
                if(field->utype && field->utype->getType() == utype_method_prototype) {
                    return (Method*)field->utype->getResolvedType();
                }
            }
        }
    }

    errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + ptr.classes.get(0) +  Method::paramsToString(params) + "`");
    return NULL;
}

Method* Compiler::compileMethodUtype(Expression* expr, Ast* ast) {
    Method *resolvedMethod = NULL;
    ReferencePointer ptr;
    List<Expression> expressions;
    List<Field*> params;

    compileTypeIdentifier(ptr, ast->getSubAst(ast_utype)->getSubAst(ast_type_identifier));
    compileExpressionList(expressions, ast->getSubAst(ast_value_list));
    expressionsToParams(expressions, params);

    if(ptr.classes.singular()) {
        freeList(expressions);
        return compileSingularMethodUtype(ptr, params, ast);
    } else {
        string name = ptr.classes.get(ptr.classes.size() - 1);
        ptr.classes.pop_back();

        Utype *utype = new Utype();
        resolveUtype(ptr, utype, ast);
        if(utype->isClass()) {
            ClassObject* klass = utype->getClass();
            // TODO: add overload need to see how it comes in later
            if((resolvedMethod = findFunction(klass, name, params, ast, true)) != NULL)
                return resolvedMethod;
            else if(klass->getField(name, true) != NULL) {
                Field *field = klass->getField(name, true);
                if(field->type == FNPTR)
                    return (Method*)field->utype->getResolvedType();
                errors->createNewError(GENERIC, ast->line, ast->col, " symbol `" + name + "` is not a function or pointer");
            }
        } else if(utype->getType() == utype_field) {
            errors->createNewError(GENERIC, ast->line, ast->col, " field `" + utype->toString() + "` is not a class");
        } else {
            errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + ptr.classes.get(0) +  Method::paramsToString(params) + "`");
        }
    }

    return resolvedMethod;
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

void Compiler::compileDotNotationCall(Expression* expr, Ast* ast) {

    if(ast->hasSubAst(ast_dot_fn_e)) {
        Ast* branch = ast->getSubAst(ast_dot_fn_e);
        Method* method = compileMethodUtype(expr, branch);

        if(method) {
            
        }
    } else {
        Utype* utype = compileUtype(ast->getSubAst(ast_utype));
        expr->utype->setArrayType(utype->isArray());
        expr->utype->setType(utype->getType());
        expr->utype->setResolvedType(utype->getResolvedType());
        expr->type = utypeToExpressionType(utype);
    }
}

void Compiler::compilePrimaryExpression(Expression* expr, Ast* ast) {
    Ast* branch = ast->getSubAst(0);

    switch(branch->getType()) {
        case ast_literal_e:
            compileLiteralExpression(expr, branch->getSubAst(ast_literal));
            break;
        case ast_utype_class_e:
            compileUtypeClass(expr, branch);
            break;
        case ast_dot_not_e:
            return compileDotNotationCall(expr, branch->getSubAst(ast_dotnotation_call_expr));
    }
}

void Compiler::compileExpression(Expression* expr, Ast* ast) {
    Ast *branch = ast->getSubAst(0);

    switch(branch->getType()) {
        case ast_primary_expr:
            compilePrimaryExpression(expr, branch);
            break;
    }
}

void Compiler::resolvePrototypeField(Ast* ast) {
    string name =  ast->getEntity(0).getToken();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getEntity(1).getToken();
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
        field->utype = new Utype();
        field->utype->setResolvedType(prototype);
        field->utype->setType(utype_method_prototype);
        field->type = FNPTR;

        Meta meta(current->getErrors()->getLine(ast->getEntity(0).getLine()), current->sourcefile,
                  ast->getEntity(0).getLine(), ast->getEntity(0).getColumn());
        prototype->meta.copy(meta);

        if(ast->getSubAst(ast_utype_arg_list_opt)) {
            List<Field*> params;
            parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list_opt));
            validateMethodParams(params, ast->getSubAst(ast_utype_arg_list_opt));

            prototype->params.addAll(params);

            if(ast->hasSubAst(ast_method_return_type)) {
                Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type));
                if(utype->getType() != utype_native && utype->getType() != utype_class) {
                    this->errors->createNewError(GENERIC, ast->line, ast->col, " illegal use of symbol `" + utype->toString() + "` as a return type");
            }
                prototype->utype = utype;
            } else {
                prototype->utype = new Utype();
                prototype->utype->setType(utype_native);

                DataEntity *_void = new DataEntity();
                _void->owner = prototype->owner;
                _void->type = _VOID;
                _void->meta.copy(meta);

                prototype->utype->setResolvedType(_void);
            }
        } else {
            // we need to infer the type
            Expression *expr = new Expression();
            compileExpression(expr, ast->getSubAst(ast_value)->getSubAst(ast_expression));
            Utype *utype = expr->utype;
            if(utype->getType() == utype_field)
                utype = ((Field*)utype->getResolvedType())->utype;

            if(utype->getType() == utype_method || utype->getType() == utype_method_prototype) {
                prototype->params.addAll(((Method*)utype->getResolvedType())->params);
                prototype->utype = ((Method*)utype->getResolvedType())->utype;
            } else {
                errors->createNewError(GENERIC, ast->line, ast->col, "expression being assigned to field `" + name + "` must be either a method or anonymous function");
                prototype->utype = new Utype();
                prototype->utype->setType(utype_native);

                DataEntity *_void = new DataEntity();
                _void->owner = prototype->owner;
                _void->type = _VOID;
                _void->meta.copy(meta);

                prototype->utype->setResolvedType(_void);
            }
        }
    }
}

void Compiler::resolveFieldType(Field* field, Utype *utype, Ast* ast, bool typeInference) {

    field->isArray = utype->isArray();
    if(utype->getType() == utype_class) {
        if(!typeInference) {
            field->type = CLASS;
            field->utype = utype;
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " field`" + field->fullName + "` cannot be assigned type `" + utype->toString() + "` due to invalid type assignment format");
    } else if(utype->getType() == utype_method) {
        if(typeInference) {
            field->type = VAR;
            field->utype = new Utype(VAR);
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " illegal use of function `" + utype->toString() + "` as a type");
    } else if(utype->getType() == utype_native) {
        if(!typeInference) {
            field->type = utype->getResolvedType()->type;
            field->utype = utype;
        } else
            this->errors->createNewError(GENERIC, ast->line, ast->col, " field`" + field->fullName + "` cannot be assigned type `" + utype->toString() + "` due to invalid type assignment format");
    } else if(utype->getType() == utype_literal) {
        if(typeInference) {
            field->type = utype->getResolvedType()->type;
            field->utype = new Utype(utype->getResolvedType()->type, utype->isArray());;
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
                resolveFieldType(field,fieldType->utype, ast, false);
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

void Compiler::resolveField(Ast* ast) {
    string name =  ast->getEntity(0).getToken();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getEntity(1).getToken();
    }

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
            compileExpression(&expr, ast->getSubAst(ast_expression));

            freePtr(field->utype); field->utype = NULL;
            resolveFieldType(field, expr.utype, ast, true);
        }

        // TODO: make sure type is valid before exiting, you cant have a method as the type for the field etc.
        if(ast->hasSubAst(ast_variable_decl)) {
            for(long i = 0; i < ast->getSubAstCount(); i++) {
                Ast *branch = ast->getSubAst(i);

                name =  branch->getEntity(0).getToken();
                Field *xtraField = currentScope()->klass->getField(name, false);

                // we cannot add thread local to secondary variables so me must match its locality with the first one
                xtraField->locality = field->locality;
                if(xtraField->locality == stl_stack)
                    xtraField->address = xtraField->owner->getFieldAddress(xtraField);

                if(xtraField->type == UNTYPED) {
                    xtraField->utype = new Utype();
                    xtraField->utype->setArrayType(field->utype->isArray());
                    xtraField->utype->setType(field->utype->getType());

                    if (field->utype->getResolvedType() != NULL) {
                        switch (field->utype->getType()) {
                            case utype_native:
                                xtraField->utype->setResolvedType(new DataEntity());
                                xtraField->utype->getResolvedType()->ast = field->utype->getResolvedType()->ast;
                                xtraField->utype->getResolvedType()->owner = field->utype->getResolvedType()->owner;
                                xtraField->utype->getResolvedType()->flags.addAll(field->utype->getResolvedType()->flags);
                                xtraField->utype->getResolvedType()->address = field->utype->getResolvedType()->address;
                                xtraField->utype->getResolvedType()->module = field->utype->getResolvedType()->module;
                                xtraField->utype->getResolvedType()->meta.copy(field->utype->getResolvedType()->meta);
                                xtraField->utype->getResolvedType()->fullName = field->utype->getResolvedType()->fullName;
                                xtraField->utype->getResolvedType()->name = field->utype->getResolvedType()->name;
                                xtraField->utype->getResolvedType()->type = field->utype->getResolvedType()->type;
                                break;
                            case utype_class:
                                xtraField->utype->setResolvedType(field->utype->getResolvedType());
                                break;
                            default:
                                xtraField->utype->setType(utype_unresolved);
                                xtraField->utype->setResolvedType(new DataEntity());
                                break;
                        }
                    }
                }
            }
        }
    }
}

void Compiler::resolveClassFields(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();

    if(currentClass == NULL) {
        string name = ast->getEntity(0).getToken();
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
            case ast_func_prototype:
                resolvePrototypeField(branch);
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
        flags.add(strToAccessFlag(ast->getEntity(i).getToken()));

        if(flags.last() == LOCAL && !globalScope()) {
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                         " `" + ast->getEntity(i).getToken() + "` can only be used at global scope");
        } else if(flags.last() == flg_CONST) {
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
                                     ast->getEntity(errPos).getColumn(), " `" + ast->getEntity(errPos).getToken() + "`");
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
        arg->name = ast->getEntity(0).getToken();
        arg->fullName = arg->name;
    } else {
        stringstream ss;
        ss << "arg" << guid++;
        arg->name = ss.str();
        arg->fullName = arg->name;
    }

    resolveFieldType(arg, utype, ast, false);
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
    field->name = ast->getEntity(0).getToken();
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
        Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type));
        prototype->utype = utype;
    } else {
        prototype->utype = new Utype();
        prototype->utype->setType(utype_native);

        DataEntity *_void = new DataEntity();
        _void->owner = prototype->owner;
        _void->type = _VOID;
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

                } else if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == _VOID) { // fn main(string[]);
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
                if(method->utype->getType() == utype_native && method->utype->getResolvedType()->type == _VOID) { // fn main();
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
    string name =  ast->getEntity(0).getToken() + ast->getEntity(1).getToken();
    string op =  ast->getEntity(1).getToken();
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

    if(ast->hasSubAst(ast_method_return_type)) {
        Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
        method->utype = utype;
    } else {
        method->utype = new Utype();
        method->utype->setType(utype_native);

        DataEntity *_void = new DataEntity();
        _void->owner = method->owner;
        _void->type = _VOID;
        _void->meta.copy(meta);

        method->utype->setResolvedType(_void);
    }

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
    string name =  ast->getEntity(0).getToken();
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
    string name =  ast->getEntity(0).getToken();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_delegate_impl;
    method->address = methodSize++;

    if(ast->hasSubAst(ast_method_return_type)) {
        Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
        method->utype = utype;
    } else {
        method->utype = new Utype();
        method->utype->setType(utype_native);

        DataEntity *_void = new DataEntity();
        _void->owner = method->owner;
        _void->type = _VOID;
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
    string name =  ast->getEntity(0).getToken();
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
        Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
        method->utype = utype;
    } else {
        method->utype = new Utype();
        method->utype->setType(utype_native);

        DataEntity *_void = new DataEntity();
        _void->owner = method->owner;
        _void->type = _VOID;
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

void Compiler::resolveMethod(Ast* ast) {
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
    string name =  ast->getEntity(0).getToken();
    parseUtypeArgList(params, ast->getSubAst(ast_utype_arg_list));
    validateMethodParams(params, ast->getSubAst(ast_utype_arg_list));

    Meta meta(current->getErrors()->getLine(ast->line), current->sourcefile,
              ast->line, ast->col);

    Method *method = new Method(name, currModule, currentScope()->klass, params, flags, meta);
    method->fullName = currentScope()->klass->fullName + "." + name;
    method->ast = ast;
    method->fnType = fn_normal;
    method->address = methodSize++;

    if(ast->hasSubAst(ast_method_return_type)) {
        Utype* utype = compileUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
        method->utype = utype;
    } else {
        method->utype = new Utype();
        method->utype->setType(utype_native);

        DataEntity *_void = new DataEntity();
        _void->owner = method->owner;
        _void->type = _VOID;
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

void Compiler::resolveClassMethods(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;

    if(currentClass == NULL) {
        string name = ast->getEntity(0).getToken();
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
            default:
                break;
        }
    }
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

    // we want to add all the delegates we need to validate
    if((contracter = subscriber->getSuperClass()) != NULL) {
        contracter->getAllFunctionsByType(fn_delegate, contractedMethods);
    }

    for(long long i = 0; i < subscriber->getInterfaces().size(); i++) {
        contracter = subscriber->getInterfaces().get(i);
        contracter->getAllFunctionsByType(fn_delegate, contractedMethods);

        if(contracter->getSuperClass() != NULL) {
            for(;;) {

                contracter = contracter->getSuperClass();
                contracter->getAllFunctionsByType(fn_delegate, contractedMethods);

                if(contracter->getSuperClass() == NULL)
                    break;
            }
        }
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
            }

            if(!(contract->flags.size() == sub->flags.size() && contract->flags.sameElements(sub->flags))) {
                stringstream err;
                err << "method `" << sub->toString() << "` provided different access privileges than contract method '"
                    << contract->toString() << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        } else {
            stringstream err;
            err << "no contract found in class `" << subscriber->fullName << "` for method '"
                << subscribedMethods.get(i)->toString() << "'";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
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
            }

            if(!(contract->flags.size() == sub->flags.size() && contract->flags.sameElements(sub->flags))) {
                stringstream err;
                err << "method `" << sub->toString() << "` provided different access privileges than contract method '"
                    << contract->toString() << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        } else {
            stringstream err;
            err << "contract method `" << contract->toString() << "` does not have a subscribed method implemented in class '"
                << subscriber->fullName << "'";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
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
        string name = ast->getEntity(0).getToken();
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
                    resolveMethod(branch);
                    break;
                case ast_interface_decl:
                    resolveClassMethods(branch);
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
                case ast_func_prototype:
                    resolvePrototypeField(branch);
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
        ptr.classes.add(ast->getEntity(0).getToken());
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

    if(isDClassNumberEncodable(value))
        code.push_i64(SET_Di(i64, op_MOVI, value), i64ebx);
    else {
        code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)value)), abs(getLowBytes(value)));
        code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64bmr));
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

    if(currentScope()->type >= INSTANCE_BLOCK && currentScope()->getLocalField(name) != NULL) {
        Field* field = currentScope()->getLocalField(name)->field;

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);

        if(field->isVar()) {
            if(field->isArray || field->locality == stl_thread) {
                if(field->locality == stl_thread) {
                    utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
                } else {
                    utype->getCode().push_i64(SET_Di(i64, op_MOVL, field->address));
                }
            } else {
                utype->getCode().push_i64(SET_Ci(i64, op_LOADL, i64ebx, 0, field->address));
            }
        }
        else {
            if(field->locality == stl_thread) {
                utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
            } else {
                utype->getCode().push_i64(SET_Di(i64, op_MOVL, field->address));
            }
        }
    } else if((resolvedUtype = currentScope()->klass->getField(name, true)) != NULL) {
        Field* field = (Field*)resolvedUtype;
        if(currentScope()->type == STATIC_BLOCK && !field->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col,
                                   "cannot get field `" + name + "` from self in static context");
        }

        compileFieldType(field);

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);
        validateAccess(field, ast);

        if(isFieldInlined(field)) {
            inlineVariableValue(utype->getCode(), field);
        } else {
            if(field->locality == stl_thread) {
                utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
            } else {
                if(field->flags.find(STATIC) || field->owner->isGlobalClass())
                    utype->getCode().push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                else
                    utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address));
            }
        }
    }  else if(IS_CLASS_ENUM(currentScope()->klass->getClassType()) && (resolvedUtype = currScope.get(currScope.size() - 2)->klass->getField(name, true)) != NULL) {
        Field* field = (Field*)resolvedUtype;
        if(currentScope()->type == STATIC_BLOCK && !field->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col,
                                   "cannot get field `" + name + "` from self in static context");
        }

        compileFieldType(field);

        utype->setType(utype_field);
        utype->setResolvedType(field);
        utype->setArrayType(field->isArray);
        validateAccess(field, ast);

        if(isFieldInlined(field)) {
            inlineVariableValue(utype->getCode(), field);
        } else {
            if(field->locality == stl_thread) {
                utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
            } else {
                if(field->flags.find(STATIC) || field->owner->isGlobalClass())
                    utype->getCode().push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                else
                    utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address));
            }
        }
    } else if((resolvedUtype = resolveEnum(name)) != NULL) {
        validateAccess((Field*)resolvedUtype, ast);
        utype->setType(utype_field);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(((Field*)resolvedUtype)->isArray);

        utype->getCode().push_i64(SET_Di(i64, op_MOVG, resolvedUtype->owner->address));
        utype->getCode().push_i64(SET_Di(i64, op_MOVN, resolvedUtype->address));
        return;
    } else if((resolvedUtype = resolveClass(ptr.mod, name, ast)) != NULL) {
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
        validateAccess((ClassObject*)resolvedUtype, ast);
    } else if((resolvedUtype = currentScope()->klass->getChildClass(name)) != NULL && !IS_CLASS_GENERIC(((ClassObject*)resolvedUtype)->getClassType())) {
        // global class ?
        utype->setType(utype_class);
        utype->setResolvedType(resolvedUtype);
        utype->setArrayType(false);
    } else if(resolveClass(generics, resolvedClasses, ptr.mod, name, ast, true)) {
        if(resolvedClasses.size() == 1) {
            stringstream helpfulMessage;
            helpfulMessage << "have you forgotten your type parameters? Were you possibly looking for class `"
                           << resolvedClasses.get(0)->fullName << "` it requires " << resolvedClasses.get(0)->getKeys().size()
                           << " generic types.";
            errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line,
                                   ast->getSubAst(ast_type_identifier)->col,
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
                for(long i = 0; i < klass->getKeys().size(); i++) {
                    if(i > 5) {
                        helpfulMessage << "...";
                        break;
                    }

                    helpfulMessage << klass->getKeys().get(i);
                    if((i+1) < klass->getKeys().size()) {
                        helpfulMessage << ", ";
                    }
                }
                helpfulMessage << ">";
                if((i+1) < resolvedClasses.size())
                    helpfulMessage << ", ";
            }

            helpfulMessage << "\n\t}\n";

            errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line,
                                   ast->getSubAst(ast_type_identifier)->col,
                                   " `" + name + "` " +
                                   helpfulMessage.str());
        }
    }  else if(resolveClass(classes, resolvedClasses, ptr.mod, name, ast, true)) {
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

        errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line,
                               ast->getSubAst(ast_type_identifier)->col,
                               " `" + name + "` " +
                               helpfulMessage.str());
    } else if(currentScope()->klass->getFunctionByName(name, functions)) {
        if(functions.size() > 1)
            createNewWarning(GENERIC, __WAMBIG, ast->line, ast->col, "reference to function `" + functions.get(0)->name + "` is ambiguous");

        utype->setType(utype_method);
        utype->setResolvedType(functions.get(0));
        utype->setArrayType(false);
        if(!functions.get(0)->flags.find(STATIC)) {
            errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " cannot get address from non static function `"
                                                                                                                                 + name + "` ");
        }
        utype->getCode().push_i64(SET_Di(i64, op_MOVI, functions.get(0)->address), i64ebx);
    } else {
        ClassObject* global = resolveClass(currModule, globalClass, ast);
        functions.free();

        if((resolvedUtype = global->getField(name, true)) != NULL) {
            Field* field = (Field*)resolvedUtype;
            compileFieldType(field);

            utype->setType(utype_field);
            utype->setResolvedType(field);
            utype->setArrayType(field->isArray);
            validateAccess(field, ast);

            if(isFieldInlined(field)) {
                inlineVariableValue(utype->getCode(), field);
            } else {
                if(field->locality == stl_thread) {
                    utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, field->address));
                } else {
                    if(field->flags.find(STATIC) || field->owner->isGlobalClass())
                        utype->getCode().push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    else
                        utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                    utype->getCode().push_i64(SET_Di(i64, op_MOVN, field->address));
                }
            }
        } else if(global->getFunctionByName(name, functions)) {
            if(functions.size() > 1)
                createNewWarning(GENERIC, __WAMBIG, ast->line, ast->col, "reference to function `" + functions.get(0)->name + "` is ambiguous");

            utype->setType(utype_method);
            utype->setResolvedType(functions.get(0));
            utype->setArrayType(false);
            if(!functions.get(0)->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " cannot get address from non static function `"
                                                                                                                                     + name + "` ");
            }
            utype->getCode().push_i64(SET_Di(i64, op_MOVI, functions.get(0)->address), i64ebx);
            return;
        }

        errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line,
                               ast->getSubAst(ast_type_identifier)->col,
                               " `" + name + "` ");
    }
}

/*
 * The purposes of this function is to "forward" process a field's type and value if it hasn't
 * been processed yet to prevent weird incorrect undefined errors from the compiler
 */
void Compiler::compileFieldType(Field *field) {
    if(field->type == UNTYPED) {
        currScope.add(new Scope(field->owner, field->owner->isGlobalClass() ? GLOBAL_SCOPE : CLASS_SCOPE));
        resolveField(field->ast);
        removeScope();
    }

    if(!field->inlineCheck) {
        currScope.add(new Scope(field->owner, field->owner->isGlobalClass() ? GLOBAL_SCOPE : CLASS_SCOPE));
        inlineField(field->ast);
        removeScope();
    }
}

void Compiler::resolveClassHeiarchy(DataEntity* data, ReferencePointer& ptr, Utype* utype, Ast* ast) {
    bool requiresStatic = utype->getType() == utype_field;
    string name = "";

    Field* resolvedField = NULL;
    ClassObject *resolvedClass, *klass = utype->getType() == utype_field ? ((ClassObject*)((Field*)data)->utype->getResolvedType()) : (ClassObject*)data;
    bool lastReference;

    for(unsigned int i = 1; i < ptr.classes.size(); i++) {
        lastReference = (i+1) >= ptr.classes.size();

        name = ptr.classes.at(i);
        if((resolvedClass = klass->getChildClass(name)) != NULL) {
            if(resolvedField != NULL) {
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " expected class or module before `" + name + "` ");
            }

            utype->getCode().free();
            utype->getCode().push_i64(SET_Di(i64, op_MOVG, resolvedClass->address));
            klass = resolvedClass;

            if(lastReference) {
                utype->setType(utype_class);
                utype->setResolvedType(klass);
                utype->setArrayType(false);
                validateAccess(klass, ast);
            }
        } else if((resolvedField = klass->getField(name, true)) != NULL) {
                compileFieldType(resolvedField);

                if(!lastReference && resolvedField->isArray) {
                    errors->createNewError(INVALID_ACCESS, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " field array");
                } else if(requiresStatic && !resolvedField->flags.find(STATIC) && resolvedField->type != UNDEFINED) {
                    errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "static access on instance field `" + name + "`");
                }

                validateAccess(resolvedField, ast);
                if(resolvedField->locality == stl_thread) {
                    utype->getCode().free();
                    utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, resolvedField->address));
                } else {
                    if (i == 1) {
                        if (resolvedField->flags.find(STATIC))
                            utype->getCode().push_i64(SET_Di(i64, op_MOVG, resolvedField->owner->address));
                        else
                            utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                    }

                    if (isFieldInlined(resolvedField) && !resolvedField->isEnum()) {
                        inlineVariableValue(utype->getCode(), resolvedField);
                    } else
                        utype->getCode().push_i64(SET_Di(i64, op_MOVN, resolvedField->address));
                }

                if(lastReference) {
                    utype->setType(utype_field);
                    utype->setResolvedType(resolvedField);
                    utype->setArrayType(resolvedField->isArray);
                }

                switch(resolvedField->type) {
                    case VAR:
                        if(!lastReference) {
                            errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col,
                                                   "invalid dot notation access to non-class field `" + name + "`");

                            utype->setType(utype_field);
                            utype->setResolvedType(resolvedField);
                            utype->setArrayType(resolvedField->isArray);
                            return;
                        }
                        break;
                    case CLASS:
                        klass = (ClassObject*)resolvedField->utype->getResolvedType();
                        requiresStatic = false;
                        break;
                }
        }  else {
            errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + name + "` " +
                                                      (ptr.mod == "" ? "" : "in module {" + ptr.mod + "} "));
            return;
        }
    }
}

void Compiler::resolveUtype(ReferencePointer &ptr, Utype* utype, Ast *ast) {
    if(ptr.classes.singular() && ptr.mod == "") {
        resolveSingularUtype(ptr, utype, ast);
    } else if(ptr.classes.singular() && ptr.mod != "") {
        DataEntity* resolvedUtype = NULL;
        string name = ptr.classes.get(0);
        List<Method*> functions;
        ClassObject* global = resolveClass(ptr.mod, globalClass, ast);

        if((resolvedUtype = resolveClass(ptr.mod, name, ast)) != NULL) {
            utype->setType(utype_class);
            utype->setResolvedType(resolvedUtype);
            utype->setArrayType(false);
            validateAccess((ClassObject*)resolvedUtype, ast);
        } else if(global != NULL && global->getFunctionByName(name, functions)) {
            if(functions.size() > 1)
                createNewWarning(GENERIC, __WAMBIG, ast->line, ast->col, "reference to function `" + functions.get(0)->name + "` is ambiguous");
            utype->setType(utype_method);
            utype->setResolvedType(resolvedUtype);
            utype->setArrayType(false);

            if(!functions.get(0)->flags.find(STATIC)) {
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " function pointer `" + name + "` " +
                                                "in module {" + ptr.mod + "} " + " must be static");
            }
            utype->getCode().push_i64(SET_Di(i64, op_MOVI, functions.get(0)->address), i64ebx);
        } else {
            errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + name + "` " +
                                                      "in module {" + ptr.mod + "} ");
        }
    } else {
        DataEntity* resolvedUtype = NULL;
        string name = ptr.classes.get(0);
        List<Method*> functions;

        if((resolvedUtype = resolveClass(ptr.mod, name, ast)) != NULL) {
            utype->setType(utype_class);
            resolveClassHeiarchy(resolvedUtype, ptr, utype, ast);
        } else if(currentScope()->type != CLASS_SCOPE && currentScope()->getLocalField(name) != NULL) {
            resolvedUtype = currentScope()->getLocalField(name)->field;

            /**
             * You cannot access a var in this manner because it
             * is not a data structure
             */
            if(resolvedUtype->type != CLASS) {
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col,
                                       "field `" + name + "` is not a class");
                return;
            }
            else {
                if(((Field*)resolvedUtype)->locality == stl_thread) {
                    utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, resolvedUtype->address));
                } else {
                    utype->getCode().push_i64(SET_Di(i64, op_MOVL, resolvedUtype->address));
                }
            }

            validateAccess((Field*)resolvedUtype, ast);
            resolveClassHeiarchy(((Field*)resolvedUtype)->utype->getResolvedType(), ptr, utype, ast);
        } else if((resolvedUtype = currentScope()->klass->getField(name, true)) != NULL) {
            compileFieldType((Field*)resolvedUtype);
            validateAccess((Field*)resolvedUtype, ast);

            if(((Field*)resolvedUtype)->locality == stl_thread) {
                utype->getCode().push_i64(SET_Di(i64, op_TLS_MOVL, resolvedUtype->address));
            } else {
                if(currentScope()->type == STATIC_BLOCK && !resolvedUtype->flags.find(STATIC)) {
                    errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col,
                                           "cannot get object `" + name + "` from self in static context");
                }

                if (resolvedUtype->flags.find(STATIC))
                    utype->getCode().push_i64(SET_Di(i64, op_MOVG, resolvedUtype->owner->address));
                else {
                    utype->getCode().push_i64(SET_Di(i64, op_MOVL, 0));
                }
                utype->getCode().push_i64(SET_Di(i64, op_MOVN, resolvedUtype->address));
            }

            validateAccess((Field*)resolvedUtype, ast);
            resolveClassHeiarchy(((Field*)resolvedUtype)->utype->getResolvedType(), ptr, utype, ast);
            return;
        } else if((resolvedUtype = currentScope()->klass->getChildClass(name)) != NULL) {
            resolveClassHeiarchy(resolvedUtype, ptr, utype, ast);
            return;
        } else {
            name = ptr.classes.last();
            ptr.classes.pop_back();

            resolveUtype(ptr, utype, ast);
            if(utype->getType() == utype_class || (utype->getType() == utype_field && utype->getResolvedType()->type == CLASS)) {
                DataEntity* klass = utype->getType() == utype_class? utype->getResolvedType() : ((Field*)utype->getResolvedType())->utype->getResolvedType();
                // TODO: look into adding function overloads as well
                if(((ClassObject*)klass)->getFunctionByName(name, functions)) {
                    if(functions.size() > 1)
                        createNewWarning(GENERIC, __WAMBIG, ast->line, ast->col, "reference to function `" + functions.get(0)->name + "` is ambiguous");
                    utype->setType(utype_method);
                    utype->setResolvedType(resolvedUtype);
                    utype->setArrayType(false);

                    if(!functions.get(0)->flags.find(STATIC)) {
                        errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " function pointer `" + name + "` " +
                                                                                                                                             "in module {" + ptr.mod + "} " + " must be static");
                    }
                    utype->getCode().push_i64(SET_Di(i64, op_MOVI, functions.get(0)->address), i64ebx);
                    return;
                }
            }
            utype->getCode().free();

            errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + name + "` " +
                                                      (ptr.mod == "" ? "" : "in module {" + ptr.mod + "} "));
        }
    }
}

Utype* Compiler::compileUtype(Ast *ast) {
    if(ast->getType() != ast_utype)
        throw runtime_error("check parseUtype()");

    Utype *utype = new Utype();
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

    ClassObject *generic = parent != NULL ? parent->getChildClass(name) : findClass(mod, name, generics);
    if(generic && utypes.size() == generic->getKeys().size()) {
        stringstream ss;

        // build unique class name
        ss << generic->name << "<";
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
            newClass->setSuperClass(parent);
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
                resolveAllDelegates(newClass->ast, newClass);

                if(processingStage > POST_PROCESSING) {
                    // post processing code
                    resolveClassFields(newClass->ast, newClass);
                    inlineClassFields(newClass->ast, newClass);

                    // TODO: add additional code

                    if (IS_CLASS_INTERFACE(newClass->getClassType()))
                        newClass->setClassType(class_interface);
                    else
                        newClass->setClassType(class_normal);
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
            module << ast->getEntity(i).getToken();
            if(ast->getEntity(i+1) == "#") {
                hashProcessed = true;
                ptr.mod = module.str();
                i++;
            }
        } else {
            string name = ast->getEntity(i).getToken();
            if(name != ".") {
                if (!failed && ((i + 1) < ast->getEntityCount()) && ast->getEntity(i + 1) == "<") {
                    ClassObject *genericClass = compileGenericClassReference(
                            ptr.mod, name, parent, ast->getSubAst(genericListPos++));
                    ptr.classes.add(genericClass ? genericClass->name : "?");

                    parent = genericClass;
                    failed = parent == NULL;
                    i += 2;
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

ClassObject *Compiler::resolveClassReference(Ast *ast, ReferencePointer &ptr) {
    if(ptr.classes.singular()) {
        ClassObject* klass = resolveClass(ptr.mod, ptr.classes.get(0), ast);
        if(klass == NULL) {
            if(findClass(ptr.mod, ptr.classes.get(0), generics)) {
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
            if(findClass(ptr.mod, ptr.classes.get(0), generics)) {
                errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0)
                                                                + "`. It was found to be a generic class, have you missed the key parameters to the class perhaps?");
            } else {
                errors->createNewError(COULD_NOT_RESOLVE, ast, " `" + ptr.classes.get(0) + "`");
            }
        } else {
            Utype utype;
            utype.setType(utype_class);
            resolveClassHeiarchy(klass, ptr, &utype, ast);

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
            if (currClass->getSuperClass() != NULL && (currentScope()->klass->isClassRelated(base) ||
                    base->isClassRelated(currClass))) {
                errors->createNewError(GENERIC, ast->getSubAst(0)->line, ast->getSubAst(0)->col,
                                       "cyclic dependency of class `" + currentScope()->klass->fullName + "` in parent class `" +
                                       base->fullName + "`");
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

    string name =  ast->getEntity(0).getToken();

    if(currentClass == NULL) {
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
    if(IS_CLASS_INTERFACE(currentClass->getClassType())) {
        if ( base != NULL && !IS_CLASS_INTERFACE(base->getClassType())) {
            stringstream err;
            err << "interface '" << currentClass->fullName << "' must inherit another interface class";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            err.str("");
        } else
            currentClass->setSuperClass(base);
    } else
        currentClass->setSuperClass(base);

    // we also want to resolve any interfaces as well
    if(ast->hasSubAst(ast_reference_pointer_list)) {
        List<ReferencePointer*> refPtrs;
        List<ClassObject*> interfaces;
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
                err << "duplicate class '" << _interface->name << "'";
                errors->createNewError(GENERIC, branch->line, branch->col, err.str());
            } else
                interfaces.addif(_interface);
        }

        currentClass->setInterfaces(interfaces);
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
    string name =  ast->getEntity(0).getToken();
    Token tmp(name, IDENTIFIER, 0, 0);
    if(parser::isStorageType(tmp)) {
        name = ast->getEntity(1).getToken();
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
    string name =  ast->getEntity(0).getToken();
    Field *field = currentScope()->klass->getField(name, false);

    if(!field->inlineCheck) {
        field->inlineCheck = true;

        if(ast->hasSubAst(ast_expression)) {
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
        } else {
            inlinedFields.add(KeyPair<Field*, double>(field, guid++));
        }
    }
}

void Compiler::inlineEnumFields(Ast* ast, ClassObject* currentClass) {
    Ast* block = ast->getSubAst(ast_enum_identifier_list);

    if(currentClass == NULL) {
        string name = ast->getEntity(0).getToken();
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
        string name = ast->getEntity(0).getToken();
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
            default:
                /* ignore */
                break;
        }
    }
    removeScope();
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

    preProcessGenericClasses(unstableClasses);
    inheritRespectiveClasses();
    resolveAllMethods();
    resolveAllDelegates();
    resolveAllFields();
    inlineFields();
    postProcessGenericClasses();

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
                case ast_func_prototype:
                    preProccessVarDecl(branch);
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

void Compiler::compile() {

    if(preprocess() && postProcess()) {
        processingStage = COMPILING;
        // TODO: write compileUnprocessedClasses() to be called for everything else and free unprocessedClasses list

        cout << "ready to compile!!!";
    }
}

string Compiler::parseModuleDecl(Ast *ast) {
    string module = ast->getEntity(0).getToken();
    for(long i = 1; i < ast->getEntityCount(); i++)
    {
        module += ast->getEntity(i).getToken();
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
    if(c_options.warnings) {
        if(warning_map[type]) {
            if(c_options.werrors){
                errors->createNewError(error, line, col, xcmnts);
            } else {
                errors->createNewWarning(error, line, col, xcmnts);
            }
        }
    }
}

void Compiler::createGlobalClass() {
    List<AccessFlag > flags;
    flags.add(PUBLIC);
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

Method*
Compiler::findFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &)) {
    List<Method*> funcs;
    k->getFunctionByName(method->name, funcs);

    if(!funcs.empty()) {
        for(long long i = 0; i < funcs.size(); i++) {
            if(paramaterMatchFun(funcs.get(i)->params, method->params)) {
                funcs.free();
                return funcs.get(i);
            }
        }
    }

    funcs.free();
    return NULL;
}
