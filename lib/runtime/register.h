//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_REGISTER_H
#define SHARP_REGISTER_H

enum _register {
    i64adx=0x0,        // address register
    i64cx=0x1,         // counter register
    i64cmt=0x2,        // compute register
    i64ebx=0x3,        // data register 1
    i64ecx=0x4,        // data register 2
    i64ecf=0x5,        // general register 1
    i64edf=0x6,         // general register 2
    i64ehf=0x7,         // general register 3
    i64bmr=0x8,        // math register
    i64egx=0x9        // temporary storage register
};

#endif //SHARP_REGISTER_H
