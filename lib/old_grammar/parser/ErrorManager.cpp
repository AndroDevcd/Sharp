//
// Created by bknun on 1/18/2018.
//

#include "ErrorManager.h"
#include <sstream>
#include "../../util/KeyPair.h"
#include "Ast.h"
#include "../Runtime.h"

void initalizeErrors()
{
    KeyPair<error_type, string> err;

    err.set(UNEXPECTED_SYMBOL, "unexpected symbol");
    predefinedErrors.push_back(err);

    err.set(ILLEGAL_NUMBER_FORMAT, "illegal number format mismatch");
    predefinedErrors.push_back(err);

    err.set(UNEXPECTED_EOF, "unexpected end of file");
    predefinedErrors.push_back(err);

    err.set(EXPECTED_STRING_LITERAL_EOF, "expected string literal before end of file");
    predefinedErrors.push_back(err);

    err.set(ILLEGAL_STRING_FORMAT, "illegal string format");
    predefinedErrors.push_back(err);

    err.set(EXPECTED_CHAR_LITERAL_EOF, "expected character literal before end of file");
    predefinedErrors.push_back(err);

    err.set(ILLEGAL_CHAR_LITERAL_FORMAT, "illegal character literal format");
    predefinedErrors.push_back(err);

    err.set(GENERIC, "");
    predefinedErrors.push_back(err);

    err.set(ILLEGAL_ACCESS_DECLARATION, "illegal specification of access specifier(s)");
    predefinedErrors.push_back(err);

    err.set(ILLEGAL_BRACKET_MISMATCH, "illegal symbol mismatch, unexpected bracket");
    predefinedErrors.push_back(err);

    err.set(MISSING_BRACKET, "missing bracket");
    predefinedErrors.push_back(err);

    err.set(INVALID_ACCESS_SPECIFIER, "invalid access specifier");
    predefinedErrors.push_back(err);

    err.set(MULTIPLE_DEFINITION, "multiple definition of");
    predefinedErrors.push_back(err);

    err.set(PREVIOUSLY_DEFINED, "");
    predefinedErrors.push_back(err);

    err.set(DUPLICATE_CLASS, "duplicate class:");
    predefinedErrors.push_back(err);

    err.set(REDUNDANT_TOKEN, "redundant token");
    predefinedErrors.push_back(err);

    err.set(INTERNAL_ERROR, "internal runtime error");
    predefinedErrors.push_back(err);

    err.set(COULD_NOT_RESOLVE, "could not resolve symbol");
    predefinedErrors.push_back(err);

    err.set(EXPECTED_REFRENCE_OF_TYPE, "expected refrence of type");
    predefinedErrors.push_back(err);

    err.set(INVALID_CAST, "invalid cAst of type");
    predefinedErrors.push_back(err);

    err.set(REDUNDANT_CAST, "redundant cAst of type");
    predefinedErrors.push_back(err);

    err.set(REDUNDANT_IMPORT, "redundant self import of module");
    predefinedErrors.push_back(err);

    err.set(UNEXPECTED_TOKEN, "unexpected token");
    predefinedErrors.push_back(err);

    err.set(INVALID_ACCESS, "invalid access of");
    predefinedErrors.push_back(err);

    err.set(SYMBOL_ALREADY_DEFINED, "");
    predefinedErrors.push_back(err);

    err.set(INVALID_PARAM, "invalid param of type");
    predefinedErrors.push_back(err);

    err.set(INCOMPATIBLE_TYPES, "incompatible types");
    predefinedErrors.push_back(err);

    err.set(DUPlICATE_DECLIRATION, "duplicate declaration of");
    predefinedErrors.push_back(err);
}

void ErrorManager::printError(ParseError &err) {
    if(err.warning)
        cout << filname << ":" << err.line << ":" << err.col << ": warning S60" << err.id << ":  " << err.error.c_str()
             << endl;
    else
        cout << filname << ":" << err.line << ":" << err.col << ": error S80" << err.id << ":  " << err.error.c_str()
             << endl;

    cout << '\t' << getLine(err.line) << endl << '\t';

    for(int i = 0; i < err.col-1; i++)
        cout << " ";
    cout << "^" << endl;
}

string ErrorManager::getErrors(list<ParseError>* errors)
{
    stringstream errorlist;
    for(const ParseError &err : *errors)
    {
        if(err.warning)
            errorlist << filname << ":" << err.line << ":" << err.col << ": warning S60" << err.id << ":  " << err.error.c_str()
                      << endl;
        else
            errorlist << filname << ":" << err.line << ":" << err.col << ": error S80" << err.id << ":  " << err.error.c_str()
                      << endl;

        errorlist << '\t' << getLine(err.line) << endl << '\t';


        for(int i = 0; i < err.col-1; i++)
            errorlist << " ";
        errorlist << "^" << endl;
    }

    return errorlist.str();
}

void ErrorManager::printErrors() {
    if(!asis) {
        if(_err) {
            if(aggressive || (errors->size() == 0 && unfilteredErrors->size() > 0)) // print aggressive errors
                cout << getErrors(unfilteredErrors);
            else // print optimized errors
                cout << getErrors(errors);
        }
        cout << getErrors(warnings);
    }
}

int ErrorManager::createNewError(error_type err, token_entity token, string xcmts) {
    KeyPair<error_type, string> kp = getErrorById(err);
    ParseError e(kp, token, xcmts);
    ParseError lastError = cm ? lastCheckedError : this->lastError;

    if(shouldReport(&token, lastError, e) || (aggressive && asis))
    {
        if(asis) {
            printError(e);
        } else if(cm) {
            getPossibleErrorList()->push_back(e);
            lastCheckedError = e;
            return 1;
        }

        _err = true;
        errors->push_back(e);
        unfilteredErrors->push_back(e);
        this->lastError = e;
        return 1;
    }
    else {
        unfilteredErrors->push_back(e);
    }

    return 0;
}

bool ErrorManager::shouldReport(token_entity *token, const ParseError &lastError,
                          const ParseError &e) const {
    if(lastError.error != e.error && !(lastError.line == e.line && lastError.col == e.col)
       && (lastError.error.find(e.error) == std::string::npos) && !hasError(errors, e))
    {
        if(token != NULL && !(token->getId() == SINGLE || token->getId() == CHAR_LITERAL ||
                              token->getId() == STRING_LITERAL || token->getId() == INTEGER_LITERAL))
            return (lastError.error.find(token->getToken()) == std::string::npos) &&
                   ((lastError.line-e.line)!=-1);

        return true;
    }


    return false;
}


bool ErrorManager::shouldReportWarning(token_entity *token, const ParseError &lastError,
                                 const ParseError &e) const {
    if(lastError.error != e.error && !(lastError.line == e.line && lastError.col == e.col)
       && (lastError.error.find(e.error) == std::string::npos))
    {
        if(token != NULL && !(token->getId() == SINGLE || token->getId() == CHAR_LITERAL ||
                              token->getId() == STRING_LITERAL || token->getId() == INTEGER_LITERAL))
            return (lastError.error.find(token->getToken()) == std::string::npos) &&
                   ((lastError.line-e.line)!=-1);

        return true;
    }


    return false;
}

void ErrorManager::createNewError(error_type err, int l, int c, string xcmts) {
    KeyPair<error_type, string> kp = getErrorById(err);
    ParseError e(kp, l,c, xcmts);
    ParseError last_err = cm ? lastCheckedError : lastError;

    if(shouldReport(NULL, last_err, e) || (aggressive && asis))
    {
        if(asis) {
            printError(e);
        } else if(cm) {
            getPossibleErrorList()->push_back(e);
            lastCheckedError = e;
            return;
        }

        _err = true;
        errors->push_back(e);
        unfilteredErrors->push_back(e);
        lastError = e;
    }
    else {
        unfilteredErrors->push_back(e);
    }
}

void ErrorManager::createNewWarning(error_type err, int l, int c, string xcmts) {
    KeyPair<error_type, string> kp = getErrorById(err);
    ParseError e(true, kp, l,c, xcmts);
    ParseError last_err;
    if(warnings->size() > 0) {
        last_err = *std::next(warnings->begin(), warnings->size()-1);
    } else {
        last_err = cm ? lastCheckedError : lastError;
    }

    if(warnings->size() == 0 || shouldReportWarning(NULL, last_err, e)) {
        if(asis)
            printError(e);

        warnings->push_back(e);
    }
}

string ErrorManager::getLine(int line) {
    if((line-1) >= lines.size())
        return "End of File";
    else
        return lines.get(line-1);
}

KeyPair<error_type, string> ErrorManager::getErrorById(error_type err) {
    return *std::next(predefinedErrors.begin(), (int)err);
}

bool ErrorManager::hasErrors() {
    return _err && unfilteredErrors->size() != 0;
}

void ErrorManager::enableErrorCheckMode() {
    this->cm = true;
    addPossibleErrorList();
}

list<ParseError> *ErrorManager::getPossibleErrorList() {
    return *std::next(possibleErrors->begin(), teCursor);
}

void ErrorManager::removePossibleErrorList() {
    if(possibleErrors->size() != 0)
    {
        list<ParseError> *lst = *std::next(possibleErrors->begin(), teCursor);
        lst->clear();
        delete lst;
        possibleErrors->pop_back();
        teCursor--;
        if(teCursor < 0) cm = false;
    }
}

void ErrorManager::addPossibleErrorList() {
    possibleErrors->push_back(new list<ParseError>());
    teCursor++;
}

void ErrorManager::fail() {
    if(possibleErrors->size() > 0) {

        for(ParseError &err : *getPossibleErrorList())
        {
            if(shouldReport(NULL, lastError, err)) {
                errors->push_back(err);
                lastError = err;
                unfilteredErrors->push_back(err);
            }
        }

        lastCheckedError = ParseError();
        removePossibleErrorList();

        if(teCursor <= 0) {
            lastError = lastCheckedError;
            _err = true;
        }
    }

}

void ErrorManager::pass() {
    lastCheckedError = ParseError();
    removePossibleErrorList();
}

void ErrorManager::free() {
    this->cm = false;
    this->_err = false;
    this->lastCheckedError.free();
    this->lastError.free();
    this->lines.free();
    RuntimeEngine::freeList(*errors);
    RuntimeEngine::freeList(*warnings);
    RuntimeEngine::freeList(*unfilteredErrors);
    for(std::list<ParseError>* lst : *possibleErrors) {
        RuntimeEngine::freeList(*lst);
        delete (lst);
    }
    possibleErrors->clear();
    delete (errors); this->errors = NULL;
    delete (warnings); this->warnings = NULL;
    delete (possibleErrors); this->possibleErrors = NULL;
    delete (unfilteredErrors); this->unfilteredErrors = NULL;
}

bool ErrorManager::hasError(list <ParseError> *e, const ParseError &perror) const {
    for(ParseError& pe : *e) {
        if(pe.error == perror.error)
            return true;
    }
    return false;
}

int ErrorManager::createNewError(error_type err, Ast *pAst, string xcmts) {
    KeyPair<error_type, string> kp = getErrorById(err);
    ParseError e(kp, pAst->line, pAst->col, xcmts);
    ParseError last_err = cm ? lastCheckedError : lastError;

    if(shouldReport(NULL, last_err, e) || (aggressive && asis))
    {
        if(asis) {
            printError(e);
        } else if(cm) {
            getPossibleErrorList()->push_back(e);
            lastCheckedError = e;
            return 1;
        }

        _err = true;
        errors->push_back(e);
        unfilteredErrors->push_back(e);
        lastError = e;
        return 1;
    }
    else {
        unfilteredErrors->push_back(e);
    }

    return 0;
}

void ErrorManager::createNewWarning(error_type err, Ast *pAst, string xcmts) {
    KeyPair<error_type, string> kp = getErrorById(err);
    ParseError e(true, kp, pAst->line, pAst->col, xcmts);
    ParseError last_err;
    if(warnings->size() > 0) {
        last_err = *std::next(warnings->begin(), warnings->size()-1);
    } else {
        last_err = cm ? lastCheckedError : lastError;
    }

    if(warnings->size() == 0 || shouldReportWarning(NULL, last_err, e)) {
        if(asis)
            printError(e);

        warnings->push_back(e);
    }
}