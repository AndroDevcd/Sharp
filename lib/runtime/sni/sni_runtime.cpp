//
// SNI INSIDE THE VM (see sni_runtime.h for the map, sni.h for the library's side)
//
// A NATIVE CALL, START TO END (sni_call_native)
//
//      level := sensors.read(3);
//
//   1. LINK, the first time: find "sensors#__srt_global.read" in the table of a
//      library the program LOADED (load_library("sensors") - nothing is ever
//      loaded by the VM on its own), and compare the signature text the library
//      was built with - "(_int32)decimal" - with the one this program's function
//      has. A difference is an unsatisfied_link_error that shows both.
//   2. PACK: each argument on the VM's stack becomes one `sni_value` - a whole
//      number, a decimal, or a reference.
//   3. ENTER NATIVE: the thread counts as paused for the garbage collector
//      (a `gc_safe_region`), so a collection never waits for C++.
//   4. The library's glue calls the C++ function.
//   5. LEAVE NATIVE: waits here while a collection is running.
//   6. The result goes where a Sharp function's result goes (the slot of the
//      first argument), and a problem the C++ code reported is raised.
//
// A REFERENCE IS THE OBJECT'S ADDRESS. The collector never moves an object, so
// a reference can simply be where the object is; C++ is told never to look
// inside. What keeps the object alive is that the VM remembers it: in the
// thread's `locals` while the native function runs (the collector marks them:
// sni_mark_locals), in the collector's pin table for a kept reference
// (tgc_pin). Nothing is counted and nothing is copied.
//
// WHILE C++ RUNS the thread may not touch objects without telling the VM. The
// few `env` functions that allocate (`new_array`, `new_text`) step back into the
// VM for that moment (`in_vm` below); reading an array in place needs nothing -
// the array cannot move or die while the reference to it is held.
//
//      Java comparison: JNI's native method linking (RegisterNatives /
//      Java_pkg_Class_method lookup), local and global references, and the
//      "thread in native" state HotSpot keeps for safepoints.
//

#include "../cffi/c_bind.h"
#include "sni_runtime.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../memory/tracing_collector.h"
#include "../memory/type_check.h"
#include "../reflect/reflect_helpers.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/fiber/fiber.h"
#include "../../core/access_flag.h"
#include "../../core/thread_state.h"
#include "../../core/opcode/opcode_macros.h"
#include "../multitasking/thread/thread_controller.h"
#include <mutex>
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#define sni_open(path) ((void*) LoadLibraryA((path).c_str()))
#define sni_symbol(handle, name) ((void*) GetProcAddress((HMODULE) (handle), name))
#define sni_close(handle) FreeLibrary((HMODULE) (handle))
#else
#include <dlfcn.h>
#define sni_open(path) dlopen((path).c_str(), RTLD_NOW)
#define sni_symbol(handle, name) dlsym(handle, name)
#define sni_close(handle) dlclose(handle)
#endif

Int sni_grace_millis = 2000;

// ---------------------------------------------------------------------------------
// 1. The libraries
// ---------------------------------------------------------------------------------

/** One loaded library: its name as the program says it, the system's handle, its table. */
struct sni_loaded {
    std::string name;
    void *handle;
    const sni_library *table;
};

static std::recursive_mutex libraries_mutex;
static std::vector<sni_loaded> libraries;           // loaded, in the order they were loaded

/**
 * THE FILES load_library(path) TRIES. `path` is a file path like any other: a
 * path that is not absolute is read from the folder the VM was started in.
 * NOTHING IS SEARCHED - no folder list, no system path. The one convenience:
 * the system's ending may be left off, so one line of Sharp works everywhere:
 *
 *      load_library("libs/sensors")     Windows:  libs/sensors.dll
 *                                       Linux:    libs/sensors.so, libs/libsensors.so
 */
static std::vector<std::string> places_for(const std::string &path) {
#ifdef _WIN32
    const char *ending = ".dll";
#elif defined(__APPLE__)
    const char *ending = ".dylib";
#else
    const char *ending = ".so";
#endif
    size_t cut = path.find_last_of("/\\");
    // (a bare name gets "./" in front: without it the system would search its own folders)
    std::string folder = cut == std::string::npos ? std::string("./") : path.substr(0, cut + 1);
    std::string file = cut == std::string::npos ? path : path.substr(cut + 1);

    // (with the ending first: a program `sensors` and its library `sensors.dll` often sit side by side)
    std::vector<std::string> places;
    places.push_back(folder + file + ending);                    // sensors -> sensors.dll, sensors.so
    places.push_back(folder + "lib" + file + ending);            // sensors -> libsensors.so
    places.push_back(folder + file);                             // exactly as written
    return places;
}

/** Is there a file at this path? (So "not there" and "there, but it does not load" are told apart.) */
static bool file_is_there(const std::string &path) {
    FILE *file = fopen(path.c_str(), "rb");
    if(file == nullptr) return false;
    fclose(file);
    return true;
}

/**
 * Why the system refused to load a library file that IS there - in words that say what to do.
 * Called right after the failed load (it reads the system's last error).
 */
static std::string why_not_loaded() {
#ifdef _WIN32
    DWORD code = GetLastError();
    if(code == ERROR_BAD_EXE_FORMAT)
        return "it is not a Windows library of this kind (64-bit). It was built for another system - for example with"
               "\n    the g++ of Linux or WSL, which writes a Linux library whatever the file is called - or as 32-bit."
               "\n    Build it with a Windows g++ (MinGW-w64, 64-bit).";
    if(code == ERROR_MOD_NOT_FOUND)
        return "a library IT needs was not found - usually the C++ runtime of the compiler that built it"
               "\n    (libstdc++-6.dll, libgcc_s_seh-1.dll, libwinpthread-1.dll)."
               "\n    Build it with `-static`, so it needs none:  g++ -shared -static -std=c++11 ...";
    char text[512] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, text, sizeof(text), nullptr);
    std::string said = text;
    while(!said.empty() && (said.back() == '\n' || said.back() == '\r' || said.back() == ' ')) said.pop_back();
    return "Windows says: " + said + " (error " + std::to_string((long long) code) + ")";
#else
    const char *said = dlerror();
    std::string text = said == nullptr ? "no reason given" : said;
    if(text.find("invalid ELF header") != std::string::npos || text.find("wrong ELF class") != std::string::npos)
        return "it is not a library for this system (" + text + ")."
               "\n    It was built for another system (a Windows .dll, or 32-bit). Build it here:  g++ -shared -fPIC ...";
    return "the system says: " + text;
#endif
}

static sni_loaded *loaded_library(const std::string &name) {
    for(sni_loaded &library : libraries)
        if(library.name == name) return &library;
    return nullptr;
}

/** Load `name`. Returns "" when it is loaded (or was already), else the reason it could not be. */
static std::string load(const std::string &name) {
    std::lock_guard<std::recursive_mutex> guard(libraries_mutex);
    if(loaded_library(name) != nullptr) return "";

    std::string looked;
    std::string refused;            // a file that IS there and that the system would not load, and why
    for(const std::string &place : places_for(name)) {
        void *handle = sni_open(place);
        if(handle == nullptr) {
            // the file is there and the system will not load it: that is another mistake than a wrong path.
            // (The other spellings are still tried; it is reported when none of them loads.)
            std::string why = why_not_loaded();         // (asked first: looking at the file changes the system's last error)
            if(refused.empty() && file_is_there(place))
                refused = "`" + place + "` is there, but it could not be loaded: " + why;
            looked += "\n        " + place;
            continue;
        }
        typedef const sni_library* (*table_function)();
        table_function table_of = (table_function) sni_symbol(handle, "sni_library_table");
        if(table_of == nullptr) {
            sni_close(handle);
            return "`" + place + "` was found, but it is not a Sharp native library: it has no `sni_library_table`."
                   "\n    Compile the glue file `sharpc -sni` wrote (<name>_sni.cpp) into the library.";
        }
        const sni_library *table = table_of();
        if(table == nullptr || table->version < 1 || table->version > SNI_VERSION) {
            sni_close(handle);
            return "`" + place + "` was built for SNI version " + std::to_string(table == nullptr ? 0 : table->version)
                   + ", and this VM knows versions 1 to " + std::to_string(SNI_VERSION) + "."
                   "\n    Build the library again with the sni.h that belongs to this Sharp.";
        }
        libraries.push_back(sni_loaded{ name, handle, table });
        return "";
    }
    if(!refused.empty()) return refused;
    return "no such file. Looked for:" + looked +
           "\n    Give load_library the path of the library file (a path that is not absolute starts at the folder"
           "\n    `sharp` is run in); the ending (.dll, .so) may be left off. No other folder is searched.";
}

void sni_load_library(const std::string &name) {
    std::string problem = load(name);
    if(!problem.empty())
        throw vm_exception(vm.unsatisfied_link_except, "could not load the native library `" + name + "`: " + problem);
}

Int sni_free_library(const std::string &name) {
    std::lock_guard<std::recursive_mutex> guard(libraries_mutex);
    for(size_t i = 0; i < libraries.size(); i++) {
        if(libraries[i].name != name) continue;
        // its functions must be found again before they are called again
        const sni_library *table = libraries[i].table;
        for(uInt f = 0; f < vm.mf.methods; f++) {
            for(int32_t k = 0; k < table->count; k++)
                if(vm.methods[f].nativeGlue == table->functions[k].glue) vm.methods[f].nativeGlue = nullptr;
        }
        sni_close(libraries[i].handle);
        libraries.erase(libraries.begin() + i);
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------------
// 2. Linking: the C++ function of a native Sharp function
// ---------------------------------------------------------------------------------

/** The Sharp name of a number type, as signatures spell it; "" for anything that is not a number. */
static const char *number_name(int type) {
    switch(type) {
        case type_int8: return "_int8";
        case type_int16: return "_int16";
        case type_int32: return "_int32";
        case type_int64: return "_int64";
        case type_uint8: return "_uint8";
        case type_uint16: return "_uint16";
        case type_uint32: return "_uint32";
        case type_uint64: return "_uint64";
        case type_var: return "decimal";
        default: return "";
    }
}

/** Is this parameter or result passed as a number (not an array of numbers, not an object)? */
static bool is_number(sharp_type *type, bool isArray) {
    return type != nullptr && !isArray && number_name(type->type)[0] != 0;
}

/** One type as a signature spells it: `_int32`, `decimal[]`, `ref` for every object, `nil` for no result. */
static std::string type_text(sharp_type *type, bool isArray) {
    if(type == nullptr || type->type == type_nil) return "nil";
    const char *number = number_name(type->type);
    if(number[0] == 0) return "ref";
    return isArray ? std::string(number) + "[]" : std::string(number);
}

/**
 * The signature of a native function as text - "(self,_int32,decimal[])ref" -
 * the same text the compiler writes into the library's table (the compiler:
 * backend/finalizer/native, `signature_text`). `self` stands for the object of
 * a function that belongs to a class.
 */
static std::string signature_of(sharp_function *fun) {
    std::string text = "(";
    bool first = true;
    if(!check_flag(fun->flags, flag_static)) {
        text += "self";
        first = false;
    }
    for(int i = 0; i < fun->paramSize; i++) {
        if(!first) text += ",";
        first = false;
        text += type_text(fun->params[i].type, fun->params[i].isArray);
    }
    return text + ")" + type_text(fun->returnType, fun->arrayReturn);
}

/** Look for `fun` in one library's table; true when it is there (and now linked). Throws when its signature differs. */
static bool link_in(const sni_loaded &library, sharp_function *fun) {
    std::string wanted = signature_of(fun);
    const char *other = nullptr;        // the signature of an entry with the name but not the signature
    for(int32_t i = 0; i < library.table->count; i++) {
        const sni_function &entry = library.table->functions[i];
        if(fun->fullName != entry.name) continue;
        // (overloads share a name: the one with this signature is the one)
        if(wanted != entry.signature) { other = entry.signature; continue; }
        fun->nativeGlue = entry.glue;
        return true;
    }
    if(other != nullptr) {
        throw vm_exception(vm.unsatisfied_link_except,
            "the native library `" + library.name + "` was built for another version of `" + fun->fullName + "`"
            "\n    the program declares:   " + wanted +
            "\n    the library was built:  " + other +
            "\n    Run `sharpc -sni <folder>` again and rebuild the library.");
    }
    return false;
}

void sni_link(sharp_function *fun) {
    std::lock_guard<std::recursive_mutex> guard(libraries_mutex);
    if(fun->nativeGlue != nullptr) return;
    for(const sni_loaded &library : libraries)
        if(link_in(library, fun)) return;

    std::string loaded;
    for(const sni_loaded &library : libraries) loaded += (loaded.empty() ? "" : ", ") + library.name;
    if(loaded.empty()) {
        throw vm_exception(vm.unsatisfied_link_except,
            "the native function `" + fun->fullName + "` was called, and no native library is loaded"
            "\n    Load the library that has it before the first call:  load_library(\"path/to/library\");");
    }
    throw vm_exception(vm.unsatisfied_link_except,
        "no loaded native library has the function `" + fun->fullName + "` " + signature_of(fun) +
        "\n    libraries loaded: " + loaded +
        "\n    Load the library that has it (load_library), or build it again from the files `sharpc -sni <folder>` writes.");
}

Int sni_link_from(const std::string &name, sharp_function *fun) {
    std::lock_guard<std::recursive_mutex> guard(libraries_mutex);
    sni_loaded *library = loaded_library(name);
    if(library == nullptr || fun == nullptr || !fun->nativeFunc) return 0;
    fun->nativeGlue = nullptr;
    return link_in(*library, fun) ? 1 : 0;
}

// ---------------------------------------------------------------------------------
// 3. The thread's side
// ---------------------------------------------------------------------------------

static sni_thread_state *state_of(sharp_thread *thread) {
    if(thread->sni == nullptr) thread->sni = new sni_thread_state();
    return thread->sni;
}

sni_thread_state *sni_state_of(sharp_thread *thread) {
    return state_of(thread);
}

void sni_free_thread_state(sharp_thread *thread) {
    // (an abandoned thread is still inside C++ and may come back to its state: it is left alone)
    if(thread->sni != nullptr && thread->sni->depth > 0) return;
    delete thread->sni;
    thread->sni = nullptr;
}

bool sni_in_native(sharp_thread *thread) {
    return thread != nullptr && thread->sni != nullptr && thread->sni->depth > 0;
}

std::string sni_native_name(sharp_thread *thread) {
    if(!sni_in_native(thread) || thread->sni->function == nullptr) return "";
    return thread->sni->function->fullName;
}

bool sni_may_switch_fiber(sharp_thread *thread) {
    return !sni_in_native(thread);
}

void sni_abandon(sharp_thread *thread) {
    if(thread != nullptr && thread->sni != nullptr) thread->sni->abandoned = true;
}

bool sni_was_abandoned(sharp_thread *thread) {
    return sni_in_native(thread) && thread->sni->abandoned;
}

void sni_mark_locals(sharp_thread *thread, void (*mark)(sharp_object *o)) {
    if(thread == nullptr || thread->sni == nullptr) return;
    for(sharp_object *o : thread->sni->locals) mark(o);
}

/**
 * For the few `env` functions that make objects: the thread stops counting as
 * paused for as long as this lives (and waits first while a collection runs),
 * then counts as paused again - C++ goes on.
 */
struct in_vm {
    sharp_thread *thread;
    explicit in_vm(sharp_thread *t) : thread(t) { tgc_leave_safe(thread); }
    ~in_vm() { tgc_enter_safe(thread); }
};

static sharp_thread *thread_of(sni_env *env) {
    return (sharp_thread*) env->thread;
}

// ---------------------------------------------------------------------------------
// 4. What `env` offers (the table of sni.h)
// ---------------------------------------------------------------------------------

static void api_raise(sni_env *env, const char *class_name, const char *message);

static int32_t api_should_stop(sni_env *env) {
    sharp_thread *thread = thread_of(env);
    return hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED || vm.state == VM_SHUTTING_DOWN;
}

static int32_t api_pending(sni_env *env) {
    return state_of(thread_of(env))->pending ? 1 : 0;
}

static void api_clear(sni_env *env) {
    state_of(thread_of(env))->pending = false;
}

static void api_raise(sni_env *env, const char *class_name, const char *message) {
    sni_thread_state *state = state_of(thread_of(env));
    state->pending = true;
    state->exceptionClass = class_name == nullptr ? "" : class_name;
    state->exceptionMessage = message == nullptr ? "" : message;
}

static sni_ref api_keep(sni_env *env, sni_ref ref) {
    tgc_pin((sharp_object*) ref);           // (the pin table has its own lock; no object is read)
    return ref;
}

static void api_drop(sni_env *env, sni_ref ref) {
    tgc_unpin((sharp_object*) ref);
}

static sni_array array_of(sharp_object *o) {
    sni_array result;
    result.data = nullptr;
    result.count = 0;
    result.element_size = 0;
    result.kind = 0;
    if(o == nullptr) return result;
    result.kind = o->type;
    result.element_size = element_size(o->type);
    result.count = o->size;
    result.data = o->size == 0 ? nullptr : o->data;
    return result;
}

static sni_array api_array(sni_env *env, sni_ref ref) {
    sharp_object *o = (sharp_object*) ref;
    if(o != nullptr && o->type > type_var) {
        api_raise(env, "std#illegal_argument_exception", "sni: `array` was given an object that is not an array of numbers");
        return array_of(nullptr);
    }
    return array_of(o);
}

static sni_ref api_new_array(sni_env *env, int32_t kind, int64_t count) {
    if(count < 0 || number_name(kind)[0] == 0) {
        api_raise(env, "std#illegal_argument_exception", "sni: `new_array` needs a number kind (SNI_INT8 ... SNI_DECIMAL) and a count of 0 or more");
        return nullptr;
    }
    sharp_thread *thread = thread_of(env);
    in_vm here(thread);
    try {
        sharp_object *made = count == 0 ? create_empty_array((data_type) kind) : create_object((Int) count, (data_type) kind);
        state_of(thread)->locals.push_back(made);
        return (sni_ref) made;
    } catch(vm_exception &problem) {
        api_raise(env, "std#out_of_memory_exception", "sni: no memory for a new array");
        return nullptr;
    }
}

/** The `_int8[]` that holds a Sharp string's bytes (the string itself when it is handed as the array); null when there is none. */
static sharp_object *bytes_of(sharp_object *text) {
    if(text == nullptr) return nullptr;
    if(text->type <= type_var) return text;                     // already the array
    object *data = resolve_field("data", text);
    return data == nullptr ? nullptr : data->o;
}

static sni_array api_text(sni_env *env, sni_ref ref) {
    return array_of(bytes_of((sharp_object*) ref));
}

static sni_ref api_new_text(sni_env *env, const char *chars, int64_t count) {
    if(chars == nullptr || count < 0) count = 0;
    sharp_thread *thread = thread_of(env);
    in_vm here(thread);
    try {
        sharp_object *text = create_object(vm.string_class);
        state_of(thread)->locals.push_back(text);               // (in use before the second allocation can collect)
        if(count > 0) {
            object *data = resolve_field("data", text);
            if(data != nullptr) {
                sharp_object *bytes = create_object((Int) count, type_int8);
                memcpy(bytes->data, chars, (size_t) count);
                copy_object(data, bytes);
            }
        }
        return (sni_ref) text;
    } catch(vm_exception &problem) {
        api_raise(env, "std#out_of_memory_exception", "sni: no memory for a new string");
        return nullptr;
    }
}

static const sni_api the_api = {
    SNI_VERSION,
    api_should_stop,
    api_raise,
    api_pending,
    api_clear,
    api_keep,
    api_drop,
    api_array,
    api_new_array,
    api_text,
    api_new_text,
};

// ---------------------------------------------------------------------------------
// 5. The call
// ---------------------------------------------------------------------------------

void sni_call_native(sharp_function *fun, Int argSize) {
    sharp_thread *thread = thread_self;
    fiber *task = thread->task;
    // a function of an `extern` block is a C function: no glue, the VM hands the arguments over itself
    if(fun->externC != nullptr) { c_call_extern(fun, argSize); return; }
    if(fun->nativeGlue == nullptr) sni_link(fun);

    sni_thread_state *state = state_of(thread);
    bool hasSelf = !check_flag(fun->flags, flag_static);
    if(argSize != fun->paramSize + (hasSelf ? 1 : 0)) {
        throw vm_exception(vm.runtime_except, "native function `" + fun->fullName + "` was called with "
                           + std::to_string(argSize) + " argument(s) on the stack; its declaration has "
                           + std::to_string(fun->paramSize + (hasSelf ? 1 : 0)));
    }

    // ---- 2. pack: the arguments are the top `argSize` slots, the first one lowest
    stack_item *first = task->sp - argSize + 1;
    sni_value small[16];
    std::vector<sni_value> large;
    sni_value *arguments = small;
    if(argSize > 16) {
        large.resize((size_t) argSize);
        arguments = large.data();
    }
    for(Int i = 0; i < argSize; i++) {
        Int parameter = i - (hasSelf ? 1 : 0);
        if(parameter >= 0 && is_number(fun->params[parameter].type, fun->params[parameter].isArray)) {
            if(fun->params[parameter].type->type == type_var) arguments[i].d = first[i].var.d;
            else arguments[i].i = first[i].var.i;
        } else {
            // (the object stays in its stack slot for the whole call: the collector sees it there)
            arguments[i].r = (sni_ref) first[i].obj.o;
        }
    }

    // ---- 3 to 5. the C++ function, with the thread counted as paused
    sni_value result;
    result.i = 0;
    sni_env env;
    env.api = &the_api;
    env.thread = thread;
    size_t localsBefore = state->locals.size();
    sharp_function *outer = state->function;
    state->function = fun;
    state->depth++;
    {
        gc_safe_region native;
        fun->nativeGlue(&env, arguments, &result);
    }                                   // (waits here while a collection is running)
    state->depth--;
    state->function = outer;

    // ---- 6. the result, where a Sharp function leaves its result: the first argument's slot
    task = thread->task;
    bool returns = fun->returnType != nullptr && fun->returnType->type != type_nil;
    if(!returns) {
        task->sp -= argSize;
    } else {
        if(argSize == 0) {
            grow_stack
            stack_overflow_check
            task->sp++;
        } else {
            task->sp -= argSize - 1;
        }
        if(is_number(fun->returnType, fun->arrayReturn)) {
            if(fun->returnType->type == type_var) {
                TC(TC_ST(task->sp) = tk_dec;)
                task->sp->var.d = result.d;
            } else {
                TC(TC_ST(task->sp) = tk_int;)
                task->sp->var.i = result.i;
            }
        } else {
            copy_object(&task->sp->obj, (sharp_object*) result.r);
        }
    }
    state->locals.resize(localsBefore);

    if(state->pending) {
        state->pending = false;
        sharp_class *raised = state->exceptionClass.empty() ? nullptr : locate_class(state->exceptionClass.c_str());
        if(raised == nullptr) {
            throw vm_exception(vm.runtime_except, state->exceptionMessage
                               + (state->exceptionClass.empty() ? "" : " (native code asked for `" + state->exceptionClass + "`, which this program does not have)"));
        }
        throw vm_exception(raised, state->exceptionMessage);
    }
}
