//
// Created by bknun on 1/6/2017.
//

#ifndef SHARP_TOKENTYPE_H
#define SHARP_TOKENTYPE_H

/**
 * This enum represents the different types of individual tokens that make up a
 * token group or `token entity`
 */
enum token_type
{
    NUMBER,
    LETTER,
    UNDERSCORE,
    LEFTPAREN,
    RIGHTPAREN,
    LEFTCURLY,
    RIGHTCURLY,
    HASH, // #
    DOT, // .
    PLUS,
    MINUS,
    MULT,
    DIV, //
    MOD, // %
    COLON, // :
    SEMICOLON,
    DUBQUOTE, // "
    SINGQUOTE, // '
    COMMA,
    NEWLINE,
    LTE, // <=
    GTE, // >=
    EQEQ, // ==
    PLUSEQ,
    MINUSEQ,
    MULTEQ,
    DIVEQ,
    ANDEQ,
    OREQ,
    XOREQ,
    MODEQ,
    NOTEQ, // !=
    SHL, // <<
    SHR, // >>
    LESSTHAN,
    GREATERTHAN,
    AND,
    ANDAND, // &&
    OR,
    OROR, // ||
    XOR, // ^
    NOT, // !
    ASSIGN, // =
    LEFTBRACE, // [
    RIGHTBRACE, // ]
    QUESMK, // ?
    PTR, // ->
    INC, // ++
    DEC, // --
    _EOF = 9999, // end of file

    NONE // not a type
};

#endif //SHARP_TOKENTYPE_H
