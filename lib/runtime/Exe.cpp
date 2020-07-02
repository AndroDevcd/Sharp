//
// Created by BraxtonN on 2/15/2018.
//

#include <cstdlib>
#include "Exe.h"
#include "../util/File.h"
#include "../util/KeyPair.h"
#include "symbols/Method.h"
#include "Manifest.h"
#include "symbols/Field.h"
#include "symbols/ClassObject.h"
#include "symbols/Object.h"
#include "../util/zip/zlib.h"
#include "main.h"
#include "VirtualMachine.h"

runtime::String tmpStr;
string exeErrorMessage;
uInt currentPos = 0;

bool validateAuthenticity(File::buffer& exe);

native_string& getString(File::buffer& exe, Int length = -1);
Int parseInt(File::buffer& exe);
int32_t geti32(File::buffer& exe);
Symbol* getSymbol(File::buffer& buffer);

void parseSourceFile(SourceFile &sourceFile, native_string &data);

int Process_Exe(std::string &exe)
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

    if(buffer.at(currentPos++) != manif){
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_MANIFEST;
    }

    for (;;) {
        currentFlag = buffer.at(currentPos++);
        processedFlags++;
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
            case eoh:
                processedFlags--;
                break;

            case 0x2:
                vm.manifest.application = getString(buffer);
                break;
            case 0x4:
                vm.manifest.version = getString(buffer);
                break;
            case 0x5:
                vm.manifest.debug = buffer.at(currentPos++) == '1';
                break;
            case 0x6:
                vm.manifest.entryMethod = geti32(buffer);
                break;
            case 0x7:
                vm.manifest.methods = geti32(buffer);
                break;
            case 0x8:
                vm.manifest.classes = geti32(buffer);
                break;
            case 0x9:
                vm.manifest.fvers = parseInt(buffer);
                break;
            case 0x0c:
                vm.manifest.strings = geti32(buffer);
                break;
            case 0x0e:
                vm.manifest.target = parseInt(buffer);
                break;
            case 0x0f:
                vm.manifest.sourceFiles = geti32(buffer);
                break;
            case 0x1b:
                vm.manifest.threadLocals = geti32(buffer);
                break;
            case 0x1c:
                vm.manifest.constants = geti32(buffer);
                break;
            default: {
                exeErrorMessage = "file `" + exe + "` may be corrupt.\n";
                return CORRUPT_MANIFEST;
            }
        }

        if(currentFlag == eoh) {

            if(processedFlags != HEADER_SIZE)
                return CORRUPT_MANIFEST;
            else if(vm.manifest.target > BUILD_VERS)
                return UNSUPPORTED_BUILD_VERSION;

            if(!(vm.manifest.fvers >= min_file_vers && vm.manifest.fvers <= file_vers)) {
                stringstream err;
                err << "unsupported file version of: " << vm.manifest.fvers << ". Sharp supports file versions from `"
                    << min_file_vers << "-" << file_vers << "` that can be processed. Are you possibly targeting the incorrect virtual machine?";
                exeErrorMessage = err.str();
                return UNSUPPORTED_FILE_VERS;
            }
            break;
        }
    }

    if(buffer.at(currentPos++) != ssymbol){
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    /* Symbol section */
    const Int nativeSymbolCount = 14;
    uInt sourceFilesCreated=0;
    uInt itemsProcessed=0;
    uInt functionPointersProcessed=0;

    vm.classes =(ClassObject*)malloc(sizeof(ClassObject)*vm.manifest.classes);
    vm.methods = (Method*)malloc(sizeof(Method)*vm.manifest.methods);
    vm.strings = (runtime::String*)malloc(sizeof(runtime::String)*(vm.manifest.strings)); // TODO: create "" string from compiler as the 0'th element
    vm.constants = (double*)malloc(sizeof(double)*(vm.manifest.constants));
    vm.staticHeap = (Object*)calloc(vm.manifest.classes, sizeof(Object));
    vm.metaData.init();
    vm.nativeSymbols = (Symbol*)malloc(sizeof(Symbol)*nativeSymbolCount);

    vm.manifest.functionPointers = geti32(buffer);
    vm.funcPtrSymbols = (Method*)malloc(sizeof(Method)*vm.manifest.functionPointers);

    for(Int i = 0; i < nativeSymbolCount; i++) {
        vm.nativeSymbols[i].init();
        vm.nativeSymbols[i].type = (DataType)i;
    }


    if(vm.classes == NULL || vm.methods == NULL || vm.staticHeap == NULL
       || vm.strings == NULL) {
        exeErrorMessage = "Failed to allocate memory for the program.";
        return OUT_OF_MEMORY;
    }

    for (;;) {

        currentFlag = buffer.at(currentPos++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_symbol: {
                if(functionPointersProcessed >= vm.manifest.functionPointers) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                Method &method = vm.funcPtrSymbols[functionPointersProcessed++];
                method.init();
                method.fnType = fn_ptr;

                Int paramSize = geti32(buffer);
                if(paramSize > 0) {
                    method.params = (Param *) malloc(sizeof(Param) * paramSize);
                    for (Int i = 0; i < paramSize; i++) {
                        method.params[i].utype =
                                getSymbol(buffer);
                        method.params[i].isArray = buffer.at(currentPos++) == '1';

                        if(method.params[i].utype == NULL)
                            return CORRUPT_FILE;
                    }
                }

                method.utype = getSymbol(buffer);
                method.arrayUtype = buffer.at(currentPos++) == '1';

                if(method.utype == NULL)
                    return CORRUPT_FILE;
                break;
            }

            case data_class: {
                Int address = geti32(buffer);
                if(address >= vm.manifest.classes) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                ClassObject* klass = &vm.classes[address];
                klass->init();
                klass->address = address;

                address = geti32(buffer);
                if(address != -1) klass->owner = &vm.classes[address];
                address = geti32(buffer);
                if(address != -1) klass->super = &vm.classes[address];

                klass->guid = geti32(buffer);
                klass->name = getString(buffer);
                klass->fullName = getString(buffer);
                klass->staticFields = geti32(buffer);
                klass->instanceFields = geti32(buffer);
                klass->totalFieldCount = klass->staticFields + klass->instanceFields;
                klass->methodCount = geti32(buffer);
                klass->interfaceCount = geti32(buffer);

                if(klass->totalFieldCount > 0)
                    klass->fields = (Field*)malloc(sizeof(Field)*klass->totalFieldCount);
                if(klass->methodCount > 0)
                    klass->methods = (Method**)malloc(sizeof(Method**)*klass->methodCount);
                if(klass->interfaceCount > 0)
                    klass->interfaces = (ClassObject**)malloc(sizeof(ClassObject**)*klass->interfaceCount);

                itemsProcessed=0;
                if(klass->totalFieldCount != 0) {
                    for( ;; ) {
                        if(buffer.at(currentPos) == data_field) {
                            if(itemsProcessed >= klass->totalFieldCount) {
                                exeErrorMessage = "file `" + exe + "` may be corrupt";
                                return CORRUPT_FILE;
                            }

                            currentPos++;
                            Field *field = &klass->fields[itemsProcessed++];
                            field->init();

                            field->name = getString(buffer);
                            field->fullName = getString(buffer);
                            field->address = geti32(buffer);
                            field->type = (DataType) geti32(buffer);
                            field->guid = geti32(buffer);
                            field->flags = geti32(buffer);
                            field->isArray = buffer.at(currentPos++) == '1';
                            field->threadLocal = buffer.at(currentPos++) == '1';
                            field->utype = getSymbol(buffer);
                            field->owner = &vm.classes[geti32(buffer)];

                            if(field->utype == NULL)
                                return CORRUPT_FILE;
                        } else if(buffer.at(currentPos) == 0x0 || buffer.at(currentPos) == 0x0a || buffer.at(currentPos) == 0x0d){ /* ignore */ }
                        else
                            break;
                        currentPos++;
                    }

                    if(itemsProcessed != klass->totalFieldCount) {
                        exeErrorMessage = "Failed to process all class fields in executable.";
                        return CORRUPT_FILE;
                    }
                }

                itemsProcessed = 0;
                if(klass->methodCount != 0) {
                    for( ;; ) {
                        if(buffer.at(currentPos) == data_method) {
                            if(itemsProcessed >= klass->methodCount) {
                                exeErrorMessage = "file `" + exe + "` may be corrupt";
                                return CORRUPT_FILE;
                            }

                            currentPos++;
                            klass->methods[itemsProcessed++] = &vm.methods[geti32(buffer)];
                        } else if(buffer.at(currentPos) == 0x0 || buffer.at(currentPos) == 0x0a || buffer.at(currentPos) == 0x0d){ /* ignore */ }
                        else
                            break;

                        currentPos++;
                    }

                    if(itemsProcessed != klass->methodCount) {
                        exeErrorMessage = "Failed to process all class methods in executable.";
                        return CORRUPT_FILE;
                    }
                }

                itemsProcessed = 0;
                if(klass->interfaceCount != 0) {
                    for( ;; ) {
                        if(buffer.at(currentPos) == data_interface) {
                            if(itemsProcessed >= klass->interfaceCount) {
                                exeErrorMessage = "file `" + exe + "` may be corrupt";
                                return CORRUPT_FILE;
                            }

                            currentPos++;
                            klass->interfaces[itemsProcessed++] = &vm.classes[geti32(buffer)];
                        } else if(buffer.at(currentPos) == 0x0 || buffer.at(currentPos) == 0x0a || buffer.at(currentPos) == 0x0d){ /* ignore */ }
                        else
                            break;

                        currentPos++;
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
    if(buffer.at(currentPos++) != sstring) {
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    for (;;) {

        currentFlag = buffer.at(currentPos++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_string: {
                if(itemsProcessed >= vm.manifest.strings) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                vm.strings[itemsProcessed].init();
                vm.strings[itemsProcessed++] = getString(buffer, geti32(buffer));
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
            if (itemsProcessed != vm.manifest.strings) {
                exeErrorMessage = "Failed to process all strings in executable.";
                return CORRUPT_FILE;
            }

            break;
        }
    }

    /* Constants section */
    itemsProcessed=0;
    if(buffer.at(currentPos++) != sconst) {
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    for (;;) {

        currentFlag = buffer.at(currentPos++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_const: {
                if(itemsProcessed >= vm.manifest.constants) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                vm.constants[itemsProcessed++] = std::strtod(getString(buffer).c_str(), NULL);
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
            if (itemsProcessed != vm.manifest.constants) {
                exeErrorMessage = "Failed to process all constants in executable.";
                return CORRUPT_FILE;
            }

            break;
        }
    }

    if(buffer.at(currentPos) == data_compress) {
        currentPos++;
        // restructure buffer
        stringstream buf, __outbuf__;
        for(uInt i = currentPos; i < buffer.size(); i++) {
            __outbuf__ << buffer.at(i);
        }

        Zlib zlib;
        Zlib::AUTO_CLEAN=(true);
        zlib.Decompress_Buffer2Buffer(__outbuf__.str(), buf);

        buffer.end();
        buffer << buf.str(); buf.str("");
        currentPos = 0;
    }

    if(buffer.at(currentPos++) != sdata) {
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    /* Data section */
    itemsProcessed = 0;
    for (;;) {

        currentFlag = buffer.at(currentPos++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_method: {
                if(itemsProcessed >= vm.manifest.methods) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                Method* method = &vm.methods[itemsProcessed++];
                method->init();

                method->address = geti32(buffer);
                method->guid = geti32(buffer);
                method->name = getString(buffer);
                method->fullName = getString(buffer);
                method->sourceFile = geti32(buffer);
                method->owner = &vm.classes[geti32(buffer)];
                method->fnType = (function_type)geti32(buffer);
                method->stackSize = geti32(buffer);
                method->cacheSize = geti32(buffer);
                method->flags = geti32(buffer);
                method->nativeFunc = ((method->flags >> 9) & 1u);
                method->delegateAddress = geti32(buffer);
                method->fpOffset = geti32(buffer);
                method->spOffset = geti32(buffer);
                method->frameStackOffset = geti32(buffer);
                method->utype = getSymbol(buffer);
                method->arrayUtype = buffer.at(currentPos++) == '1';
                Int paramSize = geti32(buffer);
                method->paramSize = paramSize;

                if(method->utype == NULL)
                    return CORRUPT_FILE;

                if(paramSize > 0) {
                    method->params = (Param *) malloc(sizeof(Param) * paramSize);
                    for (Int i = 0; i < paramSize; i++) {
                        method->params[i].utype =
                                getSymbol(buffer);
                        method->params[i].isArray =
                                buffer.at(currentPos++) == '1';

                        if (method->params[i].utype == NULL)
                            return CORRUPT_FILE;
                    }
                }

//                    if(c_options.jit) {
//                        if(method->address==316) method->isjit = true;
//                        if(method->address==7) method->isjit = true;
//                    }

                long len = geti32(buffer);
                for(Int i = 0; i < len; i++) {
                    Int pc = geti32(buffer);
                    Int line = geti32(buffer);
                    method->lineTable
                            .add(LineData(pc, line));
                }


                len = geti32(buffer);
                for(Int i = 0; i < len; i++) {
                    TryCatchData &tryCatchData = method->tryCatchTable.__new();

                    tryCatchData.init();
                    tryCatchData.try_start_pc= geti32(buffer);
                    tryCatchData.try_end_pc= geti32(buffer);
                    tryCatchData.block_start_pc= geti32(buffer);
                    tryCatchData.block_end_pc= geti32(buffer);

                    Int caughtExceptions = geti32(buffer);
                    for(Int j =0; j < caughtExceptions; j++) {
                        CatchData &catchData = tryCatchData.catchTable.__new();
                        catchData.handler_pc = geti32(buffer);
                        catchData.localFieldAddress = geti32(buffer);
                        catchData.caughtException = &vm.classes[geti32(buffer)];
                    }

                    if(buffer.at(currentPos++) == 1) {
                        tryCatchData.finallyData = (FinallyData*)malloc(sizeof(FinallyData));
                        tryCatchData.finallyData->start_pc = geti32(buffer);
                        tryCatchData.finallyData->end_pc = geti32(buffer);
                        tryCatchData.finallyData->exception_object_field_address = geti32(buffer);
                    }
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
            if(itemsProcessed != vm.manifest.methods)
                throw std::runtime_error("text section may be corrupt");
            currentPos--;
            break;
        }
    }

    itemsProcessed=0;
    for (;;) {

        currentFlag = buffer.at(currentPos++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_byte: {
                if(itemsProcessed >= vm.manifest.methods) {
                    exeErrorMessage = "file `" + exe + "` may be corrupt";
                    return CORRUPT_FILE;
                }

                Method* method = &vm.methods[itemsProcessed++];

                if(method->cacheSize > 0) {
                    method->bytecode = (opcode_instr*)malloc(sizeof(opcode_instr)*method->cacheSize);
                    for(int64_t i = 0; i < method->cacheSize; i++) {
                        method->bytecode[i] = geti32(buffer);
                    }
                } else if(method->fnType != fn_delegate) {
                    exeErrorMessage = "method `" + method->fullName.str() + "` is missing bytecode";
                    return CORRUPT_FILE;
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

    if(buffer.at(currentPos++) != smeta) {
        exeErrorMessage = "file `" + exe + "` may be corrupt";
        return CORRUPT_FILE;
    }

    /* Meta Data Section */
    for(;;) {
        currentFlag = buffer.at(currentPos++);
        switch (currentFlag) {
            case 0x0:
            case 0x0a:
            case 0x0d:
                break;

            case data_file: {

                SourceFile &sourceFile = vm.metaData.files.__new();
                sourceFile.init();
                sourceFile.name = getString(buffer);

                if(vm.manifest.debug) {
                    String sourceFileData(getString(buffer, geti32(buffer)));
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

Symbol* getSymbol(File::buffer& buffer) {
    DataType type = (DataType)geti32(buffer);
    if((type >= _INT8 && type <= _UINT64)
       || type == VAR || type == OBJECT
       || type == NIL) {
        return &vm.nativeSymbols[type];
    } else if(type == CLASS) {
        return &vm.classes[geti32(buffer)];
    } else if(type == FNPTR) {
        return &vm.funcPtrSymbols[geti32(buffer)];
    } else {
        exeErrorMessage = "invalid field type found in symbol table";
        return NULL;
    }
}

int32_t geti32(File::buffer& exe) {
    int32_t i32 =SET_i32(
            exe.at(currentPos), exe.at(currentPos + 1),
            exe.at(currentPos + 2), exe.at(currentPos + 3)
    ); currentPos+=EXE_BYTE_CHUNK;

    return i32;
}

native_string& getString(File::buffer& exe, Int length) {
    tmpStr.free();
    if(length != -1) {
        for (; length > 0; length--) {
            tmpStr += exe.at(currentPos);
            currentPos++;
        }
    } else {
        while (exe.at(currentPos++) != nil) {
            tmpStr += exe.at(currentPos - 1);
        }
    }

    return tmpStr;
}

Int parseInt(File::buffer& exe) {
#if _ARCH_BITS == 32
    return strtol(getString(exe).c_str(), NULL, 0);
#else
    return strtoll(getString(exe).c_str(), NULL, 0);
#endif
}

native_string& string_forward(File::buffer& str, size_t begin, size_t end) {
    tmpStr.free();
    if(begin >= str.size() || end >= str.size())
        throw std::invalid_argument("unexpected end of stream");

    size_t it=0;
    for(size_t i = begin; it++ < end; i++)
        tmpStr +=str.at(i);

    return tmpStr;
}

bool validateAuthenticity(File::buffer& exe) {
    if(exe.at(currentPos++) == file_sig && string_forward(exe, currentPos, 3) == "SEF") {
        currentPos += 3 + zoffset;

        /* Check file's digital signature */
        if(exe.at(currentPos++) == digi_sig1 && exe.at(currentPos++) == digi_sig2
           && exe.at(currentPos++) == digi_sig3) {
            return true;
        }
    }
    return false;
}
