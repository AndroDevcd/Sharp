//
// SNI - THE SHARP NATIVE INTERFACE
// ================================
// How a Sharp program calls a function written in C++, and what that function
// may do while it runs. This one header is everything a native library sees of
// the VM; it is copied next to your code by `sharpc -sni <folder>`.
//
// THE WHOLE IDEA IN FOUR LINES
//
//      Sharp:    native "sensors" { def read(port: _int32) : decimal; }
//                load_library("sensors");                     (before the first call)
//      build:    sharpc main.sharp -o app -sni native/        (writes native/sensors_sni.h and .cpp)
//      C++:      double sensors_read(sni_env* env, int32_t port) { return 0.5 * port; }
//      library:  g++ -shared native/*.cpp -o sensors.dll      (or sensors.so)
//
// Your function gets real C++ types for numbers. Everything else - text, an
// array, any object - arrives as an `sni_ref`, and you ask `env` about it.
//
// THE THREE THINGS TO KNOW
//
//   1. A REFERENCE (`sni_ref`) you were handed, or that `env` gave you, is good
//      until your function returns. To hold on to an object for longer, ask:
//      `env->keep(ref)` - and `env->drop(ref)` when you are done with it.
//
//   2. A LOOP THAT RUNS FOR A WHILE asks `env->should_stop()` now and then and
//      returns when it says yes. The VM cannot interrupt C++; this is how a
//      thread that is asked to stop gets to stop.
//
//   3. A PROBLEM is reported with `env->raise("std#illegal_argument_exception",
//      "port must be 0 or more")`, followed by a plain `return`. The exception
//      is raised in Sharp, at the call.
//
// This header is plain C underneath (a table of functions with a version
// number), with C++ spellings on top, so `env->text(s)` in C++ is
// `env->api->text(env, s)` in C.
//
//      Java comparison: the JNI's JNIEnv, jobject, NewGlobalRef.
//

#ifndef SHARP_SNI_H
#define SHARP_SNI_H

#include <stdint.h>

#ifdef __cplusplus
#include <string>
extern "C" {
#endif

/** The version of this interface. A library says which one it was built with; the VM refuses one it does not know. */
#define SNI_VERSION 1

/** A reference to a Sharp object: text, an array, an instance of a class. 0 is null. Never look inside it. */
typedef struct sni_object_* sni_ref;

/** One argument or one result, as the generated glue passes it: a whole number, a decimal, or a reference. */
typedef union sni_value {
    int64_t i;
    double d;
    sni_ref r;
} sni_value;

/**
 * The elements of a number array (or the bytes of a text), read and written in
 * place - no copy is made. `data` points at `count` elements of `element_size`
 * bytes each. It stays right for as long as you may use the reference it came
 * from. `data` is null for null and for an array without elements.
 */
typedef struct sni_array {
    void* data;
    int64_t count;
    int32_t element_size;       // 1, 2, 4 or 8
    int32_t kind;               // SNI_INT8 ... SNI_DECIMAL: what the elements are
} sni_array;

/** The kinds of number an array can hold (for `new_array`, and in `sni_array.kind`). */
#define SNI_INT8    0
#define SNI_INT16   1
#define SNI_INT32   2
#define SNI_INT64   3
#define SNI_UINT8   4
#define SNI_UINT16  5
#define SNI_UINT32  6
#define SNI_UINT64  7
#define SNI_DECIMAL 9

struct sni_env;

/** What the VM offers a native function. New entries are only ever added at the end. */
typedef struct sni_api {
    int32_t version;

    /** 1 when this thread was asked to stop (or the program is ending): return as soon as you can. */
    int32_t (*should_stop)(struct sni_env* env);

    /** Report a problem: an exception of this Sharp class (`module#class`) with this message is raised when you return. */
    void (*raise)(struct sni_env* env, const char* class_name, const char* message);
    /** 1 when a problem was reported (by you, or by something `env` did for you) and not cleared. */
    int32_t (*pending)(struct sni_env* env);
    /** Forget the reported problem. */
    void (*clear)(struct sni_env* env);

    /** Keep an object alive after your function returns. Returns the reference to use from then on. */
    sni_ref (*keep)(struct sni_env* env, sni_ref ref);
    /** Give a kept reference back. */
    void (*drop)(struct sni_env* env, sni_ref ref);

    /** The elements of a number array, in place. */
    sni_array (*array)(struct sni_env* env, sni_ref array);
    /** A new number array of `count` elements of this kind (SNI_INT8 ...), all 0. */
    sni_ref (*new_array)(struct sni_env* env, int32_t kind, int64_t count);

    /** The bytes of a Sharp string (UTF-8, NOT ended by a 0), in place. */
    sni_array (*text)(struct sni_env* env, sni_ref string);
    /** A new Sharp string holding these `count` bytes. */
    sni_ref (*new_text)(struct sni_env* env, const char* chars, int64_t count);
} sni_api;

/** What every native function is handed first: the way back into the VM. Use it only on the thread it was given to. */
typedef struct sni_env {
    const sni_api* api;
    void* thread;               // the VM's own; nothing for a library to read

#ifdef __cplusplus
    bool should_stop() { return api->should_stop(this) != 0; }

    void raise(const char* class_name, const char* message) { api->raise(this, class_name, message); }
    void raise(const char* class_name, const std::string& message) { api->raise(this, class_name, message.c_str()); }
    bool pending() { return api->pending(this) != 0; }
    void clear() { api->clear(this); }

    sni_ref keep(sni_ref ref) { return api->keep(this, ref); }
    void drop(sni_ref ref) { api->drop(this, ref); }

    sni_array array(sni_ref ref) { return api->array(this, ref); }
    sni_ref new_array(int32_t kind, int64_t count) { return api->new_array(this, kind, count); }

    /** A copy of a Sharp string's text ("" for null). */
    std::string text(sni_ref string) {
        sni_array bytes = api->text(this, string);
        return bytes.data == nullptr ? std::string() : std::string((const char*) bytes.data, (size_t) bytes.count);
    }
    sni_ref new_text(const std::string& chars) { return api->new_text(this, chars.data(), (int64_t) chars.size()); }
    sni_ref new_text(const char* chars) { return new_text(std::string(chars == nullptr ? "" : chars)); }
#endif
} sni_env;

// ---------------------------------------------------------------------------------
// What a library exports. `sharpc -sni` writes all of it (<library>_sni.cpp): you
// never fill these in by hand.
// ---------------------------------------------------------------------------------

/** The glue of one native function: unpacks `arguments`, calls your function, stores what it returned in `result`. */
typedef void (*sni_glue)(sni_env* env, sni_value* arguments, sni_value* result);

/** One native function of a library: its Sharp name, its signature as text, and its glue. */
typedef struct sni_function {
    const char* name;           // "sensors#__srt_global.read"
    const char* signature;      // "(_int32)decimal": the VM compares it with the program's
    sni_glue glue;
} sni_function;

/** A library's table. The one function a library exports, `sni_library_table`, returns it. */
typedef struct sni_library {
    int32_t version;            // SNI_VERSION when it was built
    const char* name;           // "sensors"
    int32_t count;
    const sni_function* functions;
} sni_library;

#if defined(_WIN32) || defined(__CYGWIN__)
#define SNI_EXPORT __declspec(dllexport)
#else
#define SNI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
}
#endif

#endif //SHARP_SNI_H
