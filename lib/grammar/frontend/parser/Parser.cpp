//
// Created by BraxtonN on 10/11/2019.
//

#include "Parser.h"
#include "../../main.h"
#include "Ast.h"

#define current() \
    (*_current)

#define advance() \
    if(_current->getType() != _EOF) \
        _current++;

#define isEnd() \
    (_current->getType() == _EOF)

void parser::parse() {
    if(toks->size() == 0)
        return;

    lines.addAll(toks->getLines());
    sourcefile = toks->file;

    errors = new ErrorManager(lines, sourcefile, false, c_options.aggressive_errors);
    _current= &toks->getTokens().get(0);

    while(true) {
        if(panic)
            break;

        // evaluate
        if(isAccessDecl(current()))
        {
            parseAccessTypes();
        }
        CHECK_ERRLMT(return;)

        if(isEnd())
        {
            break;
        } else if(isModuleDecl(current()))
        {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

            parseModuleDecl(NULL);
        } else if(isImportDecl(current()))
        {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

            parseImportDecl(NULL);
        } else if(isClassDecl(current()))
        {
            parseClassDecl(NULL);
        } else if(isMutateDecl(current()))
        {
            parseMutateDecl(NULL);
        }
        else if(isMethodDecl(current()))
        {
            parseMethodDecl(NULL);
        }
        else if(isAliasDeclaration(current()))
        {
            parseAliasDeclaration(NULL);
        }
        else if(isInterfaceDecl(current()))
        {
            parseInterfaceDecl(NULL);
        }
        else if(isEnumDecl(current()))
        {
            parseEnumDecl(NULL);
        }
        else if((isVariableDecl(current()) && (*peek(1) == ":" || *peek(1) == ":=")) ||
                (isStorageType(current()) && (isVariableDecl(*peek(1)) && (*peek(2) == ":" || *peek(2) == ":="))))
        {
            parseVariableDecl(NULL);
        }
        else if(isPrototypeDecl(current()) || (isStorageType(current()) && isPrototypeDecl(*peek(1))))
        {
            parsePrototypeDecl(NULL);
        }
        else
        {
            // "expected class, or import declaration"
            errors->createNewError(UNEXPECTED_SYMBOL, current(), " `" + current().getValue() + "`; expected class, enum, or import declaration");
            parseAll(NULL);
        }

        _continue:
        advance();
        access_types.free();
    }

    parsed = !panic;
}

void parser::parseInterfaceDecl(Ast *ast) {
    Ast *branch = getBranch(ast, ast_interface_decl);
    addAccessTypes(branch);
    access_types.free();

    // class name
    expectIdentifier(branch);

    if(peek(1)->getValue() == "<") {
        branch->setAstType(ast_generic_interface_decl);

        expect(branch, "<", false);
        parseIdentifierList(branch);
        expect(branch, ">", false);
    }

    if(peek(1)->getValue() == "base")
    {
        expect(branch, "base");
        parseReferencePointer(branch);
    }

    parseInterfaceBlock(branch);
}

void parser::parseMethodDecl(Ast *ast) {
    Ast* branch = getBranch(ast, ast_method_decl);

    addAccessTypes(branch);
    access_types.free();

    if(*peek(1) == IDENTIFIER && *peek(1) != "operator")
        parseReferencePointer(branch);
    else
        errors->createNewError(GENERIC, current(), "expected identifier");

    parseUtypeArgList(branch);

    if(peek(1)->getType() == COLON) {
        parseMethodReturnType(branch);

        if(peek(1)->getType() == EQUALS) {
            expect(branch, "=", true);
            parseExpression(branch);
            expect(branch, ";", false);
        } else {
            parseBlock(branch);
        }
    } else if(peek(1)->getType() == INFER) {
        expect(branch, ":=", true);
        parseExpression(branch);
        expect(branch, ";", false);
    } else {
        parseBlock(branch);
    }
}

void parser::parseDelegateDecl(Ast *ast) {
    Ast* branch = getBranch(ast, ast_delegate_impl);


    addAccessTypes(branch);
    access_types.free();

    expect(branch, "delegate", false);

    expect(branch, ":", false);
    expect(branch, ":", false);
    expectIdentifier(branch);

    parseUtypeArgList(branch);

    if(*peek(1) == ";") {
        expect(branch, ";");
        branch->setAstType(ast_delegate_decl);
    } else {
        if(peek(1)->getType() == COLON) {
            parseMethodReturnType(branch);

            if(peek(1)->getType() == EQUALS) {
                expect(branch, "=", true);
                parseExpression(branch);
                expect(branch, ";", false);
            } else if(*peek(1) == ";") {
                expect(branch, ";");
                branch->setAstType(ast_delegate_decl);
            } else {
                parseBlock(branch);
            }
        } else if(peek(1)->getType() == INFER) {
            expect(branch, ":=", true);
            parseExpression(branch);
            expect(branch, ";", false);
        } else {
            parseBlock(branch);
        }
    }
}

void parser::parseBlock(Ast* ast) {
    Ast* branch = getBranch(ast, ast_block);

    bool curly = false;
    if(*peek(1) == "{") {
        expect(branch, "{");
        curly = true;
    }

    while(!isEnd())
    {
        CHECK_ERRLMT(return;)

        advance();
        if (current().getType() == RIGHTCURLY)
        {
            if(!curly)
                errors->createNewError(GENERIC, current(), "expected '{'");
            _current--;
            break;
        }
        else if(current().getType() == LEFTCURLY)
        {
            _current--;
            parseBlock(branch);
        }
        else if(current().getType() == _EOF)
        {
            errors->createNewError(UNEXPECTED_EOF, current());
            break;
        }
        else {
            parseStatement(branch);

            if(!curly) {
                access_types.free();
                break;
            }
        }

        access_types.free();
    }

    if(curly)
        expect(branch, "}");
}

void parser::parseLambdaBlock(Ast* ast) {
    Ast* branch = getBranch(ast, ast_block);

    while(!isEnd())
    {
        CHECK_ERRLMT(return;)

        advance();
        if (current().getType() == RIGHTCURLY)
        {
            _current--;
            break;
        }
        else if(current().getType() == LEFTCURLY)
        {
            _current--;
            parseBlock(branch);
        }
        else if(current().getType() == _EOF)
        {
            errors->createNewError(UNEXPECTED_EOF, current());
            break;
        }
        else {
            if(!parseStatement(branch))
                break;
        }

        access_types.free();
    }

    expect(branch, "}");
}

void parser::parseReturnStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_return_stmnt);

    branch->addToken(current());

    if(*peek(1) != ";")
        parseExpression(branch);

    expect(branch, ";");
}


void parser::parseSwitchDeclarator(Ast* ast) {
    Ast* branch = getBranch(ast, ast_switch_declarator);
    advance();
    branch->addToken(current()); // case | default

    if(branch->getEntity(0).getValue() == "case")
        parseExpression(branch);
    else {
        int i = 0;
    }
    expect(branch, ":");

    retry:
    if(isSwitchDeclarator(*peek(1)) || peek(1)->getType() == RIGHTCURLY) return;
    if(peek(1)->getType() == LEFTCURLY) {
        parseBlock(branch);
        goto retry;
    } else {
        advance();
        errors->enterProtectedMode();
        Token* old = _current;
        if(!parseStatement(branch))
        {
            _current=old;
            errors->pass();
            if(branch->getSubAstCount() == 1) {
                branch->freeSubAsts();
                branch->freeEntities();
            }
            else {
                branch->freeLastSub();
            }
            return;
        } else {
            errors->fail();
            goto retry;
        }
    }
}

void parser::parseSwitchBlock(Ast* ast) {
    Ast* branch = getBranch(ast, ast_switch_block);

    expect(branch, "{");

    if(isSwitchDeclarator(*peek(1)))
    {
        parseSwitchDeclarator(branch);
        _pSwitchDecl:
        if(isSwitchDeclarator(*peek(1)))
        {
            parseSwitchDeclarator(branch);
            goto _pSwitchDecl;
        }
    }

    expect(branch, "}");
}

void parser::parseSwitchStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_switch_statement);

    expect(branch, "switch");

    expect(branch, "(");
    parseExpression(ast);
    expect(branch, ")");

    parseSwitchBlock(ast);
}


void parser::parseAssemblyBlock(Ast *ast) {
    Ast* branch = getBranch(ast, ast_assembly_block);

    if(peek(1)->getId() == STRING_LITERAL) {
        advance();
        branch->addToken(current());

        while(peek(1)->getId() == STRING_LITERAL) {
            advance();
            branch->addToken(current());
        }
    } else {
        errors->createNewError(GENERIC, current(), "expected string literal");
    }
}

void parser::parseAssemblyStmnt(Ast *ast) {
    Ast* branch = getBranch(ast, ast_assembly_statement);

    expect(branch, "asm");

    expect(branch, "(");
    parseAssemblyBlock(branch);
    expect(branch, ")");
    expect(branch, ";");
}

void parser::parseAliasDeclaration(Ast *ast) {
    Ast* branch = getBranch(ast, ast_alias_decl);

    access_types.free();
    if(isAccessDecl(current()))
    {
        parseAccessTypes();
    }

    parseUtype(branch);
    expect(branch, "as", false);
    expectIdentifier(branch);
    expect(branch, ";", false);
}

void parser::parseIfStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_if_statement);

    expect(branch, "(");
    parseExpression(branch);
    expect(branch, ")");

    parseBlock(branch);

    Ast* tmp;
    bool isElse = false;
    condexpr:
    if(peek(1)->getValue() == "else")
    {
        if(peek(2)->getValue() == "if")
        {
            tmp = getBranch(branch, ast_elseif_statement);

            advance();
            tmp->addToken(current());
            advance();
            tmp->addToken(current());

            expect(branch, "(");
            parseExpression(tmp);
            expect(branch, ")");
        }
        else
        {
            tmp = getBranch(branch, ast_else_statement);

            advance();
            tmp->addToken(current());
            isElse = true;
        }


        parseBlock(tmp);
        if(!isElse)
            goto condexpr;
    }
}

void parser::parseForStmnt(Ast *ast) {
    Ast* branch = getBranch(ast, ast_for_statement);

    expect(branch, "for");

    expect(branch, "(");

    if(isForLoopCompareSymbol(peek(1)->getValue())) {
        expect(branch, peek(1)->getValue());
        parseExpression(branch);

        expect(branch, ":");
        parseBlock(branch);
    } else {
        if(parseExpression(branch) && isForLoopCompareSymbol(peek(1)->getValue())) {
            branch->freeLastSub();
            parseExpression(branch);
            expect(branch, peek(1)->getValue());

            expect(branch, ":");
            parseBlock(branch);
        } else {
            if(peek(1)->getType() != SEMICOLON) {
                parseVariableDecl(branch);
            }

            if(peek(1)->getType() != SEMICOLON) {
                parseExpression(branch);
                branch->getLastSubAst()->setAstType(ast_for_expresion_cond);
            }
            expect(branch, ";");

            if(peek(1)->getType() != RIGHTPAREN) {
                parseExpression(branch);
                branch->getLastSubAst()->setAstType(ast_for_expresion_iter);
            }
            expect(branch, ")");

            parseBlock(branch);
        }
    }
}

int commas=0;
bool parser::parseStatement(Ast* ast) {
    Ast* branch = getBranch(ast, ast_statement);
    CHECK_ERRLMT(return false;)

    access_types.free();
    if(isAccessDecl(current()))
    {
        parseAccessTypes();
    }

    if(isReturnStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseReturnStatement(branch);
        return true;
    }
    else if(isIfStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseIfStatement(branch);
        return true;
    }
    else if(isSwitchStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseSwitchStatement(branch);
        return true;
    }
    else if(isAssemblyStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseAssemblyStmnt(branch);
        return true;
    }
    else if(isForStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseForStmnt(branch);
        return true;
    }
    else if(isAliasDeclaration(current()))
    {
        parseAliasDeclaration(branch);
        return true;
    }
//    else if(islock_stmnt(current()))
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        parse_lockstmnt(pAst );
//        return true;
//    }
//    else if(isforeach_stmnt(current()))
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        parse_foreachstmnt(pAst );
//        return true;
//    }
//    else if(iswhile_stmnt(current()))
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        parse_whilestmnt(pAst );
//        return true;
//    }
//    else if(isdowhile_stmnt(current()))
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        parse_dowhilestmnt(pAst);
//        return true;
//    }
//    else if(istrycatch_stmnt(current()))
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        parse_trycatch(pAst);
//        return true;
//    }
//    else if(isthrow_stmnt(current()))
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        parse_throwstmnt(pAst);
//        return true;
//    }
//    else if(current().getToken() == "continue")
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        Ast* pAst2 = get_ast(pAst, ast_continue_statement);
//
//        expect_token(pAst2, "continue", "`continue`");
//
//        expect(SEMICOLON, pAst, "`;`");
//        return true;
//    }
//    else if(current().getToken() == "break")
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        Ast* pAst2 = get_ast(pAst, ast_break_statement);
//
//        expect_token(pAst2, "break", "`break`");
//        expect(SEMICOLON, pAst2, "`;`");
//        return true;
//    }
//    else if(current().getToken() == "goto")
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        Ast* pAst2 = get_ast(pAst, ast_goto_statement);
//
//        expect_token(pAst2, "goto", "`goto`");
//
//        expectidentifier(pAst2);
//        // TODO: add support for calling goto labels[9];
//        expect(SEMICOLON, pAst2, "`;`");
//        return true;
//    }
//    else if(isvariable_decl(current()) || (isstorage_type(current()) && isvariable_decl(peek(1))))
//    {
//        parse_variabledecl(pAst);
//        return true;
//    }
//    else if(isprototype_decl(current()) || (isstorage_type(current()) && isprototype_decl(peek(1))))
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        parse_prototypedecl(pAst);
//        return true;
//    }
//        /* these are just in case there is a missed bracket anywhere */
//    else if(ismodule_decl(current()))
//    {
//        errors->createNewError(GENERIC, current(), "module declaration not allowed here");
//        parse_moduledecl(pAst);
//    }
//    else if(isclass_decl(current()))
//    {
//        errors->createNewError(GENERIC, current(), "unexpected class declaration");
//        parse_classdecl(pAst);
//    }
//    else if(isenum_decl(current()))
//    {
//        errors->createNewError(GENERIC, current(), "enum declaration cannot be local");
//        parse_enumdecl(pAst);
//    }
//    else if(isinterface_decl(current()))
//    {
//        errors->createNewError(GENERIC, current(), "unexpected interface declaration");
//        parse_interfacedecl(NULL);
//    }
//    else if(isimport_decl(current()))
//    {
//        errors->createNewError(GENERIC, current(), "import declaration not allowed here (why are you putting this here lol?)");
//        parse_importdecl(pAst);
//    }
//    else if(current().getTokenType() == SEMICOLON)
//    {
//        if(access_types.size() > 0)
//            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//        /* we don't care about empty statements but we allow them */
//        if(commas > 1) {
//            commas = 0;
//            errors->createNewWarning(GENERIC, current().getLine(), current().getColumn(), "unnecessary semicolon ';'");
//        } else
//            commas++;
//        return true;
//    }
//    else
//    {
//        // save parser state
//        errors->enterProtectedMode();
//        this->retainstate(pAst);
//        pushback();
//
//        /*
//         * variable decl?
//         */
//        if(parse_utype(pAst))
//        {
//            errors->pass();
//            if(peek(1).getTokenType() == COLON)
//            {
//                if(access_types.size() > 0)
//                    errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
//
//                pAst = this->rollback();
//                parse_labeldecl(pAst);
//                return true;
//            }
//            else if(peek(1).getId() == IDENTIFIER)
//            {
//                // Variable decliration
//                pAst = this->rollback();
//                parse_variabledecl(pAst);
//                return true;
//            }
//        } else
//            errors->pass();
//
//        pAst = this->rollback();
//        pushback();
//
//        errors->enterProtectedMode();
//        this->retainstate(pAst);
//        if(!parse_expression(pAst))
//        {
//            errors->pass();
//            advance();
//            errors->createNewError(GENERIC, pAst, "not a statement");
//            return false;
//        } else {
//            if(access_types.size() > 0)
//                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, pAst);
//
//            errors->fail();
//            this->dumpstate();
//
//            if(peek(1).getTokenType() != SEMICOLON)
//                errors->createNewError(GENERIC, pAst, "expected `;`");
//            else
//                expect(SEMICOLON, pAst, "`;`");
//            return true;
//        }
//    }

    return false;
}

void parser::parseOperatorDecl(Ast *ast) {
    Ast* branch = getBranch(ast, ast_operator_decl);


    addAccessTypes(branch);
    access_types.free();
    expect(branch, "operator");

    advance();
    if(!isOverrideOperator(current().getValue()))
        errors->createNewError(GENERIC, current(), "expected override operator");
    else
        branch->addToken(current());

    parseUtypeArgList(branch);

    if(peek(1)->getType() == COLON) {
        parseMethodReturnType(branch);

        if(peek(1)->getType() == EQUALS) {
            expect(branch, "=", true);
            parseExpression(branch);
            expect(branch, ";", false);
        } else {
            parseBlock(branch);
        }
    } else if(peek(1)->getType() == INFER) {
        expect(branch, ":=", true);
        parseExpression(branch);
        expect(branch, ";", false);
    } else {
        parseBlock(branch);
    }
}

void parser::parseInterfaceBlock(Ast* ast) {
    Ast *branch = getBranch(ast, ast_block);
    expect(branch, "{");

    int brackets = 1;
    while(!isEnd() && brackets > 0)
    {
        CHECK_ERRLMT(return;)

        advance();
        if(isAccessDecl(current()))
        {
            parseAccessTypes();
        }

        if(isModuleDecl(current()))
        {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected module declaration");
            parseModuleDecl(branch);
        }
        else if(isInterfaceDecl(current()))
        {
            parseInterfaceDecl(branch);
        }
        else if(isMutateDecl(current()))
        {
            errors->createNewError(GENERIC, current(), "unexpected mutate declaration");
            parseMutateDecl(NULL);
        }
        else if(isImportDecl(current()))
        {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected import declaration");
            parseImportDecl(branch);
        }
        else if(isAliasDeclaration(current()))
        {
            parseAliasDeclaration(branch);
        }
        else if((isVariableDecl(current()) && (*peek(1) == ":" || *peek(1) == ":=")) ||
            (isStorageType(current()) && (isVariableDecl(*peek(1)) && (*peek(2) == ":" || *peek(2) == ":="))))
        {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected variable declaration");
            parseVariableDecl(branch);
        }
        else if(isPrototypeDecl(current()) || (isStorageType(current()) && isPrototypeDecl(*peek(1))))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected function pointer");
            parsePrototypeDecl(branch);
        }
        else if(isMethodDecl(current()))
        {
            if(peek(1)->getValue() == "operator") {
                if(access_types.size() > 0)
                {
                    errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
                }
                errors->createNewError(GENERIC, current(), "unexpected operator declaration");
                parseOperatorDecl(branch);
            }
            else if(peek(1)->getValue() == "delegate") {
                parseDelegateDecl(branch);
            }
            else {
                if(access_types.size() > 0)
                {
                    errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
                }
                errors->createNewError(GENERIC, current(), "unexpected method declaration");
                parseMethodDecl(branch);
            }
        }
        else if(isInitDecl(current()))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected init declaration");
            parseInitDecl(branch);
        }
        else if(isConstructorDecl())
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected constructor declaration");
            parseConstructor(branch);
        }
        else if(current().getType() == _EOF)
        {
            errors->createNewError(UNEXPECTED_EOF, current());
            break;
        }
        else if (current().getType() == RIGHTCURLY)
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
                    _current--;
                    break;
                }
            }
        }
        else if(current().getType() == LEFTCURLY)
            brackets++;
        else {
            // save parser state
            _current--;

            errors->createNewError(GENERIC, current(), "expected delegate declaration");
            parseAll(branch);
        }

        access_types.free();
    }

    if(brackets != 0)
        errors->createNewError(MISSING_BRACKET, current(), " expected `}` at end of interface declaration");

    expect(branch, "}");
}

/**
 * This function is used to parse through every possible outcome that the parser can run into when it does not find a class or import
 * statement to process. This alleviates your console getting flooded with a bunch of unnessicary "unexpected symbol" complaints.
 *
 */
void parser::parseAll(Ast *ast) {

    if(isAccessDecl(current()))
    {
        parseAccessTypes();
    }
    CHECK_ERRLMT(return;)

    if(current().getType() == _EOF)
    {
        return;
    }
    else if(isMethodDecl(current()))
    {
        if(peek(1)->getValue() == "operator")
            parseOperatorDecl(ast);
        else if(peek(1)->getValue() == "delegate")
            parseDelegateDecl(ast);
        else
            parseMethodDecl(ast);
    }
    else if(isModuleDecl(current()))
    {
        if(access_types.size() > 0)
        {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parseModuleDecl(ast);
    }
    else if(isClassDecl(current()))
    {
        parseClassDecl(ast);
    }
    else if(isMutateDecl(current()))
    {
        parseMutateDecl(ast);
    }
    else if(isEnumDecl(current()))
    {
        parseEnumDecl(ast);
    }
    else if(isInitDecl(current()))
    {
        parseInitDecl(ast);
    }
    else if(isInterfaceDecl(current()))
    {
        parseInterfaceDecl(ast);
    }
    else if(isAliasDeclaration(current()))
    {
        parseAliasDeclaration(ast);
    }
    else if(isImportDecl(current()))
    {
        if(access_types.size() > 0)
        {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
        }
        parseImportDecl(ast);
    } else
        parseStatement(ast);
}

void parser::parseEnumDecl(Ast *ast) {
    Ast* branch = getBranch(ast, ast_enum_decl);

    addAccessTypes(branch);
    access_types.free();

    expectIdentifier(branch);
    parseEnumBlock(branch);
}

void parser::parseInitDecl(Ast *ast) {
    Ast* branch = getBranch(ast, ast_enum_decl);

    if(access_types.size() > 0) {
        errors->createNewError(GENERIC, current(), "access types not allowed here");
    }
    access_types.free();

    parseBlock(branch);
}

void parser::parseEnumBlock(Ast *ast) {
    Ast* branch = getBranch(ast, ast_enum_identifier_list);

    expect(branch, "{");

    parseEnumIdentifier(branch);
    pRefPtr:
    if(peek(1)->getType() == COMMA)
    {
        expect(branch, ",");
        parseEnumIdentifier(branch);
        goto pRefPtr;
    }

    expect(branch, "}");
    expect(branch, ";");
}

void parser::parseEnumIdentifier(Ast *ast) {
    Ast* branch = getBranch(ast, ast_enum_identifier);

    expectIdentifier(branch);

    if(peek(1)->getType() == EQUALS) {
        expect(branch, "=");

        parseExpression(branch);
    }
}

void parser::parseConstructor(Ast *ast) {
    Ast* branch = getBranch(ast, ast_construct_decl);

    addAccessTypes(branch);
    access_types.free();
    _current--;

    expectIdentifier(branch);

    parseUtypeArgList(branch);
    parseBlock(branch);
}

bool parser::parseTypeIdentifier(Ast* ast) {
    Ast *branch = getBranch(ast, ast_type_identifier);

    advance();
    if(isNativeType(current().getValue())){
        branch->addToken(current());
        return true;
    } else
        _current--;

    errors->enterProtectedMode();
    if(!parseReferencePointer(branch)){
        errors->pass();
    }
    else {

        errors->fail();
        return true;
    }

    return false;
}

bool parser::parseUtype(Ast* ast) {
    Ast *branch = getBranch(ast, ast_utype);

    if(parseTypeIdentifier(branch))
    {
        if(peek(1)->getType() == LEFTBRACE && peek(2)->getType() == RIGHTBRACE)
        {
            expect(branch, "[");
            expect(branch, "]");
        }

        return true;
    }
    else
        errors->createNewError(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

bool parser::parseUtypeNaked(Ast* ast) {
    Ast *branch = getBranch(ast, ast_utype);

    if(parseTypeIdentifier(branch))
    {
        return true;
    }
    else
        errors->createNewError(GENERIC, current(), "expected native type or reference pointer");

    return false;
}

void parser::parseGetter(Ast* ast) {
    if(*peek(1) == "get") {
        Ast *branch = getBranch(ast, ast_getter);
        expect(branch, "get");
        parseBlock(branch);
    }
}

void parser::parseSetter(Ast* ast) {
    if(*peek(1) == "set") {
        Ast *branch = getBranch(ast, ast_setter);
        expect(branch, "set");

        expectIdentifier(branch);
        expect(branch, "->");
        parseBlock(branch);
    }
}

void parser::parseVariableDecl(Ast* ast) {
    Ast *branch = getBranch(ast, ast_variable_decl);

    if(ast == NULL || ast->getType() != ast_variable_decl) {
        addAccessTypes(branch);
        access_types.free();

        if(isStorageType(current())) {
            branch->addToken(current());
        } else
            _current--;
    }

    expectIdentifier(branch);
    if(current() == "apples") {
        int i = 0;
    }

    if(ast == NULL || ast->getType() != ast_variable_decl) {
        if(*peek(1) == ":") {
            expect(branch, ":");
            parseUtype(branch);

            if(*peek(1) == "=") {
                advance();
                parseExpression(branch);
            }
        } else if(*peek(1) == ":=") {
            expect(branch, ":=");
            parseExpression(branch);
        }
    } else {
        if(*peek(1) == "=") {
            advance();
            parseExpression(branch);
        }
    }

    if(peek(1)->getType() == COMMA) {
        expect(branch, ",");

        parseVariableDecl(ast == NULL || ast->getType() != ast_variable_decl ? branch : ast);

        if(ast == NULL || ast->getType() != ast_variable_decl)
            expect(branch, ";");
    } else {
        expect(branch, ";");

        if (*peek(1) == "get" || *peek(1) == "set") {
            if (*peek(1) == "get") {
                parseGetter(branch);
                parseSetter(branch);
            } else {
                parseSetter(branch);
                parseGetter(branch);
            }
        }
    }

}


bool parser::isAssignExprSymbol(string token) {
    return token == "+=" || token == "-="||
           token == "*=" || token == "/="||
           token == "&=" || token == "|="||
           token == "^=" || token == "%="||
           token == "=";
}

bool parser::isForLoopCompareSymbol(string token) {
    return token == "<" || token == ">"||
           token == "<=" || token == ">=";
}

bool parser::isExprSymbol(string token) {
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
            isAssignExprSymbol(token);
}

bool parser::isOverrideOperator(string token) {
    return isAssignExprSymbol(token) ||
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

void parser::parseVectorArray(Ast* ast) {
    Ast* branch = getBranch(ast, ast_vector_array);
    expect(branch, "{");

    if(peek(1)->getType() != RIGHTCURLY)
    {
        parseExpression(branch);
        _pExpr:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");
            parseExpression(branch);
            goto _pExpr;
        }
    }

    expect(branch, "}");
}

void parser::parseDictionaryType(Ast* ast) {

    if(*peek(1) == "as") {
        Ast *branch = getBranch(ast, ast_dictionary_type);
        expect(branch, "as");

        expect(branch, "<");
        parseUtype(branch);
        expect(branch, ",");
        parseUtype(branch);
        expect(branch, ">");
    }
}

void parser::parseDictExpression(Ast* ast) {
    Ast* branch = getBranch(ast, ast_dictionary_array);
    expect(branch, "{");
    parseDictionaryType(branch);

    if(peek(1)->getType() != COLON)
    {
        parseExpression(branch);
        _pExpr:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");
            parseExpression(branch);
            goto _pExpr;
        }
    }

    expect(branch, "}");

    /*
     * { : }++ or { : }--
     * Just in case the user does some stupid shit... we shall support it
     */
    incCheck:
    if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
    {
        Ast* incBranch = getBranch(branch, ast_post_inc_e);
        advance();
        incBranch->addToken(current());
        goto incCheck;
    }
    else {
        errors->enterProtectedMode();
        Token* old = _current;
        if(!parseDotNotCallExpr(branch))
        {
            _current=old;
            errors->pass();
            branch->freeLastSub();
        } else {
            errors->fail();
        }
    }
}

bool parser::parseExpression(Ast* ast) {
    Ast *branch = getBranch(ast, ast_expression);
    CHECK_ERRLMT(return false;)

    /* ++ or -- after the expression */
    if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
    {
        advance();
        branch->addToken(current());
        Ast *exprAst = getBranch(branch, ast_pre_inc_e);
        parseExpression(exprAst);
        return true;
    }

    Token* old = _current;
    if(parsePrimaryExpr(branch)) {
        if(!isExprSymbol(peek(1)->getValue()))
            return true;
    }
    else {

        _current=old;
        if(branch->getSubAstCount() == 1) {
            branch->freeSubAsts();
            branch->freeEntities();
        }
        else {
            branch->freeLastSub();
        }
    }

    if(peek(1)->getType() == LEFTCURLY)
    {
        errors->enterProtectedMode();
        old=_current;
        advance();

        if(*peek(1) == ":" || *peek(1) == "as") {
            errors->fail();
            _current--;
            parseDictExpression(branch);
        } else {
            parseExpression(branch);
            if(*peek(1) == ":") {
                _current = old;
                errors->pass();
                branch->freeLastSub();
                parseDictExpression(branch);
            } else {
                _current = old;
                errors->pass();
                branch->freeLastSub();
                parseVectorArray(getBranch(branch, ast_vect_e));
            }

        }
        return true;
    }

    /* expression <assign-expr> expression */
    if(isAssignExprSymbol(peek(1)->getValue()))
    {
        advance();
        branch->addToken(current());

        Ast *right = new Ast(branch->getType(), branch->line, branch->col);
        parseExpression(right);
        branch->addAst(right->getLastSubAst());
        branch->encapsulate(ast_assign_e);

        if(!isExprSymbol(peek(1)->getValue()))
            return true;
    }

    bool success = binary(branch);


    /* expression '?' expression ':' expression */
    if(peek(1)->getType() == QUESMK)
    {
        advance();
        branch->addToken(current());

        parseExpression(branch);

        expect(branch, ":");

        parseExpression(branch);
        branch->encapsulate(ast_ques_e);
        return true;
    }

    //errors->createNewError(GENERIC, current(), "expected expression");
    return success;
}


bool parser::binary(Ast *ast) {
    bool success = shift(ast);

    while(match(5, AND, XOR, OR, ANDAND, OROR)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast->getType(), ast->line, ast->col);
        shift(right);
        ast->addAst(right);
        ast->encapsulate(ast_and_e);
        success = true;
    }

    return success;
}

bool parser::shift(Ast *ast) {
    bool success = equality(ast);

    while(match(2, SHL, SHR)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast->getType(), ast->line, ast->col);
        equality(right);
        ast->addAst(right);
        ast = ast->encapsulate(ast_shift_e);
        success = true;
    }

    return success;
}

bool parser::equality(Ast *ast) {
    bool success = comparason(ast);

    while(match(2, EQEQ, NOTEQ)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast->getType(), ast->line, ast->col);
        comparason(right);
        ast->addAst(right);
        ast = ast->encapsulate(ast_equal_e);
        success = true;
    }

    return success;
}

bool parser::comparason(Ast *ast) {
    bool success = addition(ast);

    while(match(4, GREATERTHAN, _GTE, LESSTHAN, _LTE)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast->getType(), ast->line, ast->col);
        addition(right);
        ast->addAst(right);
        ast = ast->encapsulate(ast_less_e);
        success = true;
    }

    return success;
}

bool parser::addition(Ast *ast) {
    bool success = multiplication(ast);

    while(match(2, MINUS, PLUS)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast_expression, ast->line, ast->col);
        multiplication(right);
        ast->addAst(right);
        ast = ast->encapsulate(ast_add_e);
        success = true;
    }

    return success;
}

bool parser::multiplication(Ast *ast) {
    bool success = unary(ast);

    while(match(3, _DIV, _MOD, MULT)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast_expression, ast->line, ast->col);
        unary(right);
        ast->addAst(right);
        ast = ast->encapsulate(ast_mult_e);
        success = true;
    }

    return success;
}

bool parser::unary(Ast *ast) {
    if(match(1, MINUS)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast->getType(), ast->line, ast->col);
        bool success = unary(right);
        ast->addAst(right);
        ast = ast->encapsulate(ast_add_e);
        return success;
    } else if(match(1, NOT)) {
        advance();
        ast->addToken(current());

        Ast *right = new Ast(ast->getType(), ast->line, ast->col);
        bool success = unary(right);
        ast->addAst(right);
        ast = ast->encapsulate(ast_not_e);
        return success;
    }

    errors->enterProtectedMode();
    Token* old = _current;
    if(!parsePrimaryExpr(ast))
    {
        errors->pass();
        _current=old;
        if(ast->getSubAstCount() == 1) {
            ast->freeSubAsts();
            ast->freeEntities();
        }
        else {
            ast->freeLastSub();
        }
        return false;
    } else
    {
        errors->fail();
        return true;
    }
}

bool parser::parseLiteral(Ast* ast) {
    Ast* branch = getBranch(ast, ast_literal);

    Token *t = peek(1);
    if(t->getId() == CHAR_LITERAL || t->getId() == INTEGER_LITERAL
       || t->getId() == STRING_LITERAL || t->getId() == HEX_LITERAL
       || t->getValue() == "true" || t->getValue() == "false")
    {
        advance();
        branch->addToken(current());
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected literal of type (string, char, hex, or bool)");
        return false;
    }
}

bool parser::parseFieldInitializatioin(Ast *ast) {
    Ast* branch = getBranch(ast, ast_field_init);

    if(peek(1)->getValue() == "base") {
        expect(branch, "base");
        expect(branch, "->");
    }

    if(parseUtypeNaked(branch) && peek(1)->getType() == EQUALS) {
        expect(branch, "=");

        if(!parseExpression(branch)){
            errors->createNewError(GENERIC, branch->getLastSubAst(), "expected expression");
        }
        return true;
    }

    return false;
}

void parser::parseFieldInitList(Ast *ast) {
    Ast *branch = getBranch(ast, ast_field_init_list);

    cout << "field init list" << endl;
    expect(branch, "{");

    if(peek(1)->getType() != RIGHTCURLY)
    {
        parseFieldInitializatioin(branch);

        _pField:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");
            if(!parseFieldInitializatioin(branch)){
                errors->createNewError(GENERIC, branch->getLastSubAst(), "expected field initializer");
            }
            goto _pField;
        }
    }

    expect(branch, "}");
}

void parser::parseExpressionList(Ast* ast, string beginChar, string endChar) {
    Ast* branch = getBranch(ast, ast_expression_list);

    expect(branch, beginChar);

    if(peek(1)->getValue() != endChar)
    {
        parseExpression(branch);

        _pValue:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");
            if(!parseExpression(branch)){
                errors->createNewError(GENERIC, branch->getLastSubAst(), "expected expression");
            }
            goto _pValue;
        }
    }

    expect(branch, endChar);
}

bool parser::parseDotNotCallExpr(Ast* ast) {
    Ast *branch = getBranch(ast, ast_dotnotation_call_expr);

    if(peek(1)->getType() == DOT)
    {
        advance();
        branch->addToken(current());
    }

    if(parseUtypeNaked(branch)) {
        if(peek(1)->getType() == LEFTPAREN) {
            parseExpressionList(branch, "(", ")");

            branch->encapsulate(ast_dot_fn_e);
        }

        /* func()++ or func()--
         * This expression rule dosen't process correctly by itsself
         * so we hav to do it ourselves
         */
        incCheck:
        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
        {
            Ast* incBranch = getBranch(branch, ast_post_inc_e);
            advance();
            incBranch->addToken(current());
            goto incCheck;
        } else if(*peek(1) == "as")
        {
            Ast* castBranch = getBranch(branch, ast_cast_e);
            expect(castBranch, "as");
            parseUtype(castBranch);
            return true;
        }
        else if(peek(1)->getType() == LEFTBRACE) {
            Ast* arrayBranch = getBranch(branch, ast_arry_e);
            advance();
            arrayBranch->addToken(current());

            parseExpression(arrayBranch);
            expect(arrayBranch, "]");

            if(peek(1)->getType() == DOT) {
                errors->enterProtectedMode();
                Token* old = _current;
                if(!parseDotNotCallExpr(arrayBranch))
                {
                    _current=old;
                    errors->pass();
                    arrayBranch->freeLastSub();
                } else {
                    errors->fail();
                }
            }
            goto incCheck;
        }
        else {
            errors->enterProtectedMode();
            Token* old = _current;
            if(!parseDotNotCallExpr(branch))
            {
                _current=old;
                errors->pass();
                branch->freeLastSub();
            } else {
                errors->fail();
            }
        }

    } else {
        _current--;
        return false;
    }

    return true;
}

bool parser::parseArrayExpression(Ast* ast) {
    Ast* branch = getBranch(ast, ast_array_expression);

    errors->enterProtectedMode();
    Token* old = _current;
    expect(branch, "[");

    if(peek(1)->getType() != RIGHTBRACE) {
        if(!parseExpression(branch))
        {
            _current=old;
            errors->pass();

            if(branch->getSubAstCount() == 1) {
                branch->freeSubAsts();
                branch->freeEntities();
            }
            else {
                branch->freeLastSub();
            }
            return false;
        } else
        {
            errors->fail();
        }
    } else {
        if (peek(2)->getType() == LEFTCURLY) {
            _current--;
            errors->pass();
            return false;
        } else {
            errors->fail();

            errors->createNewError(GENERIC, branch, "expected expression after '['");
            return false;
        }
    }

    expect(branch, "]");
    return true;
}


bool parser::parseUtypeArg(Ast* ast) {
    Ast* branch = getBranch(ast, ast_utype_arg);

    if(isVariableDecl(*peek(1)) && (*peek(2) == ":")) {
        expectIdentifier(branch);
        expect(branch, ":", false);
        parseUtype(branch);
        return true;
    } else {
        errors->createNewError(GENERIC, current(), "expected native type or reference pointer");
    }

    return false;
}

bool parser::parseLambdaArg(Ast* ast) {
    Ast* branch = getBranch(ast, ast_lambda_arg);

    if(isVariableDecl(*peek(1)) && (*peek(2) == ":")) {
        expectIdentifier(branch);
        expect(branch, ":", false);
        parseUtype(branch);
        return true;
    } else {
        if(expectIdentifier(branch)) {
            return true;
        }
    }

    return false;
}

bool parser::parseUtypeArgOpt(Ast* ast) {
    Ast* branch = getBranch(ast, ast_utype_arg);

    if(isVariableDecl(*peek(1)) && (*peek(2) == ":")) {
        expectIdentifier(branch);
        expect(branch, ":", false);
        parseUtype(branch);
        return true;
    } else {
        if(parseUtype(branch)) {
            return true;
        } else
            errors->createNewError(GENERIC, current(), "expected native type or reference pointer");
    }

    return false;
}

void parser::parseMethodReturnType(Ast *ast) {
    if(peek(1)->getType() == COLON)
    {
        Ast* branch = getBranch(ast, ast_method_return_type);
        advance();

        if(*peek(1) == "nil") {
            advance();
            branch->addToken(current());
        } else
            parseUtype(branch);
    }
}

void parser::parseLambdaReturnType(Ast *ast) {
    if(peek(1)->getType() == LEFTPAREN)
    {
        Ast* branch = getBranch(ast, ast_method_return_type);
        expect(branch, "(", false);

        if(*peek(1) == "nil") {
            advance();
            branch->addToken(current());
        } else
            parseUtype(branch);

        expect(branch, ")", false);
    }
}

void parser::parsePrototypeValueAssignment(Ast *ast) {
    advance();
    if(current() == "=" || current() == ":=")
    {
        Ast* branch = getBranch(ast, ast_value);
        branch->addToken(current());
        parseExpression(branch);
    }
    else
        _current--;
}

void parser::parsePrototypeDecl(Ast *ast, bool semicolon) {
    Ast* branch = getBranch(ast, ast_func_prototype);

    addAccessTypes(branch);
    access_types.free();

    if(isStorageType(current())) {
        if(!semicolon) {

        } else
            branch->addToken(current());
    } else
        _current--;

    expect(branch, "fn", false);
    expectIdentifier(branch);

    if(!semicolon || peek(1)->getType() == LEFTPAREN) {
        parseUtypeArgListOpt(branch);
        parseMethodReturnType(branch); // assign-expr operators must return void
    }

    if(semicolon) {

        if((*peek(1) != "=" && *peek(1) != ":=") && !branch->hasSubAst(ast_utype_arg_list_opt)) {
            errors->createNewError(GENERIC, current(), "expected `=` or `:=` after function pointer was declared");
        } else {
            if(*peek(1) == ":=") {
                if(branch->hasSubAst(ast_utype_arg_list_opt))
                    errors->createNewError(GENERIC, current(), "invalid syntax using `:=` to infer type when type of field is already known, use `=`instead.");
            } else if(*peek(1) == "=") {
                if(!branch->hasSubAst(ast_utype_arg_list_opt))
                    errors->createNewError(GENERIC, current(), "invalid syntax using `=` when type of field is not known, use `:=` instead to infer the type");
            } else {
                if(!branch->hasSubAst(ast_utype_arg_list_opt))
                    errors->createNewError(GENERIC, current(), "invalid syntax, cannot create field without typing information provided. Please specify the type, or infer it.");
            }
            parsePrototypeValueAssignment(branch);
        }
    }
    else if(isAssignExprSymbol(peek(1)->getValue()))
        errors->createNewError(GENERIC, current(), "operator `" + peek(1)->getValue() + "` on function pointer is not allowed here");

    if(semicolon)
        expect(branch, ";");
}

void parser::parseUtypeArgListOpt(Ast* ast) {
    Ast* branch = getBranch(ast, ast_utype_arg_list_opt);
    expect(branch, "(");

    if(peek(1)->getType() != RIGHTPAREN)
    {
        if(isPrototypeDecl(*peek(1))) {
            advance();
            parsePrototypeDecl(branch, false);
        }
        else
            parseUtypeArgOpt(branch);
        _puTypeArgOpt:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");

            if(isPrototypeDecl(*peek(1))){
                advance();
                parsePrototypeDecl(branch, false);
            }
            else
                parseUtypeArgOpt(branch);
            goto _puTypeArgOpt;
        }
    }

    expect(branch, ")");
}

void parser::parseUtypeArgList(Ast* ast) {
    Ast* branch = getBranch(ast, ast_utype_arg_list);
    expect(branch, "(");

    if(peek(1)->getType() != RIGHTPAREN)
    {
        if(isPrototypeDecl(*peek(1))){
            advance();
            parsePrototypeDecl(branch, false);
        }
        else
            parseUtypeArg(branch);
        _puTypeArg:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");

            if(isPrototypeDecl(*peek(1))){
                advance();
                parsePrototypeDecl(branch, false);
            }
            else
                parseUtypeArg(branch);
            goto _puTypeArg;
        }
    }

    expect(branch, ")");
}

void parser::parseLambdaArgList(Ast* ast) {
    Ast* branch = getBranch(ast, ast_lambda_arg_list);

    if(isPrototypeDecl(*peek(1))){
        advance();
        parsePrototypeDecl(branch, false);
    }
    else
        parseLambdaArg(branch);
    _pLambdaArg:
    if(peek(1)->getType() == COMMA)
    {
        expect(branch, ",");

        if(isPrototypeDecl(*peek(1))){
            advance();
            parsePrototypeDecl(branch, false);
        }
        else
            parseLambdaArg(branch);
        goto _pLambdaArg;
    }
}

bool parser::parsePrimaryExpr(Ast* ast) {
    Ast* branch = getBranch(ast, ast_primary_expr);

    errors->enterProtectedMode();
    Token* old = _current;
    if(parseLiteral(branch))
    {
        errors->fail();
        branch->encapsulate(ast_literal_e);

        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC) {
            branch = branch->getSubAst(ast_literal_e);
            goto checkInc;
        }
        else if(peek(1)->getType() == DOT)
            errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
        else if(peek(1)->getType() == LEFTBRACE)
            errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
        else if(peek(1)->getValue() == "as")
            goto asExpr;
        return true;
    }
    branch->freeLastSub();
    errors->pass();
    _current=old;

    errors->enterProtectedMode();
    if(peek(1)->getType() == DOT) {
        advance();
        branch->addToken(current());
    }

    old=_current;
    if(parseUtype(branch))
    {
        if(peek(1)->getType() == DOT && *peek(2) == "class")
        {
            expect(branch, ".");
            expect(branch, "class");

            errors->fail();
            branch->encapsulate(ast_utype_class_e);

            if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC) {
                branch = branch->getSubAst(ast_utype_class_e);
                goto checkInc;
            }
            else if(peek(1)->getType() == DOT)
                errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
            else if(peek(1)->getType() == LEFTBRACE)
                errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
            else if(peek(1)->getValue() == "as")
                goto asExpr;
            return true;
        }else {
            branch->freeLastSub();
        }
    } else
        branch->freeLastSub();
    errors->pass();
    _current=old;

    if(peek(1)->getValue() == "self")
    {
        expect(branch, "self");

        if(peek(1)->getType() == PTR) {
            expect(branch, "->");
            parseDotNotCallExpr(branch);
            branch->encapsulate(ast_self_e);
            return true;
        }

        branch->encapsulate(ast_self_e);

        if(peek(1)->getValue() != "as")
            branch = branch->getSubAst(ast_self_e);

        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            goto checkInc;
        else if(peek(1)->getType() == DOT)
            goto dotNot;
        else if(peek(1)->getType() == LEFTBRACE)
            goto arrayExpr;
        else if(peek(1)->getValue() == "as")
            goto asExpr;
        return true;
    }

    if(peek(1)->getValue() == "base")
    {
        advance();
        expect(branch, "base", "");
        expect(branch, "->");
        parseDotNotCallExpr(branch);

        branch->encapsulate(ast_base_e);
        return true;
    }

    dotNot:
    errors->enterProtectedMode();
    old=_current;
    if(parseDotNotCallExpr(branch)) {
        errors->fail();
        branch->encapsulate(ast_dot_not_e);
        if(!match(3, LEFTBRACE, _INC, _DEC))
            return true;
    } else {
        errors->pass();
        _current=old;
        branch->freeLastSub();
    }

    if(peek(1)->getValue() == "new")
    {
        expect(branch, "new");
        parseUtypeNaked(branch);
        bool newClass = false;

        if(peek(1)->getType() == LEFTBRACE && parseArrayExpression(branch)){}
        else if(peek(1)->getType() == LEFTBRACE) {
            branch->freeLastSub();
            expect(branch, "[");
            expect(branch, "]");
            parseVectorArray(branch);
        }
        else if(peek(1)->getType() == LEFTPAREN) {
            parseExpressionList(branch, "(", ")");
            newClass = true;
        } else if(peek(1)->getType() == LEFTCURLY) {
            if(peek(2)->getId() == IDENTIFIER) {
                if(peek(2)->getValue() == "base") {
                    parseFieldInitList(branch);
                } else if(peek(3)->getType() == EQUALS){
                    parseFieldInitList(branch);
                } else {
                    parseExpressionList(branch, "{", "}");
                }
            } else {
                parseExpressionList(branch, "{", "}");
            }
        } else {
            errors->createNewError(GENERIC, current(), "expected '[' or '(' or '{' after new expression");
            return true;
        }

        branch->encapsulate(ast_new_e);

        if(peek(1)->getValue() != "as")
            branch = branch->getSubAst(ast_new_e);

        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            goto checkInc;
        else if(peek(1)->getType() == DOT)
            goto dotNot;
        else if(peek(1)->getType() == LEFTBRACE)
            errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
        else if(peek(1)->getValue() == "as")
            goto asExpr;
        return true;
    }

    if(peek(1)->getValue() == "{")
    {
        errors->enterProtectedMode();
        old=_current;
        advance();

        if(peek(1)->getValue() == "->") {
            Ast* newAst = getBranch(branch, ast_lambda_function);
            advance();

            parseLambdaReturnType(newAst);
            errors->fail();
            parseLambdaBlock(newAst);
            branch = newAst;

            if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
                goto checkInc;
            else if(peek(1)->getType() == DOT)
                errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
            else if(peek(1)->getType() == LEFTBRACE)
                errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
            return true;
        } else {
            Ast* newAst = getBranch(branch, ast_lambda_function);
            parseLambdaArgList(newAst);
            if(peek(1)->getValue() == "->") {
                advance();

                parseLambdaReturnType(newAst);
                errors->fail();
                parseLambdaBlock(newAst);

                if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
                    errors->createNewError(GENERIC, current(), "unexpected symbol `" + peek(1)->getValue() + "`");
                else if(peek(1)->getType() == DOT)
                    errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
                else if(peek(1)->getType() == LEFTBRACE)
                    errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
                return true;
            } else {
                branch->freeLastSub();
                errors->pass();
                _current=old;
            }
        }
    }

    if(peek(1)->getValue() == "sizeof")
    {
        Ast* sizeofBranch = getBranch(branch, ast_sizeof_e);
        expect(sizeofBranch, "sizeof");

        expect(sizeofBranch, "(");
        parseExpression(sizeofBranch);
        expect(sizeofBranch, ")");

        if(peek(1)->getValue() != "as")
            branch = sizeofBranch;

        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            goto checkInc;
        else if(peek(1)->getType() == DOT)
            errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
        else if(peek(1)->getType() == LEFTBRACE)
            errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
        else if(peek(1)->getValue() == "as")
            goto asExpr;
        return true;
    }

    if(peek(1)->getType() == LEFTPAREN)
    {
        Ast* parenBranch = getBranch(branch, ast_paren_e);
        advance();
        parenBranch->addToken(current());

        parseExpression(parenBranch);

        expect(parenBranch, ")");
        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            goto checkInc;
        else if(peek(1)->getType() == DOT)
            goto dotNot;
        else if(peek(1)->getType() == LEFTBRACE)
            goto arrayExpr;
        else if(peek(1)->getValue() == "as")
            goto asExpr;

        return true;
    }

    if(*peek(1) == "null")
    {
        expect(branch, "null");
        branch->encapsulate(ast_null_e);

        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            errors->createNewError(GENERIC, current(), "unexpected symbol `" + peek(1)->getValue() + "`");
        else if(peek(1)->getType() == DOT)
            errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
        else if(peek(1)->getType() == LEFTBRACE)
            errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
        return true;
    }

    arrayExpr:
    if(peek(1)->getType() == LEFTBRACE)
    {
        Ast* arrayBranch = getBranch(branch, ast_arry_e);
        expect(arrayBranch, "[");
        parseExpression(arrayBranch);
        expect(arrayBranch, "]");


        if(*peek(1) == "."){
            errors->enterProtectedMode();
            old=_current;
            if(!parseDotNotCallExpr(arrayBranch)) {

                arrayBranch->freeLastSub();
                errors->pass();
                _current=old;
            }
            else {
                errors->fail();
                if(*peek(1) == "[") {
                    branch = arrayBranch;
                    goto arrayExpr;
                }
            }

            return true;
        } else if(*peek(1) == "[") {
            advance();
            errors->createNewError(GENERIC, current(), "unexpected symbol `" + peek(1)->getValue() + "`");
        }

        if(!match(2, _INC, _DEC))
            return true;
    }

    asExpr:
    if(*peek(1) == "as")
    {
        branch->encapsulate(ast_primary_expr);
        branch->encapsulate(ast_expression);
        branch->encapsulate(ast_cast_e);
        branch = branch->getSubAst(ast_cast_e);
        expect(branch, "as");
        parseUtype(branch);

        errors->fail();
        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            errors->createNewError(GENERIC, current(), "unexpected symbol `" + peek(1)->getValue() + "`");
        else if(peek(1)->getType() == DOT)
            errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
        else if(peek(1)->getType() == LEFTBRACE)
            errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
        return true;
    }

    /* ++ or -- after the expression */
    checkInc:
    if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
    {
        Ast* postIncBranch = getBranch(branch, ast_post_inc_e);
        advance();
        postIncBranch->addToken(current());

        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            goto checkInc;
        else if(peek(1)->getType() == DOT)
            goto dotNot;
        else if(peek(1)->getType() == LEFTBRACE)
            goto arrayExpr;
        else if(peek(1)->getValue() == "as")
            goto asExpr;
        return true;
    }

    return false;
}

void parser::parseMutateDecl(Ast* ast) {
    Ast *branch = getBranch(ast, ast_mutate_decl);

    if(access_types.size() > 0) {
        errors->createNewError(GENERIC, current(), "access types not allowed here");
    }
    access_types.free();

    // class name we do this for reuseability purposses on the back end
    Ast *nameAst = getBranch(branch, ast_name);
    parseReferencePointer(nameAst);

    if(peek(1)->getValue() == "base")
    {
        expect(branch, "base");
        parseReferencePointer(branch);
    }

    if(peek(1)->getValue() == ":")
    {
        expect(branch, ":");
        parseReferencePointerList(branch);
    }

    parseClassBlock(branch);
}

void parser::parseClassDecl(Ast* ast) {
    Ast *branch = getBranch(ast, ast_class_decl);
    addAccessTypes(branch);
    access_types.free();

    // class name
    expectIdentifier(branch);

    if(peek(1)->getValue() == "<") {
        branch->setAstType(ast_generic_class_decl);

        expect(branch, "<", false);
        parseIdentifierList(branch);
        expect(branch, ">", false);
    }

    if(peek(1)->getValue() == "base")
    {
        expect(branch, "base");
        parseReferencePointer(branch);
    }

    if(peek(1)->getValue() == ":")
    {
        expect(branch, ":");
        parseReferencePointerList(branch);
    }

    parseClassBlock(branch);
}

void parser::parseClassBlock(Ast *ast) {
    Ast *branch = getBranch(ast, ast_block);
    expect(ast, "{");

    int brackets = 1;
    while(!isEnd() && brackets > 0)
    {
        CHECK_ERRLMT(return;)

        advance();
        if(isAccessDecl(current()))
        {
            parseAccessTypes();
        }

        if(isModuleDecl(current()))
        {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected module declaration");
            parseModuleDecl(branch);
        }
        else if(isClassDecl(current()))
        {
            parseClassDecl(branch);
        }
        else if(isMutateDecl(current()))
        {
            parseMutateDecl(NULL);
        }
        else if(isInterfaceDecl(current()))
        {
            parseInterfaceDecl(branch);
        }
        else if(isImportDecl(current()))
        {
            if(access_types.size() > 0)
            {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            }
            errors->createNewError(GENERIC, current(), "unexpected import declaration");
            parseImportDecl(branch);
        }
        else if((isVariableDecl(current()) && (*peek(1) == ":" || *peek(1) == ":=")) ||
                (isStorageType(current()) && (isVariableDecl(*peek(1)) && (*peek(2) == ":" || *peek(2) == ":="))))
        {
            parseVariableDecl(branch);
        }
        else if(isAliasDeclaration(current()))
        {
            parseAliasDeclaration(branch);
        }
        else if(isPrototypeDecl(current()) || (isStorageType(current()) && isPrototypeDecl(*peek(1))))
        {
            parsePrototypeDecl(branch);
        }
        else if(isEnumDecl(current()))
        {
            parseEnumDecl(branch);
        }
        else if(isInitDecl(current()))
        {
            parseInitDecl(branch);
        }
        else if(isMethodDecl(current()))
        {
            if(peek(1)->getValue() == "operator")
                parseOperatorDecl(branch);
            else if(peek(1)->getValue() == "delegate")
                parseDelegateDecl(branch);
            else
                parseMethodDecl(branch);
        }
        else if(isConstructorDecl())
        {
            parseConstructor(branch);
        }
        else if(current().getType() == _EOF)
        {
            errors->createNewError(UNEXPECTED_EOF, current());
            break;
        }
        else if (current().getType() == RIGHTCURLY)
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
                    _current--;
                    break;
                }
            }
        }
        else if(current().getType() == LEFTCURLY)
            brackets++;
        else {
            // save parser state
            errors->createNewError(GENERIC, current(), "expected method, class, or variable declaration");
            parseAll(branch);
        }

        access_types.free();
    }

    if(brackets != 0)
        errors->createNewError(MISSING_BRACKET, current(), " expected `}` at end of class declaration");

    expect(branch, "}");
}

void parser::addAccessTypes(Ast *ast) {
    if(access_types.size() > 0) {
        Ast *branch = getBranch(ast, ast_access_type);
        for(int i = 0; i < access_types.size(); i++) {
            branch->addToken(access_types.get(i));
        }
    }
}

bool parser::parseReferencePointer(Ast *ast) {
    Ast *branch = getBranch(ast, ast_refrence_pointer);

    advance();
    if(current().getId() == IDENTIFIER && isKeyword(current().getValue())) {
        if(current() != "operator")
            return false;
        else if(isOverrideOperator(peek(1)->getValue())){
            _current--;
            expect(ast, "operator");
            advance();
            ast->addToken(current());
            return true;
        } else
            return false;
    } else
        _current--;

    if(!expectIdentifier(branch))
        return false;

    while(peek(1)->getType() == DOT && *peek(2) != "class") {
        expect(branch, ".");

        expectIdentifier(branch);
    }

    if(peek(1)->getValue() == "#") {
        expect(branch, "#");
        expectIdentifier(branch);
    }

    if(peek(1)->getValue() == "<") {
        expect(branch, "<");
        parseUtypeList(ast->getType() == ast_refrence_pointer ? ast : branch);
        expect(branch, ">");
    }

    while(peek(1)->getType() == DOT && *peek(2) != "class") {
        expect(branch, ".");

        expectIdentifier(branch);

        if(peek(1)->getValue() == "<") {
            expect(branch, "<");
            parseUtypeList(ast->getType() == ast_refrence_pointer ? ast : branch);
            expect(branch, ">");
        }
    }

    return true;
}

Token* parser::peek(int forward) {
    if((_current-&toks->getTokens().get(0))+forward >= toks->size())
        return &toks->getTokens().get(toks->getEntityCount()-1);
    else
        return _current+forward;
}

bool parser::isAccessDecl(Token &token) {
    return
            (token.getId() == IDENTIFIER && token.getValue() == "protected") ||
            (token.getId() == IDENTIFIER && token.getValue() == "private") ||
            (token.getId() == IDENTIFIER && token.getValue() == "static") ||
            (token.getId() == IDENTIFIER && token.getValue() == "local") ||
            (token.getId() == IDENTIFIER && token.getValue() == "const") ||
            (token.getId() == IDENTIFIER && token.getValue() == "ext") ||
            (token.getId() == IDENTIFIER && token.getValue() == "stable") ||
            (token.getId() == IDENTIFIER && token.getValue() == "public");
}

bool parser::isModuleDecl(Token &token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "mod");
}

bool parser::isImportDecl(Token &token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "import");
}

bool parser::isNativeType(string type) {
    return type == "var" || type == "object"
           || type == "_int8" || type == "_int16"
           || type == "_int32" || type == "_int64"
           || type == "_uint8" || type == "_uint16"
           || type == "_uint32" || type == "_uint64";
}

bool parser::isVariableDecl(Token &token) {
    return (!isKeyword(token.getValue()) && token.getId() == IDENTIFIER);
}

bool parser::isClassDecl(Token &token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "class");
}

bool parser::isMutateDecl(Token &token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "mutate");
}

bool parser::isEnumDecl(Token &token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "enum");
}

bool parser::isStorageType(Token &token) {
    return (token.getId() == IDENTIFIER && (token.getValue() == "thread_local"));
}

bool parser::isInterfaceDecl(Token &token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "interface");
}

bool parser::isPrototypeDecl(Token& token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "fn");
}

bool parser::isMethodDecl(Token& token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "def");
}

bool parser::isInitDecl(Token& token) {
    return (token.getId() == IDENTIFIER && token.getValue() == "init");
}

bool parser::isReturnStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "return");
}

bool parser::isIfStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "if");
}

bool parser::isSwitchStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "switch");
}

bool parser::isAssemblyStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "asm");
}

bool parser::isForStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "for");
}

bool parser::isAliasDeclaration(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "alias");
}

bool parser::isSwitchDeclarator(Token& t) {
    return t.getId() == IDENTIFIER && (t.getValue() == "case" || t.getValue() == "default");
}

bool parser::isConstructorDecl() {
    return current().getId() == IDENTIFIER && !isKeyword(current().getValue()) &&
           peek(1)->getType() == LEFTPAREN;
}

bool parser::isKeyword(string key) {
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
           || key == "switch" || key == "default" || key == "fn" || key == "local"
           || key == "thread_local" || key == "nil" || key == "ext"  || key == "stable"
           || key == "mutate" || key == "init" || key == "get" || key == "set" || key == "alias"
           || key == "as";
}

void parser::parseAccessTypes() {
    while(isAccessDecl(current()))
    {
        access_types.push_back(current());
        advance();
    }
}

bool parser::expectIdentifier(Ast* ast) {
    advance();

    if(current().getId() == IDENTIFIER && !isKeyword(current().getValue()))
    {
        if(ast != NULL)
            ast->addToken(current());
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected identifier");
    }
    return false;
}

void parser::parseModuleDecl(Ast *ast) {
    Ast *branch = getBranch(ast, ast_module_decl);

    expectIdentifier(branch);

    while(peek(1)->getType() == DOT) {
        expect(branch, ".");

        expectIdentifier(branch);
    }

    expect(branch, ";", false);
}

void parser::parseIdentifierList(Ast *ast) {
    Ast *branch = getBranch(ast, ast_identifier_list);

    expectIdentifier(branch);
    while(peek(1)->getType() == COMMA) {
        expect(branch, ",");

        expectIdentifier(branch);
    }
}

void parser::parseReferencePointerList(Ast *ast) {
    Ast *branch = getBranch(ast, ast_reference_pointer_list);

    parseReferencePointer(branch);
    while(peek(1)->getType() == COMMA) {
        expect(branch, ",");

        parseReferencePointer(branch);
    }
}

void parser::parseUtypeList(Ast *ast) {
    Ast *branch = getBranch(ast, ast_utype_list);

    parseUtype(branch);
    while(peek(1)->getType() == COMMA) {
        expect(branch, ",");

        parseUtype(branch);
    }
}

void parser::parseImportDecl(Ast *ast) {
    Ast *branch = getBranch(ast, ast_import_decl);

    expectIdentifier(branch);

    while(peek(1)->getType() == DOT) {
        expect(branch, ".");

        if(peek(1)->getType() == MULT) {
            expect(branch, "*");
            break;
        } else
            expectIdentifier(branch);
    }

    expect(branch, ";", false);
}

void parser::expect(Ast* ast, string token, bool addToken, const char *expectedstr) {
    advance();

    if(current().getValue() == token)
    {
        if(addToken)
            ast->addToken(current());
    }
    else {
        if(expectedstr != nullptr)
            errors->createNewError(GENERIC, current(), "expected " + string(expectedstr));
        else
            errors->createNewError(GENERIC, current(), "expected `" + token + "`");
    }
}

Ast * parser::getBranch(Ast *ast, ast_type type) {
    Ast *branch = new Ast(type, current().getLine(),
            current().getColumn());

    if(ast == NULL)
    {
        tree.push_back(branch);
        ast_cursor++;

        return astAt(ast_cursor);
    }
    else {
        ast->addAst(branch);

        return branch;
    }
}

ErrorManager *parser::getErrors() {
    return errors;
}

void parser::free() {
    this->toks = NULL;
    this->_current = NULL;

    if(this->tree.size() > 0) {
        /*
         * free ast tree
         */
        Ast* pAst;
        for(int64_t i = 0; i < this->tree.size(); i++)
        {
            pAst = tree.get(i);
            pAst->free();
            delete(pAst);
        }

        ast_cursor = 0;
        access_types.free();
        this->tree.free();
        access_types.free();
        errors->free();
        delete (errors); this->errors = NULL;
    }
}

bool parser::match(int num_args, ...) {
    va_list ap;
    bool found = false;

    va_start(ap,num_args);
    for(size_t loop=0;loop<num_args;++loop) {
        if(peek(1)->getType() == va_arg(ap,int)) {
            found = true;
            break;
        }
    }

    va_end(ap);
    return found;
}

Ast *parser::astAt(long p) {
    return tree.at(p);
}
