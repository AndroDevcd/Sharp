//
// Created by BraxtonN on 10/11/2019.
//

#ifndef SHARP_PARSER_H
#define SHARP_PARSER_H

#include "../../../../stdimports.h"
#include "../tokenizer/tokenizer.h"
#include "Ast.h"
#include "../../json/json_value.h"

class parser {
public:
    parser(tokenizer *tokenizer)
            :
            errors(NULL),
            toks(tokenizer),
            parsed(false),
            panic(false),
            recursion(0),
            lines()
    {
        if(tokenizer != NULL && tokenizer->getErrors() != NULL &&
           !tokenizer->getErrors()->hasErrors())
        {
            lines.addAll(tokenizer->getLines());
            access_types.init();
            tree.init();
            parse();
        }
    }

    ~parser() {
        free();
    }

    ErrorManager* getErrors();
    Ast* astAt(long p);
    size_t size() { return tree.size(); }
    void free();
    string getData() { return toks ? toks->getData() : ""; }
    List<string> &getLines() { return lines; }
    tokenizer *getTokenizer() { return toks; }
    static bool isStorageType(Token &t);
    static bool isNativeType(string t);
    static bool isOverrideOperator(string t);
    static bool isAssignExprSymbol(string t);
    static bool isElvisOperator(string t);
    json_value* exportData();
    void importData(json_value*);
    void exportLines(json_object*);

    bool parsed, panic;
    long recursion;

private:
    void parse();
    Token* peek(int forward);

    bool isAccessDecl(Token &t);
    bool isModuleDecl(Token &t);
    bool isImportDecl(Token &t);
    bool isClassDecl(Token &t);
    bool isMutateDecl(Token &t);
    bool isInterfaceDecl(Token &t);
    bool isObfuscateDecl(Token &t);
    bool isVariableDecl(Token &t);
    bool isMethodDecl(Token &t);
    bool isInitDecl(Token& token);
    bool isEnumDecl(Token &t);
    bool isStaticDecl(Token &t);
    bool isReturnStatement(Token &t);
    bool isIfStatement(Token &t);
    bool isSwitchStatement(Token &t);
    bool isForStatement(Token &t);
    bool isForEachStatement(Token &t);
    bool isWhileStatement(Token &t);
    bool isDoWhileStatement(Token &t);
    bool isThrowStatement(Token &t);
    bool isGotoStatement(Token &t);
    bool isWhenStatement(Token &t);
    bool isBreakStatement(Token &t);
    bool isAsmStatement(Token &t);
    bool isContinueStatement(Token &t);
    bool isLockStatement(Token &t);
    bool isTryCatchStatement(Token &t);
    bool isAliasDeclaration(Token &t);
    bool isComponentDeclaration(Token &t);
    bool isSwitchDeclarator(Token &t);
    bool isConstructorDecl();
    bool isKeyword(string s);
    bool iaAssemblyInstruction(string s);
    bool isExprSymbol(string token);
    bool isForLoopCompareSymbol(string t);
    bool isObfuscationOption(Token &token);
    void parseAccessTypes();
    bool expectIdentifier(Ast*);
    bool expectOverrideOperator(Ast* ast);
    void parseModuleDecl(Ast*);
    void parseImportDecl(Ast*);
    void parseImportItem(Ast*);
    void parseArrayItems(Ast*);
    void parseClassDecl(Ast*);
    void parseMutateDecl(Ast*);
    void parseInterfaceDecl(Ast*);
    void parseObfuscateBlock(Ast*);
    void parseObfuscateElement(Ast *ast);
    void parseObfuscateDecl(Ast*);
    void parseGenericIdentifierList(Ast*);
    void addAccessTypes(Ast*);
    bool parseReferencePointer(Ast*);
    void parseUtypeList(Ast*);
    void parseReferencePointerList(Ast*);
    void parseClassBlock(Ast*);
    void parseInterfaceBlock(Ast*);
    void parseVariableDecl(Ast*);
    void parseInjectRequest(Ast*);
    void parseGetter(Ast *ast);
    void parseSetter(Ast *ast);
    bool parseTypeIdentifier(Ast*);
    bool parseUtype(Ast*);
    bool parseUtypeNaked(Ast*);
    bool parseExpression(Ast*,bool ignoreBinary = false);
    void parseDictExpression(Ast* ast);
    void parseDictElement(Ast* ast);
    void parseDictionaryType(Ast* ast);
    void parseVectorArray(Ast*);
    bool match(int num_args, ...);
    bool equality(Ast*);
    bool shift(Ast*);
    bool comparason(Ast*);
    bool multiplication(Ast*);
    bool exponent(Ast*);
    bool parseFunctionPtr(Ast* ast);
    bool addition(Ast*);
    bool unary(Ast*);
    bool binary(Ast*);
    bool parsePrimaryExpr(Ast*);
    bool parseLiteral(Ast*);
    bool parseRegister(Ast*);
    bool parseAsmLiteral(Ast *ast);
    bool parseDotNotCallExpr(Ast*);
    void parseExpressionList(Ast*,string,string);
    bool parseUtypeArg(Ast*);
    bool parseUtypeArgOpt(Ast*);
    bool parseLambdaArg(Ast*);
    void parseUtypeArgList(Ast*);
    bool parseUtypeArgListOpt(Ast*);
    void parseLambdaArgList(Ast*);
    bool parseArrayExpression(Ast*);
    void parseBaseClassUtype(Ast *ast);
    void parseMethodReturnType(Ast*);
    void parseLambdaReturnType(Ast*);
    void parseMethodDecl(Ast*);
    void parseConstructor(Ast*);
    void parseBaseClassConstructor(Ast*);
    void parseAll(Ast*);
    void parseEnumDecl(Ast*);
    void parseEnumBlock(Ast*);
    void parseInitDecl(Ast *ast);
    void parseEnumIdentifier(Ast*);
    void parseBlock(Ast*);
    void parseLambdaBlock(Ast*);
    bool parseStatement(Ast*);
    void parseReturnStatement(Ast*);
    void parseIfStatement(Ast*);
    void parseAliasDeclaration(Ast *ast);
    void parseComponentDeclaration(Ast *ast);
    void parseComponentTypeList(Ast *ast);
    void parseComponentType(Ast *ast);
    void parseSingleComponentType(Ast *ast);
    void parseFactoryComponentType(Ast *ast);
    void parseComponentName(Ast *ast);
    void parseForStatement(Ast *ast);
    void parseForEachStatement(Ast *ast);
    void parseWhileStatement(Ast*);
    void parseDoWhileStatement(Ast*);
    void parseThrowStatement(Ast*);
    void parseGotoStatement(Ast*);
    void parseWhenStatement(Ast*);
    void parseAsmStatement(Ast*);
    void parseAsmBlock(Ast*);
    void parseAssemblyInstruction(Ast*);
    void parseWhenBlock(Ast*);
    void parseBreakStatement(Ast*);
    void parseContinueStatement(Ast*);
    void parseLockStatement(Ast*);
    void parseTryCatchStatement(Ast*);
    void parseCatchClause(Ast*);
    void parseFinallyBlock(Ast*);
    void parseCatchChain(Ast*);
    void parseLabelDecl(Ast* ast);

    Ast* getBranch(Ast *parent, ast_type type);
    void expect(Ast* ast, string token, bool addToken = true, const char *expectedstr = nullptr);

    List<Ast*> tree;
    Token* _current;
    tokenizer *toks;
    List<string> lines;
    List<Token> access_types;
    ErrorManager *errors;
};

#define _SHARP_CERROR_LIMIT options.max_errors

#define CHECK_ERRLMT(exit_proc) \
    if(panic) exit_proc \
    else if(errors->getUnfilteredErrorCount() > _SHARP_CERROR_LIMIT) { \
        panic = true; \
        exit_proc \
    }


#define RETAIN_RECURSION(rec) \
    long oldRecursionVal = recursion; \
    recursion = rec;

#define RESTORE_RECURSION() \
    recursion = oldRecursionVal;


#endif //SHARP_PARSER_H
