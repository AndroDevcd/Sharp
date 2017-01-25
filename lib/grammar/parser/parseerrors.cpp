//
// Created by bknun on 1/6/2017.
//
#include <sstream>
#include "parseerrors.h"
#include "../../util/keypair.h"

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

    err.set(MISSING_BRACKET, "missing bracket");
    predefined_errs.push_back(err);
}

string Errors::geterrors(list<parseerror>* errors)
{
    stringstream errorlist;
    for(const parseerror &err : *errors)
    {
        errorlist << "error S800" << err.id << ":  " << err.error.c_str()
                  << ", at line " << err.line << ":" << err.col << endl;
        errorlist << '\t' << getline(err.line) << endl << '\t';

        for(int i = 0; i < err.col-1; i++)
            errorlist << " ";
        errorlist << "^" << endl;
    }

    return errorlist.str();
}

string Errors::getuo_errors() {
    return geterrors(uo_errors);
}

string Errors::getall_errors() {
    return geterrors(errors);
}

int Errors::newerror(p_errors err, token_entity token, string xcmts) {
    keypair<p_errors, string> kp = geterrorbyid(err);
    parseerror e(kp, token, xcmts);
    parseerror last_err = mode == _emcheck ? lastcheckederr : lasterr;

    if(shouldreport(&token, xcmts, last_err, e))
    {
        if(mode == _emcheck) {
            _testerrors->push_back(e);
            lastcheckederr = e;
            return 1;
        }

        _err = true;
        errors->push_back(e);
        uo_errors->push_back(e);
        lasterr = e;
        return 1;
    }
    else {
        uo_errors->push_back(e);
    }

    return 0;
}

bool Errors::shouldreport(token_entity *token, const string &xcmts,
                          const parseerror &last_err, const parseerror &e) const {
    if(last_err.error != e.error && !(last_err.line == e.line && last_err.col == e.col)
            && (last_err.error.find(xcmts) == std::string::npos))
    {
        if(token != NULL)
            return (last_err.error.find(token->gettoken()) == std::string::npos);

        return true;
    }

    return false;
}

void Errors::newerror(p_errors err, int l, int c, string xcmts) {
    keypair<p_errors, string> kp = geterrorbyid(err);
    parseerror e(kp, l,c, xcmts);
    parseerror last_err = mode == _emcheck ? lastcheckederr : lasterr;

    if(shouldreport(NULL, xcmts, last_err, e))
    {
        if(mode == _emcheck) {
            _testerrors->push_back(e);
            lastcheckederr = e;
            return;
        }

        _err = true;
        errors->push_back(e);
        uo_errors->push_back(e);
        lasterr = e;
    }
    else {
        uo_errors->push_back(e);
    }
}

string Errors::getline(int line) {
    if((line-1) >= lines->size())
        return "End of File";
    else
        return *std::next(lines->begin(), line-1);
}

keypair<p_errors, string> Errors::geterrorbyid(p_errors err) {
    return *std::next(predefined_errs.begin(), err);
}

bool Errors::_errs() {
    return _err;
}

void Errors::enable(int mode) {
    if(this->mode != _emnone) {
        this->mode = _emnone;
        newerror(GENERIC, 0,0, "faulty mode set, mode enabled unexpectedly.");
    }

    this->mode = mode;
}

void Errors::disable(void *arg) {
    bool pass;

    switch(mode) {
        case _emcheck:
            pass = (bool)arg;
            if(_testerrors->size() > 0) {
                if(pass)
                    goto end;

                for(parseerror &err : *_testerrors)
                {
                    errors->push_back(err);
                    uo_errors->push_back(err);
                }
                _err = true;
                lasterr = lastcheckederr;
            }

        end:
            _testerrors->clear();
            lastcheckederr = parseerror();
            mode = _emnone;
            break;
        default:
            break;
    }
}

void Errors::free() {
    this->mode = _emnone;
    this->_err = false;
    this->lastcheckederr = parseerror();
    this->lasterr = parseerror();
    this->lines = NULL;
    this->errors->clear();
    this->_testerrors->clear();
    std::free(errors); this->errors = NULL;
    std::free(_testerrors); this->_testerrors = NULL;
}
