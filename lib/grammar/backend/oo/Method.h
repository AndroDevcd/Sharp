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
        handler_pc = invalidAddr;
        localFieldAddress = invalidAddr;
        classAddress = invalidAddr;
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
            end_pc(0),
            exception_object_field_address(-1)
    {
    }

    void operator=(FinallyData *fd) {
        start_pc=fd->start_pc;
        end_pc=fd->end_pc;
        exception_object_field_address=fd->exception_object_field_address;
    }

    uInt exception_object_field_address;
    uInt start_pc, end_pc;
};

struct LineData {
    LineData()
            :
            start_pc(0),
            line(0)
    {
    }

    LineData(uInt startPc, Int line)
            :
            start_pc(startPc),
            line(line)
    {
    }

    void operator=(LineData *ld) {
        start_pc=ld->start_pc;
        line=ld->line;
    }

    uInt start_pc;
    Int line;
};

struct AsmData {
    AsmData()
            :
            start_pc(0),
            end_pc(0)
    {
    }

    void operator=(AsmData *ad) {
        start_pc=ad->start_pc;
        end_pc=ad->end_pc;
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

    void operator=(TryCatchData *data) {
        this->try_start_pc=data->try_start_pc;
        this->try_end_pc=data->try_end_pc;
        this->catchTable.addAll(data->catchTable);
        this->finallyData = data->finallyData;
    }

    Int try_start_pc, try_end_pc;
    Int block_start_pc, block_end_pc;
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
        col(0),
        scopeLevel(0)
    {
    }

    BranchTable(Int branch_pc, string labelName, Int _offset,
                Int line, Int col, Int scopeLevel)
            :
            branch_pc(branch_pc),
            _offset(_offset),
            labelName(labelName),
            resolved(false),
            line(line),
            col(col),
            scopeLevel(scopeLevel)
    {
    }

    bool resolved;          // have we already processed this branch map?
    Int branch_pc;          // where was the branch initated in the code
    string labelName;           // the label we were trying to access
    Int _offset;                // any offset to the address label
    Int line, col;          // where did it happen?
    Int scopeLevel;        // what part of scope was it found on?

    void free() {
        labelName = "";
    }
};

struct LabelData {
    LabelData()
            :
            start_pc(invalidAddr),
            labelName(""),
            scopeLevel(-1)
    {
    }

    LabelData(string labelName, Int start_pc, Int scopeLevel)
        :
            start_pc(start_pc),
            labelName(labelName),
            scopeLevel(scopeLevel)
    {
    }

    Int start_pc;          // where was the label created in the code
    string labelName;      // the name of the label
    Int scopeLevel;        // what part of scope was it found on?
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
            localVariablesSize(0),
            protectedCodeTable()
    {
    }

    Field* getLocalField(string name) {
        if(locals.size() == 0) return NULL;

        for(Int i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i)->name == name) {
                return locals.get(i);
            }
        }
        return NULL;
    }

    LabelData getLabel(string name) {
        for(Int i = 0; i < labelMap.size(); i++) {
            if(labelMap.get(i).labelName == name)
                return labelMap.get(i);
        }

        return LabelData();
    }

    CodeHolder code;
    Int localVariablesSize;
    List<Field*> locals;
    List<TryCatchData> tryCatchTable;
    List<KeyPair<Int, string>> localAliases;
    List<LabelData> labelMap;
    List<BranchTable> branchTable;
    List<AsmData> protectedCodeTable;
    List<LineData> lineTable;
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

    Method(string name, ModuleData* module, ClassObject* owner, Int guid, List<Field*> &params, List<AccessFlag> &flags, Meta &meta)
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
        this->guid = guid;
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
