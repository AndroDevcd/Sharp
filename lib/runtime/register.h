//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_REGISTER_H
#define SHARP_REGISTER_H

enum _register {
    ADX=0x0,        // address register
    CX=0x1,         // counter register
    CMT=0x2,        // compute register
    EBX=0x3,        // data register 1
    ECX=0x4,        // data register 2
    ECF=0x5,        // general register 1
    EDF=0x6,         // general register 2
    EHF=0x7,         // general register 3
    BMR=0x8,        // math register
    EGX=0x9        // temporary storage register
};

#endif //SHARP_REGISTER_H
