/*
 * C functions for testing calls to C: every kind of argument and result the calling conventions
 * treat differently. Plain C on purpose (c_test_functions.c is compiled as C, on its own, so the
 * compiler cannot see the callers and has to follow the convention to the letter).
 *
 * engine_test.cpp calls each one twice - directly, and through lib/runtime/cffi - and the two
 * results must be the same. The same file is built as a library (c_test.dll / libc_test.so) for
 * the Sharp side's tests.
 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(C_TEST_LIBRARY)
#define C_TEST __declspec(dllexport)
#else
#define C_TEST
#endif

typedef struct { int32_t x, y, w, h; } rect16;
typedef struct { float x, y; } vec2;
typedef struct { uint8_t r, g, b, a; } color4;
typedef struct { double x, y; } dvec2;
typedef struct { int64_t a; double b; } mixed_id;
typedef struct { double a; int32_t b; } mixed_di;
typedef struct { int64_t a, b, c; } big24;
typedef struct { float x, y, z; } vec3;
typedef struct { float x, y, z, w; } vec4;
typedef struct { double a, b, c, d; } dvec4;
typedef struct { int8_t a; int64_t b; } pad16;
typedef struct { int16_t a; int8_t b; } small4;
typedef struct { int8_t a, b, c; } tiny3;
typedef struct { int32_t a; int8_t b; } odd8;
typedef struct { char name[20]; int32_t n; double v; } record32;
typedef struct { vec2 at; color4 tint; } nested12;
typedef struct { int64_t a; } one8;
typedef struct { double d; } oned;
typedef struct { float f; } onef;
typedef struct { int32_t a; float b; } if8;
typedef struct { float a; int32_t b; double c; } fid16;
typedef struct { double a; float b; float c; } dff16;
typedef struct { int32_t values[40]; } block160;

/* numbers in, a number out */
C_TEST int32_t add3(int32_t a, int32_t b, int32_t c);
C_TEST int64_t sum10(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, int64_t h, int64_t i, int64_t j);
C_TEST double mix12(int32_t a, double b, int32_t c, double d, int64_t e, float f, int32_t g, double h, int8_t i, float j, int64_t k, double l);
C_TEST double fsum10(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j);
C_TEST double dsum10(double a, double b, double c, double d, double e, double f, double g, double h, double i, double j);
C_TEST int64_t narrow(int8_t a, uint8_t b, int16_t c, uint16_t d, int32_t e, uint32_t f);
C_TEST double i64_align(int32_t a, int64_t b, int32_t c, int64_t d, int32_t e, int64_t f);
C_TEST double fp_backfill(float a, double b, float c, double d, float e, double f, float g);
C_TEST double twenty(int32_t a, double b, int32_t c, double d, int32_t e, double f, int32_t g, double h, int32_t i, double j,
                     int32_t k, double l, int32_t m, double n, int32_t o, double p, int32_t q, double r, int32_t s, double t);

/* every width of result */
C_TEST int8_t ret_i8(int32_t x);
C_TEST uint8_t ret_u8(int32_t x);
C_TEST int16_t ret_i16(int32_t x);
C_TEST uint16_t ret_u16(int32_t x);
C_TEST int32_t ret_i32(int64_t x);
C_TEST uint32_t ret_u32(int64_t x);
C_TEST int64_t ret_i64(int64_t x);
C_TEST float ret_f32(float x);
C_TEST double ret_f64(double x);
C_TEST void *ret_ptr(void *p, int32_t add);
C_TEST void no_result(int32_t *into, int32_t value);

/* pointers */
C_TEST int64_t text_length(const char *text);
C_TEST int64_t sum_array(const int32_t *values, int32_t count);
C_TEST void fill_rect(rect16 *into, int32_t base);
C_TEST double read_rect(const rect16 *from);

/* a struct in */
C_TEST double take_rect16(rect16 v);
C_TEST double take_vec2(vec2 v);
C_TEST double take_color4(color4 v);
C_TEST double take_dvec2(dvec2 v);
C_TEST double take_mixed_id(mixed_id v);
C_TEST double take_mixed_di(mixed_di v);
C_TEST double take_big24(big24 v);
C_TEST double take_vec3(vec3 v);
C_TEST double take_vec4(vec4 v);
C_TEST double take_dvec4(dvec4 v);
C_TEST double take_pad16(pad16 v);
C_TEST double take_small4(small4 v);
C_TEST double take_tiny3(tiny3 v);
C_TEST double take_odd8(odd8 v);
C_TEST double take_record32(record32 v);
C_TEST double take_nested12(nested12 v);
C_TEST double take_one8(one8 v);
C_TEST double take_oned(oned v);
C_TEST double take_onef(onef v);
C_TEST double take_if8(if8 v);
C_TEST double take_fid16(fid16 v);
C_TEST double take_dff16(dff16 v);
C_TEST double take_block160(block160 v);

/* structs among other arguments, and after the registers have run out */
C_TEST double after_many(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, rect16 r, vec2 v, double x, big24 g, color4 col, float f);
C_TEST double many_vectors(vec2 a, vec2 b, vec2 c, vec2 d, vec2 e, dvec2 f, dvec4 g, vec4 h, vec3 i);
C_TEST double ints_then_struct(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, pad16 p, small4 s, mixed_id m);
C_TEST double split_struct(int32_t a, rect16 r, int32_t b, big24 g, int32_t c);
C_TEST double changes_its_copy(big24 g, block160 b);
C_TEST double changes_its_copy_of(big24 g);
C_TEST double read_rect_or(const rect16 *from, double otherwise);
C_TEST rect16 *same_rect(rect16 *r);

/* a struct out */
C_TEST rect16 make_rect16(int32_t seed);
C_TEST vec2 make_vec2(int32_t seed);
C_TEST color4 make_color4(int32_t seed);
C_TEST dvec2 make_dvec2(int32_t seed);
C_TEST mixed_id make_mixed_id(int32_t seed);
C_TEST mixed_di make_mixed_di(int32_t seed);
C_TEST big24 make_big24(int32_t seed);
C_TEST vec3 make_vec3(int32_t seed);
C_TEST vec4 make_vec4(int32_t seed);
C_TEST dvec4 make_dvec4(int32_t seed);
C_TEST pad16 make_pad16(int32_t seed);
C_TEST small4 make_small4(int32_t seed);
C_TEST tiny3 make_tiny3(int32_t seed);
C_TEST odd8 make_odd8(int32_t seed);
C_TEST record32 make_record32(int32_t seed);
C_TEST nested12 make_nested12(int32_t seed);
C_TEST one8 make_one8(int32_t seed);
C_TEST oned make_oned(int32_t seed);
C_TEST onef make_onef(int32_t seed);
C_TEST if8 make_if8(int32_t seed);
C_TEST fid16 make_fid16(int32_t seed);
C_TEST dff16 make_dff16(int32_t seed);
C_TEST block160 make_block160(int32_t seed);
/* a struct out with arguments after the hidden pointer */
C_TEST big24 scale_big24(big24 v, int32_t by, double add);
C_TEST vec2 add_vec2(vec2 a, vec2 b);

/* functions handed to C (the callbacks' tests) */
typedef int32_t (*int_callback)(int32_t a, int32_t b);
typedef double (*mixed_callback)(int32_t a, double b, float c, int64_t d, void *data);
typedef int32_t (*compare_callback)(const void *a, const void *b);
C_TEST int32_t call_int(int_callback f, int32_t a, int32_t b);
C_TEST double call_mixed(mixed_callback f, void *data);
C_TEST void sort_ints(int32_t *values, int32_t count, compare_callback compare);
C_TEST void keep_callback(int_callback f);
C_TEST int32_t call_kept(int32_t a, int32_t b);
C_TEST int32_t call_on_other_thread(int_callback f, int32_t a, int32_t b);

/* variable arguments */
C_TEST int64_t va_ints(int32_t count, ...);
C_TEST double va_doubles(int32_t count, ...);
C_TEST double va_mixed(const char *kinds, ...);
C_TEST double va_after_float(float first, double second, int32_t count, ...);

/* mistakes on purpose (the protections' tests) */
C_TEST void write_past_end(uint8_t *bytes, int32_t size, int32_t extra);
C_TEST int32_t read_null(const int32_t *p);

#ifdef __cplusplus
}
#endif
