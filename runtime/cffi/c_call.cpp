//
// CALLING C (see c_call.h for what this is and how it is used)
//
// THE PARTS OF THIS FILE
//      1. types from their text, and how a struct is laid out
//      2. THE PLAN, once per convention: where each argument travels
//             Windows x64     four places, each a whole-number OR a floating point register; the rest on the stack
//             Linux x64       6 whole-number + 8 floating point registers; small structs are cut into 8-byte parts
//             ARM64           8 + 8 registers; a struct of 1-4 equal floats gets a register for each
//             ARM32           4 + 8 (16 single) registers; a struct may be half in registers, half on the stack
//      3. THE CALL: the registers and the stack are filled from the plan, then one of a handful of
//         C++ function types is called that carries every register
//
// Everything here is little-endian (all four systems are): the low bytes of a number come first,
// so "the first 4 bytes of an 8-byte number" is its low half.
//
#include "c_call.h"
#include <cstring>

#if defined(_WIN64)
#define C_ABI_WIN64 1
#elif defined(__x86_64__)
#define C_ABI_SYSV64 1
#elif defined(__aarch64__)
#define C_ABI_ARM64 1
#elif defined(__arm__) && defined(__ARM_PCS_VFP)
#define C_ABI_ARM32 1
#else
#define C_ABI_NONE 1
#endif

/** the most bytes of arguments that may travel on the stack in one call */
static const uint32_t MAX_STACK_WORDS = 128;
static const uint32_t WORD = (uint32_t) sizeof(void*);
static const uint32_t MAX_STACK = MAX_STACK_WORDS * (uint32_t) sizeof(void*);
/** the largest struct handed over by value */
static const uint32_t MAX_STRUCT = 4096;

const char *c_convention() {
#if defined(C_ABI_WIN64)
    return "Windows x64";
#elif defined(C_ABI_SYSV64)
    return "System V x64";
#elif defined(C_ABI_ARM64)
    return "ARM64";
#elif defined(C_ABI_ARM32)
    return "ARM32 (hard float)";
#else
    return "";
#endif
}

// ---------------------------------------------------------------------------------
// 1. Types
// ---------------------------------------------------------------------------------

uint32_t c_size_of(c_kind kind) {
    switch(kind) {
        case C_I8: case C_U8: return 1;
        case C_I16: case C_U16: return 2;
        case C_I32: case C_U32: case C_F32: return 4;
        case C_I64: case C_U64: case C_F64: return 8;
        case C_PTR: return (uint32_t) sizeof(void*);
        default: return 0;
    }
}

static bool is_float(c_kind kind) { return kind == C_F32 || kind == C_F64; }

static uint32_t round_up(uint32_t value, uint32_t to) { return (value + to - 1) / to * to; }

static void skip_blanks(const char *&p) { while(*p == ' ' || *p == '\t') p++; }

static bool kind_named(const std::string &name, c_kind &kind) {
    static const struct { const char *name; c_kind kind; } names[] = {
        { "void", C_VOID }, { "i8", C_I8 }, { "u8", C_U8 }, { "i16", C_I16 }, { "u16", C_U16 },
        { "i32", C_I32 }, { "u32", C_U32 }, { "i64", C_I64 }, { "u64", C_U64 }, { "f32", C_F32 }, { "f64", C_F64 },
        { "ptr", C_PTR }, { "cstr", C_PTR }, { "buf", C_PTR }, { "fn", C_PTR },
    };
    for(const auto &known : names)
        if(name == known.name) { kind = known.kind; return true; }
    // C's own widths: long is 32 bits on Windows and on a 32-bit system, 64 on 64-bit Linux
    bool long_is_64 = sizeof(long) == 8;
    if(name == "long") { kind = long_is_64 ? C_I64 : C_I32; return true; }
    if(name == "ulong") { kind = long_is_64 ? C_U64 : C_U32; return true; }
    if(name == "size") { kind = sizeof(size_t) == 8 ? C_U64 : C_U32; return true; }
    if(name == "ssize") { kind = sizeof(size_t) == 8 ? C_I64 : C_I32; return true; }
    return false;
}

static bool parse_one(const char *&p, c_type &type, std::string &why, int depth) {
    skip_blanks(p);
    const char *start = p;
    type = c_type();
    if(*p == '*') {
        // a pointer to a struct: a pointer to C; what it points to is c_bind.cpp's business
        p++;
        c_type pointed;
        if(!parse_one(p, pointed, why, depth + 1)) return false;
        if(pointed.kind != C_STRUCT) { why = "`*` stands in front of a struct: *{i32 i32}"; return false; }
        type.kind = C_PTR;
        type.size = c_size_of(C_PTR);
        type.align = type.size;
        type.text = std::string(start, p);
        return true;
    }
    if(*p == '{') {
        if(depth > 8) { why = "structs inside structs, more than 8 deep"; return false; }
        p++;
        type.kind = C_STRUCT;
        while(true) {
            skip_blanks(p);
            if(*p == ',') { p++; continue; }
            if(*p == '}') { p++; break; }
            if(*p == 0) { why = "a `{` is never closed"; return false; }
            // {@module#class x:i32}: the Sharp class the struct is (c_bind.cpp makes objects of it)
            if(*p == '@') {
                while(*p != 0 && *p != ' ' && *p != '}') p++;
                continue;
            }
            // a member may carry its name:  x:i32   (the names are for c_bind.cpp; the layout does not need them)
            {
                const char *ahead = p;
                while((*ahead >= 'a' && *ahead <= 'z') || (*ahead >= 'A' && *ahead <= 'Z') || (*ahead >= '0' && *ahead <= '9') || *ahead == '_') ahead++;
                if(ahead != p && *ahead == ':') p = ahead + 1;
            }
            c_type member;
            if(!parse_one(p, member, why, depth + 1)) return false;
            if(member.kind == C_VOID) { why = "`void` cannot be a member of a struct"; return false; }
            uint32_t count = 1;
            skip_blanks(p);
            if(*p == '[') {
                p++;
                count = 0;
                bool digits = false;
                while(*p >= '0' && *p <= '9') { count = count * 10 + (uint32_t) (*p - '0'); p++; digits = true; if(count > MAX_STRUCT) break; }
                if(!digits || *p != ']' || count == 0) { why = "an array member is written  type[count]  with a count of 1 or more"; return false; }
                p++;
            }
            for(uint32_t k = 0; k < count; k++) {
                uint32_t offset = round_up(type.size, member.align);
                if(offset + member.size > MAX_STRUCT) { why = "a struct of more than " + std::to_string(MAX_STRUCT) + " bytes cannot be handed over by value: hand over a pointer to it"; return false; }
                if(member.kind == C_STRUCT) {
                    for(const c_field &inner : member.fields) type.fields.push_back(c_field{ inner.kind, offset + inner.offset });
                } else {
                    type.fields.push_back(c_field{ member.kind, offset });
                }
                type.size = offset + member.size;
            }
            if(member.align > type.align) type.align = member.align;
        }
        if(type.fields.empty()) { why = "a struct with no members"; return false; }
        type.size = round_up(type.size, type.align);
        // {...}=16: how many bytes the program says C has for this struct. A difference means the fields
        // are not the C struct's members: one is missing, of another type, or in another place.
        if(*p == '=') {
            p++;
            uint32_t said = 0;
            while(*p >= '0' && *p <= '9') { said = said * 10 + (uint32_t) (*p - '0'); p++; }
            if(said != type.size) {
                why = "the struct " + std::string(start, p) + " is " + std::to_string(type.size) + " bytes the way its fields are laid out on this system ("
                      + c_convention() + "), and it is declared to be " + std::to_string(said)
                      + " (c_bytes): a field is missing, has another type than the C member, or stands in another place";
                return false;
            }
        }
    } else {
        std::string name;
        while((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_') name += *p++;
        if(name.empty()) { why = std::string("a type was expected at `") + start + "`"; return false; }
        if(name == "fn" && *p == '(') {
            // a function handed to C: fn(i32,i32)i32 - a pointer; what it is called with is c_callback.cpp's business
            int depth_in = 0;
            while(*p != 0) {
                if(*p == '(') depth_in++;
                if(*p == ')') { depth_in--; if(depth_in == 0) { p++; break; } }
                p++;
            }
            c_type returned;
            if(depth_in != 0 || !parse_one(p, returned, why, depth + 1)) { if(why.empty()) why = "the `(` of a function type is never closed"; return false; }
            type.kind = C_PTR;
            type.size = c_size_of(C_PTR);
            type.align = type.size;
            type.text = std::string(start, p);
            return true;
        }
        if(!kind_named(name, type.kind)) {
            why = "`" + name + "` is not a C type this knows (i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 ptr cstr buf fn long ulong size ssize void, or a struct in { })";
            return false;
        }
        type.size = c_size_of(type.kind);
        type.align = type.size == 0 ? 1 : type.size;
    }
    type.text = std::string(start, p);
    return true;
}

bool c_parse_type(const std::string &text, c_type &type, std::string &why) {
    const char *p = text.c_str();
    if(!parse_one(p, type, why, 0)) return false;
    skip_blanks(p);
    if(*p != 0) { why = std::string("something is left over after the type: `") + p + "`"; return false; }
    return true;
}

// ---------------------------------------------------------------------------------
// 2. The plan
// ---------------------------------------------------------------------------------

static void add_move(c_signature &s, size_t arg, int source, int place, uint32_t from, uint32_t to, uint32_t bytes) {
    s.moves.push_back(c_move{ (uint16_t) arg, (uint8_t) source, (uint8_t) place, (uint16_t) from, (uint16_t) to, (uint16_t) bytes });
}

static void add_piece(c_signature &s, int place, uint32_t from, uint32_t to, uint32_t bytes) {
    s.pieces.push_back(c_move{ 0, 0, (uint8_t) place, (uint16_t) from, (uint16_t) to, (uint16_t) bytes });
}

/** Room on the stack for `bytes` at a multiple of `align`: where it starts. false when the stack is full. */
static bool stack_room(c_signature &s, uint32_t bytes, uint32_t align, uint32_t &at, std::string &why) {
    at = round_up(s.stack_bytes, align);
    uint32_t end = at + round_up(bytes, WORD);
    if(end > MAX_STACK) {
        why = "the arguments need more than " + std::to_string(MAX_STACK) + " bytes of stack, the most one call may use: hand over large structs by pointer";
        return false;
    }
    s.stack_bytes = end;
    return true;
}

/** Room for a copy of a struct that travels as "the address of a copy": where in the copies it starts. */
static uint32_t copy_room(c_signature &s, const c_type &type) {
    uint32_t at = round_up(s.copy_bytes, 16);
    s.copy_bytes = at + type.size;
    return at;
}

/** 1 to 4 members, all `float` or all `double`, and nothing else: ARM hands such a struct over in floating point registers. */
static bool equal_floats(const c_type &type, uint32_t &count, uint32_t &member_size) {
    if(type.kind != C_STRUCT || type.fields.empty() || type.fields.size() > 4) return false;
    c_kind kind = type.fields[0].kind;
    if(!is_float(kind)) return false;
    for(const c_field &field : type.fields)
        if(field.kind != kind) return false;
    count = (uint32_t) type.fields.size();
    member_size = c_size_of(kind);
    return type.size == count * member_size;
}

#if defined(C_ABI_WIN64)

// Result shapes: 0 a whole number or pointer (rax), 1 a float or double (xmm0)
static bool plan(c_signature &s, std::string &why) {
    uint32_t slot = 0;
    const c_type &r = s.result;
    if(r.kind == C_STRUCT) {
        if(r.size == 1 || r.size == 2 || r.size == 4 || r.size == 8) {
            add_piece(s, C_IN_INT, 0, 0, r.size);
        } else {
            s.result_in_memory = true;                      // the caller says where: a pointer in the first place
            add_move(s, 0, C_FROM_RESULT_ADDRESS, C_IN_INT, 0, 0, 8);
            slot = 1;
        }
    } else if(is_float(r.kind)) {
        s.shape = 1;
        add_piece(s, C_IN_FP, 0, 0, r.size);
    } else if(r.kind != C_VOID) {
        add_piece(s, C_IN_INT, 0, 0, r.size);
    }

    for(size_t a = 0; a < s.args.size(); a++, slot++) {
        const c_type &t = s.args[a];
        int source = C_FROM_VALUE;
        uint32_t from = 0, bytes = 8;
        // One of the variable arguments of a function is read by it from the WHOLE-NUMBER register of
        // its place (or the stack), a float too: there it travels as the 8 bytes of a double.
        bool variable = s.variadic_from >= 0 && (int) a >= s.variadic_from;
        bool fp = is_float(t.kind) && !variable;
        if(t.kind == C_STRUCT) {
            if(t.size == 1 || t.size == 2 || t.size == 4 || t.size == 8) {
                bytes = t.size;                             // as the whole number its bytes make, floats in it or not
            } else {
                source = C_FROM_COPY_ADDRESS;
                from = copy_room(s, t);
            }
        } else if(fp) {
            bytes = t.size;
        }
        if(slot < 4) {
            if(fp) s.fp_slots |= (uint8_t) (1u << slot);
            add_move(s, a, source, fp ? C_IN_FP : C_IN_INT, from, slot * 8, bytes);
        } else {
            uint32_t at;
            if(!stack_room(s, 8, 8, at, why)) return false;
            add_move(s, a, source, C_IN_STACK, from, at, bytes);
        }
    }
    return true;
}

#elif defined(C_ABI_SYSV64)

// Result shapes: 0 rax:rdx, 1 xmm0:xmm1, 2 rax then xmm0, 3 xmm0 then rax

/** Is the 8-byte part `part` of this struct all floats and doubles (it then travels in a floating point register)? */
static bool part_is_float(const c_type &type, uint32_t part) {
    for(const c_field &field : type.fields)
        if(field.offset / 8 == part && !is_float(field.kind)) return false;
    return true;
}

static bool plan(c_signature &s, std::string &why) {
    uint32_t ints = 0, fps = 0;
    const c_type &r = s.result;
    if(r.kind == C_STRUCT) {
        if(r.size > 16) {
            s.result_in_memory = true;
            add_move(s, 0, C_FROM_RESULT_ADDRESS, C_IN_INT, 0, 0, 8);
            ints = 1;
        } else {
            bool first = part_is_float(r, 0), second = r.size > 8 && part_is_float(r, 1);
            bool two = r.size > 8;
            uint32_t rest = r.size > 8 ? r.size - 8 : 0;
            if(first && (!two || second)) {
                s.shape = 1;
                add_piece(s, C_IN_FP, 0, 0, two ? 8 : r.size);
                if(two) add_piece(s, C_IN_FP, 8, 8, rest);
            } else if(!first && (!two || !second)) {
                s.shape = 0;
                add_piece(s, C_IN_INT, 0, 0, two ? 8 : r.size);
                if(two) add_piece(s, C_IN_INT, 8, 8, rest);
            } else if(!first) {
                s.shape = 2;
                add_piece(s, C_IN_INT, 0, 0, 8);
                add_piece(s, C_IN_FP, 0, 8, rest);
            } else {
                s.shape = 3;
                add_piece(s, C_IN_FP, 0, 0, 8);
                add_piece(s, C_IN_INT, 0, 8, rest);
            }
        }
    } else if(is_float(r.kind)) {
        s.shape = 1;
        add_piece(s, C_IN_FP, 0, 0, r.size);
    } else if(r.kind != C_VOID) {
        add_piece(s, C_IN_INT, 0, 0, r.size);
    }

    for(size_t a = 0; a < s.args.size(); a++) {
        const c_type &t = s.args[a];
        uint32_t at;
        if(t.kind == C_STRUCT) {
            uint32_t parts = (t.size + 7) / 8, need_int = 0, need_fp = 0;
            if(t.size <= 16)
                for(uint32_t k = 0; k < parts; k++) (part_is_float(t, k) ? need_fp : need_int)++;
            if(t.size > 16 || ints + need_int > 6 || fps + need_fp > 8) {
                // on the stack, whole (a struct is never half in registers here)
                if(t.align > 8) { why = "a struct that is aligned to more than 8 bytes"; return false; }
                if(!stack_room(s, t.size, 8, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, t.size);
                continue;
            }
            for(uint32_t k = 0; k < parts; k++) {
                uint32_t bytes = t.size - k * 8 < 8 ? t.size - k * 8 : 8;
                if(part_is_float(t, k)) add_move(s, a, C_FROM_VALUE, C_IN_FP, k * 8, fps++ * 8, bytes);
                else add_move(s, a, C_FROM_VALUE, C_IN_INT, k * 8, ints++ * 8, bytes);
            }
        } else if(is_float(t.kind)) {
            if(fps < 8) add_move(s, a, C_FROM_VALUE, C_IN_FP, 0, fps++ * 8, t.size);
            else {
                if(!stack_room(s, 8, 8, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, t.size);
            }
        } else {
            if(ints < 6) add_move(s, a, C_FROM_VALUE, C_IN_INT, 0, ints++ * 8, 8);
            else {
                if(!stack_room(s, 8, 8, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, 8);
            }
        }
    }
    return true;
}

#elif defined(C_ABI_ARM64)

// Result shapes: 0 x0:x1, 1 the floating point registers v0-v3, 2 written where x8 says (a struct over 16 bytes)
static bool plan(c_signature &s, std::string &why) {
    uint32_t ints = 0, fps = 0, count = 0, member = 0, at = 0;
    const c_type &r = s.result;
    if(r.kind == C_STRUCT) {
        if(equal_floats(r, count, member)) {
            s.shape = 1;
            for(uint32_t k = 0; k < count; k++) add_piece(s, C_IN_FP, k * 8, k * member, member);
        } else if(r.size <= 16) {
            add_piece(s, C_IN_INT, 0, 0, r.size);
        } else {
            s.shape = 2;                                    // (x8 is not an argument register: the call itself hands it over)
            s.result_in_memory = true;
        }
    } else if(is_float(r.kind)) {
        s.shape = 1;
        add_piece(s, C_IN_FP, 0, 0, r.size);
    } else if(r.kind != C_VOID) {
        add_piece(s, C_IN_INT, 0, 0, r.size);
    }

    for(size_t a = 0; a < s.args.size(); a++) {
        const c_type &t = s.args[a];
        if(t.kind == C_STRUCT && equal_floats(t, count, member)) {
            if(fps + count <= 8) {
                for(uint32_t k = 0; k < count; k++) add_move(s, a, C_FROM_VALUE, C_IN_FP, k * member, fps++ * 8, member);
            } else {
                fps = 8;                                    // no floating point register is used after this
                if(!stack_room(s, t.size, 8, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, t.size);
            }
        } else if(t.kind == C_STRUCT && t.size <= 16) {
            uint32_t words = (t.size + 7) / 8;
            if(ints + words <= 8) {
                for(uint32_t k = 0; k < words; k++) {
                    uint32_t bytes = t.size - k * 8 < 8 ? t.size - k * 8 : 8;
                    add_move(s, a, C_FROM_VALUE, C_IN_INT, k * 8, ints++ * 8, bytes);
                }
            } else {
                ints = 8;
                if(!stack_room(s, t.size, 8, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, t.size);
            }
        } else if(t.kind == C_STRUCT) {
            uint32_t from = copy_room(s, t);                // over 16 bytes: the address of a copy
            if(ints < 8) add_move(s, a, C_FROM_COPY_ADDRESS, C_IN_INT, from, ints++ * 8, 8);
            else {
                if(!stack_room(s, 8, 8, at, why)) return false;
                add_move(s, a, C_FROM_COPY_ADDRESS, C_IN_STACK, from, at, 8);
            }
        } else if(is_float(t.kind)) {
            if(fps < 8) add_move(s, a, C_FROM_VALUE, C_IN_FP, 0, fps++ * 8, t.size);
            else {
                if(!stack_room(s, 8, 8, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, t.size);
            }
        } else {
            if(ints < 8) add_move(s, a, C_FROM_VALUE, C_IN_INT, 0, ints++ * 8, 8);
            else {
                if(!stack_room(s, 8, 8, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, 8);
            }
        }
    }
    return true;
}

#elif defined(C_ABI_ARM32)

// Result shapes: 0 r0:r1, 1 the floating point registers d0-d3 (s0-s3 for floats)
//
// The floating point registers are 16 singles (s0-s15); a double takes two of them (d0 is s0+s1).
// A float may fill a single that an earlier double left free. Once one floating point argument has
// gone to the stack, every later one does.

/** `count` free singles in a row, the first one at a multiple of `step` (2 for doubles): where, or -1. */
static int free_singles(bool *used, uint32_t count, uint32_t step) {
    for(uint32_t at = 0; at + count <= 16; at += step) {
        bool free = true;
        for(uint32_t k = 0; k < count; k++)
            if(used[at + k]) { free = false; break; }
        if(free) {
            for(uint32_t k = 0; k < count; k++) used[at + k] = true;
            return (int) at;
        }
    }
    return -1;
}

static bool plan(c_signature &s, std::string &why) {
    uint32_t ints = 0, count = 0, member = 0, at = 0;
    bool singles[16] = { false };
    bool fp_on_stack = false;
    // a function with variable arguments: every argument, and the result, as if there were no
    // floating point registers (a double is two words in the whole-number registers or on the stack)
    bool no_fp = s.variadic_from >= 0;
    const c_type &r = s.result;
    if(r.kind == C_STRUCT) {
        if(!no_fp && equal_floats(r, count, member)) {
            s.shape = 1;
            for(uint32_t k = 0; k < count; k++) add_piece(s, C_IN_FP, k * member, k * member, member);
        } else if(r.size <= 4) {
            add_piece(s, C_IN_INT, 0, 0, r.size);
        } else {
            s.result_in_memory = true;
            add_move(s, 0, C_FROM_RESULT_ADDRESS, C_IN_INT, 0, 0, 4);
            ints = 1;
        }
    } else if(is_float(r.kind) && !no_fp) {
        s.shape = 1;
        add_piece(s, C_IN_FP, 0, 0, r.size);
    } else if(r.kind != C_VOID) {
        add_piece(s, C_IN_INT, 0, 0, r.size);
    }

    for(size_t a = 0; a < s.args.size(); a++) {
        const c_type &t = s.args[a];
        bool floats = !no_fp && (is_float(t.kind) || (t.kind == C_STRUCT && equal_floats(t, count, member)));
        if(floats) {
            if(t.kind != C_STRUCT) { count = 1; member = t.size; }
            int found = fp_on_stack ? -1 : free_singles(singles, count * (member / 4), member / 4);
            if(found >= 0) {
                for(uint32_t k = 0; k < count; k++) add_move(s, a, C_FROM_VALUE, C_IN_FP, k * member, (uint32_t) found * 4 + k * member, member);
            } else {
                fp_on_stack = true;
                if(!stack_room(s, count * member, member, at, why)) return false;
                add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, count * member);
            }
            continue;
        }
        uint32_t size = t.kind == C_STRUCT ? t.size : (t.size <= 4 ? 4 : 8);
        uint32_t align = t.kind == C_STRUCT ? (t.align >= 8 ? 8 : 4) : size;
        uint32_t words = (size + 3) / 4;
        if(align == 8) ints = round_up(ints, 2);
        if(ints + words <= 4) {
            add_move(s, a, C_FROM_VALUE, C_IN_INT, 0, ints * 4, size);
            ints += words;
        } else if(t.kind == C_STRUCT && ints < 4 && s.stack_bytes == 0) {
            // half in the registers that are left, half on the stack
            uint32_t in_registers = (4 - ints) * 4;
            add_move(s, a, C_FROM_VALUE, C_IN_INT, 0, ints * 4, in_registers);
            ints = 4;
            if(!stack_room(s, size - in_registers, 4, at, why)) return false;
            add_move(s, a, C_FROM_VALUE, C_IN_STACK, in_registers, at, size - in_registers);
        } else {
            ints = 4;
            if(!stack_room(s, size, align, at, why)) return false;
            add_move(s, a, C_FROM_VALUE, C_IN_STACK, 0, at, size);
        }
    }
    return true;
}

#else

static bool plan(c_signature &s, std::string &why) {
    why = "calling C is not available on this processor yet (it is on x64 Windows and Linux, ARM64 and ARM32 with hard float)";
    return false;
}

#endif

bool c_parse_signature(const std::string &text, c_signature &s, std::string &why) {
    s = c_signature();
    s.text = text;
    const char *p = text.c_str();
    skip_blanks(p);
    if(*p != '(') { why = "a signature is written  (argument,argument)result"; return false; }
    p++;
    while(true) {
        skip_blanks(p);
        if(*p == ')') { p++; break; }
        if(*p == ',') { p++; continue; }
        if(*p == 0) { why = "the `(` of the signature is never closed"; return false; }
        if(p[0] == '.' && p[1] == '.' && p[2] == '.') {
            p += 3;
            if(s.variadic_from < 0) s.variadic_from = (int) s.args.size();
            continue;
        }
        c_type type;
        if(!parse_one(p, type, why, 0)) return false;
        if(type.kind == C_VOID) {
            if(s.args.empty()) { skip_blanks(p); if(*p == ')') continue; }      // (void): no arguments
            why = "`void` cannot be an argument";
            return false;
        }
        // one of the variable arguments: C hands a float over as a double there (and so must we)
        if(s.variadic_from >= 0 && type.kind == C_F32) { type.kind = C_F64; type.size = 8; type.align = 8; }
        s.args.push_back(type);
        if(s.args.size() > (size_t) C_MAX_ARGUMENTS) { why = "more than " + std::to_string(C_MAX_ARGUMENTS) + " arguments"; return false; }
    }
    if(!parse_one(p, s.result, why, 0)) return false;
    skip_blanks(p);
    if(*p != 0) { why = std::string("something is left over after the result type: `") + p + "`"; return false; }
    if(s.result.kind == C_STRUCT && s.result.size > C_MAX_RESULT) {
        why = "a struct result of more than " + std::to_string(C_MAX_RESULT) + " bytes";
        return false;
    }
    return plan(s, why);
}

// ---------------------------------------------------------------------------------
// 3. The call
// ---------------------------------------------------------------------------------
//
// One C++ function type per shape of result carries every argument register; `stack` words follow
// as further arguments, which the C++ compiler puts on the stack in order - exactly where the
// convention wants the arguments that did not get a register.

#define LIST8(v, k) v[k], v[k + 1], v[k + 2], v[k + 3], v[k + 4], v[k + 5], v[k + 6], v[k + 7]
#define LIST32(v, k) LIST8(v, k), LIST8(v, k + 8), LIST8(v, k + 16), LIST8(v, k + 24)
#define LIST128(v) LIST32(v, 0), LIST32(v, 32), LIST32(v, 64), LIST32(v, 96)
#define TYPE8(T) T, T, T, T, T, T, T, T
#define TYPE32(T) TYPE8(T), TYPE8(T), TYPE8(T), TYPE8(T)
#define TYPE128(T) TYPE32(T), TYPE32(T), TYPE32(T), TYPE32(T)

typedef uintptr_t word_t;

#if defined(C_ABI_WIN64)

template<bool FP> struct slot {
    typedef uint64_t type;
    static type of(const uint64_t *r, const double *f, int i) { return r[i]; }
};
template<> struct slot<true> {
    typedef double type;
    static type of(const uint64_t *r, const double *f, int i) { return f[i]; }
};

#define SLOT_TYPES typename slot<(M & 1) != 0>::type, typename slot<(M & 2) != 0>::type, typename slot<(M & 4) != 0>::type, typename slot<(M & 8) != 0>::type
#define SLOT_VALUES slot<(M & 1) != 0>::of(r, f, 0), slot<(M & 2) != 0>::of(r, f, 1), slot<(M & 4) != 0>::of(r, f, 2), slot<(M & 8) != 0>::of(r, f, 3)

template<int M, class R> static R go(void *fn, const uint64_t *r, const double *f, const word_t *s, uint32_t words) {
    if(words == 0) return ((R (*)(SLOT_TYPES)) fn)(SLOT_VALUES);
    if(words <= 8) return ((R (*)(SLOT_TYPES, TYPE8(word_t))) fn)(SLOT_VALUES, LIST8(s, 0));
    if(words <= 32) return ((R (*)(SLOT_TYPES, TYPE32(word_t))) fn)(SLOT_VALUES, LIST32(s, 0));
    return ((R (*)(SLOT_TYPES, TYPE128(word_t))) fn)(SLOT_VALUES, LIST128(s));
}

template<class R> static R go_slots(int mask, void *fn, const uint64_t *r, const double *f, const word_t *s, uint32_t words) {
    switch(mask) {
#define CASE(m) case m: return go<m, R>(fn, r, f, s, words);
        CASE(0) CASE(1) CASE(2) CASE(3) CASE(4) CASE(5) CASE(6) CASE(7)
        CASE(8) CASE(9) CASE(10) CASE(11) CASE(12) CASE(13) CASE(14) CASE(15)
#undef CASE
    }
    return R();
}

static void call(const c_signature &sig, void *fn, const uint8_t *ints, const uint8_t *fps, const uint8_t *stack,
                 uint8_t *int_result, uint8_t *fp_result, void *memory) {
    uint64_t r[4];
    double f[4];
    memcpy(r, ints, sizeof(r));
    memcpy(f, fps, sizeof(f));
    uint32_t words = sig.stack_bytes / 8;
    if(sig.shape == 1) {
        double result = go_slots<double>(sig.fp_slots, fn, r, f, (const word_t*) stack, words);
        memcpy(fp_result, &result, 8);
    } else {
        uint64_t result = go_slots<uint64_t>(sig.fp_slots, fn, r, f, (const word_t*) stack, words);
        memcpy(int_result, &result, 8);
    }
}

#elif defined(C_ABI_SYSV64) || defined(C_ABI_ARM64) || defined(C_ABI_ARM32)

#if defined(C_ABI_SYSV64)
#define INT_TYPES word_t, word_t, word_t, word_t, word_t, word_t
#define INT_VALUES r[0], r[1], r[2], r[3], r[4], r[5]
#define INT_COUNT 6
#elif defined(C_ABI_ARM64)
#define INT_TYPES TYPE8(word_t)
#define INT_VALUES LIST8(r, 0)
#define INT_COUNT 8
#else
#define INT_TYPES word_t, word_t, word_t, word_t
#define INT_VALUES r[0], r[1], r[2], r[3]
#define INT_COUNT 4
#endif

// Linux x64: a function with variable arguments is told, in a register of its own, how many floating
// point registers carry arguments. A call through a function type that has variable arguments sets
// it - so every call is made that way there: a function without them never looks.
#if defined(C_ABI_SYSV64)
#define STACK_TYPES(list) ...
#define NO_STACK_TYPES , ...
#else
#define STACK_TYPES(list) list
#define NO_STACK_TYPES
#endif

template<class R> static R go(void *fn, const word_t *r, const double *f, const word_t *s, uint32_t words) {
    if(words == 0) return ((R (*)(INT_TYPES, TYPE8(double) NO_STACK_TYPES)) fn)(INT_VALUES, LIST8(f, 0));
    if(words <= 8) return ((R (*)(INT_TYPES, TYPE8(double), STACK_TYPES(TYPE8(word_t)))) fn)(INT_VALUES, LIST8(f, 0), LIST8(s, 0));
    if(words <= 32) return ((R (*)(INT_TYPES, TYPE8(double), STACK_TYPES(TYPE32(word_t)))) fn)(INT_VALUES, LIST8(f, 0), LIST32(s, 0));
    return ((R (*)(INT_TYPES, TYPE8(double), STACK_TYPES(TYPE128(word_t)))) fn)(INT_VALUES, LIST8(f, 0), LIST128(s));
}

struct ret_ii { word_t a, b; };
struct ret_dd { double a, b; };
struct ret_id { word_t a; double b; };
struct ret_di { double a; word_t b; };
struct ret_d4 { double a, b, c, d; };
template<int N> struct ret_big { char bytes[N]; };

static void call(const c_signature &sig, void *fn, const uint8_t *ints, const uint8_t *fps, const uint8_t *stack,
                 uint8_t *int_result, uint8_t *fp_result, void *memory) {
    word_t r[INT_COUNT];
    double f[8];
    memcpy(r, ints, sizeof(r));
    memcpy(f, fps, sizeof(f));
    const word_t *s = (const word_t*) stack;
    uint32_t words = sig.stack_bytes / WORD;
#if defined(C_ABI_SYSV64)
    switch(sig.shape) {
        case 1: { ret_dd result = go<ret_dd>(fn, r, f, s, words); memcpy(fp_result, &result, 16); break; }
        case 2: { ret_id result = go<ret_id>(fn, r, f, s, words); memcpy(int_result, &result.a, 8); memcpy(fp_result, &result.b, 8); break; }
        case 3: { ret_di result = go<ret_di>(fn, r, f, s, words); memcpy(fp_result, &result.a, 8); memcpy(int_result, &result.b, 8); break; }
        default: { ret_ii result = go<ret_ii>(fn, r, f, s, words); memcpy(int_result, &result, 16); break; }
    }
#elif defined(C_ABI_ARM64)
    switch(sig.shape) {
        case 1: { ret_d4 result = go<ret_d4>(fn, r, f, s, words); memcpy(fp_result, &result, 32); break; }
        case 2: {
            // the C++ compiler hands the function the address of `result` in x8, which is where a C
            // function writes a struct of more than 16 bytes
            uint32_t size = sig.result.size;
            if(size <= 64) { ret_big<64> result = go<ret_big<64> >(fn, r, f, s, words); memcpy(memory, &result, size); }
            else if(size <= 512) { ret_big<512> result = go<ret_big<512> >(fn, r, f, s, words); memcpy(memory, &result, size); }
            else { ret_big<4096> result = go<ret_big<4096> >(fn, r, f, s, words); memcpy(memory, &result, size); }
            break;
        }
        default: { ret_ii result = go<ret_ii>(fn, r, f, s, words); memcpy(int_result, &result, 16); break; }
    }
#else
    if(sig.shape == 1) { ret_d4 result = go<ret_d4>(fn, r, f, s, words); memcpy(fp_result, &result, 32); }
    else { uint64_t result = go<uint64_t>(fn, r, f, s, words); memcpy(int_result, &result, 8); }
#endif
}

#else

static void call(const c_signature &sig, void *fn, const uint8_t *ints, const uint8_t *fps, const uint8_t *stack,
                 uint8_t *int_result, uint8_t *fp_result, void *memory) {
}

#endif

/** One number or pointer as the bytes C has for it, in 8 bytes (a whole number carries its sign all the way up). */
static uint64_t bits_of(c_kind kind, const c_value &value) {
    uint64_t bits = 0;
    switch(kind) {
        case C_I8: bits = (uint64_t) (int64_t) (int8_t) value.i; break;
        case C_U8: bits = (uint8_t) value.i; break;
        case C_I16: bits = (uint64_t) (int64_t) (int16_t) value.i; break;
        case C_U16: bits = (uint16_t) value.i; break;
        case C_I32: bits = (uint64_t) (int64_t) (int32_t) value.i; break;
        case C_U32: bits = (uint32_t) value.i; break;
        case C_I64: case C_U64: bits = (uint64_t) value.i; break;
        case C_F32: { float f = (float) value.d; uint32_t b; memcpy(&b, &f, 4); bits = b; break; }
        case C_F64: memcpy(&bits, &value.d, 8); break;
        case C_PTR: bits = (uint64_t) (uintptr_t) value.p; break;
        default: break;
    }
    return bits;
}

void c_invoke(const c_signature &sig, void *function, const c_value *arguments, c_value *result, void *struct_result) {
    uint8_t ints[8 * 8] = { 0 };
    uint8_t fps[8 * 8] = { 0 };
    // (as words, so that it is aligned for them)
    word_t stack_words[MAX_STACK_WORDS];
    uint8_t *stack = (uint8_t*) stack_words;
    if(sig.stack_bytes > 0) memset(stack, 0, sig.stack_bytes);
    uint64_t bits[C_MAX_ARGUMENTS];
    uint64_t addresses[C_MAX_ARGUMENTS];
    uint8_t few_copies[512];
    std::vector<uint8_t> many_copies;
    uint8_t *copies = few_copies;
    if(sig.copy_bytes > sizeof(few_copies)) {
        many_copies.resize(sig.copy_bytes);
        copies = many_copies.data();
    }
    // a struct result that is written through a hidden pointer needs a place even when the caller wants none
    uint8_t few_result[64];
    std::vector<uint8_t> large_result;
    void *memory = struct_result;
    if(memory == nullptr && sig.result.kind == C_STRUCT) {
        memory = few_result;
        if(sig.result.size > sizeof(few_result)) {
            large_result.resize(sig.result.size);
            memory = large_result.data();
        }
    }

    size_t count = sig.args.size();
    for(size_t a = 0; a < count; a++)
        if(sig.args[a].kind != C_STRUCT) bits[a] = bits_of(sig.args[a].kind, arguments[a]);

    for(const c_move &move : sig.moves) {
        const uint8_t *from;
        if(move.source == C_FROM_RESULT_ADDRESS) {
            addresses[0] = (uint64_t) (uintptr_t) memory;
            from = (const uint8_t*) &addresses[0];
        } else if(move.source == C_FROM_COPY_ADDRESS) {
            // C may change a struct it was handed by value: it gets a copy, never the caller's own bytes
            uint8_t *copy = copies + move.from;
            memcpy(copy, arguments[move.arg].p, sig.args[move.arg].size);
            addresses[move.arg] = (uint64_t) (uintptr_t) copy;
            from = (const uint8_t*) &addresses[move.arg];
        } else if(sig.args[move.arg].kind == C_STRUCT) {
            from = (const uint8_t*) arguments[move.arg].p + move.from;
        } else {
            from = (const uint8_t*) &bits[move.arg] + move.from;
        }
        uint8_t *to = (move.place == C_IN_INT ? ints : move.place == C_IN_FP ? fps : stack) + move.to;
        memcpy(to, from, move.bytes);
    }

    uint8_t int_result[16] = { 0 };
    uint8_t fp_result[32] = { 0 };
    call(sig, function, ints, fps, stack, int_result, fp_result, memory);

    if(sig.result.kind == C_VOID) return;
    if(sig.result.kind == C_STRUCT) {
        if(!sig.result_in_memory) {
            for(const c_move &piece : sig.pieces)
                memcpy((uint8_t*) memory + piece.to, (piece.place == C_IN_INT ? int_result : fp_result) + piece.from, piece.bytes);
        }
        if(result != nullptr) result->p = memory == few_result || !large_result.empty() ? nullptr : memory;
        return;
    }
    if(result == nullptr) return;
    uint8_t raw[8] = { 0 };
    for(const c_move &piece : sig.pieces)
        memcpy(raw + piece.to, (piece.place == C_IN_INT ? int_result : fp_result) + piece.from, piece.bytes);
    switch(sig.result.kind) {
        case C_I8: { int8_t v; memcpy(&v, raw, 1); result->i = v; break; }
        case C_U8: { uint8_t v; memcpy(&v, raw, 1); result->i = v; break; }
        case C_I16: { int16_t v; memcpy(&v, raw, 2); result->i = v; break; }
        case C_U16: { uint16_t v; memcpy(&v, raw, 2); result->i = v; break; }
        case C_I32: { int32_t v; memcpy(&v, raw, 4); result->i = v; break; }
        case C_U32: { uint32_t v; memcpy(&v, raw, 4); result->i = v; break; }
        case C_I64: case C_U64: { int64_t v; memcpy(&v, raw, 8); result->i = v; break; }
        case C_F32: { float v; memcpy(&v, raw, 4); result->d = v; break; }
        case C_F64: { memcpy(&result->d, raw, 8); break; }
        case C_PTR: { uintptr_t v = 0; memcpy(&v, raw, sizeof(v)); result->p = (void*) v; break; }
        default: break;
    }
}
