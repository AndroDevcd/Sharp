//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_EXE_H
#define SHARP_EXE_H

#include "../../stdimports.h"
#include "Manifest.h"
#include "oo/Field.h"
#include "oo/ClassObject.h"
#include "oo/Object.h"

extern Manifest manifest;
extern Meta metaData;

struct i64_t {
    int32_t A;
    int32_t B;
};

#define offset 0xf
#define file_sig 0x0f
#define digi_sig1 0x07
#define digi_sig2 0x1b
#define digi_sig3 0x0c
#define manif 0x1
#define eoh 0x03
#define nil 0x0
#define hsz 0x0a
#define eos 0x1d
#define sdata 0x05
#define sstring 0x02
#define stext 0x0e
#define smeta 0x06
#define data_class 0x2f
#define data_file 0x2c
#define data_method 0x4c
#define data_field 0x22
#define data_string 0x1e
#define data_byte 0x05
#define I64_BYTES 8

#define SET_i32A(mi, i) mi.A=(i >> 32)

#define SET_i32B(mi, i) mi.B=( i & 0xFFFFFFFF)

#define SET_i64(mi, i) \
    SET_i32A(mi,i); \
    SET_i32B(mi,i); \


#define GET_i64(A, B) ((((int64_t)A << 32)) | ((int64_t)B & 0xFFFFFFFF))

#define GET_i32w(i) ((uint8_t)(i >> 24))
#define GET_i32x(i) ((uint8_t)((i >> 16) & 0xff))
#define GET_i32y(i) ((uint8_t)((i >> 8) & 0xff ))
#define GET_i32z(i) ((uint8_t)(i & 0xff))

#define SET_i32(w,x,y,z) (((uint8_t)z) | ((uint8_t)y << 8) | ((uint8_t)x << 16) | ((uint8_t)w << 24))

int Process_Exe(std::string);


#endif //SHARP_EXE_H
