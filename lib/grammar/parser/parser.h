//
// Created by bknun on 1/7/2017.
//

#ifndef SHARP_PARRSER_H
#define SHARP_PARRSER_H

#include "../../../stdimports.h"
#include "parseerrors.h"
#include "tokenizer/tokenizer.h"
#include "parserstate.h"
#include "ast.h"

class parser
{
public:
    parser(tokenizer *tokenizer)
    :
            toks(tokenizer),
            cursor(0),
            ast_cursor(-1),
            rStateCursor(-1),
            parsed(false),
            tree(NULL)
    {
        if(tokenizer != NULL && tokenizer->geterrors() != NULL &&
                !tokenizer->geterrors()->_errs())
        {
            access_types = new list<token_entity>();
            tree = new list<ast>();
            rState = new list<parser_state>();

            parse();
        }
        else {
            std::free(access_types); access_types = NULL;
            std::free(tree); tree = NULL;
        }
    }

    Errors* geterrors();
    ast* ast_at(long p);
    void free();
    bool parsed;

private:
    void parse();
    bool isend();
    token_entity current();
    void advance();
    token_entity peek(int forward);
    static bool iskeyword(string key);

    int64_t cursor;
    tokenizer *toks;
    list<ast> *tree;
    list<parser_state>* rState;
    int64_t rStateCursor;
    token_entity* _current;
    int64_t  ast_cursor;
    list<token_entity> *access_types;
    Errors *errors;

    void eval(ast* _ast);

    bool ismodule_decl(token_entity entity);

    bool isclass_decl(token_entity entity);

    bool isimport_decl(token_entity entity);

    void parse_moduledecl(ast* _ast);

    void parse_importdecl(ast* _ast);

    bool isvariable_decl(token_entity token);

    bool ismethod_decl(token_entity token);

    bool isnative_type(string type);

    bool isaccess_decl(token_entity token);

    void parse_classdecl(ast* ast);

    void parse_accesstypes();

    void remove_accesstypes();

    bool expect(token_type type, const char *expectedstr);

    void parse_classblock(ast *pAst);

    ast *get_ast(ast *pAst, ast_types typ);

    void pushback();

    void parse_variabledecl(ast *pAst);

    bool parse_value(ast *pAst);

    void parse_methoddecl(ast *pAst);

    void expect(token_type ty, ast *pAst, const char *expectedstr);

    void parse_methodblock(ast *pAst);

    void parse_methodreturn_type(ast *pAst);

    bool isreturn_stmnt(token_entity entity);

    void parse_returnstmnt(ast *pAst);

    void parse_all(ast *pAst);

    bool expectidentifier(ast *pAst);

    bool parse_type_identifier(ast *pAst);

    bool parse_reference_pointer(ast *pAst);

    void parse_statement(ast *pAst);

    void parse_modulename(ast *pAst);

    void parse_valueassignment(ast *pAst);

    void retainstate(ast*);
    void dumpstate();
    ast* rollback();

    bool parse_expression(ast *pAst);

    bool parse_primaryexpr(ast *pAst);

    bool parse_literal(ast *pAst);

    bool parse_utype(ast *pAst);

    void expect_token(ast *pAst, string token, const char *message);

    bool isexprkeyword(string basic_string);

    void parse_typeargs(ast *pAst);

    bool parse_utypearg(ast *pAst);

    void parse_utypearg_list(ast *pAst);

    void parse_valuelist(ast *pAst);

    bool isexprsymbol(string basic_string);

    bool isassign_exprsymbol(string token);

    void parse_memaccess_flag(ast *pAst);

    bool ismemaccess_flag(string basic_string);

    bool ismacros_decl(token_entity entity);

    void parse_macrosdecl(ast *pAst);

    void parse_block(ast *pAst);
};

#endif //SHARP_PARRSER_H
