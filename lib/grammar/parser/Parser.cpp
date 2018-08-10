//
// Created by BraxtonN on 1/22/2018.
//

#include "Parser.h"
#include "../Runtime.h"
#include <sstream>

#ifdef POSIX_
#include <stdarg.h>
#endif

#define advance() \
    if((cursor+1)<toks->getEntityCount()) \
        _current = &toks->getEntities().get(++cursor); \
    else _current =toks->EOF_token;

#define current() \
    (*_current)

void Parser::parse()
{
    if(toks->getEntities().size() == 0)
        return;

    lines.addAll(toks->getLines());
    sourcefile = toks->file;

    errors = new ErrorManager(lines, sourcefile, false, c_options.aggressive_errors);
    _current= &toks->getEntities().get(cursor);

    while(!isend())
    {
        if(panic)
            break;

        // evaluate
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
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            parse_moduledecl(NULL);
        }
        else if(isclass_decl(current()))
        {
            parse_classdecl(NULL);
        }
        else if(ismethod_decl(current()))
        {
            parse_methoddecl(NULL);
        }
        else if(isinterface_decl(current()))
        {
            parse_interfacedecl(NULL);
        }
        else if(isenum_decl(current()))
        {
            parse_enumdecl(NULL);
        }
        else if(isimport_decl(current()))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            parse_importdecl(NULL);
        }
        else if(isvariable_decl(current()))
        {
            parse_variabledecl(NULL);
        }
        else
        {
            // save parser state
            errors->enableErrorCheckMode();
            pushback();

            unsigned long old = cursor+1;
            /*
             * variable decl?
             */
            if(parse_utype(NULL))
            {
                cursor = old;
                _current = &toks->getEntities().get(cursor);

                errors->pass();
                if(current().getId() == IDENTIFIER)
                {
                    // Variable decliration
                    tree->pop_back();
                    ast_cursor--;
                    parse_variabledecl(NULL);
                    goto _continue;
                }
            } else
                errors->pass();

            tree->pop_back();
            ast_cursor--;
            cursor = old;
            _current = &toks->getEntities().get(cursor);

            // "expected class, or import declaration"
            errors->createNewError(UNEXPECTED_SYMBOL, current(), " `" + current().getToken() + "`; expected class, enum, or import declaration");
            parse_all(NULL);
        }

        _continue:
        advance();
        remove_accesstypes();
    }

    parsed = true;
    if(errors->hasErrors())
        return;
}

ErrorManager* Parser::getErrors()
{
    return errors;
}

bool Parser::isend() {
    return current().getTokenType() == _EOF;
}

void Parser::parse_interfacedecl(Ast* _ast) { // 1
    _ast = get_ast(_ast, ast_interface_decl);

    for(int i = 0; i < access_types.size(); i++) {
        _ast->addEntity(access_types.get(i));
    }
    _ast->addEntity(current());


    expectidentifier(_ast);

    if(peek(1).getToken() == "<") {
        _ast->setAstType(ast_generic_interface_decl);

        expect(LESSTHAN, "`<`");
        parse_identifier_list(_ast);
        expect(GREATERTHAN, "`>`");
    }

    if(peek(1).getToken() == "base")
    {
        advance();
        expect_token(_ast, "base", "");

        parse_reference_pointer(_ast);
    }

    parse_interfaceblock(_ast);
}

void Parser::parse_classdecl(Ast* _ast) { // 1
    _ast = get_ast(_ast, ast_class_decl);

    for(int i = 0; i < access_types.size(); i++) {
        _ast->addEntity(access_types.get(i));
    }
    _ast->addEntity(current());


    expectidentifier(_ast);

    if(peek(1).getToken() == "<") {
        _ast->setAstType(ast_generic_class_decl);

        expect(LESSTHAN, "`<`");
        parse_identifier_list(_ast);
        expect(GREATERTHAN, "`>`");
    }

    if(peek(1).getToken() == "base")
    {
        advance();
        expect_token(_ast, "base", "");

        parse_reference_pointer(_ast);
    }

    if(peek(1).getToken() == ":")
    {
        expect(COLON, "`:`");

        parse_reference_identifier_list(_ast);
    }

    parse_classblock(_ast);
}

void Parser::parse_enumblock(Ast *_ast) {
    _ast = get_ast(_ast, ast_enum_identifier_list);

    expect(LEFTCURLY, "`{`");

    parse_enumidentifier(_ast);
    pRefPtr:
    if(peek(1).getTokenType() == COMMA)
    {
        expect(COMMA, _ast, "`,`");
        parse_enumidentifier(_ast);
        goto pRefPtr;
    }

    expect(RIGHTCURLY, "`}`");
    expect(SEMICOLON, "`;`");
}

void Parser::parse_enumidentifier(Ast *_ast) {
    _ast = get_ast(_ast, ast_enum_identifier);

    expectidentifier(_ast);

    if(peek(1).getTokenType() == ASSIGN) {
        expect(ASSIGN, "`=`");

        parse_value(_ast);
    }
}

void Parser::parse_enumdecl(Ast* _ast) { // 1
    _ast = get_ast(_ast, ast_enum_decl);

    for(int i = 0; i < access_types.size(); i++) {
        _ast->addEntity(access_types.get(i));
    }
    _ast->addEntity(current());


    expectidentifier(_ast);
    parse_enumblock(_ast);
}

void Parser::parse_importdecl(Ast* _ast) {
    _ast = get_ast(_ast, ast_import_decl);
    _ast->addEntity(current());

    bool tmp = false;
    parse_modulename(_ast, tmp);

    if(tmp)
        errors->createNewError(GENERIC, current(), "module-name does not allow for generic typing ");

    expect(SEMICOLON, "`;`");
}

void Parser::parse_moduledecl(Ast* _ast) {
    _ast = get_ast(_ast, ast_module_decl);
    _ast->addEntity(current());

    bool tmp = false;
    parse_modulename(_ast, tmp);
    if(tmp)
        errors->createNewError(GENERIC, current(), "module-name does not allow for generic typing ");

    if(!expect(SEMICOLON, "`;`"))
        return;
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

bool Parser::isprototype_decl(token_entity token) {
    return token.getId() == IDENTIFIER && token.getToken() == "fn";
}

bool Parser::ismethod_decl(token_entity token) {
    return token.getId() == IDENTIFIER && token.getToken() == "def";
}

bool Parser::isenum_decl(token_entity token) {
    return token.getId() == IDENTIFIER && token.getToken() == "enum";
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

bool Parser::isinterface_decl(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "interface";
}

bool Parser::isimport_decl(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "import";
}

bool Parser::isreturn_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "return";
}

bool Parser::isswitch_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "switch";
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

bool Parser::islock_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "lock";
}

bool Parser::isforeach_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "foreach";
}

bool Parser::isassembly_stmnt(token_entity entity) {
    return entity.getId() == IDENTIFIER && entity.getToken() == "asm";
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

bool Parser::isswitch_declarator(token_entity entity) {
    return entity.getId() == IDENTIFIER && (entity.getToken() == "case" || entity.getToken() == "default");
}

bool Parser::isconstructor_decl() {
    return current().getId() == IDENTIFIER && !iskeyword(current().getToken()) &&
           peek(1).getTokenType() == LEFTPAREN;
}

bool Parser::isnative_type(string type) {
    return type == "var" || type == "object"
           || type == "_int8" || type == "_int16"
           || type == "_int32" || type == "_int64"
           || type == "_uint8" || type == "_uint16"
           || type == "_uint32" || type == "_uint64";
}

bool Parser::isspecial_native_type(string type) {
    return type == "_int8" || type == "_int16"
           || type == "_int32" || type == "_int64"
            || type == "_uint8" || type == "_uint16"
               || type == "_uint32" || type == "_uint64";
}

bool Parser::isaccess_decl(token_entity token) {
    return
            token.getId() == IDENTIFIER && token.getToken() == "protected" ||
            token.getId() == IDENTIFIER && token.getToken() == "private" ||
            token.getId() == IDENTIFIER && token.getToken() == "static" ||
            token.getId() == IDENTIFIER && token.getToken() == "const" ||
            token.getId() == IDENTIFIER && token.getToken() == "public";
}

void Parser::parse_accesstypes() {
    remove_accesstypes();

    while(isaccess_decl(current()))
    {
        access_types.push_back(current());
        advance();
    }
}

void Parser::remove_accesstypes() {
    access_types.free();
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

void Parser::parse_interfaceblock(Ast *pAst) {
    expect(LEFTCURLY, "`{` after interface declaration");
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
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected module declaration");
            parse_moduledecl(pAst);
        }
        else if(isinterface_decl(current()))
        {
            parse_interfacedecl(pAst);
        }
        else if(isimport_decl(current()))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected import declaration");
            parse_importdecl(pAst);
        }
        else if(isvariable_decl(current()))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected variable declaration");
            parse_variabledecl(pAst);
        }
        else if(isprototype_decl(current()))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected protype declaration");
            parse_prototypedecl(pAst);
        }
        else if(ismethod_decl(current()))
        {
            if(peek(1).getToken() == "operator") {
                if(access_types.size() > 0)
                {
                    errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
                }
                errors->createNewError(GENERIC, current(), "unexpected operator declaration");
                parse_operatordecl(pAst);
            }
            else if(peek(1).getToken() == "delegate") {
                parse_delegatedecl(pAst);
            }
            else {
                if(access_types.size() > 0)
                {
                    errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
                }
                errors->createNewError(GENERIC, current(), "unexpected method declaration");
                parse_methoddecl(pAst);
            }
        }
        else if(isconstructor_decl())
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected constructor declaration");
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
            errors->createNewError(GENERIC, current(), "expected delegate prototype declaration");
            parse_all(pAst);
        }

        remove_accesstypes();
    }

    if(brackets != 0)
        errors->createNewError(MISSING_BRACKET, current(), " expected `}` at end of interface declaration");

    expect(RIGHTCURLY, "`}` at end of interface declaration");
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
            if(access_types.size() > 0)
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
        else if(isinterface_decl(current()))
        {
            parse_interfacedecl(pAst);
        }
        else if(isimport_decl(current()))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected import declaration");
            parse_importdecl(pAst);
        }
        else if(isvariable_decl(current()))
        {
            parse_variabledecl(pAst);
        }
        else if(isprototype_decl(current()))
        {
            parse_prototypedecl(pAst);
        }
        else if(isenum_decl(current()))
        {
            parse_enumdecl(pAst);
        }
        else if(ismethod_decl(current()))
        {
            if(peek(1).getToken() == "operator")
                parse_operatordecl(pAst);
            else if(peek(1).getToken() == "delegate")
                parse_delegatedecl(pAst);
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

        return pAst->getLastSubAst();
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

        for(int i = 0; i < access_types.size(); i++) {
            pAst->addEntity(access_types.get(i));
        }
        remove_accesstypes();
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

void Parser::parse_prototypedecl(Ast *pAst, bool semicolon) {
    pAst = get_ast(pAst, ast_func_prototype);

    for(int i = 0; i < access_types.size(); i++) {
        pAst->addEntity(access_types.get(i));
    }
    remove_accesstypes();

    if(pAst->getEntityCount()>0)
        pushback();

    if(!isprototype_decl(current()))
        advance();
    expect_token(
            pAst, "fn", "`fn`");

    expectidentifier(pAst);

    parse_utypearg_list_opt(pAst);
    parse_methodreturn_type(pAst); // assign-expr operators must return void
    parse_prototype_valueassignment(pAst);

    if(semicolon)
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

void Parser::parse_prototype_valueassignment(Ast *pAst) {
    advance();
    if(isassign_exprsymbol(current().getToken()))
    {
        if(current().getToken() != "=") {
            errors->createNewError(GENERIC, current(), "expected assign operator `=`");
        }
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
        if(peek(1).getTokenType() == LEFTBRACE && peek(2).getTokenType() == RIGHTBRACE)
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
            if(!parse_value(pAst)){
                errors->createNewError(GENERIC, pAst->getLastSubAst(), "expected value");
            }
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

bool Parser::parse_dot_notation_call_expr(Ast *pAst) {
    pAst = get_ast(pAst, ast_dotnotation_call_expr);

    if(peek(1).getTokenType() == DOT)
    {
        advance();
        pAst->addEntity(current());
    }

    if(parse_utype_naked(pAst)) {
        if(peek(1).getTokenType() == LEFTPAREN) {
            parse_valuelist(pAst);

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

                if(peek(1).getTokenType() == DOT) {
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
        } else {
            this->dumpstate();
            errors->fail();

            errors->createNewError(GENERIC, pAst, "expected expression after '['");
            return false;
        }
    }

    expect(RIGHTBRACE, pAst, "`]`");

    return true;

}

bool Parser::match(int num_args, ...) {
    va_list ap;
    bool found = false;

    va_start(ap,num_args);
    for(size_t loop=0;loop<num_args;++loop) {
        if(peek(1).getTokenType() == va_arg(ap,int)) {
            found = true;
            break;
        }
    }

    va_end(ap);
    return found;
}

bool Parser::binary(Ast *pAst) {
    bool parsed = shift(pAst);

    while(match(5, AND, XOR, OR, ANDAND, OROR)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, pAst->getType(), pAst->line, pAst->col);
        shift(&right);
        pAst->addAst(right);
        pAst->encapsulate(ast_and_e);
        parsed = true;
    }

    return parsed;
}

bool Parser::shift(Ast *pAst) {
    bool parsed = equality(pAst);

    while(match(2, SHL, SHR)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, pAst->getType(), pAst->line, pAst->col);
        equality(&right);
        pAst->addAst(right);
        pAst = pAst->encapsulate(ast_shift_e);
        parsed = true;
    }

    return parsed;
}

bool Parser::equality(Ast *pAst) {
    bool parsed = comparason(pAst);

    while(match(2, EQEQ, NOTEQ)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, pAst->getType(), pAst->line, pAst->col);
        comparason(&right);
        pAst->addAst(right);
        pAst = pAst->encapsulate(ast_equal_e);
        parsed = true;
    }

    return parsed;
}

bool Parser::comparason(Ast *pAst) {
    bool parsed = addition(pAst);

    while(match(4, GREATERTHAN, _GTE, LESSTHAN, _LTE)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, pAst->getType(), pAst->line, pAst->col);
        addition(&right);
        pAst->addAst(right);
        pAst = pAst->encapsulate(ast_less_e);
        parsed = true;
    }

    return parsed;
}

bool Parser::addition(Ast *pAst) {
    bool parsed = multiplication(pAst);

    while(match(2, MINUS, PLUS)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, ast_expression, pAst->line, pAst->col);
        multiplication(&right);
        pAst->addAst(right);
        pAst = pAst->encapsulate(ast_add_e);
        parsed = true;
    }

    return parsed;
}

bool Parser::multiplication(Ast *pAst) {
    bool parsed = unary(pAst);

    while(match(3, _DIV, _MOD, MULT)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, ast_expression, pAst->line, pAst->col);
        unary(&right);
        pAst->addAst(right);
        pAst = pAst->encapsulate(ast_mult_e);
        parsed = true;
    }

    return parsed;
}

bool Parser::unary(Ast *pAst) {
    if(match(1, MINUS)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, pAst->getType(), pAst->line, pAst->col);
        bool parsed = unary(&right);
        pAst->addAst(right);
        pAst = pAst->encapsulate(ast_add_e);
        return parsed;
    } else if(match(1, NOT)) {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, pAst->getType(), pAst->line, pAst->col);
        bool parsed = unary(&right);
        pAst->addAst(right);
        pAst = pAst->encapsulate(ast_not_e);
        return parsed;
    }

    this->retainstate(pAst);
    errors->enableErrorCheckMode();
    if(!parse_primaryexpr(pAst))
    {
        errors->pass();
        this->rollbacklast();
        return false;
    } else
    {
        this->dumpstate();
        errors->fail();
        return true;
    }
}


bool Parser::parse_primaryexpr(Ast *pAst) {
    pAst = get_ast(pAst, ast_primary_expr);


    errors->enableErrorCheckMode();
    this->retainstate(pAst);
    if(parse_literal(pAst))
    {
        this->dumpstate();
        errors->fail();
        pAst->encapsulate(ast_literal_e);
        return true;
    }
    pAst = this->rollback();
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


    if(peek(1).getToken() == "self")
    {
        advance();
        expect_token(pAst, "self", "");

        if(peek(1).getTokenType() == PTR) {
            expect(PTR, pAst, "`->` after self");
            parse_dot_notation_call_expr(pAst);
        }

        pAst->encapsulate(ast_self_e);
        return true;
    }

    if(peek(1).getToken() == "base")
    {
        advance();
        expect_token(pAst, "base", "");
        expect(PTR, pAst, "`->` after base");
        parse_dot_notation_call_expr(pAst);

        pAst->encapsulate(ast_base_e);
        return true;
    }

    this->retainstate(pAst);
    errors->enableErrorCheckMode();
    if(parse_dot_notation_call_expr(pAst)) {
        this->dumpstate();
        errors->fail();
        pAst->encapsulate(ast_dot_not_e);
        if(!match(3, LEFTBRACE, _INC, _DEC))
            return true;
    } else {
        errors->pass();
        pAst = this->rollback();
    }

    if(peek(1).getToken() == "new")
    {
        advance();
        expect_token(pAst, "new", "");
        parse_utype_naked(pAst);
        bool newClass = false;

        if(peek(1).getTokenType() == LEFTBRACE && parse_array_expression(pAst)){}
        else if(peek(1).getTokenType() == LEFTBRACE) {
            expect(LEFTBRACE, pAst, "`[`");
            expect(RIGHTBRACE, pAst, "`]`");
            parse_vectorarray(pAst);
        }
        else if(peek(1).getTokenType() == LEFTPAREN) {
            parse_valuelist(pAst);
            newClass = true;
        }

        pAst = pAst->encapsulate(ast_new_e);
        if(peek(1).getTokenType() != LEFTBRACE) {
            if(newClass && match(1, DOT)) {
                parse_dot_notation_call_expr(pAst);
            }
                return true;
        }
    }

    if(peek(1).getToken() == "sizeof")
    {
        advance();
        expect_token(pAst, "sizeof", "");

        expect(LEFTPAREN, pAst, "`(`");
        parse_expression(pAst);
        expect(RIGHTPAREN, pAst, "`)`");

        pAst->encapsulate(ast_sizeof_e);
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
                this->rollbacklast();
            }
        }

    }

    if(peek(1).getTokenType() == LEFTPAREN)
    {
        advance();
        pAst->addEntity(current());

        parse_expression(pAst);

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
            pAst = pAst->encapsulate(ast_paren_e);

        if(peek(1).getTokenType() != LEFTBRACE)
            return true;
    }

    if(peek(1).getToken() == "null")
    {
        advance();
        expect_token(pAst, "null", "");
        pAst->encapsulate(ast_null_e);
        return true;
    }

    if(peek(1).getTokenType() == LEFTBRACE)
    {
        expect(LEFTBRACE, pAst, "`[`");
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
        pAst = pAst->encapsulate(ast_arry_e);

        if(!match(2, _INC, _DEC))
            return true;
    }

    /* ++ or -- after the expression */
    if(peek(1).getTokenType() == _INC || peek(1).getTokenType() == _DEC)
    {
        advance();
        pAst->addEntity(current());
        pAst->encapsulate(ast_post_inc_e);
        return true;
    }

    return false;
}

bool Parser::parse_expression(Ast *pAst) {
    pAst = get_ast(pAst, ast_expression);
    CHECK_ERRORS_RETURN(false)

    /* ++ or -- after the expression */
    if(peek(1).getTokenType() == _INC || peek(1).getTokenType() == _DEC)
    {
        advance();
        pAst->addEntity(current());
        parse_expression(pAst);
        pAst->encapsulate(ast_pre_inc_e);
        return true;
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

    /* expression <assign-expr> expression */
    if(isassign_exprsymbol(peek(1).getToken()))
    {
        advance();
        pAst->addEntity(current());

        Ast right(pAst, pAst->getType(), pAst->line, pAst->col);
        parse_expression(&right);
        pAst->addAst(*right.getLastSubAst());
        pAst->encapsulate(ast_assign_e);

        if(!isexprsymbol(peek(1).getToken()))
            return true;
    }

    bool parsed = binary(pAst);


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

    //errors->createNewError(GENERIC, current(), "expected expression");
    return parsed;
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
        if(isprototype_decl(peek(1)))
            parse_prototypedecl(pAst, false);
        else
            parse_utypearg_opt(pAst);
        _puTypeArgOpt:
        if(peek(1).getTokenType() == COMMA)
        {
            expect(COMMA, pAst, "`,`");

            if(isprototype_decl(peek(1)))
                parse_prototypedecl(pAst, false);
            else
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
        if(isprototype_decl(peek(1)))
            parse_prototypedecl(pAst, false);
        else
            parse_utypearg(pAst);
        _puTypeArg:
        if(peek(1).getTokenType() == COMMA)
        {
            expect(COMMA, pAst, "`,`");

            if(isprototype_decl(peek(1)))
                parse_prototypedecl(pAst, false);
            else
                parse_utypearg(pAst);
            goto _puTypeArg;
        }
    }

    expect(RIGHTPAREN, pAst, "`)`");
}

void Parser::parse_block(Ast* pAst) {
    bool curly = false;
    if(peek(1).getToken() == "{") {
        expect(LEFTCURLY, "`{`");
        curly = true;
    }
    pAst = get_ast(pAst, ast_block);

    while(!isend())
    {
        CHECK_ERRORS

        advance();
        if (current().getTokenType() == RIGHTCURLY)
        {
            if(!curly)
                errors->createNewError(GENERIC, current(), "expected '{'");
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
        }
        else {
            parse_statement(pAst);

            if(!curly) {

                remove_accesstypes();
                break;
            }
        }

        remove_accesstypes();
    }

    if(curly)
        expect(RIGHTCURLY, "`}`");
}

void Parser::parse_switch_declarator(Ast* pAst) {
    pAst = get_ast(pAst, ast_switch_declarator);
    advance();
    pAst->addEntity(current()); // case | default

    if(pAst->getEntity(0).getToken() == "case")
        parse_expression(pAst);
    else {
        int i = 0;
    }
    expect(COLON, "`:`");

    retry:
    if(isswitch_declarator(peek(1)) || peek(1).getTokenType() == RIGHTCURLY) return;
    if(peek(1).getTokenType() == LEFTCURLY) {
        parse_block(pAst);
        goto retry;
    } else {
        advance();
        errors->enableErrorCheckMode();
        this->retainstate(pAst);
        if(!parse_statement(pAst))
        {
            errors->pass();
            this->rollbacklast();
            return;
        } else {
            errors->fail();
            this->dumpstate();
            goto retry;
        }
    }
}

void Parser::parse_switchblock(Ast* pAst) {
    pAst = get_ast(pAst, ast_switch_block);

    expect(LEFTCURLY, "`{`");

    if(isswitch_declarator(peek(1)))
    {
        parse_switch_declarator(pAst);
        _pSwitchDecl:
        if(isswitch_declarator(peek(1)))
        {
            parse_switch_declarator(pAst);
            goto _pSwitchDecl;
        }
    }

    expect(RIGHTCURLY, "`}`");
}

void Parser::parse_operatordecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_operator_decl);


    for(int i = 0; i < access_types.size(); i++) {
        pAst->addEntity(access_types.get(i));
    }
    remove_accesstypes();
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


    remove_accesstypes();
    parse_block(pAst);
}

void Parser::parse_delegatedecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_delegate_decl);


    for(int i = 0; i < access_types.size(); i++) {
        pAst->addEntity(access_types.get(i));
    }
    remove_accesstypes();
    pAst->addEntity(current());

    advance();
    expect_token(
            pAst, "delegate", "`delegate`");

    expect(COLON, pAst, "`:`");
    expect(COLON, pAst, "`:`");
    expectidentifier(pAst);

    parse_utypearg_list(pAst);

    parse_methodreturn_type(pAst); // assign-expr operators must return void
    if(peek(1).getTokenType() == LEFTCURLY)
    {
        remove_accesstypes();
        parse_block(pAst);
    } else {
        expect(SEMICOLON, pAst, "`;`");
        pAst->setAstType(ast_delegate_post_decl);
    }
}

void Parser::parse_constructor(Ast *pAst) {
    pAst = get_ast(pAst, ast_construct_decl);

    for(int i = 0; i < access_types.size(); i++) {
        pAst->addEntity(access_types.get(i));
    }
    remove_accesstypes();
    pushback();

    expectidentifier(pAst);

    parse_utypearg_list(pAst);
    parse_block(pAst);
}

void Parser::parse_methoddecl(Ast *pAst) {
    pAst = get_ast(pAst, ast_method_decl);

    for(int i = 0; i < access_types.size(); i++) {
        pAst->addEntity(access_types.get(i));
    }
    remove_accesstypes();
    pAst->addEntity(current());

    expectidentifier(pAst);

    parse_utypearg_list(pAst);

    parse_methodreturn_type(pAst);

    remove_accesstypes();
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

    expect_token(pAst, "asm", "`asm`");

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

void Parser::parse_lockstmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_lock_statement);

    expect_token(pAst, "lock", "`lock`");

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

void Parser::parse_switchstmnt(Ast *pAst) {
    pAst = get_ast(pAst, ast_switch_statement);

    expect_token(pAst, "switch", "`switch`");

    expect(LEFTPAREN, pAst, "`(`");
    parse_expression(pAst);
    expect(RIGHTPAREN, pAst, "`)`");

    parse_switchblock(pAst);
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
    expect(COLON, pAst, "`:` after label declaration");

    if(peek(1).getTokenType() == LEFTCURLY)
        parse_block(pAst);
    else {
        advance();
        parse_statement(pAst);
    }

}

int commas=0;
bool Parser::parse_statement(Ast* pAst) {
    pAst = get_ast(pAst, ast_statement);
    CHECK_ERRORS_RETURN(false)

    remove_accesstypes();
    if(isaccess_decl(current()))
    {
        parse_accesstypes();
    }

    if(isreturn_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_returnstmnt(pAst);
        return true;
    }
    else if(isif_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_ifstmnt(pAst);
        return true;
    }
    else if(isswitch_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_switchstmnt(pAst);
        return true;
    }
    else if(isassembly_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_assemblystmnt(pAst );
        return true;
    }
    else if(isfor_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_forstmnt(pAst );
        return true;
    }
    else if(islock_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_lockstmnt(pAst );
        return true;
    }
    else if(isforeach_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_foreachstmnt(pAst );
        return true;
    }
    else if(iswhile_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_whilestmnt(pAst );
        return true;
    }
    else if(isdowhile_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_dowhilestmnt(pAst);
        return true;
    }
    else if(istrycatch_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_trycatch(pAst);
        return true;
    }
    else if(isthrow_stmnt(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_throwstmnt(pAst);
        return true;
    }
    else if(current().getToken() == "continue")
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        Ast* pAst2 = get_ast(pAst, ast_continue_statement);

        expect_token(pAst2, "continue", "`continue`");

        expect(SEMICOLON, pAst, "`;`");
        return true;
    }
    else if(current().getToken() == "break")
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        Ast* pAst2 = get_ast(pAst, ast_break_statement);

        expect_token(pAst2, "break", "`break`");
        expect(SEMICOLON, pAst2, "`;`");
        return true;
    }
    else if(current().getToken() == "goto")
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        Ast* pAst2 = get_ast(pAst, ast_goto_statement);

        expect_token(pAst2, "goto", "`goto`");

        expectidentifier(pAst2);
        // TODO: add support for calling goto labels[9];
        expect(SEMICOLON, pAst2, "`;`");
        return true;
    }
    else if(isvariable_decl(current()))
    {
        parse_variabledecl(pAst);
        return true;
    }
    else if(isprototype_decl(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parse_prototypedecl(pAst);
        return true;
    }
        /* these are just in case there is a missed bracket anywhere */
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
    else if(isenum_decl(current()))
    {
        errors->createNewError(GENERIC, current(), "enum declaration cannot be local");
        parse_enumdecl(pAst);
    }
    else if(isinterface_decl(current()))
    {
        errors->createNewError(GENERIC, current(), "unexpected interface declaration");
        parse_interfacedecl(NULL);
    }
    else if(isimport_decl(current()))
    {
        errors->createNewError(GENERIC, current(), "import declaration not allowed here (why are you putting this here lol?)");
        parse_importdecl(pAst);
    }
    else if(current().getTokenType() == SEMICOLON)
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        /* we don't care about empty statements but we allow them */
        if(commas > 1) {
            commas = 0;
            errors->createNewWarning(GENERIC, current().getLine(), current().getColumn(), "unnecessary semicolon ';'");
        } else
            commas++;
        return true;
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
                if(access_types.size() > 0)
                    errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

                pAst = this->rollback();
                parse_labeldecl(pAst);
                return true;
            }
            else if(peek(1).getId() == IDENTIFIER)
            {
                // Variable decliration
                pAst = this->rollback();
                parse_variabledecl(pAst);
                return true;
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
            errors->createNewError(GENERIC, pAst, "not a statement");
            return false;
        } else {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, pAst);

            errors->fail();
            this->dumpstate();

            if(peek(1).getTokenType() != SEMICOLON)
                errors->createNewError(GENERIC, pAst, "expected `;`");
            else
                expect(SEMICOLON, pAst, "`;`");
            return true;
        }
    }

    return false;
}

void Parser::parse_modulename(Ast* pAst, bool &parsedGeneric, bool keepLocal)
{
    if(!keepLocal)
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
        if(parse_template_decl(pAst)) parsedGeneric = true;
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
            if(expectidentifier(pAst)){
                if(parse_template_decl(pAst))
                    parsedGeneric = true;
            }
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
        else if(peek(1).getToken() == "delegate")
            parse_delegatedecl(pAst);
        else
            parse_methoddecl(pAst);
    }
    else if(ismodule_decl(current()))
    {
        if(access_types.size() > 0)
        {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parse_moduledecl(pAst);
    }
    else if(isclass_decl(current()))
    {
        parse_classdecl(pAst);
    }
    else if(isenum_decl(current()))
    {
        parse_enumdecl(pAst);
    }
    else if(isinterface_decl(current()))
    {
        parse_interfacedecl(NULL);
    }
    else if(isimport_decl(current()))
    {
        if(access_types.size() > 0)
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
           || key == "static" || key == "protected"
           || key == "private" || key == "def"
           || key == "import" || key == "return"
           || key == "self" || key == "const"
           || key == "public" || key == "new"
           || key == "null" || key == "operator"
           || key == "base" || key == "if" || key == "while" || key == "do"
           || key == "try" || key == "catch"
           || key == "finally" || key == "throw" || key == "continue"
           || key == "goto" || key == "break" || key == "else"
           || key == "object" || key == "asm" || key == "for" || key == "foreach"
           || key == "var" || key == "sizeof"|| key == "_int8" || key == "_int16"
           || key == "_int32" || key == "_int64" || key == "_uint8"
           || key == "_uint16"|| key == "_uint32" || key == "_uint64"
           || key == "delegate" || key == "interface" || key == "lock" || key == "enum"
           || key == "switch" || key == "default" || key == "fn";
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

bool Parser::parse_template_decl(Ast *pAst) {
    if(peek(1).getTokenType() == LESSTHAN) {
        advance();
        Ast tmp(pAst->getParent(), pAst);
        unsigned long tmpCursor = cursor;
        if(peek(1).getTokenType() == GREATERTHAN) {
            errors->createNewError(GENERIC, current(), "expected identifier before `>`");
            advance();
            return true;
        }

        if(parse_utype(pAst) && (peek(1).getTokenType() == GREATERTHAN || peek(1).getTokenType() == COMMA))
        {
            cursor = tmpCursor;
            pAst->copy(&tmp);
            errors->pass();
            parse_utype_list(pAst);
            if(peek(1).getTokenType() == GREATERTHAN) {
                expect(GREATERTHAN, pAst, "`>`");
                pAst->freeLastEntity();
            } else
                expect(GREATERTHAN, pAst, "`>`");
            tmp.free();

            return true;
        } else
            pAst->copy(&tmp);
        cursor = tmpCursor;
        tmp.free();
        errors->pass();
        pushback();
    }

    get_ast(pAst, ast_none); // filler so we dont process it
    return false;
}

void Parser::parse_reference_identifier_list(Ast *ast) {
    ast = get_ast(ast, ast_reference_identifier_list);

    parse_reference_pointer(ast);
    pRefPtr:
    if(peek(1).getTokenType() == COMMA)
    {
        expect(COMMA, ast, "`,`");
        parse_reference_pointer(ast);
        goto pRefPtr;
    }
}

void Parser::parse_utype_list(Ast *ast) {
    ast = get_ast(ast, ast_utype_list);

    parse_utype(ast);
    pRefPtr:
    if(peek(1).getTokenType() == COMMA)
    {
        expect(COMMA, ast, "`,`");
        parse_utype(ast);
        goto pRefPtr;
    }
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
    bool parsedGeneric = false;
    parse_modulename(pAst, parsedGeneric, true);

    advance();
    if(current().getTokenType() == HASH) {
        if(parsedGeneric)
            errors->createNewError(GENERIC, current(), "module-name does not allow for generic typing ");
        pAst->addEntity(current());

        if(expectidentifier(pAst)) {
            parse_template_decl(pAst);
            advance();
        }

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

                parse_template_decl(pAst);
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
        this->state.free();
        delete (this->tree); this->tree = NULL;
        access_types.free();
        errors->free();
        delete (errors); this->errors = NULL;
    }
}

void Parser::retainstate(Ast* pAst) {
    state.add(ParserState(pAst, cursor, ast_cursor));
    rStateCursor++;
}

void Parser::dumpstate() {
    if(rStateCursor >= 0)
    {
        state.pop_back();
        rStateCursor--;
    }
}

Ast* Parser::rollbacklast() {
    if(rStateCursor >= 0)
    {
        ParserState* ps = &state.last();
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
        state.pop_back();
        rStateCursor--;
        return pAst;
    }

    return NULL;
}

Ast* Parser::popBacklast() {
    if(rStateCursor >= 0)
    {
        ParserState* ps = &state.last();
        Ast* pAst = ps->ast;
        ast_cursor = ps->astCursor;
        cursor = ps->cursor-1;

        state.pop_back();
        rStateCursor--;
        return pAst;
    }

    return NULL;
}

Ast* Parser::rollback() {
    if(rStateCursor >= 0)
    {
        ParserState* ps = &state.last();
        Ast* pAst = ps->ast;
        ast_cursor = ps->astCursor;
        cursor = ps->cursor-1;
        advance();

        pAst->free();
        pAst->copy(&ps->copy);
        state.pop_back();
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

void Parser::parse_identifier_list(Ast *pAst) {
    pAst = get_ast(pAst, ast_identifier_list);

    expectidentifier(pAst);
    pRefPtr:
    if(peek(1).getTokenType() == COMMA)
    {
        expect(COMMA, pAst, "`,`");
        expectidentifier(pAst);
        goto pRefPtr;
    }
}
