//
// Created by bknun on 2/18/2017.
//

#include "Exe.h"
#include "../../util/file.h"
#include "../startup.h"
#include "Environment.h"
#include "../oo/Field.h"
#include "../interp/vm.h"
#include "../oo/Method.h"
#include "../oo/Object.h"
#include "../interp/Opcode.h"

#define offset 0xf
#define file_sig 0x0f
#define digi_sig1 0x07
#define digi_sig2 0x1b
#define digi_sig3 0x0c
#define manif 0x1
#define eoh 0x03
#define nil 0x0
#define hsz 0x0a
#define eos 0x1d
#define sdata 0x05
#define sstring 0x02
#define stext 0x0e
#define smeta 0x06
#define data_class 0x2f
#define data_method 0x4c
#define data_field 0x22
#define data_string 0x1e
#define data_byte 0x05


Manifest manifest;
Meta meta;

uint64_t n = 0, jobIndx=0;
stringstream stackdump;

bool checkFile(file::stream& exe);

string getstring(file::stream& exe);

double getnumber(file::stream& exe);

int64_t getlong(file::stream& exe);

void getField(file::stream& exe, list <MetaField>& mFields, Field* field);

void getMethod(file::stream& exe, ClassObject *parent, Method* method);

ClassObject *findClass(int64_t superClass);

bool overflowOp(int op) ;

int Process_Exe(std::string exe)
{
    file::stream _fStream;
    int __bitFlag, hdr_cnt=0;

    jobIndx++;
    updateStackFile("reading executable");
    if(!file::exists(exe.c_str())){
        error("file `" + exe + "` doesnt exist!");
    }

    manifest.executable = exe;
    file::read_alltext(exe.c_str(), _fStream);
    if(_fStream.empty())
        return 1;

    try {
        if(!checkFile(_fStream)) {
            error("file `" + exe + "` could not be ran");
        }

        jobIndx++;
        updateStackFile("processing manifest");
        bool manifestFlag = false;
        for (;;) {

            hdr_cnt++;
            __bitFlag = _fStream.at(n++);
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
                    manifest.application =getstring(_fStream);
                    break;
                case 0x4:
                    manifest.version =getstring(_fStream);
                    break;
                case 0x5:
                    manifest.debug = _fStream.at(n++) != nil;
                    break;
                case 0x6:
                    manifest.entry =getlong(_fStream);
                    break;
                case 0x7:
                    manifest.methods =getlong(_fStream);
                    break;
                case 0x8:
                    manifest.classes =getlong(_fStream);
                    break;
                case 0x9:
                    manifest.fvers =getlong(_fStream);
                    break;
                case 0x0b:
                    manifest.isize =getlong(_fStream);
                    break;
                case 0x0c:
                    manifest.strings =getlong(_fStream);
                    break;
                case 0x0e:
                    manifest.baseaddr =getlong(_fStream);
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

        if(_fStream.at(n++) != sdata)
            throw std::runtime_error("file `" + exe + "` may be corrupt");

        /* Data section */
        list<MetaClass> mClasses;
        list<MetaField> mFields;
        int64_t classRefptr=0, macroRefptr=0;
        updateStackFile("processing .data section");

        env->classes =(ClassObject*)malloc(sizeof(ClassObject)*manifest.classes);
        env->objects = (gc_object*)malloc(sizeof(gc_object)*manifest.classes);
        env->methods = (Method*)malloc(sizeof(Method)*manifest.methods);
        env->strings = (String*)malloc(sizeof(String)*manifest.strings);
        env->bytecode = (int64_t*)malloc(sizeof(int64_t)*manifest.isize);

        for (;;) {

            __bitFlag = _fStream.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_class: {
                    int64_t fieldPtr=0, functionPtr=0;
                    ClassObject* c = &env->classes[classRefptr++];
                    mClasses.push_back(MetaClass(c, getlong(_fStream)));

                    c->id = getlong(_fStream);
                    c->name = getstring(_fStream);
                    c->fieldCount = getlong(_fStream);
                    c->methodCount = getlong(_fStream);
                    if(c->fieldCount != 0) {
                        c->flds = (Field*)malloc(sizeof(Field)*c->fieldCount);
                    } else
                        c->flds = NULL;
                    if(c->methodCount != 0) {
                        c->methods = (Method*)malloc(sizeof(Method)*c->methodCount);
                    } else
                        c->methods = NULL;
                    c->super = NULL;
                    c->fields = NULL;

                    if(c->fieldCount != 0) {
                        for( ;; ) {
                            if(_fStream.at(n) == data_field) {
                                n++;
                                getField(_fStream, mFields, &c->flds[fieldPtr++]);
                            } else if(_fStream.at(n) == 0x0a || _fStream.at(n) == 0x0d) {
                                n++;
                            } else
                                break;
                        }

                        if(fieldPtr != c->fieldCount) {
                            throw std::runtime_error("invalid field size");
                        }
                    }

                    if(c->methodCount != 0) {
                        for( ;; ) {
                            if(_fStream.at(n) == data_method) {
                                n++;
                                getMethod(_fStream, c, &c->methods[functionPtr]);

                                if(manifest.entry == c->methods[functionPtr].id)
                                    manifest.main = &c->methods[functionPtr];
                                functionPtr++;
                            } else if(_fStream.at(n) == 0x0a || _fStream.at(n) == 0x0d){
                                n++;
                            } else
                                break;
                        }

                        if(functionPtr != c->methodCount) {
                            throw std::runtime_error("invalid method size");
                        }
                    }
                    break;
                }

                case data_method:
                    getMethod(_fStream, NULL, &env->methods[macroRefptr++]);
                    break;
                case sstring:
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == sstring) {
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
        updateStackFile("processing .string section");

        for (;;) {

            __bitFlag = _fStream.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_string: {
                    env->strings[stringPtr].id = getlong(_fStream);
                    env->strings[stringPtr].value = getstring(_fStream);

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

        if(_fStream.at(n++) != stext)
            throw std::runtime_error("file `" + exe + "` may be corrupt");

        /* Text section */
        uint64_t bRef=0, extra=0;
        updateStackFile("processing .text section");

        for (;;) {

            __bitFlag = _fStream.at(n++);
            switch (__bitFlag) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_byte: {
                    if(bRef >= manifest.isize)
                        throw std::runtime_error("text section may be corrupt");

                    env->bytecode[bRef] = GET_mi64(
                            SET_mi32(_fStream.at(n), _fStream.at(n+1),
                            _fStream.at(n+2), _fStream.at(n+3)
                        ), SET_mi32(_fStream.at(n+4), _fStream.at(n+5),
                            _fStream.at(n+6), _fStream.at(n+7)
                        )
                    ); n+=8;

                    if(overflowOp(GET_OP(env->bytecode[bRef])))
                    {
                        env->bytecode[++bRef] = GET_mi64(
                                SET_mi32(_fStream.at(n), _fStream.at(n+1),
                            _fStream.at(n+2), _fStream.at(n+3)
                        ), SET_mi32(_fStream.at(n+4), _fStream.at(n+5),
                            _fStream.at(n+6), _fStream.at(n+7)
                        )
                        ); n+=8;
                    }
                    bRef++;
                    break;
                }

                case eos:
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(__bitFlag == eos) {
                if(bRef != manifest.isize)
                    throw std::runtime_error("text section may be corrupt");
                break;
            }
        }
        jobIndx-=2;
    } catch(std::exception &e) {
        updateStackFile("exception thrown: " + string(e.what()));
        return 1;
    }

    return 0;
}

bool overflowOp(int op) {
    return op == MOVI ||
           op == MOVBI;
}

ClassObject *findClass(int64_t superClass) {
    for(uint64_t i = 0; i < manifest.classes; i++) {
        if(env->classes[i].id == superClass)
            return &env->classes[i];
    }

    return NULL;
}

void getMethod(file::stream& exe, ClassObject *parent, Method* method) {
    method->name = getstring(exe);
    method->id = getlong(exe);
    method->entry = getlong(exe);
    method->locals = getlong(exe);
    method->owner = parent;
    method->retAdr = -1;
}

void getField(file::stream& exe, list <MetaField>& mFields, Field* field) {
    field->name = getstring(exe);
    field->id = getlong(exe);
    field->type = (int)getlong(exe);
    field->isstatic = (bool)getlong(exe);
    field->owner = NULL;
    mFields.push_back(MetaField(field, getlong(exe)));
}

string getstring(file::stream& exe) {
    std::string s;
    while(exe.at(n++) != nil) {
        s+=exe.at(n-1);
    }

    return s;
}

int64_t getlong(file::stream& exe) {
    return strtoll(getstring(exe).c_str(), NULL, 0);
}

std::string string_forward(file::stream& str, size_t begin, size_t end) {
    if(begin >= str.size() || end >= str.size())
        throw std::invalid_argument("unexpected end of stream");

    size_t it=0;
    string s;
    for(size_t i = begin; it++ < end; i++)
        s +=str.at(i);

    return s;
}

bool checkFile(file::stream& exe) {
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

void pushStackDump() {
    file::stream s(stackdump.str());
    file::write((manifest.executable + ".stackdump").c_str(), s);
}

void updateStackFile(string status) {
    for(int i = 0; i < jobIndx; i++) {
        stackdump << "\t";
    }
    stackdump << status << endl;
}