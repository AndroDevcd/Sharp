/* See c_test_functions.h. Each function's result depends on every argument, each with its own weight,
   so an argument that arrives in the wrong place changes the result. */
#include "c_test_functions.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

int32_t add3(int32_t a, int32_t b, int32_t c) { return a + 10 * b + 100 * c; }

int64_t sum10(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, int64_t h, int64_t i, int64_t j) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j;
}

double mix12(int32_t a, double b, int32_t c, double d, int64_t e, float f, int32_t g, double h, int8_t i, float j, int64_t k, double l) {
    return a + 2 * b + 3 * c + 4 * d + 5 * (double) e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j + 11 * (double) k + 12 * l;
}

double fsum10(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j;
}

double dsum10(double a, double b, double c, double d, double e, double f, double g, double h, double i, double j) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j;
}

int64_t narrow(int8_t a, uint8_t b, int16_t c, uint16_t d, int32_t e, uint32_t f) {
    return (int64_t) a + 3 * (int64_t) b + 5 * (int64_t) c + 7 * (int64_t) d + 11 * (int64_t) e + 13 * (int64_t) f;
}

double i64_align(int32_t a, int64_t b, int32_t c, int64_t d, int32_t e, int64_t f) {
    return a + 2 * (double) b + 3 * c + 4 * (double) d + 5 * e + 6 * (double) f;
}

double fp_backfill(float a, double b, float c, double d, float e, double f, float g) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g;
}

double twenty(int32_t a, double b, int32_t c, double d, int32_t e, double f, int32_t g, double h, int32_t i, double j,
              int32_t k, double l, int32_t m, double n, int32_t o, double p, int32_t q, double r, int32_t s, double t) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j
         + 11 * k + 12 * l + 13 * m + 14 * n + 15 * o + 16 * p + 17 * q + 18 * r + 19 * s + 20 * t;
}

int8_t ret_i8(int32_t x) { return (int8_t) x; }
uint8_t ret_u8(int32_t x) { return (uint8_t) x; }
int16_t ret_i16(int32_t x) { return (int16_t) x; }
uint16_t ret_u16(int32_t x) { return (uint16_t) x; }
int32_t ret_i32(int64_t x) { return (int32_t) x; }
uint32_t ret_u32(int64_t x) { return (uint32_t) x; }
int64_t ret_i64(int64_t x) { return x * 3; }
float ret_f32(float x) { return x * 1.5f; }
double ret_f64(double x) { return x * 1.5; }
void *ret_ptr(void *p, int32_t add) { return (char*) p + add; }
void no_result(int32_t *into, int32_t value) { *into = value * 2; }

int64_t text_length(const char *text) { return text == NULL ? -1 : (int64_t) strlen(text); }

int64_t sum_array(const int32_t *values, int32_t count) {
    int64_t sum = 0;
    for(int32_t i = 0; i < count; i++) sum += values[i];
    return sum;
}

void fill_rect(rect16 *into, int32_t base) { into->x = base; into->y = base + 1; into->w = base + 2; into->h = base + 3; }
double read_rect(const rect16 *from) { return from->x + 2 * from->y + 3 * from->w + 4 * from->h; }

double take_rect16(rect16 v) { return v.x + 2 * v.y + 3 * v.w + 4 * v.h; }
double take_vec2(vec2 v) { return v.x + 2 * v.y; }
double take_color4(color4 v) { return v.r + 2 * v.g + 3 * v.b + 4 * v.a; }
double take_dvec2(dvec2 v) { return v.x + 2 * v.y; }
double take_mixed_id(mixed_id v) { return (double) v.a + 2 * v.b; }
double take_mixed_di(mixed_di v) { return v.a + 2 * v.b; }
double take_big24(big24 v) { return (double) v.a + 2 * (double) v.b + 3 * (double) v.c; }
double take_vec3(vec3 v) { return v.x + 2 * v.y + 3 * v.z; }
double take_vec4(vec4 v) { return v.x + 2 * v.y + 3 * v.z + 4 * v.w; }
double take_dvec4(dvec4 v) { return v.a + 2 * v.b + 3 * v.c + 4 * v.d; }
double take_pad16(pad16 v) { return v.a + 2 * (double) v.b; }
double take_small4(small4 v) { return v.a + 2 * v.b; }
double take_tiny3(tiny3 v) { return v.a + 2 * v.b + 3 * v.c; }
double take_odd8(odd8 v) { return v.a + 2 * v.b; }
double take_record32(record32 v) { return (double) strlen(v.name) + v.name[0] + 2 * v.n + 3 * v.v; }
double take_nested12(nested12 v) { return v.at.x + 2 * v.at.y + 3 * v.tint.r + 4 * v.tint.g + 5 * v.tint.b + 6 * v.tint.a; }
double take_one8(one8 v) { return (double) v.a; }
double take_oned(oned v) { return v.d; }
double take_onef(onef v) { return v.f; }
double take_if8(if8 v) { return v.a + 2 * v.b; }
double take_fid16(fid16 v) { return v.a + 2 * v.b + 3 * v.c; }
double take_dff16(dff16 v) { return v.a + 2 * v.b + 3 * v.c; }
double take_block160(block160 v) {
    double sum = 0;
    for(int i = 0; i < 40; i++) sum += (i + 1) * (double) v.values[i];
    return sum;
}

double after_many(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, rect16 r, vec2 v, double x, big24 g, color4 col, float f) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * take_rect16(r) + 7 * take_vec2(v) + 8 * x + 9 * take_big24(g) + 10 * take_color4(col) + 11 * f;
}

double many_vectors(vec2 a, vec2 b, vec2 c, vec2 d, vec2 e, dvec2 f, dvec4 g, vec4 h, vec3 i) {
    return take_vec2(a) + 2 * take_vec2(b) + 3 * take_vec2(c) + 4 * take_vec2(d) + 5 * take_vec2(e)
         + 6 * take_dvec2(f) + 7 * take_dvec4(g) + 8 * take_vec4(h) + 9 * take_vec3(i);
}

double ints_then_struct(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, pad16 p, small4 s, mixed_id m) {
    return (double) (a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g) + 8 * take_pad16(p) + 9 * take_small4(s) + 10 * take_mixed_id(m);
}

double split_struct(int32_t a, rect16 r, int32_t b, big24 g, int32_t c) {
    return a + 2 * take_rect16(r) + 3 * b + 4 * take_big24(g) + 5 * c;
}

/* A struct handed over by value is the function's own: changing it must not reach the caller. */
double changes_its_copy(big24 g, block160 b) {
    double before = take_big24(g) + take_block160(b);
    g.a = -1; g.b = -1; g.c = -1;
    memset(&b, 0x55, sizeof(b));
    return before;
}

rect16 make_rect16(int32_t s) { rect16 v = { s, s + 1, s + 2, s + 3 }; return v; }
vec2 make_vec2(int32_t s) { vec2 v = { s + 0.5f, s + 1.25f }; return v; }
color4 make_color4(int32_t s) { color4 v = { (uint8_t) s, (uint8_t) (s + 1), (uint8_t) (s + 2), (uint8_t) (s + 3) }; return v; }
dvec2 make_dvec2(int32_t s) { dvec2 v = { s + 0.5, s + 1.25 }; return v; }
mixed_id make_mixed_id(int32_t s) { mixed_id v = { (int64_t) s * 1000000007LL, s + 0.75 }; return v; }
mixed_di make_mixed_di(int32_t s) { mixed_di v = { s + 0.75, s * 3 }; return v; }
big24 make_big24(int32_t s) { big24 v = { s, (int64_t) s * 4000000000LL, -(int64_t) s }; return v; }
vec3 make_vec3(int32_t s) { vec3 v = { s + 0.5f, s + 1.5f, s + 2.5f }; return v; }
vec4 make_vec4(int32_t s) { vec4 v = { s + 0.5f, s + 1.5f, s + 2.5f, s + 3.5f }; return v; }
dvec4 make_dvec4(int32_t s) { dvec4 v = { s + 0.5, s + 1.5, s + 2.5, s + 3.5 }; return v; }
pad16 make_pad16(int32_t s) { pad16 v; memset(&v, 0, sizeof(v)); v.a = (int8_t) -s; v.b = (int64_t) s * 5000000000LL; return v; }
small4 make_small4(int32_t s) { small4 v; memset(&v, 0, sizeof(v)); v.a = (int16_t) (s * 100); v.b = (int8_t) -s; return v; }
tiny3 make_tiny3(int32_t s) { tiny3 v = { (int8_t) s, (int8_t) -s, (int8_t) (s + 1) }; return v; }
odd8 make_odd8(int32_t s) { odd8 v; memset(&v, 0, sizeof(v)); v.a = s * 100000; v.b = (int8_t) -s; return v; }
record32 make_record32(int32_t s) { record32 v; memset(&v, 0, sizeof(v)); strcpy(v.name, "record"); v.name[0] = (char) ('a' + s % 26); v.n = s; v.v = s + 0.125; return v; }
nested12 make_nested12(int32_t s) { nested12 v; v.at = make_vec2(s); v.tint = make_color4(s + 7); return v; }
one8 make_one8(int32_t s) { one8 v = { (int64_t) s * 6000000000LL }; return v; }
oned make_oned(int32_t s) { oned v = { s + 0.375 }; return v; }
onef make_onef(int32_t s) { onef v = { s + 0.375f }; return v; }
if8 make_if8(int32_t s) { if8 v = { s * 7, s + 0.5f }; return v; }
fid16 make_fid16(int32_t s) { fid16 v = { s + 0.5f, s * 9, s + 0.25 }; return v; }
dff16 make_dff16(int32_t s) { dff16 v = { s + 0.25, s + 0.5f, s + 0.75f }; return v; }
block160 make_block160(int32_t s) { block160 v; for(int i = 0; i < 40; i++) v.values[i] = s * (i + 1); return v; }

big24 scale_big24(big24 v, int32_t by, double add) { big24 r = { v.a * by, v.b * by + (int64_t) add, v.c - by }; return r; }
vec2 add_vec2(vec2 a, vec2 b) { vec2 r = { a.x + b.x, a.y + b.y }; return r; }

int32_t call_int(int_callback f, int32_t a, int32_t b) { return f(a, b) + 1; }
double call_mixed(mixed_callback f, void *data) { return f(7, 2.5, 1.25f, 9000000000LL, data) * 2; }
void sort_ints(int32_t *values, int32_t count, compare_callback compare) { qsort(values, (size_t) count, sizeof(int32_t), compare); }

static int_callback kept = NULL;
void keep_callback(int_callback f) { kept = f; }
int32_t call_kept(int32_t a, int32_t b) { return kept == NULL ? -1 : kept(a, b); }

struct other_call { int_callback f; int32_t a, b, result; };
#ifdef _WIN32
static DWORD WINAPI on_other(LPVOID given) { struct other_call *c = (struct other_call*) given; c->result = c->f(c->a, c->b); return 0; }
int32_t call_on_other_thread(int_callback f, int32_t a, int32_t b) {
    struct other_call c = { f, a, b, 0 };
    HANDLE thread = CreateThread(NULL, 0, on_other, &c, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return c.result;
}
#else
static void *on_other(void *given) { struct other_call *c = (struct other_call*) given; c->result = c->f(c->a, c->b); return NULL; }
int32_t call_on_other_thread(int_callback f, int32_t a, int32_t b) {
    struct other_call c = { f, a, b, 0 };
    pthread_t thread;
    pthread_create(&thread, NULL, on_other, &c);
    pthread_join(thread, NULL);
    return c.result;
}
#endif

void write_past_end(uint8_t *bytes, int32_t size, int32_t extra) { memset(bytes, 0x7f, (size_t) (size + extra)); }
int32_t read_null(const int32_t *p) { return *p; }

/* for the Sharp side's struct tests */
double changes_its_copy_of(big24 g) { double before = take_big24(g); g.a = -1; g.b = -1; g.c = -1; return before; }
double read_rect_or(const rect16 *from, double otherwise) { return from == NULL ? otherwise : read_rect(from); }
rect16 *same_rect(rect16 *r) { return r; }
