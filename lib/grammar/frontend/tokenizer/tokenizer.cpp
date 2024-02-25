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

#ifdef WIN32_
#define is_newline(c) \
    (('\n' == c))
#endif

#ifdef POSIX_
    #define is_newline(c) \
        (('\n' == c))
#endif

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
    col++; cursor++;  \
    if(cursor < toks.size()) \
        current = toks.at(cursor); \
    else is_end = true;

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

void tokenizer::add_token(token_id id) {
    tokens.add(Token(toks.substr(start, (cursor-start)+1), id, col, line, NONE));
}
void tokenizer::add_token(token_type type) {
    tokens.add(Token(toks.substr(start, (cursor-start)+1), SINGLE, col, line, type));
}

Int entry = 0;
void tokenizer::parse()
{
    if(len == 0)
        return;

    parse_lines();
    errors = new ErrorManager(&lines, file, false, options.aggressive_errors);
    EOF_token = new Token("", SINGLE, 1, lines.size(), _EOF);

    current = toks.at(0);

    while(!is_end) {
        start = cursor;

        if(line == 5777) {
            int i = 0;
            if(cursor >= 11649) {
                int k  = 0;
            }
        }
        switch (current) {
            case ' ':
            case '\t':
            case '\b':
            case '\v':
            case '\r':
            case '\f':
                // ignore whitespace
                break;

            case '\n': {
                newline()
                advance()
                continue;
            }

            case '_':
            case 'A': case 'B': case 'C': case 'D': case 'E':
            case 'F': case 'G': case 'H': case 'I': case 'J':
            case 'K': case 'L': case 'M': case 'N': case 'O':
            case 'P': case 'Q': case 'R': case 'S': case 'T':
            case 'U': case 'V': case 'W': case 'X': case 'Y':
            case 'Z':
            case 'a': case 'b': case 'c': case 'd': case 'e':
            case 'f': case 'g': case 'h': case 'i': case 'j':
            case 'k': case 'l': case 'm': case 'n': case 'o':
            case 'p': case 'q': case 'r': case 's': case 't':
            case 'u': case 'v': case 'w': case 'x': case 'y':
            case 'z':
                parseIdentifier();
                break;

            case '"':
                if (parseString())
                    continue;
            break;

            case '\'':
                parseChar();
                break;

            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
            case '8': case '9':
                parseNumber();
                break;

            case '/':{
                if (match('/')) {
                    // A comment goes until the end of the line.
                    Int size = toks.size();
                    while (!is_newline(current) && !is_end) {
                        advance();
                    }

                    if(is_newline(current)) {
                        newline()
                        advance()
                        continue;
                    }
                } else if(match('*')) {
                    Int nests = 0;
                    entry++;
                    bool found_end = false;

                    // A multiline comment goes until the [*/].
                    while(!is_end)
                    {
                        if(is_newline(current)){
                            newline()
                            advance()
                            continue;
                        } else if(current == '*' && peek(1) == '/') {
                            if(nests == 0) {
                                found_end = true;
                                break;
                            } else nests--;
                        } else if(current == '/' && peek(1) == '*')
                            nests++;

                        advance();
                    }

                    if(!found_end)
                        errors->createNewError(GENERIC, line, col, "expected `*/` at end of block comment");
                    advance();
                } else {
                    if(match('='))
                        add_token(DIVEQ);
                    else
                        add_token(_DIV);
                }
                break;
            }

            case '{': {
                if(dynamicString)
                    brackets++;
                add_token(LEFTCURLY);
                break;
            }

            case '}': {
                if(dynamicString) {
                    if(--brackets == 0) {
                        dynamicString = false;
                        if (peek(1) == '"') {
                            advance();
                            advance();
                            tokens.add(Token(")", SINGLE, col, line, RIGHTPAREN));
                            continue;
                        } else {
                            tokens.add(Token(")", SINGLE, col, line, RIGHTPAREN));
                            tokens.add(Token("+", SINGLE, col, line, PLUS));
                            if(parseString())
                                continue;
                        }
                    } else
                        add_token(RIGHTCURLY);
                } else
                    add_token(RIGHTCURLY);
                break;
            }
            case '(': add_token(LEFTPAREN); break;
            case ')': add_token(RIGHTPAREN); break;
            case '[': add_token(LEFTBRACE); break;
            case ']': add_token(RIGHTBRACE); break;
            case ',': add_token(COMMA); break;
            case '.': add_token(DOT); break;
            case ';': add_token(SEMICOLON); break;
            case '#': add_token(HASH); break;

            case '?': {
                if(match(':'))
                    add_token(ELVIS);
                else
                    add_token(match('.') ? SAFEDOT : QUESMK);
                break;
            }
            case '$': add_token(DOLLAR); break;
            case '@': add_token(AT); break;
            case '!': {
                if(match('!')) {
                    if(match('.'))
                        add_token(FORCEDOT);
                    else add_token(DOUBLEBANG);
                }
                else
                    add_token(match('=') ? NOTEQ : NOT);
                break;
            }
            case '=': add_token(match('=') ? EQEQ : EQUALS); break;
            case ':': add_token(match('=') ? INFER : COLON); break;
            case '<': {
                if(match('<'))
                    add_token(SHL);
                else
                    add_token(match('=') ? _LTE : LESSTHAN);
                break;
            }

            case '>': {
                if(match('>'))
                    add_token(SHR);
                else
                    add_token(match('=') ? _GTE : GREATERTHAN);
                break;
            }

            case '&': {
                if(match('='))
                    add_token(ANDEQ);
                else
                    add_token(match('&') ? ANDAND : AND);
                break;
            }

            case '|': {
                if(match('='))
                    add_token(OREQ);
                else
                    add_token(match('|') ? OROR : OR);
                break;
            }

            case '^': add_token(match('=') ? XOREQ : XOR); break;
            case '%': add_token(match('=') ? MODEQ : _MOD); break;
            case '+': {
                if(match('='))
                    add_token(PLUSEQ);
                else
                    add_token(match('+') ? _INC : PLUS);
                break;
            }

            case '-': {
                if(match('='))
                    add_token(MINUSEQ);
                else if(match('>'))
                    add_token(PTR);
                else
                    add_token(match('-') ? _DEC : MINUS);
                break;
            }

            case '*': {
                if(match('='))
                    add_token(MULTEQ);
                else if(match('*'))
                    add_token(EXPONENT);
                else
                    add_token(MULT);
                break;
            }
            default:
                errors->createNewError(UNEXPECTED_SYMBOL, line, col, " `" + string(1, current) + "`");
                break;
        }

        advance()
    }

    end:
//    GUARD(globalLock)
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

void tokenizer::parseNumber() {
    if(current == '0' && peek(1) == 'x') {
        bool underscore_ok = false;

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

            advance();
        }

        cursor--;
        add_token(HEX_LITERAL);
        return;
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
                if(!isdigit(peek(1)))
                    break;

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
                if (post_e_sign_found)
                {
                    errors->createNewError(ILLEGAL_NUMBER_FORMAT, line, col, ", duplicate exponent sign postfix");
                    return;
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

                advance();
            }
        }

        cursor--;
        add_token(INTEGER_LITERAL);
    }
}

void tokenizer::parseChar() {
    if (tokensLeft() < 2)
    {
        errors->createNewError(EXPECTED_CHAR_LITERAL_EOF, line, col);
        advance();
        return;
    }
    advance();

    start = cursor;
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
        }
        else if (escaped)
        {
            hascharacter = true;
            if(current != '0' && current != 'a' && !isletter((char) tolower(current))
               && current != '\\' && current != '\"' && current != '\'')
            {
                errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", text preceding `\\` must be alpha, '\"', or '\\'' only");
                return;
            }

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
        cursor--;
        if(!hascharacter) {
            errors->createNewError(ILLEGAL_CHAR_LITERAL_FORMAT, line, col, ", character literals cannot be empty");
        } else
            add_token(CHAR_LITERAL);
        cursor++;
    }
    else
    {
        tokens.add(Token(from_escaped_string(toks.substr(start, (cursor-start))), CHAR_LITERAL, col, line));
    }
}

bool tokenizer::parseString() {
    if (tokensLeft() < 2)
    {
        errors->createNewError(EXPECTED_STRING_LITERAL_EOF, line, col);
        advance();
        return false;
    }

    advance();
    if(dynamicString && brackets == 0) {
        dynamicString = false;
        errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", expected `}` before end of string");
        return false;
    }

    start = cursor;
    bool escaped_found = false;
    bool escaped = false;

    while (!isend())
    {
        if(current == '\n')
        {
            errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", expected `\"` before end of line");
            newline();
            return false;
        }
        else if (!escaped && ('\\' == current))
        {
            escaped_found = true;
            escaped = true;
            advance();
            continue;
        }
        else if (!escaped)
        {
            if ('"' == current)
                break;

            if('$' == current && dynamicStringSupport) {
                bool addPlus = true;
                if(start != cursor) {
                    cursor--;
                    if (!escaped_found)
                        add_token(STRING_LITERAL);
                    else
                        tokens.add(Token(from_escaped_string(toks.substr(start, (cursor-start)+1)), STRING_LITERAL, col, line));
                    cursor++;

                    advance();
                } else if(peek(1) != '$') {
                    if(!tokens.empty() && tokens.last().getType() != PLUS)
                        tokens.add(Token("", STRING_LITERAL, col, line));
                    else addPlus = false;
                    advance();
                }


                if('{' == current) {
                    advance();
                    // we gotta tokenize he rest of the data set  var to check the state where once we encounter a '}' we jump back to parsing a string
                    dynamicString = true;
                    brackets = 1;

                    if(addPlus)
                        tokens.add(Token(string(1, '+'), SINGLE, col, line, PLUS));
                    tokens.add(Token(string(1, '('), SINGLE, col, line, LEFTPAREN));
                    return true;
                } else if('$' != current){
                    start = cursor;
                    if(addPlus)
                        tokens.add(Token(string(1, '+'), SINGLE, col, line, PLUS));

                    if(!isend() && (isalnum(current) || current == '_'))
                        parseIdentifier();
                    else
                        errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", text preceding `$` must be alpha numeric, or '_' only");

                    advance(); // should this be here?
                    if('"' == current || isend()) {
                        return false;
                    } else {
                        tokens.add(Token(string(1,'+'), SINGLE, col, line, PLUS));
                        start = cursor;
                        continue;
                    }
                } else{
                    tokens.add(Token(string(1,'$'), SINGLE, col, line, DOLLAR));
                    advance();

                    if('"' == current || is_end) {
                        return false;
                    } else
                        start = cursor;
                }
            }
        }
        else if (escaped)
        {
            if(!isletter((char) tolower(current)) && ('\\' != current) && ('\"' != current) && ('\'' != current))
            {
                errors->createNewError(ILLEGAL_STRING_FORMAT, line, col, ", text preceding `\\` must be alpha, '\\', or '\"' only");
                return false;
            }

            escaped = false;
        }

        advance();
    }

    if (isend())
    {
        errors->createNewError(UNEXPECTED_EOF, line, col);
        return false;
    }

    cursor--;
    if(!escaped_found)
        add_token(STRING_LITERAL);
    else tokens.add(Token(from_escaped_string(toks.substr(start, (cursor-start)+1)), STRING_LITERAL, col, line));
    cursor++;
    return false;
}

CXX11_INLINE
void tokenizer::parseIdentifier() {
    start = cursor;
    bool hasletter = false;

    while(!is_end && (isalnum(current) || current == '_'))
    {
        if(isletter(current))
            hasletter = true;
        advance();
    }

    cursor--;
    if(!hasletter)
        errors->createNewError(GENERIC, line, col, " expected at least 1 letter in identifier");
    else
        add_token(IDENTIFIER);
}

string tokenizer::from_escaped_string(string msg) {
    string escapedmessage;
    for(unsigned long i = 0; i < msg.length(); i++)
    {
        if(msg.at(i) == '\\')
        {
            switch(msg.at(i+1)) {
                case 'n':
                    escapedmessage += "\n";
                    break;
                case 't':
                    escapedmessage += '\t';
                    break;
                case 'b':
                    escapedmessage += '\b';
                    break;
                case 'v':
                    escapedmessage += '\v';
                    break;
                case 'r':
                    escapedmessage += '\r';
                    break;
                case 'f':
                    escapedmessage += '\f';
                    break;
                case '0':
                    escapedmessage += '\0';
                    break;
                case 'a':
                    escapedmessage += '\a';
                    break;
                case '\\':
                    escapedmessage += '\\';
                    break;
                default:
                    escapedmessage += msg.at(i+1);
                    break;
            }

            i++;
        }
        else
            escapedmessage += msg.at(i);
    }
    return escapedmessage;
}

string tokenizer::to_escaped_string(string msg) {
    stringstream escapedmessage;
    for(unsigned long i = 0; i < msg.length(); i++)
    {
        switch(msg.at(i)) {
            case '\n':
                escapedmessage << "\\n";
                break;
            case '\t':
                escapedmessage << "\\t";
                break;
            case '\b':
                escapedmessage << "\\b";
                break;
            case '\v':
                escapedmessage << "\\v";
                break;
            case '\r':
                escapedmessage << "\\r";
                break;
            case '\f':
                escapedmessage << "\\f";
                break;
            case '\0':
                escapedmessage << "\\0";
                break;
            case '\a':
                escapedmessage << "\\a";
                break;
            case '"':
                escapedmessage << "\\\"";
                break;
            default:
                escapedmessage << msg.at(i);
                break;
        }
    }
    return escapedmessage.str();
}

ErrorManager* tokenizer::getErrors()
{
    return errors;
}

bool tokenizer::match(char c) {
    if(peek(1)==c) {
        advance();
        return true;
    }

    return false;
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