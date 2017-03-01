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
    _new=0x6, // ([integer], [integer]) arg1: address to object arg2: address to class
    casti=0x7, // no args
    casts=0x8, // no args
    castl=0x9, // no args
    castc=0xa, // no args
    castb=0xb, // no args
    castf=0xc, // no args
    castd=0xd, // no args
    add=0xe, // no args
    sub=0xf, // no args
    mult=0x10, // no args
    _div=0x11, // no args
    mod=0x12, // no args
    pop=0x13, // no args
    load=0x14, // ([integer]) arg1: object address
    geti, // noargs
    gets, // noargs
    getl, // noargs
    getc, // noargs
    getb, // noargs
    getf, // noargs
    getd, // noargs
    get_str, // noargs
    get_arrx, // ([integer]) arg1: index of array
    _new2, // ([integer]) arg1: address to class
    null, // no args
    _new3, // ([integer]) arg1: type of native object
    _new4, // ([integer]) arg1: length of array
    _new5, // no args
    get_classx, // ([integer]) arg1: index of field
    get_ref, // no args
    rstore, // no args
    istore, // no args
    sstore, // no args
    lstore, // no args
    cstore, // no args
    bstore, // no args
    fstore, // no args
    dstore, // no args
    store_str, // no args
    _copy, // no args
    ifeq,
    ifneq,
    iflt,
    ifge,
    ifgt,
    ifle,
    str_cmpeq,
    str_cmpne,
    str_cmplt,
    str_cmpgt,
    str_cmple,
    str_cmpge,
    jmpeq,
    jmpne,
    neg,
    _and,
    _or,
    _xor,
    and2,
    or2,
    _goto,
    _iadr,
    invoke, // ([integer], [integer]) arg1: address to function arg2: address to class object
};

#endif //SHARP_OPCODE_H
