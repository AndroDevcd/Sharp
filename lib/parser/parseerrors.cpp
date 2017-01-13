//
// Created by bknun on 1/6/2017.
//
#include <sstream>
#include "parseerrors.h"
#include "../util/keypair.h"

void int_errs()
{
    keypair<p_errors , string> err;

    err.set(UNEXPECTED_SYMBOL, "unexpected symbol");
    predefined_errs.push_back(err);

    err.set(ILLEGAL_NUMBER_FORMAT, "illegal number format mismatch");
    predefined_errs.push_back(err);

    err.set(UNEXPECTED_EOF, "unexpected end of file");
    predefined_errs.push_back(err);

    err.set(EXPECTED_STRING_LITERAL_EOF, "expected string literal before end of file");
    predefined_errs.push_back(err);

    err.set(ILLEGAL_STRING_FORMAT, "illegal string format");
    predefined_errs.push_back(err);

    err.set(EXPECTED_CHAR_LITERAL_EOF, "expected character literal before end of file");
    predefined_errs.push_back(err);

    err.set(ILLEGAL_CHAR_LITERAL_FORMAT, "illegal character literal format");
    predefined_errs.push_back(err);

    err.set(GENERIC, "");
    predefined_errs.push_back(err);

    err.set(ILLEGAL_ACCESS_DECLARATION, "illegal specification of access specifier(s)");
    predefined_errs.push_back(err);

    err.set(ILLEGAL_BRACKET_MISMATCH, "illegal symbol mismatch, unexpected bracket");
    predefined_errs.push_back(err);
}

string Errors::getall_errors() {
    stringstream errorlist;
    for(const parseerror &err : *errors)
    {
        errorlist << "error S" << err.id << ":  " << err.error.c_str()
                  << ", at line " << err.line << ":" << err.col << endl;
        errorlist << '\t' << getline(err.line) << endl;

        for(int i = 0; i < (err.col-1); i++)
            errorlist << " ";
        errorlist << '\t' << "^" << endl;
    }

    return errorlist.str();
}

void Errors::newerror(p_errors err, token_entity token, string xcmts) {
    keypair<p_errors, string> kp = geterrorbyid(err);
    parseerror e(kp, token, xcmts);

    if(lasterr.error != e.error && !(lasterr.line == e.line && lasterr.col == e.col))
    {
        _err = true;
        errors->push_back(e);
        lasterr = e;
    }
}

void Errors::newerror(p_errors err, int l, int c, string xcmts) {
    keypair<p_errors, string> kp = geterrorbyid(err);
    parseerror e(kp, l,c, xcmts);

    if(lasterr.error != e.error && !(lasterr.line == l && lasterr.col == c))
    {
        _err = true;
        errors->push_back(e);
        lasterr = e;
    }
}

string Errors::getline(int line) {
    return *std::next(lines->begin(), line-1);
}

keypair<p_errors, string> Errors::geterrorbyid(p_errors err) {
    return *std::next(predefined_errs.begin(), (long)err);
}

bool Errors::_errs() {
    return _err;
}
