//
// Created by BraxtonN on 2/15/2018.
//

#include <cstdlib>
#include "Exe.h"
#include "../util/File.h"
#include "../util/KeyPair.h"
#include "Environment.h"
#include "oo/Method.h"
#include "Manifest.h"
#include "oo/Field.h"
#include "oo/ClassObject.h"
#include "oo/Object.h"
#include "../util/zip/zlib.h"
#include "main.h"
#include "VirtualMachine.h"

string exeErrorMessage;
uInt index = 0;

bool validateAuthenticity(File::buffer& exe);

string getString(File::buffer& exe);
Int parseInt(File::buffer& exe);
int32_t geti32(File::buffer& exe);

void parseSourceFile(SourceFile &sourceFile, native_string &data);

int Process_Exe(std::string exe)
{
    File::buffer buffer;
    Int processedFlags=0, currentFlag;

    if(!File::exists(exe.c_str())){
        exeErrorMessage = "file `" + exe + "` doesnt exist!\n";
        return FILE_DOES_NOT_EXIST;
    }

    File::read_alltext(exe.c_str(), buffer);
    if(buffer.empty()) {
        exeErrorMessage = "file `" + exe + "` is empty.\n";
        return EMPTY_FILE;
    }

    if(!validateAuthenticity(buffer)) {
        exeErrorMessage = "file `" + exe + "` could not be ran.\n";
        return FILE_NOT_AUTHENTIC;
    }

    for (;;) {
        if(buffer.at(index++) != manif){
            exeErrorMessage = "file `" + exe + "` may be corrupt";
            return CORRUPT_MANIFEST;
        }

        currentFlag = buffer.at(index++);
        processedFlags++;
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
            case eoh:
                processedFlags--;
                break;

            case 0x2:
                vm->manifest.application = getString(buffer);
                break;
            case 0x4:
                vm->manifest.version = getString(buffer);
                break;
            case 0x5:
                vm->manifest.debug = buffer.at(index++) == '1';
                break;
            case 0x6:
                vm->manifest.entryMethod = geti32(buffer);
                break;
            case 0x7:
                vm->manifest.methods = geti32(buffer);
                break;
            case 0x8:
                vm->manifest.classes = geti32(buffer);
                break;
            case 0x9:
                vm->manifest.fvers = parseInt(buffer);
                break;
            case 0x0c:
                vm->manifest.strings = geti32(buffer);
                break;
            case 0x0e:
                vm->manifest.target = parseInt(buffer);
                break;
            case 0x0f:
                vm->manifest.sourceFiles = geti32(buffer);
                break;
            case 0x1b:
                vm->manifest.threadLocals = geti32(buffer);
                break;
            case 0x1c:
                vm->manifest.constants = geti32(buffer);
                break;
            default: {
                exeErrorMessage = "file `" + exe + "` may be corrupt.\n";
                return CORRUPT_MANIFEST;
            }
        }

        if(currentFlag == eoh) {
            if(!(vm->manifest.fvers >= min_file_vers && vm->manifest.fvers <= file_vers)) {
                stringstream err;
                err << "unsupported file version of: " << vm->manifest.fvers << ". Sharp supports file versions from `"
                    << min_file_vers << "-" << file_vers << "` that can be processed. Are you possibly targeting the incorrect virtual machine?";
                exeErrorMessage = err.str();
                return UNSUPPORTED_FILE_VERS;
            }

            if(processedFlags != HEADER_SIZE)
                return CORRUPT_MANIFEST;
            else if(vm->manifest.target > BUILD_VERS)
                return UNSUPPORTED_BUILD_VERSION;

            break;
        }
    }

    if(buffer.at(index++) != ssymbol){
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    /* Data section */
    uInt sourceFilesCreated=0;
    uInt itemsProcessed=0;
    vm->classes =(ClassObject*)malloc(sizeof(ClassObject)*vm->manifest.classes);
    vm->methods = (Method*)malloc(sizeof(Method)*vm->manifest.methods);
    vm->strings = (runtime::String*)malloc(sizeof(runtime::String)*(vm->manifest.strings)); // TODO: create "" string from compiler as the 0'th element
    vm->constants = (double*)malloc(sizeof(double)*(vm->manifest.constants));
    vm->staticHeap = (Object*)calloc(vm->manifest.classes, sizeof(Object));
    vm->metaData.init();


    if(vm->classes == NULL || vm->methods == NULL || vm->staticHeap == NULL
       || vm->strings == NULL) {
        exeErrorMessage = "Failed to allocate memory for the program.";
        return OUT_OF_MEMORY;
    }

    for (;;) {

        currentFlag = buffer.at(index++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_class: {
                itemsProcessed=0;
                Int address = geti32(buffer);
                if(address >= vm->manifest.classes) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                ClassObject* klass = &vm.classes[address];
                klass->init();

                address = geti32(buffer);
                if(address != -1) klass->owner = &vm.classes[address];
                address = geti32(buffer);
                if(address != -1) klass->super = &vm.classes[address];

                klass->serial = address;
                klass->name = getString(buffer);
                klass->fullName = getString(buffer);
                klass->staticFields = geti32(buffer);
                klass->instanceFields = geti32(buffer);
                klass->totalFieldCount = klass->staticFields + klass->instanceFields;
                klass->methodCount = geti32(buffer);
                klass->interfaceCount = geti32(buffer);

                klass->fields = (Field*)malloc(sizeof(Field)*klass->totalFieldCount);
                klass->methods = (Method**)malloc(sizeof(Method**)*klass->methodCount);
                klass->interfaces = (ClassObject**)malloc(sizeof(ClassObject**)*klass->interfaceCount);

                if(klass->totalFieldCount != 0) {
                    for( ;; ) {
                        if(buffer.at(index) == data_field) {
                            index++;
                            Field *field = &klass->fields[itemsProcessed++];
                            field->name.init();

                            field->name = getString(buffer);
                            field->address = geti32(buffer);
                            field->type = (DataType) parseInt(buffer);
                            field->accessFlags = geti32(buffer);
                            field->isArray = buffer.at(index++) == '1';
                            field->threadLocal = buffer.at(index++) == '1';

                            address = geti32(buffer);
                            if(address != -1) field->klass = &vm.classes[address];
                            field->owner = &vm->classes[geti32(buffer)];
                        } else if(buffer.at(index) == 0x0a || buffer.at(index) == 0x0d){ /* ignore */ }
                        else
                            break;
                        index++;
                    }

                    if(itemsProcessed != klass->totalFieldCount) {
                        exeErrorMessage = "Failed to process all class fields in executable.";
                        return CORRUPT_FILE;
                    }
                }

                itemsProcessed = 0;
                if(klass->methodCount != 0) {
                    for( ;; ) {
                        if(buffer.at(index) == data_method) {
                            index++;
                            klass->methods[itemsProcessed++] = &vm->methods[geti32(buffer)];
                        } else if(buffer.at(index) == 0x0a || buffer.at(index) == 0x0d){ /* ignore */ }
                        else
                            break;

                        index++;
                    }

                    if(itemsProcessed != klass->methodCount) {
                        exeErrorMessage = "Failed to process all methods in executable.";
                        return CORRUPT_FILE;
                    }
                }


                itemsProcessed = 0;
                if(klass->interfaceCount != 0) {
                    for( ;; ) {
                        if(buffer.at(index) == data_interface) {
                            index++;
                            klass->interfaces[itemsProcessed++] = &vm->classes[geti32(buffer)];
                        } else if(buffer.at(index) == 0x0a || buffer.at(index) == 0x0d){ /* ignore */ }
                        else
                            break;

                        index++;
                    }

                    if(itemsProcessed != klass->interfaceCount) {
                        exeErrorMessage = "Failed to process all interfaces in executable.";
                        return CORRUPT_FILE;
                    }
                }
                break;
            }
            case eos:
                break;
            default: {
                exeErrorMessage = "file `" + exe + "` may be corrupt";
                return CORRUPT_FILE;
            }
        }

        if(currentFlag == eos) {
            break;
        }
    }

    /* String section */
    itemsProcessed=0;
    if(buffer.at(index++) != sstring) {
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    for (;;) {

        currentFlag = buffer.at(index++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_string: {
                index++;
                vm->strings[itemsProcessed].init();
                vm->strings[itemsProcessed++] = getString(buffer);
                break;
            }

            case eos:
                break;

            default: {
                exeErrorMessage = "file `" + exe + "` may be corrupt";
                return CORRUPT_FILE;
            }
        }

        if (currentFlag == eos) {
            if (itemsProcessed != vm->manifest.strings) {
                exeErrorMessage = "Failed to process all strings in executable.";
                return CORRUPT_FILE;
            }

            break;
        }
    }

    if(buffer.at(index) == data_compress) {
        index++;
        // restructure buffer
        stringstream buf, __outbuf__;
        for(uInt i = index; i < buffer.size(); i++) {
            __outbuf__ << buffer.at(i);
        }

        Zlib zlib;
        Zlib::AUTO_CLEAN=(true);
        zlib.Decompress_Buffer2Buffer(__outbuf__.str(), buf);

        buffer.end();
        buffer << buf.str(); buf.str("");
        index = 0;
    }

    if(buffer.at(index++) != sdata) {
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    /* Text section */
    for (;;) {

        currentFlag = buffer.at(index++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_method: {
                if(itemsProcessed >= vm->manifest.methods) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                Method* method = &vm->methods[itemsProcessed++];
                method->init();

                method->address = geti32(buffer);
                method->isjit = parseInt(buffer);
                method->name = getString(buffer);
                method->fullName = getString(buffer);
                method->sourceFile = geti32(buffer);
                method->owner = &vm->classes[geti32(buffer)];
                method->paramSize = geti32(buffer);
                method->stackSize = geti32(buffer);
                method->cacheSize = geti32(buffer);
                method->isStatic = parseInt(buffer);
                method->returnsData = parseInt(buffer);
                method->delegateAddress = geti32(buffer);
                method->stackEqulizer = geti32(buffer);
//                    if(c_options.jit) {
//                        if(method->address==316) method->isjit = true;
//                        if(method->address==7) method->isjit = true;
//                    }

                long len = geti32(buffer);
                for(Int i = 0; i < len; i++) {
                    method->lineNumbers
                            .add(line_table(
                                    geti32(buffer),
                                    geti32(buffer)
                            ));
                }

                len = geti32(buffer);
                long addressLen, valuesLen;
                for(Int i = 0; i < len; i++) {
                    SwitchTable &st = method->switchTable.__new();
                    st.init();

                    addressLen = geti32(buffer);
                    for(Int x = 0; x < addressLen; x++) {
                        st.addresses.add(geti32(buffer));
                    }

                    valuesLen = geti32(buffer);
                    for(Int x = 0; x < valuesLen; x++) {
                        st.values.add(geti32(buffer));
                    }

                    st.defaultAddress = geti32(buffer);
                }


                len = parseInt(buffer);
                for(Int i = 0; i < len; i++) {
                    ExceptionTable &e = method->exceptions.__new();

                    e.init();
                    e.handler_pc= geti32(buffer);
                    e.end_pc= geti32(buffer);
                    e.className= getString(buffer);
                    e.local= geti32(buffer);
                    e.start_pc= geti32(buffer);
                }

                len = parseInt(buffer);
                FinallyTable ft;

                for(Int i = 0; i < len; i++) {
                    ft.start_pc= geti32(buffer);
                    ft.end_pc= geti32(buffer);
                    ft.try_start_pc= geti32(buffer);
                    ft.try_end_pc= geti32(buffer);
                    method->finallyBlocks.push_back(ft);
                }
                break;
            }

            case data_byte:
                break;
            default: {
                exeErrorMessage = "file `" + exe + "` may be corrupt";
                return CORRUPT_FILE;
            }
        }

        if(currentFlag == data_byte) {
            if(itemsProcessed != vm->manifest.methods)
                throw std::runtime_error("text section may be corrupt");
            index--;
            break;
        }
    }

    itemsProcessed=0;
    for (;;) {

        currentFlag = buffer.at(index++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_byte: {
                Method* method = &vm->methods[itemsProcessed++];

                if(method->paramSize > 0) {
                    method->params = (int64_t*)malloc(sizeof(int64_t)*method->paramSize);
                    method->arrayFlag = (bool*)malloc(sizeof(bool)*method->paramSize);
                    for(unsigned int i = 0; i < method->paramSize; i++) {
                        method->params[i] = parseInt(buffer);
                        method->arrayFlag[i] = (bool) parseInt(buffer);
                    }
                }

                if(method->cacheSize > 0) {
                    method->bytecode = (int64_t*)malloc(sizeof(int64_t)*method->cacheSize);
                    for(int64_t i = 0; i < method->cacheSize; i++) {
                        method->bytecode[i] = geti32(buffer);
                    }
                }
                break;
            }

            case eos:
                break;
            default: {
                exeErrorMessage = "file `" + exe + "` may be corrupt";
                return CORRUPT_FILE;
            }
        }

        if(currentFlag == eos) {
            break;
        }
    }

    if(buffer.at(index++) != smeta) {
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    for(;;) {
        currentFlag = buffer.at(index++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_file: {

                SourceFile &sourceFile = vm->metaData.files.__new();
                sourceFile.init();
                sourceFile.id = geti32(buffer);
                sourceFile.name = getString(buffer);

                if(vm->manifest.debug) {
                    String sourceFileData(getString(buffer));
                    parseSourceFile(sourceFile, sourceFileData);
                }
                break;
            }

            case eos:
                break;
            default: {
                exeErrorMessage = "file `" + exe + "` may be corrupt";
                return CORRUPT_FILE;
            }
        }

        if(currentFlag == eos) {
            break;
        }
    }

    return 0;
}

void parseSourceFile(SourceFile &sourceFile, native_string &data) {
    native_string line;
    for(unsigned int i = 0; i < data.len; i++) {
        if(data.chars[i] == '\n')
        {
            sourceFile.lines.__new().init();
            sourceFile.lines.last() = line;

            line.free();
        } else {
            line += data.chars[i];
        }
    }

    sourceFile.lines.__new().init();
    sourceFile.lines.last() = line;
    line.free();
}

int32_t geti32(File::buffer& exe) {
    int32_t i32 =SET_i32(
            exe.at(index), exe.at(index + 1),
                exe.at(index + 2), exe.at(index + 3)
    ); index+=EXE_BYTE_CHUNK;

    return i32;
}

string getString(File::buffer& exe) {
    string s;
    char c = exe.at(index);
    while(exe.at(index++) != nil) {
        s+=exe.at(index - 1);
    }

    return s;
}

Int parseInt(File::buffer& exe) {
#if _ARCH_BITS == 32
    return strtol(getString(exe).c_str(), NULL, 0);
#else
    return strtoll(getstring(exe).c_str(), NULL, 0);
#endif
}

native_string string_forward(File::buffer& str, size_t begin, size_t end) {
    if(begin >= str.size() || end >= str.size())
        throw std::invalid_argument("unexpected end of stream");

    size_t it=0;
    native_string s;
    for(size_t i = begin; it++ < end; i++)
        s +=str.at(i);

    return s;
}

bool validateAuthenticity(File::buffer& exe) {
    if(exe.at(index++) == file_sig && string_forward(exe, index, 3) == "SEF") {
        index += 3 + zoffset;

        /* Check file's digital signature */
        if(exe.at(index++) == digi_sig1 && exe.at(index++) == digi_sig2
           && exe.at(index++) == digi_sig3) {
            return true;
        }
    }
    return false;
}
