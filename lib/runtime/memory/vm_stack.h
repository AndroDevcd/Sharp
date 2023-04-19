//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_VM_STACK_H
#define SHARP_VM_STACK_H

struct sharp_object;

struct stack_item {
    double var;
    sharp_object *obj;
};

#endif //SHARP_VM_STACK_H
