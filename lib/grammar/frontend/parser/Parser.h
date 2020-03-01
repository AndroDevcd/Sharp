//
// Created by BraxtonN on 10/11/2019.
//

#ifndef SHARP_PARSER_H
#define SHARP_PARSER_H

#include "../../../../stdimports.h"
#include "../tokenizer/tokenizer.h"
#include "Ast.h"

class parser {
public:
    parser(tokenizer *tokenizer)
            :
            toks(tokenizer),
            ast_cursor(-1),
            rStateCursor(-1),
            parsed(false),
            data(""),
            panic(false)
    {
        if(tokenizer != NULL && tokenizer->getErrors() != NULL &&
           !tokenizer->getErrors()->hasErrors())
        {
            data = tokenizer->getData();
            access_types.init();
            tree.init();
            lines.init();
            parse();
        }
    }

    ErrorManager* getErrors();
    Ast* astAt(long p);
    size_t size() { return tree.size(); }
    void free();
    const string &getData() const;
    tokenizer *getTokenizer() const;
    static bool isStorageType(Token &t);
    static bool isNativeType(string t);

    bool parsed, panic;
    List<string> lines;
    string sourcefile;


private:
    void parse();
    Token* peek(int forward);

    bool isAccessDecl(Token &t);
    bool isModuleDecl(Token &t);
    bool isImportDecl(Token &t);
    bool isClassDecl(Token &t);
    bool isMutateDecl(Token &t);
    bool isInterfaceDecl(Token &t);
    bool isPrototypeDecl(Token &t);
    bool isVariableDecl(Token &t);
    bool isMethodDecl(Token &t);
    bool isInitDecl(Token& token);
    bool isEnumDecl(Token &t);
    bool isStaticDecl(Token &t);
    bool isReturnStatement(Token &t);
    bool isIfStatement(Token &t);
    bool isSwitchStatement(Token &t);
    bool isAssemblyStatement(Token &t);
    bool isForStatement(Token &t);
    bool isAliasDeclaration(Token &t);
    bool isSwitchDeclarator(Token &t);
    bool isConstructorDecl();
    bool isKeyword(string s);
    bool isExprSymbol(string token);
    bool isOverrideOperator(string t);
    bool isAssignExprSymbol(string t);
    bool isForLoopCompareSymbol(string t);
    void parseAccessTypes();

    bool expectIdentifier(Ast*);
    void parseModuleDecl(Ast*);
    void parseImportDecl(Ast*);
    void parseClassDecl(Ast*);
    void parseMutateDecl(Ast*);
    void parseInterfaceDecl(Ast*);
    void parseIdentifierList(Ast*);
    void addAccessTypes(Ast*);
    bool parseReferencePointer(Ast*);
    void parseUtypeList(Ast*);
    void parseReferencePointerList(Ast*);
    void parseClassBlock(Ast*);
    void parseInterfaceBlock(Ast*);
    void parseVariableDecl(Ast*);
    void parseGetter(Ast *ast);
    void parseSetter(Ast *ast);
    bool parseTypeIdentifier(Ast*);
    bool parseUtype(Ast*);
    bool parseUtypeNaked(Ast*);
    bool parseExpression(Ast*);
    void parseDictExpression(Ast* ast);
    void parseDictionaryType(Ast* ast);
    void parseVectorArray(Ast*);
    bool match(int num_args, ...);
    bool equality(Ast*);
    bool shift(Ast*);
    bool comparason(Ast*);
    bool multiplication(Ast*);
    bool addition(Ast*);
    bool unary(Ast*);
    bool binary(Ast*);
    bool parsePrimaryExpr(Ast*);
    bool parseLiteral(Ast*);
    bool parseDotNotCallExpr(Ast*);
    void parseExpressionList(Ast*,string,string);
    void parseFieldInitList(Ast*);
    bool parseUtypeArg(Ast*);
    bool parseUtypeArgOpt(Ast*);
    bool parseLambdaArg(Ast*);
    void parseUtypeArgList(Ast*);
    void parseLambdaArgList(Ast*);
    void parseUtypeArgListOpt(Ast*);
    bool parseArrayExpression(Ast*);
    bool parseFieldInitializatioin(Ast*);
    void parsePrototypeDecl(Ast*, bool semicolon = true);
    void parseMethodReturnType(Ast*);
    void parsePrototypeValueAssignment(Ast*);
    void parseLambdaReturnType(Ast*);
    void parseMethodDecl(Ast*);
    void parseDelegateDecl(Ast*);
    void parseOperatorDecl(Ast*);
    void parseConstructor(Ast*);
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
    void parseSwitchStatement(Ast*);
    void parseSwitchBlock(Ast*);
    void parseSwitchDeclarator(Ast*);
    void parseAssemblyStmnt(Ast*);
    void parseAliasDeclaration(Ast *ast);
    void parseAssemblyBlock(Ast*);
    void parseForStmnt(Ast*);

    Ast* getBranch(Ast *ast, ast_type type);
    void expect(Ast* ast, string token, bool addToken = true, const char *expectedstr = nullptr);

    List<Ast*> tree;
    int64_t rStateCursor;
    Token* _current;
    unsigned long ast_cursor;
    tokenizer *toks;
    std::string data;
    List<Token> access_types;
    ErrorManager *errors;
};

#define _SHARP_CERROR_LIMIT c_options.error_limit

#define CHECK_ERRLMT(exit_proc) \
    if(panic) exit_proc \
    else if(errors->getUnfilteredErrorCount() > _SHARP_CERROR_LIMIT) { \
        panic = true; \
        exit_proc \
    }


#endif //SHARP_PARSER_H
