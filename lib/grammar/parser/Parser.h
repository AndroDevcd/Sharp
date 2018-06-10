//
// Created by BraxtonN on 1/22/2018.
//

#ifndef SHARP_PARSER_H
#define SHARP_PARSER_H

#include "../../../stdimports.h"
#include "ErrorManager.h"
#include "tokenizer/tokenizer.h"
#include "ParserState.h"
#include "Ast.h"

class Parser {
public:
    Parser(tokenizer *tokenizer)
    :
    toks(tokenizer),
        cursor(0),
        ast_cursor(-1),
        rStateCursor(-1),
        parsed(false),
        tree(NULL),
        data(""),
        panic(false)
    {
        if(tokenizer != NULL && tokenizer->getErrors() != NULL &&
           !tokenizer->getErrors()->hasErrors())
        {
            data = tokenizer->getData();
            access_types.init();
            tree = new list<Ast>();
            state = new list<ParserState>();
            lines.init();
            parse();
        }
    }

    ErrorManager* getErrors();
    Ast* ast_at(long p);
    size_t treesize() { return tree->size(); }
    void free();
    const string &getData() const;
    tokenizer *getTokenizer() const;

    bool parsed;
    List<string> lines;
    string sourcefile;

    static bool isnative_type(string type);
    static bool isassign_exprsymbol(string token);
    static bool isspecial_native_type(string type);

    /**
     * This is a special flag that denotes that the compiler has detected too many errors
     * and you need to fix your program!
     */
    bool panic;

    static bool iskeyword(string key);


private:
    void parse();
    bool isend();
    token_entity peek(int forward);

    unsigned long cursor;
    list<Ast> *tree;
    list<ParserState>* state;
    int64_t rStateCursor;
    token_entity* _current;
    unsigned long ast_cursor;
    tokenizer *toks;
    std::string data;
    List<token_entity> access_types;
    ErrorManager *errors;

    bool ismodule_decl(token_entity entity);

    bool isclass_decl(token_entity entity);

    bool isimport_decl(token_entity entity);

    void parse_moduledecl(Ast* ast);

    void parse_importdecl(Ast* ast);

    bool isvariable_decl(token_entity token);

    bool ismethod_decl(token_entity token);

    bool isaccess_decl(token_entity token);

    void parse_classdecl(Ast* ast);

    void parse_accesstypes();

    void remove_accesstypes();

    bool expect(token_type type, const char *expectedstr);

    void parse_classblock(Ast *pAst);

    Ast *get_ast(Ast *pAst, ast_types typ);

    void pushback();

    void parse_variabledecl(Ast *pAst);

    bool parse_value(Ast *pAst);

    void parse_methoddecl(Ast *pAst);

    void expect(token_type ty, Ast *pAst, const char *expectedstr);

    void parse_methodreturn_type(Ast *pAst);

    bool isreturn_stmnt(token_entity entity);

    void parse_returnstmnt(Ast *pAst);

    void parse_all(Ast *pAst);

    bool expectidentifier(Ast *pAst);

    bool parse_type_identifier(Ast *pAst);

    bool parse_reference_pointer(Ast *pAst);

    bool parse_statement(Ast *pAst);

    void parse_modulename(Ast *pAst, bool &parsedGeneric);

    void parse_valueassignment(Ast *pAst);

    void retainstate(Ast*);
    void dumpstate();
    Ast* rollback();

    bool parse_expression(Ast *pAst);

    bool parse_primaryexpr(Ast *pAst);

    bool parse_literal(Ast *pAst);

    bool parse_utype(Ast *pAst);

    void expect_token(Ast *pAst, string token, const char *message);

    bool isexprkeyword(string basic_string);

    bool parse_utypearg(Ast *pAst);

    void parse_utypearg_list(Ast *pAst);

    void parse_valuelist(Ast *pAst);

    bool isexprsymbol(string basic_string);

    void parse_block(Ast *pAst);

    void parse_operatordecl(Ast *pAst);

    bool isoverride_operator(std::string t);

    void parse_vectorarray(Ast *pAst);

    bool isif_stmnt(token_entity entity);

    bool iswhile_stmnt(token_entity entity);

    void parse_whilestmnt(Ast *pAst);

    void parse_ifstmnt(Ast *pAst);

    bool isdowhile_stmnt(token_entity entity);

    void parse_dowhilestmnt(Ast *pAst);

    bool istrycatch_stmnt(token_entity entity);

    void parse_trycatch(Ast *pAst);

    void parse_catchclause(Ast *pAst);

    void parse_finallyblock(Ast *pAst);

    bool parse_utypearg_opt(Ast *pAst);

    bool isthrow_stmnt(token_entity entity);

    void parse_throwstmnt(Ast *pAst);

    void parse_labeldecl(Ast *pAst);

    void parse_utypearg_list_opt(Ast *pAst);

    void parse_constructor(Ast *pAst);

    bool isconstructor_decl();

    bool parse_dot_notation_call_expr(Ast *pAst);

    Ast *rollbacklast();

    bool isassembly_stmnt(token_entity entity);

    void parse_assemblystmnt(Ast *pAst);

    void parse_assembly_block(Ast *pAst);

    bool isfor_stmnt(token_entity entity);

    bool isforeach_stmnt(token_entity entity);

    void parse_forstmnt(Ast *pAst);

    void parse_foreachstmnt(Ast *pAst);

    bool parse_utype_naked(Ast *pAst);

    bool parse_array_expression(Ast *pAst);

    void parse_delegatedecl(Ast *pAst);

    bool isinterface_decl(token_entity entity);

    void parse_interfacedecl(Ast *_ast);

    void parse_interfaceblock(Ast *pAst);

    void parse_reference_identifier_list(Ast *ast);

    bool islock_stmnt(token_entity entity);

    void parse_lockstmnt(Ast *pAst);

    bool isprototype_decl(token_entity token);

    void parse_prototypedecl(Ast *pAst);

    void parse_prototype_valueassignment(Ast *pAst);

    void parse_identifier_list(Ast *pAst);

    void parse_utype_list(Ast *ast);

    bool parse_template_decl(Ast *pAst);

    Ast *popBacklast();

    bool isenum_decl(token_entity token);

    void parse_enumdecl(Ast *_ast);

    void parse_enumblock(Ast *pAst);

    void parse_enumidentifier(Ast *_ast);

    bool isswitch_stmnt(token_entity entity);

    void parse_switchstmnt(Ast *pAst);

    void parse_switchblock(Ast *pAst);

    bool isswitch_declarator(token_entity entity);

    void parse_switch_declarator(Ast *pAst);
};

#define _SHARP_CERROR_LIMIT c_options.error_limit

#define CHECK_ERRORS \
    if(panic) return; \
    else if(errors->getUnfilteredErrorCount() > _SHARP_CERROR_LIMIT) { \
        panic = true; \
        return; \
    }

#define SEMTEX_CHECK_ERRORS \
    if(panic) goto report; \
    else if(errors->getUnfilteredErrorCount() > _SHARP_CERROR_LIMIT) { \
        panic = true; \
        goto report; \
    }

#define CHECK_ERRORS_RETURN(x) \
    if(panic) return x; \
    else if(errors->getUnfilteredErrorCount() > _SHARP_CERROR_LIMIT) { \
        panic = true; \
        return x; \
    }


#endif //SHARP_PARSER_H
