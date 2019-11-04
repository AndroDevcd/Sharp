//
// Created by BraxtonN on 10/18/2019.
//

#ifndef SHARP_COMPILER_H
#define SHARP_COMPILER_H

#include "../../../stdimports.h"
#include "../frontend/parser/Parser.h"
#include "Scope.h"

class Compiler {

public:
    Compiler(string outFile,List<parser*> &parsers)
    :
        outFile(outFile),
        failed(0),
        succeeded(0),
        errCount(0),
        rawErrCount(0),
        guid(0),
        classSize(0),
        methodSize(0),
        threadLocals(0),
        currModule(""),
        panic(false),
        lastNoteMsg(""),
        lastNote(),
        errors(NULL),
        current(NULL)
    {
        modules.init();
        sourceFiles.init();
        noteMessages.init();
        classes.init();
        generics.init();
        currScope.init();
        importMap.init();
        this->parsers.init();
        this->parsers.addAll(parsers);

        compile();
    }

    template<class T>
    static void freeList(List<T> &lst)
    {
        for(unsigned int i = 0; i < lst.size(); i++)
        {
            lst.get(i).free();
        }
        lst.free();
    }

    template<class T>
    static void freeListPtr(List<T> &lst)
    {
        for(unsigned int i = 0; i < lst.size(); i++)
        {
            lst.get(i)->free();
        }
        lst.free();
    }

    template<class T>
    static void freeList(list<T> &lst)
    {
        for(T item : lst)
        {
            item.free();
        }
        lst.clear();
    }

    void generate();
    void cleanup();

    static bool simpleParameterMatch(List<Field*> &params, List<Field*> &comparator);

    List<string> failedParsers;
    List<string> succeededParsers;
    long long failed, succeeded;
    long errCount, rawErrCount;
private:
    bool panic;
    long guid;
    long classSize;
    long methodSize;
    long threadLocals;
    string outFile;
    string lastNoteMsg;
    Meta lastNote;
    List<parser*> parsers;
    List<string> modules;
    List<string> sourceFiles;
    List<string> noteMessages;
    List<ClassObject*> classes;
    List<ClassObject*> generics;
    List<Scope*> currScope;
    List<KeyPair<string, List<string>>>  importMap;
    parser* current;
    ErrorManager *errors;
    string currModule;

    bool preprocess();
    void compile();
    void createGlobalClass();
    void addDefaultConstructor(ClassObject* klass, Ast* ast);
    void createNewWarning(error_type error, int type, int line, int col, string xcmnts);
    string parseModuleDecl(Ast* ast);
    void parseClassAccessFlags(List<AccessFlag> &flags, Ast *ast);
    void preProccessClassDecl(Ast* ast, bool isInterface);
    void preProccessGenericClassDecl(Ast* ast, bool isInterface);
    void parseIdentifierList(Ast *ast, List<string> &idList);
    StorageLocality strtostl(string locality);
    void preProccessVarDeclHelper(List<AccessFlag>& flags, Ast* ast);
    void preProccessVarDecl(Ast* ast);
    int64_t checkstl(StorageLocality locality);
    void parseVariableAccessFlags(List<AccessFlag> &flags, Ast *ast);
    ClassObject* addChildClassObject(string name, List<AccessFlag> &flags, ClassObject* owner, Ast* ast);
    void removeScope();
    void preProccessImportDecl(Ast *branch, List<string> &imports);
    void preproccessImports();
    void preProccessEnumDecl(Ast *ast);
    void preProccessEnumVar(Ast *ast);
    AccessFlag strToAccessFlag(string str);
    void printNote(Meta& meta, string msg);
    ClassObject* addGlobalClassObject(string name, List<AccessFlag>& flags, Ast *ast, List<ClassObject*> &classList);
    ClassObject* findClass(string mod, string className, List<ClassObject*> &classes);
};

#define CHECK_CMP_ERRORS(exit_proc) \
    if(panic) exit_proc \
    else if((rawErrCount + errors->getUnfilteredErrorCount()) > _SHARP_CERROR_LIMIT) { \
        panic = true; \
        exit_proc \
    }

#define currentScope() \
    (currScope.last())

#endif //SHARP_COMPILER_H
