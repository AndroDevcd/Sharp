//
// Created by bknun on 1/6/2017.
//
#include "tokenizer.h"
#include <sstream>
#include <string>

uint64_t tokenizer::getentitycount()
{
    return entites->size();
}

list<token_entity>* tokenizer::getentities()
{
    return entites;
}

void tokenizer::parse()
{
    if(toks.length() == 0)
        return;

    parse_lines();
    errors = new Errors(lines);

    while(!isend())
    {
        scan();
    }

    if(!errors->_errs())
    {
        EOF_token = new token_entity("", SINGLE, 0, line, _EOF);
        entites->push_back(*EOF_token);
    }
}


void tokenizer::scan()
{
    invalidate_whitespace();
    invalidate_comments();

    if(isend())
    {
        return;
    }
    else if(issymbol(current()))
    {
        scan_symbol();
    }
    else if(isletter(current()))
    {
        scan_identifier();
    }
    else if(isdigit(current()))
    {
        scan_number();
    }
    else if('"' == current())
    {
        scan_stringliteral();
    }
    else if('\'' == current())
    {
        scan_characterliteral();
    }
    else
    {
        errors->newerror(UNEXPECTED_SYMBOL, line, col, " `" + string(1, current()) + "`");
        advance();
    }
}

void tokenizer::scan_characterliteral() {
    stringstream character;
    if (tokensLeft() < 2)
    {
        errors->newerror(EXPECTED_CHAR_LITERAL_EOF, line, col);
        advance();
        return;
    }
    advance();

    bool escaped_found = false;
    bool escaped = false;
    bool hascharacter = false;

    while (!isend())
    {
        if (!escaped && ('\\' == current()))
        {
            if(hascharacter)
            {
                errors->newerror(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", a chacacter literal cannot contain more than a single character; expected `'`");
                return;
            }

            character << current();
            escaped_found = true;
            escaped = true;
            advance();
            continue;
        }
        else if (!escaped)
        {
            if ('\'' == current())
                break;

            if(hascharacter)
            {
                errors->newerror(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", a chacacter literal cannot contain more than a single character; expected `'`");
                return;
            }

            hascharacter = true;
            character << current();
        }
        else if (escaped)
        {
            hascharacter = true;
            if(!isletter((char) tolower(current())))
            {
                errors->newerror(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", text preceding `\\` must be alpha only");
                return;
            }
            character << current();
            escaped = false;
        }
        advance();
    }

    if (isend())
    {
        errors->newerror(UNEXPECTED_EOF, line, col);
        return;
    }

    if (!escaped_found)
        entites->push_back(token_entity(character.str(), CHAR_LITERAL, col, line));
    else
    {
        string msg = character.str();
        entites->push_back(token_entity(get_escaped_string(msg), CHAR_LITERAL, col, line));
    }

    advance();
    return;
}

void tokenizer::scan_symbol() {
    if(!peekend(1))
    {
        token_type  type = NONE;

        char chs[] = {current(), peek(1)};

        if ((chs[0] == '<') && (chs[1] == '=')) type = LTE;
        else if ((chs[0] == '>') && (chs[1] == '=')) type = GTE;
        else if ((chs[0] == '<') && (chs[1] == '>')) type = NOTEQ;
        else if ((chs[0] == '=') && (chs[1] == '=')) type = EQEQ;
        else if ((chs[0] == '<') && (chs[1] == '<')) type = SHL;
        else if ((chs[0] == '>') && (chs[1] == '>')) type = SHR;
        else if ((chs[0] == '&') && (chs[1] == '&')) type = AND;
        else if ((chs[0] == '|') && (chs[1] == '|')) type = OR;

        if (NONE != type)
        {
            string s;
            s.append(1,chs[0]);
            s.append(1,chs[1]);

            entites->push_back(token_entity(s, SINGLE, col, line, type));
            cursor += 2;
            return;
        }
    }

    if ('<' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, LESSTHAN));
    else if ('>' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, GREATERTHAN));
    else if (';' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, SEMICOLON));
    else if (':' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, COLON));
    else if ('+' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, PLUS));
    else if ('-' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, MINUS));
    else if ('*' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, MULT));
    else if (',' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, COMMA));
    else if ('=' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, ASSIGN));
    else if ('#' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, HASH));
    else if ('!' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, NOT));
    else if ('/' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, DIV));
    else if ('%' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, MOD));
    else if ('(' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, LEFTPAREN));
    else if (')' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, RIGHTPAREN));
    else if ('{' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, LEFTCURLY));
    else if ('}' == current())
        entites->push_back(token_entity(string(1, current()), SINGLE, col, line, RIGHTCURLY));
    else
    {
        errors->newerror(UNEXPECTED_SYMBOL, line, col, " `" + string(1, current()) + "`");
    }

    advance();
}


void tokenizer::scan_identifier() {
    stringstream var;

    while(!isend() && (isalnum(current()) || current() == '_'))
    {
        var << current();
        advance();
    }

    entites->push_back(token_entity(var.str(), IDENTIFIER, col, line));
}

void tokenizer::scan_stringliteral() {
    stringstream message;
    if (tokensLeft() < 2)
    {
        errors->newerror(EXPECTED_STRING_LITERAL_EOF, line, col);
        advance();
        return;
    }
    advance();

    bool escaped_found = false;
    bool escaped = false;

    while (!isend())
    {
        if(current() == '\n')
        {
            errors->newerror(ILLEGAL_STRING_FORMAT, line, col, ", expected `\"` before end of line");
            newline();
            return;
        }
        else if (!escaped && ('\\' == current()))
        {
            message << current();
            escaped_found = true;
            escaped = true;
            advance();
            continue;
        }
        else if (!escaped)
        {
            if ('"' == current())
                break;

            message << current();
        }
        else if (escaped)
        {

            if(!isletter((char) tolower(current())))
            {
                errors->newerror(ILLEGAL_STRING_FORMAT, line, col, ", text preceding `\\` must be alpha only");
                return;
            }

            message << current();
            escaped = false;
        }

        advance();
    }

    if (isend())
    {
        errors->newerror(UNEXPECTED_EOF, line, col);
        return;
    }

    if (!escaped_found)
        entites->push_back(token_entity(message.str(), STRING_LITERAL, col, line));
    else
    {
        string msg = message.str();
        entites->push_back(token_entity(get_escaped_string(msg), STRING_LITERAL, col, line));
    }

    advance();
    return;
}

string tokenizer::get_escaped_string(string msg) const {
    stringstream escapedmessage;
    for(unsigned long i = 0; i < msg.length(); i++)
        {
            if(msg.at(i) == '\\')
            {
                switch(msg.at(i+1)) {
                    case 'n':
                        escapedmessage << endl;
                        break;
                    case 't':
                        escapedmessage << '\t';
                        break;
                    case 'b':
                        escapedmessage << '\b';
                        break;
                    case 'v':
                        escapedmessage << '\v';
                        break;
                    case 'r':
                        escapedmessage << '\r';
                        break;
                    case 'f':
                        escapedmessage << '\f';
                        break;
                    default:
                        escapedmessage << msg.at(i+1);
                        break;
                }

                i++;
            }
            else
                escapedmessage << msg.at(i);
        }
    return escapedmessage.str();
}

void tokenizer::scan_number() {
    /*
            Attempt to match a valid numeric value in one of the following formats:
            1. 123456
            2. 123.456
            3. 123.456e3
            4. 123.456E3
            5. 123.456e+3
            6. 123.456E+3
            7. 123.456e-3
            8. 123.456E-3
            9. 12345e5
         */
    bool dot_found         = false;
    bool e_found           = false;
    bool post_e_sign_found = false;
    stringstream num;

    while (!isend())
    {

        if ('.' == current())
        {
            num << current();
            if (dot_found)
            {
                errors->newerror(ILLEGAL_NUMBER_FORMAT, line, col, ", double decimal");
                return;
            }
            dot_found = true;
            advance();
            continue;
        }
        else if (ismatch('e',current()))
        {
            num << current();
            const char c = peek(1);

            if (peekend(1))
            {
                errors->newerror(ILLEGAL_NUMBER_FORMAT, line, col, ", missing exponent prefix");
                return;
            }
            else if (
                    ('+' != c) &&
                    ('-' != c) &&
                    !isdigit(c)
                    )
            {
                errors->newerror(ILLEGAL_NUMBER_FORMAT, line, col, ", expected `+`, `-`, or digit");
                return;
            }

            e_found = true;
            advance();
            continue;
        }
        else if (e_found && issign(current()))
        {
            num << current();
            if (post_e_sign_found)
            {
                errors->newerror(ILLEGAL_NUMBER_FORMAT, line, col, ", duplicate exponent sign postfix");
                return;
            }

            post_e_sign_found = true;
            advance();
            continue;
        }
        else if (('.' != current()) && !isdigit(current()))
            break;
        else
        {
            num << current();
            advance();
        }
    }

    entites->push_back(token_entity(num.str(), NUMBER_LITERAL, col, line));
    return;
}

static inline bool comment_start(const char c0, const char c1, int& mode)
{
    mode = 0;
    if ('/' == c0)
    {
        if ('/' == c1) { mode = 1;}
        else if ('*' == c1) { mode = 2;}
    }
    return (mode != 0);
}

static inline bool comment_end(const char c0, const char c1, const int mode)
{
    return ((1 == mode) && ('\n' == c0)) ||
           ((2 == mode) && ( '*' == c0) && ('/' == c1));
}

void tokenizer::invalidate_comments() {
    int mode = 0;

    if (isend() || peekend(1))
        return;
    else if (!comment_start(current(),peek(1),mode))
        return;

    advance();
    advance();

    while (!isend() && !comment_end(current(),peek(1),mode))
    {
        if(current() == '\n'){
            newline();
        }

        advance();
    }

    if(current() == '\n'){
        newline();
    }

    if (!isend())
    {
        cursor += mode;
        invalidate_whitespace();
        invalidate_comments();
    }
}

void tokenizer::invalidate_whitespace() {
    while(!isend() && iswhitespace(current()))
    {
        if(current() == '\n')
        {
            newline();
        }

        advance();
    }
}

Errors* tokenizer::geterrors()
{
    return errors;
}

//char c;
//for( ;; ) {
//c = toks.at(cursor++);
//
//if(isletter(c))
//{
//
//}
//else if(isnumber(c))
//{
//
//}
//else if(issymbol(c))
//{
//
//}
//}


char tokenizer::peek(int forward)
{
    if(cursor+forward >= toks.length())
        return toks.at(toks.length()-1);
    else
        return toks.at(cursor+forward);
}

bool tokenizer::isletter(char c)
{
    return isalpha(c);
}

bool tokenizer::isnumber(char c)
{
    return isdigit(c);
}

bool tokenizer::issymbol(char c)
{
    return ('+' == c) || ('-' == c) ||
           ('*' == c) || ('/' == c) ||
           ('^' == c) || ('<' == c) ||
           ('>' == c) || ('=' == c) ||
           (',' == c) || ('!' == c) ||
           ('(' == c) || (')' == c) ||
           ('[' == c) || (']' == c) ||
           ('{' == c) || ('}' == c) ||
           ('%' == c) || (':' == c) ||
           ('?' == c) || ('&' == c) ||
           ('|' == c) || (';' == c) ||
           ('!' == c) || ('.' == c) |
           ('#' == c);
}

bool tokenizer::isend()
{
    return cursor>=toks.length();
}

bool tokenizer::peekend(int forward)
{
    return (cursor+forward)>=toks.length();
}

bool tokenizer::iswhitespace(char c)
{
    return (' '  == c) || ('\n' == c) ||
           ('\r' == c) || ('\t' == c) ||
           ('\b' == c) || ('\v' == c) ||
           ('\f' == c);
}

char tokenizer::advance()
{
    col++;
    return toks.at(cursor++);
}

void tokenizer::newline()
{
    col = 0, line++;
}

char tokenizer::current()
{
    return toks.at(cursor);
}

bool tokenizer::ismatch(char i, char b) {
    return tolower(i) == tolower(b);
}

bool tokenizer::issign(char s) {
    return s == '+' || s == '-';
}

uint64_t  tokenizer::tokensLeft()
{
    return toks.length() - cursor;
}

void tokenizer::parse_lines() {
    stringstream line;
    bool parsed_first = false;

    for(uint64_t i = 0; i < toks.size(); i++)
    {
        if(toks.at(i) == '\n')
        {
            parsed_first = true;
            lines->push_back(line.str());
            line.str("");
        }
        else
            line << toks.at(i);
    }

    if(!parsed_first)
        lines->push_back(line.str());
}

list<string>* tokenizer::getlines() {
    return lines;
}

