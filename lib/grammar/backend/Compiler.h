//
// Created by BraxtonN on 10/18/2019.
//

#ifndef SHARP_COMPILER_H
#define SHARP_COMPILER_H

#include "../../../stdimports.h"
#include "../frontend/parser/Parser.h"
#include "Scope.h"
#include "ReferencePointer.h"
#include "data/Utype.h"
#include "Expression.h"
#include "oo/FunctionType.h"
#include "data/Alias.h"

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
        delegateGUID(0),
        typeInference(false)
    {
        modules.init();
        sourceFiles.init();
        noteMessages.init();
        classes.init();
        enums.init();
        generics.init();
        warnings.init();
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
    static bool isUtypeConvertableToNativeClass(Utype *dest, Utype *src);
    static bool isUtypeClass(Utype* utype, string mod, int names, ...);

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
    bool typeInference;
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
    List<string> warnings;
    List<ClassObject*> classes;
    List<ClassObject*> enums;
    List<ClassObject*> generics;
    List<ClassObject*> unProcessedClasses; /* We cant compile everything at once so this will hold all classes that cant be immediatley processed at the time */
    List<Scope*> currScope;
    List<string> stringMap;
    List<KeyPair<Field*, double>> inlinedFields;
    List<KeyPair<string, List<string>>>  importMap;
    List<Method*> lambdas;
    parser* current;
    ErrorManager *errors;
    string currModule;

    bool preprocess();
    bool postProcess();
    void preprocessMutations();
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
    void inlineClassMutateFields(Ast *ast);
    void createNewWarning(error_type error, int type, int line, int col, string xcmnts);
    string parseModuleDecl(Ast* ast);
    void parseClassAccessFlags(List<AccessFlag> &flags, Ast *ast);
    bool matchesFlag(AccessFlag flags[], int len, int startPos, AccessFlag flag);
    void inlineClassFields(Ast* ast, ClassObject* currentClass = NULL);
    void inlineField(Ast* ast);
    void inlineFieldHelper(Ast* ast);
    void inlineEnumFields(Ast* ast, ClassObject* currentClass = NULL);
    void inlineEnumField(Ast* ast);
    void preProccessClassDeclForMutation(Ast* ast);
    void preProcessMutation(Ast *ast, ClassObject *currentClass = NULL);
    bool isWholeNumber(double value);
    void preProccessClassDecl(Ast* ast, bool isInterface, ClassObject* currentClass = NULL);
    void preProccessGenericClassDecl(Ast* ast, bool isInterface);
    void parseIdentifierList(Ast *ast, List<string> &idList);
    StorageLocality strtostl(string locality);
    void preProccessVarDeclHelper(List<AccessFlag>& flags, Ast* ast);
    void preProccessVarDecl(Ast* ast);
    void preProccessAliasDecl(Ast* ast);
    int64_t checkstl(StorageLocality locality);
    void resolveAlias(Ast* ast);
    void parseVariableAccessFlags(List<AccessFlag> &flags, Ast *ast);
    ClassObject* addChildClassObject(string name, List<AccessFlag> &flags, ClassObject* owner, Ast* ast);
    void removeScope();
    void inheritEnumClass();
    void resolveBaseClasses();
    __int64 dataTypeToOpcode(DataType type);
    void convertUtypeToNativeClass(Utype *clazz, Utype *paramUtype, IrCode &code, Ast* ast);
    void convertNativeClassToUtype(Utype *clazz, Utype *paramUtype, IrCode &code, Ast *ast);
    void resolveSuperClass(Ast *ast, ClassObject* currentClass = NULL);
    void parseReferencePointerList(List<ReferencePointer*> &refPtrs, Ast *ast);
    ClassObject* resolveBaseClass(Ast *ast, ClassObject* currentClass);
    ClassObject *resolveClassReference(Ast *ast, ReferencePointer &ptr, bool allowGenerics = false);
    void compileReferencePtr(ReferencePointer &ptr, Ast* ast);
    ClassObject* compileGenericClassReference(string &mod, string &name, ClassObject* parent, Ast *ast);
    void compileUtypeList(Ast *ast, List<Utype *> &types);
    Utype* compileUtype(Ast *ast, bool intanceCaptured = false);
    void resolveUtype(ReferencePointer &ptr, Utype* utype, Ast *ast);
    void inlineVariableValue(IrCode &code, Field *field);
    bool isDClassNumberEncodable(double var);
    Field *resolveEnum(string name);
    void compileAliasType(Alias *alias);
    void resolveClassHeiarchy(DataEntity* data, bool fromClass, ReferencePointer& refrence, Utype* utype, Ast* ast);
    int64_t getLowBytes(double var);
    DataType strToNativeType(string &str);
    double getInlinedFieldValue(Field* field);
    void compileTypeIdentifier(ReferencePointer &ptr, Ast *ast);
    void inheritObjectClass();
    void resolveAllFields();
    void resolveAllMethods();
    void resolveAllDelegates();
    void resolveClassMutateFields(Ast *ast);
    void resolveDelegateMutateMethods(Ast *ast);
    void resolveAllDelegates(Ast *ast, ClassObject* currentClass = NULL);
    Method* validateDelegatesHelper(Method *method, List<Method*> &list);
    void validateDelegates(ClassObject *subscriber, Ast *ast);
    ClassObject* getExtensionFunctionClass(Ast* ast);
    void resolveClassMutateMethods(Ast *ast);
    void resolveMethod(Ast* ast, ClassObject *currentClass = NULL);
    void resolveGlobalMethod(Ast* ast);
    void compileMethodReturnType(Method* fun, Ast *ast, bool wait = false);
    void resolveDelegate(Ast* ast);
    void resolveDelegateImpl(Ast* ast);
    void resolveConstructor(Ast* ast);
    void resolveOperatorOverload(Ast* ast);
    void checkMainMethodSignature(Method* method);
    Field* compileUtypeArg(Ast* ast);
    void compileParenExpression(Expression* expr, Ast* ast);
    void compileArrayExpression(Expression* expr, Ast* ast);
    void compilePreIncExpression(Expression* expr, Ast* ast);
    void compileLambdaExpression(Expression* expr, Ast* ast);
    void compileNotExpression(Expression* expr, Ast* ast);
    Method* findLambdaByAst(Ast *ast);
    void compileLambdaArgList(List<Field*> &fields, Ast* ast);
    Field* compileLambdaArg(Ast *ast);
    void compilePostIncExpression(Expression* expr, bool compileExpression, Ast* ast);
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
    void validateAccess(Method *function, Ast* pAst);
    void validateAccess(Alias *alias, Ast* pAst);
    void checkTypeInference(Alias *alias, Ast* ast);
    bool resolveClass(List<ClassObject*> &classes, List<ClassObject*> &results, string mod, string name, Ast* pAst, bool match = false);
    void resolveSingularUtype(ReferencePointer &ptr, Utype* utype, Ast *ast);
    void preProccessImportDecl(Ast *branch, List<string> &imports);
    void preproccessImports();
    void preProccessEnumDecl(Ast *ast);
    void preProccessEnumVar(Ast *ast);
    AccessFlag strToAccessFlag(string str);
    void printNote(Meta& meta, string msg);
    bool isLambdaFullyQualified(Method *lambda);
    ClassObject* addGlobalClassObject(string name, List<AccessFlag>& flags, Ast *ast, List<ClassObject*> &classList);
    ClassObject* findClass(string mod, string className, List<ClassObject*> &classes, bool match = false);
    void inheritEnumClassHelper(Ast *ast, ClassObject *enumClass);
    void resolveField(Ast* ast);
    void resolveFieldType(Field* field, Utype *utype, Ast* ast);
    void resolvePrototypeField(Ast* ast);
    void compileExpression(Expression* expr, Ast* ast);
    void compilePrimaryExpression(Expression* expr, Ast* ast);
    void compileCastExpression(Expression *expr, bool compileExpr, Ast *ast);
    void compileSizeOfExpression(Expression* expr, Ast* ast);
    void compileClassCast(Utype *utype, Expression *castExpr, Expression *outExpr);
    void compileUtypeClass(Expression* expr, Ast* ast);
    void compileNativeCast(Utype *utype, Expression *castExpr, Expression *outExpr);
    void compileDotNotationCall(Expression* expr, Ast* ast);
    void compileSelfExpression(Expression* expr, Ast* ast);
    void compileBaseExpression(Expression* expr, Ast* ast);
    void compileNullExpression(Expression* expr, Ast* ast);
    void compileNewExpression(Expression* expr, Ast* ast);
    void compileFieldInitialization(Expression* expr, List<KeyPair<Field*, bool>> &fields, Ast* ast);
    void compileNewArrayExpression(Expression *expr, Ast *ast, Utype *arrayType);
    void compileVectorExpression(Expression* expr, Ast* ast, Utype *compareType = NULL);
    expression_type utypeToExpressionType(Utype *utype);
    Method* compileMethodUtype(Expression* expr, Ast* ast);
    void fullyQualifyLambda(Utype *lambdaQualifier, Utype *lambda);
    void compileExpressionList(List<Expression*>& lst, Ast* ast);
    void inheritObjectClassHelper(Ast *ast, ClassObject *klass);
    void compileLiteralExpression(Expression* expr, Ast* ast);
    void parseCharLiteral(Expression* expr, Token &token);
    void parseIntegerLiteral(Expression* expr, Token &token);
    string invalidateUnderscores(string str);
    Method* findFunction(ClassObject *k, string name, List<Field*> &params, Ast* ast, bool checklBase = false, function_type type = fn_undefined);
    bool paramsContainNonQualifiedLambda(List<Field*> &params);
    bool isLambdaUtype(Utype *type);
    Method* compileSingularMethodUtype(ReferencePointer &ptr, Expression *expr, List<Field*> &params, Ast* ast);
    Method* findGetterSetter(ClassObject *klass, string name, List<Field*> &params, Ast* ast);
    bool isAllIntegers(string int_string);
    string codeToString(IrCode &code);
    string registerToString(int64_t r);
    string find_class(int64_t id);
    void printExpressionCode(Expression *expr);
    void parseBoolLiteral(Expression* expr, Token &token);
    void parseHexLiteral(Expression* expr, Token &token);
    void parseStringLiteral(Expression* expr, Token &token);
    bool addFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &));
    Method* findFunction(ClassObject *k, Method *method, bool (*paramaterMatchFun)(List<Field *> &, List<Field *> &));
    void resolveGenericFieldMutations(ClassObject *unprocessedClass);
    void expressionsToParams(List<Expression*>& expressions, List<Field*> &params);
    bool resolveExtensionFunctions(ClassObject *unprocessedClass);
    void compileFieldType(Field *field);
    void checkTypeInference(Ast *ast);
    void assignFieldInitExpressionValue(Field *field, Expression *assignExpr, IrCode *resultCode, Ast *ast);
    void resolveUnprocessedClassMutations(ClassObject *unprocessedClass);
    void resolveGetter(Ast *ast, Field *field);
    void resolveSetter(Ast *ast, Field *field);
    void compileFieldGetterCode(IrCode &code, Field *field);
    void compilePostAstExpressions(Expression *expr, Ast *ast, long startPos = 1);
    void getContractedMethods(ClassObject *subscriber, List<Method *> &contractedMethods);
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

#define RETAIN_BLOCK_TYPE(bt) \
    BlockType oldType = currentScope()->type; \
    currentScope()->type = bt;

#define RESTORE_BLOCK_TYPE() \
    currentScope()->type = oldType;

#define RETAIN_TYPE_INFERENCE(ti) \
    bool oldTypeInference = typeInference; \
    typeInference = ti;

#define RESTORE_TYPE_INFERENCE() \
    typeInference = oldTypeInference;

#endif //SHARP_COMPILER_H
