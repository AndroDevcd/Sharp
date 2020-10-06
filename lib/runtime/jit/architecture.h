//
// Created by BraxtonN on 9/25/2019.
//

#ifndef SHARP_ARCHITECTURE_H
#define SHARP_ARCHITECTURE_H

#include <cstdint>

#if defined(_M_X64) || defined(__x86_64__)
#define _ARCH_X86 64
#elif defined(_M_IX86) || defined(__X86__) || defined(__i386__)
#define _ARCH_X86 32
#else
  #define _ARCH_X86 0
#endif

#if defined(__arm64__) || defined(__aarch64__)
# define _ARCH_ARM 64
#elif defined(_M_ARM) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
#define _ARCH_ARM 32
#else
#define _ARCH_ARM 0
#endif

#if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
#define _ARCH_MIPS 64
#elif defined(_MIPS_ARCH_MIPS32) || defined(_M_MRX000) || defined(__mips__)
#define _ARCH_MIPS 32
#else
#define _ARCH_MIPS 0
#endif

#define _ARCH_BITS       (_ARCH_X86 | _ARCH_ARM | _ARCH_MIPS)
#if _ARCH_BITS == 0
#undef _ARCH_BITS
  #if defined (__LP64__) || defined(_LP64)
    #define _ARCH_BITS 64
  #else
    #define _ARCH_BITS 32
  #endif
#endif

#if (defined(__ARMEB__))  || \
    (defined(__MIPSEB__)) || \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#define _ARCH_LE 0
  #define _ARCH_BE 1
#else
#define _ARCH_LE 1
#define _ARCH_BE 0
#endif


#if _ARCH_BITS == 64
typedef int64_t x86int_t;
#else
typedef int32_t x86int_t;
#endif

#if (_ARCH_X86 == 64)  || \
    (_ARCH_X86 == 32)
#define BUILD_JIT 0
#endif


#endif //SHARP_ARCHITECTURE_H
