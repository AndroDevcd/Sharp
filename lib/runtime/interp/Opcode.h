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
    geti=0x15, // noargs
    gets=0x16, // noargs
    getl=0x17, // noargs
    getc=0x18, // noargs
    getb=0x19, // noargs
    getf=0x1a, // noargs
    getd=0x1b, // noargs
    get_str=0x1c, // noargs
    get_arrx=0x1d, // ([integer]) arg1: index of array
    _new2=0x1e, // ([integer]) arg1: address to class
    null=0x1f, // no args
    _new3=0x20, // ([integer]) arg1: type of native object
    _new4=0x21, // ([integer]) arg1: length of array
    _new5=0x22, // no args
    get_classx=0x23, // ([integer]) arg1: index of field
    get_ref=0x24, // no args
    rstore=0x25, // no args
    istore=0x26, // no args
    sstore=0x27, // no args
    lstore=0x28, // no args
    cstore=0x29, // no args
    bstore=0x2a, // no args
    fstore=0x2b, // no args
    dstore=0x2c, // no args
    store_str=0x2d, // no args
    _copy=0x2e, // no args
    ifeq=0x2f, //
    ifneq=0x30,
    iflt=0x31,
    ifge=0x32,
    ifgt=0x33,
    ifle=0x34,
    str_cmpeq=0x35,
    str_cmpne=0x36,
    str_cmplt=0x37,
    str_cmpgt=0x38,
    str_cmple=0x39,
    str_cmpge=0x3a,
    jmpeq=0x3b,
    jmpne=0x3c,
    neg=0x3d,
    _and=0x3e,
    _or=0x3f,
    _xor=0x40,
    and2=0x41,
    or2=0x42,
    _goto=0x43,
    _iadr=0x44,
    invoke=0x45, // ([integer], [integer]) arg1: address to function arg2: address to class object
    instance_store=0x46, // no args
    get_self=0x47,
    arry_len=0x48,
    _throw=0x49,
    lload=0x4a, // ([integer]) arg1: object index
    _catch=0x4b,
    str_append=0x4c,
    str_append2=0x4d,
    _strtod=0x4e,
    _strtol=0x4f,
    _lsh=0x50,
    _rsh=0x51,
    _lbl=0x52, // no args
    iinc=0x53, // no args
    sinc=0x54, // no args
    linc=0x55, // no args
    cinc=0x56, // no args
    binc=0x57, // no args
    finc=0x58, // no args
    dinc=0x59, // no args
    idec=0x5a, // no args
    sdec=0x5b, // no args
    ldec=0x5c, // no args
    cdec=0x5d, // no args
    bdec=0x5e, // no args
    fdec=0x5f, // no args
    ddec=0x60, // no args
    _goto_e=0x61, // no args
    _goto_ne=0x62, // no args
    _swap=0x63, // no args
};

#endif //SHARP_OPCODE_H
