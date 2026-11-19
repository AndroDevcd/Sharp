# Calling C directly: `extern` blocks

How a Sharp program calls a function of any C library without a line of C++.
(The older way, with C++ written against Sharp's own interface, is SNI: `sni.md`.)

## 1. The whole thing

```
import ( std.c )

extern "c" {                                        // "c": the functions every C program has
    def strlen(text: string) : c_size;
}

extern "sdl" {                                      // a library, by a name of your choosing
    def SDL_GetTicks() : c_uint;
    def SDL_Delay(ms: c_uint);
}

def main() {
    c_library("sdl", "SDL2");                       // which file "sdl" is
    println("" + strlen("hello") + " " + SDL_GetTicks());
}
```

Each `def` in the block is a declaration without a body, named exactly as the C function is.
Nothing is generated and nothing is built: the first call opens the library, looks the function
up by its name and hands the arguments over as the machine's C calling convention wants them.

`extern` is a word only in front of a library's name; it stays free as a name everywhere else.

## 2. Types

| written | C | notes |
|---|---|---|
| `c_char` `c_uchar` `c_short` `c_ushort` `c_int` `c_uint` `c_longlong` `c_ulonglong` | the same | other names for `_int8` ... `_uint64`, which may be written instead |
| `c_double` | `double` | `decimal` |
| `c_float` | `float` | a decimal in Sharp |
| `c_long` `c_ulong` | `long` | 32 bits on Windows and 32-bit machines, 64 on 64-bit Linux |
| `c_size` `c_ssize` | `size_t` `ssize_t` | as wide as an address |
| `pointer` | any pointer | an address as a number; 0 is NULL |
| `string` | `const char *` | handed over as a copy with the 0 at its end; null is NULL. A C string that comes back is copied into a new string |
| `_int32[]`, `decimal[]` ... | a pointer to the items | in place: C reads and writes the array itself |
| a class based on `c_struct` | a struct **by value** | section 3 |
| ... written with `?` | a **pointer** to the struct | null is NULL |
| nothing after the `:` | `void` | |

The names are aliases in `std/c/types.sharp`. `c_float`, `c_long`, `c_ulong`, `c_size`, `c_ssize`
and `pointer` are known to the compiler by name, because C tells them apart and Sharp does not.

## 3. Structs

```
class SDL_Rect base c_struct { x : c_int; y : c_int; w : c_int; h : c_int; }

extern "sdl" {
    def SDL_RenderFillRect(renderer: pointer, rect: SDL_Rect?) : c_int;     // const SDL_Rect *
}
```

The fields, in order, are the members. A member that is a struct is a field of that struct's
class; a pointer member is a `pointer`. For the length of a call the VM lays the object's fields
out as C does; after a call that took a pointer, what C wrote is read back into the object. A
struct that comes back is a new object (through a pointer: a copy of what it points to).

C never holds the object itself. For a struct C keeps the address of, use C's own memory.

## 4. Which file is the library

For `extern "sdl"`, in this order:

1. what the program said: `c_library("sdl", "C:/libs/SDL2.dll")` (said more than once: tried in order)
2. what the project's `settings.json` says for the system the program runs on:
   ```json
   "c_libraries": {
     "sqlite": "sqlite3",
     "sdl": { "windows": "SDL2.dll", "linux": [ "libSDL2-2.0.so.0", "libs/libSDL2.so" ] }
   }
   ```
3. the name itself

Each of those is looked for exactly there when it is an absolute path; otherwise beside the
program, in the project's folder (the one with `settings.json`), in the folder the program was
started in, and then wherever the system looks for libraries. The ending (`.dll`, `.so`) and a
leading `lib` are added when left off.

`c_check_libraries()` opens every library and finds every function at once, so a missing one
shows when the program starts and not at its first call.

## 5. What is caught

| mistake | what happens |
|---|---|
| a parameter C cannot be handed (an object, an array of objects) | a compile error naming the function and the parameter |
| the library is not found | `unsatisfied_link_error` listing every file that was tried |
| the library has no such function | `unsatisfied_link_error` naming the library's file and the function |
| `c_library()` after the library was opened | `illegal_state_exception` |
| a null struct handed over by value | `nullptr_exception` before C is entered |
| C crashes | the report says `crashed inside the C function ...`, with its declaration and what it was handed - never "contact the language developer", which is kept for a fault of the VM itself |

What nothing can catch: a declaration that does not match the C function. A library file carries
no types. The crash report is the tool for that.

## 6. How a call is made (no machine code of our own)

`lib/runtime/cffi/c_call.cpp`. The signature text the compiler wrote - `(i32,ptr,{x:i32 y:i32})f64` -
becomes a plan: for each argument, which bytes go to which register or stack place under this
machine's convention. A call fills three blocks (integer registers, floating point registers,
stack) and calls through a C++ function type that takes all of those registers, so the C++
compiler loads every one and the C function reads the ones it uses. One file covers Windows x64,
Linux x64, ARM64 and ARM32 (hard float).

## 7. Where the code is

| part | where |
|---|---|
| `extern "name" { }` in the parser | `frontend/syntax/parser/parse_declarations.sharp`, `parse_extern_block` |
| the signature text, the checks | `backend/finalizer/sni/extern_functions.sharp` |
| the section of the executable | `backend/finalizer/output/sections.sharp`, `write_externs`; the VM: `lib/runtime/exe/exe_processor.cpp` |
| a struct class keeps all its fields | `backend/finalizer/layout/kept.sharp` |
| the calling conventions | `lib/runtime/cffi/c_call.cpp` |
| libraries, linking, the call, structs, the crash report | `lib/runtime/cffi/c_bind.cpp`, `lib/runtime/sig_handler.cpp` |
| the library | `lib/support/<version>/std/c/` |
| tests | `tests/cffi/engine_test.sh` (the conventions, without the VM), `tests/cffi/run.sh` (from Sharp) |
