//
// Created by bknun on 2/18/2017.
//

#include "Exe.h"
#include "../../util/file.h"
#include "../startup.h"
#include "Environment.h"

#define offset 0xf
#define file_sig 0x0f
#define digi_sig1 0x07
#define digi_sig2 0x1b
#define digi_sig3 0x0c
#define eoh 0x03
#define nil 0x0
#define hsz 0x0c
#define sdata 0x05
#define scode 0x0e
#define smeta 0x06
#define data_class 0x2f
#define data_method 0x4c
#define data_var 0x22


Manifest manifest;
Meta meta;

uint64_t n = 0;

bool checkFile(string exe);

string getstring(string exe);

double getnumber(string exe);

int64_t getlong(string exe);

int Process_Exe(std::string exe)
{
    string f = "";
    int fl, ct=0;

    if(!file::exists(exe.c_str())){
        error("file `" + exe + "` doesnt exist!");
    }

    f = file::read_alltext(exe.c_str());
    if(f == "")
        return 1;

    try {
        if(!checkFile(f)) {
            error("file `" + exe + "` could not be ran");
        }

        for (;;) {

            ct++;
            fl = f.at(n++);
            switch (fl) {
                case 0x0:
                case 0x0a:
                case eoh:
                    ct--;
                    break;

                case 0x1:
                    manifest.application =getstring(f);
                    break;
                case 0x2:
                    manifest.version =getstring(f);
                    break;
                case 0x4:
                    manifest.debug = f.at(n++) != nil;
                    break;
                case 0x5:
                    manifest.entry =getlong(f);
                    break;
                case 0x6:
                    manifest.methods =getlong(f);
                    break;
                case 0x7:
                    manifest.classes =getlong(f);
                    break;
                case 0x8:
                    manifest.fvers =getlong(f);
                    break;
                case 0x9:
                    manifest.isize =getlong(f);
                    break;
                case 0x11:
                    manifest.addrs =getlong(f);
                    break;
                case 0x12:
                    manifest.laddrs =getlong(f);
                    break;
                case 0x13:
                    manifest.saddrs =getlong(f);
                    break;
                case 0x14:
                    manifest.classes =getlong(f);
                    break;
                default:
                    error("file `" + exe + "` may be corrupt");
                    return 1; /* for lint */
            }

            if(fl == eoh) {
                if(ct != hsz || manifest.target != mvers)
                    return 1;

                break;
            }
        }

        if(f.at(n++) != sdata)
            error("file `" + exe + "` may be corrupt");

        /* Data section */
        for (;;) {

            fl = f.at(n++);
            switch (fl) {
                case data_class:
                    string name = getstring(f);

                    break;
                default:
                    error("file `" + exe + "` may be corrupt");
                    return 1; /* for lint */
            }
        }
    } catch(std::exception &e) {
    }

    return 0;
}

string getstring(string exe) {
    std::string s;
    while(exe.at(n) != nil) {
        s+=exe.at(n++);
    }

    return s;
}

double getnumber(string exe) {
    return atof(getstring(exe).c_str());
}

int64_t getlong(string exe) {
    return strtoll(getstring(exe).c_str(), NULL, 10);
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
