//
// Created by bknun on 8/8/2022.
//

#include "generator.h"
#include "../optimization/optimizer.h"
#include "file_generator.h"
#include "../../../compiler_info.h"
#include <random>
#include "code/code_info.h"
#include "function_generator.h"
#include "code/code_context.h"
#include "../../../../util/File.h"
#include "native_generator.h"

Int UUIDGenerator = 0;
const int range_from  = 0;
const int range_to    = 10000000;
std::random_device                  rand_dev;
std::mt19937                        generator(std::chrono::duration_cast<std::chrono::milliseconds>
                                                      (std::chrono::high_resolution_clock::now().time_since_epoch()).count());
std::uniform_int_distribution<int>  distr(range_from, range_to);

List<sharp_function*> compressedCompilationFunctions;
List<sharp_class*> compressedCompilationClasses;
List<double> constantMap;
List<string> stringMap;

void offsetUUIDGenerator()
{
    UUIDGenerator = uniqueId + distr(generator);
}

bool is_greater_than(sharp_function* e1, sharp_function* e2) {
    return e1->ci->address > e2->ci->address;
}

bool is_greater_than(sharp_class* e1, sharp_class* e2) {
    return e1->ci->address > e2->ci->address;
}

void print_field_addresses(sharp_class *sc, bool isStatic, stringstream &ss) {
    if(sc->baseClass && !isStatic) {
        print_field_addresses(sc->baseClass, isStatic, ss);
    }

    for(Int i = 0; i < sc->fields.size(); i++) {
        auto sf = sc->fields.get(i);

        if(sf->used && check_flag(sf->flags, flag_static) == isStatic) {
            ss << "\t [" << sf->ci->address << "]";

            if(isStatic) ss << " static";
            ss << " : " << sf->fullName << endl;
        }
    }
}

void pre_generate_addresses() {
    offsetUUIDGenerator();

    for(Int i = 0; i < compressedCompilationFiles.size(); i++) {
        generate_addresses(compressedCompilationFiles.get(i));
    }
}

void generate() {
    for(Int i = 0; i < compressedCompilationFunctions.size(); i++) {
        generate(compressedCompilationFunctions.get(i));
    }

    application_id = abs(distr(generator));
//    remove_delegate_functions();
    compressedCompilationFunctions.linearSort(is_greater_than);
    compressedCompilationClasses.linearSort(is_greater_than);

    stringstream ss;
    for(Int i = 0; i < compressedCompilationFunctions.size(); i++) {
        auto sf = compressedCompilationFunctions.get(i);
        ss << function_to_str(sf) << endl;
        ss << "address: " << sf->ci->address << endl;
        ss << "file: " << get_true_source_file(sf)->name << endl;
        ss << "owner: " << sf->owner->fullName << endl;
        ss << "frameStackOffset: " << sf->ci->frameStackOffset << endl;
        ss << "fpOffset: " << sf->ci->fpOffset << endl;
        ss << "spOffset: " << sf->ci->spOffset << endl;
        ss << "stackSize: " << sf->ci->stackSize;
        ss << "\ncode:\n\n" << code_to_string(sf->ci) << endl;
    }

    ss << endl << "classes:\n";
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        ss << "[" << i << "]: " << compressedCompilationClasses.get(i)->fullName << " - "
            << compressedCompilationClasses.get(i)->ci->address << endl;

        print_field_addresses(compressedCompilationClasses.get(i), true, ss);
        print_field_addresses(compressedCompilationClasses.get(i), false, ss);
    }

    ss << endl << endl << "constants:\n";
    for(Int i = 0; i < constantMap.size(); i++) {
        ss << "[" << i << "]: " << constantMap.get(i) << endl;
    }

    ss << endl << endl << "strings:\n";
    for(Int i = 0; i < stringMap.size(); i++) {
        ss << "[" << i << "]: " << stringMap.get(i) << endl;
    }

    if(options.compile_mode == project_mode) {
        string div;

#ifdef __WIN32
        div = "\\";
#else
        div = "/";
#endif

        string buildDir = options.project_dir + div + "build";
        string outFile = buildDir + div + options.out + ".aout";

        File::makeDir(buildDir);
        if (File::write(outFile.c_str(), ss.str())) {
            cout << PROG_NAME << ": error: failed to write out to executable " << outFile << endl;
        }
    } else {
        string outFile = options.out + ".aout";
        if (File::write(outFile.c_str(), ss.str())) {
            cout << PROG_NAME << ": error: failed to write out to executable " << options.out << endl;
        }
    }

    generate_native_code();
    generate_exe();
}

void generation_error(string message) {

    cout << PROG_NAME << ": <code-generator> fatal error: " << message << endl;
    if(cc.container) {
        cout << "note: in function: " << function_to_str(cc.container) << endl;
        cout << "address: " << cc.container->ci->address << endl;
        cout << "file: " << get_true_source_file(cc.container)->name << endl;
        cout << "owner: " << cc.container->owner->fullName << endl;
        cout << "frameStackOffset: " << cc.container->ci->frameStackOffset << endl;
        cout << "fpOffset: " << cc.container->ci->fpOffset << endl;
        cout << "spOffset: " << cc.container->ci->spOffset << endl;
        cout << "stackSize: " << cc.container->ci->stackSize;
        cout << "\ncode:\n\n" << code_to_string(cc.lineTable, cc.instructions) << endl;
    }
    exit(1);
}