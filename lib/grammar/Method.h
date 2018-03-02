//
// Created by BraxtonN on 2/5/2018.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H

#include "../../stdimports.h"
#include "Param.h"
#include "List.h"
#include "Assembler.h"
#include "Exception.h"

class ClassObject;

class Method {

public:
    Method()
            :
            name(""),
            owner(NULL),
            klass(NULL),
            type(UNDEFINED),
            module(""),
            note(),
            array(false),
            code(),
            sourceFileLink(0),
            localVariables(0),
            isConstructor(false),
            address(0)
    {
        this->finallyBlocks.init();
        this->unique_address_table.init();
        this->exceptions.init();
        this->line_table.init();
        this->modifiers.init();
        this->params.init();
        this->assembly_table.init();
    }

    Method(string name, string module, ClassObject* klass, List<Param>& params, List<AccessModifier>& modifiers,
           ClassObject* rtype, RuntimeNote note, long long sourceFile)
            :
            name(name),
            owner(klass),
            klass(rtype),
            type(UNDEFINED),
            module(module),
            note(note),
            array(false),
            code(),
            sourceFileLink(sourceFile),
            localVariables(0),
            isConstructor(false),
            address(0)
    {
        this->finallyBlocks.init();
        this->unique_address_table.init();
        this->exceptions.init();
        this->line_table.init();
        this->modifiers.init();
        this->params.init();
        this->assembly_table.init();
        this->params.addAll(params);
        this->modifiers.addAll(modifiers);
    }

    Method(string name, string module, ClassObject* klass, List<Param>& params, List<AccessModifier>& modifiers,
           FieldType rtype, RuntimeNote note, long long sourceFile)
            :
            name(name),
            owner(klass),
            type(rtype),
            klass(NULL),
            module(module),
            note(note),
            array(false),
            code(),
            sourceFileLink(sourceFile),
            localVariables(0),
            isConstructor(false),
            address(0)
    {
        this->finallyBlocks.init();
        this->unique_address_table.init();
        this->exceptions.init();
        this->line_table.init();
        this->modifiers.init();
        this->params.init();
        this->assembly_table.init();
        this->params.addAll(params);
        this->modifiers.addAll(modifiers);
    }

    ClassObject* getParentClass() { return owner; }
    string getName() { return name; }
    string getFullName();
    string getModule() { return module; }
    size_t paramCount() { return params.size(); }
    size_t nativeParamCount() { return params.size() + (isStatic() ? 0 : 1); }
    List<Param>& getParams() { return params; }
    Param& getParam(int p) { return params.get(p); }
    bool hasModifier(AccessModifier m) { return modifiers.find(m); }
    bool isStatic() { return modifiers.find(STATIC); }
    void free() {
        modifiers.free();

        for(unsigned int i = 0; i < params.size(); i++) {
            params.get(i).free();
        }
        params.free();
    }

    RuntimeNote note;
    int64_t address;
    ClassObject* klass;
    FieldType type;
    Assembler code;
    ClassObject* owner;
    long long sourceFileLink;
    bool array, isConstructor;
    int64_t localVariables;
    List<KeyPair<int64_t, long>> line_table;
    List<KeyPair<int64_t, int64_t>> assembly_table;
    List<ExceptionTable> exceptions;
    List<FinallyTable> finallyBlocks;
    List<long> unique_address_table;
private:
    List<AccessModifier> modifiers; // 3 max modifiers
    string name;
    string module;
    List<Param> params;
};



#endif //SHARP_METHOD_H
