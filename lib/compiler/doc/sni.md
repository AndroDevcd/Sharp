# SNI: the Sharp Native Interface

How a Sharp program calls a function written in C++.
(Java comparison: the JNI. The differences are at the end.)

## 1. The whole thing in four steps

**1. Declare** the functions in Sharp, without a body, and **load** the library before the first call:

```
import ( platform )

native "sensors" {
    def read(port: _int32) : decimal;
    def label(port: _int32) : string;
}

def main() {
    load_library("sensors");
    println("" + read(3));
}
```

The name `"sensors"` only **groups** the functions: it names the generated files and begins the
C++ function names. It loads nothing. Loading is always a call the program makes (section 5).

**2. Compile** with `-sni <folder>`:

```
sharpc main.sharp -o app -sni native
```

That writes three files into `native/`, again with every compile. Never edit them.

**In a project** (`sharpc -P <folder>`, a folder with `settings.json`) no `-sni` is needed: when
the program has native functions the files go to `build/outputs/sni/`, beside the executable.
(`-sni <folder>` still puts them somewhere else.)

| file | what it is |
|---|---|
| `sni.h` | the interface itself |
| `sensors_sni.h` | the C++ prototypes you have to define |
| `sensors_sni.cpp` | the glue the VM calls |

**3. Write** the functions in a file of your own:

```cpp
#include "sensors_sni.h"

double sensors_read(sni_env* env, int32_t port) { return 0.5 * port; }

sni_ref sensors_label(sni_env* env, int32_t port) {
    return env->new_text("port " + std::to_string(port));
}
```

**4. Build** the library:

```
Linux:    g++ -shared -fPIC -std=c++11 -I native sensors.cpp native/sensors_sni.cpp -o sensors.so
Windows:  g++ -shared -static -std=c++11 -I native sensors.cpp native/sensors_sni.cpp -o sensors.dll
```

A working example: `examples/C++ Hello World`.

## 2. Declaring native functions

| written | group | C++ name |
|---|---|---|
| `native "sensors" { def read(...) }` | `sensors` | `sensors_read` |
| `native def version() : _int32;` in module `app` | `app` (the module's name) | `app_version` |
| `native def spin(...)` in class `motor`, module `app` | `app` | `app_motor_spin` |
| a second function with the same name (an overload) | | `sensors_read_2` |

- A native block may stand in a file or inside a class.
- A native function of a class that is not `static` belongs to its objects: the C++ function gets
  the object as `sni_ref self`, right after `env`.
- A native function has no body. (`native` with a body is an error.)

## 3. How values cross

| Sharp | C++ |
|---|---|
| `_int8` ... `_int64`, `_uint8` ... `_uint64` | `int8_t` ... `int64_t`, `uint8_t` ... `uint64_t` |
| `decimal` | `double` |
| no result | `void` |
| `string`, an array, any object | `sni_ref` |

An `sni_ref` is a reference. Never look inside it; ask `env`:

| `env->` | what it does |
|---|---|
| `text(ref)` | a `std::string` copy of a Sharp string |
| `new_text("...")` | a new Sharp string |
| `array(ref)` | the elements of a number array **in place** (`data`, `count`, `element_size`): no copy, writes are seen by Sharp |
| `new_array(SNI_INT32, count)` | a new number array, all 0 |
| `keep(ref)` / `drop(ref)` | hold an object after your function returns / give it back |
| `raise("std#illegal_argument_exception", "message")` | report a problem: the exception is raised in Sharp when you return |
| `pending()` / `clear()` | was a problem reported? / forget it |
| `should_stop()` | was this thread asked to stop? |

## 4. The three rules

1. **A reference lasts until your function returns.** One you were handed, or one `env` made for
   you. To use an object later (from another call), `keep` it, and `drop` it when you are done.
2. **A loop that runs for a while asks `env->should_stop()`** and returns when it says yes. The VM
   cannot interrupt C++. A thread that is asked to stop and stays inside C++ is waited for
   2 seconds; then the VM prints a warning naming the function and stops waiting. The thread is
   never killed half way (it could hold a lock); it ends with the program.
3. **After `raise`, return.** The exception appears at the call in Sharp. A class the program does
   not have becomes a `runtime_exception` with the same message.

## 5. What the VM checks

The library's table carries each function's signature as text, written when the program was
compiled: `(_int32)decimal`, `(self,decimal[])ref`. The VM builds the same text from the running
program and compares the two when it links a function. When they differ - the declaration changed
and the library was not rebuilt - the call is refused:

```
unsatisfied_link_error (the native library `sensors` was built for another version of `app#__srt_global.read`
    the program declares:   (_int32,decimal)decimal
    the library was built:  (_int32)decimal
    Run `sharpc -sni <folder>` again and rebuild the library.)
```

## 5a. Loading a library

Nothing is loaded automatically. The program says which file, with a path:

```
load_library("sensors");            // sensors.dll / sensors.so
load_library("libs/sensors");       // in a folder
load_library("C:/tools/sensors");   // anywhere
```

- The ending may be left off, so the same line works everywhere: `.dll` on Windows, `.so` on
  Linux (and `lib` + name + `.so` is tried too).
- A path that is not absolute starts at the folder `sharp` is run in, like any file path.
  Nothing is searched: no folder list, no system path.
- One library file may hold several groups; a group may also be split over several files. When a
  native function is first called, the VM looks for it in every library that is loaded.
- `free_library("sensors")` releases it again (the same text as it was loaded with).

What is refused, each with a message that says what to do:

| mistake | message |
|---|---|
| the file is not there | `could not load the native library ...: no such file. Looked for:` and every path tried |
| the file is there but was built for another system (a Linux library named `.dll`, built in WSL) | `... is there, but it could not be loaded: it is not a Windows library of this kind ...` |
| the `.dll` needs the compiler's own DLLs and they are not found | `... a library IT needs was not found ... Build it with -static` |
| a native call before any `load_library` | `... was called, and no native library is loaded` + the line to add |
| the function is in no loaded library | `no loaded native library has the function ...` + the libraries that are loaded |

## 5b. A project builds its C++ itself

In a project nobody has to type a g++ line. Say where the C++ is, in `settings.json`:

```json
{
  "name": "app",
  "output": "app",
  "native": { "sources": [ "src/cpp" ] }
}
```

`sharpc -P .` (and Build in CLion) then does all of it:

| step | result |
|---|---|
| the Sharp code | `build/outputs/app` |
| the glue | `build/outputs/sni/` |
| the C++ | `build/outputs/<group>.dll` on Windows, `<group>.so` on Linux |

One library per group: `native "foo" { }` becomes `foo.dll`, built from the project's C++ files and
the group's glue. The program loads it from its own folder, so it runs from anywhere:

```
load_library(program_folder() + "/foo");
```

`program_folder()` (module `platform`) is the folder the running program's file is in.

**Nothing is built twice.** The C++ is compiled again only when a source, a header or the glue
changed, or the command did (another compiler, other flags).

**A project with native functions and no `"native"` setting** still gets its glue; the build prints
the line to add.

### The C++ toolchain

```json
"native": {
  "sources": [ "src/cpp" ],
  "toolchain": {
    "compiler": "C:/msys64/ucrt64/bin/g++.exe",
    "flags": [ "-O2", "-Wall" ],
    "include": [ "third_party/include" ],
    "libraries": [ "ws2_32" ],
    "windows": { "compiler": "C:/msys64/ucrt64/bin/g++.exe" },
    "linux":   { "compiler": "g++", "flags": [ "-O2" ],
                 "processor": { "arm": { "libraries": [ "wiringPi", "rf24" ] } } },
    "processor": { "arm": { "flags": [ "-mfpu=neon" ] } }
  }
}
```

| setting | what it is |
|---|---|
| `compiler` | g++ or clang++: a path, or a name looked up on the `PATH` |
| `flags` | added after the flags Sharp needs, so yours win (`-std=c++20` replaces the default `-std=c++17`) |
| `include` | folders that hold only headers (`-I`): another library's. The `sources` folders are searched for headers already - do not name them again |
| `libraries` | more libraries to link (`-l`) |
| `windows`, `linux` | the same four settings, read on top of the others on that system only - and `sources`: C++ for that system only |
| `processor` | an object of processors - `arm` or `x86`, each covering 32 and 64 bit - each with `sources`, `flags`, `include`, `libraries`. Used only when the C++ compiler builds for that processor. Written at the top it counts on every system; inside `windows` / `linux`, for that processor on that system. The system and the processor are separate because they are: ARM runs Linux and Windows. |

**`sources` is the one place a folder of C++ is named.** Wherever it is written - under `native`, in a system's block, in a processor's - the folder is compiled *and* searched for headers. So C++ written once per processor needs nothing else:

```json
"native": {
  "sources": [ "native/common" ],
  "toolchain": {
    "processor": {
      "arm": { "sources": [ "native/arm" ] },
      "x86": { "sources": [ "native/x86" ] }
    }
  }
}
```

On a Raspberry Pi `native/common` and `native/arm` are built; on a PC `native/common` and `native/x86`. The two folders may hold files of the same names. `include` is only for folders that hold nothing but headers - another library's.

The flags Sharp always gives itself: `-shared`, `-std=c++17`, `-static` on Windows (the C++ runtime
goes inside the library, so the `.dll` needs no other `.dll`), `-fPIC` elsewhere, and `-I` for the
glue folder.

**Which compiler is used** - the first of these that names one:

1. `-cxx <compiler>` on the command line, for one build
2. `"compiler"` in the project's `settings.json`
3. the environment variable `SHARP_CXX`: this machine's default (the CLion plugin sets it from
   Settings > Sharp > C++ compiler)
4. a search: `g++`, then `clang++` on the `PATH`; on Windows also where MinGW is usually installed
   (CLion's own, MSYS2, LLVM)

The build prints which one it used and who named it:
`C++ compiler: C:\...\mingw\bin\g++.exe   (found where it is usually installed)`.

Not supported: Microsoft's `cl.exe` (other flags; refused with a message). The g++ inside WSL is
never picked for a Windows build - it writes Linux libraries, which Windows cannot load.

When the C++ does not build, the C++ compiler's own messages are shown, then the command that was
run, and the build fails.

## 6. Garbage collection, threads, fibers

- While C++ runs, the thread counts as paused for the collector: a collection never waits for a
  native function, however long it takes.
- Objects do not move, so an array's `data` pointer stays right for as long as you may use the
  reference.
- A fiber that has a C++ function on its stack is never switched to another fiber: the C++ part of
  its stack cannot be taken apart. It can be suspended or asked to stop.

## 7. Where the code is

| part | where |
|---|---|
| the interface | `lib/runtime/sni/sni.h` |
| the VM's side: linking, the call, the stop rule | `lib/runtime/sni/sni_runtime.cpp` |
| `native "name" { }` in the parser | `frontend/syntax/parser/parse_declarations.sharp`, `parse_native_block` |
| libraries, C++ names, signatures | `backend/finalizer/sni/native_libraries.sharp` |
| the generated files | `backend/finalizer/sni/native_files.sharp` |
| the copy of sni.h the compiler writes | `backend/finalizer/sni/sni_header.sharp` (made by `build-scripts/embed_sni_header.py`) |
| building a project's C++ | `native_build.sharp`; the settings: `configuration/settings_processor.sharp`, `read_native_setting` |
| `program_folder()` | `lib/support/<version>/platform/dl.sharp`; VM interrupt `0x53` |
| tests | `tests/sni.sh`, `tests/sni/` |

## 8. Compared with the JNI

- The glue is generated C++ you can read, not a calling-convention trick; numbers are typed on
  both sides and checked when the library is linked.
- No `FindClass` / `GetMethodID` strings in the core. (A generated, typed map of classes and
  fields is the next step.)
- Not there yet: reading fields, calling Sharp from C++, threads made by C++.
