//
// The C++ side of the library "app": the native functions of tests/sni/basic/main.sharp that
// are declared alone (`native def`), so they belong to the library named after their module.
//
#include "app_sni.h"

// a function of a class: `self` is the object it was called on
int32_t app_counter_plus(sni_env* env, sni_ref self, int32_t extra) {
    return self == nullptr ? -1 : 100 + extra;
}

int32_t app_counter_twice(sni_env* env, int32_t x) { return x * 2; }

int32_t app_version(sni_env* env) { return 1; }
