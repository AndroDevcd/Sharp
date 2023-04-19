//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_STACK_LIMITS_H
#define SHARP_STACK_LIMITS_H

#include "../../../../stdimports.h"
#include "../../memory/garbage_collector.h"

#define INTERNAL_STACK_SIZE (KB_TO_BYTES(64) / sizeof(StackElement))
#define INTERNAL_STACK_MIN KB_TO_BYTES(4)
#define STACK_SIZE MB_TO_BYTES(1)
#define STACK_MIN KB_TO_BYTES(50)
#define STACK_OVERFLOW_BUF KB_TO_BYTES(10) // VERY LARGE OVERFLOW BUFFER FOR jit STACK OFERFLOW CATCHES

extern size_t threadStackSize;
extern size_t virtualStackSize;

bool valid_stack_size(uInt stackSize);
bool valid_internal_stack_size(uInt stackSize);

void initialize_main_thread_stack(std::list<string>& appArgs);

#endif //SHARP_STACK_LIMITS_H
