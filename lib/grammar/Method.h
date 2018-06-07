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

#define DELEGATE_ADDRESS_DEFAULT -1

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
            address(-1),
            delegate(false),
            delegatePost(false),
            delegateAddress(DELEGATE_ADDRESS_DEFAULT),
            key("")
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
           ClassObject* rtype, RuntimeNote note, long long sourceFile, bool delegate, bool delegatePost)
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
            address(-1),
            delegate(delegate),
            delegatePost(delegatePost),
            delegateAddress(DELEGATE_ADDRESS_DEFAULT),
            key("")
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
           FieldType rtype, RuntimeNote note, long long sourceFile, bool delegate, bool delegatePost)
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
            address(-1),
            delegate(delegate),
            delegatePost(delegatePost),
            delegateAddress(DELEGATE_ADDRESS_DEFAULT),
            key("")
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
    bool sameModifiers(Method *func) {
        if(func->modifiers.size() != modifiers.size())
            return false;

        for(int i = 0; i < modifiers.size(); i++) {
            if(!func->hasModifier(modifiers.get(i))) {
                return false;
            }
        }

        return true;
    }
    void free() {
        modifiers.free();
        name.clear();
        key.clear();
        module.clear();

        for(unsigned int i = 0; i < params.size(); i++) {
            params.get(i).free();
        }
        params.free();
    }

    void operator=(Method func) {
        this->note = func.note;
        this->address=func.address;
        this->delegateAddress=func.delegateAddress;
        this->klass=func.klass;
        this->type=func.type;
        this->code.inject(code.size(), func.code);
        this->owner=func.owner;
        this->sourceFileLink=func.sourceFileLink;
        this->array=func.array;
        this->isConstructor=func.isConstructor;
        this->delegate=func.delegate;
        this->delegatePost=func.delegatePost;
        this->localVariables=func.localVariables;
        this->line_table.addAll(func.line_table);
        this->assembly_table.addAll(func.assembly_table);
        this->exceptions.addAll(func.exceptions);
        this->finallyBlocks.addAll(func.finallyBlocks);
        this->unique_address_table.addAll(func.unique_address_table);
        this->modifiers.addAll(func.modifiers);
        this->name=func.name;
        this->module=func.module;
        this->params.addAll(func.params);
        this->key=func.key;
    }

    RuntimeNote note;
    int64_t address, delegateAddress;
    ClassObject* klass;
    FieldType type;
    string key; // generic identifier
    Assembler code;
    ClassObject* owner;
    long long sourceFileLink;
    bool array, isConstructor;
    bool delegate, delegatePost;
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
