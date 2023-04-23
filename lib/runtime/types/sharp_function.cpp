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
    new (&sf->lineTable)  list<line_data>();
    new (&sf->tryCatchTable)  list<try_catch_data>();
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
        ss << "\""; ss << get_item_at(vm.md.files, sf->sourceFile)->name << "\"";
    }
    else
        ss << "\"Unknown File\"";

    Int x, line=-1, ptr=-1;
    for(x = 0; x < sf->lineTable.size(); x++)
    {
        if(get_item_at(sf->lineTable, x).pc >= pc) {
            if(x > 0)
                ptr = x-1;
            else
                ptr = x;
            break;
        }

        if(!((x+1) < sf->lineTable.size())) {
            ptr = x;
        }
    }

    if(ptr != -1) {
        ss << ", line " << (line = get_item_at(sf->lineTable, ptr).line_number);
    } else
        ss << ", line ?";

    ss << ", in "; ss << sf->fullName << "(" << (sf->nativeFunc ? "native" : "") << ")";

    if(c_options.debugMode) {
        ss << " ["
           << sf->address << "] $" << pc;
        ss << " " << (sf->nativeFunc ? "(jit)" : "");
    }

    if(line != -1 && vm.md.files.size() > 0) {
        ss << get_info(get_item_at(vm.md.files, sf->sourceFile), line);
    }
}