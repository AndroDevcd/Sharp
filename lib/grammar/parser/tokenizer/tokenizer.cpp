//
// Created by bknun on 11/13/2017.
//

#include "tokenizer.h"
#include "../../runtime.h"
#include <sstream>
#include <string>

unsigned long tokenizer::getEntityCount()
{
    return entites.size();
}

List<token_entity>& tokenizer::getEntities()
{
    return entites;
}

void tokenizer::parse()
{
    if(toks.length() == 0)
        return;

    parse_lines();
    errors = new ErrorManager(lines, file, false, c_options.aggressive_errors);

    while(!isend())
    {
        scan();
    }

    if(!errors->hasErrors())
    {
        EOF_token = new token_entity("", SINGLE, 0, line, _EOF);
        entites.push_back(*EOF_token);
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
    else if(issymbol(current))
    {
        scan_symbol();
    }
    else if(isletter(current) || current == '_')
    {
        scan_identifier();
    }
    else if(isnumber(current))
    {
        if(current == '0' && peek(1) == 'x')
            scan_hex();
        else
            scan_number();
    }
    else if('"' == current)
    {
        scan_stringliteral();
    }
    else if('\'' == current)
    {
        scan_characterliteral();
    }
    else
    {
        errors->createNewError(UNEXPECTED_SYMBOL, line, col, " `" + string(1, current) + "`");
        advance();
    }
}

void tokenizer::scan_characterliteral() {
    stringstream character;
    if (tokensLeft() < 2)
    {
        errors->createNewError(EXPECTED_CHAR_LITERAL_EOF, line, col);
        advance();
        return;
    }
    advance();

    bool escaped_found = false;
    bool escaped = false;
    bool hascharacter = false;

    while (!isend())
    {
        if (!escaped && ('\\' == current))
        {
            if(hascharacter)
            {
                errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", a chacacter literal cannot contain more than a single character; expected `'`");
                return;
            }

            character << current;
            escaped_found = true;
            escaped = true;
            advance();
            continue;
        }
        else if (!escaped)
        {
            if ('\'' == current)
                break;

            if(hascharacter)
            {
                errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", a chacacter literal cannot contain more than a single character; expected `'`");
                return;
            }

            hascharacter = true;
            character << current;
        }
        else if (escaped)
        {
            hascharacter = true;
            if(!isletter((char) tolower(current)) && current != '\\')
            {
                errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", text preceding `\\` must be alpha only");
                return;
            }
            character << current;
            escaped = false;
        }
        advance();
    }

    if (isend())
    {
        errors->createNewError(UNEXPECTED_EOF, line, col);
        return;
    }

    if (!escaped_found)
    {
        if(character.str() == "") {
            errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", character literals cannot be empty");
        } else
            entites.add(token_entity(character.str(), CHAR_LITERAL, col, line));
    }
    else
    {
        string msg = character.str();
        entites.add(token_entity(get_escaped_string(msg), CHAR_LITERAL, col, line));
    }

    advance();
    return;
}

void tokenizer::scan_symbol() {
    if(!peekend(1))
    {
        token_type  type = NONE;

        char chs[] = {current, peek(1)};

        if ((chs[0] == '<') && (chs[1] == '=')) type = _LTE;
        else if ((chs[0] == '>') && (chs[1] == '=')) type = _GTE;
        else if ((chs[0] == '!') && (chs[1] == '=')) type = NOTEQ;
        else if ((chs[0] == '=') && (chs[1] == '=')) type = EQEQ;
        else if ((chs[0] == '<') && (chs[1] == '<')) type = SHL;
        else if ((chs[0] == '>') && (chs[1] == '>')) type = SHR;
        else if ((chs[0] == '&') && (chs[1] == '&')) type = ANDAND;
        else if ((chs[0] == '|') && (chs[1] == '|')) type = OROR;
        else if ((chs[0] == '-') && (chs[1] == '>')) type = PTR;
        else if ((chs[0] == '+') && (chs[1] == '+')) type = _INC;
        else if ((chs[0] == '-') && (chs[1] == '-')) type = _DEC;
        else if ((chs[0] == '+') && (chs[1] == '=')) type = PLUSEQ;
        else if ((chs[0] == '*') && (chs[1] == '=')) type = MULTEQ;
        else if ((chs[0] == '-') && (chs[1] == '=')) type = MINUSEQ;
        else if ((chs[0] == '/') && (chs[1] == '=')) type = DIVEQ;
        else if ((chs[0] == '&') && (chs[1] == '=')) type = ANDEQ;
        else if ((chs[0] == '|') && (chs[1] == '=')) type = OREQ;
        else if ((chs[0] == '^') && (chs[1] == '=')) type = XOREQ;
        else if ((chs[0] == '%') && (chs[1] == '=')) type = MODEQ;

        if (NONE != type)
        {
            string s;
            s.append(1,chs[0]);
            s.append(1,chs[1]);

            entites.add(token_entity(s, SINGLE, col, line, type));
            cursor += 2;
            return;
        }
    }

    if ('<' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, LESSTHAN));
    else if ('>' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, GREATERTHAN));
    else if (';' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, SEMICOLON));
    else if (':' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, COLON));
    else if ('+' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, PLUS));
    else if ('-' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, MINUS));
    else if ('*' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, MULT));
    else if (',' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, COMMA));
    else if ('=' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, ASSIGN));
    else if ('#' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, HASH));
    else if ('!' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, NOT));
    else if ('/' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, _DIV));
    else if ('%' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, _MOD));
    else if ('(' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, LEFTPAREN));
    else if (')' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, RIGHTPAREN));
    else if ('{' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, LEFTCURLY));
    else if ('}' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, RIGHTCURLY));
    else if ('.' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, DOT));
    else if ('[' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, LEFTBRACE));
    else if (']' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, RIGHTBRACE));
    else if ('&' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, AND));
    else if ('|' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, OR));
    else if ('^' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, XOR));
    else if ('?' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, QUESMK));
    else if ('$' == current)
        entites.add(token_entity(string(1, current), SINGLE, col, line, DOLLAR));
    else
    {
        errors->createNewError(UNEXPECTED_SYMBOL, line, col, " `" + string(1, current) + "`");
    }

    advance();
}


void tokenizer::scan_identifier() {
    stringstream var;
    bool hasletter = false;

    while(!isend() && (isalnum(current) || current == '_'))
    {
        if(isletter(current))
            hasletter = true;

        var << current;
        advance();
    }

    if(!hasletter)
        errors->createNewError(GENERIC, line, col, " expected at least 1 letter in identifier `" + var.str() + "`");
    else
        entites.add(token_entity(var.str(), IDENTIFIER, col, line));
}

void tokenizer::scan_stringliteral() {
    stringstream message;
    if (tokensLeft() < 2)
    {
        errors->createNewError(EXPECTED_STRING_LITERAL_EOF, line, col);
        advance();
        return;
    }
    advance();

    bool escaped_found = false;
    bool escaped = false;

    while (!isend())
    {
        if(current == '\n')
        {
            errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", expected `\"` before end of line");
            newline();
            return;
        }
        else if (!escaped && ('\\' == current))
        {
            message << current;
            escaped_found = true;
            escaped = true;
            advance();
            continue;
        }
        else if (!escaped)
        {
            if ('"' == current)
                break;

            message << current;
        }
        else if (escaped)
        {

            if(!isletter((char) tolower(current)))
            {
                errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", text preceding `\\` must be alpha only");
                return;
            }

            message << current;
            escaped = false;
        }

        advance();
    }

    if (isend())
    {
        errors->createNewError(UNEXPECTED_EOF, line, col);
        return;
    }

    if (!escaped_found)
        entites.add(token_entity(message.str(), STRING_LITERAL, col, line));
    else
        entites.add(token_entity(get_escaped_string(message.str()), STRING_LITERAL, col, line));

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

void tokenizer::scan_hex() {
    stringstream num;
    bool underscore_ok = false;

    num << "0x";
    advance();
    advance();
    while(!isend() && (ishexnum(current) || current == '_'))
    {
        if(ishexnum(current))
            underscore_ok = true;
        else {
            if(!underscore_ok) {
                errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", unexpected or illegally placed underscore");
                break;
            }

            advance();
            continue;
        }

        num << current;
        advance();
    }

    entites.add(token_entity(num.str(), HEX_LITERAL, col, line));
}

void tokenizer::scan_number() {
    /*
            Attempt to match a valid numeric value in one of the following formats:
            1. 123456
            2. 123.456
            3. 1_23.456e3
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
    bool underscore_ok     = false;
    stringstream num;

    while (!isend())
    {

        if ('_' == current)
        {
            if(!underscore_ok || peek(-1) == '.')
            {
                errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", unexpected or illegally placed underscore");
                return;
            }
            advance();
        }
        else if ('.' == current)
        {
            num << current;
            if (dot_found)
            {
                errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", double decimal");
                return;
            }
            dot_found = true;
            advance();
            continue;
        }
        else if (ismatch('e',current))
        {
            underscore_ok = false;
            num << current;
            const char c = peek(1);

            if (peekend(1))
            {
                errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", missing exponent prefix");
                return;
            }
            else if (
                    ('+' != c) &&
                    ('-' != c) &&
                    ! isnumber(c)
                    )
            {
                errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", expected `+`, `-`, or digit");
                return;
            }

            e_found = true;
            advance();
            continue;
        }
        else if (e_found && issign(current))
        {
            num << current;
            if (post_e_sign_found)
            {
                errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", duplicate exponent sign postfix");
                return;
            }

            post_e_sign_found = true;
            advance();
            continue;
        }
        else if (('.' != current) && !isdigit(current))
            break;
        else
        {
            if(isdigit(current) && !e_found)
                underscore_ok = true;

            num << current;
            advance();
        }
    }

    entites.add(token_entity(num.str(), INTEGER_LITERAL, col, line));
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
    else if (!comment_start(current,peek(1),mode))
        return;

    advance();
    advance();

    while (!isend() && !comment_end(current,peek(1),mode))
    {
        if(current == '\n'){
            newline();
        }

        advance();
    }

    if(current == '\n'){
        newline();
    }

    if (!isend())
    {
        cursor += mode;
        invalidate_whitespace();
        invalidate_comments();
    }
    else if(isend() && !comment_end(current,peek(1),mode))
    {
        errors->createNewError(GENERIC, line, col, "expected closed comment before end of file");
    }
}

void tokenizer::invalidate_whitespace() {
    while(!isend() && iswhitespace(current))
    {
        if(current == '\n')
        {
            newline();
        }

        advance();
    }
}

ErrorManager* tokenizer::getErrors()
{
    return errors;
}

bool tokenizer::ismatch(char i, char b) {
    return tolower(i) == tolower(b);
}


void tokenizer::parse_lines() {
    stringstream line;
    bool parsed_first = false;

    for(unsigned long i = 0; i < toks.size(); i++)
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

list<string>* tokenizer::getLines() {
    return lines;
}

void tokenizer::free() {
    this->line = 0;
    this->col = 0;
    this->toks = "";
    this->cursor = 0;
    this->errors->free();
    this->lines->clear();
    this->entites.free();
    delete (this->errors); this->errors = NULL;
    delete (this->lines); this->lines = NULL;

}

const string &tokenizer::getData() const {
    return toks;
}