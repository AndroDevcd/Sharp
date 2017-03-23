//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_STRING_H
#define SHARP_STRING_H

#include <string>
#include "../../../stdimports.h"

/* native String Refrence */
class String {
public:
    int64_t id;
    string value;
};

class nString {
public:
    nString(string value)
            :
            len(0)
    {
        if(value == "") {
            chars = NULL;
        } else {
            chars = (char*)malloc(sizeof(char)*len);
            set(value);
        }
    }

    nString()
            :
            len(0)
    {
        chars = NULL;
    }

    void init() {
        len=0;
        chars=NULL;
    }

    ~nString()
    {
        free();
    }

    void operator=(const string &str) {
        free();

        if(str != "") {
            len = str.size();
            chars = (char*)malloc(sizeof(char)*len);
            set(str);
        }
    }

    void operator=(const nString &_str) {
        free();

        const string s = string(_str.chars,_str.len);
        if(s != "") {
            len = s.size();
            chars = (char*)malloc(sizeof(char)*len);
            set(s);
        }
    }

    void operator+=(const string &s) {
        // TODO: implement
    }

    CXX11_INLINE
    string str() {
        if(len==0) return "";
        return string(chars,len);
    }

    void free() {
        if(len != 0) {
            std:: free(chars); chars = NULL;
            len = 0;
        }
    }

    char *chars;
    int64_t len;

private:

    CXX11_INLINE
    void set(string value) {
        for(int64_t i = 0; i < len; i++)
            chars[i] = value.at(i);
    }
};


#endif //SHARP_STRING_H
