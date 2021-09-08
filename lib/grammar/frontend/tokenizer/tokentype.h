//
// Created by bknun on 11/13/2017.
//

#ifndef SHARP_TOKENTYPE_H
#define SHARP_TOKENTYPE_H

enum token_type
{
    NUMBER=0,
    LETTER,
    UNDERSCORE,
    LEFTPAREN,
    RIGHTPAREN,
    LEFTCURLY,
    RIGHTCURLY,
    HASH,               // #
    DOT,                // .
    PLUS,
    MINUS,
    MULT,
    EXPONENT,           // **
    _DIV,               //
    _MOD,               // %
    COLON,              // :
    SEMICOLON,
    DUBQUOTE,           // "
    SINGQUOTE,          // '
    COMMA,
    NEWLINE,
    _LTE,               // <=
    _GTE,               // >=
    EQEQ,               // ==
    PLUSEQ,
    MINUSEQ,
    MULTEQ,
    DIVEQ,
    ANDEQ,
    OREQ,
    XOREQ,
    MODEQ,
    NOTEQ,              // !=
    ELVIS,              // ?:
    SAFEDOT,            // ?.
    FORCEDOT,           // !!.
    SHL,                // <<
    SHR,                // >>
    LESSTHAN,
    GREATERTHAN,
    AT,
    AND,
    ANDAND,             // &&
    OR,
    OROR,               // ||
    XOR,                // ^
    NOT,                // !
    EQUALS,             // =
    INFER,              // :=
    LEFTBRACE,          // [
    RIGHTBRACE,         // ]
    QUESMK,             // ?
    PTR,                // ->
    _INC,               // ++
    _DEC,               // --
    DOLLAR,             // $

    NONE,                // not a type
    _EOF = 0xfff9,      // end of file
};

#endif //SHARP_TOKENTYPE_H
