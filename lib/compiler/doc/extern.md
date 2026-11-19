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
| a function type: `(c_int, c_int)(c_int)` | a pointer to a function | a Sharp function C can call: section 3a |
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

## 3a. Functions handed to C

```
extern "c" { def qsort(items: _int32[], count: c_size, width: c_size, compare: (pointer, pointer)(c_int)); }

qsort(values, n, 4, { a: pointer, b: pointer -> return ...; });
```

A parameter with a function type takes a lambda or a function's name; null is NULL. C may call it
for as long as the call it was handed to runs - enough for `qsort` and "call this for each".

When C keeps the pointer and calls it later (an event handler, a timer), say so first:

```
on_tick := { interval: c_uint, data: pointer -> return interval; };
c_keep(on_tick);                    // before it is handed over
SDL_AddTimer(1000, on_tick, 0);
...
c_drop(on_tick);                    // C no longer has it
```

A kept function has one address in C until it is dropped, however often it is handed over.

- What C calls it with are numbers and `pointer`s (a text or a struct arrives as a `pointer`:
  read it with `c_memory.view`).
- A callback runs on the thread that made the C call, while a C call of that thread runs. One that
  arrives on a thread C started itself cannot run: it is reported and answers 0.
- An exception thrown inside a callback is raised in Sharp when the C call returns.
- At most 32 functions of one shape are handed to C at the same time (8 on Windows).

## 3b. Memory that is C's

`c_memory` (`std/c/memory.sharp`) is a block from C's `malloc` that stays at one address - for what
C keeps hold of after a call - and a checked view onto memory C gave you:

```
block := new c_memory(16);
block.put_int32(0, 640).put_int32(4, 480);
some_c_function(block.address());
width := block.int32_at(0);
block.release();

header := c_memory.view(pointer_from_c, 24);        // C's memory: read and written, not released
```

Every read and write is checked against the block's size; a released block refuses every use.
Guard bytes around a block of ours show when C wrote past an end: `damaged()`, and `release()`
throws. The class is written in Sharp alone, with `malloc`, `free` and `memcpy` called directly.

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
| a read or write outside a `c_memory` block, or after `release()` | `out_of_bounds_exception`, `illegal_state_exception` |
| C wrote past the end of a `c_memory` block | `damaged()` is true; `release()` throws |
| too many functions handed to C at once | `illegal_state_exception` naming the limit |
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
| functions handed to C | `lib/runtime/cffi/c_callback.cpp` |
| `"c_libraries"` in settings.json | `configuration/settings_processor.sharp`, `read_c_libraries_setting` |
| the library | `lib/support/<version>/std/c/` |
| tests | `tests/cffi/engine_test.sh` (the conventions, without the VM), `tests/cffi/run.sh` (from Sharp) |
