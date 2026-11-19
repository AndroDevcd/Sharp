//
// The C++ side of helloworld.sharp.
//
// hello_sni.h is written by `sharpc helloworld.sharp -o helloworld -sni generated`.
// It declares one C++ function for each function of the `native "hello" { }` block:
// the library's name, `_`, the function's name.
//
#include "hello_sni.h"

// Sharp: def greeting(name: string) : string
// Text arrives as a reference (sni_ref); `env` reads it and makes new text.
sni_ref hello_greeting(sni_env* env, sni_ref name) {
    return env->new_text("Hello, " + env->text(name) + "!");
}

// Sharp: def add(a: _int32, b: _int32) : _int32
// Numbers arrive as the C++ types of the same size.
int32_t hello_add(sni_env* env, int32_t a, int32_t b) {
    return a + b;
}
