//
// Created by bknun on 9/12/2017.
//

#ifndef SHARP_RUNTIME_H
#define SHARP_RUNTIME_H

#include "../../stdimports.h"
#include "List.h"
#include "parser/Parser.h"
#include "ClassObject.h"
#include "../runtime/Opcode.h"

struct Scope;
class ReferencePointer;
class ResolvedReference;
struct Expression;

class ResolvedReference {
public:
    ResolvedReference()
            :
            type(UNDEFINED),
            field(NULL),
            method(NULL),
            klass(NULL),
            oo(NULL),
            referenceName(""),
            array(false),
            resolved(false),
            isMethod(false)
    {
    }

    static string typeToString(FieldType type) {
        if(type==CLASS)
            return "class";
        else if(type==OBJECT)
            return "object";
        else if(type==VAR)
            return "var";
        else if(type==TYPEVOID)
            return "void";
        else if(type==UNDEFINED)
            return "undefined";
        else
            return "unresolved";
    }

    string typeToString() {
        if(isMethod)
            return "method";
        else if(type==CLASS)
            return field != NULL ? field->klass->getFullName() : (klass==NULL ? "?" : klass->getFullName());
        else if(type==OBJECT)
            return "object";
        else if(type==VAR)
            return "var";
        else if(type==TYPEVOID)
            return "void";
        else if(type==UNDEFINED)
            return "undefined";
        else if(type==CLASSFIELD) {
            if(field->type==CLASS)
                return field->klass->getFullName();
            else
                return typeToString(field->type);
        }
        else
            return "unresolved";
    }

    void free() {
        referenceName.clear();
    }

    bool isVar() { return type==VAR; }
    bool dynamicObject() { return type==OBJECT; }

    string referenceName;
    bool array, isMethod, resolved;
    FieldType type;
    ClassObject* klass;
    Field* field;
    Method* method;
    OperatorOverload* oo;

    bool isArray() {
        if(field != NULL)
            return field->isArray;

        return array;
    }
};

enum expression_type {
    expression_var=1,
    expression_class=3,
    expression_objectclass=5,
    expression_expression=7,
    expression_string=8,
    expression_native=9,
    expression_field=10,
    expression_lclass=11,
    expression_void=12,
    expression_unresolved=13,
    expression_null=14,
    expression_generic=15, // special case for generic utypes
    expression_unknown=0x900f
};

struct Expression {
    Expression()
            :
            type(expression_unknown),
            utype(),
            code(),
            dot(false),
            link(NULL),
            newExpression(false),
            ifExpression(false),
            func(false),
            intValue(0),
            value(""),
            literal(false),
            arrayElement(false),
            boolExpressions()
    {
    }

    Expression(Ast* pAst)
            :
            type(expression_unknown),
            utype(),
            code(),
            dot(false),
            link(pAst),
            newExpression(false),
            ifExpression(false),
            func(false),
            intValue(0),
            value(""),
            literal(false),
            arrayElement(false),
            boolExpressions(),
            inCmtRegister(false)
    {
    }

    void utype_refrence_toexpression(ResolvedReference ref);

    expression_type type;
    ResolvedReference utype;
    Assembler code;
    Ast* link;
    bool dot, newExpression, func, literal, arrayElement, inCmtRegister, ifExpression;
    string value;
    double intValue;
    List<long> boolExpressions;

    bool arrayObject() {
        switch(type) {
            case expression_field:
                return utype.field->isArray;
            default:
                return utype.array;
        }
    }
    int trueType() {
        switch(type) {
            case expression_field:
                return utype.field->type;
            case expression_objectclass:
                return OBJECT;
            case expression_lclass:
                return CLASS;
            case expression_var:
                if(func || arrayElement || literal)
                    return VAR;
                else
                    return utype.type;
            default:
                return utype.type;
        }
    }

    bool isArray() {
        if(arrayElement)
            return false;
        else
            return utype.isArray();
    }
    string typeToString();
    void free() {
        utype.free();
        boolExpressions.free();
        code.free();
        *this=(Expression(link));
    }

    void operator=(Expression e);

    void inject(Expression &expression);
};

struct Block {
    Block()
            :
            code(),
            assembly_table()
    {
    }

    Assembler code;
    List<KeyPair<int64_t, int64_t>> assembly_table;

    void free() {
        code.free();
    }
};

class ReferencePointer {
public:
    ReferencePointer() {
        classHeiarchy.init();
        module = "";
        referenceName = "";
    }

    void operator=(ReferencePointer ptr) {
        this->module = ptr.module;
        this->referenceName = ptr.referenceName;

        this->classHeiarchy.addAll(ptr.classHeiarchy);
    }

    void free() {
        classHeiarchy.free();
        referenceName.clear();
        module.clear();
    }

    bool singleRefrence() {
        return module == "" && classHeiarchy.size() == 0;
    }

    bool singleRefrenceModule() {
        return module != "" && classHeiarchy.size() == 0;
    }

    void print() {
        // dev code
        cout << "refrence pointer -----" << endl;
        cout << "id: " << referenceName << endl;
        cout << "mod: " << module << endl;
        cout << "class: ";
        for(int i = 0; i < classHeiarchy.size(); i++)
            cout << classHeiarchy.at(i) << ".";
        cout << endl;
    }

    string module;
    List<string> classHeiarchy;
    string referenceName;

    string toString() {
        stringstream ss;
        if(module != "")
            ss << module << "#";
        for(int i = 0; i < classHeiarchy.size(); i++)
            ss << classHeiarchy.at(i) << ".";
        ss << referenceName;
        return ss.str();
    }
};

struct BranchTable {
    BranchTable()
            :
            branch_pc(0),
            line(0),
            col(0),
            store(false),
            registerWatchdog(0),
            _offset(0),
            labelName("")
    {
    }

    int64_t branch_pc;          // where was the branch initated in the code
    string labelName;           // the label we were trying to access
    int line, col;

    bool store;                 // is this a store instruction/
    int registerWatchdog;      // if this is a store instruction tell me what register to put the data in
    long _offset;                // any offset to the address label

    void free() {
        labelName.clear();
    }
};

enum ScopeType {
    GLOBAL_SCOPE,
    CLASS_SCOPE,
    INSTANCE_BLOCK,
    STATIC_BLOCK
};

struct Scope {
    Scope()
            :
            type(GLOBAL_SCOPE),
            klass(NULL),
            self(false),
            base(false),
            currentFunction(NULL),
            blocks(0),
            loops(0),
            trys(0),
            uniqueLabelSerial(0),
            reachable(true),
            last_statement(0)
    {
        locals.init();
        label_map.init();
        branches.init();
    }

    Scope(ScopeType type, ClassObject* klass)
            :
            type(type),
            klass(klass),
            self(false),
            base(false),
            currentFunction(NULL),
            blocks(0),
            loops(0),
            trys(0),
            uniqueLabelSerial(0),
            reachable(true),
            last_statement(0)
    {
        locals.init();
        label_map.init();
        branches.init();
    }

    Scope(ScopeType type, ClassObject* klass, Method* func)
            :
            type(type),
            klass(klass),
            self(false),
            base(false),
            currentFunction(func),
            blocks(0),
            loops(0),
            trys(0),
            uniqueLabelSerial(0),
            reachable(true),
            last_statement(0)
    {
        locals.init();
        label_map.init();
        branches.init();
    }

    KeyPair<int, Field>* getLocalField(string field_name) {
        if(locals.size() == 0) return NULL;

        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).value.name == field_name) {
                return &locals.get(i);
            }
        }
        return NULL;
    }

    int getLocalFieldIndex(string field_name) {
        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).value.name == field_name) {
                return i;
            }
        }
        return -1;
    }

    int64_t getLabel(std::string name) {
        for(unsigned int i = 0; i < label_map.size(); i++) {
            if(label_map.get(i).key == name)
                return label_map.get(i).value;
        }
        return -1;
    }

    void addBranch(string label, long offset, Assembler& assembler, int line, int col) {
        BranchTable bt;
        assembler.__asm64.add(0);               // add empty instruction for branch later
        bt.branch_pc = assembler.__asm64.size()-1;
        bt.line=line;
        bt.col=col;
        bt.labelName = label;
        bt._offset=offset;
        branches.push_back(bt);
    }

    void addStore(string label, int _register, long offset, Assembler& assembler, int line, int col) {
        BranchTable bt;
        bt.branch_pc=assembler.__asm64.size();
        assembler.__asm64.add(0);               // add empty instruction for storing later
        assembler.__asm64.add(0);
        bt.line=line;
        bt.col=col;
        bt.labelName = label;
        bt.store=true;
        bt._offset = offset;
        bt.registerWatchdog=_register;
        branches.push_back(bt);
    }

    void addStore(string label, int _register, long offset, Assembler& assembler, Expression expr, int line, int col) {
        BranchTable bt;
        bt.branch_pc=assembler.__asm64.size()+expr.code.size();
        assembler.__asm64.add(0);               // add empty instruction for storing later
        assembler.__asm64.add(0);
        bt.line=line;
        bt.col=col;
        bt.labelName = label;
        bt.store=true;
        bt._offset = offset;
        bt.registerWatchdog=_register;
        branches.push_back(bt);
    }

    ScopeType type;
    ClassObject* klass;
    Method* currentFunction;
    List<KeyPair<int, Field>> locals;
    List<KeyPair<std::string, int64_t>> label_map;
    List<BranchTable> branches;
    int blocks;
    long loops, trys, uniqueLabelSerial, last_statement;
    bool self, base, reachable;

    void free() {
        locals.free();
        label_map.free();
    }

    void removeLocals(int block) {
        readjust:
        if(locals.size() == 0) return;

        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).key==block) {
                locals.remove(i);
                goto readjust;
            }
        }
    }

    void removeLocals(string name) {
        if(locals.size() == 0) return;

        readjust:
        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).value.name==name) {
                locals.remove(i);
                return;
            }
        }
    }
};

class RuntimeEngine {
public:
    RuntimeEngine(const string exportFile, List<Parser*> &parsers)
    :
            exportFile(exportFile),
            modules(),
            parsers(),
            failedParsers(),
            succeededParsers(),
            sourceFiles(),
            scopeMap(),
            classes(),
            errorCount(0),
            unfilteredErrorCount(0),
            importMap(),
            noteMessages(),
            resolvedFields(false),
            classSize(0),
            inline_map(),
            methods(0),
            main(NULL),
            stringMap(),
            panic(false),
            allMethods(),
            staticMainInserts(),
            preprocessed(false)
    {
        this->parsers.addAll(parsers);
        uniqueSerialId = 0;
        uniqueDelegateId = 0;

        for(int i = 0; i < parsers.size(); i++)
        {
            Parser *p = parsers.get(i);
            if(!p->parsed) {
                return;
            }
        }

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

    List<string> failedParsers;
    List<string> succeededParsers;
    static unsigned long uniqueSerialId;
    static unsigned long uniqueDelegateId;
    long errorCount, unfilteredErrorCount;
    bool panic;

    List<Scope> scopeMap;
    Parser* activeParser;

    void generate();

    void cleanup();

    static string invalidateUnderscores(string basic_string);

    ClassObject *getClass(string module, string name, List<ClassObject*> &classes);

    static int64_t get_low_bytes(double var);

    static Operator stringToOp(string op);

    List<ClassObject*> classes;
private:
    bool preprocessed;
    List<Parser*> parsers;
    List<string> modules;
    List<string> sourceFiles;
    List<ClassObject*> generics;
    List<KeyPair<string, List<string>>>  importMap;
    List<KeyPair<string, double>>  inline_map;
    List<string> stringMap;
    string exportFile;
    ErrorManager* errors;
    string currentModule;
    bool resolvedFields;
    unsigned long methods;
    unsigned long classSize;
    Method* main;
    List<Method*> allMethods;
    Block *currentBlock;

    /* One off variables */
    RuntimeNote lastNote;
    string lastNoteMsg;
    List<string> noteMessages;
    Assembler staticMainInserts;
    int64_t i64;

    void compile();

    bool preprocess();

    bool module_exists(string name);

    void add_module(string name);

    string parseModuleName(Ast *ast);

    void parseClassDecl(Ast *ast, bool isInterface= false);

    Scope* addScope(Scope scope);

    bool isTokenAccessDecl(token_entity token);

    AccessModifier entityToModifier(token_entity entity);

    list<AccessModifier> parseAccessModifier(Ast *ast);

    bool parseAccessDecl(Ast *ast, List<AccessModifier> &modifiers, int &startpos);

    void parseClassAccessModifiers(List<AccessModifier> &modifiers, Ast *ast);

    ClassObject *addGlobalClassObject(string name, List<AccessModifier> &modifiers, Ast *pAst);

    bool addClass(ClassObject* klass);

    bool classExists(string module, string name, List<ClassObject*> &classes);

    void printNote(RuntimeNote &note, string msg);

    ClassObject *addChildClassObject(string name, List<AccessModifier> &modifiers, Ast *ast, ClassObject *super);

    void removeScope();

    void parseVarDecl(Ast *ast);

    void parseVarAccessModifiers(List<AccessModifier> &modifiers, Ast *ast);

    int parseVarAccessSpecifiers(List<AccessModifier> &modifiers);

    void resolveAllFields();

    void resolveClassDecl(Ast *ast, bool in);

    ReferencePointer parseReferencePtr(Ast *ast, bool getAst=true);

    ResolvedReference resolveReferencePointer(ReferencePointer &ptr, Ast* pAst);

    ClassObject *tryClassResolve(string moduleName, string name, Ast* pAst);

    ClassObject *resolveClassRefrence(Ast *ast, ReferencePointer &ptr);

    bool expectReferenceType(ResolvedReference refrence, FieldType expectedType, bool method, Ast *ast);

    ClassObject *parseBaseClass(Ast *ast, int startpos);

    void resolveVarDecl(Ast *ast,bool);

    Expression parseUtype(Ast *ast);

    ReferencePointer parseTypeIdentifier(Ast *ast);

    FieldType tokenToNativeField(string entity);

    void resolveUtype(ReferencePointer& refrence, Expression& expression, Ast* pAst);

    bool isFieldInlined(Field *field);

    double getInlinedFieldValue(Field *field);

    void inlineVariableValue(Expression &expression, Field *field);

    bool isDClassNumberEncodable(double var);

    void resolveClassHeiarchy(ClassObject *klass, ReferencePointer &refrence, Expression &expression, Ast *pAst,
                              bool requreMovg = true, bool requireStatic = true);

    void resolveFieldHeiarchy(Field *field, ReferencePointer &refrence, Expression &expression, Ast *pAst);

    void resolveMethodDecl(Ast *ast);

    void parseMethodAccessModifiers(List<AccessModifier> &modifiers, Ast *pAst);

    int parseMethodAccessSpecifiers(List<AccessModifier> &modifiers);

    void parseMethodParams(List<Param> &params, KeyPair<List<string>, List<ResolvedReference>> fields, Ast *pAst);

    bool containsParam(List<Param> params, string param_name);

    Field fieldMapToField(string param_name, ResolvedReference &utype, Ast *pAst);

    KeyPair<List<string>, List<ResolvedReference>> parseUtypeArgList(Ast *ast);

    KeyPair<string, ResolvedReference> parseUtypeArg(Ast *ast);

    void parseMethodReturnType(Expression &expression, Method &method);

    void resolveOperatorDecl(Ast *ast);

    void resolveConstructorDecl(Ast *ast);

    void parsConstructorAccessModifiers(List<AccessModifier> &modifiers, Ast *ast);

    void addDefaultConstructor(ClassObject *klass, Ast *ast);

    void resolveSelfUtype(Scope *scope, ReferencePointer &reference, Expression &expression, Ast *ast);

    void resolveBaseUtype(Scope *scope, ReferencePointer &reference, Expression &expression, Ast *ast);

    ResolvedReference getBaseClassOrField(string name, ClassObject *start, Ast *pAst);

    void resolveAllMethods();

    Method *getMainMethod(Parser *p);

    void analyzeImportDecl(Ast *pAst);

    void createNewWarning(error_type error, int line, int col, string xcmnts);

    void analyzeClassDecl(Ast *ast);

    void setHeadClass(ClassObject *klass);

    void analyzeVarDecl(Ast *ast);

    Expression parseValue(Ast *ast);

    Expression parseExpression(Ast *ast);

    Expression parsePrimaryExpression(Ast *ast);

    Expression parseLiteral(Ast *pAst);

    void parseCharLiteral(token_entity token, Expression &expression);

    void parseIntegerLiteral(token_entity token, Expression &expression);

    void parseHexLiteral(token_entity token, Expression &expression);

    void parseStringLiteral(token_entity token, Expression &expression);

    void parseBoolLiteral(token_entity token, Expression &expression);

    Expression psrseUtypeClass(Ast *pAst);

    Expression parseDotNotationCall(Ast *pAst);

    Method *resolveMethodUtype(Ast *utype, Ast *valueLst, Expression &out);

    List<Expression> parseValueList(Ast *pAst);

    bool expressionListToParams(List<Param> &params, List<Expression> &expressions);

    bool splitMethodUtype(string &name, ReferencePointer &ptr);

    string paramsToString(List<Param> &param);

    void pushAuthenticExpressionToStackNoInject(Expression &expression, Expression &out);

    void pushExpressionToPtr(Expression &expression, Expression &out, bool check = false);

    void pushExpressionToStack(Expression &expression, Expression &out);

    expression_type methodReturntypeToExpressionType(Method *fn);

    Expression &parseDotNotationChain(Ast *pAst, Expression &expression, unsigned int startpos);

    Expression parseDotNotationCallContext(Expression &contextExpression, Ast *pAst);

    Method *resolveContextMethodUtype(ClassObject *classContext, Ast *pAst, Ast *pAst2, Expression &out,
                                      Expression &contextExpression);

    void resolveUtypeContext(ClassObject *classContext, ReferencePointer &refrence, Expression &expression, Ast *pAst);

    void pushExpressionToStackNoInject(Expression &expression, Expression &out);

    Expression parseUtypeContext(ClassObject *classContext, Ast *pAst);

    Expression parseArrayExpression(Expression &interm, Ast *pAst);

    bool currentRefrenceAffected(Expression &expr);

    void pushExpressionToRegister(Expression &expr, Expression &out, int reg);

    void pushExpressionToRegisterNoInject(Expression &expr, Expression &out, int reg);

    Expression parseSelfExpression(Ast *pAst);

    Expression parseSelfDotNotationCall(Ast *pAst);

    Method *resolveSelfMethodUtype(Ast *utype, Ast *valueList, Expression &out);

    Expression parseBaseExpression(Ast *pAst);

    Expression parseBaseDotNotationCall(Ast *pAst);

    Method *resolveBaseMethodUtype(Ast *utype, Ast *valueList, Expression &out);

    Expression parseNullExpression(Ast *pAst);

    Expression parseNewExpression(Ast *pAst);

    List<Expression> parseVectorArray(Ast *pAst);

    void checkVectorArray(Expression &utype, List<Expression> &vecArry);

    void parseNewArrayExpression(Expression &out, Expression &utype, Ast *pAst);

    Expression parseSizeOfExpression(Ast *pAst);

    Expression parsePostInc(Ast *pAst);

    Expression parseIntermExpression(Ast *pAst);

    void postIncClass(Expression &out, token_entity op, ClassObject *klass);

    Expression parseArrayExpression(Ast *pAst);

    Expression parseCastExpression(Ast *pAst);

    void parseNativeCast(Expression &utype, Expression &expression, Expression &out);

    void parseClassCast(Expression &utype, Expression &arg, Expression &out);

    Expression parsePreInc(Ast *pAst);

    void preIncClass(Expression &out, token_entity op, ClassObject *klass);

    Expression parseParenExpression(Ast *pAst);

    Expression parseNotExpression(Ast *pAst);

    void notClass(Expression &out, ClassObject *klass, Ast *pAst);

    Expression parseBinaryExpression(Ast *pAst);

    Expression parseAddExpression(Ast *pAst);

    Expression parseUnary(token_entity operand, Expression &right, Ast *pAst);

    void parseAddExpressionChain(Expression &out, Ast *pAst);

    bool addExpressions(Expression &out, Expression &leftExpr, Expression &rightExpr, token_entity operand, double *varout);

    Opcode operandToOp(token_entity operand);

    void addNative(token_entity operand, FieldType type, Expression &out, Expression &left, Expression &right, Ast *pAst);

    bool equals(Expression &left, Expression &right, string msg = "");

    void
    addClass(token_entity operand, ClassObject *klass, Expression &out, Expression &left, Expression &right, Ast *pAst);

    void addClassChain(token_entity operand, ClassObject *klass, Expression &out, Expression &left, Expression &right,
                       Ast *pAst);

    bool constructNewString(Expression &stringExpr, Expression &right, token_entity operand, Expression &out, Ast *pAst);

    void constructNewString(Expression &stringExpr, Expression &out);

    void addStringConstruct(token_entity operand, ClassObject *klass, Expression &out, Expression &left, Expression &right,
                            Ast *pAst);

    bool isMathOp(token_entity entity);

    Expression parseMultExpression(Ast *pAst);

    Expression parseShiftExpression(Ast *pAst);

    bool shiftLiteralExpressions(Expression &out, Expression &leftExpr, Expression &rightExpr, token_entity operand);

    Opcode operandToShftOp(token_entity operand);

    void shiftNative(token_entity operand, Expression &out, Expression &left, Expression &right, Ast *pAst);

    Expression parseLessExpression(Ast *pAst);

    void lessThanLiteralExpressions(Expression &out, Expression &leftExpr, Expression &rightExpr, token_entity operand);

    void lessThanNative(token_entity operand, Expression &out, Expression &left, Expression &right, Ast *pAst);

    Opcode operandToLessOp(token_entity operand);

    Expression parseEqualExpression(Ast *pAst);

    void assignValue(token_entity operand, Expression &out, Expression &left, Expression &right, Ast *pAst);

    bool equalsNoErr(Expression &left, Expression &right);

    void assignNative(token_entity operand, Expression &out, Expression &left, Expression &right, Ast *pAst);

    Opcode operandToCompareOp(token_entity operand);

    Expression parseAndExpression(Ast *pAst);

    void parseAndExpressionChain(Expression &out, Ast *pAst);

    Expression parseAssignExpression(Ast *pAst);

    Expression parseQuesExpression(Ast *pAst);

    Expression fieldToExpression(Ast *pAst, Field &field);

    Expression fieldToExpression(Ast *pAst, string name);

    void initalizeNewClass(ClassObject *klass, Expression &out);

    void parseMethodDecl(Ast *pAst, bool delegate=false);

    void parseBlock(Ast *pAst, Block &block);

    void parseStatement(Block &block, Ast *pAst);

    void addLine(Block &block, Ast *pAst);

    void parseReturnStatement(Block &block, Ast *pAst);

    void parseIfStatement(Block &block, Ast *pAst);

    void parseAssemblyStatement(Block &block, Ast *pAst);

    void parseAssemblyBlock(Block &block, Ast *pAst);

    void parseForStatement(Block &block, Ast *pAst);

    void parseUtypeArg(Ast *pAst, Scope *scope, Block &block, Expression *comparator = NULL);

    bool validateLocalField(string name, Ast *pAst);

    Field utypeArgToField(KeyPair<string, ResolvedReference> arg);

    int64_t get_label(string label);

    void parseForEachStatement(Block &block, Ast *pAst);

    void getArrayValueOfExpression(Expression &expr, Expression &out);

    void assignUtypeForeach(Ast *pAst, Scope *scope, Block &block, Expression &assignExpr);

    void parseWhileStatement(Block &block, Ast *pAst);

    void parseDoWhileStatement(Block &block, Ast *pAst);

    void parseTryCatchStatement(Block &block, Ast *pAst);

    ClassObject *parseCatchClause(Block &block, Ast *pAst, ExceptionTable et);

    void parseFinallyBlock(Block &block, Ast *pAst);

    void parseThrowStatement(Block &block, Ast *pAst);

    void parseContinueStatement(Block &block, Ast *pAst);

    void parseBreakStatement(Block &block, Ast *pAst);

    void parseGotoStatement(Block &block, Ast *pAst);

    void parseLabelDecl(Block &block, Ast *pAst);

    void createLabel(string name, Assembler &code, int line, int col);

    bool label_exists(string label);

    void parseVarDecl(Block &block, Ast *pAst);

    void resolveAllBranches(Block &block);

    void reorderFinallyBlocks(Method *method);

    void parseOperatorDecl(Ast *pAst);

    void parseConstructorDecl(Ast *pAst);

    string generate_header();

    string generate_manifest();

    string generate_data_section();

    string class_to_stream(ClassObject &klass);

    string field_to_stream(Field &field);

    string generate_string_section();

    string generate_text_section();

    string method_to_stream(Method *method);

    string generate_meta_section();

    void createDumpFile();

    string find_method(int64_t id);

    string find_class(int64_t id);

    bool isWholeNumber(double value);

    string getString(long index);

    Opcode assignOperandToOp(token_entity operand);

    bool equalsVectorArray(Expression &left, Expression &right);

    void readjustAddresses(Method *func, unsigned int);

    void inlineFields();

    void resolveDelegatePostDecl(Ast *ast);

    void resolveDelegateDecl(Ast *ast);

    void resolveAllDelegates();

    void resolveClassDeclDelegates(Ast *ast);

    void resolveAllInterfaces();

    void resolveInterfaceDecl(Ast *ast);

    List<ClassObject *> parseRefrenceIdentifierList(Ast *ast);

    void validateDelegates(ClassObject *host, ClassObject *klass, Ast*, bool useBase);

    void verifyClassAccess(ClassObject *klass, Ast* pAst);

    void verifyMethodAccess(Method *fn, Ast* pAst);

    void verifyFieldAccess(Field *field, Ast *pAst);

    void parseLockStatement(Block &block, Ast *pAst);

    void isMemoryObject(Expression &expression, Ast *pAst);

    void parseGenericClassDecl(Ast *ast);

    void parseIdentifierList(Ast *pAst, List<string> &idList);

    ClassObject *addGlobalGenericClassObject(string name, List<AccessModifier> &modifiers, Ast *pAst);

    bool addGeericClass(ClassObject* klass);

    void resolveGenericClassDecl(Ast *ast, bool inlineField);

    void parseUtypeList(Ast *pAst, List<Expression> &list);

    void
    findAndCreateGenericClass(std::string module, string &klass, List<Expression> &utypes, ClassObject* parent, Ast *pAst);

    void traverseGenericClass(ClassObject *klass, List<Expression> &utypes, Ast* pAst);

    void traverseMethod(ClassObject *klass, Method *func, Ast* pAst);

    void traverseField(ClassObject *klass, Field *field, Ast* pAst);

    void analyzeGenericClass(ClassObject *generic);
};


#define currentScope() (scopeMap.empty() ? NULL : &scopeMap.last())

#define init_constructor_postfix "()<init>"

#define for_label_begin_id "$$for_start"

#define for_label_end_id "$$for_end"

#define try_label_end_id "$$try_end"

#define generic_label_id "$$L"

#define __init_label_address(code) (code.__asm64.size() == 0 ? 0 : code.__asm64.size() - 1)

#define unique_label_id(x) "$$L" << (x)

#define progname "bootstrap"
#define progvers "0.2.239"

struct options {
    ~options()
    {
        out.clear();
        vers.clear();
    }

    /*
     * Activate aggressive error reporting for the compiler.
     */
    bool aggressive_errors = false;

    /*
     * Only compile all the files
     */
    bool compile = false;

    /*
     * Only compile support files
     */
    bool compileBootstrap = false;

    /*
     * Output file to write to
     */
    string out = "out";

    /*
     * Application version
     */
    string vers = "1.0";

    /*
     * Disable warnings
     */
    bool warnings = true;

    /*
     * Optimize code
     */
    bool optimize = false;

    /*
     * Set code to be debuggable (flag only used in manifest creation)
     */
    bool debug = true;

    /*
     * Strip debugging info (if-applicable)
     */
    bool strip = false;

    /*
     * Enable warnings as errors
     */
    bool werrors = false;

    /*
     * Allow unsafe code
     */
    bool unsafe = false;

    /*
     * Easter egg to enable magic mode
     */
    bool magic = false;

    /*
     * Easter egg to enable debug mode
     *
     * Allows you to see a little information
     * on what the compiler is doing
     */
    bool debugMode = false;

    /*
     * Dump object code
     */
    bool objDump = false;

    /*
     * Maximum errors the compiler will allow
     */
    unsigned long error_limit = 1000;

    /*
     * Machine platform target to run on
     */
    int target = versions.ALPHA;
};

int _bootstrap(int argc, const char* argv[]);
void rt_error(string message);
void printVersion();
std::string to_lower(string s);
bool all_integers(string int_string);
void exec_runtime(List<string>& files);
extern unsigned long long optimizationResult;

extern options c_options;
#define opt(v) strcmp(argv[i], v) == 0

#endif //SHARP_RUNTIME_H
