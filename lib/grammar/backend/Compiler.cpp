//
// Created by BraxtonN on 10/18/2019.
//

#include "Compiler.h"
#include "../main.h"
#include "oo/ClassObject.h"

string globalClass = "__SRT_Global";
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

            if(flag==LOCAL && currentScope()->type != GLOBAL_SCOPE) {
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

        if(flags.last() == LOCAL && currentScope()->type != GLOBAL_SCOPE) {
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
    long long stlAddress = IS_CLASS_GENERIC(currentScope()->klass->getClassType()) ? 0 : checkstl(locality);
    if((prevField = currentScope()->klass->getField(name, false)) != NULL) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "field `" + name + "` is already defined in the scope");
        printNote(prevField->meta, "field `" + name + "` previously defined here");
    } else {
        currentScope()->klass->addField(new Field(UNDEFINED, guid++, name, NULL, flags, meta, locality, stlAddress));
    }
}
void Compiler::preProccessVarDecl(Ast* ast) {
    List<AccessFlag> flags;

    if(ast->hasSubAst(ast_access_type)){
        parseVariableAccessFlags(flags, ast->getSubAst(ast_access_type));

        if(currentScope()->type == GLOBAL_SCOPE) {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PUBLIC);
            flags.addif(STATIC);
        } else {
            if(!flags.find(PUBLIC) && !flags.find(PRIVATE) && !flags.find(PROTECTED))
                flags.add(PRIVATE);
        }
    } else {
        if(currentScope()->type == GLOBAL_SCOPE) {
            flags.add(PUBLIC);
            flags.add(STATIC);
        } else
            flags.add(PRIVATE);
    }

    preProccessVarDeclHelper(flags, ast);

    if(ast->hasSubAst(ast_variable_decl)) {
        for(long i = 0; i < i < ast->getSubAstCount(); i++) {
            preProccessVarDeclHelper(flags, ast->getSubAst(i));
        }
    }
}

void Compiler::parseIdentifierList(Ast *ast, List<string> &idList) {
    ast = ast->getSubAst(ast_identifier_list);

    for(long i = 0; i < ast->getEntityCount(); i++) {
        string Key = ast->getEntity(i).getToken();
        if(Key != ",") {
            idList.push_back(Key);
        }
    }
}

void Compiler::preProccessGenericClassDecl(Ast* ast, bool isInterface) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;
    ClassObject* currentClass;

    if(ast->hasSubAst(ast_access_type)){
        parseClassAccessFlags(flags, ast->getSubAst(ast_access_type));
    } else {
        flags.add(PUBLIC);
    }

    string className =  ast->getEntity(0).getToken();
    List<string> identifierList;
    parseIdentifierList(ast, identifierList);

    if(currentScope()->klass == NULL) {
        currentClass = addGlobalClassObject(className, flags, ast, generics);

        stringstream ss;
        ss << currModule << "#" << currentClass->name;
        currentClass->fullName = ss.str();
    }
    else {
        currentClass = addChildClassObject(className, flags, currentScope()->klass, ast);

        stringstream ss;
        ss << currentScope()->klass->fullName << "." << currentClass->name;
        ss << "<";
        for(long i = 0; i < identifierList.size(); i++) {
            ss << identifierList.get(i);

            if((i + 1) < identifierList.size()) {
                ss << ", ";
            }
        }
        ss << ">";
        currentClass->fullName = ss.str();
    }
    currentClass->addKeys(identifierList);

    if(isInterface)
        currentClass->setClassType(class_interface | class_generic);
    else
        currentClass->setClassType(class_generic);
    currentClass->ast = ast;
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
            case ast_func_prototype:
                preProccessVarDecl(branch);
                break;
            case ast_method_decl: /* Will be parsed later */
                break;
            case ast_operator_decl: /* Will be parsed later */
                break;
            case ast_construct_decl: /* Will be parsed later */
                break;
            case ast_delegate_impl: /* Will be parsed later */
                break;
            case ast_delegate_decl: /* Will be parsed later */
                break;
            case ast_enum_decl: /* Will be parsed later */
                preProccessEnumDecl(branch);
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
            default:
                stringstream err;
                err << ": unknown ast type: " << branch->getType();
                errors->createNewError(INTERNAL_ERROR, branch->line, branch->col, err.str());
                break;
        }
    }
    removeScope();
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
        currentScope()->klass->addField(new Field(UNDEFINED, guid++, name, NULL, flags, meta, stl_stack, 0));
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

    if(currentScope()->klass == NULL) {
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

void Compiler::preProccessClassDecl(Ast* ast, bool isInterface) {
    Ast* block = ast->getLastSubAst();
    List<AccessFlag> flags;
    ClassObject* currentClass;

    if(ast->hasSubAst(ast_access_type)){
        parseClassAccessFlags(flags, ast->getSubAst(ast_access_type));
    } else {
        flags.add(PUBLIC);
    }

    string className =  ast->getEntity(0).getToken();
    if(currentScope()->klass == NULL) {
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

    currentClass->setClassType(isInterface ? class_interface : class_normal);
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

bool Compiler::preprocess() {

    bool success = true;
    for(unsigned long i = 0; i < parsers.size(); i++) {
        current = parsers.get(i);
        errors = new ErrorManager(current->lines, current->sourcefile, true, c_options.aggressive_errors);

        currModule = "$unknown";
        KeyPair<string, List<string>> resolveMap;
        List<string> imports;

        sourceFiles.addif(current->sourcefile);
        currScope.add(new Scope(NULL, GLOBAL_SCOPE));
        for(unsigned long x = 0; x < current->size(); x++)
        {
            Ast *branch = current->astAt(x);
            if(x == 0)
            {
                if(branch->getType() == ast_module_decl) {
                    modules.addif(currModule = parseModuleDecl(branch));
                    imports.push_back(currModule);
                    // add class for global methods
                    createGlobalClass();
                    currScope.last()->klass = findClass(currModule, globalClass, classes);
                    continue;
                } else {
                    modules.addif(currModule = "__srt_undefined");
                    imports.push_back(currModule);
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

        resolveMap.set(current->sourcefile, imports);
        importMap.push_back(resolveMap);
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

    return success;
}

void Compiler::compile() {

    if(preprocess()) {
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

ClassObject* Compiler::findClass(string mod, string className, List<ClassObject*> &classes) {
    ClassObject* klass = NULL;
    for(unsigned int i = 0; i < classes.size(); i++) {
        klass = classes.get(i);
        if(klass->name == className) {
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

        method->type=fn_constructor;
        if(!IS_CLASS_GENERIC(klass->getClassType()))
            method->address = methodSize++;
        klass->addConstructor(method);
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
        stringstream ss;
        ss << currModule << "#" << global->name;
        global->fullName = ss.str();
        addDefaultConstructor(global, NULL);
        classes.add(global); // global class ref
    }

    flags.free();
}
