//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_OPCODE_H
#define SHARP_OPCODE_H

enum Opcode {
    nop=0x0, // no args
    push_str=0x1, // ([integer]) arg1: integer string refrence
    _int=0x2, // ([integer]) arg1: system call id
    pushi=0x3, // ([integer]) arg1: integer literal to push to stack
    ret=0x4, // no args
    hlt=0x5, // no args
    _new=0x6 // ([integer], [integer]) arg1: address to object arg2: address to class
};

#endif //SHARP_OPCODE_H
