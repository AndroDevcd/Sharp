//
// Created by bknun on 9/18/2022.
//

#include "symbol_processor.h"
#include "exe_processor.h"
#include "../../core/exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../types/sharp_type.h"
#include "../types/sharp_field.h"

void process_symbols(KeyPair<int, string> &result) {
    check_section(ssymbol, CORRUPT_FILE, result);

    const Int nativeSymbolCount = 14;
    uInt itemsProcessed=0;
    uInt functionPointersProcessed=0;
    uInt functionPtrIndex=0;

    vm.mf.functionPointers = next_int32();
    vm.classes = (sharp_class*) calloc(vm.mf.classes, sizeof(sharp_class));
    vm.methods = (sharp_function*) calloc(vm.mf.methods + vm.mf.functionPointers, sizeof(sharp_function));
    vm.strings = (string*)calloc(vm.mf.strings, sizeof(string)); // TODO: create "" string from compiler as the 0'th element
    vm.constants = (double*)calloc(vm.mf.constants, sizeof(double));
    vm.staticHeap = (object*)calloc(vm.mf.classes, sizeof(object));

    vm.nativeTypes = (sharp_type*) calloc(nativeSymbolCount, sizeof(sharp_type));
    functionPtrIndex = vm.mf.methods;
    
    if(vm.classes == nullptr || vm.methods == nullptr || vm.staticHeap == nullptr
       || vm.strings == nullptr || vm.constants  == nullptr || vm.nativeTypes == nullptr) {
        result.with(OUT_OF_MEMORY, "Failed to allocate memory for the program.");
        throw runtime_error("");
    }
    
    for(Int i = 0; i < nativeSymbolCount; i++) {
        vm.nativeTypes[i].type = (data_type)i;
    }


    for(;;) {\
        switch(next_char()) {\
            case 0x0:\
            case 0x0a:\
            case 0x0d:\
            case (eos):\
                break;\

        case data_symbol: {
            if(functionPointersProcessed >= vm.mf.functionPointers) {
                result.with(CORRUPT_FILE, "file `" + executable + "` may be corrupt");
                throw runtime_error("");
            }

            functionPointersProcessed++;
            sharp_function &sf = vm.methods[functionPtrIndex++];
            sf.fnType = blueprint_function;

            Int paramSize = next_int32();
            if(paramSize > 0) {
                sf.params = (function_param *) calloc(paramSize, sizeof(function_param));
                for (Int i = 0; i < paramSize; i++) {
                    sf.params[i].type = next_type();
                    sf.params[i].isArray = next_char() == '1';
                }
            }

            sf.returnType = next_type();
            sf.arrayReturn = next_char() == '1';
            break;
        }

        case data_class: {
            Int address = next_int32();
            if(address >= vm.mf.classes || address < 0) {
                throw invalid_argument("file `" + executable + "` may be corrupt");
            }

            sharp_class* sc = &vm.classes[address];
            sc->address = address;

            address = next_int32();
            if(address != -1) sc->owner = &vm.classes[address];
            address = next_int32();
            if(address != -1) sc->base = &vm.classes[address];

            sc->guid = next_int32();
            next_string(sc->name);
            next_string(sc->fullName);
            sc->hash = get_hash(sc->fullName);
            sc->staticFields = next_int32();
            sc->instanceFields = next_int32();
            sc->totalFieldCount = sc->staticFields + sc->instanceFields;
            sc->methodCount = next_int32();
            sc->interfaceCount = next_int32();

            if(sc->fullName == "std#int" || sc->address == 9) {
                int i = 0;
            }
            if(sc->totalFieldCount > 0) {
                sc->fields = (sharp_field*) calloc(sc->totalFieldCount, sizeof(sharp_field));
            }
            if(sc->methodCount > 0) {
                sc->methods = (sharp_function**) calloc(sc->methodCount, sizeof(sharp_function**));
            }
            if(sc->interfaceCount > 0) {
                sc->interfaces = (sharp_class**) calloc(sc->interfaceCount,sizeof(sharp_class**));
            }

            itemsProcessed=0;
            if(sc->totalFieldCount != 0) {
                for( ;; ) {
                    if(next_char() == data_field) {
                        if(itemsProcessed >= sc->totalFieldCount) {
                            throw invalid_argument("file `" + executable + "` may be corrupt");
                        }

                        sharp_field *field = &sc->fields[itemsProcessed++];

                        next_string(field->name);
                        next_string(field->fullName);
                        field->address = next_int32();
                        field->guid = next_int32();
                        field->flags = next_int32();
                        field->isArray = next_char() == '1';
                        field->threadLocal = next_char() == '1';
                        field->type = next_type();
                        field->owner = &vm.classes[next_int32()];

                    } else if(current_char() == 0x0 || current_char() == 0x0a || current_char() == 0x0d){ /* ignore */ }
                    else {
                        cursor--;
                        break;
                    }
                }

                if(itemsProcessed != sc->totalFieldCount) {
                    throw invalid_argument("Failed to process all class fields in executable.");
                }
            }

            itemsProcessed = 0;
            if(sc->methodCount != 0) {
                for( ;; ) {
                    if(next_char() == data_method) {
                        if(itemsProcessed >= sc->methodCount) {
                            throw invalid_argument("file `" + executable + "` may be corrupt");
                        }

                        Int funcAddress = next_int32();
                        sc->methods[itemsProcessed++] = &vm.methods[funcAddress];
                    } else if(current_char() == 0x0 || current_char() == 0x0a || current_char() == 0x0d){ /* ignore */ }
                    else {
                        cursor--;
                        break;
                    }
                }

                if(itemsProcessed != sc->methodCount) {
                    throw invalid_argument("Failed to process all class methods in executable.");
                }
            }

            itemsProcessed = 0;
            if(sc->interfaceCount != 0) {
                for( ;; ) {
                    if(next_char() == data_interface) {
                        if(itemsProcessed >= sc->interfaceCount) {
                            throw invalid_argument("file `" + executable + "` may be corrupt");
                        }

                        sc->interfaces[itemsProcessed++] = &vm.classes[next_int32()];
                    } else if(current_char() == 0x0 || current_char() == 0x0a || current_char() == 0x0d){ /* ignore */ }
                    else {
                        cursor--;
                        break;
                    }
                }

                if(itemsProcessed != sc->interfaceCount) {
                    throw invalid_argument("Failed to process all interfaces in executable.");
                }
            }
            continue;
        }
            default: {
                result.with(CORRUPT_MANIFEST, "file `" + executable + "` may be corrupt.\n");
                throw runtime_error("");
            }
        }

        if(current_char() == eos)  {
            break;
        }
    }
}
