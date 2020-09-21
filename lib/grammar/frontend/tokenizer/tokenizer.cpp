//
// Created by bknun on 11/13/2017.
//

#include "tokenizer.h"
#include "../../main.h"

#define iswhitespace(c) \
    ((' '  == c) || ('\n' == c) || \
    ('\r' == c) || ('\t' == c) || \
    ('\b' == c) || ('\v' == c) || \
    ('\f' == c)) \

#define current \
    ((cursor < len) ? \
        toks.at(cursor) \
    : toks.at(len-1)) \

#define newline() \
    col = 0, line++;

#define isend() \
    (cursor>=len)

#define peekend(forward) \
    ((cursor+forward)>=len)

#define peek(forward) \
    (((cursor+forward) >= len || (cursor+forward) < 0) ? \
        toks.at(len-1) : toks.at(cursor+forward))

#define issymbol(c) \
    (('+' == c) || ('-' == c) || \
    ('*' == c) || ('/' == c) || \
    ('^' == c) || ('<' == c) || \
    ('>' == c) || ('=' == c) || \
    (',' == c) || ('!' == c) || \
    ('(' == c) || (')' == c) || \
    ('[' == c) || (']' == c) || \
    ('{' == c) || ('}' == c) || \
    ('%' == c) || (':' == c) || \
    ('?' == c) || ('&' == c) || \
    ('|' == c) || (';' == c) || \
    ('!' == c) || ('.' == c) || \
    ('#' == c) || ('$' == c) || \
    ('@' == c))

#define isnumber(c) \
    isdigit(c)

#define isletter(c) \
    isalpha(c)

#define ishexnum(c) \
    (isdigit(c) || ((c >= 65) && (c <= 72)) || \
        ((c >= 97) && (c <= 104)))

#define advance() \
    col++; cursor++;

#define tokensLeft() \
    (len - cursor)

#define issign(s) \
    ((s == '+') || (s == '-'))

unsigned long tokenizer::getEntityCount()
{
    return tokens.size();
}

List<Token>& tokenizer::getTokens()
{
    return tokens;
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

void tokenizer::parse()
{
    if(len == 0)
        return;

    parse_lines();
    errors = new ErrorManager(&lines, file, false, c_options.aggressive_errors);
    EOF_token = new Token("", SINGLE, 1, lines.size(), _EOF);

    for(;;)
    {
        start:
        invalidate_whitespace:
        while(!isend() && iswhitespace(current))
        {
            if(current == '\n')
            {
                newline();
            }

            advance();
        }

        // invalidate_comments
        int mode = 0;
        if (isend() || peekend(1))
            goto scan;
        else if (!comment_start(current,peek(1),mode))
            goto scan;

        col+=2; cursor+=2;
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
            goto invalidate_whitespace;
        }
        else if(isend() && !comment_end(current,peek(1),mode))
        {
            errors->createNewError(GENERIC, line, col, "expected closed comment before end of file");
        }

        scan:
        if(isend())
        {
            goto end;
        }
        else if(issymbol(current)) // <, >, <<, etc.
        {
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
                else if ((chs[0] == '*') && (chs[1] == '*')) type = EXPONENT; // TODO: talk about exponent
                else if ((chs[0] == '-') && (chs[1] == '=')) type = MINUSEQ;
                else if ((chs[0] == '/') && (chs[1] == '=')) type = DIVEQ;
                else if ((chs[0] == '&') && (chs[1] == '=')) type = ANDEQ;
                else if ((chs[0] == '|') && (chs[1] == '=')) type = OREQ;
                else if ((chs[0] == '^') && (chs[1] == '=')) type = XOREQ;
                else if ((chs[0] == '%') && (chs[1] == '=')) type = MODEQ;
                else if ((chs[0] == ':') && (chs[1] == '=')) type = INFER;

                if (NONE != type)
                {
                    string s;
                    s.append(1,chs[0]);
                    s.append(1,chs[1]);

                    tokens.add(Token(s, SINGLE, col, line, type));
                    cursor += 2;
                    goto start;
                }
            }

            if ('<' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, LESSTHAN));
            else if ('>' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, GREATERTHAN));
            else if (';' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, SEMICOLON));
            else if (':' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, COLON));
            else if ('+' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, PLUS));
            else if ('-' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, MINUS));
            else if ('*' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, MULT));
            else if (',' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, COMMA));
            else if ('=' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, EQUALS));
            else if ('#' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, HASH));
            else if ('!' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, NOT));
            else if ('/' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, _DIV));
            else if ('%' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, _MOD));
            else if ('(' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, LEFTPAREN));
            else if (')' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, RIGHTPAREN));
            else if ('{' == current) {
                if(dynamicString)
                    brackets++;
                tokens.add(Token(string(1, current), SINGLE, col, line, LEFTCURLY));
            }
            else if ('}' == current) {
                if(dynamicString) {
                    if(--brackets == 0) {
                        dynamicString = false;
                        if (peek(1) == '"') {
                            advance()
                            advance()
                            tokens.add(Token(string(1, ')'), SINGLE, col, line, RIGHTPAREN));
                            goto start;
                        } else {
                            tokens.add(Token(string(1, ')'), SINGLE, col, line, RIGHTPAREN));
                            tokens.add(Token(string(1, '+'), SINGLE, col, line, PLUS));
                            goto process_string;
                        }
                    } else {
                        tokens.add(Token(string(1, current), SINGLE, col, line, RIGHTCURLY));
                    }
                } else
                    tokens.add(Token(string(1, current), SINGLE, col, line, RIGHTCURLY));
            }
            else if ('.' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, DOT));
            else if ('[' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, LEFTBRACE));
            else if (']' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, RIGHTBRACE));
            else if ('&' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, AND));
            else if ('|' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, OR));
            else if ('^' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, XOR));
            else if ('?' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, QUESMK));
            else if ('$' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, DOLLAR));
            else if ('@' == current)
                tokens.add(Token(string(1, current), SINGLE, col, line, AT));
            else
            {
                errors->createNewError(UNEXPECTED_SYMBOL, line, col, " `" + string(1, current) + "`");
            }

            advance();
            goto start;
        }
        else if(isletter(current) || current == '_') // int, dave, etc.
        {
            parseIdentifier();
            goto start;
        }
        else if(isnumber(current)) // 0xff2ea, 3.14159, 773_721_3376, etc
        {
            if(current == '0' && peek(1) == 'x') {
                stringstream num;
                bool underscore_ok = false;

                num << "0x";
                col+=2; cursor+=2;
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

                tokens.add(Token(num.str(), HEX_LITERAL, col, line));
                goto start;
            }
            else {
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
                            goto start;
                        }
                        advance();
                    }
                    else if ('.' == current)
                    {
                        if(!isdigit(peek(1)))
                            break;

                        num << current;
                        if (dot_found)
                        {
                            errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", double decimal");
                            goto start;
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
                            goto start;
                        }
                        else if (
                                ('+' != c) &&
                                ('-' != c) &&
                                ! isnumber(c)
                                )
                        {
                            errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", expected `+`, `-`, or digit");
                            goto start;
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
                            goto start;
                        }

                        post_e_sign_found = true;
                        advance();
                        continue;
                    }
                    else if (('.' != current) && !isdigit(current)) {
                        break;
                    }
                    else
                    {
                        if(isdigit(current) && !e_found)
                            underscore_ok = true;

                        num << current;
                        advance();
                    }
                }

                tokens.add(Token(num.str(), INTEGER_LITERAL, col, line));
                goto start;
            }
        }
        else if('"' == current) // "Hello, World", "Name: \n", etc.
        {
            process_string:
            stringstream message;
            if (tokensLeft() < 2)
            {
                errors->createNewError(EXPECTED_STRING_LITERAL_EOF, line, col);
                advance();
                goto start;
            }
            advance();

            if(dynamicString && brackets == 0) {
                dynamicString = false;
                errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", expected `}` before end of string");
                goto start;
            }

            bool escaped_found = false;
            bool escaped = false;

            while (!isend())
            {
                if(current == '\n')
                {
                    errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", expected `\"` before end of line");
                    newline();
                    goto start;
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

                    if('$' == current) {
                        advance();

                        if('{' == current) {
                            advance();
                            saveString(message, escaped_found);
                            // we gotta tokenize he rest of the data set  var to check the state where once we encounter a '}' we jump back to parsing a string
                            dynamicString = true;
                            brackets = 1;

                            tokens.add(Token(string(1, '+'), SINGLE, col, line, PLUS));
                            tokens.add(Token(string(1, '('), SINGLE, col, line, LEFTPAREN));
                            goto start;
                        } else if('$' != current){
                            saveString(message, escaped_found);

                            message.str("");
                            tokens.add(Token(string(1, '+'), SINGLE, col, line, PLUS));

                            if(!isend() && (isalnum(current) || current == '_'))
                                parseIdentifier();
                            else
                                errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", text preceding `$` must be alpha numeric, or '_' only");

                            if('"' == current || isend()) {
                                advance()
                                goto start;
                            } else {
                                tokens.add(Token(string(1, '+'), SINGLE, col, line, PLUS));
                                continue;
                            }
                        }
                    }

                    message << current;
                }
                else if (escaped)
                {
                    if('$' != current && !isletter((char) tolower(current)) && ('\\' != current) && ('\"' != current) && ('\'' != current))
                    {
                        errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", text preceding `\\` must be alpha, '$', '\\', or '\"' only");
                        goto start;
                    }

                    message << current;
                    escaped = false;
                }

                advance();
            }

            if (isend())
            {
                errors->createNewError(UNEXPECTED_EOF, line, col);
                goto start;
            }

            saveString(message, escaped_found);
            advance();
            goto start;
        }
        else if('\'' == current) // 'a', '\t', etc.
        {
            stringstream character;
            if (tokensLeft() < 2)
            {
                errors->createNewError(EXPECTED_CHAR_LITERAL_EOF, line, col);
                advance();
                goto start;
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
                        goto start;
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
                        goto start;
                    }

                    hascharacter = true;
                    character << current;
                }
                else if (escaped)
                {
                    hascharacter = true;
                    if(current != '0' && current != 'a' && !isletter((char) tolower(current))
                       && current != '\\' && current != '\"' && current != '\'')
                    {
                        errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", text preceding `\\` must be alpha, '\"', or '\\'' only");
                        goto start;
                    }
                    character << current;
                    escaped = false;
                }
                advance();
            }

            if (isend())
            {
                errors->createNewError(UNEXPECTED_EOF, line, col);
                goto start;
            }

            if (!escaped_found)
            {
                if(character.str().empty()) {
                    errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", character literals cannot be empty");
                } else
                    tokens.add(Token(character.str(), CHAR_LITERAL, col, line));
            }
            else
            {
                string msg = character.str();
                tokens.add(Token(get_escaped_string(msg), CHAR_LITERAL, col, line));
            }

            advance();
            goto start;
        }
        else
        {
            errors->createNewError(UNEXPECTED_SYMBOL, line, col, " `" + string(1, current) + "`");
            advance();
            goto start;
        }
    }


    end:

//    for(Int i = 0; i < tokens.size(); i++) {
//        cout << tokens.at(i).getValue()
//        << ":" << tokens.at(i).getLine() << ":" << tokens.at(i).getColumn()
//        << " " << tokenTypeToString(tokens.at(i).getType()) << endl;
//    }
//    cout << "done\n\n";

    tokens.push_back(*EOF_token);
}

string tokenizer::tokenTypeToString(token_type type) {
        switch(type) {
            case NUMBER:
                return "NUMBER";
            case LETTER:
                return "LETTER";
            case UNDERSCORE:
                return "UNDERSCORE";
            case LEFTPAREN:
                return "LEFTPAREN";
            case RIGHTPAREN:
                return "RIGHTPAREN";
            case LEFTCURLY:
                return "LEFTCURLY";
            case RIGHTCURLY:
                return "RIGHTCURLY";
            case HASH:
                return "HASH";
            case DOT:
                return "DOT";
            case PLUS:
                return "PLUS";
            case MINUS:
                return "MINUS";
            case MULT:
                return "MULT";
            case _DIV:
                return "_DIV";
            case _MOD:
                return "_MOD";
            case COLON:
                return "COLON";
            case SEMICOLON:
                return "SEMICOLON";
            case DUBQUOTE:
                return "DUBQUOTE";
            case SINGQUOTE:
                return "SINGQUOTE";
            case COMMA:
                return "COMMA";
            case NEWLINE:
                return "NEWLINE";
            case _LTE:
                return "_LTE";
            case _GTE:
                return "_GTE";
            case EQEQ:
                return "EQEQ";
            case PLUSEQ:
                return "PLUSEQ";
            case MINUSEQ:
                return "MINUSEQ";
            case MULTEQ:
                return "MULTEQ";
            case DIVEQ:
                return "DIVEQ";
            case ANDEQ:
                return "ANDEQ";
            case OREQ:
                return "OREQ";
            case XOREQ:
                return "XOREQ";
            case MODEQ:
                return "MODEQ";
            case NOTEQ:
                return "NOTEQ";
            case SHL:
                return "SHL";
            case SHR:
                return "SHR";
            case LESSTHAN:
                return "LESSTHAN";
            case GREATERTHAN:
                return "GREATERTHAN";
            case AND:
                return "AND";
            case ANDAND:
                return "ANDAND";
            case OR:
                return "OR";
            case OROR:
                return "OROR";
            case XOR:
                return "XOR";
            case NOT:
                return "NOT";
            case EQUALS:
                return "EQUALS";
            case INFER:
                return "INFER";
            case LEFTBRACE:
                return "LEFTBRACE";
            case RIGHTBRACE:
                return "RIGHTBRACE";
            case QUESMK:
                return "QUESMK";
            case PTR:
                return "PTR";
            case _INC:
                return "_INC";
            case _DEC:
                return "_DEC";
            case DOLLAR:
                return "DOLLAR";
            case _EOF:
                return "_EOF";
            case NONE:
                return "NONE";
        }

        return "?";
}

CXX11_INLINE
void tokenizer::parseIdentifier() {
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
        tokens.add(Token(var.str(), IDENTIFIER, col, line));
}

CXX11_INLINE
void tokenizer::saveString(const stringstream &message, bool escaped_found) {
    if (!escaped_found)
        tokens.add(Token(message.str(), STRING_LITERAL, col, line));
    else
        tokens.add(Token(get_escaped_string(message.str()), STRING_LITERAL, col, line));
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
                case '0':
                    escapedmessage << '\0';
                    break;
                case 'a':
                    escapedmessage << '\a';
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

ErrorManager* tokenizer::getErrors()
{
    return errors;
}

bool tokenizer::ismatch(char i, char b) {
    return tolower(i) == tolower(b);
}

void tokenizer::parse_lines() {
    stringstream line;

    for(unsigned long i = 0; i < len; i++)
    {
        if(toks[i] == '\n')
        {
            lines.add(line.str());
            line.str("");
        }
        else
            line << toks[i];
    }

    if(!line.str().empty())
        lines.add(line.str());
}

List<string>& tokenizer::getLines() {
    return lines;
}

void tokenizer::free() {
    this->line = 0;
    this->col = 0;
    this->cursor = 0;
    this->len = 0;
    empty.clear();
    this->errors->free();
    this->lines.free();
    this->tokens.free();
    delete (this->errors); this->errors = NULL;
    toks.clear();
}

string &tokenizer::getData() {
    return !toks.empty() ? toks : empty;
}