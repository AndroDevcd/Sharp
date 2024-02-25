//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_SHARP_FUNCTION_H
#define SHARP_SHARP_FUNCTION_H

#include "../../../stdimports.h"
#include "../../core/data_type.h"
#include "../../core/function_type.h"
#include "line_data.h"
#include "try_catch_data.h"
#include "../../util/linked_list.h"
#include "../snb/snb_internal.h"

struct sharp_class;
struct sharp_type;

struct function_param {
    sharp_type *type;
    bool isArray;
};

struct sharp_function {
    int32_t address;
    int32_t guid;
    string name;
    string fullName;
    sharp_class *owner;
    uint32_t flags;
    uint32_t* bytecode;
    function_type fnType;
    int stackSize;
    int paramSize;
    sharp_type* returnType;
    bool arrayReturn;
    long sourceFile;
    int32_t bytecodeSize;
    long delegateAddress;
    int spOffset;
    int fpOffset;
    int frameStackOffset;
    bool nativeFunc;
    uint32_t linkAddr;
    bridge_fun bridge;
    function_param *params;
    linkedlist<line_data> lineTable;
    linkedlist<try_catch_data> tryCatchTable;
};

void init_struct(sharp_function *sf);
string get_info(sharp_function *sf, Int pc);

#endif //SHARP_SHARP_FUNCTION_H
