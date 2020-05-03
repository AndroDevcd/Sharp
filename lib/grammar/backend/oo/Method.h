//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H


#include "../data/DataEntity.h"
#include "Field.h"
#include "FunctionType.h"
#include "../code/CodeHolder.h"

struct CatchData {
    CatchData()
            :
            handler_pc(invalidAddr),
            localFieldAddress(invalidAddr),
            classAddress(invalidAddr)
    {
    }

    void init() {
        CatchData();
    }

    void free() {
    }

    void operator=(CatchData* ct) {
        this->handler_pc=ct->handler_pc;
        this->handler_pc=ct->handler_pc;
        this->classAddress=ct->classAddress;
    }

    Int handler_pc;
    Int localFieldAddress;
    Int classAddress;
};

struct FinallyData {
    FinallyData()
            :
            start_pc(0),
            end_pc(0)
    {
    }

    void operator=(FinallyData *fd) {
        start_pc=fd->start_pc;
        end_pc=fd->end_pc;
    }

    uInt start_pc, end_pc;
};


struct TryCatchData {
    TryCatchData()
            :
            try_start_pc(invalidAddr),
            try_end_pc(invalidAddr),
            finallyData(NULL),
            catchTable()
    {
    }

    void init() {
        TryCatchData();
    }

    ~TryCatchData();

    void operator=(TryCatchData *data) {
        this->try_start_pc=data->try_start_pc;
        this->try_end_pc=data->try_end_pc;
        this->catchTable.addAll(data->catchTable);
        this->finallyData = data->finallyData;
    }

    Int try_start_pc, try_end_pc;
    List<CatchData> catchTable;
    FinallyData *finallyData;
};

struct BranchTable {
    BranchTable()
    :
        branch_pc(0),
        labelName(""),
        _offset(0),
        resolved(false),
        line(0),
        col(0)
    {
    }

    BranchTable(Int branch_pc, string labelName, Int _offset,
                Int line, Int col)
            :
            branch_pc(branch_pc),
            _offset(_offset),
            labelName(labelName),
            resolved(false),
            line(line),
            col(col)
    {
    }

    bool resolved;          // have we already processed this branch map?
    Int branch_pc;          // where was the branch initated in the code
    string labelName;           // the label we were trying to access
    Int _offset;                // any offset to the address label
    Int line, col;          // where did it happen?

    void free() {
        labelName = "";
    }
};

struct CodeData {
    CodeData()
    :
            code(),
            locals(),
            tryCatchTable(),
            localAliases(),
            labelMap(),
            branchTable(),
            localVariablesSize(0)
    {
    }

    Field* getLocalField(string name, Int scopeLevel) {
        if(locals.size() == 0) return NULL;

        for(Int i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i)->name == name && locals.at(i)->scopeLevel == scopeLevel) {
                return locals.get(i);
            }
        }
        return NULL;
    }

    Field* getLocalFieldHereOrHigher(string name, Int scopeLevel) {
        if(locals.size() == 0) return NULL;

        for(Int i = locals.size()-1; i >= 0; i--) {
            if (locals.at(i)->name == name && locals.at(i)->scopeLevel <= scopeLevel) {
                return locals.get(i);
            }
        }
        return NULL;
    }

    Int getLabelAddress(string name) {
        for(Int i = 0; i < labelMap.size(); i++) {
            if(labelMap.get(i).key == name)
                return labelMap.get(i).value;
        }

        return invalidAddr;
    }

    CodeHolder code;
    Int localVariablesSize;
    List<Field*> locals;
    List<TryCatchData> tryCatchTable;
    List<KeyPair<Int, string>> localAliases;
    List<KeyPair<string, Int>> labelMap;
    List<BranchTable> branchTable;
};

class Method : public DataEntity {
public:
    Method()
    :
        DataEntity(),
        fnType(fn_undefined),
        utype(NULL),
        overload(' '),
        extensionFun(false),
        delegateAddr(invalidAddr),
        data()
    {
        type = METHOD;
        params.init();
    }

    Method(string& name, string& module, ClassObject* owner, List<Field*> &params, List<AccessFlag> &flags, Meta &meta)
            :
            DataEntity(),
            fnType(fn_undefined),
            utype(NULL),
            overload(' '),
            extensionFun(false),
            data()
    {
        this->type = METHOD;
        this->params.init();
        this->name=name;
        this->module=module;
        this->owner=owner;
        this->flags.addAll(flags);
        this->params.addAll(params);
        this->meta.copy(meta);
        if(!flags.find(STATIC))
            this->data.localVariablesSize++; // allocate 1 slot for the class instance
    }

    void free();
    string toString();
    static string paramsToString(List<Field*> &params);
    void copy(Method *method) {
        DataEntity::copy(method);
        fnType = method->fnType;
        overload = method->overload;
        extensionFun = method->extensionFun;
        utype = method->utype;
        params.addAll(method->params);
    }

    function_type fnType;
    char overload;
    bool extensionFun;
    long delegateAddr;
    Utype* utype;
    List<Field*> params;
    CodeData data;
};


#endif //SHARP_METHOD_H
