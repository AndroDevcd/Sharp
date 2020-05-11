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

    errors = new ErrorManager(&toks->getLines(), toks->file, false, c_options.aggressive_errors);
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

//    cout << branch->toString() << endl;
//    cout << std::flush;
//    int i = 0;
}

void parser::parseMethodDecl(Ast *ast) {
    Ast* branch = getBranch(ast, ast_method_decl);

    addAccessTypes(branch);
    access_types.free();

    parseReferencePointer(branch);
    if(isOverrideOperator(current().getValue()))
    {
        errors->createNewError(GENERIC, current(), "expected identifier");
    }

    parseUtypeArgList(branch);
    if (peek(1)->getType() == COLON) {
        parseMethodReturnType(branch);

        if (peek(1)->getType() == EQUALS) {
            expect(branch, "=", true);
            parseExpression(branch);
            expect(branch, ";", false);
        } else if(*peek(1) == ";") {
            expect(branch, ";", false);
            branch->setAstType(ast_delegate_decl);
        } else {
            parseBlock(branch);
            if(branch->getLastSubAst()->sub_asts.singular() // TODO: talk about in next episode
               && branch->getLastSubAst()->sub_asts.get(0)->getSubAst(0)->getType() == ast_expression) {
                errors->createNewError(GENERIC, current(), "expected `=` before expression");
            }
        }
    } else if (peek(1)->getType() == INFER) {
        expect(branch, ":=", true);
        parseExpression(branch);
        expect(branch, ";", false);
    } else if(*peek(1) == ";") {
        expect(branch, ";", false);
        branch->setAstType(ast_delegate_decl);
    }
    else {
        parseBlock(branch);
        if(branch->getLastSubAst()->sub_asts.singular()
            && branch->getLastSubAst()->sub_asts.get(0)->getSubAst(0)->getType() == ast_expression) {
            errors->createNewError(GENERIC, current(), "expected `=` before expression");
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
                errors->createNewError(UNEXPECTED_SYMBOL, current(), "'}'");
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

    if(branch->getToken(0).getValue() == "case")
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
                branch->freeTokens();
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

void parser::parseForStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_for_statement);

    _current--;
    expect(branch, "for");

    expect(branch, "(");

    if(peek(1)->getType() != SEMICOLON) {
        _current++;
        parseVariableDecl(branch);
    }

    if(peek(1)->getType() != SEMICOLON) {
        Ast *exprCond = getBranch(branch, ast_for_expresion_cond);
        parseExpression(exprCond);
    }
    expect(branch, ";");

    if(peek(1)->getType() != RIGHTPAREN) {
        Ast *exprIter = getBranch(branch, ast_for_expresion_iter);
        parseExpression(exprIter);
    }

    expect(branch, ")");

    parseBlock(branch);
}

void parser::parseWhileStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_while_statement);

    _current--;
    expect(branch, "while", false);

    expect(branch, "(", false);
    parseExpression(branch);
    expect(branch, ")", false);

    parseBlock(branch);
}

void parser::parseDoWhileStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_do_while_statement);

    _current--;
    expect(branch, "do", false);
    parseBlock(branch);

    expect(branch, "while", false);
    expect(branch, "(", false);
    parseExpression(branch);
    expect(branch, ")", false);
    expect(branch, ";", false);
}

void parser::parseThrowStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_throw_statement);

    _current--;
    expect(branch, "throw", false);
    parseExpression(branch);
    expect(branch, ";", false);
}

void parser::parseGotoStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_goto_statement);

    _current--;
    expect(branch, "goto", false);
    expectIdentifier(branch);
    expect(branch, ";", false);
}

void parser::parseWhenStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_when_statement);

    _current--;
    expect(branch, "when", false);
    if(peek(1)->getValue() == "(") {
        expect(branch, "(", false);
        parseExpression(branch);
        expect(branch, ")", false);
    }

    parseWhenBlock(branch);
}

void parser::parseAsmStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_assembly_statement);

    _current--;
    expect(branch, "asm", false);
    if(peek(1)->getValue() == "volatile") {
        expect(branch, "volatile");
    }

    parseAsmBlock(branch);
}

void parser::parseAssemblyInstruction(Ast *ast) {
    Ast* branch = getBranch(ast, ast_assembly_instruction);

    if(*peek(1) == "nop") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "int") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "movi") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "ret") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "hlt") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "newVarArray") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "cast") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "mov8") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "mov16") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "mov32") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "mov64") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "movu8") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "movu16") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "movu32") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "movu64") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "rstore") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "add") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    }  else if(*peek(1)  == "sub") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "mul") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "div") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "mod") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "iadd") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "isub") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "idiv") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "imod") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "pop") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "inc") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "dec") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "movr") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "iaload") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "brh") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "ife") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "ifne") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "lt") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "gt") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "le") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "ge") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "movl") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "movsl") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "sizeof") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "put") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "putc") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "checklen") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "jmp") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "loadpc") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "pushobj") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "del") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "call") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "newClass") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "movn") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "sleep") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "te") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "tne") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "lock") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "ulock") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "movg") {
        expect(branch, peek(1)->getValue());
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "movnd") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "newObjArray") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "not") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "skip") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "loadVal") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "shl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "shr") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "skipife") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "skipifne") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "and") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "uand") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "or") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "xor") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "throw") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "checknull") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "returnObj") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "newClassArray") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "newString") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "addl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "subl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "mull") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "divl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "modl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseAsmLiteral(branch);
    } else if(*peek(1)  == "iaddl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "isubl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "idivl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "imull") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "imodl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "loadl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "popObject") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "smovr") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "andl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "orl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "xorl") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "rmov") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == "smov") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "returnVal") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "istore") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "smovr2") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "smovr3") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "istorel") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "popl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "pushNull") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "ipushl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "pushl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "itest") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "invokeDelegate") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "get") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "isadd") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "je") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "jne") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "ipopl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "cmp") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "calld") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
    } else if(*peek(1)  == "varCast") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "tlsMovl") {
        expect(branch, peek(1)->getValue());
        parseLiteral(branch);
    } else if(*peek(1)  == "dup") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "popObj2") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "swap") {
        expect(branch, peek(1)->getValue());
    } else if(*peek(1)  == "ldc") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseLiteral(branch);
    } else if(*peek(1)  == "neg") {
        expect(branch, peek(1)->getValue());
        parseRegister(branch);
        expect(branch, ",", false);
        parseRegister(branch);
    } else if(*peek(1)  == ".") {
        expect(branch, peek(1)->getValue());
        expectIdentifier(branch);
        expect(branch, ":");
        parseAssemblyInstruction(branch);
    } else {
        errors->createNewError(GENERIC, current(), "expected assembly instruction");
    }
}

void parser::parseAsmBlock(Ast *ast) {
    Ast* branch = getBranch(ast, ast_assembly_block);

    expect(branch, "{");

    parseAssemblyInstruction(branch);

    _pCatch:
    if(iaAssemblyInstruction(peek(1)->getValue()))
    {
        parseAssemblyInstruction(branch);
        goto _pCatch;
    }

    expect(branch, "}");
}

void parser::parseWhenBlock(Ast *ast) {
    Ast* branch = getBranch(ast, ast_when_block);

    expect(branch, "{", false);

    pWhenClause:
    if(peek(1)->getValue() == "else") {
        Ast* whenClause = getBranch(branch, ast_when_else_clause);

        expect(whenClause, "else", false);
        expect(whenClause, "->", false);
        parseBlock(whenClause);
        goto endOfClause;
    } else {
        Ast* whenClause = getBranch(branch, ast_when_clause);

        parseExpression(whenClause);
        while(peek(1)->getValue() == ",") {
            expect(whenClause, ",", false);
            parseExpression(whenClause);
        }

        expect(whenClause, "->", false);
        parseBlock(whenClause);
    }

    if(peek(1)->getValue() != "}") {
        goto pWhenClause;
    }

    endOfClause:
    if(!branch->hasSubAst(ast_when_clause)) {
        errors->createNewError(GENERIC, current(), "expected when clause");
    }

    expect(branch, "}", false);
}

void parser::parseBreakStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_break_statement);

    _current--;
    expect(branch, "break", false);
    expect(branch, ";", false);
}

void parser::parseContinueStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_continue_statement);

    _current--;
    expect(branch, "continue", false);
    expect(branch, ";", false);
}

void parser::parseLockStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_lock_statement);

    _current--;
    expect(branch, "lock", false);
    parseExpression(branch);
    expect(branch, "->", false);
    parseBlock(branch);
}

void parser::parseCatchClause(Ast *ast) {
    Ast *branch = getBranch(ast, ast_catch_clause);

    expect(branch, "catch", false);

    expect(branch, "(", false);
    parseUtypeArgOpt(branch);
    expect(branch, ")", false);

    parseBlock(branch);
}

void parser::parseCatchChain(Ast *ast) {
    if(*peek(1) == "catch")
    {
        parseCatchClause(ast);

        _pCatch:
        if(peek(1)->getValue() == "catch")
        {
            parseCatchClause(ast);
            goto _pCatch;
        }
    }
}

void parser::parseFinallyBlock(Ast *ast) {
    if(*peek(1) == "finally")
    {
        Ast *branch = getBranch(ast, ast_finally_block);

        expect(branch, "finally", false);
        parseBlock(branch);
    }
}

void parser::parseTryCatchStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_trycatch_statement);

    _current--;
    expect(branch, "try", false);
    parseBlock(branch);
    parseCatchChain(branch);
    parseFinallyBlock(branch);

    if(!branch->hasSubAst(ast_catch_clause) && !branch->hasSubAst(ast_finally_block)) {
        errors->createNewError(GENERIC, current(), "expected `catch` or `finally`");
    }
}

void parser::parseForEachStatement(Ast *ast) {
    Ast* branch = getBranch(ast, ast_foreach_statement);

    _current--;
    expect(branch, "foreach");

    expect(branch, "(");
    if(*peek(2) == ":")
        parseUtypeArg(branch);
    else {
        Ast* arg = getBranch(branch, ast_utype_arg);
        expectIdentifier(arg);
    }

    expect(branch, "in", false);
    parseExpression(branch);
    expect(branch, ")");

    parseBlock(branch);
}

void parser::parseLabelDecl(Ast* ast) {
    Ast *branch = getBranch(ast, ast_label_decl);

    _current--;
    expectIdentifier(branch);
    expect(ast, ":", false);
    parseBlock(branch);
}

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
    else if(isForStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseForStatement(branch);
        return true;
    }
    else if(isAliasDeclaration(current()))
    {
        parseAliasDeclaration(branch);
        return true;
    }
    else if((isVariableDecl(current()) && (*peek(1) == ":" || *peek(1) == ":=")) ||
       (isStorageType(current()) && (isVariableDecl(*peek(1)) && (*peek(2) == ":" || *peek(2) == ":="))))
    {
        if(*peek(1) == ":") {
            Token *old = _current;
            _current++;

            errors->enterProtectedMode();
            parseUtype(branch);
            branch->freeLastSub();
            errors->pass();

            if(*peek(1) == "=" || *peek(1) == ";"
                || *peek(1) == ",") {
                _current = old;
                parseVariableDecl(branch);
            } else {
                _current = old;
                goto labelDecl;
            }
        } else
            parseVariableDecl(branch);
    }
    else if(isForEachStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseForEachStatement(branch);
        return true;
    }
    else if(isWhileStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseWhileStatement(branch);
        return true;
    }
    else if(isDoWhileStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseDoWhileStatement(branch);
        return true;
    }
    else if(isThrowStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseThrowStatement(branch);
        return true;
    }
    else if(isGotoStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseGotoStatement(branch);
        return true;
    }
    else if(isBreakStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseBreakStatement(branch);
        return true;
    }
    else if(isContinueStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseContinueStatement(branch);
        return true;
    }
    else if(isLockStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseLockStatement(branch);
        return true;
    }
    else if(isTryCatchStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseTryCatchStatement(branch);
        return true;
    }
    else if(isWhenStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseWhenStatement(branch);
        return true;
    }
    else if(isAsmStatement(current()))
    {
        if(access_types.size() > 0)
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());

        parseAsmStatement(branch);
        return true;
    }
    else if(current().getType() == SEMICOLON)
    {
        if(access_types.size() > 0) {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, branch);
            access_types.free();
        }
        errors->createNewWarning(GENERIC, current().getLine(), current().getColumn(), "unnecessary semicolon ';'");
        return true;
    }
    else
    {
        /*
         * label decl?
         */
        if(current().getId() == IDENTIFIER && !isKeyword(current().getValue())
            && peek(2)->getType() == COLON)
        {
            labelDecl:
            if(access_types.size() > 0) {
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, branch);
                access_types.free();
            }

            parseLabelDecl(branch);
            return true;
        }

        errors->enterProtectedMode();
        if(access_types.size() > 0) {
            errors->createNewError(ILLEGAL_ACCESS_DECLARATION, branch);
            access_types.free();
        }

        _current--;
        if(!parseExpression(branch))
        {
            errors->pass();
            advance()
            branch->freeLastSub();
            errors->createNewError(GENERIC, branch, "not a statement");
            return false;
        } else {

            errors->fail();
            expect(branch, ";", false);
            return true;
        }
    }

    return false;
}

void parser::parseOperatorDecl(Ast *ast) {
    Ast* branch = getBranch(ast, ast_operator_decl);

    addAccessTypes(branch);
    access_types.free();
    expect(branch, "operator");
    expectOverrideOperator(branch);

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
            parseMutateDecl(branch);
        }
        else if(isImportDecl(current()))
        {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected import declaration");
            parseImportDecl(branch);
        }
        else if(current() == "get") {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected get declaration");
            parseGetter(ast);
        } else if(current() == "set") {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected set declaration");
            parseSetter(ast);
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
        else if(isMethodDecl(current()))
        {
            if(peek(1)->getValue() == "operator") {
                if(access_types.size() > 0)
                {
                    errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
                }
                errors->createNewError(GENERIC, current(), "unexpected operator declaration");
                parseOperatorDecl(branch);
            } else {
                parseMethodDecl(branch);

                if(branch->getLastSubAst()->getType() != ast_delegate_decl)
                    errors->createNewError(GENERIC, current(), "unexpected method declaration");
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
    else if(current() == "get") {
        _current--;
        parseGetter(ast);
    } else if(current() == "set") {
        _current--;
        parseSetter(ast);
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
    Ast* branch = getBranch(ast, ast_init_decl);

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
    pEnumIdentifier:
    if(peek(1)->getType() == COMMA)
    {
        expect(branch, ",");
        parseEnumIdentifier(branch);
        goto pEnumIdentifier;
    }

    expect(branch, "}");
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

bool parser::parseFunctionPtr(Ast* ast) {
    Ast *branch = getBranch(ast, ast_func_ptr);

    if(peek(1)->getType() == LEFTPAREN) {
        parseUtypeArgListOpt(branch);

        if(peek(1)->getType() == LEFTPAREN) {
            expect(branch, "(", false);

            if(peek(1)->getType() != RIGHTPAREN) {
                Ast *returnType = getBranch(branch, ast_method_return_type);

                if(*peek(1) == "nil")
                    expect(returnType, "nil");
                else
                    parseUtype(returnType);
            }

            expect(branch, ")", false);
            return true;
        }
    }

    return false;
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
    Token *old=_current;
    if(parseFunctionPtr(branch)) {
        errors->fail();
        return true;
    } else {
        errors->pass();
        branch->freeLastSub();
        _current = old;
    }

    return parseReferencePointer(branch);
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

long recursion = 0;
void parser::parseVariableDecl(Ast* ast) {
    Ast *branch = getBranch(ast, ast_variable_decl);
    recursion++;

    if(recursion == 1) {
        addAccessTypes(branch);
        access_types.free();

        if(isStorageType(current())) {
            branch->addToken(current());
        } else
            _current--;
    }

    expectIdentifier(branch);
    if(recursion == 1) {
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

//            cout << branch->getLastSubAst()->toString() << endl;
//            cout << std::flush;
//            int i = 0;
        }
    } else {
        if(*peek(1) == "=") {
            advance();
            parseExpression(branch);
        }
    }

    if(peek(1)->getType() == COMMA) {
        expect(branch, ",");

        parseVariableDecl(recursion <= 1 ? branch : ast);

        if(recursion == 1)
            expect(branch, ";");
    } else {
        if(recursion == 1)
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

    recursion--;
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
           token == "**" ||
            isAssignExprSymbol(token);
}

bool parser::isOverrideOperator(string token) {
    return isAssignExprSymbol(token) ||
           token == "++" ||token == "--" ||
           token == "*" || token == "/" ||
           token == "%" || token == "-" ||
           token == "+" || token == "==" ||
           token == ">>" || token == "<<"||
           token == "<" || token == ">"||
           token == "<=" || token == ">="||
           token == "!=" || token == "!"||
           token == "[" || token == "**";
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
            expect(branch, ",", false);
            parseExpression(branch);
            goto _pExpr;
        }
    }

    expect(branch, "}");
}

void parser::parseDictionaryType(Ast* ast) {

    if(*peek(1) == "as") {
        Ast *branch = getBranch(ast, ast_dictionary_type);
        expect(branch, "as", false);

        expect(branch, "<", false);
        parseUtype(branch);
        expect(branch, ",", false);
        parseUtype(branch);
        expect(branch, ">", false);
    }
}

void parser::parseDictElement(Ast* ast) {
    Ast* branch = getBranch(ast, ast_dictionary_element);

    parseExpression(branch);
    expect(branch, ":", false);
    parseExpression(branch);
}

void parser::parseDictExpression(Ast* ast) {
    Ast* branch = getBranch(ast, ast_dictionary_array);
    expect(branch, "{");
    parseDictionaryType(branch);

    if(peek(1)->getType() != COLON)
    {
        parseDictElement(branch);
        _pExpr:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",", false);
            parseDictElement(branch);
            goto _pExpr;
        }
    } else
        expect(branch, ":", false);

    expect(branch, "}");

    /*
     * { : }++ or { : }-- { 200 : "hello" }[200]
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
        } else if(peek(1)->getType() == LEFTPAREN) {
            parseExpressionList(branch, "(", ")");

            branch->encapsulate(ast_dot_fn_e);
        }
        goto incCheck;
    }
    else {
        if(peek(1)->getType() == DOT) {
            errors->enterProtectedMode();
            Token *old = _current;
            if (!parseDotNotCallExpr(branch)) {
                _current = old;
                errors->pass();
                branch->freeLastSub();
            } else {
                errors->fail();
            }
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
        Ast *exprAst = getBranch(branch, ast_pre_inc_e);
        exprAst->addToken(current());
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
        branch->freeLastSub();
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

    if(!success) errors->createNewError(GENERIC, current(), "expected expression");
    return success;
}

// [2] + [3]
bool parser::binary(Ast *ast) {
    bool success = equality(ast);

    while(match(5, AND, XOR, OR, ANDAND, OROR)) {
        if(isExprSymbol(peek(2)->getValue()))
            errors->createNewError(GENERIC, *peek(2), "expected expression");

        if(!ast->sub_asts.empty()) {
            Ast *left = ast->sub_asts.last();
            ast->sub_asts.pop_back();
            Ast *branch = getBranch(ast, ast_and_e);
            branch->addAst(left);

            advance();
            branch->addToken(current());
            equality(branch);
            success = true;
        } else return false;
    }

    return success;
}

bool parser::equality(Ast *ast) {
    bool success = comparason(ast);

    while(match(2, EQEQ, NOTEQ)) {
        if(isExprSymbol(peek(2)->getValue()))
            errors->createNewError(GENERIC, *peek(2), "expected expression");

        if(!ast->sub_asts.empty()) {
            Ast *left = ast->sub_asts.last();
            ast->sub_asts.pop_back();
            Ast *branch = getBranch(ast, ast_equal_e);
            branch->addAst(left);

            advance();
            branch->addToken(current());
            comparason(branch);
            success = true;
        } else return false;
    }

    return success;
}

bool parser::comparason(Ast *ast) {
    bool success = shift(ast);

    while(match(4, GREATERTHAN, _GTE, LESSTHAN, _LTE)) {
        if(isExprSymbol(peek(2)->getValue()))
            errors->createNewError(GENERIC, *peek(2), "expected expression");

        if(!ast->sub_asts.empty()) {
            Ast *left = ast->sub_asts.last();
            ast->sub_asts.pop_back();
            Ast *branch = getBranch(ast, ast_less_e);
            branch->addAst(left);

            advance();
            branch->addToken(current());
            shift(branch);
            success = true;
        } else return false;
    }

    return success;
}

bool parser::shift(Ast *ast) {
    bool success = addition(ast);

    while(match(2, SHL, SHR)) {
        if(isExprSymbol(peek(2)->getValue()))
            errors->createNewError(GENERIC, *peek(2), "expected expression");

        if(!ast->sub_asts.empty()) {
            Ast *left = ast->sub_asts.last();
            ast->sub_asts.pop_back();
            Ast *branch = getBranch(ast, ast_shift_e);
            branch->addAst(left);

            advance();
            branch->addToken(current());
            addition(branch);
            success = true;
        } else return true;
    }

    return success;
}

bool parser::addition(Ast *ast) {
    bool success = multiplication(ast);

    while(match(2, MINUS, PLUS)) {
        if(isExprSymbol(peek(2)->getValue()))
            errors->createNewError(GENERIC, *peek(2), "expected expression");

        if(!ast->sub_asts.empty()) {
            Ast *left = ast->sub_asts.last();
            ast->sub_asts.pop_back();

            Ast *branch = getBranch(ast, ast_add_e);
            branch->addAst(left);


            advance();
            branch->addToken(current());
            multiplication(branch);
            success = true;
        } else return false;
    }

    return success;
}

bool parser::multiplication(Ast *ast) {
    bool success = exponent(ast);

    while(match(3, _DIV, _MOD, MULT)) {
        if(isExprSymbol(peek(2)->getValue()))
            errors->createNewError(GENERIC, *peek(2), "expected expression");

        if(!ast->sub_asts.empty()) {
            Ast *left = ast->sub_asts.last();
            ast->sub_asts.pop_back();
            Ast *branch = getBranch(ast, ast_mult_e);
            branch->addAst(left);

            advance();
            branch->addToken(current());
            exponent(branch);
            success = true;
        } else return false;
    }

    return success;
}

bool parser::exponent(Ast *ast) {
    bool success = unary(ast);

    while(match(1, EXPONENT)) {
        if(isExprSymbol(peek(2)->getValue()))
            errors->createNewError(GENERIC, *peek(2), "expected expression");

        if(!ast->sub_asts.empty()) {
            Ast *left = ast->sub_asts.last();
            ast->sub_asts.pop_back();
            Ast *branch = getBranch(ast, ast_exponent_e);
            branch->addAst(left);

            advance();
            branch->addToken(current());
            unary(branch);
            success = true;
        } else return false;
    }


    return success;
}

bool parser::unary(Ast *ast) {
    Ast *branch = getBranch(ast, ast_expression);

    if(match(1, MINUS)) {
        advance();
        branch->addToken(current());

        bool success = unary(branch);
        branch->setAstType(ast_minus_e);
        return success;
    } else if(match(1, NOT)) {
        advance();
        branch->addToken(current());

        bool success = unary(branch);
        branch->setAstType(ast_minus_e);
        return success;
    }

    errors->enterProtectedMode();
    Token *old = _current;
    if (!parsePrimaryExpr(branch)) {
        errors->pass();
        _current = old;
        ast->freeLastSub();
        return false;
    } else {
        errors->fail();
        return true;
    }
}

bool parser::parseAsmLiteral(Ast* ast) {
    Ast* branch = getBranch(ast, ast_assembly_literal);

    Token *t = peek(1);
    if(t->getId() == CHAR_LITERAL || t->getId() == INTEGER_LITERAL
       || t->getId() == STRING_LITERAL || t->getId() == HEX_LITERAL
       || t->getValue() == "true" || t->getValue() == "false")
    {
        advance();
        branch->addToken(current());
        return true;
    }
    else if(t->getType() == AT) { // label
        expect(branch, "@");
        expectIdentifier(branch);
        if(peek(1)->getType() == PLUS || peek(1)->getType() == MINUS) {
            advance();
            branch->addToken(current());

            if(t->getId() == INTEGER_LITERAL
               || t->getId() == HEX_LITERAL)
            {
                advance();
                branch->addToken(current());
                return true;
            } else {
                errors->createNewError(GENERIC, current(), "expected literal of type (integer, hex)");
                return false;
            }
        }
        return true;
    } else if(t->getType() == LEFTCURLY) { // any
        expect(branch, "{");
        parseUtype(branch);

        if(peek(1)->getType() == PLUS || peek(1)->getType() == MINUS) {
            advance();
            branch->addToken(current());

            if(t->getId() == INTEGER_LITERAL
               || t->getId() == HEX_LITERAL)
            {
                advance();
                branch->addToken(current());
                return true;
            } else {
                errors->createNewError(GENERIC, current(), "expected literal of type (integer, hex)");
                return false;
            }
        }
        expect(branch, "}");
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected literal of type (integer, char, hex, bool, or label)");
        return false;
    }
}

bool parser::parseRegister(Ast* ast) {
    Ast* branch = getBranch(ast, ast_assembly_register);

    if(peek(1)->getValue() == "%")
        expect(branch, "%", false);

    Token *reg = peek(1);
    if(to_lower(reg->getValue()) == "adx"
        || to_lower(reg->getValue()) == "cx"
        || to_lower(reg->getValue()) == "cmt"
        || to_lower(reg->getValue()) == "ebx"
        || to_lower(reg->getValue()) == "ecx"
        || to_lower(reg->getValue()) == "ecf"
        || to_lower(reg->getValue()) == "edf"
        || to_lower(reg->getValue()) == "ehf"
        || to_lower(reg->getValue()) == "bmr"
        || to_lower(reg->getValue()) == "egx")
    {
        advance();
        branch->addToken(current());
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected literal of type (integer, char, hex, or bool)");
        return false;
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
        errors->createNewError(GENERIC, current(), "expected literal of type (integer, string, char, hex, or bool)");
        return false;
    }
}

void parser::parseBaseClassUtype(Ast *ast) {
    if(peek(1)->getType() == AT) {
        Ast *branch = getBranch(ast, ast_base_utype);
        expect(branch, "@", false);
        parseUtype(branch);
    }
}

bool parser::parseFieldInitializatioin(Ast *ast) {
    Ast* branch = getBranch(ast, ast_field_init);

    if(peek(1)->getValue() == "base") {
        expect(branch, "base");
        parseBaseClassUtype(branch);
        expect(branch, "->");
    }

    if(parseUtypeNaked(branch) && peek(1)->getType() == EQUALS) {
        expect(branch, "=");

        parseExpression(branch);
        return true;
    }

    return false;
}

void parser::parseFieldInitList(Ast *ast) {
    Ast *branch = getBranch(ast, ast_field_init_list);

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
            parseExpression(branch);
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

        /* func()++ or func()-- or func()[0] or func().someField
         * This expression rule dosen't process correctly by itsself
         * so we hav to do it ourselves
         */
        incCheck:
        if(peek(1)->getType() == LEFTPAREN) { // TODO: talk about this in next episode
            Ast* funCallBranch = getBranch(branch, ast_dot_fn_e);
            parseExpressionList(funCallBranch, "(", ")");
            goto incCheck;
        }
        else if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
        {
            Ast* incBranch = getBranch(branch, ast_post_inc_e);
            advance();
            incBranch->addToken(current());
            goto incCheck;
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
            } else if(peek(1)->getType() == LEFTPAREN) {
                parseExpressionList(branch, "(", ")");

                branch->encapsulate(ast_dot_fn_e);
            }
            goto incCheck;
        }
        else {
            if(peek(1)->getType() == DOT) {
                errors->enterProtectedMode();
                Token *old = _current;
                if (!parseDotNotCallExpr(branch)) {
                    _current = old;
                    errors->pass();
                    branch->freeLastSub();
                } else {
                    errors->fail();
                }
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
        errors->createNewError(GENERIC, current(), "expected variable declaration");
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
        return parseUtype(branch);
    }
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
void parser::parseUtypeArgListOpt(Ast* ast) {
    Ast* branch = getBranch(ast, ast_utype_arg_list_opt);
    expect(branch, "(");

    if(peek(1)->getType() != RIGHTPAREN)
    {
        parseUtypeArgOpt(branch);
        _puTypeArgOpt:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");

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
        parseUtypeArg(branch);

        _puTypeArg:
        if(peek(1)->getType() == COMMA)
        {
            expect(branch, ",");

            parseUtypeArg(branch);
            goto _puTypeArg;
        }
    }

    expect(branch, ")");
}

void parser::parseLambdaArgList(Ast* ast) {
    Ast* branch = getBranch(ast, ast_lambda_arg_list);

    parseLambdaArg(branch);

    _pLambdaArg:
    if(peek(1)->getType() == COMMA)
    {
        expect(branch, ",", false);
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
            branch = branch->getLastSubAst();
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
                errors->createNewError(GENERIC, current(), "unexpected symbol `" + peek(1)->getValue() + "`");
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
            branch = branch->getLastSubAst();

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
        expect(branch, "base");
        parseBaseClassUtype(branch);

        if(*peek(1) == "->") {
            expect(branch, "->");
            parseDotNotCallExpr(branch);
        }

        branch->encapsulate(ast_base_e);

        if(peek(1)->getValue() != "as")
            branch = branch->getLastSubAst();

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

    dotNot:
    errors->enterProtectedMode();
    old=_current;
    if(parseDotNotCallExpr(branch)) {
        errors->fail();
        Ast *branchToStore = branch->sub_asts.last();
        Ast* dotNotBranch = getBranch(branch, ast_dot_not_e);
        dotNotBranch->addAst(branchToStore);
        branch->sub_asts.removeAt(branch->sub_asts.size() - 2);

        if(peek(1)->getValue() != "as")
            branch = branch->getLastSubAst();

        if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            goto checkInc;
        else if(peek(1)->getType() == DOT)
            goto dotNot;
        else if(peek(1)->getType() == LEFTBRACE)
            goto arrayExpr;
        else if(peek(1)->getValue() == "as")
            goto asExpr;
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

        if(peek(1)->getType() == LEFTBRACE && parseArrayExpression(branch)){}
        else if(peek(1)->getType() == LEFTBRACE) {
            branch->freeLastSub();
            expect(branch, "[");
            expect(branch, "]");
            parseVectorArray(branch);
        }
        else if(peek(1)->getType() == LEFTPAREN) {
            parseExpressionList(branch, "(", ")");
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
            branch = branch->getLastSubAst();

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
                errors->createNewError(GENERIC, current(), "unexpected symbol `" + peek(1)->getValue() + "`");
            else if(peek(1)->getType() == DOT)
                errors->createNewError(GENERIC, current(), "unexpected symbol `.`");
            else if(peek(1)->getType() == LEFTBRACE)
                errors->createNewError(GENERIC, current(), "unexpected symbol `[`");
            else if(peek(1)->getValue() == "as")
                errors->createNewError(GENERIC, current(), "unexpected symbol `as`");
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
                else if(peek(1)->getValue() == "as")
                    errors->createNewError(GENERIC, current(), "unexpected symbol `as`");
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
        if(peek(1)->getValue() != "as")
            branch = parenBranch;

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
                if(peek(1)->getValue() != "as")
                    branch = branch->getLastSubAst();

                if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
                    goto checkInc;
                else if(peek(1)->getType() == DOT)
                    goto dotNot;
                else if(peek(1)->getType() == LEFTBRACE)
                    goto arrayExpr;
                else if(peek(1)->getValue() == "as")
                    goto asExpr;
            }

            return true;
        } else if(*peek(1) == "[") {
            advance();
            errors->createNewError(GENERIC, current(), "unexpected symbol `" + peek(1)->getValue() + "`");
        } else if(peek(1)->getType() == _INC || peek(1)->getType() == _DEC)
            goto checkInc;

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

//    cout << branch->toString() << endl;
//    cout << std::flush;
//    int i = 0;
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
            parseMutateDecl(branch);
        }
        else if(isInterfaceDecl(current()))
        {
            parseInterfaceDecl(branch);
        }
        else if(current() == "get") {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected get declaration");

            _current--;
            parseGetter(ast);
        } else if(current() == "set") {
            if(access_types.size() > 0)
                errors->createNewError(ILLEGAL_ACCESS_DECLARATION, current());
            errors->createNewError(GENERIC, current(), "unexpected set declaration");

            _current--;
            parseSetter(ast);
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
            errors->createNewError(GENERIC, current(), "expected method, class, or variable declaration");
            parseAll(branch);
        }

        access_types.free();
    }

    if(brackets != 0)
        errors->createNewError(MISSING_BRACKET, current(), " expected `}` at end of class declaration");
    else
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

    if(*peek(1) == "operator") {
        expect(ast, "operator", false);
        expectOverrideOperator(branch);
        return true;
    }

    if(!expectIdentifier(branch))
        return false;

    while(peek(1)->getType() == DOT && *peek(2) != "class") {
        expect(branch, ".");

        if(*peek(1) == "operator") {
            expect(ast, "operator", false);
            expectOverrideOperator(branch);
            return true;
        }
        expectIdentifier(branch);
    }

    if(peek(1)->getValue() == "#") {
        expect(branch, "#");
        expectIdentifier(branch);
    }

    if(peek(1)->getValue() == "<") {
        Token *old = _current;
        expect(branch, "<");
        errors->enterProtectedMode();
        parseUtypeList(branch);

        if(*peek(1) == ">") {
            errors->fail();
            expect(branch, ">");
        }
        else {
            errors->pass();
            _current = old;
            branch->freeLastSub();
            branch->freeLastToken();
        }
    }

    while(peek(1)->getType() == DOT && *peek(2) != "class") {
        expect(branch, ".");

        if(*peek(1) == "operator") {
            expect(ast, "operator", false);
            expectOverrideOperator(branch);
            return true;
        }

        expectIdentifier(branch);
        if(peek(1)->getValue() == "<") {
            Token *old = _current;
            expect(branch, "<");
            errors->enterProtectedMode();
            parseUtypeList(branch);

            if(*peek(1) == ">") {
                errors->fail();
                expect(branch, ">");
            }
            else {
                errors->pass();
                _current = old;
                branch->freeLastSub();
                branch->freeLastToken();
            }
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
    return token.getId() == IDENTIFIER &&
            ((token.getValue() == "protected") ||
            (token.getValue() == "private") ||
            (token.getValue() == "static") ||
            (token.getValue() == "local") ||
            (token.getValue() == "const") ||
            (token.getValue() == "ext") ||
            (token.getValue() == "stable") ||
            (token.getValue() == "public"));
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

bool parser::isForStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "for");
}

bool parser::isForEachStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "foreach");
}

bool parser::isWhileStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "while");
}

bool parser::isDoWhileStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "do");
}

bool parser::isAliasDeclaration(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "alias");
}

bool parser::isThrowStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "throw");
}

bool parser::isGotoStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "goto");
}

bool parser::isWhenStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "when");
}

bool parser::isBreakStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "break");
}

bool parser::isAsmStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "asm");
}

bool parser::isContinueStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "continue");
}

bool parser::isLockStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "lock");
}

bool parser::isTryCatchStatement(Token& t) {
    return (t.getId() == IDENTIFIER && t.getValue() == "try");
}

bool parser::isSwitchDeclarator(Token& t) {
    return t.getId() == IDENTIFIER && (t.getValue() == "case" || t.getValue() == "default");
}

bool parser::isConstructorDecl() {
    return current().getId() == IDENTIFIER && !isKeyword(current().getValue()) &&
           peek(1)->getType() == LEFTPAREN;
}

bool parser::iaAssemblyInstruction(string key) {
    return key == "nop" ||
    key  == "int" ||
    key  == "movi" ||
    key  == "ret" ||
    key  == "hlt" ||
    key  == "newVarArray" ||
    key  == "cast" ||
    key  == "mov8" ||
    key  == "mov16" ||
    key  == "mov32" ||
    key  == "mov64" ||
    key  == "movu8" ||
    key  == "movu16" ||
    key  == "movu32" ||
    key  == "movu64" ||
    key  == "rstore" ||
    key  == "add" ||
    key  == "sub" ||
    key  == "mul" ||
    key  == "div" ||
    key  == "mod" ||
    key  == "iadd" ||
    key  == "isub" ||
    key  == "idiv" ||
    key  == "imod" ||
    key  == "pop" ||
    key  == "inc" ||
    key  == "dec" ||
    key  == "movr" ||
    key  == "iaload" ||
    key  == "brh" ||
    key  == "ife" ||
    key  == "ifne" ||
    key  == "lt" ||
    key  == "gt" ||
    key  == "le" ||
    key  == "ge" ||
    key  == "movl" ||
    key  == "movsl" ||
    key  == "sizeof" ||
    key  == "put" ||
    key  == "putc" ||
    key  == "checklen" ||
    key  == "jmp" ||
    key  == "loadpc" ||
    key  == "pushobj" ||
    key  == "del" ||
    key  == "call" ||
    key  == "newClass" ||
    key  == "movn" ||
    key  == "sleep" ||
    key  == "te" ||
    key  == "tne" ||
    key  == "lock" ||
    key  == "ulock" ||
    key  == "movg" ||
    key  == "movnd" ||
    key  == "newObjArray" ||
    key  == "not" ||
    key  == "skip" ||
    key  == "loadVal" ||
    key  == "shl" ||
    key  == "shr" ||
    key  == "skipife" ||
    key  == "skipifne" ||
    key  == "and" ||
    key  == "uand" ||
    key  == "or" ||
    key  == "xor" ||
    key  == "throw" ||
    key  == "checknull" ||
    key  == "returnObj" ||
    key  == "newClassArray" ||
    key  == "newString" ||
    key  == "addl" ||
    key  == "subl" ||
    key  == "mull" ||
    key  == "divl" ||
    key  == "modl" ||
    key  == "iaddl" ||
    key  == "isubl" ||
    key  == "idivl" ||
    key  == "imull" ||
    key  == "imodl" ||
    key  == "loadl" ||
    key  == "popObject" ||
    key  == "smovr" ||
    key  == "andl" ||
    key  == "orl" ||
    key  == "xorl" ||
    key  == "rmov" ||
    key  == "smov" ||
    key  == "returnVal" ||
    key  == "istore" ||
    key  == "smovr2" ||
    key  == "smovr3" ||
    key  == "istorel" ||
    key  == "popl" ||
    key  == "pushNull" ||
    key  == "ipushl" ||
    key  == "pushl" ||
    key  == "itest" ||
    key  == "invokeDelegate" ||
    key  == "get" ||
    key  == "isadd" ||
    key  == "je" ||
    key  == "jne" ||
    key  == "ipopl" ||
    key  == "cmp" ||
    key  == "calld" ||
    key  == "varCast" ||
    key  == "tlsMovl" ||
    key  == "dup" ||
    key  == "popObj2" ||
    key  == "swap" ||
    key  == "ldc" ||
    key  == "neg" ||
    key  == ".";
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
           || key == "switch" || key == "default" || key == "local"
           || key == "thread_local" || key == "nil" || key == "ext"  || key == "stable"
           || key == "mutate" || key == "init" || key == "get" || key == "set" || key == "alias"
           || key == "as" || key == "in" || key == "volatile";
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

bool parser::expectOverrideOperator(Ast* ast) {
    advance();

    if(isOverrideOperator(current().getValue()))
    {
        if(ast != NULL) {
            if(current() == "[") {
                ast->addToken(current());
                expect(ast, "]", false);
            } else
                ast->addToken(current());
        }
        return true;
    }
    else {
        errors->createNewError(GENERIC, current(), "expected override operator");
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

Ast * parser::getBranch(Ast *parent, ast_type type) {
    Ast *branch;

    if(type == ast_expression) {
        branch = new Ast(type, peek(1)->getLine(),
                         peek(1)->getColumn());
    }
    else {
        branch = new Ast(type, current().getLine(),
                current().getColumn());
    }

    if(parent == NULL)
    {
        tree.push_back(branch);
        return tree.last();
    }
    else {
        parent->addAst(branch);
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
