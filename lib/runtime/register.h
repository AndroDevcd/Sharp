//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_REGISTER_H
#define SHARP_REGISTER_H

enum _register {
    adx=0x0,        // address register
    cx=0x1,         // counter register
    cmt=0x2,        // compute register
    ebx=0x3,        // data register 1
    ecx=0x4,        // data register 2
    ecf=0x5,        // general register 1
    edf=0x6,         // general register 2
    ehf=0x7,         // general register 3
    bmr=0x8,        // math register
    egx=0x9        // temporary storage register
};

#endif //SHARP_REGISTER_H
