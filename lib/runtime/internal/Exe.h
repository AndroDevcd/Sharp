//
// Created by bknun on 2/18/2017.
//

#ifndef SHARP_EXE_H
#define SHARP_EXE_H

#include "../../../stdimports.h"
#include "../Manifest.h"
#include "../Meta.h"

extern stringstream sdf;
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


#define GET_mi64(A, B) (((int64_t)A << 32) | (B))

#define GET_mi32w(i) (i >> 24)
#define GET_mi32x(i) ((i >> 16) & 0xff)
#define GET_mi32y(i) ((i >> 8) & 0xff )
#define GET_mi32z(i) (i & 0xff)

#define SET_mi32(w,x,y,z) ((z) | ((int32_t)y << 8) | ((int32_t)x << 16) | ((int32_t)w << 24))

#define GET_mi32(exe) \
    SET_mi32( \
        GET_mi32w(exe.at(n++)), GET_mi32x(exe.at(n++)), \
        GET_mi32y(exe.at(n++)), GET_mi32z(exe.at(n++)) \
    )

int Process_Exe(std::string);

Manifest& getManifest();

Meta& getMetaData();

void pushStackDump();

void updateStackFile(string status);

#endif //SHARP_EXE_H
