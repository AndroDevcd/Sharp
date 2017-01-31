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
            uid(0)
    {
        for(parser* p : parsers) {
            if(!p->parsed)
                return;
        }

        modules = new list<string>();
        classes = new list<ClassObject>();
        staticfunctions = new list<Method*>();
        staticfields = new list<Field*>();
        interpret();
    }


    bool module_exists(string name);
    bool class_exists(string module, string name);
    bool add_class(ClassObject& klass);
    void add_module(string name);
    Method* getStaticFunction(ClassObject* klass, string name, list<Param>& params);
    Field* getStaticField(ClassObject* klass, string name);
    ClassObject* getClass(string module, string name);
    void cleanup();

private:
    list<parser*> parsers;
    string out;
    Errors* errors;
    list<string>* modules;
    list<ClassObject>* classes;
    uint64_t uid;
    /* Quick lookup for static fiends and objects */
    const list<Method*>* staticfunctions;
    const list<Field*> *staticfields;

    void interpret();

    bool preprocess();
};

#endif //SHARP_RUNTIME_H
