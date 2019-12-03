//
// Created by BraxtonN on 10/18/2019.
//

#ifndef SHARP_COMPILER_H
#define SHARP_COMPILER_H

#include "../../../stdimports.h"
#include "../frontend/parser/Parser.h"
#include "Scope.h"
#include "ReferencePointer.h"
#include "Utype.h"
#include "Expression.h"

class Compiler {

public:
    Compiler(string outFile,List<parser*> &parsers)
    :
        outFile(outFile),
        failed(0),
        succeeded(0),
        errCount(0),
        rawErrCount(0),
        classSize(0),
        methodSize(0),
        threadLocals(0),
        currModule(""),
        panic(false),
        lastNoteMsg(""),
        lastNote(),
        errors(NULL),
        current(NULL),
        processingStage(0),
        hasMainMethod(false),
        mainMethod(NULL),
        mainSignature(0),
        delegateGUID(0)
    {
        modules.init();
        sourceFiles.init();
        noteMessages.init();
        classes.init();
        enums.init();
        generics.init();
        currScope.init();
        importMap.init();
        stringMap.init();
        failedParsers.init();
        unProcessedClasses.init();
        succeededParsers.init();
        inlinedFields.init();
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


    template<class T>
    static void freePtr(T *ptr)
    {
        ptr->free();
        delete ptr;
    }

    void generate();
    void cleanup();

    static bool simpleParameterMatch(List<Field*> &params, List<Field*> &comparator);
    static bool complexParameterMatch(List<Field*> &params, List<Field*> &comparator);

    List<string> failedParsers;
    List<string> succeededParsers;
    long long failed, succeeded;
    long errCount, rawErrCount;
    static long guid;
private:
    bool panic;
    long classSize;
    long methodSize;
    long delegateGUID;
    long threadLocals;
    long mainSignature;
    bool hasMainMethod;
    long processingStage;
    string outFile;
    string lastNoteMsg;
    __int64 i64;
    Meta lastNote;
    Method* mainMethod;
    List<parser*> parsers;
    List<string> modules;
    List<string> sourceFiles;
    List<string> noteMessages;
    List<ClassObject*> classes;
    List<ClassObject*> enums;
    List<ClassObject*> generics;
    List<ClassObject*> unProcessedClasses; /* We cant compile everything at once so this will hold all classes that cant be immediatley processed at the time */
    List<Scope*> currScope;
    List<string> stringMap;
    List<KeyPair<Field*, double>> inlinedFields;
    List<KeyPair<string, List<string>>>  importMap;
    parser* current;
    ErrorManager *errors;
    string currModule;

    bool preprocess();
    bool postProcess();
    void compile();
    void createGlobalClass();
    void inheritRespectiveClasses();
    void inlineFields();
    void postProcessGenericClasses();
    void postProcessUnprocessedClasses();
    void handleImports();
    void preProcessGenericClasses(long long unstableClasses);
    void preProcessUnprocessedClasses(long long unstableClasses);
    parser *getParserBySourceFile(string name);
    void addDefaultConstructor(ClassObject* klass, Ast* ast);
    void createNewWarning(error_type error, int type, int line, int col, string xcmnts);
    string parseModuleDecl(Ast* ast);
    void parseClassAccessFlags(List<AccessFlag> &flags, Ast *ast);
    void inlineClassFields(Ast* ast, ClassObject* currentClass = NULL);
    void inlineField(Ast* ast);
    void inlineFieldHelper(Ast* ast);
    void inlineEnumFields(Ast* ast, ClassObject* currentClass = NULL);
    void inlineEnumField(Ast* ast);
    bool isWholeNumber(double value);
    void preProccessClassDecl(Ast* ast, bool isInterface, ClassObject* currentClass = NULL);
    void preProccessGenericClassDecl(Ast* ast, bool isInterface);
    void parseIdentifierList(Ast *ast, List<string> &idList);
    StorageLocality strtostl(string locality);
    void preProccessVarDeclHelper(List<AccessFlag>& flags, Ast* ast);
    void preProccessVarDecl(Ast* ast);
    int64_t checkstl(StorageLocality locality);
    void parseVariableAccessFlags(List<AccessFlag> &flags, Ast *ast);
    ClassObject* addChildClassObject(string name, List<AccessFlag> &flags, ClassObject* owner, Ast* ast);
    void removeScope();
    void inheritEnumClass();
    void resolveBaseClasses();
    void resolveSuperClass(Ast *ast, ClassObject* currentClass = NULL);
    void parseReferencePointerList(List<ReferencePointer*> &refPtrs, Ast *ast);
    ClassObject* resolveBaseClass(Ast *ast, ClassObject* currentClass);
    ClassObject *resolveClassReference(Ast *ast, ReferencePointer &ptr);
    void compileReferencePtr(ReferencePointer &ptr, Ast* ast);
    ClassObject* compileGenericClassReference(string &mod, string &name, ClassObject* parent, Ast *ast);
    void compileUtypeList(Ast *ast, List<Utype *> &types);
    Utype* compileUtype(Ast *ast);
    void resolveUtype(ReferencePointer &ptr, Utype* utype, Ast *ast);
    void inlineVariableValue(IrCode &code, Field *field);
    bool isDClassNumberEncodable(double var);
    Field *resolveEnum(string name);
    void resolveClassHeiarchy(DataEntity* data, ReferencePointer& refrence, Utype* utype, Ast* ast);
    int64_t getLowBytes(double var);
    DataType strToNativeType(string &str);
    double getInlinedFieldValue(Field* field);
    void compileTypeIdentifier(ReferencePointer &ptr, Ast *ast);
    void inheritObjectClass();
    void resolveAllFields();
    void resolveAllMethods();
    void resolveAllDelegates();
    void resolveAllDelegates(Ast *ast, ClassObject* currentClass = NULL);
    Method* validateDelegatesHelper(Method *method, List<Method*> &list);
    void validateDelegates(ClassObject *subscriber, Ast *ast);
    void resolveMethod(Ast* ast);
    void resolveDelegate(Ast* ast);
    void resolveDelegateImpl(Ast* ast);
    void resolveConstructor(Ast* ast);
    void resolveOperatorOverload(Ast* ast);
    void checkMainMethodSignature(Method* method);
    Field* compileUtypeArg(Ast* ast);
    Field* compileFuncPrototypeArg(Ast *ast);
    bool containsParamType(List<Field*> &params, DataType type);
    void validateMethodParams(List<Field*>& params, Ast* ast);
    void parseUtypeArgList(List<Field*> &params, Ast* ast);
    bool containsParam(List<Field*> &params, string name);
    void parseMethodAccessFlags(List<AccessFlag> &flags, Ast *ast);
    void resolveClassMethods(Ast* ast, ClassObject* currentClass = NULL);
    void resolveClassFields(Ast* ast, ClassObject* currentClass = NULL);
    bool isFieldInlined(Field* field);
    ClassObject* resolveClass(string mod, string name, Ast* pAst);
    void validateAccess(ClassObject *klass, Ast* pAst);
    void validateAccess(Field *field, Ast* pAst);
    bool resolveClass(List<ClassObject*> &classes, List<ClassObject*> &results, string mod, string name, Ast* pAst, bool match = false);
    void resolveSingularUtype(ReferencePointer &ptr, Utype* utype, Ast *ast);
    void preProccessImportDecl(Ast *branch, List<string> &imports);
    void preproccessImports();
    void preProccessEnumDecl(Ast *ast);
    void preProccessEnumVar(Ast *ast);
    AccessFlag strToAccessFlag(string str);
    void printNote(Meta& meta, string msg);
    ClassObject* addGlobalClassObject(string name, List<AccessFlag>& flags, Ast *ast, List<ClassObject*> &classList);
    ClassObject* findClass(string mod, string className, List<ClassObject*> &classes, bool match = false);
    void inheritEnumClassHelper(Ast *ast, ClassObject *enumClass);
    void resolveField(Ast* ast);
    void resolveFieldType(Field* field, Utype *utype, Ast* ast, bool typeInference = false);
    void resolvePrototypeField(Ast* ast);
    void compileExpression(Expression* expr, Ast* ast);
    void compilePrimaryExpression(Expression* expr, Ast* ast);
    void compileUtypeClass(Expression* expr, Ast* ast);
    void compileDotNotationCall(Expression* expr, Ast* ast);
    expression_type utypeToExpressionType(Utype *utype);
    Method* compileMethodUtype(Expression* expr, Ast* ast);
    void compileExpressionList(List<Expression>& lst, Ast* ast);
    void inheritObjectClassHelper(Ast *ast, ClassObject *klass);
    void compileLiteralExpression(Expression* expr, Ast* ast);
    void parseCharLiteral(Expression* expr, Token &token);
    void parseIntegerLiteral(Expression* expr, Token &token);
    string invalidateUnderscores(string str);
    Method* findFunction(ClassObject *k, string name, List<Field*> &params, Ast* ast, bool checklBase = false);
    Method* compileSingularMethodUtype(ReferencePointer &ptr, List<Field*> &params, Ast* ast);
    bool isAllIntegers(string int_string);
    void parseBoolLiteral(Expression* expr, Token &token);
    void parseHexLiteral(Expression* expr, Token &token);
    void parseStringLiteral(Expression* expr, Token &token);
    bool addFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &));
    Method* findFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &));
    void expressionsToParams(List<Expression>& expressions, List<Field*> &params);
    void compileFieldType(Field *field);
};

enum ProcessingStage {
    PREPROCESSING=0,
    POST_PROCESSING=1,
    COMPILING=2,
};

#define CHECK_CMP_ERRORS(exit_proc) \
    if(panic) exit_proc \
    else if((rawErrCount + errors->getUnfilteredErrorCount()) > _SHARP_CERROR_LIMIT) { \
        panic = true; \
        exit_proc \
    }

#define currentScope() \
    (currScope.last())

#define globalScope() \
    (currentScope()->type == GLOBAL_SCOPE)

#endif //SHARP_COMPILER_H
