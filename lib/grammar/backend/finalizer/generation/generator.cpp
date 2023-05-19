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

void generate() {
    offsetUUIDGenerator();

    for(Int i = 0; i < compressedCompilationFiles.size(); i++) {
        generate_addresses(compressedCompilationFiles.get(i));
    }

    for(Int i = 0; i < compressedCompilationFunctions.size(); i++) {
        generate(compressedCompilationFunctions.get(i));
    }

    remove_delegate_functions();
    compressedCompilationFunctions.linearSort(is_greater_than);
    compressedCompilationClasses.linearSort(is_greater_than);

    generate_exe();
}

void generation_error(string message) {

    cout << PROG_NAME << ": <code-generator> fatal error: " << message << endl;
    cout << "note: in function: " << function_to_str(cc.container) << endl;
    exit(1);
}