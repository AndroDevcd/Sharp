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

Manifest manifest;
Meta metaData;

uint64_t n = 0, jobIndx=0;

bool checkFile(File::buffer& exe);

string getstring(File::buffer& exe);

int64_t getlong(File::buffer& exe);

void getField(File::buffer& exe, _List<KeyPair<int64_t, Field*>> &fieldMap, Field* field);

void getMethod(File::buffer& exe, ClassObject *parent, Method* method);

ClassObject *findClass(int64_t superClass);

int64_t geti64(File::buffer& exe) ;

void parse_source_file(_List<native_string> &list, native_string str);

int Process_Exe(std::string exe)
{
    File::buffer buffer, reserve;
    int __bitFlag, hdr_cnt=0, compressed = 0;

    if(!File::exists(exe.c_str())){
        std::runtime_error("file `" + exe + "` doesnt exist!");
    }

    manifest.executable.init();
    manifest.executable = exe;
    File::read_alltext(exe.c_str(), buffer);
    if(buffer.empty())
        return 1;

    try {
        if(!checkFile(buffer)) {
            throw std::runtime_error("file `" + exe + "` could not be ran");
        }

        bool manifestFlag = false;
        int var = 0;
        for (;;) {

            hdr_cnt++;
            __bitFlag = buffer.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                case eoh:
                    hdr_cnt--;
                    break;

                case manif:
                    hdr_cnt--;
                    manifestFlag = true;
                    break;

                case 0x2:
                    manifest.application.init();
                    manifest.application = getstring(buffer);
                    break;
                case 0x4:
                    manifest.version.init();
                    manifest.version =getstring(buffer);
                    break;
                case 0x5:
                    manifest.debug = buffer.at(n++) == '1';
                    break;
                case 0x6:
                    manifest.entryMethod =geti64(buffer);
                    break;
                case 0x7:
                    manifest.methods =geti64(buffer);
                    break;
                case 0x8:
                    manifest.classes =geti64(buffer);
                    break;
                case 0x9:
                    manifest.fvers =getlong(buffer);
                    break;
                case 0x0c:
                    manifest.strings =geti64(buffer);
                    break;
                case 0x0e:
                    manifest.target =getlong(buffer);
                    break;
                case 0x0f:
                    manifest.sourceFiles =getlong(buffer);
                    break;
                case 0x1b:
                    manifest.threadLocals =geti64(buffer);
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == eoh) {
                if(!manifestFlag)
                    throw std::runtime_error("missing manifest flag");
                if(manifest.fvers != file_vers)
                    throw std::runtime_error("unknown file version");

                if(hdr_cnt != hsz || manifest.target > mvers)
                    return 1;

                break;
            }
        }

        if(buffer.at(n++) != sdata)
            throw std::runtime_error("file `" + exe + "` may be corrupt");

        /* Data section */
        _List<KeyPair<int64_t, ClassObject*>> mSuperClasses;
        _List<KeyPair<int64_t, ClassObject*>> mBaseClasses;
        _List<KeyPair<int64_t, Field*>> mFields;
        int64_t fileRefptr=0;

        manifest.classes += AUX_CLASSES;
        env->classes =(ClassObject*)malloc(sizeof(ClassObject)*manifest.classes);
        env->methods = (Method*)malloc(sizeof(Method)*manifest.methods);
        env->strings = (runtime::String*)malloc(sizeof(runtime::String)*(manifest.strings+1));
        env->globalHeap = (Object*)malloc(sizeof(Object)*manifest.classes);
        env->sourceFiles = (native_string*)malloc(sizeof(native_string)*manifest.sourceFiles);
        env->strings[manifest.strings].value.init();
        env->strings[manifest.strings].id = -1;

        for(unsigned long i = 0; i < manifest.classes; i++) {
            env->globalHeap[i].object = NULL;
        }

        if(env->classes == NULL || env->methods == NULL || env->globalHeap == NULL
           || env->strings == NULL) {
            throw Exception("Failed to allocate memory for program,"
                                    " try reducing program size!");
        }

        for (;;) {

            __bitFlag = buffer.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_class: {
                    int64_t fieldPtr=0, functionPtr=0;
                    long l1 = getlong(buffer);
                    long l2 = getlong(buffer);
                    long serial = geti64(buffer);
                    ClassObject* klass = &env->classes[serial];
                    mSuperClasses.add(KeyPair<int64_t, ClassObject*>(l1, klass));
                    mBaseClasses.add(KeyPair<int64_t, ClassObject*>(l2, klass));

                    klass->serial = serial;
                    klass->name.init();
                    klass->name = getstring(buffer);
                    klass->fieldCount = getlong(buffer);
                    klass->methodCount = getlong(buffer);
                    klass->interfaceCount = getlong(buffer);

                    if(klass->fieldCount != 0) {
                        klass->fields = (Field*)malloc(sizeof(Field)*klass->fieldCount);
                    } else
                        klass->fields = NULL;
                    if(klass->methodCount != 0) {
                        klass->methods = (unsigned long*)malloc(sizeof(unsigned long)*klass->methodCount);
                    } else
                        klass->methods = NULL;
                    if(klass->interfaceCount != 0) {
                        klass->interfaces = (unsigned long*)malloc(sizeof(unsigned long)*klass->interfaceCount);
                    } else
                        klass->interfaces = NULL;
                    klass->super = NULL;
                    klass->base = NULL;

                    if(klass->fieldCount != 0) {
                        for( ;; ) {
                            if(buffer.at(n) == data_field) {
                                n++;
                                getField(buffer, mFields, &klass->fields[fieldPtr++]);
                            } else if(buffer.at(n) == 0x0a || buffer.at(n) == 0x0d) {
                                n++;
                            } else
                                break;

                            if(fieldPtr > klass->fieldCount) {
                                throw std::runtime_error("invalid field size");
                            }
                        }
                    }

                    if(klass->methodCount != 0) {
                        for( ;; ) {
                            if(buffer.at(n) == data_method) {
                                n++;
                                klass->methods[functionPtr++] = geti64(buffer);
                                n++;
                            } else if(buffer.at(n) == 0x0a || buffer.at(n) == 0x0d){
                                n++;
                            } else
                                break;
                        }

                        if(functionPtr != klass->methodCount) {
                            throw std::runtime_error("invalid method size");
                        }
                    }

                    functionPtr = 0;
                    if(klass->interfaceCount != 0) {
                        for( ;; ) {
                            if(buffer.at(n) == data_interface) {
                                n++;
                                klass->interfaces[functionPtr++] = geti64(buffer);
                                n++;
                            } else if(buffer.at(n) == 0x0a || buffer.at(n) == 0x0d){
                                n++;
                            } else
                                break;
                        }

                        if(functionPtr != klass->interfaceCount) {
                            throw std::runtime_error("invalid interface size");
                        }
                    }
                    break;
                }
                case data_file:
                    env->sourceFiles[fileRefptr].init();
                    env->sourceFiles[fileRefptr] = getstring(buffer);

                    fileRefptr++;
                    break;
                case eos:
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == eos) {
                break;
            }
        }

        /* Resolve classes */
        for(unsigned long i = 0; i < mSuperClasses.size(); i++) {
            KeyPair<int64_t, ClassObject*> &klass =
                    mSuperClasses.get(i);
            if(klass.key != -1)
                klass.value->super = findClass(klass.key);
        }
        mSuperClasses.free();

        for(unsigned long i = 0; i < mBaseClasses.size(); i++) {
            KeyPair<int64_t, ClassObject*> &klass =
                    mBaseClasses.get(i);
            if(klass.key != -1)
                klass.value->base = findClass(klass.key);
        }
        mBaseClasses.free();

        for(unsigned long i = 0; i < mFields.size(); i++) {
            KeyPair<int64_t, Field*> &field =
                    mFields.get(i);
            field.value->owner = findClass(field.key);
        }

        /* String section */
        int64_t stringPtr=0, stringCnt=0;

        for (;;) {

            __bitFlag = buffer.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_string: {
                    env->strings[stringPtr].id = geti64(buffer); n++;
                    env->strings[stringPtr].value.init();
                    env->strings[stringPtr].value = getstring(buffer);

                    stringPtr++, stringCnt++;
                    break;
                }

                case eos:
                    break;

                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == eos) {
                if(stringCnt != manifest.strings)
                    throw std::runtime_error("invalid string size");
                break;
            }
        }

        if(buffer.at(n) == data_compress) {
            n++;
            compressed=1;
            // restructure buffer
            int len = geti64(buffer);
            stringstream buf, __outbuf__;

            for(long long i = n+len; i < buffer.size(); i++) {
                reserve << buffer.at(i);
            }

            for(long long i = n; i < buffer.size(); i++) {
                __outbuf__ << buffer.at(i);
            }

            Zlib zlib;
            Zlib::AUTO_CLEAN=(true);
            zlib.Decompress_Buffer2Buffer(__outbuf__.str(), buf);

            buffer.end();
            buffer << buf.str(); buf.str("");
            n = 0;
        }

        if(buffer.at(n++) != stext)
            throw std::runtime_error("file `" + exe + "` may be corrupt");

        /* Text section */
        uint64_t aspRef=0;

        for (;;) {

            __bitFlag = buffer.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_method: {
                    if(aspRef >= manifest.methods)
                        throw std::runtime_error("text section may be corrupt");

                    Method* method = &env->methods[aspRef++];
                    method->init();

                    method->address = geti64(buffer);
                    method->isjit = getlong(buffer);
                    method->name = getstring(buffer);
                    method->fullName = getstring(buffer);
                    method->sourceFile = getlong(buffer);
                    method->owner = findClass(geti64(buffer));
                    method->paramSize = geti64(buffer);
                    method->stackSize = geti64(buffer);
                    method->cacheSize = geti64(buffer);
                    method->isStatic = getlong(buffer);
                    method->returnVal = getlong(buffer);
                    method->delegateAddress = geti64(buffer);
                    method->stackEqulizer = geti64(buffer);
                    if(method->address==302) method->isjit = true;

                    long len = getlong(buffer);
                    line_table lt;
                    for(long i = 0; i < len; i++) {
                        lt.line_number = geti64(buffer);
                        lt.pc = geti64(buffer);
                        method->lineNumbers.push_back(lt);
                    }

                    len = getlong(buffer);
                    long addressLen, valuesLen;
                    for(long i = 0; i < len; i++) {
                        method->switchTable.__new();
                        method->switchTable.last().init();
                        SwitchTable &st = method->switchTable.last();

                        addressLen = getlong(buffer);
                        for(long x = 0; x < addressLen; x++) {
                            st.addresses.add(geti64(buffer));
                        }

                        valuesLen = getlong(buffer);
                        for(long x = 0; x < valuesLen; x++) {
                            st.values.add(geti64(buffer));
                        }

                        st.defaultAddress = geti64(buffer);
                    }


                    len = getlong(buffer);
                    ExceptionTable et;

                    for(long i = 0; i < len; i++) {
                        et.handler_pc=geti64(buffer);
                        et.end_pc=geti64(buffer);
                        et.className=getstring(buffer);
                        et.local=geti64(buffer);
                        et.start_pc=geti64(buffer);
                        method->exceptions.__new();

                        ExceptionTable &e = method->exceptions.get(method->exceptions.size()-1);
                        e.init();

                        e = et;
                    }

                    len = getlong(buffer);
                    FinallyTable ft;

                    for(long i = 0; i < len; i++) {
                        ft.start_pc=geti64(buffer);
                        ft.end_pc=geti64(buffer);
                        ft.try_start_pc=geti64(buffer);
                        ft.try_end_pc=geti64(buffer);
                        method->finallyBlocks.push_back(ft);
                    }
                    break;
                }

                case data_byte:
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == data_byte) {
                if(aspRef != manifest.methods)
                    throw std::runtime_error("text section may be corrupt");
                n--;
                break;
            }
        }

        aspRef=0;

        for (;;) {

            __bitFlag = buffer.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_byte: {
                    Method* method = &env->methods[aspRef++];

                    if(method->paramSize > 0) {
                        method->params = (int64_t*)malloc(sizeof(int64_t)*method->paramSize);
                        method->arrayFlag = (bool*)malloc(sizeof(bool)*method->paramSize);
                        for(unsigned int i = 0; i < method->paramSize; i++) {
                            method->params[i] = getlong(buffer);
                            method->arrayFlag[i] = (bool)getlong(buffer);
                        }
                    }

                    if(method->cacheSize > 0) {
                        method->bytecode = (int64_t*)malloc(sizeof(int64_t)*method->cacheSize);
                        for(int64_t i = 0; i < method->cacheSize; i++) {
                            method->bytecode[i] = geti64(buffer);
                        }
                    }
                    break;
                }

                case eos:
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == eos) {
                break;
            }
        }


        if(compressed) {
            buffer.end();
            buffer << reserve;
            reserve.end();
            n = 0;
        }

        if(manifest.debug) {
            if(buffer.at(n++) != smeta)
                throw std::runtime_error("file `" + exe + "` may be corrupt");

            long sourceid=0;
            for(;;) {
                __bitFlag = buffer.at(n++);
                switch (__bitFlag) {
                    case 0x0:
                    case 0x0a:
                    case 0x0d:
                        break;

                    case data_file: {
                        metaData.sourceFiles.__new();
                        source_file &sf = metaData.sourceFiles.get(
                                metaData.sourceFiles.size()-1);

                        sf.id = sourceid++;
                        parse_source_file(sf.source_line, getstring(buffer));
                        break;
                    }

                    case eos:
                        break;
                    default:
                        throw std::runtime_error("file `" + exe + "` may be corrupt");
                }

                if(__bitFlag == eos) {
                    break;
                }
            }
        }
    } catch(std::exception &e) {
        cout << "error " << e.what();
        return 1;
    }

    return 0;
}

void parse_source_file(_List<native_string> &list, native_string str) {
    list.init();

    native_string line;
    for(unsigned int i = 0; i < str.len; i++) {
        if(str.chars[i] == '\n')
        {
            list.__new();
            list.last().init();
            list.last() = line;

            line.free();
        } else {
            line += str.chars[i];
        }
    }

    list.__new();
    list.last().init();
    list.last() = line;

    line.free();
}

int64_t geti64(File::buffer& exe) {
    int64_t i64 =GET_i64(
            SET_i32(exe.at(n), exe.at(n+1),
                     exe.at(n+2), exe.at(n+3)
            ), SET_i32(exe.at(n+4), exe.at(n+5),
                        exe.at(n+6), exe.at(n+7)
            )
    ); n+=I64_BYTES;

    return i64;
}

ClassObject *findClass(int64_t superClass) {
    for(uint64_t i = 0; i < manifest.classes; i++) {
        if(env->classes[i].serial == superClass)
            return &env->classes[i];
    }

    return NULL;
}

void getField(File::buffer& exe, _List<KeyPair<int64_t, Field*>> &fieldMap, Field* field) {
    field->name.init();
    field->name = getstring(exe);
    field->serial = getlong(exe);
    field->type = (FieldType)getlong(exe);
    field->isStatic = (bool)getlong(exe);
    field->isArray = (bool)getlong(exe);
    field->isThreadLocal = (bool)getlong(exe);
    field->owner = NULL;
    fieldMap.add(KeyPair<int64_t, Field*>(getlong(exe), field));
}

string getstring(File::buffer& exe) {
    string s;
    char c = exe.at(n);
    while(exe.at(n++) != nil) {
        s+=exe.at(n-1);
    }

    return s;
}

int64_t getlong(File::buffer& exe) {
    native_string integer;
    integer = getstring(exe);
    return strtoll(integer.str().c_str(), NULL, 0);
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

bool checkFile(File::buffer& exe) {
    if(exe.at(n++) == file_sig && string_forward(exe, n, 3) == "SEF") {
        n +=3 + zoffset;

        /* Check file's digital signature */
        if(exe.at(n++) == digi_sig1 && exe.at(n++) == digi_sig2
           && exe.at(n++) == digi_sig3) {
            return true;
        }
    }
    return false;
}
