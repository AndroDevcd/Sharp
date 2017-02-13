//
// Created by BraxtonN on 1/30/2017.
//

#ifndef SHARP_RUNTIME_H
#define SHARP_RUNTIME_H

#include "../../stdimports.h"
#include "parser/parser.h"
#include "ClassObject.h"

class runtime
{
public:
    runtime(string out, list<parser*> parsers)
    :
            parsers(parsers),
            out(out),
            uid(0),
            errs(0),
            uo_errs(0),
            current_module(""),
            last_note("","",0,0),
            last_notemsg("")
    {
        for(parser* p : parsers) {
            if(!p->parsed)
                return;
        }

        macros = new list<Method>();
        modules = new list<string>();
        classes = new list<ClassObject>();
        import_map = new list<keypair<string, list<string>>>();
        interpret();
    }


    bool module_exists(string name);
    bool class_exists(string module, string name);
    bool add_class(ClassObject klass);
    bool add_macros(Method macro);
    void add_module(string name);
    ClassObject* getClass(string module, string name);
    Method* getmacros(string module, string name, list<Param> params);
    void cleanup();

    Errors* errors;
    size_t errs, uo_errs;
private:
    parser* _current;
    list<parser*> parsers;
    string out;
    list<string>* modules;
    list<Method>* macros;
    string current_module;
    list<ClassObject>* classes;
    list<keypair<string, std::list<string>>>*  import_map;
    uint64_t uid;

    /* One off variables */
    RuntimeNote last_note;
    string last_notemsg;

    void interpret();

    bool preprocess();

    string ast_tostring(ast *pAst);

    list<AccessModifier> preprocc_access_modifier(ast *trunk);

    AccessModifier entity_tomodifier(token_entity entity);

    void preprocc_class_decl(ast *trunk, ClassObject* parent);

    void preprocc_var_decl(ast *pAst, ClassObject *pObject);

    bool isaccess_decl(token_entity token);

    int isvar_access_specifiers(list<AccessModifier>& modifiers);

    NativeField entity_tonativefield(token_entity entity);

    void preprocc_method_decl(ast *pAst, ClassObject *pObject);

    int ismethod_access_specifiers(list<AccessModifier> &modifiers);

    list<Param> ast_toparams(ast *pAst, ClassObject* parent);

    string get_modulename(ast *pAst);

    list<string> parse_templArgs(ast *pAst);

    void preprocc_operator_decl(ast *pAst, ClassObject *pObject);

    void preprocc_constructor_decl(ast *pAst, ClassObject *pObject);

    void preprocc_macros_decl(ast *pAst, ClassObject *pObject);

    int ismacro_access_specifiers(list<AccessModifier> &modifiers);

    void warning(p_errors error, int line, int col, string xcmnts);

    void printnote(RuntimeNote& note, string msg);

    void parse_class_decl(ast *pAst, ClassObject* pObject);

    ClassObject *parse_base_class(ast *pAst);
};

#define progname "bootstrap"
#define progvers "0.1.0"

struct options {
    /*
     * Activate aggressive error reporting for the bootstrapper.
     */
    bool aggressive_errors = false;

    /*
     * Only compile all the files
     */
    bool compile = false;

    /*
     * Output file to write to
     */
    string out = "out";

    /*
     * Disable warnings
     */
    bool warnings = true;

    /*
     * Optimize code
     */
    bool optimize = false;

    /*
     * Enable warnings as errors
     */
    bool werrors = false;
};

int _bootstrap(int argc, const char* argv[]);

extern options c_options;

template <class T>
inline T& element_at(list<T>& l, size_t x) {
    return *std::next(l.begin(), x);
}

template <class T>
inline T& element_at(list<T>& l, T search) {
    for(T var : l) {
        if(search == var)
            return var;
    }

    return *std::next(l.begin(), 0);
}

template <class T>
inline long element_index(list<T>& l, T search) {
    long iter=0;
    for(T var : l) {
        if(search == var)
            return iter;
        iter++;
    }

    return -1;
}

template <class T>
inline bool element_has(list<T>& l, T search) {
    for(T var : l) {
        if(search == var)
            return true;
    }
    return false;
}

#endif //SHARP_RUNTIME_H
