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

bool checkFile(string exe);

string getstring(string exe);

double getnumber(string exe);

int64_t getlong(string exe);

void getField(string exe, list <MetaField>& mFields, Field* field);

void getMethod(string exe, ClassObject *parent, Method* method);

ClassObject *findClass(int64_t superClass);

int Process_Exe(std::string exe)
{
    string f = "";
    int fl, ct=0;

    jobIndx++;
    updateStackFile("reading executable");
    if(!file::exists(exe.c_str())){
        error("file `" + exe + "` doesnt exist!");
    }

    manifest.executable = exe;
    f = file::read_alltext(exe.c_str());
    if(f == "")
        return 1;

    try {
        if(!checkFile(f)) {
            error("file `" + exe + "` could not be ran");
        }

        jobIndx++;
        updateStackFile("processing manifest");
        bool manifestFlag = false;
        for (;;) {

            ct++;
            fl = f.at(n++);
            switch (fl) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                case eoh:
                    ct--;
                    break;

                case manif:
                    ct--;
                    manifestFlag = true;
                    break;

                case 0x2:
                    manifest.application =getstring(f);
                    break;
                case 0x4:
                    manifest.version =getstring(f);
                    break;
                case 0x5:
                    manifest.debug = f.at(n++) != nil;
                    break;
                case 0x6:
                    manifest.entry =getlong(f);
                    break;
                case 0x7:
                    manifest.methods =getlong(f);
                    break;
                case 0x8:
                    manifest.classes =getlong(f);
                    break;
                case 0x9:
                    manifest.fvers =getlong(f);
                    break;
                case 0x0b:
                    manifest.isize =getlong(f);
                    break;
                case 0x0c:
                    manifest.strings =getlong(f);
                    break;
                case 0x0e:
                    manifest.baseaddr =getlong(f);
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(fl == eoh) {
                if(!manifestFlag)
                    throw std::runtime_error("missing manifest flag");
                if(manifest.fvers != 1)
                    throw std::runtime_error("unknown file version");

                if(ct != hsz || manifest.target > mvers)
                    return 1;

                break;
            }
        }

        if(f.at(n++) != sdata)
            throw std::runtime_error("file `" + exe + "` may be corrupt");

        /* Data section */
        list<MetaClass> mClasses;
        list<MetaField> mFields;
        int64_t cRef=0, mRef=0;
        updateStackFile("processing .data section");

        env->classes = new ClassObject[manifest.classes]();
        env->objects = new gc_object[manifest.classes]();
        env->methods = new Method[manifest.methods]();
        env->strings = new String[manifest.strings]();
        env->bytecode = new double[manifest.isize];

        for (;;) {

            fl = f.at(n++);
            switch (fl) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_class: {
                    int64_t fc=0, mc=0;
                    ClassObject* c = &env->classes[cRef];
                    mClasses.push_back(MetaClass(c, getlong(f)));

                    c->id = getlong(f);
                    c->name = getstring(f);
                    c->fieldCount = getlong(f);
                    c->methodCount = getlong(f);
                    if(c->fieldCount != 0) {
                        c->flds = new Field[c->fieldCount];
                    } else
                        c->flds = NULL;
                    if(c->methodCount != 0) {
                        c->methods = new Method[c->methodCount];
                    } else
                        c->methods = NULL;
                    c->super = NULL;
                    c->fields = NULL;

                    if(c->fieldCount != 0) {
                        for( ;; ) {
                            if(f.at(n) == data_field) {
                                n++;
                                getField(f, mFields, &c->flds[fc++]);
                            } else if(f.at(n) == 0x0a || f.at(n) == 0x0d) {
                                n++;
                            } else
                                break;
                        }

                        if(fc != c->fieldCount) {
                            throw std::runtime_error("invalid field size");
                        }
                    }

                    if(c->methodCount != 0) {
                        for( ;; ) {
                            if(f.at(n) == data_method) {
                                n++;
                                getMethod(f, c, &c->methods[mc]);

                                if(manifest.entry == c->methods[mc].id)
                                    manifest.main = &c->methods[mc];
                                mc++;
                            } else if(f.at(n) == 0x0a || f.at(n) == 0x0d){
                                n++;
                            } else
                                break;
                        }

                        if(mc != c->methodCount) {
                            throw std::runtime_error("invalid method size");
                        }
                    }

                    cRef++;
                    break;
                }

                case data_method:
                    getMethod(f, NULL, &env->methods[mRef++]);
                    break;
                case sstring:
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(fl == sstring) {
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
        int64_t sRef=0, sc=0;
        updateStackFile("processing .string section");

        for (;;) {

            fl = f.at(n++);
            switch (fl) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_string: {
                    sc++;
                    env->strings[sRef].id = getlong(f);
                    env->strings[sRef].value = getstring(f);

                    sRef++;
                    break;
                }

                case eos:
                    break;

                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(fl == eos) {
                if(sc != manifest.strings)
                    throw std::runtime_error("invalid string size");
                break;
            }
        }

        if(f.at(n++) != stext)
            throw std::runtime_error("file `" + exe + "` may be corrupt");

        /* Text section */
        uint64_t dc=0, bRef=0;
        updateStackFile("processing .text section");

        for (;;) {

            fl = f.at(n++);
            switch (fl) {
                case 0x0:
                case 0x0a:
                case 0x0d:
                    break;

                case data_byte: {
                    dc++;
                    int bytes = f.at(n++);
                    if(bRef >= manifest.isize || (bRef+bytes) >= manifest.isize)
                        throw std::runtime_error("text section may be corrupt");

                    env->bytecode[bRef++] = f.at(n++); // instruction
                    for(int i = 0; i < bytes; i++) {
                        dc++;
                        env->bytecode[bRef++] = getnumber(f); // operands
                    }
                }

                case eos:
                    break;
                default:
                    throw std::runtime_error("file `" + exe + "` may be corrupt");
            }

            if(fl == eos) {
                if(dc != manifest.isize)
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

ClassObject *findClass(int64_t superClass) {
    for(uint64_t i = 0; i < manifest.classes; i++) {
        if(env->classes[i].id == superClass)
            return &env->classes[i];
    }

    return NULL;
}

void getMethod(string exe, ClassObject *parent, Method* method) {
    method->name = getstring(exe);
    method->id = getlong(exe);
    method->entry = getlong(exe);
    method->locals = getlong(exe);
    method->owner = parent;
    method->ret = -1;
}

void getField(string exe, list <MetaField>& mFields, Field* field) {
    field->name = getstring(exe);
    field->id = getlong(exe);
    field->type = (int)getlong(exe);
    field->isstatic = (bool)getlong(exe);
    field->owner = NULL;
    mFields.push_back(MetaField(field, getlong(exe)));
}

string getstring(string exe) {
    std::string s;
    while(exe.at(n++) != nil) {
        s+=exe.at(n-1);
    }

    return s;
}

double getnumber(string exe) {
    return atof(getstring(exe).c_str());
}

int64_t getlong(string exe) {
    return strtoll(getstring(exe).c_str(), NULL, 0);
}

std::string string_forward(std::string str, size_t begin, size_t end) {
    if(begin >= str.size() || end >= str.size())
        throw new std::invalid_argument("unexpected end of stream");

    size_t it=0;
    string s;
    for(size_t i = begin; it++ < end; i++)
        s +=str.at(i);

    return s;
}

bool checkFile(string exe) {
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

Manifest& getManifest() {
    return manifest;
}

Meta& getMetaData() {
    return meta;
}

void pushStackDump() {
    file::write((manifest.executable + ".stackdump").c_str(), stackdump.str());
}

void updateStackFile(string status) {
    for(int i = 0; i < jobIndx; i++) {
        stackdump << "\t";
    }
    stackdump << status << endl;
}