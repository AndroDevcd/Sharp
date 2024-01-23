 //
// Created by bknun on 8/8/2022.
//

#include <iomanip>
#include "file_generator.h"
#include "field_generator.h"
#include "class_generator.h"
#include "function_generator.h"
#include "../../types/types.h"
#include "generator.h"
#include "../optimization/optimizer.h"
#include "../../../../util/zip/zlib.h"
#include "../../../../util/File.h"
#include "../../../../core/exe_macros.h"

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

        if(sc->name == "build") {
            int j = 0;
        }
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

 string put_int32(int32_t i32)
 {
     string str;

     str+=(uint8_t)GET_i32w(i32);
     str+=(uint8_t)GET_i32x(i32);
     str+=(uint8_t)GET_i32y(i32);
     str+=(uint8_t)GET_i32z(i32);
     return str;
 }

 sharp_file* get_true_source_file(sharp_class *sc) {
     if(sc->genericBuilder != NULL) {
         return get_true_source_file(sc->genericBuilder);
     } else return sc->implLocation.file;
 }

 sharp_file* get_true_source_file(sharp_function *sf) {
     if(sf->owner->genericBuilder != NULL) {
         return get_true_source_file(sf->owner);
     } else return sf->implLocation.file;
 }

void build_manifest() {
    exeBuf << (char)manif;
    exeBuf << ((char)0x02); exeBuf << options.out << ((char)nil);
    exeBuf << ((char)0x4); exeBuf << options.version << ((char)nil);
    exeBuf << ((char)0x5); exeBuf << (options.debug ? 1 : 0);
    exeBuf << ((char)0x6); exeBuf << put_int32(genesis_method->ci->address);
    exeBuf << ((char)0x7); exeBuf << put_int32(compressedCompilationFunctions.size());
    exeBuf << ((char)0x8); exeBuf << put_int32(compressedCompilationClasses.size());
    exeBuf << ((char)0x9); exeBuf << file_vers << ((char)nil);
    exeBuf << ((char)0x0c); exeBuf << put_int32(stringMap.size());
    exeBuf << ((char)0x0e); exeBuf << options.target << ((char)nil);
    exeBuf << ((char)0x0f); exeBuf << put_int32(compressedCompilationFiles.size());
    exeBuf << ((char)0x1b); exeBuf << put_int32(threadLocalCount);
    exeBuf << ((char)0x1c); exeBuf << put_int32(constantMap.size());
    exeBuf << ((char)0x2a); exeBuf << put_int32(application_id);
    exeBuf << '\n' << (char)eoh;
}

void build_header() {
    exeBuf << (char)file_sig << "SEF";
    exeBuf << copy_chars(nil, zoffset);
    exeBuf << (char)digi_sig1 << (char)digi_sig2 << (char)digi_sig3;
}

void add_symbol(sharp_type &type, stringstream &ss = exeBuf) {
    ss << put_int32(type.type);
    if(type == type_class) {
        ss << put_int32(type._class->ci->address);
    } else if(type == type_function_ptr) {
        ss << put_int32(functionPointers.indexof(type.fun));
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

    exeBuf << put_int32(functionPointers.size());
    for(Int i = 0; i < functionPointers.size(); i++) {
        sharp_function *fun = functionPointers.get(i);
        exeBuf << (char)data_symbol;

        exeBuf << put_int32(fun->parameters.size());
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
    exeBuf << put_int32(field->ci->address);
    exeBuf << put_int32(field->uid);
    exeBuf << put_int32(field->flags);
    exeBuf << (field->type.isArray ? 1 : 0);
    exeBuf << (field->fieldType == localized_field ? 1 : 0);
    add_symbol(field->type);
    exeBuf << put_int32(field->owner->ci->address);
    exeBuf << ((char)nil) << ((char)nil);
}

void build_field_data(sharp_class *sc, bool instanceFieldOnly = false) {
    if(sc->baseClass != NULL) {
        build_field_data(sc->baseClass, true);
    }

    for (Int i = 0; i < sc->fields.size(); i++) {
        sharp_field *field = sc->fields.get(i);

        if(field->used && !check_flag(field->flags, flag_static))
            build_field_data(field);
    }

    if(!instanceFieldOnly) {
        for (Int i = 0; i < sc->fields.size(); i++) {
            sharp_field *field = sc->fields.get(i);

            if (field->used && check_flag(field->flags, flag_static))
                build_field_data(field);
        }
    }
}

void build_method_data(sharp_class *sc) {
    if(sc->baseClass != NULL)
        build_method_data(sc->baseClass);

    for(Int i = 0; i < sc->functions.size(); i++) {
        sharp_function *function = sc->functions.get(i);

        if(function->used) {
            exeBuf << (char) data_method;
            exeBuf << put_int32(function->ci->address) << ((char) nil);
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
            exeBuf << put_int32(intf->ci->address) << ((char) nil);
        }
    }
}

void build_symbol_section() {
    exeBuf << (char)ssymbol;

    add_function_pointers();
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        sharp_class *sc = compressedCompilationClasses.get(i);

        if(sc->fullName == "std#int") {
            int ll = 0;
        }
        exeBuf << (char)data_class;
        exeBuf << put_int32(sc->ci->address);
        exeBuf << (sc->owner == NULL ? put_int32(-1) : put_int32(sc->owner->ci->address));
        exeBuf << (sc->baseClass == NULL ? put_int32(-1) : put_int32(sc->baseClass->ci->address));
        exeBuf << put_int32(sc->uid);
        exeBuf << sc->name << ((char)nil);
        exeBuf << sc->fullName << ((char)nil);
        exeBuf << put_int32(get_static_field_count(sc));
        exeBuf << put_int32(get_instance_field_count(sc));
        exeBuf << put_int32(get_function_count(sc));
        exeBuf << put_int32(get_interface_count(sc));

        if(sc->name == "out_of_memory_exception") {
            int fi = 0;
        }

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
        exeBuf << put_int32(stringMap.get(i).size());
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

 void build_method_data(sharp_function *fun) {
    if(compressedCompilationFiles.indexof(get_true_source_file(fun)) == -1) {
        cout << "fun: " << fun->owner->fullName << "." << function_to_str(fun) << " no file added " << endl;
    }

    dataSec << (char)data_method;
     dataSec << put_int32(fun->ci->address);
     dataSec << put_int32(fun->uid);
     dataSec << fun->name << ((char)nil);
     dataSec << fun->fullName << ((char)nil);
     dataSec << put_int32(compressedCompilationFiles.indexof(get_true_source_file(fun)));
     dataSec << put_int32(fun->owner->ci->address);
     dataSec << put_int32(fun->type);
     dataSec << put_int32(fun->ci->stackSize);
     dataSec << put_int32(fun->ci->code.size());
     dataSec << put_int32(fun->flags);
     dataSec << put_int32(fun->delegate == NULL ? -1 : fun->delegate->ci->address);
     dataSec << put_int32(fun->ci->fpOffset);
     dataSec << put_int32(fun->ci->spOffset);
     dataSec << put_int32(fun->ci->frameStackOffset);
     add_symbol(fun->returnType, dataSec);
     dataSec << (fun->returnType.isArray ? 1 : 0);
     dataSec << put_int32(fun->parameters.size());
     for(Int i = 0; i < fun->parameters.size(); i++) {
         sharp_field *param = fun->parameters.get(i);
         add_symbol(param->type, dataSec);
         dataSec << (param->type.isArray ? 1 : 0);
     }


     dataSec << put_int32(fun->ci->lineTable.size());
     for(Int i = 0; i < fun->ci->lineTable.size(); i++) {
         dataSec << put_int32(fun->ci->lineTable.get(i)->start_pc);
         dataSec << put_int32(fun->ci->lineTable.get(i)->line);
     }

     dataSec << put_int32(fun->ci->tryCatchTable.size());
     for(Int i = 0; i < fun->ci->tryCatchTable.size(); i++) {
         try_catch_data *tryCatchData = fun->ci->tryCatchTable.get(i);
         dataSec << put_int32(tryCatchData->tryStartPc);
         dataSec << put_int32(tryCatchData->tryEndPc);
         dataSec << put_int32(tryCatchData->blockStartPc);
         dataSec << put_int32(tryCatchData->blockEndPc);

         dataSec << put_int32(tryCatchData->catchTable.size());
         for(Int j = 0; j < tryCatchData->catchTable.size(); j++) {
             catch_data *catchData = tryCatchData->catchTable.get(j);
             dataSec << put_int32(catchData->handlerPc);
             dataSec << put_int32(catchData->exceptionFieldAddress);
             dataSec << put_int32(catchData->classAddress);
         }

         if(tryCatchData->finallyData != NULL) {
             dataSec << ((char)1);
             dataSec << put_int32(tryCatchData->finallyData->startPc);
             dataSec << put_int32(tryCatchData->finallyData->endPc);
             dataSec << put_int32(tryCatchData->finallyData->exceptionFieldAddress);
         } else
             dataSec << ((char)nil);
     }
}

 void build_method_code(sharp_function *fun) {
     dataSec << (char)data_byte;

     for(Int i = 0; i < fun->ci->code.size(); i++) {
         dataSec << put_int32(fun->ci->code.get(i));
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
             dataSec << put_int32(compressedCompilationFiles.get(i)->p->getData().size());
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
        string buildDir = options.project_dir + div + "build"+ div + "outputs";
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