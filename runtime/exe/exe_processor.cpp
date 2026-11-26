//
// Created by bknun on 9/16/2022.
//

#include "../cffi/c_bind.h"
#include "exe_processor.h"
#include "../util/File.h"
#include "../main.h"
#include "exe_macros.h"
#include "manifest_processor.h"
#include "../types/data_type.h"
#include "../virtual_machine.h"
#include "symbol_processor.h"
#include "string_processor.h"
#include "constant_processor.h"
#include "compressed_data_processor.h"
#include "data_processor.h"
#include "meta_data_processor.h"
#include "bytecode_processor.h"
#include "../types/sharp_field.h"

string exeData;
int cursor = -1;

void validate_authenticity() {
    string marker;
    if(next_char() == file_sig) {
        next_chars(marker, 3);

        if(marker == "SEF") {
            cursor += zoffset;

            /* Check file's digital signature */
            if(next_char() == digi_sig1 && next_char() == digi_sig2
               && next_char() == digi_sig3) {
                return;
            }
        }
    }

    throw runtime_error("file `" + executable + "` could not be ran.\n");
}

void process_extern_data(KeyPair<int, string> &result);
void process_debugger_data(KeyPair<int, string> &result);

KeyPair<int, string> process_exe() {
    KeyPair<int, string> result(0, "");

    if(!File::exists(executable.c_str())){
        return result.with(FILE_DOES_NOT_EXIST, "file `" + executable + "` doesnt exist!\n");
    }

    File::read_alltext(executable.c_str(), exeData);
    if(exeData.empty()) {
        return result.with(EMPTY_FILE, "file `" + executable + "` is empty.\n");
    }

    try {
        validate_authenticity();

        process_manifest(result);
        process_symbols(result);
        process_strings(result);
        process_constants(result);
        process_compressed_data(result);
        process_function_data(result);
        process_bytecode(result);
        process_meta_data(result);
        process_extern_data(result);
        process_debugger_data(result);
    } catch(invalid_argument &e) {
        return result.with(CORRUPT_FILE, e.what());
    } catch(overflow_error &e) {
        return result.with(CORRUPT_FILE, e.what());
    } catch(underflow_error &e) {
        return result.with(CORRUPT_FILE, e.what());
    } catch(runtime_error &e) {
        return result.with(CORRUPT_FILE, e.what());
    }

    return result;
}

//
// The extern section: the C functions the program calls (the compiler: backend/finalizer/output,
// write_externs). It stands after the meta section and only in a program that has extern
// functions - an executable that ends with the meta section has none.
//
//      0x3d   then per function:  0x3f, its address (int32), the library, the C function, the signature (texts)
//             per library the project placed:  0x3e, the library, the system ("" for every one), the place (texts)
//             the project's folder:  0x3c, the folder (text)
//      0x1d
//
void process_extern_data(KeyPair<int, string> &result) {
    if((cursor + 1) >= exeData.size() || exeData[cursor + 1] != 0x3d) return;
    next_char();
    for(;;) {
        char tag = next_char();
        if(tag == 0x1d) break;
        if(tag == 0x0a) continue;
        if(tag == 0x3f) {
            int32_t address = next_int32();
            string library, symbol, signature;
            next_string(library);
            next_string(symbol);
            next_string(signature);
            if(address < 0 || (uInt) address >= vm.mf.methods) {
                result.with(CORRUPT_FILE, "file `" + executable + "` may be corrupt: an extern function that is not in the function table.\n");
                throw runtime_error("");
            }
            c_declare(&vm.methods[address], library, symbol, signature);
        } else if(tag == 0x3e) {
            string library, system, place;
            next_string(library);
            next_string(system);
            next_string(place);
#if defined(_WIN32)
            const char *here = "windows";
#elif defined(__APPLE__)
            const char *here = "mac";
#else
            const char *here = "linux";
#endif
            if(system.empty() || system == here) c_project_library(library, place);
        } else if(tag == 0x3c) {
            string folder;
            next_string(folder);
            c_project_folder(folder);
        } else {
            result.with(CORRUPT_FILE, "file `" + executable + "` may be corrupt (its extern section).\n");
            throw runtime_error("");
        }
    }
}

//
// The debugger's section: what sdb (debugger) shows and the VM itself never asks for - the
// names of local variables and where each lives, and which fields are bools (the compiler:
// backend/finalizer/output/debug_locals.sharp, which says what every number means). It stands
// last, and only in a program built with -dbg and without -O.
//
//      0x3b   0x38: the program's names were replaced (-obf);  0x37: it was optimized (-O)
//             per function:  0x3a, its address (int32), how many variables follow (int32),
//                 per variable: its name (text), kind, first line, last line, slot (int32 each),
//                 how many steps follow (int32) and each step (int32), field (int32)
//             per class:  0x39, its address (int32), how many fields follow (int32),
//                 per field: its place among the class's fields, kind (int32 each)
//      0x1d
//
static void corrupt_debugger_section(KeyPair<int, string> &result) {
    result.with(CORRUPT_FILE, "file `" + executable + "` may be corrupt (its debugger section).\n");
    throw runtime_error("");
}

void process_debugger_data(KeyPair<int, string> &result) {
    if((cursor + 1) >= exeData.size() || exeData[cursor + 1] != 0x3b) return;
    next_char();
    for(;;) {
        char tag = next_char();
        if(tag == 0x1d) break;
        if(tag == 0x0a) continue;
        if(tag == 0x38) { vm.mf.obfuscated = true; continue; }
        if(tag == 0x37) { vm.mf.optimized = true; continue; }

        int32_t address = next_int32();
        int32_t count = next_int32();
        if(address < 0 || count < 0) corrupt_debugger_section(result);

        if(tag == 0x3a) {
            if((uInt) address >= vm.mf.methods || vm.methods[address].locals != nullptr) corrupt_debugger_section(result);

            sharp_function *function = &vm.methods[address];
            function->locals = new local_data[count];
            function->localCount = count;
            for(int32_t i = 0; i < count; i++) {
                local_data &local = function->locals[i];
                next_string(local.name);
                local.kind = next_int32();
                local.firstLine = next_int32();
                local.lastLine = next_int32();
                local.slot = next_int32();
                local.stepCount = next_int32();
                if(local.stepCount < 0 || local.stepCount > 1000) corrupt_debugger_section(result);
                local.steps = local.stepCount > 0 ? new int32_t[local.stepCount] : nullptr;
                for(int32_t step = 0; step < local.stepCount; step++) local.steps[step] = next_int32();
                local.field = next_int32();
            }
        } else if(tag == 0x39) {
            if((uInt) address >= vm.mf.classes) corrupt_debugger_section(result);

            sharp_class *of = &vm.classes[address];
            for(int32_t i = 0; i < count; i++) {
                int32_t place = next_int32();
                int32_t kind = next_int32();
                if(place < 0 || place >= of->totalFieldCount) corrupt_debugger_section(result);
                of->fields[place].debugKind = kind;
            }
        } else {
            corrupt_debugger_section(result);
        }
    }
}

void check_section(int section_type, Int error_code, KeyPair<int, string> &result) {
    if(next_char() != section_type){
        result.with(error_code, "file `" + executable + "` may be corrupt");
        throw runtime_error("");
    }
}

char next_char() {
    if((cursor + 1) < exeData.size())
        return exeData[++cursor];
    else {
        throw overflow_error("buffer overflow, expected data at end of exe file");
    }
}

char current_char() {
    if(cursor >= 0)
        return exeData[cursor];
    else {
        throw underflow_error("buffer underflow, exe file not processed yet");
    }
}


void next_chars(string &str, size_t len) {
    while(len > 0) {
        str += next_char();
        len--;
    }
}

void next_string(string &str) {
    while(next_char() != nil) {
        str += current_char();
    }
}

int32_t next_int32() {
    string num;
    next_chars(num, 4);
    return SET_i32(num[0], num[1], num[2], num[3]);
}

Int parse_int() {
    string s;
    next_string(s);
    return strtoll(s.c_str(), NULL, 0);        // (64 bits on every machine: `long` is 32 on a 32-bit CPU and on Windows)
}

sharp_type *next_type() {
    data_type type = (data_type)next_int32();
    if((type >= type_int8 && type <= type_uint64)
       || type == type_var || type == type_object
       || type == type_nil) {
        return &vm.nativeTypes[type];
    } else if(type == type_class) {
        sharp_type *t = new sharp_type();
        t->sc = &vm.classes[next_int32()];
        t->type = type_class;

        return t;
    } else if(type == type_function_ptr) {
        sharp_type *t = new sharp_type();
        t->fun = &vm.methods[vm.mf.methods + next_int32()];
        t->type = type_function_ptr;
        return t;
    } else {
        throw invalid_argument("invalid type found in symbol table");
    }
}

