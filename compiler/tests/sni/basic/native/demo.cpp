//
// The C++ side of the library "demo" (tests/sni/basic/main.sharp).
// demo_sni.h is written by `sharpc -sni`: every prototype in it must be defined here,
// with exactly these types, or this file does not compile.
//
#include "demo_sni.h"
#include <string>

int32_t demo_add(sni_env* env, int32_t a, int32_t b) { return a + b; }

double demo_half(sni_env* env, double x) { return x / 2; }

int64_t demo_widths(sni_env* env, int8_t a, int16_t b, int64_t c, uint8_t d, uint16_t e, uint32_t f, uint64_t g) {
    // -1 + -2 + -3 + 250 + 65000 + 4000000000 + 5
    return (int64_t) a + b + c + d + e + (int64_t) f + (int64_t) g;
}

int8_t demo_narrow(sni_env* env, int64_t x) { return (int8_t) x; }       // 300 -> 44

sni_ref demo_shout(sni_env* env, sni_ref s) {
    std::string text = env->text(s);
    for(char &c : text) if(c >= 'a' && c <= 'z') c = (char) (c - 32);
    return env->new_text(text);
}

int64_t demo_sum(sni_env* env, sni_ref values) {
    sni_array a = env->array(values);           // the elements, in place
    int64_t total = 0;
    for(int64_t i = 0; i < a.count; i++) total += ((int32_t*) a.data)[i];
    return total;
}

void demo_double_all(sni_env* env, sni_ref values) {
    sni_array a = env->array(values);
    for(int64_t i = 0; i < a.count; i++) ((double*) a.data)[i] *= 2;     // written straight into the Sharp array
}

sni_ref demo_range(sni_env* env, int32_t count) {
    sni_ref made = env->new_array(SNI_INT64, count);
    sni_array a = env->array(made);
    for(int64_t i = 0; i < a.count; i++) ((int64_t*) a.data)[i] = i;
    return made;
}

void demo_fail(sni_env* env, int32_t code) {
    env->raise("std#illegal_argument_exception", "code " + std::to_string(code) + " is not allowed");
}

void demo_fail_unknown(sni_env* env) {
    env->raise("nowhere#no_such_exception", "raised with a class the program does not have");
}

void demo_nothing(sni_env* env) { }

static sni_ref kept = nullptr;

void demo_remember(sni_env* env, sni_ref o) { kept = env->keep(o); }
sni_ref demo_recall(sni_env* env) { return kept; }
void demo_forget(sni_env* env) { env->drop(kept); kept = nullptr; }

sni_ref demo_churn(sni_env* env, int32_t count) {
    // many objects made inside one call: each stays alive until the call returns
    sni_ref last = nullptr;
    for(int32_t i = 0; i < count; i++) {
        env->new_array(SNI_INT64, 64);
        last = env->new_text("made " + std::to_string(i));
    }
    return last;
}

int32_t demo_pick(sni_env* env, int32_t a) { return 1; }
int32_t demo_pick_2(sni_env* env, double a) { return 2; }
