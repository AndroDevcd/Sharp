//
// EXTERN FUNCTIONS IN THE VM: a Sharp function that IS a C function
//
//      extern "sdl" { def SDL_GetTicks() : c_uint; }
//
// The executable says, per extern function, which library, which C function and its
// signature (the compiler: backend/finalizer/sni/extern_functions.sharp). Here that becomes a call:
//
//      c_declare        the loader hands over what the executable says
//      c_call_extern    the function is called: found the first time (the library is opened, the
//                       function looked up in it, the signature turned into a plan), then the
//                       arguments are taken off the VM's stack, handed to C (c_call.h), and the
//                       result put where a Sharp function leaves its result
//
// WHICH FILE IS THE LIBRARY "sdl"? In this order:
//      1. what the program said:  c_library("sdl", "C:/libs/SDL2.dll")   (several may be said: tried in order)
//      2. what the project's settings.json said for this system (the compiler wrote it into the executable)
//      3. the name itself
// and each of those is looked for
//      - exactly there, when it is an absolute path
//      - otherwise beside the program, in the project's folder, in the folder the program was started in,
//      - and then wherever the system looks for libraries (the PATH on Windows, the loader's folders on Linux)
// with and without the system's ending (.dll / .so, and `lib` in front).
// The library "c" is C's own: the functions every C program has (strlen, malloc, qsort ...).
//
#pragma once

#include "../../../stdimports.h"
#include <string>

struct sharp_function;
struct sharp_thread;
struct c_binding;

/** From the loader: `fun` is the C function `symbol` of `library`, with this signature. */
void c_declare(sharp_function *fun, const std::string &library, const std::string &symbol, const std::string &signature);

/** From the loader: where the project says `library` is on this system (a path or a name; may be called more than once). */
void c_project_library(const std::string &library, const std::string &place);
/** From the loader: the folder of the project the program was built from ("" when it was not built from one). */
void c_project_folder(const std::string &folder);

/** From Sharp, c_library(name, place): where `library` is. Said before the library's first call. */
void c_set_library(const std::string &library, const std::string &place);

/** An extern function is called: `argSize` arguments are on the stack. */
void c_call_extern(sharp_function *fun, Int argSize);

struct object;
struct sharp_object;
/**
 * For functions handed to C (c_callback.cpp): a struct C handed over, as a new object of the struct's
 * class in `slot`; and the struct an object is, as C's bytes. `text` is the struct as a signature
 * spells it, {@module#class x:i32 y:i32}. false and `why` when it cannot be done.
 */
bool c_struct_to_object(const std::string &text, const uint8_t *bytes, object *slot, std::string &why);
bool c_struct_from_object(const std::string &text, sharp_object *o, uint8_t *bytes, std::string &why);

/** Open the library and find the function now (instead of at the first call). Throws when it is not there. */
void c_link(sharp_function *fun);
/** Find every extern function of the program now: "" when all are there, otherwise what is missing. */
std::string c_link_all();

/**
 * For the crash report: when this thread is inside a C function right now, what that function
 * is, how it was declared and what it was handed; "" when it is not.
 */
std::string c_describe_call(sharp_thread *thread);
