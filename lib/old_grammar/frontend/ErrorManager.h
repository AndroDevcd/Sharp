//
// Created by bknun on 1/18/2018.
//

#ifndef SHARP_PARSEERROR_H
#define SHARP_PARSEERROR_H

#include <list>
#include "../../../stdimports.h"
#include "../../util/KeyPair.h"
#include "tokenizer/token.h"
#include "../List.h"

enum error_type
{
    UNEXPECTED_SYMBOL = 0,
    ILLEGAL_NUMBER_FORMAT = 1,
    UNEXPECTED_EOF = 2,
    EXPECTED_STRING_LITERAL_EOF = 3,
    ILLEGAL_STRING_FORMAT = 4,
    EXPECTED_CHAR_LITERAL_EOF = 5,
    ILLEGAL_CHAR_LITERAL_FORMAT = 6,
    GENERIC = 7,
    ILLEGAL_ACCESS_DECLARATION = 8,
    ILLEGAL_BRACKET_MISMATCH = 9,
    MISSING_BRACKET = 10,
    INVALID_ACCESS_SPECIFIER = 11,
    MULTIPLE_DEFINITION = 12,
    PREVIOUSLY_DEFINED = 13,
    DUPLICATE_CLASS = 14,
    REDUNDANT_TOKEN = 15,
    INTERNAL_ERROR = 16,
    COULD_NOT_RESOLVE = 17,
    EXPECTED_REFRENCE_OF_TYPE = 18,
    INVALID_CAST = 19,
    REDUNDANT_CAST = 20,
    REDUNDANT_IMPORT = 21,
    UNEXPECTED_TOKEN = 22,
    INVALID_ACCESS = 23,
    SYMBOL_ALREADY_DEFINED = 24,
    INVALID_PARAM = 25,
    INCOMPATIBLE_TYPES = 26,
    DUPlICATE_DECLIRATION = 27,

    NO_ERR = 999
};

static std::list<KeyPair<error_type, string>> predefinedErrors;
void initalizeErrors();
class parser;

struct ParseError
{
public:
    ParseError()
            :
            error()
    {
        id = NO_ERR;
        line = -1;
        col = -1;
    }
    ParseError(const ParseError &pe)
    {
        operator=(pe);
    }

    ParseError(KeyPair<error_type, string> err, int l, int c, string addon = "")
    {
        id = err.key;
        error = (err.value + addon);
        line = l;
        col = c;
        warning = false;
    }

    ParseError(bool warning, KeyPair<error_type, string> err, int l, int c, string addon = "")
    {
        id = err.key;
        error = (err.value + addon);
        line = l;
        col = c;
        this->warning = warning;
    }

    ParseError(KeyPair<error_type, string> err, Token token, string addon = "")
    {
        id = err.key;
        error = (err.value + addon);
        line = token.getLine();
        col = token.getColumn();
        warning = false;
    }

    void operator=(const ParseError &pe)
    {
        error = pe.error;
        id=pe.id;
        line=pe.line;
        col=pe.col;
        warning=pe.warning;
    }

    void free() {
        error.clear();
    }

    error_type id;
    string error;
    int line;
    int col;
    bool warning;
};

class Ast;

class ErrorManager
{
public:
    ErrorManager(List<string>* lines, string file_name, bool asis, bool aggressiveRoporting)
            :
            lines(),
            _err(false),
            protectedMode(false),
            filename(file_name),
            asis(asis),
            aggressive(aggressiveRoporting)
    {
        this->lines = lines;
        errors = new list<ParseError>();
        warnings = new list<ParseError>();
        unfilteredErrors = new list<ParseError>();
        possibleErrors = new list<std::list<ParseError>*>();
        lastError = ParseError();
        lastCheckedError = ParseError();
    }

    void update(parser *p, bool asis, bool aggressiveRoporting);
    void printErrors();
    uint64_t getErrorCount() { return errors->size(); }
    uint64_t getWarningCount() { return warnings->size(); }
    uint64_t getUnfilteredErrorCount() { return unfilteredErrors->size(); }
    int createNewError(error_type err, Token token, string xcmts = "");
    int createNewError(error_type err, Ast* pAst, string xcmts = "");
    void createNewError(error_type err, int line, int col, string xcmts = "");
    void createNewWarning(error_type err, int line, int col, string xcmts);
    void createNewWarning(error_type err, Ast* pAst, string xcmts);
    bool hasErrors();
    bool hasWarnings() { return warnings; }
    void enterProtectedMode();
    void fail();
    void pass();
    void enableAggressive() { aggressive = true; }

    void free();
    string getLine(int line);

    string filename;
    List<string>* lines;
private:
    KeyPair<error_type, string> getErrorById(error_type);
    list<ParseError>* getPossibleErrorList();
    void addPossibleErrorList();
    void removePossibleErrorList();

    list<ParseError>* errors, *unfilteredErrors, *warnings;
    list<std::list<ParseError>*>* possibleErrors;
    ParseError lastError;
    ParseError lastCheckedError;
    bool _err, protectedMode;
    bool asis, aggressive;

    bool shouldReport(Token *token, const ParseError &last_err, const ParseError &e) const;

    string getErrors(list<ParseError> *errors);


    void printError(ParseError &err);

    bool hasError(list <ParseError> *e, const ParseError &parseerror1) const;

    bool shouldReportWarning(Token *token, const ParseError &last_err, const ParseError &e) const;
};


#endif //SHARP_PARSEERROR_H
