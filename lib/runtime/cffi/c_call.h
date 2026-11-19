//
// CALLING C: a function of any C library, called with the arguments C expects where C expects
// them - without a line of C++ written for that function.
//
//      c_signature s;
//      c_parse_signature("(i32,ptr,{i32 i32 i32 i32})i32", s, why);    // once: what the function takes
//      c_invoke(s, address, arguments, &result, nullptr);               // each call
//
// THE TEXT OF A SIGNATURE (what the compiler writes for an `extern` function):
//
//      (argument,argument,...)result
//
//      i8 u8 i16 u16 i32 u32 i64 u64      whole numbers of that many bits
//      f32 f64                            float, double
//      ptr                                any pointer   (cstr, buf and fn are pointers too: the names say
//                                         what Sharp hands over - c_bind.cpp reads them)
//      long ulong size                    C's long, unsigned long and size_t: as wide as THIS system has them
//      void                               no result
//      { i32 f32 {i8 i8} u8[16] }         a struct handed over by value: its members in order
//      { x:i32 y:i32 }                    ... each may carry its name
//      *{ x:i32 y:i32 }                   a pointer to such a struct
//      fn(i32,i32)i32                     a function handed to C: a pointer to a function with that signature
//
// HOW A CALL IS MADE, without machine code of our own. Every calling convention says, for each
// argument, which register or which place on the stack it travels in. c_parse_signature works that out
// once (the PLAN: a list of "copy these bytes of that argument there"). c_invoke fills three blocks of
// bytes from it - the integer registers, the floating point registers, the stack - and calls the
// function through a C++ function type that takes ALL of those registers: the C++ compiler loads every
// one, the C function reads the ones it uses. What comes back is read from the result registers the
// same way. One file, four conventions: Windows x64, Linux x64, ARM64, ARM32 (hard float).
//
// This file knows nothing of the VM: tests/cffi/engine_test.cpp tests it on its own.
//
#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum c_kind : uint8_t {
    C_VOID, C_I8, C_U8, C_I16, C_U16, C_I32, C_U32, C_I64, C_U64, C_F32, C_F64, C_PTR, C_STRUCT
};

/** One number or pointer inside a struct, and where it starts. */
struct c_field {
    c_kind kind;
    uint32_t offset;
};

struct c_type {
    c_kind kind = C_VOID;
    uint32_t size = 0;
    uint32_t align = 1;
    std::vector<c_field> fields;        // C_STRUCT: every number in it (those of a struct inside it too)
    std::string text;                   // as it was written
};

/** "Copy `bytes` bytes of argument `arg`, starting at `from`, to `to` of the registers or the stack." */
struct c_move {
    uint16_t arg;
    uint8_t source;                     // C_FROM_...
    uint8_t place;                      // C_IN_...
    uint16_t from;
    uint16_t to;
    uint16_t bytes;
};

enum { C_FROM_VALUE, C_FROM_COPY_ADDRESS, C_FROM_RESULT_ADDRESS };
enum { C_IN_INT, C_IN_FP, C_IN_STACK };

struct c_signature {
    std::vector<c_type> args;
    c_type result;
    std::string text;

    // ---- the plan (c_parse_signature)
    std::vector<c_move> moves;          // the arguments
    std::vector<c_move> pieces;         // the result: `place` is the result registers (C_IN_INT / C_IN_FP), `to` is in the result
    uint8_t shape = 0;                  // which registers the result comes back in (the convention's own numbering)
    uint8_t fp_slots = 0;               // Windows: which of the four first arguments travel in floating point registers
    bool result_in_memory = false;      // the function writes its result where a hidden pointer says
    uint32_t stack_bytes = 0;
    uint32_t copy_bytes = 0;            // room for the structs that are handed over as "the address of a copy"
};

/** What a call hands over for one argument, and what it gets back. */
union c_value {
    int64_t i;                          // every whole number
    double d;                           // f32 and f64
    void *p;                            // a pointer; for a struct, where its bytes are
};

/** The most arguments a function may have, and the largest struct that may come back by value. */
const int C_MAX_ARGUMENTS = 32;
const uint32_t C_MAX_RESULT = 4096;

/** The name of the calling convention this was built for ("" when it is not one of the four). */
const char *c_convention();

/** One type from its text. false and `why` when the text is wrong. */
bool c_parse_type(const std::string &text, c_type &type, std::string &why);

/**
 * A signature from its text, with the plan for this system. false and `why` when the text is
 * wrong or names something this system's convention cannot do.
 */
bool c_parse_signature(const std::string &text, c_signature &signature, std::string &why);

/**
 * Call `function`.
 *
 *      arguments       one per argument of the signature
 *      result          where a number or pointer result goes (may be null for void)
 *      struct_result   where a struct result goes: `signature.result.size` bytes
 */
void c_invoke(const c_signature &signature, void *function, const c_value *arguments, c_value *result, void *struct_result);

/** How wide `kind` is on this system. */
uint32_t c_size_of(c_kind kind);
