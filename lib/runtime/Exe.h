//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_EXE_H
#define SHARP_EXE_H

#include "../../stdimports.h"

#define zoffset 0xf
#define file_sig 0x0f
#define digi_sig1 0x07
#define digi_sig2 0x1b
#define digi_sig3 0x0c
#define manif 0x1
#define eoh 0x03
#define nil 0x0
#define eos 0x1d
#define ssymbol 0x05
#define sstring 0x02
#define sconst 0x02 // TODO: add constant section
#define sdata 0x0e
#define smeta 0x06
#define data_class 0x2f
#define data_interface 0x2e
#define data_file 0x2c
#define data_method 0x4c
#define data_compress 0x5b
#define data_field 0x22
#define data_string 0x1e
#define data_const 0x1c
#define data_symbol 0x1f
#define data_byte 0x05
#define EXE_BYTE_CHUNK 4
#define min_file_vers 4
#define file_vers 4
#define BUILD_VERS versions.ALPHA
#define HEADER_SIZE 0xc

// Processing errors
#define CORRUPT_FILE 0x001
#define UNSUPPORTED_FILE_VERS 0x002
#define CORRUPT_MANIFEST 0x003
#define EMPTY_FILE 0x004
#define FILE_DOES_NOT_EXIST 0x005
#define FILE_NOT_AUTHENTIC 0x006
#define UNSUPPORTED_BUILD_VERSION 0x007
#define OUT_OF_MEMORY 0x008

#define data_compress_threshold 0xFFFF

#define GET_i32w(i) ((uint8_t)(i >> 24))
#define GET_i32x(i) ((uint8_t)((i >> 16) & 0xff))
#define GET_i32y(i) ((uint8_t)((i >> 8) & 0xff ))
#define GET_i32z(i) ((uint8_t)(i & 0xff))

#define SET_i32(w,x,y,z) (((uint8_t)z) | ((uint8_t)y << 8) | ((uint8_t)x << 16) | ((uint8_t)w << 24))

int Process_Exe(std::string&);
extern string exeErrorMessage;


#endif //SHARP_EXE_H
