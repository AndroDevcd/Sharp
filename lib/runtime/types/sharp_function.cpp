//
// Created by bknun on 9/18/2022.
//

#include "sharp_function.h"
#include "../virtual_machine.h"
#include "../../util/list_helper.h"
#include "../main.h"

void init_struct(sharp_function *sf) {
    sf->address = 0;
    sf->guid = 0;
    new (&sf->name) string();
    new (&sf->fullName)  string();
    new (&sf->lineTable)  linkedlist<line_data>();
    new (&sf->tryCatchTable)  linkedlist<try_catch_data>();
    sf->owner = nullptr;
    sf->flags = 0;
    sf->bytecode = nullptr;
    sf->fnType = normal_function;
    sf->stackSize = 0;
    sf->paramSize = 0;
    sf->returnType = nullptr;
    sf->arrayReturn = 0;
    sf->sourceFile = 0;
    sf->bytecodeSize = 0;
    sf->delegateAddress = 0;
    sf->spOffset = 0;
    sf->fpOffset = 0;
    sf->frameStackOffset = 0;
    sf->nativeFunc = false;
    sf->linkAddr = 0;
    sf->bridge = nullptr;
}

string get_info(sharp_function *sf, Int pc) {
    stringstream ss;
    ss << "\tSource ";
    if(sf->sourceFile != -1 && sf->sourceFile < vm.manifest.sourceFiles) {
        ss << "\""; ss << vm.mdata.files.node_at(sf->sourceFile)->data->name << "\"";
    }
    else
        ss << "\"Unknown File\"";

    Int x=0, line=-1, linePos=-1;
    for(Int i = 0; i < sf->lineTable.size; i++) {
        auto info = sf->lineTable.node_at(i)->data;
        if(info.pc >= pc) {
            if(x > 0)
                linePos = x - 1;
            else
                linePos = x;
            break;
        }

        if(!((x+1) < sf->lineTable.size)) {
            linePos = x;
        }
        x++;
    }

    if(linePos != -1) {
        ss << ", line " << (line = sf->lineTable.node_at(linePos)->data.line_number);
    } else
        ss << ", line ?";

    ss << ", in "; ss << sf->fullName << "(" << (sf->nativeFunc ? "native" : "") << ")";

    if(c_options.debugMode) {
        ss << " ["
           << sf->address << "] $" << pc;
        ss << " " << (sf->nativeFunc ? "(jit)" : "");
    }

    if(sf->sourceFile != -1 && line != -1 && vm.mdata.files.size > 0) {
        auto info = get_info(vm.mdata.files.node_at(sf->sourceFile)->data, line);
        if(!info.empty()) ss << info;
    }

    return ss.str();
}