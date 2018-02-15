//
// Created by BraxtonN on 2/15/2018.
//

#include "Exe.h"
#include "../util/File.h"
#include "init.h"
#include "Environment.h"

Manifest manifest;
Meta metaData;

uint64_t n = 0, jobIndx=0;

bool checkFile(File::buffer& exe);

string getstring(File::buffer& exe);

int64_t getlong(File::buffer& exe);

void getField(File::buffer& exe, List<KeyPair<int64_t, Field*>> &fieldMap, Field* field);

void getMethod(File::buffer& exe, ClassObject *parent, Method* method);

ClassObject *findClass(int64_t superClass);

int64_t geti64(File::buffer& exe) ;

void parse_source_file(List<native_string> &list, native_string str);

int Process_Exe(std::string exe)
{
    file::buffer buffer;
    int __bitFlag, hdr_cnt=0;

    jobIndx++;
    if(!file::exists(exe.c_str())){
        std::runtime_error("file `" + exe + "` doesnt exist!");
    }

    manifest.executable.init();
    manifest.executable = exe;
    file::read_alltext(exe.c_str(), buffer);
    if(buffer.empty())
        return 1;

    try {
        if(!checkFile(buffer)) {
            throw std::runtime_error("file `" + exe + "` could not be ran");
        }

        jobIndx++;
        bool manifestFlag = false;
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
                    manifest.application =getstring(buffer);
                    break;
                case 0x4:
                    manifest.version.init();
                    manifest.version =getstring(buffer);
                    break;
                case 0x5:
                    manifest.debug = buffer.at(n++) == 1;
                    break;
                case 0x6:
                    manifest.entry =getmi64(buffer);
                    break;
                case 0x7:
                    manifest.methods =getmi64(buffer);
                    break;
                case 0x8:
                    manifest.classes =getmi64(buffer);
                    break;
                case 0x9:
                    manifest.fvers =getlong(buffer);
                    break;
                case 0x0c:
                    manifest.strings =getmi64(buffer);
                    break;
                case 0x0e:
                    manifest.target =getlong(buffer);
                    break;
                case 0x0f:
                    manifest.sourceFiles =getlong(buffer);
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == eoh) {
                if(!manifestFlag)
                    throw std::runtime_error("missing manifest flag");
                if(manifest.fvers != 1)
                    throw std::runtime_error("unknown file version");

                if(hdr_cnt != hsz || manifest.target > mvers)
                    return 1;

                break;
            }
        }

        if(buffer.at(n++) != sdata)
            throw std::runtime_error("file `" + exe + "` may be corrupt");

        /* Data section */
        list<MetaClass> mClasses;
        list<MetaField> mFields;
        int64_t classRefptr=0, macroRefptr=0, fileRefptr=0;

        env->classes =(ClassObject*)malloc(sizeof(ClassObject)*(manifest.classes + AUX_CLASSES));
        env->methods = (sh_asp*)malloc(sizeof(sh_asp)*manifest.methods);
        env->strings = (String*)malloc(sizeof(String)*(manifest.strings+1));
        env->globalHeap = (Object*)malloc(sizeof(Object)*manifest.classes);
        env->sourceFiles = (nString*)malloc(sizeof(nString)*manifest.sourceFiles);

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
                    ClassObject* klass = &env->classes[classRefptr++];
                    mClasses.push_back(MetaClass(klass, getlong(buffer)));

                    klass->id = getmi64(buffer);
                    klass->name.init();
                    klass->name = getstring(buffer);
                    klass->fieldCount = getlong(buffer);
                    klass->methodCount = getlong(buffer);

                    if(klass->fieldCount != 0) {
                        klass->flds = (Field*)malloc(sizeof(Field)*klass->fieldCount);
                    } else
                        klass->flds = NULL;
                    if(klass->methodCount != 0) {
                        klass->methods = (int64_t *)malloc(sizeof(int64_t)*klass->methodCount);
                    } else
                        klass->methods = NULL;
                    klass->super = NULL;

                    if(klass->fieldCount != 0) {
                        for( ;; ) {
                            if(buffer.at(n) == data_field) {
                                n++;
                                getField(buffer, mFields, &klass->flds[fieldPtr++]);
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
                                klass->methods[functionPtr++] = getmi64(buffer);
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
        for(MetaClass& metaClass : mClasses) {
            if(metaClass.super != -1)
                metaClass.c->super = findClass(metaClass.super);
        }

        for(MetaField& metaField : mFields) {
            metaField.field->owner = findClass(metaField.owner);
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
                    env->strings[stringPtr].id = getmi64(buffer); n++;
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

        env->strings[stringPtr].id = stringPtr+1;
        env->strings[stringPtr].value.init();
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

                    sh_asp* adsp = &env->methods[aspRef++];
                    adsp->init();

                    adsp->id = getmi64(buffer);
                    adsp->name = getstring(buffer);
                    adsp->sourceFile = getlong(buffer);
                    adsp->owner = findClass(getmi64(buffer));
                    adsp->param_size = getmi64(buffer);
                    adsp->frame_init = getmi64(buffer);
                    adsp->cache_size = getmi64(buffer);
                    adsp->self = getlong(buffer);

                    long len = getlong(buffer);
                    line_table lt;
                    for(long i = 0; i < len; i++) {
                        lt.pc = getmi64(buffer);
                        lt.line_number = getmi64(buffer);
                        adsp->lineNumbers.push_back(lt);
                    }

                    len = getlong(buffer);
                    ExceptionTable et;

                    for(long i = 0; i < len; i++) {
                        et.handler_pc=getmi64(buffer);
                        et.end_pc=getmi64(buffer);
                        et.klass=getstring(buffer);
                        et.local=getmi64(buffer);
                        et.start_pc=getmi64(buffer);
                        adsp->exceptions.push_back();

                        ExceptionTable &e = adsp->exceptions.get(adsp->exceptions.size()-1);
                        e.init();

                        e = et;
                    }

                    len = getlong(buffer);
                    FinallyTable ft;

                    for(long i = 0; i < len; i++) {
                        ft.start_pc=getmi64(buffer);
                        ft.end_pc=getmi64(buffer);
                        adsp->finallyBlocks.push_back(ft);
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
                    sh_asp* adsp = &env->methods[aspRef++];

                    if(adsp->param_size > 0) {
                        adsp->params = (int64_t*)malloc(sizeof(int64_t)*adsp->param_size);
                        adsp->arrayFlag = (bool*)malloc(sizeof(bool)*adsp->param_size);
                        for(unsigned int i = 0; i < adsp->param_size; i++) {
                            adsp->params[i] = getlong(buffer);
                            adsp->arrayFlag[i] = (bool)getlong(buffer);
                        }
                    }

                    if(adsp->cache_size > 0) {
                        adsp->bytecode = (int64_t*)malloc(sizeof(int64_t)*adsp->cache_size);
                        for(int64_t i = 0; i < adsp->cache_size; i++) {
                            adsp->bytecode[i] = getmi64(buffer);
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
                        metaData.sourceFiles.push_back();
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
        jobIndx-=2;
    } catch(std::exception &e) {
        cout << "error " << e.what();
        return 1;
    }

    return 0;
}

void parse_source_file(List<native_string> &list, native_string str) {
    list.init();

    native_string line;
    for(unsigned int i = 0; i < str.len; i++) {
        if(str.chars[i] == '\n')
        {
            list.push_back();
            list.get(list.size()-1).init();
            list.get(list.size()-1) = line;

            line.free();
        } else {
            line += str.chars[i];
        }
    }

    list.push_back();
    list.get(list.size()-1).init();
    list.get(list.size()-1) = line;

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
        if(env->classes[i].id == superClass)
            return &env->classes[i];
    }

    return NULL;
}

void getField(File::buffer& exe, List<KeyPair<int64_t, Field*>> &fieldMap, Field* field) {
    field->name.init();
    field->name = getstring(exe);
    field->serial = getlong(exe);
    field->type = (int)getlong(exe);
    field->isStatic = (bool)getlong(exe);
    field->isArray = (bool)getlong(exe);
    field->owner = NULL;
    mFields.push_back(MetaField(field, getlong(exe)));
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
        n +=3 + offset;

        /* Check file's digital signature */
        if(exe.at(n++) == digi_sig1 && exe.at(n++) == digi_sig2
           && exe.at(n++) == digi_sig3) {
            return true;
        }
    }
    return false;
}
