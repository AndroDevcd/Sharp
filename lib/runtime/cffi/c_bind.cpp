//
// EXTERN FUNCTIONS IN THE VM (see c_bind.h)
//
// A CALL, START TO END (c_call_extern)
//
//      ticks := SDL_GetTicks();
//
//   1. LINK, the first time: open the library (the places c_bind.h lists), find the C function by
//      its name, turn the signature text into the plan for this machine's calling convention.
//      Each of the three can fail, and each failure is an unsatisfied_link_error that says what was
//      looked for and where.
//   2. TAKE the arguments off the VM's stack, each as the signature says: a number, a decimal, an
//      address, a string (copied, with the 0 C wants at its end), an array (its items, in place).
//   3. CALL, with the thread counted as paused for the garbage collector - the same rule as for a
//      native function (sni_runtime.cpp), and the same bookkeeping: the thread knows which function
//      it is in, so a crash inside C can be told from a crash of the VM (sig_handler.cpp).
//   4. The result goes where a Sharp function leaves its result.
//
// WHAT C IS HANDED STAYS PUT. The collector never moves an object, and an array handed to C sits
// in its stack slot for the whole call, so its items cannot go away under C. A string is copied:
// C gets its own bytes, with the 0 at the end, for the length of the call.
//
#include "c_bind.h"
#include "c_call.h"
#include "../sni/sni_runtime.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../memory/tracing_collector.h"
#include "../memory/garbage_collector.h"
#include "../types/sharp_field.h"
#include "../types/sharp_class.h"
#include "../memory/type_check.h"
#include "../reflect/reflect_helpers.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/fiber/fiber.h"
#include "../../core/access_flag.h"
#include "../../core/opcode/opcode_macros.h"
#include "../../util/File.h"
#include <mutex>
#include <cstring>
#include <cstdio>
#include <deque>

#ifdef _WIN32
#include <windows.h>
#define lib_open(path) ((void*) LoadLibraryA((path).c_str()))
#define lib_symbol(handle, name) ((void*) GetProcAddress((HMODULE) (handle), name))
#else
#include <dlfcn.h>
#define lib_open(path) dlopen((path).c_str(), RTLD_NOW | RTLD_GLOBAL)
#define lib_symbol(handle, name) dlsym(handle, name)
#endif

extern string executable;

// ---------------------------------------------------------------------------------
// 1. The libraries
// ---------------------------------------------------------------------------------

struct c_library {
    std::string name;
    std::vector<std::string> said;          // by the program (c_library): tried first
    std::vector<std::string> project;       // by the project's settings.json
    void *handle = nullptr;
    std::string file;                       // what was opened
};

static std::recursive_mutex bind_mutex;
static std::deque<c_library> libraries;     // (a deque: a library's address stays the same when another is added)
static std::string project_folder;

static c_library &library_named(const std::string &name) {
    for(c_library &library : libraries)
        if(library.name == name) return library;
    libraries.emplace_back();
    libraries.back().name = name;
    return libraries.back();
}

void c_project_library(const std::string &library, const std::string &place) {
    std::lock_guard<std::recursive_mutex> guard(bind_mutex);
    library_named(library).project.push_back(place);
}

void c_project_folder(const std::string &folder) {
    std::lock_guard<std::recursive_mutex> guard(bind_mutex);
    project_folder = folder;
}

void c_set_library(const std::string &library, const std::string &place) {
    std::lock_guard<std::recursive_mutex> guard(bind_mutex);
    c_library &found = library_named(library);
    if(found.handle != nullptr) {
        throw vm_exception(vm.ill_state_except, "c_library(\"" + library + "\", \"" + place + "\"): the library is already open, from `"
            + found.file + "`.\n    Say where a library is before the first call of one of its functions.");
    }
    found.said.push_back(place);
}

static bool is_absolute(const std::string &path) {
    if(path.empty()) return false;
    if(path[0] == '/' || path[0] == '\\') return true;
    return path.size() > 2 && path[1] == ':' && (path[2] == '/' || path[2] == '\\');
}

static bool has_folder(const std::string &path) {
    return path.find_first_of("/\\") != std::string::npos;
}

/** The folder a file is in, with the separator at its end ("" for a bare name). */
static std::string folder_of(const std::string &path) {
    size_t cut = path.find_last_of("/\\");
    return cut == std::string::npos ? std::string() : path.substr(0, cut + 1);
}

/** `place` as it is and with the endings and the `lib` this system's libraries have. */
static void spellings(const std::string &place, std::vector<std::string> &into) {
#ifdef _WIN32
    const char *ending = ".dll";
#elif defined(__APPLE__)
    const char *ending = ".dylib";
#else
    const char *ending = ".so";
#endif
    std::string folder = folder_of(place), file = place.substr(folder.size());
    bool has_ending = file.find(ending) != std::string::npos;
    if(has_ending) { into.push_back(place); return; }
    into.push_back(folder + file + ending);
    if(file.compare(0, 3, "lib") != 0) into.push_back(folder + "lib" + file + ending);
    into.push_back(place);
}

/** Every file to try for one place a library was said to be, in order. */
static void files_for(const std::string &place, std::vector<std::string> &into) {
    if(is_absolute(place)) { spellings(place, into); return; }

    std::string program;
    {
        std::string path = executable, whole;
        File::resolvePath(path, whole);
        program = folder_of(whole.empty() ? path : whole);
    }
    // beside the program, in the project, in the folder the program was started in
    if(!program.empty()) spellings(program + place, into);
    if(!project_folder.empty()) {
        std::string folder = project_folder;
        if(folder.back() != '/' && folder.back() != '\\') folder += '/';
        spellings(folder + place, into);
    }
    spellings("./" + place, into);
    // ... and, for a name without a folder, wherever the system looks for libraries
    if(!has_folder(place)) spellings(place, into);
}

/** C's own library: the functions every C program has. */
static void *open_c_runtime(std::string &file) {
#ifdef _WIN32
    for(const char *name : { "ucrtbase.dll", "msvcrt.dll" }) {
        void *handle = (void*) LoadLibraryA(name);
        if(handle != nullptr) { file = name; return handle; }
    }
    return nullptr;
#else
    file = "the C library of this process";
    return dlopen(nullptr, RTLD_NOW | RTLD_GLOBAL);
#endif
}

static std::string why_not_opened() {
#ifdef _WIN32
    DWORD code = GetLastError();
    if(code == ERROR_BAD_EXE_FORMAT) return "it is not a 64-bit Windows library";
    if(code == ERROR_MOD_NOT_FOUND) return "a library IT needs was not found";
    return "Windows error " + std::to_string((long long) code);
#else
    const char *said = dlerror();
    return said == nullptr ? "no reason given" : said;
#endif
}

static bool file_is_there(const std::string &path) {
    FILE *file = fopen(path.c_str(), "rb");
    if(file == nullptr) return false;
    fclose(file);
    return true;
}

/** Open `library`. Throws an unsatisfied_link_error that lists every file that was tried. */
static void open_library(c_library &library, const std::string &wanted_by) {
    if(library.handle != nullptr) return;
    if(library.name == "c" && library.said.empty() && library.project.empty()) {
        library.handle = open_c_runtime(library.file);
        if(library.handle != nullptr) return;
    }

    std::vector<std::string> files;
    for(const std::string &place : library.said) files_for(place, files);
    for(const std::string &place : library.project) files_for(place, files);
    if(library.said.empty() && library.project.empty()) files_for(library.name, files);

    std::string looked, refused;
    for(const std::string &file : files) {
        void *handle = lib_open(file);
        if(handle != nullptr) {
            library.handle = handle;
            library.file = file;
            return;
        }
        std::string why = why_not_opened();
        if(refused.empty() && file_is_there(file)) refused = "\n    `" + file + "` is there, but the system would not open it: " + why;
        looked += "\n        " + file;
    }
    throw vm_exception(vm.unsatisfied_link_except,
        "the C library \"" + library.name + "\" could not be opened (it is needed for `" + wanted_by + "`)." + refused +
        "\n    Looked for:" + looked +
        "\n    Say where it is before the first call:  c_library(\"" + library.name + "\", \"path/to/the/file\");"
        "\n    or in the project's settings.json:  \"c_libraries\": { \"" + library.name + "\": { \"windows\": \"...\", \"linux\": \"...\" } }");
}

// ---------------------------------------------------------------------------------
// 2. One extern function
// ---------------------------------------------------------------------------------

enum { ROLE_WHOLE, ROLE_DECIMAL, ROLE_ADDRESS, ROLE_TEXT, ROLE_ITEMS, ROLE_NOTHING, ROLE_STRUCT, ROLE_STRUCT_POINTER };

// A STRUCT IS AN OBJECT IN SHARP AND BYTES IN C. A class based on c_struct has the struct's members
// as its fields; for the length of a call the VM lays them out as C does (PACK), and afterwards reads
// back what C wrote (UNPACK). The object is what Sharp keeps; the bytes live only during the call.

/** One member of a struct: the field of the Sharp class it comes from, and where it sits in C's bytes. */
struct c_shape_field {
    std::string name;
    c_kind kind;
    uint32_t offset;
    int inner;                              // a struct inside the struct: its shape (an index into the binding's shapes); else -1
};

struct c_shape {
    uint32_t size = 0;
    std::vector<c_shape_field> fields;
};

static uint32_t rounded(uint32_t value, uint32_t to) { return (value + to - 1) / to * to; }

/** The shape of "{x:i32 at:{x:f32 y:f32}}", put into `shapes`: its index, or -1 and `why`. */
static int parse_shape(const char *&p, std::deque<c_shape> &shapes, std::string &why) {
    if(*p != '{') { why = "a struct was expected"; return -1; }
    p++;
    shapes.emplace_back();
    int index = (int) shapes.size() - 1;
    uint32_t size = 0, align = 1;
    while(true) {
        while(*p == ' ' || *p == ',') p++;
        if(*p == '}') { p++; break; }
        if(*p == 0) { why = "a `{` is never closed"; return -1; }
        c_shape_field field;
        field.inner = -1;
        while(*p != 0 && *p != ':' && *p != ' ' && *p != '}') field.name += *p++;
        if(*p != ':') { why = "a member of a struct an object is laid out as carries its name:  x:i32"; return -1; }
        p++;
        const char *start = p;
        if(*p == '{') {
            field.inner = parse_shape(p, shapes, why);
            if(field.inner < 0) return -1;
        } else {
            while(*p != 0 && *p != ' ' && *p != ',' && *p != '}') p++;
        }
        c_type member;
        if(!c_parse_type(std::string(start, p), member, why)) return -1;
        field.kind = member.kind;
        field.offset = rounded(size, member.align);
        size = field.offset + member.size;
        if(member.align > align) align = member.align;
        shapes[(size_t) index].fields.push_back(field);
    }
    shapes[(size_t) index].size = rounded(size, align);
    return index;
}

/** The field `name` of the class `o` is an object of (not a static one); null when it has none. */
static sharp_field *field_named(sharp_object *o, const std::string &name) {
    sharp_class *of = &vm.classes[CLASS(o->info)];
    for(Int i = 0; i < of->totalFieldCount; i++) {
        sharp_field &field = of->fields[i];
        if(field.name == name && !check_flag(field.flags, flag_static)) return &field;
    }
    return nullptr;
}

static sharp_field *member_field(sharp_object *o, const c_shape_field &member) {
    sharp_field *field = field_named(o, member.name);
    if(field == nullptr) {
        throw vm_exception(vm.runtime_except, "the struct class `" + vm.classes[CLASS(o->info)].fullName + "` has no field `" + member.name
                           + "`: the object is not of the class the extern function was declared with");
    }
    return field;
}

/** PACK: the object's fields, laid out as C's bytes. */
static void pack(const std::deque<c_shape> &shapes, int index, sharp_object *o, uint8_t *bytes) {
    const c_shape &shape = shapes[(size_t) index];
    for(const c_shape_field &member : shape.fields) {
        sharp_field *field = member_field(o, member);
        uint8_t *to = bytes + member.offset;
        if(member.inner >= 0) {
            sharp_object *inner = o->node[field->address].o;
            if(inner != nullptr) pack(shapes, member.inner, inner, to);     // (a struct that is not there: all zero)
            continue;
        }
        vm_cell cell = number_slot(o, field->address);
        bool decimal = field->type->type == type_var;
        int64_t whole = decimal ? (int64_t) cell.d : (int64_t) cell.i;
        double fraction = decimal ? cell.d : (double) cell.i;
        switch(member.kind) {
            case C_I8: case C_U8: { uint8_t v = (uint8_t) whole; memcpy(to, &v, 1); break; }
            case C_I16: case C_U16: { uint16_t v = (uint16_t) whole; memcpy(to, &v, 2); break; }
            case C_I32: case C_U32: { uint32_t v = (uint32_t) whole; memcpy(to, &v, 4); break; }
            case C_I64: case C_U64: { memcpy(to, &whole, 8); break; }
            case C_F32: { float v = (float) fraction; memcpy(to, &v, 4); break; }
            case C_F64: { memcpy(to, &fraction, 8); break; }
            case C_PTR: { uintptr_t v = (uintptr_t) whole; memcpy(to, &v, sizeof(v)); break; }
            default: break;
        }
    }
}

/** UNPACK: C's bytes, back into the object's fields. */
static void unpack(const std::deque<c_shape> &shapes, int index, const uint8_t *bytes, sharp_object *o) {
    const c_shape &shape = shapes[(size_t) index];
    for(const c_shape_field &member : shape.fields) {
        sharp_field *field = member_field(o, member);
        const uint8_t *from = bytes + member.offset;
        if(member.inner >= 0) {
            object *slot = &o->node[field->address];
            if(slot->o == nullptr) {
                if(field->type == nullptr || field->type->sc == nullptr) continue;
                copy_object(slot, create_object(field->type->sc));
            }
            unpack(shapes, member.inner, from, slot->o);
            continue;
        }
        int64_t whole = 0;
        double fraction = 0;
        bool is_fraction = false;
        switch(member.kind) {
            case C_I8: { int8_t v; memcpy(&v, from, 1); whole = v; break; }
            case C_U8: { uint8_t v; memcpy(&v, from, 1); whole = v; break; }
            case C_I16: { int16_t v; memcpy(&v, from, 2); whole = v; break; }
            case C_U16: { uint16_t v; memcpy(&v, from, 2); whole = v; break; }
            case C_I32: { int32_t v; memcpy(&v, from, 4); whole = v; break; }
            case C_U32: { uint32_t v; memcpy(&v, from, 4); whole = v; break; }
            case C_I64: case C_U64: { memcpy(&whole, from, 8); break; }
            case C_F32: { float v; memcpy(&v, from, 4); fraction = v; is_fraction = true; break; }
            case C_F64: { memcpy(&fraction, from, 8); is_fraction = true; break; }
            case C_PTR: { uintptr_t v = 0; memcpy(&v, from, sizeof(v)); whole = (int64_t) v; break; }
            default: break;
        }
        if(field->type->type == type_var) number_slot(o, field->address).d = is_fraction ? fraction : (double) whole;
        else number_slot(o, field->address).i = is_fraction ? (int64_t) fraction : whole;
    }
}

struct c_binding {
    sharp_function *function = nullptr;
    std::string library, symbol, text;
    c_signature signature;
    std::vector<uint8_t> roles;             // what Sharp hands over for each argument
    uint8_t result_role = ROLE_NOTHING;
    std::deque<c_shape> shapes;             // the structs among the arguments and the result ...
    std::vector<int> arg_shapes;            // ... which one each argument is (-1: not a struct)
    int result_shape = -1;
    void *address = nullptr;
    c_library *from = nullptr;
    bool linked = false;
};

static std::deque<c_binding> bindings;

void c_declare(sharp_function *fun, const std::string &library, const std::string &symbol, const std::string &signature) {
    std::lock_guard<std::recursive_mutex> guard(bind_mutex);
    bindings.emplace_back();
    c_binding &binding = bindings.back();
    binding.function = fun;
    binding.library = library;
    binding.symbol = symbol;
    binding.text = signature;
    fun->externC = &binding;
}

/** What Sharp hands over for a type written like this in a signature. */
static uint8_t role_of(const c_type &type) {
    if(type.kind == C_VOID) return ROLE_NOTHING;
    if(type.kind == C_STRUCT) return ROLE_STRUCT;
    if(!type.text.empty() && type.text[0] == '*') return ROLE_STRUCT_POINTER;
    if(type.text == "cstr") return ROLE_TEXT;
    if(type.text == "buf") return ROLE_ITEMS;
    if(type.kind == C_PTR) return ROLE_ADDRESS;
    if(type.kind == C_F32 || type.kind == C_F64) return ROLE_DECIMAL;
    return ROLE_WHOLE;
}

static void link(c_binding &binding) {
    std::lock_guard<std::recursive_mutex> guard(bind_mutex);
    if(binding.linked) return;
    sharp_function *fun = binding.function;

    std::string why;
    if(!c_parse_signature(binding.text, binding.signature, why)) {
        throw vm_exception(vm.unsatisfied_link_except,
            "the extern function `" + fun->fullName + "` cannot be called on this system (" + std::string(c_convention()) + "): " + why +
            "\n    its signature: " + binding.text);
    }
    binding.roles.clear();
    binding.arg_shapes.clear();
    binding.shapes.clear();
    auto shape_of = [&](const c_type &type) -> int {
        uint8_t role = role_of(type);
        if(role != ROLE_STRUCT && role != ROLE_STRUCT_POINTER) return -1;
        const char *p = type.text.c_str() + (role == ROLE_STRUCT_POINTER ? 1 : 0);
        int shape = parse_shape(p, binding.shapes, why);
        if(shape < 0) throw vm_exception(vm.unsatisfied_link_except, "the extern function `" + fun->fullName + "`, the struct " + type.text + ": " + why);
        return shape;
    };
    for(const c_type &type : binding.signature.args) {
        binding.roles.push_back(role_of(type));
        binding.arg_shapes.push_back(shape_of(type));
    }
    binding.result_role = role_of(binding.signature.result);
    binding.result_shape = shape_of(binding.signature.result);
    if((Int) binding.signature.args.size() != fun->paramSize) {
        throw vm_exception(vm.unsatisfied_link_except, "the extern function `" + fun->fullName + "` has " + std::to_string(fun->paramSize)
            + " parameter(s) and its signature " + binding.text + " has " + std::to_string(binding.signature.args.size()));
    }

    c_library &library = library_named(binding.library);
    open_library(library, fun->fullName);
    void *address = lib_symbol(library.handle, binding.symbol.c_str());
    if(address == nullptr) {
        throw vm_exception(vm.unsatisfied_link_except,
            "the C library \"" + library.name + "\" (" + library.file + ") has no function `" + binding.symbol + "` (declared as `" + fun->fullName + "`)."
            "\n    The name in the `extern` block must be the C function's own name, letter for letter."
            "\n    A function written in C++ has another name inside the library unless it is declared `extern \"C\"`,"
            "\n    and on Windows a library only hands out the functions it exports.");
    }
    binding.address = address;
    binding.from = &library;
    binding.linked = true;
}

void c_link(sharp_function *fun) {
    if(fun != nullptr && fun->externC != nullptr) link(*fun->externC);
}

std::string c_link_all() {
    std::string missing;
    for(c_binding &binding : bindings) {
        try { link(binding); }
        catch(vm_exception &problem) { missing += (missing.empty() ? "" : "\n") + std::string(problem.what()); }
    }
    return missing;
}

// ---------------------------------------------------------------------------------
// 3. The call
// ---------------------------------------------------------------------------------

/** The `_int8[]` that holds a Sharp string's bytes (the string itself when it is handed as the array); null when there is none. */
static sharp_object *bytes_of(sharp_object *text) {
    if(text == nullptr) return nullptr;
    if(text->type <= type_var) return text;
    object *data = resolve_field("data", text);
    return data == nullptr ? nullptr : data->o;
}

/** What a thread that is inside a C function was handed (for the crash report). */
struct c_running {
    c_binding *binding = nullptr;
    const c_value *values = nullptr;
    Int item_counts[C_MAX_ARGUMENTS];
};
static thread_local c_running running;

void c_call_extern(sharp_function *fun, Int argSize) {
    sharp_thread *thread = thread_self;
    fiber *task = thread->task;
    c_binding &binding = *fun->externC;
    if(!binding.linked) link(binding);

    if(argSize != fun->paramSize) {
        throw vm_exception(vm.runtime_except, "extern function `" + fun->fullName + "` was called with " + std::to_string(argSize)
                           + " argument(s) on the stack; its declaration has " + std::to_string(fun->paramSize));
    }

    // ---- 2. the arguments: the top `argSize` slots, the first one lowest
    stack_item *first = task->sp - argSize + 1;
    c_value values[C_MAX_ARGUMENTS];
    std::deque<std::string> texts;              // the C strings, alive until the call is over
    std::deque<std::vector<uint8_t> > structs;  // the structs as C's bytes, likewise
    uint8_t *packed[C_MAX_ARGUMENTS];           // ... which of them each argument is (null: not a struct, or a null one)
    c_running here;
    here.binding = &binding;
    here.values = values;
    for(Int i = 0; i < argSize; i++) {
        here.item_counts[i] = -1;
        packed[i] = nullptr;
        switch(binding.roles[(size_t) i]) {
            case ROLE_STRUCT:
            case ROLE_STRUCT_POINTER: {
                sharp_object *o = first[i].obj.o;
                if(o == nullptr) {
                    if(binding.roles[(size_t) i] == ROLE_STRUCT) {
                        throw vm_exception(vm.nullptr_except, "argument " + std::to_string((long long) i + 1) + " of the extern function `" + fun->fullName
                            + "` is a struct handed over by value, and it is null");
                    }
                    values[i].p = nullptr;      // a pointer to a struct may be NULL
                    break;
                }
                int shape = binding.arg_shapes[(size_t) i];
                structs.emplace_back((size_t) binding.shapes[(size_t) shape].size + 8, (uint8_t) 0);
                packed[i] = structs.back().data();
                pack(binding.shapes, shape, o, packed[i]);
                values[i].p = packed[i];
                break;
            }
            case ROLE_DECIMAL: values[i].d = first[i].var.d; break;
            case ROLE_ADDRESS: values[i].p = (void*) (uintptr_t) first[i].var.i; break;
            case ROLE_TEXT: {
                sharp_object *bytes = bytes_of(first[i].obj.o);
                if(first[i].obj.o == nullptr) { values[i].p = nullptr; break; }
                texts.emplace_back();
                if(bytes != nullptr && bytes->size > 0) {
                    // (a text's characters are numbers of the array's own width: one byte each is what C gets)
                    std::string &text = texts.back();
                    text.resize((size_t) bytes->size);
                    for(Int k = 0; k < (Int) bytes->size; k++) text[(size_t) k] = (char) integer_at(bytes, k);
                }
                values[i].p = (void*) texts.back().c_str();
                break;
            }
            case ROLE_ITEMS: {
                sharp_object *items = first[i].obj.o;
                values[i].p = items == nullptr || items->size == 0 ? nullptr : (void*) items->data;
                here.item_counts[i] = items == nullptr ? 0 : (Int) items->size;
                break;
            }
            default: values[i].i = first[i].var.i; break;
        }
    }

    // ---- 3. C, with the thread counted as paused
    c_value result;
    result.i = 0;
    std::vector<uint8_t> result_bytes;
    uint8_t *struct_result = nullptr;
    if(binding.result_role == ROLE_STRUCT) {
        result_bytes.assign((size_t) binding.signature.result.size + 8, (uint8_t) 0);
        struct_result = result_bytes.data();
    }
    sni_thread_state *state = sni_state_of(thread);
    sharp_function *outer = state->function;
    c_running outer_running = running;
    state->function = fun;
    state->depth++;
    running = here;
    {
        gc_safe_region native;
        c_invoke(binding.signature, binding.address, values, &result, struct_result);
    }                                   // (waits here while a collection is running)
    running = outer_running;
    state->depth--;
    state->function = outer;

    // what C wrote through a pointer to a struct goes back into the object (it is still in its stack slot)
    task = thread->task;
    first = task->sp - argSize + 1;
    for(Int i = 0; i < argSize; i++) {
        if(binding.roles[(size_t) i] == ROLE_STRUCT_POINTER && packed[i] != nullptr)
            unpack(binding.shapes, binding.arg_shapes[(size_t) i], packed[i], first[i].obj.o);
    }

    // ---- 4. the result, where a Sharp function leaves its result: the first argument's slot
    task = thread->task;
    if(binding.result_role == ROLE_NOTHING) {
        task->sp -= argSize;
        return;
    }
    if(argSize == 0) {
        grow_stack
        stack_overflow_check
        task->sp++;
    } else {
        task->sp -= argSize - 1;
    }
    switch(binding.result_role) {
        case ROLE_STRUCT:
        case ROLE_STRUCT_POINTER: {
            // a struct that comes back is a new object of the class the function is declared to return;
            // through a pointer it is a COPY of what the pointer points to (NULL is null)
            const uint8_t *bytes = binding.result_role == ROLE_STRUCT ? struct_result : (const uint8_t*) result.p;
            sharp_class *of = fun->returnType == nullptr ? nullptr : fun->returnType->sc;
            if(bytes == nullptr || of == nullptr) { copy_object(&task->sp->obj, (sharp_object*) nullptr); break; }
            sharp_object *made = create_object(of);
            copy_object(&task->sp->obj, made);
            unpack(binding.shapes, binding.result_shape, bytes, made);
            break;
        }
        case ROLE_DECIMAL:
            TC(TC_ST(task->sp) = tk_dec;)
            task->sp->var.d = result.d;
            break;
        case ROLE_ADDRESS:
            TC(TC_ST(task->sp) = tk_int;)
            task->sp->var.i = (Int) (uintptr_t) result.p;
            break;
        case ROLE_TEXT: {
            // a C string that comes back is copied into a Sharp string (C keeps its own); NULL is null
            const char *chars = (const char*) result.p;
            if(chars == nullptr) { copy_object(&task->sp->obj, (sharp_object*) nullptr); break; }
            size_t count = strlen(chars);
            sharp_object *text = create_object(vm.string_class);
            copy_object(&task->sp->obj, text);              // (in use before the second allocation can collect)
            if(count > 0) {
                object *data = resolve_field("data", text);
                if(data != nullptr) {
                    sharp_object *bytes = create_object((Int) count, type_int8);
                    copy_object(data, bytes);
                    for(size_t k = 0; k < count; k++) set_integer_at(bytes, (Int) k, chars[k]);
                }
            }
            break;
        }
        default:
            TC(TC_ST(task->sp) = tk_int;)
            task->sp->var.i = result.i;
            break;
    }
}

// ---------------------------------------------------------------------------------
// 4. For the crash report
// ---------------------------------------------------------------------------------

std::string c_describe_call(sharp_thread *thread) {
    if(thread == nullptr || !sni_in_native(thread) || running.binding == nullptr) return "";
    c_binding &binding = *running.binding;
    char number[64];
    std::string text = "crashed inside the C function `" + binding.symbol + "`";
    text += " (library \"" + binding.library + "\"" + (binding.from == nullptr ? "" : ": " + binding.from->file) + ")";
    text += "\n    declared as `" + binding.function->fullName + "`, signature " + binding.text;
    text += "\n    called with (";
    std::string suspect;
    for(size_t i = 0; i < binding.roles.size(); i++) {
        if(i > 0) text += ", ";
        const c_value &value = running.values[i];
        switch(binding.roles[i]) {
            case ROLE_DECIMAL: snprintf(number, sizeof(number), "%g", value.d); text += number; break;
            case ROLE_TEXT:
                if(value.p == nullptr) { text += "null"; if(suspect.empty()) suspect = "argument " + std::to_string(i + 1) + " is a null string"; }
                else text += "a string of " + std::to_string(strlen((const char*) value.p)) + " characters";
                break;
            case ROLE_ITEMS:
                if(value.p == nullptr) { text += running.item_counts[i] == 0 ? "an array with no items" : "null"; if(suspect.empty()) suspect = "argument " + std::to_string(i + 1) + " is null or an empty array: C was handed the address 0"; }
                else text += "an array of " + std::to_string((long long) running.item_counts[i]) + " items";
                break;
            case ROLE_STRUCT: text += "a struct"; break;
            case ROLE_STRUCT_POINTER: text += value.p == nullptr ? "null" : "a pointer to a struct"; break;
            case ROLE_ADDRESS:
                snprintf(number, sizeof(number), "address 0x%llx", (unsigned long long) (uintptr_t) value.p);
                text += number;
                if(value.p == nullptr && suspect.empty()) suspect = "argument " + std::to_string(i + 1) + " is the address 0";
                break;
            default: text += std::to_string((long long) value.i); break;
        }
    }
    text += ")";
    text += "\n  = note: the crash is in C code, not in the Sharp VM. The usual causes: a declaration that does not match the"
            "\n          C function (a wrong type, a parameter too few or too many), an address C may not use, an array"
            "\n          smaller than C was told.";
    if(!suspect.empty()) text += "\n  = here: " + suspect;
    return text;
}
