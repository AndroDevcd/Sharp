//
// Created by bknun on 1/7/2017.
//
#include "parser.h"
#include <sstream>

void parser::parse()
{
    if(toks->getentities() == 0)
        return;

    parsed = true;
    errors = new Errors(toks->getlines(), toks->file);
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
    else if(isextern_stmnt(current()))
    {
        parse_externstmnt(_ast);
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


void parser::parse_type_declarators(ast *pAst) {
    pAst = get_ast(pAst, ast_type_arg);

    expect(LESSTHAN, pAst, "`<`");

    expectidentifier(pAst);

    _pIdentifier:
    if(peek(1).gettokentype() == COMMA)
    {
        expect(COMMA, pAst, "`,`");
        expectidentifier(pAst);
        goto _pIdentifier;
    }

    expect(GREATERTHAN, pAst, "`>`");
}

void parser::parse_classdecl(ast* _ast) {
    _ast = get_ast(_ast, ast_class_decl);

    for(token_entity &entity : *access_types)
    {
        _ast->add_entity(entity);
    }
    _ast->add_entity(current());


    expectidentifier(_ast);
    if(peek(1).gettokentype() == LESSTHAN)
        parse_type_declarators(_ast);

    if(peek(1).gettoken() == "base")
    {
        advance();
        expect_token(_ast, "base", "");

        parse_reference_pointer(_ast);

        if(peek(1).gettokentype() == LESSTHAN)
            parse_typeargs(_ast);
    }

    parse_classblock(_ast);
}

void parser::parse_importdecl(ast* _ast) {
    _ast = get_ast(_ast, ast_import_decl);
    _ast->add_entity(current());

    parse_modulename(_ast);

    expect(SEMICOLON, "`;`");

    //cout << "parsed import declaration" << endl;
}

void parser::parse_moduledecl(ast* _ast) {
    _ast = get_ast(_ast, ast_module_decl);
    _ast->add_entity(current());

    parse_modulename(_ast);

    if(!expect(SEMICOLON, "`;`"))
        return;

    //cout << "parsed module declaration" << endl;
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

bool parser::ismacros_decl(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "macros";
}

bool parser::isif_stmnt(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "if";
}

bool parser::iswhile_stmnt(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "while";
}

bool parser::isdowhile_stmnt(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "do";
}

bool parser::istrycatch_stmnt(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "try";
}

bool parser::isthrow_stmnt(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "throw";
}

bool parser::isextern_stmnt(token_entity entity) {
    return entity.getid() == IDENTIFIER && entity.gettoken() == "extern";
}

bool parser::isconstructor_decl() {
    return current().getid() == IDENTIFIER && !iskeyword(current().gettoken()) &&
           peek(1).gettokentype() == LEFTPAREN;
}

bool parser::isnative_type(string type) {
    return type == "int" || type == "short"
           || type == "long" || type == "bool"
           || type == "char" || type == "float"
           || type == "double" || type == "string"
           /*
            * This is not a native type but we want this to be
            * able to be set as a variable
            */
           || type == "dynamic_object"
            ;
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
        else if(isextern_stmnt(current()))
        {
            parse_externstmnt(pAst);
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
            if(peek(1).gettoken() == "operator")
                parse_operatordecl(pAst);
            else
                parse_methoddecl(pAst);
        }
        else if(isconstructor_decl())
        {
            parse_constructor(pAst);
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
            // save parser state
            this->retainstate(pAst);
            pushback();

            /*
             * variable decl?
             */
            if(parse_utype(pAst))
            {
                if(peek(1).getid() == IDENTIFIER)
                {
                    // Variable decliration
                    pAst = this->rollbacklast();
                    parse_variabledecl(pAst);
                    remove_accesstypes();
                    continue;
                }
            }

            pAst = this->rollback();
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
        tree->push_back(ast(NULL, typ, current().getline(), current().getcolumn()));
        ast_cursor++;

        return ast_at(ast_cursor);
    }
    else {
        pAst->add_ast(ast(pAst, typ, current().getline(), current().getcolumn()));

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
    if(!parse_utype(pAst))
        errors->newerror(GENERIC, current(), "expected native type or reference pointer");
    parse_memaccess_flag(pAst);
    expectidentifier(pAst);

    parse_valueassignment(pAst);

    if(!expect(SEMICOLON, "`;`"))
        return;

    //cout << "parsed variable declaration" << endl;
}

void parser::parse_valueassignment(ast *pAst) {
    advance();
    if(isassign_exprsymbol(current().gettoken()))
    {
        pAst->add_entity(current());
        parse_value(pAst);
    }
    else
        pushback();
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
        if(peek(1).gettokentype() == LESSTHAN)
        {
            if(!parse_typeargs(pAst))
                return false;
        }

        if(peek(1).gettokentype() == LEFTBRACE)
        {
            pAst->add_entity(current());
            advance();

            expect(RIGHTBRACE, pAst, "`]`");
        }

        parse_memaccess_flag(pAst);
        return true;
    }
    else
        errors->newerror(GENERIC, current(), "expected native type or reference pointer");

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
    if(peek(1).gettokentype() == DOT) {
        advance();
        pAst->add_entity(current());
    }

    this->retainstate(pAst);
    if(parse_utype(pAst))
    {
        if(peek(1).gettokentype() == DOT)
        {
            expect(DOT, pAst->getsubast(0), "`.`");
            advance();

            expect_token(pAst->getsubast(0), "class", "`class` after primary expression");

            this->dumpstate();
            errors->fail();
            return true;
        }else
            pAst = this->rollback();
    } else
        pAst = this->rollback();
    errors->pass();

    errors->enablecheck_mode();
    if(peek(1).gettokentype() == DOT) {
        advance();
        pAst->add_entity(current());
    }

    if(parse_reference_pointer(pAst)) {
        if(peek(1).gettokentype() == LEFTPAREN)
        {
            parse_valuelist(pAst);
            errors->fail();

            /* func()++ or func()--
             * This expression rule dosen't process correctly by itsself
             * so we hav to do it ourselves
             */
            if(peek(1).gettokentype() == INC || peek(1).gettokentype() == DEC)
            {
                advance();
                pAst->add_entity(current());
            }
            else {
                errors->enablecheck_mode();
                this->retainstate(pAst);
                if(!parse_expression(pAst))
                {
                    errors->pass();
                    this->rollback();
                } else {
                    this->dumpstate();
                    errors->fail();
                }
            }

            return true;
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

bool skipbracket = false;
bool parser::parse_typeargs(ast *pAst) {
    pAst = get_ast(pAst, ast_type_arg);

    expect(LESSTHAN, pAst, "`<`");

    if(peek(1).gettokentype() != GREATERTHAN)
    {
        if(!parse_utype(pAst))
            return false;

        _pUtype:
        if(peek(1).gettokentype() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_utype(pAst);
            goto _pUtype;
        }
    }

    if(peek(1).gettokentype() == SHR)
    {
        advance();
        skipbracket = true;
    }
    else if(skipbracket){
        skipbracket = false; }
    else
        expect(GREATERTHAN, pAst, "`>`");

    return true;
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

bool parser::isassign_exprsymbol(string token) {
    return token == "+=" || token == "-="||
           token == "*=" || token == "/="||
           token == "&=" || token == "|="||
           token == "^=" || token == "%="||
           token == "=";
}

bool parser::isexprsymbol(string token) {
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

bool parser::isoverride_operator(string token) {
    return isassign_exprsymbol(token) ||
            token == "++" ||token == "--" ||
            token == "*" || token == "/" ||
            token == "%" || token == "-" ||
            token == "+" || token == "=";
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
                    this->dumpstate();
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

    if(peek(1).gettokentype() == LEFTCURLY)
    {
        parse_vectorarray(pAst);
        return true;
    }

    this->retainstate(pAst);
    if(parse_primaryexpr(pAst)) {
        this->dumpstate();
        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }
    else {
        this->rollback();
    }

    if(peek(1).gettoken() == "self")
    {
        advance();
        expect_token(pAst, "self", "");
        expect(PTR, pAst, "`->` after self");
        parse_expression(pAst);

        if(!isexprsymbol(peek(1).gettoken()))
            return true;
    }

    if(peek(1).gettoken() == "null")
    {
        advance();
        expect_token(pAst, "null", "");
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

        if(peek(1).gettokentype() == LEFTCURLY)
            parse_vectorarray(pAst);
        else
            parse_valuelist(pAst);

        return true;
    }

    if(peek(1).gettokentype() == DOT)
    {
        advance();
        pAst->add_entity(current());

        advance();
        expect_token(pAst, "class", "`class`");
        return true;
    }

    if(peek(1).gettokentype() == LEFTBRACE)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        expect(RIGHTBRACE, pAst, "`]`");



        if(!isexprsymbol(peek(1).gettoken())){
            errors->enablecheck_mode();
            this->retainstate(pAst);
            if(!parse_expression(pAst)) {
                this->rollback();
                errors->pass();
            }
            else {
                errors->fail();
                this->dumpstate();
            }

            return true;
        }
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

    /* expression ('<<'|'>>') expression */
    if(peek(1).gettokentype() == SHL || peek(1).gettokentype() == SHR)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        return true;
    }

    /* expression ('<=' | '>=' | '>' | '<') expression */
    if(peek(1).gettokentype() == LESSTHAN || peek(1).gettokentype() == GREATERTHAN ||
       peek(1).gettokentype() == GTE || peek(1).gettokentype() == LTE)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        return true;
    }

    /* expression ('!=' | '==' | '=') expression */
    if(peek(1).gettokentype() == EQEQ || peek(1).gettokentype() == NOTEQ ||
       peek(1).gettokentype() == ASSIGN)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        return true;
    }

    /* expression ('&' | '^' | '|' | '&&' | '||') expression */
    if(peek(1).gettokentype() == AND || peek(1).gettokentype() == OR ||
       peek(1).gettokentype() == ANDAND || peek(1).gettokentype() == OROR ||
       peek(1).gettokentype() == XOR)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);
        return true;
    }

    /* expression '?' expression ':' expression */
    if(peek(1).gettokentype() == QUESMK)
    {
        advance();
        pAst->add_entity(current());

        parse_expression(pAst);

        expect(COLON, pAst, "`:`");

        parse_expression(pAst);
        return true;
    }

    /* expression <assign-expr> expression */
    if(isassign_exprsymbol(peek(1).gettoken()))
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

bool parser::parse_utypearg(ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg);

    if(parse_utype(pAst))
    {
        expectidentifier(pAst);
        return true;
    }else
        errors->newerror(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

bool parser::parse_utypearg_opt(ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg_opt);

    if(parse_utype(pAst))
    {
        errors->enablecheck_mode();
        if(!expectidentifier(pAst)) {
            errors->pass();
            pushback();
        }
        else
            errors->fail();
        return true;
    }else
        errors->newerror(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

bool parser::ismemaccess_flag(string token) {
    return token == "&" || token == "*";
}

void parser::parse_memaccess_flag(ast *pAst) {
    if(ismemaccess_flag(peek(1).gettoken()))
    {
        pAst = get_ast(pAst, ast_mem_access_flag);
        advance();
        pAst->add_entity(current());
    }
}

void parser::parse_vectorarray(ast* pAst) {
    pAst = get_ast(pAst, ast_vector_array);
    expect(LEFTCURLY, pAst, "`{`");

    if(peek(1).gettokentype() != RIGHTPAREN)
    {
        parse_expression(pAst);
        _pExpr:
        if(peek(1).gettokentype() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_expression(pAst);
            goto _pExpr;
        }
    }

    expect(RIGHTCURLY, pAst, "`}`");
}

void parser::parse_utypearg_list_opt(ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg_list_opt);
    expect(LEFTPAREN, pAst, "`(`");

    if(peek(1).gettokentype() != RIGHTPAREN)
    {
        parse_utypearg_opt(pAst);
        _puTypeArgOpt:
        if(peek(1).gettokentype() == COMMA)
        {
            expect(COMMA, pAst, "`,`");
            parse_utypearg_opt(pAst);
            goto _puTypeArgOpt;
        }
    }

    expect(RIGHTPAREN, pAst, "`)`");
}

void parser::parse_utypearg_list(ast* pAst) {
    pAst = get_ast(pAst, ast_utype_arg_list);
    expect(LEFTPAREN, pAst, "`(`");

    if(peek(1).gettokentype() != RIGHTPAREN)
    {
        parse_utypearg(pAst);
        _puTypeArg:
            if(peek(1).gettokentype() == COMMA)
            {
                expect(COMMA, pAst, "`,`");
                parse_utypearg(pAst);
                goto _puTypeArg;
            }
    }

    expect(RIGHTPAREN, pAst, "`)`");
}

void parser::parse_block(ast* pAst) {
    expect(LEFTCURLY, "`{`");
    pAst = get_ast(pAst, ast_block);

    while(!isend())
    {
        advance();
        if (current().gettokentype() == RIGHTCURLY)
        {
            pushback();
            break;
        }
        else if(current().gettokentype() == LEFTCURLY)
        {
            pushback();
            parse_block(pAst);
        }
        else if(current().gettokentype() == _EOF)
        {
            errors->newerror(UNEXPECTED_EOF, current());
            break;
        } else
            parse_statement(pAst);

        remove_accesstypes();
    }

    expect(RIGHTCURLY, "`}`");
}

void parser::parse_macrosdecl(ast *pAst) {
    pAst = get_ast(pAst, ast_method_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->add_entity(entity);
    }
    pAst->add_entity(current());

    expectidentifier(pAst);

    parse_utypearg_list(pAst);
    parse_block(pAst);

    expect(SEMICOLON, pAst, "`;`");
}

void parser::parse_operatordecl(ast *pAst) {
    pAst = get_ast(pAst, ast_operator_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->add_entity(entity);
    }
    pAst->add_entity(current());

    advance();
    expect_token(
            pAst, "operator", "`operator`");

    advance();
    if(!isoverride_operator(current().gettoken()))
        errors->newerror(GENERIC, current(), "expected override operator");
    else
        pAst->add_entity(current());

    parse_utypearg_list(pAst);

    parse_methodreturn_type(pAst); // assign-expr operators must return void
    parse_block(pAst);
}

void parser::parse_constructor(ast *pAst) {
    pAst = get_ast(pAst, ast_construct_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->add_entity(entity);
    }
    pushback();

    expectidentifier(pAst);

    parse_utypearg_list(pAst);
    parse_block(pAst);
}

void parser::parse_methoddecl(ast *pAst) {
    pAst = get_ast(pAst, ast_method_decl);

    for(token_entity &entity : *access_types)
    {
        pAst->add_entity(entity);
    }
    pAst->add_entity(current());

    expectidentifier(pAst);

    parse_utypearg_list(pAst);

    parse_methodreturn_type(pAst);
    parse_block(pAst);

}

void parser::parse_methodblock(ast *pAst) {
    expect(LEFTCURLY, "`{` after method declaration");
    pAst = get_ast(pAst, ast_block);
    ast *ref = pAst;

    int brackets = 1;

    while(!isend() && brackets > 0)
    {
        advance();
        if (current().gettokentype() == RIGHTCURLY)
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
        } else
            parse_statement(pAst);

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
            errors->newerror(GENERIC, current(), "expected native type or reference pointer");
    }
}

void parser::parse_returnstmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_return_stmnt);

    pAst->add_entity(current());
    parse_value(pAst);

    expect(SEMICOLON, pAst, "`;`");
}

void parser::parse_whilestmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_while_statement);

    expect_token(pAst, "while", "`while`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);
}

void parser::parse_dowhilestmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_while_statement);

    expect_token(pAst, "do", "`do`");
    parse_block(pAst);

    advance();
    expect_token(pAst, "while", "`while`");
    expect(LEFTPAREN, pAst, "`(`");
    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    expect(SEMICOLON, pAst, "`;`");

}

void parser::parse_ifstmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_if_statement);

    expect_token(pAst, "if", "`if`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);

    condexpr:
    if(peek(1).gettoken() == "else")
    {
        if(peek(2).gettoken() == "if")
        {
            pAst = get_ast(pAst, ast_elseif_statement);

            advance();
            pAst->add_entity(current());
            advance();
            pAst->add_entity(current());

            expect(LEFTPAREN, pAst, "`(`");
            parse_expression(pAst);
            expect(RIGHTPAREN, pAst, "`)`");
        }
        else
        {
            pAst = get_ast(pAst, ast_else_statement);

            advance();
            pAst->add_entity(current());
        }


        parse_block(pAst);
        goto condexpr;
    }
}

void parser::parse_catchclause(ast *pAst) {
    pAst = get_ast(pAst, ast_catch_clause);

    advance();
    expect_token(pAst, "catch", "`catch`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_utypearg_opt(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_block(pAst);
}

void parser::parse_finallyblock(ast *pAst) {
    pAst = get_ast(pAst, ast_finally_block);

    advance();
    expect_token(pAst, "finally", "`finally`");
    parse_block(pAst);
}

void parser::parse_trycatch(ast *pAst) {
    pAst = get_ast(pAst, ast_trycatch_statement);

    expect_token(pAst, "try", "`try`");
    parse_block(pAst);

    while(peek(1).gettoken() == "catch")
        parse_catchclause(pAst);

    if(peek(1).gettoken() == "finally")
    {
        parse_finallyblock(pAst);
    }
}

void parser::parse_throwstmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_throw_statement);

    expect_token(pAst, "throw", "`throw`");
    parse_expression(pAst);
    expect(SEMICOLON, pAst, "`;`");
}

bool parser::parse_extern_methoddecl(ast *pAst) {
    pAst = get_ast(pAst, ast_extern_method_decl);

    if(!parse_reference_pointer(pAst)){
        return false;
    }

    if(peek(1).gettokentype() == LEFTPAREN) {
        parse_utypearg_list_opt(pAst);
        return true;
    }

    return false;
}

bool parser::parse_extern_typeideitifier_decl(ast *pAst) {
    pAst = get_ast(pAst, ast_extern_typeid_decl);
    return parse_utypearg(pAst);
}

void parser::parse_externstmnt(ast *pAst) {
    pAst = get_ast(pAst, ast_extern_statement);
    expect_token(pAst, "extern", "`extern`");

    this->retainstate(pAst);
    errors->enablecheck_mode();
    if(parse_extern_methoddecl(pAst))
    {
        errors->fail();
        this->dumpstate();
    }
    else {
        errors->pass();
        this->rollback();

        this->retainstate(pAst);
        errors->enablecheck_mode();
        if(parse_extern_typeideitifier_decl(pAst))
        {
            errors->fail();
            this->dumpstate();
        }
        else {
            this->rollback();
            errors->newerror(GENERIC, current(), "expected external method or type "
                    "identifier declaration after `extern`");
        }
    }

    expect(SEMICOLON, pAst, "`;`");
}

void parser::parse_labeldecl(ast *pAst) {
    pAst = get_ast(pAst, ast_label_decl);

    pushback();
    expectidentifier(pAst);
    expect(COLON, pAst, "`:` after label decliration");

    advance();
    parse_statement(pAst);

}

void parser::parse_statement(ast* pAst) {
    pAst = get_ast(pAst, ast_statement);

    if(isreturn_stmnt(current()))
    {
        parse_returnstmnt(pAst);
    }
    else if(isif_stmnt(current()))
    {
        parse_ifstmnt(pAst);
    }
    else if(iswhile_stmnt(current()))
    {
        parse_whilestmnt(pAst);
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
    else if(isextern_stmnt(current()))
    {
        parse_externstmnt(pAst);
    }
    else if(current().gettoken() == "continue")
    {
        ast* pAst2 = get_ast(pAst, ast_continue_statement);

        expect_token(pAst2, "continue", "`continue`");

        errors->enablecheck_mode();
        if(!expectidentifier(pAst)) {
            errors->pass();
            pushback();
        }
        else
            errors->fail();
        expect(SEMICOLON, pAst, "`;`");
    }
    else if(current().gettoken() == "break")
    {
        ast* pAst2 = get_ast(pAst, ast_break_statement);

        expect_token(pAst2, "break", "`break`");
        expect(SEMICOLON, pAst, "`;`");
    }
    else if(current().gettoken() == "goto")
    {
        ast* pAst2 = get_ast(pAst, ast_goto_statement);

        expect_token(pAst2, "goto", "`goto`");

        errors->enablecheck_mode();
        if(!expectidentifier(pAst)) {
            errors->pass();
            pushback();
        }
        else
            errors->fail();
        expect(SEMICOLON, pAst, "`;`");
    }
    else if(isvariable_decl(current()))
    {
        parse_variabledecl(pAst);
    }
        /* these are just in case there is a missed bracket anywhere */
    else if(ismacros_decl(current()))
    {
        errors->newerror(GENERIC, current(), "macros declaration not allowed here");
        parse_macrosdecl(pAst);
    }
    else if(ismodule_decl(current()))
    {
        errors->newerror(GENERIC, current(), "module declaration not allowed here");
        parse_moduledecl(pAst);
    }
    else if(isclass_decl(current()))
    {
        errors->newerror(GENERIC, current(), "unexpected class declaration");
        parse_classdecl(pAst);
    }
    else if(isimport_decl(current()))
    {
        errors->newerror(GENERIC, current(), "import declaration not allowed here (why are you putting this here lol?)");
        parse_importdecl(pAst);
    }
    else if(current().gettokentype() == SEMICOLON)
    {
        /* we don't care about empty statements but we allow them */
    }
    else
    {
        // save parser state
        this->retainstate(pAst);
        pushback();

        /*
         * variable decl?
         */
        if(parse_utype(pAst))
        {

            if(peek(1).gettokentype() == COLON)
            {
                pAst = this->rollback();
                parse_labeldecl(pAst);
                return;
            }
            else if(peek(1).getid() == IDENTIFIER)
            {
                // Variable decliration
                pAst = this->rollback();
                parse_variabledecl(pAst);
                return;
            }
        }

        pAst = this->rollback();
        pushback();

        errors->enablecheck_mode();
        this->retainstate(pAst);
        if(!parse_expression(pAst))
        {
            errors->pass();
            advance();
            errors->newerror(GENERIC, current(), "not a statement");
            return;
        } else {
            errors->fail();
            this->dumpstate();

            if(peek(1).gettokentype() != SEMICOLON)
                errors->newerror(GENERIC, current(), "expected `;`");
            else
                expect(SEMICOLON, pAst, "`;`");
        }
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

    if(current().gettokentype() == _EOF)
    {
        return;
    }
    else if(ismethod_decl(current()))
    {
        if(peek(1).gettoken() == "operator")
            parse_operatordecl(pAst);
        else
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
    } else
        parse_statement(pAst);


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
           || key == "return" || key == "self"
           || key == "const" || key == "override"
           || key == "public" || key == "new"
           || key == "void" || key == "macros"
           || key == "null" || key == "operator"
           || key == "base" || key == "if"
           || key == "while" || key == "do"
           || key == "try" || key == "catch"
           || key == "finally" || key == "throw"
           || key == "continue" || key == "goto"
           || key == "break" || key == "else"
           || key == "extern" || key == "string"
           || key == "dynamic_object";
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
    if(!parse_reference_pointer(pAst)){
        errors->pass();
    }
    else {

        errors->fail();
        return true;
    }

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

        while(current().gettokentype() == DOT ) {
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
        std::free(errors); this->errors = NULL;
    }
}

void parser::retainstate(ast* pAst) {
    rState->push_back(parser_state(pAst, cursor, ast_cursor));
    rStateCursor++;
}

void parser::dumpstate() {
    if(rStateCursor >= 0)
    {
        rState->pop_back();
        rStateCursor--;
    }
}

ast* parser::rollbacklast() {
    if(rStateCursor >= 0)
    {
        parser_state* ps = &(*std::next(rState->begin(),
                                        rStateCursor));
        ast* pAst = ps->rAst;
        ast_cursor = ps->rAstcursor;
        cursor = ps->rCursor-1;
        advance();

        if(pAst->getsubastcount() == 1) {
            ps->rAst->freesubs();
            ps->rAst->freeentities();
        }
        else {
            pAst->freelastsub();
        }
        rState->pop_back();
        rStateCursor--;
        return pAst;
    }

    return NULL;
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
