//
// Created by BraxtonN on 1/22/2018.
//

#include "Parser.h"
#include "../runtime.h"
#include <sstream>

void Parser::parse()
{
    if(toks->getEntities().size() == 0)
        return;

    parsed = true;
    std::copy(toks->getLines()->begin(), toks->getLines()->end(),
              std::back_inserter(*lines));
    sourcefile = toks->file;

    errors = new ErrorManager(lines, sourcefile, false, c_options.aggressive_errors);
    _current= &toks->getEntities().get(cursor);

    while(!isend())
    {
        if(panic)
            break;

        eval(NULL);
    }

    if(errors->hasErrors())
        return;
}

ErrorManager* Parser::geterrors()
{
    return errors;
}

bool Parser::isend() {
    return current().getTokenType() == _EOF;
}

void Parser::eval(Ast* _ast) {
    if(isaccess_decl(current()))
    {
        parse_accesstypes();
    }
    CHECK_ERRORS

    if(isend())
    {
        remove_accesstypes();
        return;
    }
    else if(ismodule_decl(current()))
    {
        if(access_types->size() > 0)
        {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parse_moduledecl(_ast);
    }
    else if(isclass_decl(current()))
    {
        parse_classdecl(_ast);
    }
    else if(isimport_decl(current()))
    {
        if(access_types->size() > 0)
        {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parse_importdecl(_ast);
    }
    else if(ismacros_decl(current()))
    {
        parse_macrosdecl(_ast);
    }
    else
    {
        // "expected class, or import declaration"
        errors->createNewError(UNEXPECTED_SYMBOL, current(), " `" + current().getToken() + "`; expected class, or import declaration");
        parse_all(_ast);
    }

    advance();
    remove_accesstypes();
}

void Parser::parse_classdecl(Ast* _ast) { // 1
    _ast = get_ast(_ast, ast_class_decl);

    for(token_entity &entity : *access_types)
    {
        _ast->addEntity(entity);
    }
    _ast->addEntity(current());


    expectidentifier(_ast);

    if(peek(1).getToken() == "base")
    {
        advance();
        expect_token(_ast, "base", "");

        parse_reference_pointer(_ast);
    }

    parse_classblock(_ast);
}

void Parser::parse_importdecl(Ast* _ast) {
    _ast = get_ast(_ast, ast_import_decl);
    _ast->addEntity(current());

    parse_modulename(_ast);

    expect(SEMICOLON, "`;`");

    //cout << "parsed import declaration" << endl;
}

void Parser::parse_moduledecl(Ast* _ast) {
    _ast = get_ast(_ast, ast_module_decl);
    _ast->addEntity(current());

    parse_modulename(_ast);

    if(!expect(SEMICOLON, "`;`"))
        return;

    //cout << "parsed module declaration" << endl;
}

token_entity Parser::current()
{
    return *_current;
}

void Parser::advance()
{
    if((cursor+1)>=toks->getEntityCount())
        _current =toks->EOF_token;
    else
        _current = &toks->getEntities().get(++cursor);
}

token_entity Parser::peek(int forward)
{

    if(cursor+forward >= toks->getEntities().size())
        return toks->getEntities().get(toks->getEntityCount()-1);
    else
        return toks->getEntities().get(cursor+forward);
}

bool Parser::isvariable_decl(token_entity token) {
    return isnative_type(token.getToken());
}

bool Parser::ismethod_decl(token_entity token) {
    return token.getId() == IDENTIFIER && token.getToken() == "fn";
}

Ast* Parser::ast_at(long p)
{
    return &(*std::next(tree->begin(), p));
}

bool Parser::ismodule_decl(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "mod";
}

bool Parser::isclass_decl(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "class";
}

bool Parser::isimport_decl(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "import";
}

bool Parser::isreturn_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "return";
}

bool Parser::ismacros_decl(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "macros";
}

bool Parser::isif_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "if";
}

bool Parser::iswhile_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "while";
}

bool Parser::isfor_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "for";
}

bool Parser::isforeach_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "foreach";
}

bool Parser::isassembly_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "__asm";
}

bool Parser::isdowhile_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "do";
}

bool Parser::istrycatch_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "try";
}

bool Parser::isthrow_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "throw";
}

bool Parser::isconstructor_decl() {
    return current().getId() == IDENTIFIER && !iskeyword(current().getToken()) &&
           peek(1).getTokenType() == LEFTPAREN;
}

bool Parser::isnative_type(string type) {
    return type == "var"

           /*
            * These reserved words are not native types
            * they are only used in casting since the Sharp VM
            * dosent store these values under the hood
            */
           || type == "__int8" || type == "__int16"
           || type == "__int32" || type == "__int64"
           || type == "__uint8" || type == "__uint16"
           || type == "__uint32" || type == "__uint64"
           /*
            * This is not a native type but we want this to be
            * able to be set as a variable
            */
           || type == "dynamic_object"
            ;
}

bool Parser::isaccess_decl(token_entity token) {
    return
            token.getId() == IDENTIFIER && token.getToken() == "protected" ||
            token.getId() == IDENTIFIER && token.getToken() == "private" ||
            token.getId() == IDENTIFIER && token.getToken() == "static" ||
            token.getId() == IDENTIFIER && token.getToken() == "const" ||
            token.getId() == IDENTIFIER && token.getToken() == "override" ||
            token.getId() == IDENTIFIER && token.getToken() == "public";
}

void Parser::parse_accesstypes() {
    remove_accesstypes();

    while(isaccess_decl(current()))
    {
        access_types->push_back(current());
        advance();
    }
}

void Parser::remove_accesstypes() {
    if(access_types->size() > 0)
        access_types->clear();
}

void Parser::expect(token_type ty, Ast* pAst, const char *expectedstr) {
    advance();

    if(current().getTokenType() == ty)
    {
        if(pAst != NULL)
            pAst->addEntity(current());
    }
    else {
        errors->createNewError(GENERIC, current(), "expected " + string(expectedstr));
    }
}


bool Parser::expectidentifier(Ast* pAst) {
    advance();

    if(current().getId() == IDENTIFIER && !iskeyword(current().getToken()))
    {
        if(pAst != NULL)
            pAst->addEntity(current());
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected identifier");
    }
    return false;
}


void Parser::expect_token(Ast *pAst, string token, const char *message) {
    if(current().getToken() != token)
    {
        errors->createNewError(GENERIC, current(), "expected " + std::string(message));
    } else {
        pAst->addEntity(current());
    }
}

bool Parser::expect(token_type type, const char *expectedstr) {
    advance();

    if(current().getTokenType() == type)
    {
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected " + string(expectedstr));
    }
    return false;
}

void Parser::parse_classblock(Ast *pAst) {
    expect(LEFTCURLY, "`{` after class declaration");
    pAst = get_ast(pAst, ast_block);

    int brackets = 1;

    while(!isend() && brackets > 0)
    {
        CHECK_ERRORS

        advance();
        if(isaccess_decl(current()))
        {
            parse_accesstypes();
        }

        if(ismodule_decl(current()))
        {
            if(access_types->size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected module declaration");
            parse_moduledecl(pAst);
        }
        else if(isclass_decl(current()))
        {
            parse_classdecl(pAst);
        }
        else if(isimport_decl(current()))
        {
            if(access_types->size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected import declaration");
            parse_importdecl(pAst);
        }
        else if(ismacros_decl(current()))
        {
            parse_macrosdecl(pAst);
        }
        else if(isvariable_decl(current()))
        {
            parse_variabledecl(pAst);
        }
        else if(ismethod_decl(current()))
        {
            if(peek(1).getToken() == "operator")
                parse_operatordecl(pAst);
            else
                parse_methoddecl(pAst);
        }
        else if(isconstructor_decl())
        {
            parse_constructor(pAst);
        }
        else if(current().getTokenType() == _EOF)
        {
            errors->createNewError(UNEXPECTED_EOF, current());
            break;
        }
        else if (current().getTokenType() == RIGHTCURLY)
        {
            if((brackets-1) < 0)
            {
                errors->createNewError(ILLEGAL_BRACKET_MISMATCH, current());
            }
            else
            {
                brackets--;

                // end of class block
                if(brackets == 0)
                {
                    pushback();
                    break;
                }
            }
        }
        else if(current().getTokenType() == LEFTCURLY)
            brackets++;
        else {
            // save parser state
            this->retainstate(pAst);
            pushback();

            /*
             * variable decl?
             */
            if(parse_utype(pAst))
            {
                if(peek(1).getId() == IDENTIFIER)
                {
                    // Variable decliration
                    pAst = this->rollbacklast();
                    parse_variabledecl(pAst);
                    remove_accesstypes();
                    continue;
                }
            }

            pAst = this->rollback();
            errors->createNewError(GENERIC, current(), "expected method, class, or variable declaration");
            parse_all(pAst);
        }

        remove_accesstypes();
    }

    if(brackets != 0)
        errors->createNewError(MISSING_BRACKET, current(), " expected `}` at end of class declaration");

    expect(RIGHTCURLY, "`}` at end of class declaration");
}

/**
 * Gets the current ast based on context of the program. If we are at the highest level,
 * the highests level ast is returned otherwise we get back a newly created ast branch in the tree
 * @param pAst
 * @return
 */
Ast * Parser::get_ast(Ast *pAst, ast_types typ) {
    if(pAst == NULL)
    {
        tree->push_back(Ast(NULL, typ, current().getLine(), current().getColumn()));
        ast_cursor++;

        return ast_at(ast_cursor);
    }
    else {
        pAst->addAst(Ast(pAst, typ, peek(1).getLine(), peek(1).getColumn()));

        return pAst->getSubAst(pAst->getSubAstCount() - 1);
    }
}

void Parser::pushback() {
    if(cursor > 0) {
        _current = &toks->getEntities().get(cursor-1);
        cursor--;
    }
}

int partialdecl = 0;
void Parser::parse_variabledecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_var_decl);

    if(partialdecl ==0) {
        for(token_entity &entity : *access_types)
        {
            pAst->addEntity(entity);
        }
        pushback();
        if(!parse_utype(pAst))
            errors->createNewError(GENERIC, current(), "expected native type or reference pointer");
    } else {
        partialdecl--;
    }

    expectidentifier(pAst);

    parse_valueassignment(pAst);

    if(peek(1).getTokenType() == COMMA) {
        partialdecl++;
        expect(COMMA, "`,`");

        parse_variabledecl(partialdecl <= 1 ? pAst : pAst->getParent());
    } else if(partialdecl == 0)
        expect(SEMICOLON, "`;`");
}

void Parser::parse_valueassignment(Ast *pAst) {
    advance();
    if(isassign_exprsymbol(current().getToken()))
    {
        pAst->addEntity(current());
        parse_value(pAst);
    }
    else
        pushback();
}

bool Parser::parse_literal(Ast *pAst) {
    pAst = get_ast(pAst, ast_literal);

    token_entity e = peek(1);
    if(e.getId() == CHAR_LITERAL || e.getId() == INTEGER_LITERAL
       || e.getId() == STRING_LITERAL || e.getId() == HEX_LITERAL
       || e.getToken() == "true" || e.getToken() == "false")
    {
        advance();
        pAst->addEntity(current());
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected literal of type (string, char, hex, or bool)");
        return false;
    }
}

void Parser::parse_assembly_block(Ast *pAst) {
    pAst = get_ast(pAst, ast_assembly_block);

    if(peek(1).getId() == STRING_LITERAL) {
        advance();
        pAst->addEntity(current());

        while(peek(1).getId() == STRING_LITERAL) {
            advance();
            pAst->addEntity(current());
        }
    } else {
        errors->createNewError(GENERIC, current(), "expected string literal");
    }
}

bool Parser::parse_utype(Ast *pAst) {
    pAst = get_ast(pAst, ast_utype);

    if(parse_type_identifier(pAst))
    {
        if(peek(1).getTokenType() == LEFTBRACE)
        {
            advance();
            pAst->addEntity(current());

            expect(RIGHTBRACE, pAst, "`]`");
        }

        return true;
    }
    else
        errors->createNewError(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

bool Parser::parse_utype_naked(Ast *pAst) {
    pAst = get_ast(pAst, ast_utype);

    if(parse_type_identifier(pAst)) {
        return true;
    }
    else
        errors->createNewError(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

bool Parser::parse_primaryexpr(Ast *pAst) {
    pAst = get_ast(pAst, ast_primary_expr);


    errors->enableErrorCheckMode();
    if(parse_literal(pAst))
    {
        errors->fail();
        pAst->encapsulate(ast_literal_e);
        return true;
    }
    errors->pass();

    errors->enableErrorCheckMode();
    if(peek(1).getTokenType() == DOT) {
        advance();
        pAst->addEntity(current());
    }

    this->retainstate(pAst);
    if(parse_utype(pAst))
    {
        if(peek(1).getTokenType() == DOT)
        {
            expect(DOT, pAst->getSubAst(0), "`.`");
            advance();

            expect_token(pAst->getSubAst(0), "class", "`class` after primary expression");

            this->dumpstate();
            errors->fail();
            pAst->encapsulate(ast_utype_class_e);
            return true;
        }else
            pAst = this->rollback();
    } else
        pAst = this->rollback();
    errors->pass();

    this->retainstate(pAst);
    errors->enableErrorCheckMode();
    if(parse_dot_notation_call_expr(pAst)) {
        this->dumpstate();
        errors->fail();
        pAst->encapsulate(ast_dot_not_e);
        return true;
    } else {
        errors->pass();
        pAst = this->rollback();
    }

    return false;
}

void Parser::parse_valuelist(Ast *pAst) {
    pAst = get_ast(pAst, ast_value_list);

    expect(LEFTPAREN, pAst, "`(`");

    if(peek(1).getTokenType() != RIGHTPAREN)
    {
        parse_value(pAst);

        _pValue:
        if(peek(1).getTokenType() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_value(pAst);
            goto _pValue;
        }
    }

    expect(RIGHTPAREN, pAst, "`)`");
}

bool Parser::isassign_exprsymbol(string token) {
    return token == "+=" || token == "-="||
           token == "*=" || token == "/="||
           token == "&=" || token == "|="||
           token == "^=" || token == "%="||
           token == "=";
}

bool Parser::isexprsymbol(string token) {
    return token == "[" || token == "++" ||
           token == "--" || token == "*" ||
           token == "/" || token == "%" ||
           token == "-" || token == "+"||
           token == ">>" || token == "<<"||
           token == "<" || token == ">"||
           token == "<=" || token == ">="||
           token == "==" || token == "!="||
           token == "&" || token == "|"||
           token == "&&" || token == "||"||
           token == "^" || token == "?" ||
           isassign_exprsymbol(token);
}

bool Parser::isoverride_operator(string token) {
    return isassign_exprsymbol(token) ||
           token == "++" ||token == "--" ||
           token == "*" || token == "/" ||
           token == "%" || token == "-" ||
           token == "+" || token == "==" ||

           token == "&&" || token == "||" ||
           token == ">>" || token == "<<"||
           token == "<" || token == ">"||
           token == "<=" || token == ">="||
           token == "!="
            ;
}

int nestedAddExprs = 0, parenExprs=0, nestedAndExprs=0;
bool Parser::parse_dot_notation_call_expr(Ast *pAst) {
    pAst = get_ast(pAst, ast_dotnotation_call_expr);

    if(peek(1).getTokenType() == DOT)
    {
        advance();
        pAst->addEntity(current());
    }

    if(parse_utype_naked(pAst)) {
        if(peek(1).getTokenType() == LEFTPAREN) {
            int nestedAdds=nestedAddExprs;
            nestedAddExprs=0;
            parse_valuelist(pAst);
            nestedAddExprs=nestedAdds;

            pAst->encapsulate(ast_dot_fn_e);
            /* func()++ or func()--
             * This expression rule dosen't process correctly by itsself
             * so we hav to do it ourselves
             */
            if(peek(1).getTokenType() == _INC || peek(1).getTokenType() == _DEC)
            {
                advance();
                pAst->addEntity(current());
            }
            else if(peek(1).getTokenType() == LEFTBRACE) {
                advance();
                pAst->addEntity(current());

                parse_expression(pAst);
                expect(RIGHTBRACE, pAst, "`]`");

                errors->enableErrorCheckMode();
                this->retainstate(pAst);
                if(!parse_dot_notation_call_expr(pAst))
                {
                    errors->pass();
                    this->rollbacklast();
                } else {
                    this->dumpstate();
                    errors->fail();
                }
            }
            else {
                errors->enableErrorCheckMode();
                this->retainstate(pAst);
                if(!parse_dot_notation_call_expr(pAst))
                {
                    errors->pass();
                    this->rollbacklast();
                } else {
                    this->dumpstate();
                    errors->fail();
                }
            }
        }
    } else {
        pushback();
        return false;
    }

    return true;
}

bool Parser::parse_array_expression(Ast* pAst) {
    pAst = get_ast(pAst, ast_array_expression);

    this->retainstate(pAst);
    errors->enableErrorCheckMode();
    expect(LEFTBRACE, pAst, "`[`");

    if(peek(1).getTokenType() != RIGHTBRACE) {
        if(!parse_expression(pAst))
        {
            errors->pass();
            this->rollback();
            return false;
        } else
        {
            this->dumpstate();
            errors->fail();
        }
    } else {
        if (peek(2).getTokenType() == LEFTCURLY) {
            pushback();
            return false;
        }
    }

    expect(RIGHTBRACE, pAst, "`]`");

    return true;

}

bool Parser::parse_expression(Ast *pAst) {
    pAst = get_ast(pAst, ast_expression);
    CHECK_ERRORS2(false)

    /* ++ or -- before the expression */
    if(peek(1).getTokenType() == _INC || peek(1).getTokenType() == _DEC)
    {
        advance();
        pAst->addEntity(current());
        parse_expression(pAst);
        pAst->encapsulate(ast_pre_inc_e);
        return true;
    }


    if(peek(1).getTokenType() == LEFTPAREN)
    {
        this->retainstate(pAst);
        errors->enableErrorCheckMode();

        advance();
        pAst->addEntity(current());

        if(!parse_utype(pAst))
        {
            errors->pass();
            this->rollback();
        } else {
            if(peek(1).getTokenType() == RIGHTPAREN)
            {
                expect(RIGHTPAREN, pAst, "`)`");
                if(!parse_expression(pAst))
                {
                    errors->pass();
                    this->rollback();
                } else
                {
                    this->dumpstate();
                    errors->fail();
                    pAst->encapsulate(ast_cast_e);
                    return true;
                }
            }else {
                errors->pass();
                this->rollback();
            }
        }

    }


    if(peek(1).getTokenType() == NOT)
    {
        advance();
        pAst->addEntity(current());

        parse_expression(pAst);

        pAst->encapsulate(ast_not_e);
        if(!isexprsymbol(peek(1).getToken()))
            return true;
    }

    if(peek(1).getTokenType() == LEFTPAREN)
    {
        advance();
        pAst->addEntity(current());

        int oldNestedExprs=nestedAddExprs;
        nestedAddExprs=0;
        parse_expression(pAst);
        nestedAddExprs=oldNestedExprs;

        expect(RIGHTPAREN, pAst, "`)`");

        if(!isexprsymbol(peek(1).getToken())) {
            this->retainstate(pAst);
            errors->enableErrorCheckMode();
            if(parse_dot_notation_call_expr(pAst)) {
                this->dumpstate();
                errors->fail();
            } else {
                errors->pass();
                pAst = this->rollbacklast();
            }

            pAst->encapsulate(ast_paren_e);
            return true;
        } else
            pAst->encapsulate(ast_paren_e);
    }

    if(peek(1).getTokenType() == LEFTCURLY)
    {
        parse_vectorarray(pAst);
        pAst->encapsulate(ast_vect_e);
        return true;
    }

    this->retainstate(pAst);
    if(parse_primaryexpr(pAst)) {
        this->dumpstate();
        if(!isexprsymbol(peek(1).getToken()))
            return true;
    }
    else {
        this->rollbacklast();
    }

    if(peek(1).getToken() == "self")
    {
        advance();
        expect_token(pAst, "self", "");

        if(peek(1).getTokenType() == PTR) {
            expect(PTR, pAst, "`->` after self");
            parse_dot_notation_call_expr(pAst);
        }

        pAst->encapsulate(ast_self_e);
        if(!isexprsymbol(peek(1).getToken()))
            return true;
    }

    if(peek(1).getToken() == "base")
    {
        advance();
        expect_token(pAst, "base", "");
        expect(PTR, pAst, "`->` after base");
        parse_dot_notation_call_expr(pAst);

        pAst->encapsulate(ast_base_e);
        if(!isexprsymbol(peek(1).getToken()))
            return true;
    }

    if(peek(1).getToken() == "null")
    {
        advance();
        expect_token(pAst, "null", "");
        pAst->encapsulate(ast_null_e);
        return true;
    }

    if(peek(1).getToken() == "new")
    {
        advance();
        expect_token(pAst, "new", "");
        parse_utype_naked(pAst);

        if(peek(1).getTokenType() == LEFTBRACE && parse_array_expression(pAst)){}
        else if(peek(1).getTokenType() == LEFTBRACE) {
            expect(LEFTBRACE, pAst, "`[`");
            expect(RIGHTBRACE, pAst, "`]`");
            parse_vectorarray(pAst);
        }
        else if(peek(1).getTokenType() == LEFTPAREN)
            parse_valuelist(pAst);

        pAst->encapsulate(ast_new_e);
        if(peek(1).getTokenType() != LEFTBRACE && !isexprsymbol(peek(1).getToken()))
            return true;
    }

    if(peek(1).getToken() == "sizeof")
    {
        advance();
        expect_token(pAst, "sizeof", "");

        expect(LEFTPAREN, pAst, "`(`");
        parse_expression(pAst);
        expect(RIGHTPAREN, pAst, "`)`");

        pAst->encapsulate(ast_sizeof_e);
        if(!isexprsymbol(peek(1).getToken()))
            return true;
    }

    if(peek(1).getTokenType() == LEFTBRACE)
    {
        advance();
        pAst->addEntity(current());

        parse_expression(pAst);
        expect(RIGHTBRACE, pAst, "`]`");



        if(!isexprsymbol(peek(1).getToken())){
            errors->enableErrorCheckMode();
            this->retainstate(pAst);
            if(!parse_dot_notation_call_expr(pAst)) {
                this->rollbacklast();
                errors->pass();
            }
            else {
                errors->fail();
                this->dumpstate();
            }

            pAst->encapsulate(ast_arry_e);
            return true;
        }
        pAst->encapsulate(ast_arry_e);
    }



    /* ++ or -- after the expression */
    if(peek(1).getTokenType() == _INC || peek(1).getTokenType() == _DEC)
    {
        advance();
        pAst->addEntity(current());

        pAst->encapsulate(ast_post_inc_e);
        if(!isexprsymbol(peek(1).getToken()))
            return true;
    }

    /* expression ('+'|'-') expression */
    if(peek(1).getTokenType() == PLUS || peek(1).getTokenType() == MINUS)
    {
        advance();
        pAst->addEntity(current());

        if(parenExprs > 0) {

            parse_expression(pAst);
            pAst->encapsulate(ast_add_e);
            parenExprs--;
        } else {
            nestedAddExprs++;
            parse_expression(nestedAddExprs == 1 ? pAst : pAst->getParent());
            if(nestedAddExprs == 1)
                pAst->encapsulate(ast_add_e);
            nestedAddExprs--;
        }


        return true;
    }

    /* expression ('*'|'/'|'%') expression */
    if(peek(1).getTokenType() == MULT || peek(1).getTokenType() == _DIV ||
       peek(1).getTokenType() == _MOD)
    {
        advance();
        pAst->addEntity(current());

        if(parenExprs > 0) {

            parse_expression(pAst);
            pAst->encapsulate(ast_mult_e);
            parenExprs--;
        } else {
            nestedAddExprs++;
            parse_expression(nestedAddExprs == 1 ? pAst : pAst->getParent());
            if(nestedAddExprs == 1)
                pAst->encapsulate(ast_mult_e);
            nestedAddExprs--;
        }
        return true;
    }


    /* expression ('<<'|'>>') expression */
    if(peek(1).getTokenType() == SHL || peek(1).getTokenType() == SHR)
    {
        advance();
        pAst->addEntity(current());

        parse_expression(pAst);
        pAst->encapsulate(ast_shift_e);
        return true;
    }

    /* expression ('<=' | '>=' | '>' | '<') expression */
    if(peek(1).getTokenType() == LESSTHAN || peek(1).getTokenType() == GREATERTHAN ||
       peek(1).getTokenType() == _GTE || peek(1).getTokenType() == _LTE)
    {
        advance();
        pAst->addEntity(current());

        parse_expression(pAst);
        pAst->encapsulate(ast_less_e);
        return true;
    }

    /* expression ('!=' | '==' | '=') expression */
    if(peek(1).getTokenType() == EQEQ || peek(1).getTokenType() == NOTEQ ||
       peek(1).getTokenType() == ASSIGN)
    {
        advance();
        pAst->addEntity(current());

        parse_expression(pAst);
        pAst->encapsulate(ast_equal_e);
        return true;
    }

    /* expression ('&' | '^' | '|' | '&&' | '||') expression */
    if(peek(1).getTokenType() == AND || peek(1).getTokenType() == OR ||
       peek(1).getTokenType() == ANDAND || peek(1).getTokenType() == OROR ||
       peek(1).getTokenType() == XOR)
    {
        advance();
        pAst->addEntity(current());

        if(parenExprs > 0) {

            parse_expression(pAst);
            pAst->encapsulate(ast_and_e);
            parenExprs--;
        } else {
            nestedAndExprs++;
            parse_expression(nestedAndExprs == 1 ? pAst : pAst->getParent());
            if(nestedAndExprs == 1)
                pAst->encapsulate(ast_and_e);
            nestedAndExprs--;
        }
        return true;
    }

    /* expression '?' expression ':' expression */
    if(peek(1).getTokenType() == QUESMK)
    {
        advance();
        pAst->addEntity(current());

        parse_expression(pAst);

        expect(COLON, pAst, "`:`");

        parse_expression(pAst);
        pAst->encapsulate(ast_ques_e);
        return true;
    }

    /* expression <assign-expr> expression */
    if(isassign_exprsymbol(peek(1).getToken()))
    {
        advance();
        pAst->addEntity(current());

        int oldAndExprs=nestedAndExprs;
        parenExprs++;
        parse_expression(pAst);
        nestedAddExprs=oldAndExprs;
        pAst->encapsulate(ast_assign_e);
        return true;
    }

    errors->createNewError(GENERIC, current(), "expected expression");
    return false;
}

bool Parser::parse_value(Ast *pAst) {
    pAst = get_ast(pAst, ast_value);
    return parse_expression(pAst);
}

bool Parser::parse_utypearg(Ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg);

    if(parse_utype(pAst))
    {
        expectidentifier(pAst);
        return true;
    }else
        errors->createNewError(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

bool Parser::parse_utypearg_opt(Ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg_opt);

    if(parse_utype(pAst))
    {
        errors->enableErrorCheckMode();
        if(!expectidentifier(pAst)) {
            errors->pass();
            pushback();
        }
        else
            errors->fail();
        return true;
    }else
        errors->createNewError(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

void Parser::parse_vectorarray(Ast* pAst) {
    pAst = get_ast(pAst, ast_vector_array);
    expect(LEFTCURLY, pAst, "`{`");

    if(peek(1).getTokenType() != RIGHTCURLY)
    {
        parse_expression(pAst);
        _pExpr:
        if(peek(1).getTokenType() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_expression(pAst);
            goto _pExpr;
        }
    }

    expect(RIGHTCURLY, pAst, "`}`");
}

void Parser::parse_utypearg_list_opt(Ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg_list_opt);
    expect(LEFTPAREN, pAst, "`(`");

    if(peek(1).getTokenType() != RIGHTPAREN)
    {
        parse_utypearg_opt(pAst);
        _puTypeArgOpt:
        if(peek(1).getTokenType() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_utypearg_opt(pAst);
            goto _puTypeArgOpt;
        }
    }

    expect(RIGHTPAREN, pAst, "`)`");
}

void Parser::parse_utypearg_list(Ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg_list);
    expect(LEFTPAREN, pAst, "`(`");

    if(peek(1).getTokenType() != RIGHTPAREN)
    {
        parse_utypearg(pAst);
        _puTypeArg:
        if(peek(1).getTokenType() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_utypearg(pAst);
            goto _puTypeArg;
        }
    }

    expect(RIGHTPAREN, pAst, "`)`");
}

void Parser::parse_block(Ast* pAst) {
    expect(LEFTCURLY, "`{`");
    pAst = get_ast(pAst, ast_block);

    while(!isend())
    {
        CHECK_ERRORS

        advance();
        if (current().getTokenType() == RIGHTCURLY)
        {
            pushback();
            break;
        }
        else if(current().getTokenType() == LEFTCURLY)
        {
            pushback();
            parse_block(pAst);
        }
        else if(current().getTokenType() == _EOF)
        {
            errors->createNewError(UNEXPECTED_EOF, current());
            break;
        } else
            parse_statement(pAst);

        remove_accesstypes();
    }

    expect(RIGHTCURLY, "`}`");
}

void Parser::parse_macrosdecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_macros_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->addEntity(entity);
    }
    pAst->addEntity(current());

    expectidentifier(pAst);

    parse_utypearg_list(pAst);
    parse_block(pAst);

    expect(SEMICOLON, pAst, "`;`");
}

void Parser::parse_operatordecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_operator_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->addEntity(entity);
    }
    pAst->addEntity(current());

    advance();
    expect_token(
            pAst, "operator", "`operator`");

    advance();
    if(!isoverride_operator(current().getToken()))
        errors->createNewError(GENERIC, current(), "expected override operator");
    else
        pAst->addEntity(current());

    parse_utypearg_list(pAst);

    parse_methodreturn_type(pAst); // assign-expr operators must return void
    parse_block(pAst);
}

void Parser::parse_constructor(Ast *pAst) {
    pAst = get_ast(pAst, ast_construct_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->addEntity(entity);
    }
    pushback();

    expectidentifier(pAst);

    parse_utypearg_list(pAst);
    parse_block(pAst);
}

void Parser::parse_methoddecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_method_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->addEntity(entity);
    }
    pAst->addEntity(current());

    expectidentifier(pAst);

    parse_utypearg_list(pAst);

    parse_methodreturn_type(pAst);
    parse_block(pAst);

}

void Parser::parse_methodreturn_type(Ast *pAst) {
    if(peek(1).getTokenType() == COLON)
    {
        pAst = get_ast(pAst, ast_method_return_type);
        advance();

        pAst->addEntity(current());
        parse_utype(pAst);
    }
}

void Parser::parse_returnstmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_return_stmnt);

    pAst->addEntity(current());

    if(peek(1).getToken() != ";")
        parse_value(pAst);

    expect(SEMICOLON, pAst, "`;`");
}

void Parser::parse_assemblystmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_assembly_statement);

    expect_token(pAst, "__asm", "`__asm`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_assembly_block(pAst);
    expect(RIGHTPAREN, pAst, "`)`");
    expect(SEMICOLON, pAst, "`;`");
}

void Parser::parse_forstmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_for_statement);

    expect_token(pAst, "for", "`for`");

    expect(LEFTPAREN, pAst, "`(`");

    if(peek(1).getTokenType() != SEMICOLON) {
        parse_utypearg(pAst);
        parse_valueassignment(pAst);
    }

    expect(SEMICOLON, pAst, "`;`"); // The inititalizer

    if(peek(1).getTokenType() != SEMICOLON) {
        parse_expression(pAst);
        pAst->getSubAst(ast_expression)->setAstType(ast_for_expresion_cond);
    }
    expect(SEMICOLON, pAst, "`;`");

    if(peek(1).getTokenType() != RIGHTPAREN) {
        parse_expression(pAst);
        pAst->getSubAst(ast_expression)->setAstType(ast_for_expresion_iter);
    }
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);
}

void Parser::parse_foreachstmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_foreach_statement);

    expect_token(pAst, "foreach", "`foreach`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_utypearg(pAst);

    expect(COLON, pAst, "`:`");

    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);
}

void Parser::parse_whilestmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_while_statement);

    expect_token(pAst, "while", "`while`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);
}

void Parser::parse_dowhilestmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_do_while_statement);

    expect_token(pAst, "do", "`do`");
    parse_block(pAst);

    advance();
    expect_token(pAst, "while", "`while`");
    expect(LEFTPAREN, pAst, "`(`");
    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    expect(SEMICOLON, pAst, "`;`");

}

void Parser::parse_ifstmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_if_statement);

    expect_token(pAst, "if", "`if`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);

    Ast* tmp;
    bool isElse = false;
    condexpr:
    if(peek(1).getToken() == "else")
    {
        if(peek(2).getToken() == "if")
        {
            tmp = get_ast(pAst, ast_elseif_statement);

            advance();
            tmp->addEntity(current());
            advance();
            tmp->addEntity(current());

            expect(LEFTPAREN, tmp, "`(`");
            parse_expression(tmp);
            expect(RIGHTPAREN, tmp, "`)`");
        }
        else
        {
            tmp = get_ast(pAst, ast_else_statement);

            advance();
            tmp->addEntity(current());
            isElse = true;
        }


        parse_block(tmp);
        if(!isElse)
            goto condexpr;
    }
}

void Parser::parse_catchclause(Ast *pAst) {
    pAst = get_ast(pAst, ast_catch_clause);

    advance();
    expect_token(pAst, "catch", "`catch`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_utypearg_opt(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);
}

void Parser::parse_finallyblock(Ast *pAst) {
    pAst = get_ast(pAst, ast_finally_block);

    advance();
    expect_token(pAst, "finally", "`finally`");
    parse_block(pAst);
}

void Parser::parse_trycatch(Ast *pAst) {
    pAst = get_ast(pAst, ast_trycatch_statement);

    expect_token(pAst, "try", "`try`");
    parse_block(pAst);

    while(peek(1).getToken() == "catch")
        parse_catchclause(pAst);

    if(peek(1).getToken() == "finally")
    {
        parse_finallyblock(pAst);
    }
}

void Parser::parse_throwstmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_throw_statement);

    expect_token(pAst, "throw", "`throw`");
    parse_expression(pAst);
    expect(SEMICOLON, pAst, "`;`");
}

void Parser::parse_labeldecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_label_decl);

    pushback();
    expectidentifier(pAst);
    expect(COLON, pAst, "`:` after label decliration");

    advance();
    parse_statement(pAst);

}

int commas=0;
void Parser::parse_statement(Ast* pAst) {
    pAst = get_ast(pAst, ast_statement);
    CHECK_ERRORS

    if(isreturn_stmnt(current()))
    {
        parse_returnstmnt(pAst);
    }
    else if(isif_stmnt(current()))
    {
        parse_ifstmnt(pAst);
    }
    else if(isassembly_stmnt(current()))
    {
        parse_assemblystmnt(pAst );
    }
    else if(isfor_stmnt(current()))
    {
        parse_forstmnt(pAst );
    }
    else if(isforeach_stmnt(current()))
    {
        parse_foreachstmnt(pAst );
    }
    else if(iswhile_stmnt(current()))
    {
        parse_whilestmnt(pAst );
    }
    else if(isdowhile_stmnt(current()))
    {
        parse_dowhilestmnt(pAst);
    }
    else if(istrycatch_stmnt(current()))
    {
        parse_trycatch(pAst);
    }
    else if(isthrow_stmnt(current()))
    {
        parse_throwstmnt(pAst);
    }
    else if(current().getToken() == "continue")
    {
        Ast* pAst2 = get_ast(pAst, ast_continue_statement);

        expect_token(pAst2, "continue", "`continue`");

        expect(SEMICOLON, pAst, "`;`");
    }
    else if(current().getToken() == "break")
    {
        Ast* pAst2 = get_ast(pAst, ast_break_statement);

        expect_token(pAst2, "break", "`break`");
        expect(SEMICOLON, pAst2, "`;`");
    }
    else if(current().getToken() == "goto")
    {
        Ast* pAst2 = get_ast(pAst, ast_goto_statement);

        expect_token(pAst2, "goto", "`goto`");

        expectidentifier(pAst2);
        // TODO: add support for calling goto labels[9];
        expect(SEMICOLON, pAst2, "`;`");
    }
    else if(isvariable_decl(current()))
    {
        parse_variabledecl(pAst);
    }
        /* these are just in case there is a missed bracket anywhere */
    else if(ismacros_decl(current()))
    {
        errors->createNewError(GENERIC, current(), "macros declaration not allowed here");
        parse_macrosdecl(pAst);
    }
    else if(ismodule_decl(current()))
    {
        errors->createNewError(GENERIC, current(), "module declaration not allowed here");
        parse_moduledecl(pAst);
    }
    else if(isclass_decl(current()))
    {
        errors->createNewError(GENERIC, current(), "unexpected class declaration");
        parse_classdecl(pAst);
    }
    else if(isimport_decl(current()))
    {
        errors->createNewError(GENERIC, current(), "import declaration not allowed here (why are you putting this here lol?)");
        parse_importdecl(pAst);
    }
    else if(current().getTokenType() == SEMICOLON)
    {

        /* we don't care about empty statements but we allow them */
        if(commas > 1) {
            commas = 0;
            errors->createNewWarning(GENERIC, current().getLine(), current().getColumn(), "unnecessary semicolon ';'");
        } else
            commas++;
    }
    else
    {
        // save parser state
        errors->enableErrorCheckMode();
        this->retainstate(pAst);
        pushback();

        /*
         * variable decl?
         */
        if(parse_utype(pAst))
        {
            errors->pass();
            if(peek(1).getTokenType() == COLON)
            {
                pAst = this->rollback();
                parse_labeldecl(pAst);
                return;
            }
            else if(peek(1).getId() == IDENTIFIER)
            {
                // Variable decliration
                pAst = this->rollback();
                parse_variabledecl(pAst);
                return;
            }
        } else
            errors->pass();

        pAst = this->rollback();
        pushback();

        errors->enableErrorCheckMode();
        this->retainstate(pAst);
        if(!parse_expression(pAst))
        {
            errors->pass();
            advance();
            errors->createNewError(GENERIC, current(), "not a statement");
            return;
        } else {
            errors->fail();
            this->dumpstate();

            if(peek(1).getTokenType() != SEMICOLON)
                errors->createNewError(GENERIC, current(), "expected `;`");
            else
                expect(SEMICOLON, pAst, "`;`");
        }
    }
}

void Parser::parse_modulename(Ast* pAst)
{
    pAst = get_ast(pAst, ast_modulename);

    if(peek(1).getToken() == "operator") {
        if(isoverride_operator(peek(2).getToken())) {
            advance();
            advance();
            pAst->addEntity(current());
            return;
        }
    } else {
        expectidentifier(pAst);
    }

    advance();
    while(current().getTokenType() == DOT) {
        if(isexprkeyword(peek(1).getToken()))
            break;

        pAst->addEntity(current());

        if(peek(1).getToken() == "operator") {
            if(isoverride_operator(peek(2).getToken())) {
                advance();
                advance();
                pAst->addEntity(current());
                advance();
                break;
            }
        } else {
            if(!expectidentifier(pAst));
        }
        advance();
    }

    pushback();
}

/**
 * This function is used to parse through every possible outcome that the parser can run into when it does not find a class or import
 * statement to process. This alleviates your console getting flooded with a bunch of unnessicary "unexpected symbol" complaints.
 *
 */
void Parser::parse_all(Ast *pAst) {

    if(isaccess_decl(current()))
    {
        parse_accesstypes();
    }
    CHECK_ERRORS

    if(current().getTokenType() == _EOF)
    {
        return;
    }
    else if(ismethod_decl(current()))
    {
        if(peek(1).getToken() == "operator")
            parse_operatordecl(pAst);
        else
            parse_methoddecl(pAst);
    }
    else if(ismodule_decl(current()))
    {
        if(access_types->size() > 0)
        {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parse_moduledecl(pAst);
    }
    else if(isclass_decl(current()))
    {
        parse_classdecl(pAst);
    }
    else if(isimport_decl(current()))
    {
        if(access_types->size() > 0)
        {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parse_importdecl(pAst);
    } else
        parse_statement(pAst);


}


bool Parser::iskeyword(string key) {
    return key == "mod" || key == "true"
           || key == "false" || key == "class"
           || key == "__int8" || key == "__int16"
           || key == "__int32" || key == "__int64"
           || key == "__uint8" || key == "__uint16"
           || key == "__uint32" || key == "__uint64"
           || key == "static" || key == "protected"
           || key == "private" || key == "fn"
           || key == "import" || key == "return"
           || key == "self" || key == "const"
           || key == "override" || key == "public" || key == "new"
           || key == "macros" || key == "null" || key == "operator"
           || key == "base" || key == "if" || key == "while" || key == "do"
           || key == "try" || key == "catch"
           || key == "finally" || key == "throw" || key == "continue"
           || key == "goto" || key == "break" || key == "else"
           || key == "dynamic_object" || key == "__asm" || key == "for" || key == "foreach"
           || key == "var" || key == "sizeof";
}

bool Parser::parse_type_identifier(Ast *pAst) {
    pAst = get_ast(pAst, ast_type_identifier);
    advance();

    if(!isnative_type(current().getToken())){
        pushback();
    }
    else {
        pAst->addEntity(current());
        return true;
    }

    errors->enableErrorCheckMode();
    if(!parse_reference_pointer(pAst)){
        errors->pass();
    }
    else {

        errors->fail();
        return true;
    }

    return false;
}

bool Parser::parse_reference_pointer(Ast *pAst) {
    pAst = get_ast(pAst, ast_refrence_pointer);

    advance();
    if(!(current().getId() == IDENTIFIER && !iskeyword(current().getToken()))) {
        if(current().getToken() != "operator")
            return false;
        else
            pushback();
    }
    else
        pushback();
    parse_modulename(pAst);

    /*
     * We want the full name to be on the ast
     */
    token_entity e;
    for(int64_t i = 0; i <  pAst->getSubAst(0)->getEntityCount(); i++)
    {
        pAst->addEntity(pAst->getSubAst(0)->getEntity(i));
    }

    pAst->freeSubAsts();

    advance();
    if(current().getTokenType() == HASH) {
        pAst->addEntity(current());

        if(expectidentifier(pAst))
            advance();

        while(current().getTokenType() == DOT ) {
            if(isexprkeyword(peek(1).getToken()))
                break;

            pAst->addEntity(current());

            if(peek(1).getToken() == "operator") {
                if(isoverride_operator(peek(2).getToken())) {
                    advance();
                    advance();
                    pAst->addEntity(current());
                    break;
                }
            } else {
                if(!expectidentifier(pAst)) break;
            }
            advance();
        }

        pushback();
    }
    else
        pushback();

    return true;
}

void Parser::free() {
    this->cursor = 0;
    this->toks = NULL;
    this->_current = NULL;

    if(this->tree != NULL) {
        /*
         * free ast tree
         */
        Ast* pAst;
        for(int64_t i = 0; i < this->tree->size(); i++)
        {
            pAst = &(*std::next(this->tree->begin(),
                                i));
            pAst->free();
        }

        ast_cursor = 0;
        remove_accesstypes();
        this->tree->clear();
        this->state->clear();
        delete (this->tree); this->tree = NULL;
        delete (this->state); this->state = NULL;
        delete (this->access_types); this->access_types = NULL;
        errors->free();
        delete (errors); this->errors = NULL;
    }
}

void Parser::retainstate(Ast* pAst) {
    state->push_back(ParserState(pAst, cursor, ast_cursor));
    rStateCursor++;
}

void Parser::dumpstate() {
    if(rStateCursor >= 0)
    {
        state->pop_back();
        rStateCursor--;
    }
}

Ast* Parser::rollbacklast() {
    if(rStateCursor >= 0)
    {
        ParserState* ps = &(*std::next(state->begin(),
                                        rStateCursor));
        Ast* pAst = ps->ast;
        ast_cursor = ps->astCursor;
        cursor = ps->cursor-1;
        advance();

        if(pAst->getSubAstCount() == 1) {
            ps->ast->freeSubAsts();
            ps->ast->freeEntities();
        }
        else {
            pAst->freeLastSub();
        }
        state->pop_back();
        rStateCursor--;
        return pAst;
    }

    return NULL;
}

Ast* Parser::rollback() {
    if(rStateCursor >= 0)
    {
        ParserState* ps = &(*std::next(state->begin(),
                                        rStateCursor));
        Ast* pAst = ps->ast;
        ast_cursor = ps->astCursor;
        cursor = ps->cursor-1;
        advance();

        ps->ast->freeSubAsts();
        ps->ast->freeEntities();
        state->pop_back();
        rStateCursor--;
        return pAst;
    }

    return NULL;
}

bool Parser::isexprkeyword(string token) {
    return (token == "class");
}

tokenizer *Parser::getTokenizer() const {
    return toks;
}

const string &Parser::getData() const {
    return data;
}