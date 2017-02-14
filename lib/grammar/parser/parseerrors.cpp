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

    err.set(INVALID_ACCESS_SPECIFIER, "invalid access specifier");
    predefined_errs.push_back(err);

    err.set(MULTIPLE_DEFINITION, "multiple definition of");
    predefined_errs.push_back(err);

    err.set(PREVIOUSLY_DEFINED, "");
    predefined_errs.push_back(err);

    err.set(DUPLICATE_CLASS, "duplicate class:");
    predefined_errs.push_back(err);

    err.set(REDUNDANT_TOKEN, "redundant token");
    predefined_errs.push_back(err);

    err.set(INTERNAL_ERROR, "internal runtime error");
    predefined_errs.push_back(err);

    err.set(COULD_NOT_RESOLVE, "could not resolve symbol");
    predefined_errs.push_back(err);

    err.set(EXPECTED_REFRENCE_OF_TYPE, "expected refrence of type");
    predefined_errs.push_back(err);
}

void Errors::print_error(parseerror &err) {
    if(err.warning)
        cout << fn << ":" << err.line << ":" << err.col << ": warning S60" << err.id << ":  " << err.error.c_str()
                  << endl;
    else
        cout << fn << ":" << err.line << ":" << err.col << ": error S80" << err.id << ":  " << err.error.c_str()
                  << endl;

    cout << '\t' << getline(err.line) << endl << '\t';

    for(int i = 0; i < err.col-1; i++)
        cout << " ";
    cout << "^" << endl;
}

string Errors::geterrors(list<parseerror>* errors)
{
    stringstream errorlist;
    for(const parseerror &err : *errors)
    {
        if(err.warning)
            errorlist << fn << ":" << err.line << ":" << err.col << ": warning S60" << err.id << ":  " << err.error.c_str()
                      << endl;
        else
            errorlist << fn << ":" << err.line << ":" << err.col << ": error S80" << err.id << ":  " << err.error.c_str()
                      << endl;

        errorlist << '\t' << getline(err.line) << endl << '\t';


        for(int i = 0; i < err.col-1; i++)
            errorlist << " ";
        errorlist << "^" << endl;
    }

    return errorlist.str();
}

void Errors::print_errors() {
    if(!asis) {
        if(_err) {
            if(aggressive) // print aggressive errors
                cout << geterrors(uo_errors);
            else // print optimized errors
                cout << geterrors(errors);
        } else // print warnings
            cout << geterrors(errors);
    }
}

int Errors::newerror(p_errors err, token_entity token, string xcmts) {
    keypair<p_errors, string> kp = geterrorbyid(err);
    parseerror e(kp, token, xcmts);
    parseerror last_err = cm ? lastcheckederr : lasterr;

    if(shouldreport(&token, last_err, e))
    {
        if(asis) {
            print_error(e);
        } else if(cm) {
            gettesterrorlist()->push_back(e);
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

bool Errors::shouldreport(token_entity *token, const parseerror &last_err,
                          const parseerror &e) const {
    if(last_err.error != e.error && !(last_err.line == e.line && last_err.col == e.col)
       && (last_err.error.find(e.error) == std::string::npos))
    {
        if(token != NULL && !(token->getid() == SINGLE || token->getid() == CHAR_LITERAL ||
                token->getid() == STRING_LITERAL || token->getid() == INTEGER_LITERAL))
            return (last_err.error.find(token->gettoken()) == std::string::npos) &&
                    ((last_err.line-e.line)!=-1);

        return true;
    }


    return false;
}

void Errors::newerror(p_errors err, int l, int c, string xcmts) {
    keypair<p_errors, string> kp = geterrorbyid(err);
    parseerror e(kp, l,c, xcmts);
    parseerror last_err = cm ? lastcheckederr : lasterr;

    if(shouldreport(NULL, last_err, e))
    {
        if(asis) {
            print_error(e);
        } else if(cm) {
            gettesterrorlist()->push_back(e);
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

void Errors::newwarning(p_errors err, int l, int c, string xcmts) {
    keypair<p_errors, string> kp = geterrorbyid(err);
    parseerror e(true, kp, l,c, xcmts);
    parseerror last_err = cm ? lastcheckederr : lasterr;

    if(asis)
        print_error(e);

    errors->push_back(e);
    warnings = true;
}

string Errors::getline(int line) {
    if((line-1) >= lines->size())
        return "End of File";
    else
        return *std::next(lines->begin(), line-1);
}

keypair<p_errors, string> Errors::geterrorbyid(p_errors err) {
    return *std::next(predefined_errs.begin(), (int)err);
}

bool Errors::_errs() {
    return _err && uo_errors->size() != 0;
}

void Errors::enablecheck_mode() {
    this->cm = true;
    addtesterror_list();
}

list<parseerror> *Errors::gettesterrorlist() {
    return *std::next(_testerrors->begin(), teCursor);
}

void Errors::removetesterror_list() {
    if(_testerrors->size() != 0)
    {
        list<parseerror> *lst = *std::next(_testerrors->begin(), teCursor);
        lst->clear();
        _testerrors->pop_back();
        teCursor--;
        if(teCursor < 0) cm = false;
    }
}

void Errors::addtesterror_list() {
    _testerrors->push_back(new list<parseerror>());
    teCursor++;
}

void Errors::fail() {
    if(_testerrors->size() > 0) {

        for(parseerror &err : *gettesterrorlist())
        {
            errors->push_back(err);
            uo_errors->push_back(err);
        }

        if(teCursor < 0) {
            lasterr = lastcheckederr;
            _err = true;
        }
    }

    lastcheckederr = parseerror();
    removetesterror_list();
}

void Errors::pass() {
    lastcheckederr = parseerror();
    removetesterror_list();
}

void Errors::free() {
    this->cm = false;
    this->_err = false;
    this->lastcheckederr = parseerror();
    this->lasterr = parseerror();
    this->lines = NULL;
    this->errors->clear();
    this->_testerrors->clear();
    std::free(errors); this->errors = NULL;
    std::free(_testerrors); this->_testerrors = NULL;
}
