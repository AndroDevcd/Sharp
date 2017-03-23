//
// Created by bknun on 2/18/2017.
//

#ifndef SHARP_EXE_H
#define SHARP_EXE_H

#include "../../../stdimports.h"
#include "../Manifest.h"
#include "../Meta.h"

extern uint64_t jobIndx;
extern Manifest manifest;

struct mi64_t {
    int32_t A;
    int32_t B;
};

#define SET_mi32A(mi, i) mi.A=(i >> 32)

#define SET_mi32B(mi, i) mi.B=( i & 0xFFFFFFFF)

#define SET_mi64(mi, i) \
    SET_mi32A(mi,i); \
    SET_mi32B(mi,i); \


#define GET_mi64(A, B) ((((int64_t)A << 32)) | ((int64_t)B & 0xFFFFFFFF))

#define GET_mi32w(i) ((uint8_t)(i >> 24))
#define GET_mi32x(i) ((uint8_t)((i >> 16) & 0xff))
#define GET_mi32y(i) ((uint8_t)((i >> 8) & 0xff ))
#define GET_mi32z(i) ((uint8_t)(i & 0xff))

#define SET_mi32(w,x,y,z) (((uint8_t)z) | ((uint8_t)y << 8) | ((uint8_t)x << 16) | ((uint8_t)w << 24))

int Process_Exe(std::string);

#endif //SHARP_EXE_H
