//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_OPCODE_MACROS_H
#define SHARP_OPCODE_MACROS_H

/**
 * 32-Bit layout
 *
 *
 * Class E Instruction
    | 00000000 00000000 0000000  | 00000000 |  max: FF
    |	       ignored			|  opcode  |
    max 255 min 0

   Class D Instruction
    | 00000000 00000000 0000000  | 0 | 00000000 |  max: 7FFFFF
    |	       argument			| +-|  opcode  |
    max 8388607 min -8388607

    Class C Instruction
    | 00000000 00000000 00 |   0000   | 0 | | 0 | 00000000 | max: 3FFFF & F
    |       argument	  | argument | +-| | +-|  opcode  |
    arg 1 max 15 min -15
    arg 2 max 262143 min -262143

    Class B Instruction
    |  0000000  |  0000000  |  0000000  | 0 | | 0 | | 0 | 00000000 | max: 7F
    | argument |  argument |  argument  | +-| | +-| | +-|  opcode  |
    max 127 min -127
 */

#define OPCODE_MASK 0xffu

#define POSITIVE 1u
#define NEGATIVE 0u
#define ERR_STATE 1
#define NO_ERR 0

#define DA_MAX 0xFFFFFFu
#define CA1_MAX 0xFu
#define CA2_MAX 0x3FFFFu
#define BA_MAX 0x7Fu

#define SET_Ei(i, op) (i=op)
#define SET_Di(i, op, a1, n) (i=((op) | ((n & POSITIVE) << 8u) | ((uint32_t)a1 << 9u)))
#define SET_Ci(i, op, a1, n, a2, n2) (i=((op) | ((n & POSITIVE) << 8u) | ((n2 & POSITIVE) << 9u) | ((uint8_t)a1 << 10u) | ((uint32_t)a2 << 14u)))
#define SET_Bi(i, op, a1, n, a2, n2, a3, n3) (i=((op) | ((n & POSITIVE) << 8u) | ((n2 & POSITIVE) << 9u) | ((n3 & POSITIVE) << 10u) | ((uint8_t)a1 << 11u) | ((uint8_t)a2 << 18u) | ((uint8_t)a3 << 25u)))

#define GET_OP(i) (i & OPCODE_MASK)
#define GET_Da(i) (int32_t)(((i >> 8u) & POSITIVE) ? (i >> 9u) : (-1 * (i >> 9u)))
#define GET_Ca(i) (int32_t)(((i >> 8u) & POSITIVE) ? (i >> 10u & CA1_MAX) : (-1 * (i >> 10u & CA1_MAX)))
#define GET_Cb(i) (int32_t)(((i >> 9u) & POSITIVE) ? (i >> 14u) : (-1 * (i >> 14u)))
#define GET_Ba(i) (int32_t)(((i >> 8u) & POSITIVE) ? (i >> 11u & BA_MAX) : (-1 * (i >> 11u & BA_MAX)))
#define GET_Bb(i) (int32_t)(((i >> 9u) & POSITIVE) ? (i >> 18u & BA_MAX) : (-1 * (i >> 18u & BA_MAX)))
#define GET_Bc(i) (int32_t)(((i >> 10u) & POSITIVE) ? (i >> 25u & BA_MAX) : (-1 * (i >> 25u & BA_MAX)))

#endif //SHARP_OPCODE_MACROS_H
