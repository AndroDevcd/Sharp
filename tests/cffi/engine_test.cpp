//
// Tests lib/runtime/cffi/c_call.cpp on its own (no VM): every function of c_test_functions.c is
// called directly and through c_invoke, and the two results must be equal.
//
//      g++ -std=c++17 -O2 engine_test.cpp ../../lib/runtime/cffi/c_call.cpp c_test_functions.o -o engine_test
//
// (c_test_functions.c is compiled as C into its own object first.) The last line it prints is
// ENGINE: PASS (n checks) or ENGINE: FAIL.
//
#include "../../lib/runtime/cffi/c_call.h"
#include "c_test_functions.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <initializer_list>

static int checks = 0, failures = 0;

static c_value I(int64_t v) { c_value c; c.i = v; return c; }
static c_value D(double v) { c_value c; c.d = v; return c; }
static c_value P(const void *v) { c_value c; c.p = (void*) v; return c; }

static void failed(const char *name, const char *what) {
    failures++;
    printf("FAIL  %s: %s\n", name, what);
}

static bool prepared(const char *name, const char *text, c_signature &s) {
    std::string why;
    if(c_parse_signature(text, s, why)) return true;
    failed(name, ("the signature " + std::string(text) + " was refused: " + why).c_str());
    return false;
}

static void same_d(const char *name, double got, double want) {
    checks++;
    if(got == want || (std::isnan(got) && std::isnan(want))) return;
    char text[160];
    snprintf(text, sizeof(text), "got %.17g, the direct call gives %.17g", got, want);
    failed(name, text);
}

static void same_i(const char *name, int64_t got, int64_t want) {
    checks++;
    if(got == want) return;
    char text[160];
    snprintf(text, sizeof(text), "got %lld, the direct call gives %lld", (long long) got, (long long) want);
    failed(name, text);
}

static double call_d(const char *name, const char *text, void *fn, std::initializer_list<c_value> args) {
    c_signature s;
    if(!prepared(name, text, s)) return NAN;
    c_value result = D(NAN);
    c_invoke(s, fn, args.begin(), &result, nullptr);
    return result.d;
}

static int64_t call_i(const char *name, const char *text, void *fn, std::initializer_list<c_value> args) {
    c_signature s;
    if(!prepared(name, text, s)) return -987654321;
    c_value result = I(-987654321);
    c_invoke(s, fn, args.begin(), &result, nullptr);
    return result.i;
}

static void call_s(const char *name, const char *text, void *fn, std::initializer_list<c_value> args, void *into) {
    c_signature s;
    if(!prepared(name, text, s)) return;
    c_invoke(s, fn, args.begin(), nullptr, into);
}

#define FN(f) ((void*) (f))

// the text of each struct of c_test_functions.h
#define T_rect16 "{i32 i32 i32 i32}"
#define T_vec2 "{f32 f32}"
#define T_color4 "{u8 u8 u8 u8}"
#define T_dvec2 "{f64 f64}"
#define T_mixed_id "{i64 f64}"
#define T_mixed_di "{f64 i32}"
#define T_big24 "{i64 i64 i64}"
#define T_vec3 "{f32 f32 f32}"
#define T_vec4 "{f32 f32 f32 f32}"
#define T_dvec4 "{f64 f64 f64 f64}"
#define T_pad16 "{i8 i64}"
#define T_small4 "{i16 i8}"
#define T_tiny3 "{i8 i8 i8}"
#define T_odd8 "{i32 i8}"
#define T_record32 "{i8[20] i32 f64}"
#define T_nested12 "{{f32 f32} {u8 u8 u8 u8}}"
#define T_one8 "{i64}"
#define T_oned "{f64}"
#define T_onef "{f32}"
#define T_if8 "{i32 f32}"
#define T_fid16 "{f32 i32 f64}"
#define T_dff16 "{f64 f32 f32}"
#define T_block160 "{i32[40]}"

// one struct type: its size, handing it over, getting it back
#define STRUCT(T) { \
    c_type type; std::string why; \
    checks++; \
    if(!c_parse_type(T_##T, type, why)) failed(#T, why.c_str()); \
    else if(type.size != sizeof(T) || type.align != alignof(T)) { \
        char text[160]; snprintf(text, sizeof(text), "laid out as %u bytes aligned to %u; C has %u aligned to %u", type.size, type.align, (unsigned) sizeof(T), (unsigned) alignof(T)); \
        failed("layout of " #T, text); \
    } \
    T value = make_##T(5); \
    same_d("take_" #T, call_d("take_" #T, "(" T_##T ")f64", FN(take_##T), { P(&value) }), take_##T(value)); \
    T got; memset(&got, 0xEE, sizeof(got)); \
    call_s("make_" #T, "(i32)" T_##T, FN(make_##T), { I(9) }, &got); \
    same_d("make_" #T, take_##T(got), take_##T(make_##T(9))); \
}

int main() {
    printf("calling C, engine test: %s\n", c_convention());

    // ---- numbers
    same_i("add3", call_i("add3", "(i32,i32,i32)i32", FN(add3), { I(1), I(2), I(3) }), add3(1, 2, 3));
    same_i("add3 negative", call_i("add3", "(i32,i32,i32)i32", FN(add3), { I(-1), I(-2), I(-3) }), add3(-1, -2, -3));
    same_i("sum10", call_i("sum10", "(i64,i64,i64,i64,i64,i64,i64,i64,i64,i64)i64", FN(sum10),
               { I(1), I(-2), I(3000000000LL), I(4), I(-5000000000LL), I(6), I(7), I(-8), I(9), I(10000000000LL) }),
           sum10(1, -2, 3000000000LL, 4, -5000000000LL, 6, 7, -8, 9, 10000000000LL));
    same_d("mix12", call_d("mix12", "(i32,f64,i32,f64,i64,f32,i32,f64,i8,f32,i64,f64)f64", FN(mix12),
               { I(1), D(2.5), I(-3), D(4.25), I(5000000000LL), D(6.5), I(7), D(-8.125), I(-9), D(10.75), I(-11), D(12.0625) }),
           mix12(1, 2.5, -3, 4.25, 5000000000LL, 6.5f, 7, -8.125, -9, 10.75f, -11, 12.0625));
    same_d("fsum10", call_d("fsum10", "(f32,f32,f32,f32,f32,f32,f32,f32,f32,f32)f64", FN(fsum10),
               { D(1.5), D(2.5), D(3.5), D(4.5), D(5.5), D(6.5), D(7.5), D(8.5), D(9.5), D(10.5) }),
           fsum10(1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f));
    same_d("dsum10", call_d("dsum10", "(f64,f64,f64,f64,f64,f64,f64,f64,f64,f64)f64", FN(dsum10),
               { D(1.5), D(2.5), D(3.5), D(4.5), D(5.5), D(6.5), D(7.5), D(8.5), D(9.5), D(10.5) }),
           dsum10(1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5));
    same_i("narrow", call_i("narrow", "(i8,u8,i16,u16,i32,u32)i64", FN(narrow), { I(-5), I(250), I(-30000), I(65000), I(-2000000000), I(4000000000LL) }),
           narrow(-5, 250, -30000, 65000, -2000000000, 4000000000u));
    // (numbers too wide for the parameter: C sees only the low bits, as it does when a C caller casts)
    same_i("narrow, cut", call_i("narrow", "(i8,u8,i16,u16,i32,u32)i64", FN(narrow), { I(0x1FB), I(0x1FA), I(0x18AD0), I(0x1FDE8), I(0x100000001LL), I(0x1EE6B2800LL) }),
           narrow((int8_t) 0x1FB, (uint8_t) 0x1FA, (int16_t) 0x18AD0, (uint16_t) 0x1FDE8, (int32_t) 0x100000001LL, (uint32_t) 0x1EE6B2800LL));
    same_d("i64_align", call_d("i64_align", "(i32,i64,i32,i64,i32,i64)f64", FN(i64_align), { I(1), I(20000000000LL), I(3), I(-40000000000LL), I(5), I(60000000000LL) }),
           i64_align(1, 20000000000LL, 3, -40000000000LL, 5, 60000000000LL));
    same_d("fp_backfill", call_d("fp_backfill", "(f32,f64,f32,f64,f32,f64,f32)f64", FN(fp_backfill), { D(1.5), D(2.25), D(3.5), D(4.25), D(5.5), D(6.25), D(7.5) }),
           fp_backfill(1.5f, 2.25, 3.5f, 4.25, 5.5f, 6.25, 7.5f));
    same_d("twenty", call_d("twenty", "(i32,f64,i32,f64,i32,f64,i32,f64,i32,f64,i32,f64,i32,f64,i32,f64,i32,f64,i32,f64)f64", FN(twenty),
               { I(1), D(2.5), I(3), D(4.5), I(5), D(6.5), I(7), D(8.5), I(9), D(10.5), I(11), D(12.5), I(13), D(14.5), I(15), D(16.5), I(17), D(18.5), I(19), D(20.5) }),
           twenty(1, 2.5, 3, 4.5, 5, 6.5, 7, 8.5, 9, 10.5, 11, 12.5, 13, 14.5, 15, 16.5, 17, 18.5, 19, 20.5));

    // ---- every width of result
    same_i("ret_i8", call_i("ret_i8", "(i32)i8", FN(ret_i8), { I(0x1F0) }), ret_i8(0x1F0));
    same_i("ret_u8", call_i("ret_u8", "(i32)u8", FN(ret_u8), { I(0x1F0) }), ret_u8(0x1F0));
    same_i("ret_i16", call_i("ret_i16", "(i32)i16", FN(ret_i16), { I(0x1F000) }), ret_i16(0x1F000));
    same_i("ret_u16", call_i("ret_u16", "(i32)u16", FN(ret_u16), { I(0x1F000) }), ret_u16(0x1F000));
    same_i("ret_i32", call_i("ret_i32", "(i64)i32", FN(ret_i32), { I(0x1F0000000LL) }), ret_i32(0x1F0000000LL));
    same_i("ret_u32", call_i("ret_u32", "(i64)u32", FN(ret_u32), { I(0x1F0000000LL) }), ret_u32(0x1F0000000LL));
    same_i("ret_i64", call_i("ret_i64", "(i64)i64", FN(ret_i64), { I(-7000000000LL) }), ret_i64(-7000000000LL));
    same_d("ret_f32", call_d("ret_f32", "(f32)f32", FN(ret_f32), { D(2.75) }), ret_f32(2.75f));
    same_d("ret_f64", call_d("ret_f64", "(f64)f64", FN(ret_f64), { D(2.75) }), ret_f64(2.75));
    {
        char block[64];
        c_signature s;
        c_value result = P(nullptr);
        if(prepared("ret_ptr", "(ptr,i32)ptr", s)) {
            c_value args[2] = { P(block), I(17) };
            c_invoke(s, FN(ret_ptr), args, &result, nullptr);
        }
        checks++;
        if(result.p != block + 17) failed("ret_ptr", "the pointer that came back is not the one handed over plus 17");
        int32_t into = 0;
        call_i("no_result", "(ptr,i32)void", FN(no_result), { P(&into), I(21) });
        same_i("no_result", into, 42);
    }

    // ---- pointers
    same_i("text_length", call_i("text_length", "(cstr)i64", FN(text_length), { P("hello, C") }), 8);
    same_i("text_length null", call_i("text_length", "(cstr)i64", FN(text_length), { P(nullptr) }), -1);
    {
        int32_t values[5] = { 10, -20, 30, 40, 50 };
        same_i("sum_array", call_i("sum_array", "(buf,i32)i64", FN(sum_array), { P(values), I(5) }), 110);
        rect16 r;
        call_i("fill_rect", "(ptr,i32)void", FN(fill_rect), { P(&r), I(7) });
        same_d("fill_rect / read_rect", call_d("read_rect", "(ptr)f64", FN(read_rect), { P(&r) }), read_rect(&r));
        same_i("fill_rect wrote", r.h, 10);
    }

    // ---- C's own widths
    {
        c_type type; std::string why;
        checks += 3;
        if(!c_parse_type("long", type, why) || type.size != sizeof(long)) failed("long", "is not as wide as C's long here");
        if(!c_parse_type("size", type, why) || type.size != sizeof(size_t)) failed("size", "is not as wide as C's size_t here");
        if(!c_parse_type("ptr", type, why) || type.size != sizeof(void*)) failed("ptr", "is not as wide as a pointer here");
    }

    // ---- structs: layout, in, out
    STRUCT(rect16) STRUCT(vec2) STRUCT(color4) STRUCT(dvec2) STRUCT(mixed_id) STRUCT(mixed_di) STRUCT(big24)
    STRUCT(vec3) STRUCT(vec4) STRUCT(dvec4) STRUCT(pad16) STRUCT(small4) STRUCT(tiny3) STRUCT(odd8)
    STRUCT(record32) STRUCT(nested12) STRUCT(one8) STRUCT(oned) STRUCT(onef) STRUCT(if8) STRUCT(fid16) STRUCT(dff16)
    STRUCT(block160)

    // ---- structs among other arguments
    {
        rect16 r = make_rect16(3); vec2 v = make_vec2(4); big24 g = make_big24(5); color4 col = make_color4(6);
        same_d("after_many", call_d("after_many", "(i32,i32,i32,i32,i32," T_rect16 "," T_vec2 ",f64," T_big24 "," T_color4 ",f32)f64", FN(after_many),
                   { I(1), I(2), I(3), I(4), I(5), P(&r), P(&v), D(8.5), P(&g), P(&col), D(11.25) }),
               after_many(1, 2, 3, 4, 5, r, v, 8.5, g, col, 11.25f));

        vec2 a = make_vec2(1), b = make_vec2(2), c = make_vec2(3), d = make_vec2(4), e = make_vec2(5);
        dvec2 f = make_dvec2(6); dvec4 g4 = make_dvec4(7); vec4 h = make_vec4(8); vec3 i3 = make_vec3(9);
        same_d("many_vectors", call_d("many_vectors", "(" T_vec2 "," T_vec2 "," T_vec2 "," T_vec2 "," T_vec2 "," T_dvec2 "," T_dvec4 "," T_vec4 "," T_vec3 ")f64", FN(many_vectors),
                   { P(&a), P(&b), P(&c), P(&d), P(&e), P(&f), P(&g4), P(&h), P(&i3) }),
               many_vectors(a, b, c, d, e, f, g4, h, i3));

        pad16 p = make_pad16(3); small4 s4 = make_small4(4); mixed_id m = make_mixed_id(5);
        same_d("ints_then_struct", call_d("ints_then_struct", "(i64,i64,i64,i64,i64,i64,i64," T_pad16 "," T_small4 "," T_mixed_id ")f64", FN(ints_then_struct),
                   { I(1), I(2), I(3), I(4), I(5), I(6), I(7), P(&p), P(&s4), P(&m) }),
               ints_then_struct(1, 2, 3, 4, 5, 6, 7, p, s4, m));

        same_d("split_struct", call_d("split_struct", "(i32," T_rect16 ",i32," T_big24 ",i32)f64", FN(split_struct), { I(1), P(&r), I(3), P(&g), I(5) }),
               split_struct(1, r, 3, g, 5));

        // a struct handed over by value is C's own copy: what C does to it does not reach ours
        block160 block = make_block160(2), block_before = block;
        big24 g_before = g;
        same_d("changes_its_copy", call_d("changes_its_copy", "(" T_big24 "," T_block160 ")f64", FN(changes_its_copy), { P(&g), P(&block) }),
               take_big24(g_before) + take_block160(block_before));
        checks++;
        if(memcmp(&g, &g_before, sizeof(g)) != 0 || memcmp(&block, &block_before, sizeof(block)) != 0)
            failed("changes_its_copy", "the caller's struct was changed by a function that took it by value");

        // a struct out, with arguments beside the hidden pointer
        big24 scaled;
        call_s("scale_big24", "(" T_big24 ",i32,f64)" T_big24, FN(scale_big24), { P(&g), I(3), D(1000.5) }, &scaled);
        same_d("scale_big24", take_big24(scaled), take_big24(scale_big24(g, 3, 1000.5)));
        vec2 sum;
        call_s("add_vec2", "(" T_vec2 "," T_vec2 ")" T_vec2, FN(add_vec2), { P(&a), P(&b) }, &sum);
        same_d("add_vec2", take_vec2(sum), take_vec2(add_vec2(a, b)));
    }

    // ---- members with names, and a pointer to a struct
    {
        rect16 r = make_rect16(4);
        same_d("named members", call_d("take_rect16", "({x:i32 y:i32 w:i32 h:i32})f64", FN(take_rect16), { P(&r) }), take_rect16(r));
        same_d("pointer to a struct", call_d("read_rect", "(*{x:i32 y:i32 w:i32 h:i32})f64", FN(read_rect), { P(&r) }), read_rect(&r));
        nested12 n = make_nested12(3);
        same_d("named nested", call_d("take_nested12", "({at:{x:f32 y:f32} tint:{r:u8 g:u8 b:u8 a:u8}})f64", FN(take_nested12), { P(&n) }), take_nested12(n));
    }

    // ---- what is refused, and that it says why
    {
        const char *wrong[] = { "", "i32", "(i32", "(i33)void", "(void,i32)void", "(i32)", "({})void", "(i32)void extra", "({i32)void", "(i8[4])void", "({i8[0]})void", "(*i32)void", "({x:})void" };
        for(const char *text : wrong) {
            c_signature s; std::string why;
            checks++;
            if(c_parse_signature(text, s, why)) failed("refused", (std::string("`") + text + "` was accepted").c_str());
            else if(why.empty()) failed("refused", (std::string("`") + text + "` was refused without a reason").c_str());
        }
    }

    if(failures == 0) printf("ENGINE: PASS (%d checks)\n", checks);
    else printf("ENGINE: FAIL (%d of %d checks failed)\n", failures, checks);
    return failures == 0 ? 0 : 1;
}
