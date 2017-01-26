//
// Created by bknun on 1/7/2017.
//
#include "parser.h"
#include <sstream>

void parser::parse()
{
    if(toks->getentities() == 0)
        return;

    errors = new Errors(toks->getlines());
    _current= &(*std::next(toks->getentities()->begin(), cursor));

    while(!isend())
    {
        eval(NULL);
    }

    if(errors->_errs())
        return;

    for(auto &ast : *tree)
    {
        int i = 0; 
    }
}

Errors* parser::geterrors()
{
    return errors;
}

bool parser::isend() {
    return current().gettokentype() == _EOF;
}

void parser::eval(ast* _ast) {
    if(isaccess_decl(current()))
    {
        parse_accesstypes();
    }

    if(isend())
    {
        remove_accesstypes();
        return;
    }
    else if(ismodule_decl(current()))
    {
        if(access_types->size() > 0)
        {
            errors->newerror(ILLEGAL_ACCESS_DECLARATION, current());
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
            errors->newerror(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parse_importdecl(_ast);
    }
    else
    {
        // "expected class, or import declaration"
        errors->newerror(UNEXPECTED_SYMBOL, current(), " `" + current().gettoken() + "`; expected class, or import declaration");
        parse_all(_ast);
    }

    advance();
    remove_accesstypes();
}

void parser::parse_classdecl(ast* _ast) {
    _ast = get_ast(_ast, ast_class_decl);

    for(token_entity &entity : *access_types)
    {
        _ast->add_entity(entity);
    }
    _ast->add_entity(current());


    expectidentifier(_ast);

    parse_classblock(_ast);
}

void parser::parse_importdecl(ast* _ast) {
    _ast = get_ast(_ast, ast_import_decl);
    _ast->add_entity(current());

    parse_modulename(_ast);

    if(!expect(SEMICOLON, "`;`"))
    {
        pushback();
        return;
    }

    cout << "parsed import declaration" << endl;
}

void parser::parse_moduledecl(ast* _ast) {
    _ast = get_ast(_ast, ast_module_decl);
    _ast->add_entity(current());

    parse_modulename(_ast);

     if(!expect(SEMICOLON, "`;`"))
         return;

     cout << "parsed module declaration" << endl;
}

token_entity parser::current()
{
    return *_current;
}

void parser::advance()
{
    if((cursor+1)>=toks->getentitycount())
        *_current =*toks->EOF_token;
    else
        _current = &(*std::next(toks->getentities()->begin()
                , ++cursor));
}

token_entity parser::peek(int forward)
{

    if(cursor+forward >= toks->getentities()->size())
        return *std::next(toks->getentities()->begin(), toks->getentitycount()-1);
    else
        return *std::next(toks->getentities()->begin(), cursor+forward);
}

bool parser::isvariable_decl(token_entity token) {
    return isnative_type(token.gettoken());
}

bool parser::ismethod_decl(token_entity token) {
    return token.getid() == IDENTIFIER && token.gettoken() == "function";
}

bool parser::isstatement_decl(token_entity token) {
    return
            (token.getid() == IDENTIFIER && token.gettoken() == "return") // return statement
            /*
             * Variable assignment
             * x = 9;
             *
             * method invocation
             * examples#Main.func(9.1e+10, Class.x)
             */
            || (token.getid() == IDENTIFIER && !iskeyword(token.gettoken()))
            || (token.getid() == IDENTIFIER && isnative_type(token.gettoken()))

            || (token.gettokentype() == SEMICOLON); // for empty statements
}

ast* parser::ast_at(long p)
{
    return &(*std::next(tree->begin(), p));
}

bool parser::ismodule_decl(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "mod";
}

bool parser::isclass_decl(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "class";
}

bool parser::isimport_decl(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "import";
}

bool parser::isreturn_stmnt(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "return";
}

bool parser::issemicolon(token_entity entity) {
    return entity.gettokentype() == SEMICOLON;
}

bool parser::isnative_type(string type) {
    return type == "int" || type == "short"
            || type == "long" || type == "bool"
            || type == "char" || type == "float"
            || type == "double";
}

bool parser::isaccess_decl(token_entity token) {
    return
            token.getid() == IDENTIFIER && token.gettoken() == "protected" ||
                    token.getid() == IDENTIFIER && token.gettoken() == "private" ||
                    token.getid() == IDENTIFIER && token.gettoken() == "static" ||
                    token.getid() == IDENTIFIER && token.gettoken() == "const" ||
                    token.getid() == IDENTIFIER && token.gettoken() == "override" ||
                    token.getid() == IDENTIFIER && token.gettoken() == "public";
}

void parser::parse_accesstypes() {
    remove_accesstypes();

    while(isaccess_decl(current()))
    {
        access_types->push_back(current());
        advance();
    }
}

void parser::remove_accesstypes() {
    if(access_types->size() > 0)
        access_types->clear();
}

void parser::expect(token_type ty, ast* pAst, const char *expectedstr) {
    advance();

    if(current().gettokentype() == ty)
    {
        if(pAst != NULL)
            pAst->add_entity(current());
    }
    else {
        errors->newerror(GENERIC, current(), "expected " + string(expectedstr));
    }
}


bool parser::expectidentifier(ast* pAst) {
    advance();

    if(current().getid() == IDENTIFIER && !iskeyword(current().gettoken()))
    {
        if(pAst != NULL)
            pAst->add_entity(current());
        return true;
    }
    else {
        errors->newerror(GENERIC, current(), "expected identifier");
    }
    return false;
}


void parser::expect_token(ast *pAst, string token, const char *message) {
    if(current().gettoken() != token)
    {
        errors->newerror(GENERIC, current(), "expected " + std::string(message));
    } else {
        pAst->add_entity(current());
    }
}

bool parser::expect(token_type type, const char *expectedstr) {
    advance();

    if(current().gettokentype() == type)
    {
        return true;
    }
    else {
        errors->newerror(GENERIC, current(), "expected " + string(expectedstr));
    }
    return false;
}

void parser::parse_classblock(ast *pAst) {
    expect(LEFTCURLY, "`{` after class declaration");
    pAst = get_ast(pAst, ast_block);

    int brackets = 1;

    while(!isend() && brackets > 0)
    {
        advance();
        if(isaccess_decl(current()))
        {
            parse_accesstypes();
        }

        if(ismodule_decl(current()))
        {
            if(access_types->size() > 0)
            {
                errors->newerror(ILLEGAL_ACCESS_DECLARATION, current());
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
                errors->newerror(ILLEGAL_ACCESS_DECLARATION, current());
            }
            parse_importdecl(pAst);
        }
        else if(isvariable_decl(current()))
        {
           parse_variabledecl(pAst);
        }
        else if(ismethod_decl(current()))
        {
            parse_methoddecl(pAst);
        }
        else if(current().gettokentype() == _EOF)
        {
            errors->newerror(UNEXPECTED_EOF, current());
            break;
        }
        else if (current().gettokentype() == RIGHTCURLY)
        {
            if((brackets-1) < 0)
            {
                errors->newerror(ILLEGAL_BRACKET_MISMATCH, current());
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
        else if(current().gettokentype() == LEFTCURLY)
            brackets++;
        else {
            errors->newerror(GENERIC, current(), "expected method, class, or variable declaration");
            parse_all(pAst);
        }

        remove_accesstypes();
    }

    if(brackets != 0)
        errors->newerror(MISSING_BRACKET, current(), " expected `}` at end of class declaration");

    expect(RIGHTCURLY, "`}` at end of class declaration");
}

/**
 * Gets the current ast based on context of the program. If we are at the highest level,
 * the highests level ast is returned otherwise we get back a newly created ast branch in the tree
 * @param pAst
 * @return
 */
ast * parser::get_ast(ast *pAst, ast_types typ) {
    if(pAst == NULL)
    {
        tree->push_back(ast(NULL, typ));
        ast_cursor++;

        return ast_at(ast_cursor);
    }
    else {
        pAst->add_ast(ast(pAst, typ));

        return pAst->getsubast(pAst->getsubastcount() - 1);
    }
}

void parser::pushback() {
    _current = &(*std::next(toks->getentities()->begin(), cursor-1));
    cursor--;
}

void parser::parse_variabledecl(ast *pAst) {
    pAst = get_ast(pAst, ast_var_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->add_entity(entity);
    }
    pushback();
    if(!parse_type_identifier(pAst))
        errors->newerror(GENERIC, current(), "expected native type or reference pointer.");
    expectidentifier(pAst);

    parse_valueassignment(pAst);

    if(!expect(SEMICOLON, "`;`"))
        return;

    cout << "parsed variable declaration" << endl;
}

void parser::parse_valueassignment(ast *pAst) {
    advance();
    if(isassiment_decl(current()))
    {
        pAst->add_entity(current());
        parse_value(pAst);
    }
    else
        pushback();
}

bool parser::isassiment_decl(token_entity token) {
    return token.gettokentype() == ASSIGN && token.gettoken() == "=";
}

bool parser::parse_literal(ast *pAst) {
    token_entity e = peek(1);
    if(e.getid() == CHAR_LITERAL || e.getid() == INTEGER_LITERAL
       || e.getid() == STRING_LITERAL || e.getid() == HEX_LITERAL
       || e.gettoken() == "true" || e.gettoken() == "false")
    {
        advance();
        pAst->add_entity(current());
        return true;
    }
    else {
        errors->newerror(GENERIC, current(), "expected literal");
        return false;
    }
}

bool parser::parse_utype(ast *pAst) {
    pAst = get_ast(pAst, ast_utype);

    if(parse_type_identifier(pAst))
    {
        if(peek(1).gettokentype() == LEFTBRACE)
        {
            pAst->add_entity(current());
            advance();

            expect(RIGHTBRACE, pAst, "`]`");
        }

        return true;
    }
    else
        errors->newerror(GENERIC, current(), "expected native type or reference pointer.");

    return false;
}

bool parser::parse_primaryexpr(ast *pAst) {
    pAst = get_ast(pAst, ast_primary_expr);

    errors->enablecheck_mode();
    if(parse_literal(pAst))
    {
        errors->fail();
        return true;
    }
    errors->pass();

    errors->enablecheck_mode();
    this->retainstate(pAst);
    if(parse_utype(pAst))
    {
        if(peek(1).gettokentype() == DOT)
        {
            expect(DOT, pAst->getsubast(0), "`.`");
            advance();

            expect_token(pAst->getsubast(0), "class", "`class` after primary expression");
            errors->fail();
            return true;
        }else
            pAst = this->rollback();
    } else
        pAst = this->rollback();
    errors->pass();

    if(peek(1).gettoken() == "void")
    {
        advance();
        pAst->add_entity(current());

        expect(DOT, pAst, "`.`");
        advance();

        expect_token(pAst, "class", "`class` after primary expression");
        return true;
    }

    errors->enablecheck_mode();
    this->retainstate(pAst);
    if(parse_reference_pointer(pAst)) {
        if(peek(1).gettokentype() == PTR)
        {
            this->rollback();
            errors->pass();
            return false;
        }
        errors->fail();
        return true;
    }
    else {
        pushback();
        errors->pass();
    }

    return false;
}

void parser::parse_typeargs(ast *pAst) {
    pAst = get_ast(pAst, ast_type_arg);

    expect(LESSTHAN, pAst, "`<`");

    if(peek(1).gettokentype() != GREATERTHAN)
    {
        parse_utype(pAst);

        _pUtype:
            if(peek(1).gettokentype() == COMMA)
            {
                expect(COMMA, pAst, "`,`");
                parse_utype(pAst);
                goto _pUtype;
            }
    }

    expect(GREATERTHAN, pAst, "`>`");
}

void parser::parse_valuelist(ast *pAst) {
    pAst = get_ast(pAst, ast_value_list);

    expect(LEFTPAREN, pAst, "`(`");

    if(peek(1).gettokentype() != RIGHTPAREN)
    {
        parse_value(pAst);

        _pValue:
        if(peek(1).gettokentype() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_value(pAst);
            goto _pValue;
        }
    }

    expect(RIGHTPAREN, pAst, "`)`");
}

bool parser::isexprsymbol(string token) {
    return token == "[" || token == "++" ||
            token == "--" || token == "*" ||
            token == "/" || token == "%" ||
            token == "-" || token == "+";
}

bool parser::parse_expression(ast *pAst) {
    pAst = get_ast(pAst, ast_expression);

    /* ++ or -- or - or + before the expression */
    if(peek(1).gettokentype() == INC || peek(1).gettokentype() == DEC
            || peek(1).gettokentype() == PLUS || peek(1).gettokentype() == MINUS)
    {
        advance();
        pAst->add_entity(current());
        parse_expression(pAst);
        return true;
    }


    if(peek(1).gettokentype() == LEFTPAREN)
    {
        this->retainstate(pAst);
        errors->enablecheck_mode();

        advance();
        pAst->add_entity(current());

        if(!parse_utype(pAst))
        {
            errors->pass();
            this->rollback();
        } else {
            if(peek(1).gettokentype() == RIGHTPAREN)
            {
                expect(RIGHTPAREN, pAst, "`)`");
                if(!parse_expression(pAst))
                {
                    errors->pass();
                    this->rollback();
                } else
                {
                    errors->fail();
                    return true;
                }
            }else {
                errors->pass();
                this->rollback();
            }
        }

    }


    if(peek(1).gettokentype() == NOT)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);

        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }

    if(peek(1).gettokentype() == LEFTPAREN)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);

        expect(RIGHTPAREN, pAst, "`)`");

        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }

    this->retainstate(pAst);
    if(parse_primaryexpr(pAst)) {
        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }
    else {
        this->rollback();
    }

    if(peek(1).gettoken() == "this")
    {
        advance();
        expect_token(pAst, "this", "");
        expect(PTR, pAst, "`->` after this");
        parse_expression(pAst);

        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }

    if(peek(1).gettoken() == "new")
    {
        advance();
        expect_token(pAst, "new", "");
        parse_utype(pAst);

        if(peek(1).gettokentype() == LESSTHAN)
        {
            parse_typeargs(pAst);
        }

        parse_valuelist(pAst);

        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }

    if(peek(1).gettokentype() == LEFTBRACE)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        expect(RIGHTBRACE, pAst, "`]`");

        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }

    /* ++ or -- after the expression */
    if(peek(1).gettokentype() == INC || peek(1).gettokentype() == DEC)
    {
        advance();
        pAst->add_entity(current());

        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }

    /* expression ('*'|'/'|'%') expression */
    if(peek(1).gettokentype() == MULT || peek(1).gettokentype() == DIV ||
            peek(1).gettokentype() == MOD)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        return true;
    }

    /* expression ('+'|'-') expression */
    if(peek(1).gettokentype() == PLUS || peek(1).gettokentype() == MINUS)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        return true;
    }

    errors->newerror(GENERIC, current(), "expected expression");
    return false;
}

bool parser::parse_value(ast *pAst) {
    pAst = get_ast(pAst, ast_value);
    return parse_expression(pAst);
}

void parser::parse_methoddecl(ast *pAst) {
    pAst = get_ast(pAst, ast_method_inv);

    for(token_entity &entity : *access_types)
    {
        pAst->add_entity(entity);
    }
    pAst->add_entity(current());

    expectidentifier(pAst);

    parse_methodparams(pAst);

    parse_methodreturn_type(pAst);
    parse_methodblock(pAst);

}

void parser::parse_methodparams(ast* pAst) {
    pAst = get_ast(pAst, ast_method_params);

    expect(LEFTPAREN, pAst, "`(`");

    int brackets = 1, errs = 3;
    bool comma = false, first = true;

    while (!isend() && brackets > 0)
    {
        advance();
        if(current().getid() == IDENTIFIER)
        {
            if(!comma && !first)
                errors->newerror(GENERIC, current(), "expected `,`");
            else
                first = false;

            pushback();
            if(!parse_type_identifier(pAst))
                errors->newerror(GENERIC, current(), "expected native type or reference pointer.");
            if(!expectidentifier(pAst))
            {
                pushback();
            }

            comma = false;
        }
        else if(current().gettokentype() == COMMA)
        {
            pAst->add_entity(current());

            if(comma)
            {
                errors->newerror(GENERIC, current(), "expected identifier before `,`");
            }

            comma = true;
        }
        else if(current().gettokentype() == LEFTPAREN)
        {
            errors->newerror(UNEXPECTED_SYMBOL, current(), "`" + current().gettoken() + "`" + " expected params");
        }
        else if(current().gettokentype() == RIGHTPAREN)
        {
            if((brackets-1) < 0)
            {
                errors->newerror(UNEXPECTED_SYMBOL, current(), " `)`");
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
        else {
            errs -= errors->newerror(UNEXPECTED_SYMBOL, current(), "`" + current().gettoken() + "`" + " expected identifier");

            if(errs < 0)
                break;
        }

        remove_accesstypes();
    }


    if(comma)
        errors->newerror(GENERIC, current(), "expected identifier after `,`");

    if(brackets != 0)
        errors->newerror(GENERIC, current(), "expected `)` at end of method args");

    expect(RIGHTPAREN, pAst, "`)`");
    cout << "parsed method declaration" << endl;
}

void parser::parse_methodblock(ast *pAst) {
    expect(LEFTCURLY, "`{` after method declaration");
    pAst = get_ast(pAst, ast_block);
    ast *ref = pAst;

    int brackets = 1;

    while(!isend() && brackets > 0)
    {
        advance();
        if(isstatement_decl(current()))
        {
            parse_statement(pAst);
        }
        else if (current().gettokentype() == RIGHTCURLY)
        {
            pAst = pAst->getparent() == NULL ? ref : pAst->getparent();
            if((brackets-1) < 0)
            {
                errors->newerror(ILLEGAL_BRACKET_MISMATCH, current());
            }
            else
            {
                brackets--;

                // end of method block
                if(brackets == 0)
                {
                    pushback();
                    break;
                }
            }
        }
        else if(current().gettokentype() == LEFTCURLY)
        {
            pAst = get_ast(pAst, ast_block);
            brackets++;
        }
        else if(current().gettokentype() == _EOF)
        {
            errors->newerror(UNEXPECTED_EOF, current());
            break;
        }
        else {
            errors->newerror(UNEXPECTED_SYMBOL, current(), " expected statement");
            parse_all(pAst);
        }

        remove_accesstypes();
    }

    if(brackets != 0)
        errors->newerror(MISSING_BRACKET, current(), " expected `}` at end of method declaration");

    expect(RIGHTCURLY, "`}` at end of method declaration");
}

void parser::parse_methodreturn_type(ast *pAst) {
    if(peek(1).gettokentype() == COLON)
    {
        pAst = get_ast(pAst, ast_method_return_type);
        advance();

        pAst->add_entity(current());
        if(!parse_type_identifier(pAst))
            errors->newerror(GENERIC, current(), "expected native type or reference pointer.");
    }
}

void parser::parse_returnstmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_return_stmnt);

    pAst->add_entity(current());
    parse_value(pAst);
}

void parser::parse_variable_assignmentstmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_var_assign_stmnt);
    parse_valueassignment(pAst);
}

void parser::parse_method_invocation(ast *pAst) {
    pAst = get_ast(pAst, ast_method_inv_params);

    expect(LEFTPAREN, pAst, "`(`");

    int brackets = 1, errs = 1;
    bool comma = false, first = true;

    while (!isend() && brackets > 0)
    {
        advance();
        if(current().getid() == IDENTIFIER)
        {
            if(!comma && !first)
                errors->newerror(GENERIC, current(), "expected `,`");
            else
                first = false;

            this->retainstate(pAst);
            pushback();
            errors->enablecheck_mode();
            if(parse_reference_pointer(pAst))
            {
                errors->fail();
                if(peek(1).gettokentype() == LEFTPAREN)
                {
                    // method invocation
                    parse_method_invocation(pAst->getsubast(0));
                    comma = false;
                    continue;
                }
                else {
                    this->cursor = (*std::next(rState->begin(),
                                               rStateCursor)).rCursor-1;
                    this->ast_cursor = (*std::next(rState->begin(),
                                                   rStateCursor)).rAstcursor;
                    pAst->freelastsub();
                    advance();
                }
            }
            errors->pass();
        }

        if(current().gettokentype() == COMMA)
        {
            pAst->add_entity(current());

            if(comma)
            {
                errors->newerror(GENERIC, current(), "expected identifier before `,`");
            }

            comma = true;
        }
            /* remember to remove these when I start processing expressions */
        else if(current().gettokentype() == LEFTPAREN)
        {
            errors->newerror(UNEXPECTED_SYMBOL, current(), "`" + current().gettoken() + "`" + " expected params");
        }
        else if(current().gettokentype() == RIGHTPAREN)
        {
            if((brackets-1) < 0)
            {
                errors->newerror(UNEXPECTED_SYMBOL, current(), " `)`");
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
        else
        {
            if(!comma && !first)
                errors->newerror(GENERIC, current(), "expected `,`");
            else
                first = false;

            pushback();
            if (!parse_value(pAst))
            {
                advance();
                errs--;
            }

            if(errs < 0)
                break;

            comma = false;
        }
    }


    if(comma)
        errors->newerror(GENERIC, current(), "expected identifier after `,`");

    if(brackets != 0)
        errors->newerror(GENERIC, current(), "expected `)` at end of method args");

    expect(RIGHTPAREN, pAst, "`)`");
    cout << "parsed method invocation params" << endl;
}

void parser::parse_statement(ast* pAst) {
    pAst = get_ast(pAst, ast_statement);

    if(isreturn_stmnt(current()))
    {
        parse_returnstmnt(pAst);
    }
    else if(isvariable_decl(current()))
    {
        parse_variabledecl(pAst);
    }
    else if(current().gettokentype() == SEMICOLON)
    {
        /* we don't care about empty statements but we allow them */
    }
    else if(current().getid() == IDENTIFIER && !iskeyword(current().gettoken()))
    {
        // save compiler state
        this->retainstate(pAst);
        pushback();

        /*
         * Method invocation? or variable assignment/decl?
         */
        if(parse_reference_pointer(pAst))
        {
            if(peek(1).gettokentype() == ASSIGN)
            {
                // Variable declaration
                parse_variable_assignmentstmnt(pAst->getsubast(0));
            }
            else if(peek(1).gettokentype() == LEFTPAREN)
            {
                // method invocation
                parse_method_invocation(pAst->getsubast(0));
                expect(SEMICOLON, "`;`");
            }
            else if(peek(1).getid() == IDENTIFIER)
            {
                // Variable decliration
                pAst = this->rollback();
                parse_variabledecl(pAst);
            }
            else {
                errors->newerror(GENERIC, current(), "not a statement");
            }
        }
    }
    else {
        errors->newerror(GENERIC, current(), "not a statement");
    }
}

void parser::parse_modulename(ast* pAst)
{
    pAst = get_ast(pAst, ast_modulename);

    expectidentifier(pAst);

    advance();
    while(current().gettokentype() == DOT) {
        if(isexprkeyword(peek(1).gettoken()))
            break;

        pAst->add_entity(current());

        expectidentifier(pAst);
        advance();
    }

    pushback();
}

/**
 * This function is used to parse through every possible outcome that the parser can run into when it does not find a class or import
 * statement to process. This alleviates your console getting flooded with a bunch of unnessicary "unexpected symbol" complaints.
 *
 */
void parser::parse_all(ast *pAst) {

    if(isaccess_decl(current()))
    {
        parse_accesstypes();
    }

    if(isstatement_decl(current()))
    {
        parse_statement(pAst);
    }
    else if(ismethod_decl(current()))
    {
        parse_methoddecl(pAst);
    }
    else if(ismodule_decl(current()))
    {
        if(access_types->size() > 0)
        {
            errors->newerror(ILLEGAL_ACCESS_DECLARATION, current());
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
            errors->newerror(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parse_importdecl(pAst);
    }
}


bool parser::iskeyword(string key) {
    return key == "mod" || key == "true"
           || key == "false" || key == "class"
           || key == "int" || key == "short"
           || key == "long" || key == "char"
           || key == "bool" || key == "float"
           || key == "double" || key == "static"
           || key == "protected" || key == "private"
           || key == "function" || key == "import"
           || key == "return" || key == "this"
           || key == "const" || key == "override"
           || key == "public" || key == "new";
}

bool parser::parse_type_identifier(ast *pAst) {
    pAst = get_ast(pAst, ast_type_identifier);
    advance();

    if(!isnative_type(current().gettoken())){
        pushback();
    }
    else {
        pAst->add_entity(current());
        return true;
    }

    errors->enablecheck_mode();
    if(!parse_reference_pointer(pAst)){}
    else {

        errors->pass();
        return true;
    }

    errors->pass();
    return false;
}

bool parser::parse_reference_pointer(ast *pAst) {
    pAst = get_ast(pAst, ast_refrence_pointer);

    advance();
    if(!(current().getid() == IDENTIFIER && !iskeyword(current().gettoken())))
        return false;
    else
        pushback();
    parse_modulename(pAst);

    /*
     * We want the full name to be on the ast
     */
    token_entity e;
    for(int64_t i = 0; i <  pAst->getsubast(0)->getentitycount(); i++)
    {
        pAst->add_entity(pAst->getsubast(0)->getentity(i));
    }

    pAst->freesubs();

    advance();
    if(current().gettokentype() == HASH) {
        pAst->add_entity(current());

        if(expectidentifier(pAst))
            advance();

        while(current().gettokentype() == DOT) {
            if(isexprkeyword(peek(1).gettoken()))
                break;

            pAst->add_entity(current());

            expectidentifier(pAst);
            advance();
        }

        pushback();
    }
    else
        pushback();

    return true;
}

void parser::free() {
    this->cursor = 0;
    this->toks = NULL;

    if(this->tree != NULL) {
        /*
         * free ast tree
         */
        ast* pAst;
        for(int64_t i = 0; i < this->tree->size(); i++)
        {
            pAst = &(*std::next(this->tree->begin(),
                                i));
            pAst->free();
        }

        ast_cursor = 0;
        remove_accesstypes();
        this->tree->clear();
        this->rState->clear();
        std::free(this->tree); this->tree = NULL;
        std::free(this->rState); this->rState = NULL;
        std::free(this->_current); this->_current = NULL;
        std::free(this->access_types); this->access_types = NULL;
        errors->free();
    }
}

void parser::retainstate(ast* pAst) {
    rState->push_back(parser_state(pAst, cursor, ast_cursor));
    rStateCursor++;
}

ast* parser::rollback() {
    if(rStateCursor >= 0)
    {
        parser_state* ps = &(*std::next(rState->begin(),
                                        rStateCursor));
        ast* pAst = ps->rAst;
        ast_cursor = ps->rAstcursor;
        cursor = ps->rCursor-1;
        advance();

        ps->rAst->freesubs();
        ps->rAst->freeentities();
        rState->pop_back();
        rStateCursor--;
        return pAst;
    }

    return NULL;
}

bool parser::isexprkeyword(string token) {
    return (token == "class");
}
