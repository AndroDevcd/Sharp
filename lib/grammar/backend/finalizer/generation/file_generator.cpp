 //
// Created by bknun on 8/8/2022.
//

#include <iomanip>
#include "file_generator.h"
#include "field_generator.h"
#include "class_generator.h"
#include "function_generator.h"
#include "../../types/types.h"
#include "../../../../runtime/Exe.h"
#include "generator.h"
#include "../optimization/optimizer.h"
#include "../../../../util/zip/zlib.h"
#include "../../../../util/File.h"

 stringstream exeBuf;
stringstream dataSec;
List<sharp_function*> functionPointers;

void generate_addresses(sharp_file *file) {
    // Static Fields
    for(Int i = 0; i < file->fields.size(); i++) {
        sharp_field *sf = file->fields.get(i);

        if(sf->used) {
            generate_address(sf);
        }
    }

    // classes
    for(Int i = 0; i < file->classes.size(); i++) {
        sharp_class *sc = file->classes.get(i);

        if(sc->used) {
            generate_class_addresses(sc);
        }
    }

    // Static Functions
    for(Int i = 0; i < file->functions.size(); i++) {
        sharp_function *sf = file->functions.get(i);

        if(sf->used) {
            generate_address(sf);
        }
    }
}


 string copy_chars(char c, int t) {
     stringstream s;
     int it = 0;

     while (it++ < t)
         s << c;

     return s.str();
 }

 string putInt32(int32_t i32) // todo: rename to have underscores in name
 {
     string str;

     str+=(uint8_t)GET_i32w(i32);
     str+=(uint8_t)GET_i32x(i32);
     str+=(uint8_t)GET_i32y(i32);
     str+=(uint8_t)GET_i32z(i32);
     return str;
 }

void build_manifest() {
    exeBuf << (char)manif;
    exeBuf << ((char)0x02); exeBuf << options.out << ((char)nil);
    exeBuf << ((char)0x4); exeBuf << options.version << ((char)nil);
    exeBuf << ((char)0x5); exeBuf << (options.debug ? 1 : 0);
    exeBuf << ((char)0x6); exeBuf << putInt32(genesis_method->ci->address);
    exeBuf << ((char)0x7); exeBuf << putInt32(compressedCompilationFunctions.size());
    exeBuf << ((char)0x8); exeBuf << putInt32(compressedCompilationClasses.size());
    exeBuf << ((char)0x9); exeBuf << file_vers << ((char)nil);
    exeBuf << ((char)0x0c); exeBuf << putInt32(stringMap.size());
    exeBuf << ((char)0x0e); exeBuf << options.target << ((char)nil);
    exeBuf << ((char)0x0f); exeBuf << putInt32(compressedCompilationFiles.size());
    exeBuf << ((char)0x1b); exeBuf << putInt32(threadLocalCount);
    exeBuf << ((char)0x1c); exeBuf << putInt32(constantMap.size());
    exeBuf << '\n' << (char)eoh;
}

void build_header() {
    exeBuf << (char)file_sig << "SEF";
    exeBuf << copy_chars(nil, zoffset);
    exeBuf << (char)digi_sig1 << (char)digi_sig2 << (char)digi_sig3;
}

void add_symbol(sharp_type &type, stringstream &ss = exeBuf) {
    ss << putInt32(type.type);
    if(type == type_class) {
        ss << putInt32(type._class->ci->address);
    } else if(type == type_function_ptr) {
        ss << putInt32(functionPointers.indexof(type.fun));
    } else if(type.type > type_class && type.type != type_nil) {
        generation_error("attempt to add invalid type to exe file");
    }
}

void add_function_pointers() {
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        sharp_class *sc = compressedCompilationClasses.get(i);

        for(Int j = 0; j < sc->fields.size(); j++) {
            auto sf = sc->fields.get(j);
            if(sf->used && sf->type == type_function_ptr) {
                functionPointers.addif(sf->type.fun);
            }
        }

        for(Int j = 0; j < compressedCompilationFunctions.size(); j++) {
            sharp_function *fun = compressedCompilationFunctions.get(j);

            if(fun->returnType == type_function_ptr) {
                functionPointers.addif(fun->returnType.fun);
            }

            for(Int x = 0; x < fun->parameters.size(); x++) {
                sharp_field *param = fun->parameters.get(x);
                if(param->type == type_function_ptr) {
                    functionPointers.addif(param->type.fun);
                }
            }
        }
    }

    exeBuf << putInt32(functionPointers.size());
    for(Int i = 0; i < functionPointers.size(); i++) {
        sharp_function *fun = functionPointers.get(i);
        exeBuf << (char)data_symbol;

        exeBuf << putInt32(fun->parameters.size());
        for(Int j = 0; j < fun->parameters.size(); j++) {
            sharp_field *param = fun->parameters.get(j);
            add_symbol(param->type);
            exeBuf << (param->type.isArray ? 1 : 0);
        }

        add_symbol(fun->returnType);
        exeBuf << (fun->returnType.isArray ? 1 : 0);
    }
}

void build_field_data(sharp_field *field) {
    exeBuf << (char)data_field;
    exeBuf << field->name << ((char)nil);
    exeBuf << field->fullName << ((char)nil);
    exeBuf << putInt32(field->ci->address);

    exeBuf << putInt32(field->type.type);
    exeBuf << putInt32(field->uid);
    exeBuf << putInt32(field->flags);
    exeBuf << (field->type.isArray ? 1 : 0);
    exeBuf << (field->fieldType == tls_field ? 1 : 0);
    add_symbol(field->type);
    exeBuf << putInt32(field->owner->ci->address);
    exeBuf << ((char)nil) << ((char)nil);
}

void build_field_data(sharp_class *sc) {
    for (Int i = 0; i < sc->fields.size(); i++) {
        sharp_field *field = sc->fields.get(i);

        if(field->used && !check_flag(field->flags, flag_static))
            build_field_data(field);
    }

    for (Int i = 0; i < sc->fields.size(); i++) {
        sharp_field *field = sc->fields.get(i);

        if(field->used && check_flag(field->flags, flag_static))
            build_field_data(field);
    }
}

void build_method_data(sharp_class *sc) {
    if(sc->baseClass != NULL)
        build_method_data(sc->baseClass);

    for(Int i = 0; i < sc->functions.size(); i++) {
        sharp_function *function = sc->functions.get(i);

        if(function->used) {
            exeBuf << (char) data_method;
            exeBuf << putInt32(function->ci->address) << ((char) nil);
        }
    }
}

void build_interface_data(sharp_class *sc) {
    if(sc->baseClass != NULL)
        build_interface_data(sc->baseClass);

    for(Int i = 0; i < sc->interfaces.size(); i++) {
        sharp_class *intf = sc->interfaces.get(i);

        if(intf->used) {
            exeBuf << (char) data_interface;
            exeBuf << putInt32(intf->ci->address) << ((char) nil);
        }
    }
}

void build_symbol_section() {
    exeBuf << (char)ssymbol;

    add_function_pointers();
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        sharp_class *sc = compressedCompilationClasses.get(i);

        exeBuf << (char)data_class;
        exeBuf << putInt32(sc->ci->address);
        exeBuf << (sc->owner == NULL ? putInt32(-1) : putInt32(sc->owner->ci->address));
        exeBuf << (sc->baseClass == NULL ? putInt32(-1) : putInt32(sc->baseClass->ci->address));
        exeBuf << putInt32(sc->uid);
        exeBuf << sc->name << ((char)nil);
        exeBuf << sc->fullName << ((char)nil);
        exeBuf << putInt32(get_static_field_count(sc));
        exeBuf << putInt32(get_instance_field_count(sc));
        exeBuf << putInt32(get_function_count(sc));
        exeBuf << putInt32(get_interface_count(sc));

        build_field_data(sc);
        build_method_data(sc);
        build_interface_data(sc);
    }

    exeBuf << '\n' << (char)eos;
}

void build_string_section() {
    exeBuf << (char)sstring;

    for(Int i = 0; i < stringMap.size(); i++) {
        exeBuf << (char)data_string;
        exeBuf << putInt32(stringMap.get(i).size());
        exeBuf << stringMap.get(i);
    }

    exeBuf << '\n' << (char)eos;
}


void build_constant_section() {
    exeBuf << (char)sconst;

    for(Int i = 0; i < constantMap.size(); i++) {
        exeBuf << (char)data_const;
        exeBuf << std::scientific
            << std::setprecision(std::numeric_limits<double>::digits10 + 1);
        exeBuf << constantMap.get(i);
        exeBuf << std::dec << ((char)nil);
    }

    exeBuf << '\n' << (char)eos;
}

 void build_method_data(sharp_function *fun) { // todo: decide at the last minute to provide regular name or obfuscated name for all types
     dataSec << (char)data_method;
     dataSec << putInt32(fun->ci->address);
     dataSec << putInt32(fun->uid);
     dataSec << fun->name << ((char)nil);
     dataSec << fun->fullName << ((char)nil);
     dataSec << putInt32(compressedCompilationFiles.indexof(fun->implLocation.file));
     dataSec << putInt32(fun->owner->ci->address);
     dataSec << putInt32(fun->type);
     dataSec << putInt32(fun->ci->stackSize);
     dataSec << putInt32(fun->ci->code.size());
     dataSec << putInt32(fun->flags);
     dataSec << putInt32(fun->delegate == NULL ? -1 : fun->delegate->ci->address);
     dataSec << putInt32(fun->ci->fpOffset);
     dataSec << putInt32(fun->ci->spOffset);
     dataSec << putInt32(fun->ci->frameStackOffset);
     add_symbol(fun->returnType, dataSec);
     dataSec << (fun->returnType.isArray ? 1 : 0);
     dataSec << putInt32(fun->parameters.size());
     for(Int i = 0; i < fun->parameters.size(); i++) {
         sharp_field *param = fun->parameters.get(i);
         add_symbol(param->type, dataSec);
         dataSec << (param->type.isArray ? 1 : 0);
     }


     dataSec << putInt32(fun->ci->lineTable.size());
     for(Int i = 0; i < fun->ci->lineTable.size(); i++) {
         dataSec << putInt32(fun->ci->lineTable.get(i)->start_pc);
         dataSec << putInt32(fun->ci->lineTable.get(i)->line);
     }

     dataSec << putInt32(fun->ci->tryCatchTable.size());
     for(Int i = 0; i < fun->ci->tryCatchTable.size(); i++) {
         try_catch_data *tryCatchData = fun->ci->tryCatchTable.get(i);
         dataSec << putInt32(tryCatchData->tryStartPc);
         dataSec << putInt32(tryCatchData->tryEndPc);
         dataSec << putInt32(tryCatchData->blockStartPc);
         dataSec << putInt32(tryCatchData->blockEndPc);

         dataSec << putInt32(tryCatchData->catchTable.size());
         for(Int j = 0; j < tryCatchData->catchTable.size(); j++) {
             catch_data *catchData = tryCatchData->catchTable.get(j);
             dataSec << putInt32(catchData->handlerPc);
             dataSec << putInt32(catchData->exceptionFieldAddress);
             dataSec << putInt32(catchData->classAddress);
         }

         if(tryCatchData->finallyData != NULL) {
             dataSec << ((char)1);
             dataSec << putInt32(tryCatchData->finallyData->startPc);
             dataSec << putInt32(tryCatchData->finallyData->endPc);
             dataSec << putInt32(tryCatchData->finallyData->exceptionFieldAddress);
         } else
             dataSec << ((char)nil);
     }
}

 void build_method_code(sharp_function *fun) {
     dataSec << (char)data_byte;

     for(Int i = 0; i < fun->ci->code.size(); i++) {
         dataSec << putInt32(fun->ci->code.get(i));
     }
 }

 void build_data_section() {
     dataSec << (char)sdata;

     for(Int i = 0; i < compressedCompilationFunctions.size(); i++) {
         build_method_data(compressedCompilationFunctions.get(i));
     }

     for(Int i = 0; i < compressedCompilationFunctions.size(); i++) {
         build_method_code(compressedCompilationFunctions.get(i));
     }


     dataSec << '\n' << (char)eos;
 }

 void build_meta_data_section() {
     dataSec << (char)smeta;

     for(Int i = 0; i < compressedCompilationFiles.size(); i++) {
         dataSec << (char)data_file;
         dataSec << compressedCompilationFiles.get(i)->name << ((char)nil);

         if(options.debug) {
             dataSec << putInt32(compressedCompilationFiles.get(i)->p->getData().size());
             dataSec << compressedCompilationFiles.get(i)->p->getData() << ((char)nil);
         }
     }

     dataSec << '\n' << (char)eos;
 }

void generate_exe() {
    build_header();
    build_manifest();
    build_symbol_section();
    build_string_section();
    build_constant_section();
    build_data_section();
    build_meta_data_section();

    string data = dataSec.str();
    if(data.size() >= data_compress_threshold) {
        dataSec.str("");

        exeBuf << (char)data_compress;
        stringstream __outbuf__;
        Zlib zlib;

        Zlib::AUTO_CLEAN=(true);
        zlib.Compress_Buffer2Buffer(data, __outbuf__, ZLIB_LAST_SEGMENT);
        data.clear();

        exeBuf << __outbuf__.str(); __outbuf__.str("");
    } else {
        exeBuf << dataSec.str();
        dataSec.str("");
    }

    if(options.compile_mode == project_mode) {
        string div;

#ifdef __WIN32
        div = "\\";
#else
        div = "/";
#endif
        string buildDir = options.project_dir + div + "build";
        string outFile = buildDir + div + options.out;

        File::makeDir(buildDir);
        if (File::write(outFile.c_str(), exeBuf.str())) {
            cout << PROG_NAME << ": error: failed to write out to executable " << outFile << endl;
        }
    } else {
        if (File::write(options.out.c_str(), exeBuf.str())) {
            cout << PROG_NAME << ": error: failed to write out to executable " << options.out << endl;
        }
    }
}