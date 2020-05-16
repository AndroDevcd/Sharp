//
// Created by BraxtonN on 10/18/2019.
//

#ifndef SHARP_COMPILER_H
#define SHARP_COMPILER_H

#include "../../../stdimports.h"
#include "../frontend/parser/Parser.h"
#include "../../runtime/Opcode.h"
#include "Scope.h"
#include "ReferencePointer.h"
#include "data/Utype.h"
#include "Expression.h"
#include "oo/FunctionType.h"
#include "data/Alias.h"
#include "ofuscation/Obfuscater.h"

class Compiler {

public:
    Compiler(string outFile,List<parser*> &parsers)
    :
        outFile(outFile),
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
        mainMethod(NULL),
        requiredSignature(NULL),
        mainSignature(0),
        delegateGUID(0),
        typeInference(false),
        obfuscateMode(false),
        enumValue(0)
    {
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
    static bool isUtypeClassConvertableToVar(Utype *dest, Utype *src);
    static bool isUtypeClass(Utype* utype, string mod, int names, ...);

    ErrorManager *errors;
    List<parser*> failedParsers;
    List<ClassObject*> classes;
    static uInt guid;
private:
    bool panic;
    long classSize;
    long methodSize;
    long delegateGUID;
    long threadLocals;
    long mainSignature;
    bool typeInference;
    bool obfuscateMode;
    long processingStage;
    long enumValue;
    string outFile;
    string lastNoteMsg;
    Meta lastNote;
    Method* mainMethod;
    List<parser*> parsers;
    List<string> noteMessages;
    List<string> warnings;
    List<ClassObject*> enums;
    List<ClassObject*> generics;
    List<ClassObject*> unProcessedClasses; /* We cant compile everything at once so this will hold all classes that cant be immediatley processed at the time */
    List<Method*> unProcessedMethods;
    List<Scope*> currScope;
    List<string> stringMap;
    List<double> constantMap;
    List<KeyPair<Field*, double>> inlinedFields;
    List<KeyPair<Field*, Int>> inlinedStringFields;
    List<KeyPair<string, List<PackageData*>>>  importMap;
    List<Method*> lambdas;
    List<Method*> functionPtrs;
    Utype* nilUtype;
    Utype* nullUtype;
    Utype* undefUtype;
    Utype* varUtype;
    Utype* objectUtype;
    parser* current;
    string currModule;
    Method* requiredSignature;
    CodeHolder staticMainInserts;
    CodeHolder tlsMainInserts;

    void setup();
    bool preprocess();
    bool postProcess();
    void preprocessMutations();
    void compile();
    void createGlobalClass();
    void compileAllFields();
    void compileAllObfuscationRules();
    void compileAllInitDecls();
    void compileAllMethods();
    void inlineFields();
    void postProcessGenericClasses();
    void compileAllUnprocessedMethods();
    void compileUnprocessedClasses();
    void validateCoreClasses(Ast *ast);
    void setupMainMethod(Ast *ast);
    void assignEnumFieldName(Field*);
    void assignEnumFieldValue(Field*);
    void compileEnumField(Field*);
    void compileEnumFields();
    void assignEnumArray(ClassObject *enumClass);
    void postProcessUnprocessedClasses();
    void handleImports();
    void initStaticClassInstance(CodeHolder &code, ClassObject *klass);
    void compileMethodDecl(Ast *ast, ClassObject* currentClass = NULL);
    void updateErrorManagerInstance(parser *parser);
    void preProcessGenericClasses(long long unstableClasses);
    void preProcessUnprocessedClasses(long long unstableClasses);
    parser *getParserBySourceFile(string name);
    void addDefaultConstructor(ClassObject* klass, Ast* ast);
    void inlineClassMutateFields(Ast *ast);
    void createNewWarning(error_type error, int type, int line, int col, string xcmnts);
    void createNewWarning(error_type error, int type, Ast* ast, string xcmnts);
    string parseModuleDecl(Ast* ast);
    void parseClassAccessFlags(List<AccessFlag> &flags, Ast *ast);
    bool matchesFlag(AccessFlag flags[], int len, int startPos, AccessFlag flag);
    void inlineClassFields(Ast* ast, ClassObject* currentClass = NULL);
    void inlineField(Ast* ast);
    void inlineFieldHelper(Ast* ast);
    void inlineEnumFields(Ast* ast);
    void inlineEnumField(Ast* ast);
    void compileInitDecl(Ast *ast);
    Method* getStaticInitFunction();
    void reconcileBranches(bool finalTry);
    void invalidateLocalAliases();
    void invalidateLocalVariables();
    Int getSkippedBlockCount(ast_type triggerStatement);
    bool compileBlock(Ast *ast);
    void compileLocalVariableDecl(Ast *ast);
    void compileForStatement(Ast *ast);
    void compileForEachStatement(Ast *ast);
    void compileWhileStatement(Ast *ast);
    void compileDoWhileStatement(Ast *ast);
    void compileThrowStatement(Ast *ast);
    void compileGotoStatement(Ast *ast);
    void compileBreakStatement(Ast *ast);
    void compileContinueStatement(Ast *ast);
    void compileLockStatement(Ast *ast);
    void compileAsmStatement(Ast *ast);
    _register compileAsmRegister(Ast *ast);
    opcode_arg compileAsmLiteral(Ast *ast);
    void compileTryCatchStatement(Ast *ast, bool *controlPaths);
    void compileWhenStatement(Ast *ast, bool *controlPaths);
    ClassObject* compileCatchClause(Ast *ast, TryCatchData &tryCatchData, bool *controlPaths);
    void compileReturnStatement(Ast *ast, bool *controlPaths);
    void compileIfStatement(Ast *ast, bool *controlPaths);
    void compileLabelDecl(Ast *ast, bool *controlPaths);
    void compileStatement(Ast *ast, bool *controlPaths);
    bool allControlPathsReturnAValue(bool *controlPaths);
    void deInitializeLocalVariables(string &name);
    bool insideFinallyBlock();
    void addLocalVariables();
    void preProccessClassDeclForMutation(Ast* ast);
    void preProcessMutation(Ast *ast, ClassObject *currentClass = NULL);
    bool isWholeNumber(double value);
    void preProccessClassDecl(Ast* ast, bool isInterface, ClassObject* currentClass = NULL);
    void compileClassFields(Ast* ast, ClassObject* currentClass = NULL);
    void compileClassObfuscations(Ast* ast, ClassObject* currentClass = NULL);
    void compileClassMethods(Ast* ast, ClassObject* currentClass = NULL);
    void compileClassInitDecls(Ast* ast, ClassObject* currentClass = NULL);
    void preProccessGenericClassDecl(Ast* ast, bool isInterface);
    void compileObfuscateDecl(Ast* ast);
    void parseIdentifierList(Ast *ast, List<string> &idList);
    StorageLocality strtostl(string locality);
    void preProccessVarDeclHelper(List<AccessFlag>& flags, Ast* ast);
    void preProccessVarDecl(Ast* ast);
    Alias* preProccessAliasDecl(Ast* ast);
    int64_t checkstl(StorageLocality locality);
    void resolveAlias(Ast* ast);
    void parseVariableAccessFlags(List<AccessFlag> &flags, Ast *ast);
    ClassObject* addChildClassObject(string name, List<AccessFlag> &flags, ClassObject* owner, Ast* ast);
    void removeScope();
    void resolveBaseClasses();
    CodeHolder& dataTypeToOpcode(DataType type, _register outRegister, _register castRegister, CodeHolder &code);
    void convertUtypeToNativeClass(Utype *clazz, Utype *paramUtype, CodeHolder &code, Ast* ast);
    void convertNativeIntegerClassToVar(Utype *clazz, Utype *paramUtype, CodeHolder &code, Ast *ast);
    void resolveSuperClass(Ast *ast, ClassObject* currentClass = NULL);
    void parseReferencePointerList(List<ReferencePointer*> &refPtrs, Ast *ast);
    ClassObject* resolveBaseClass(Ast *ast);
    ClassObject *resolveClassReference(Ast *ast, ReferencePointer &ptr, bool allowenerics = false);
    void compileReferencePtr(ReferencePointer &ptr, Ast* ast);
    ClassObject* compileGenericClassReference(string &mod, string &name, ClassObject* parent, Ast *ast);
    void compileUtypeList(Ast *ast, List<Utype *> &types);
    Utype* compileUtype(Ast *ast, bool intanceCaptured = false);
    void compileFuncPtr(Utype *utype, Ast *ast);
    void resolveUtype(ReferencePointer &ptr, Utype* utype, Ast *ast);
    void inlineVariableValue(CodeHolder &code, Field *field);
    bool isDClassNumberEncodable(double var);
    Field *resolveEnum(string name);
    void compileAliasType(Alias *alias);
    void resolveClassHeiarchy(DataEntity* data, bool fromClass, ReferencePointer& refrence, Utype* utype, Ast* ast);
    int64_t getLowBytes(double var);
    DataType strToNativeType(string &str);
    double getInlinedFieldValue(Field* field);
    Int getInlinedStringFieldAddress(Field* field);
    void compileTypeIdentifier(ReferencePointer &ptr, Ast *ast);
    void resolveAllFields();
    void resolveAllMethods();
    void resolveAllDelegates();
    void resolveClassMutateFields(Ast *ast);
    void resolveDelegateMutateMethods(Ast *ast);
    void compileFnPtrCast(Utype *utype, Expression *castExpr, Expression *outExpr);
    void resolveAllDelegates(Ast *ast, ClassObject* currentClass = NULL);
    Method* validateDelegatesHelper(Method *method, List<Method*> &list);
    void validateDelegates(ClassObject *subscriber, Ast *ast);
    string accessFlagsToString(List<AccessFlag> &flags);
    ClassObject* getExtensionFunctionClass(Ast* ast);
    void resolveClassMutateMethods(Ast *ast);
    void compileClassMutateFields(Ast *ast);
    void compileClassMutateObfuscations(Ast *ast);
    void compileClassMutateMethods(Ast *ast);
    void compileClassMutateInitDecls(Ast *ast);
    void resolveMethod(Ast* ast, ClassObject *currentClass = NULL);
    void compileClassMethod(Ast* ast);
    void resolveGlobalMethod(Ast* ast);
    void compileGlobalMethod(Ast* ast);
    void resolveClassMethod(Ast* ast);
    void compileMethodReturnType(Method* fun, Ast *ast, bool wait = false);
    void resolveConstructor(Ast* ast);
    void compileConstructor(Ast* ast);
    void resolveOperatorOverload(Ast* ast);
    void compileOperatorOverload(Ast* ast);
    void checkMainMethodSignature(Method* method);
    Field* compileUtypeArg(Ast* ast);
    void compileParenExpression(Expression* expr, Ast* ast);
    void compileArrayExpression(Expression* expr, Ast* ast);
    void compilePreIncExpression(Expression* expr, Ast* ast);
    void compileLambdaExpression(Expression* expr, Ast* ast);
    void compileNotExpression(Expression* expr, Ast* ast);
    void compileMinusExpression(Expression* expr, Ast* ast);
    Method* findLambdaByAst(Ast *ast);
    void compileLambdaArgList(List<Field*> &fields, Ast* ast);
    Field* compileLambdaArg(Ast *ast);
    void compilePostIncExpression(Expression* expr, bool compileExpression, Ast* ast);
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
    void preProccessImportDecl(Ast *branch, List<PackageData*> &imports);
    bool resolveHigherScopeSingularUtype(ReferencePointer &ptr, Utype* utype, Ast *ast);
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
    void compileVariableDecl(Ast* ast);
    void resolveFieldType(Field* field, Utype *utype, Ast* ast);
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
    void compileAssignExpression(Expression* expr, Ast* ast);
    void compileBinaryExpression(Expression* expr, Ast* ast);
    void compileInlineIfExpression(Expression* expr, Ast* ast);
    void assignValue(Expression* expr, Token &operand, Expression &leftExpr, Expression &rightExpr, Ast* ast, bool allowOverloading = true);
    void compileBinaryExpression(Expression* expr, Token &operand, Expression &leftExpr, Expression &rightExpr, Ast* ast);
    void compoundAssignValue(Expression* expr, Token &operand, Expression &leftExpr, Expression &rightExpr, Ast* ast);
    expression_type utypeToExpressionType(Utype *utype);
    Method* compileMethodUtype(Expression* expr, Ast* ast);
    void fullyQualifyLambda(Utype *lambdaQualifier, Utype *lambda);
    void compileExpressionList(List<Expression*>& lst, Ast* ast);
    void inheritObjectClassHelper(Ast *ast, ClassObject *klass);
    void compileLiteralExpression(Expression* expr, Ast* ast);
    void parseCharLiteral(Expression* expr, Token &token);
    void parseIntegerLiteral(Expression* expr, Token &token);
    string invalidateUnderscores(string str);
    Method* findFunction(ClassObject *k, string name, List<Field*> &params, Ast* ast, bool checklBase = false, function_type type = fn_undefined, bool advancedSearch = true);
    bool paramsContainNonQualifiedLambda(List<Field*> &params);
    bool isLambdaUtype(Utype *type);
    bool resolveFunctionByName(string name, List<Method*> &functions, Ast *ast);
    Method* compileSingularMethodUtype(ReferencePointer &ptr, Expression *expr, List<Field*> &params, Ast* ast);
    Method* findGetterSetter(ClassObject *klass, string name, List<Field*> &params, Ast* ast);
    Method *resolveFunction(string name, List<Field*> &params, Ast *ast);
    Field *resolveField(string name, Ast *ast);
    Alias *resolveAlias(string mod, string name, Ast *ast);
    bool isAllIntegers(string int_string);
    string codeToString(CodeHolder &code, CodeData *data = NULL);
    string registerToString(int64_t r);
    string find_class(int64_t id);
    void printExpressionCode(Expression &expr);
    void printMethodCode(Method &func, Ast *ast);
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
    void assignFieldInitExpressionValue(Field *field, Expression *assignExpr, CodeHolder *resultCode, Ast *ast);
    void resolveUnprocessedClassMutations(ClassObject *unprocessedClass);
    void resolveGetter(Ast *ast, Field *field);
    void resolveSetter(Ast *ast, Field *field);
    void compileFieldGetterCode(CodeHolder &code, Field *field);
    void compilePostAstExpressions(Expression *expr, Ast *ast, long startPos = 1);
    void getContractedMethods(ClassObject *subscriber, List<Method *> &contractedMethods);
    void findConflicts(Ast *ast, string type, string &name);
    void resolveFieldUtype(Utype *utype, Ast *ast, DataEntity *resolvedField, string &name);
    void resolveClassUtype(Utype *utype, Ast *ast, DataEntity *resolvedClass);
    void resolveFunctionByNameUtype(Utype *utype, Ast *ast, string &name, List<Method *> &functions);
    void resolveAliasUtype(Utype *utype, Ast *ast, DataEntity *resolvedAlias);
    ClassObject *getBaseClassUtype(Ast *ast);
    void compileExpressionAst(Expression *expr, Ast *branch);
    void expressionToParam(Expression &expression, Field *param);
    void pushParametersToStackAndCall(Ast *ast, Method *resolvedMethod, List<Field *> &params, CodeHolder &code);
    Field* createLocalField(string name, Utype *type, bool isArray, StorageLocality locality, List<AccessFlag> flags,
                          Int scopeLevel, Ast *ast);
    string accessFlagToString(AccessFlag flag);
    void inlineField(Field *field, Expression &expr);
    void assignFieldExpressionValue(Field *field, Ast *ast);
    void compileLocalAlias(Ast *ast);
    void initializeLocalVariable(Field *field);
    opcode_arg getAsmOffset(Ast *ast);
    void compileAssemblyInstruction(CodeHolder &code, Ast *branch, string &opcode);
    void compileMethod(Ast *ast, Method *func);
    void addLocalFields(Method *func);
    void processScopeExitLockAndFinallys(string &label);
};

enum ProcessingStage {
    PREPROCESSING=0,
    POST_PROCESSING=1,
    COMPILING=2,
};

extern string globalClass;
extern string undefinedModule;

#define CLASS_LIMIT CA2_MAX
#define CLASS_FIELD_LIMIT DA_MAX
#define LOCAL_FIELD_LIMIT CA2_MAX
#define THREAD_LOCAL_FIELD_LIMIT DA_MAX
#define FUNCTION_LIMIT DA_MAX
#define STRING_LITERAL_LIMIT DA_MAX
#define CONSTANT_LIMIT CA2_MAX
#define FUNCTION_OPCODE_LIMIT DA_MAX
#define OPCODE_SKIP_LIMIT DA_MAX

#define CHECK_CMP_ERRORS(exit_proc) \
    if(panic) exit_proc \
    else if(errors->getUnfilteredErrorCount() > _SHARP_CERROR_LIMIT) { \
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

#define RETAIN_REQUIRED_SIGNATURE(newSig) \
    Method *oldRequiredSig = requiredSignature; \
    requiredSignature = newSig;

#define RESTORE_REQUIRED_SIGNATURE() \
    requiredSignature = oldRequiredSig;

#define RETAIN_LOOP_LABELS(startLabel, endLabel) \
    string prevLoopStartLabel = currentScope()->loopStartLabel; \
    string prevLoopEndLabel = currentScope()->loopEndLabel; \
    currentScope()->loopStartLabel = startLabel; \
    currentScope()->loopEndLabel = endLabel;

#define RESTORE_LOOP_LABELS() \
    currentScope()->loopStartLabel = prevLoopStartLabel; \
    currentScope()->loopEndLabel = prevLoopEndLabel;

#define NEW_ERRORS_FOUND() \
    ((errors->getUnfilteredErrorCount() - totalErrors) > 0)

#define RETAIN_SCOPE_CLASS(bt) \
    ClassObject *oldScopeClass = currentScope()->klass; \
    currentScope()->klass = bt;

#define RESTORE_SCOPE_CLASS() \
    currentScope()->klass = oldScopeClass;

#define RETAIN_SCOPE_INIT_CHECK(initCheck) \
    bool oldInitCheckState = currentScope()->initializationCheck; \
    currentScope()->initializationCheck = initCheck;

#define RESTORE_SCOPE_INIT_CHECK() \
    currentScope()->initializationCheck = oldInitCheckState;

#define RETAIN_SCOPED_LABELS(enable) \
    bool oldScopedLabelsState = currentScope()->scopedLabels; \
    currentScope()->scopedLabels = enable;

#define RESTORE_SCOPED_LABELS() \
    currentScope()->scopedLabels = oldScopedLabelsState;

#define RETAIN_TYPE_INFERENCE(ti) \
    bool oldTypeInference = typeInference; \
    typeInference = ti;

#define RESTORE_TYPE_INFERENCE() \
    typeInference = oldTypeInference;

#define CONTROL_PATH_SIZE 8
#define MAIN_CONTROL_PATH 0
#define IF_CONTROL_PATH 1
#define ELSEIF_CONTROL_PATH 2
#define ELSE_CONTROL_PATH 3
#define TRY_CONTROL_PATH 4
#define CATCH_CONTROL_PATH 5
#define WHEN_CONTROL_PATH 6
#define WHEN_ELSE_CONTROL_PATH 7

#define INTERNAL_VARIABLE_NAME_PREFIX string("$01internal_var_")
#define INTERNAL_LABEL_NAME_PREFIX string("$02internal_label_")
#define INTERNAL_STATIC_INIT_FUNCTION string("$03internal_static_init")

#endif //SHARP_COMPILER_H
