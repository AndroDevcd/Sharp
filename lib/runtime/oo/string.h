//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_STRING_H
#define SHARP_STRING_H

#include <string>
#include "../../../stdimports.h"

class nString {
public:
    nString(string value)
            :
            len(0)
    {
        if(value == "") {
            chars = NULL;
        } else {
            len=value.size();
            chars = (char*)malloc(sizeof(char)*len);
            setstr(value);
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
            setstr(str);
        }
    }

    bool operator==(const string &str) {
        if(str.size() != len) return false;
        for(int64_t i = 0; i < len; i++) {
            if(str.at(i) != chars[i])
                return false;
        }
        return true;
    }

    void operator=(const nString &_str) {
        free();

        if(_str.len>0) {
            len = _str.len;
            chars = (char*)malloc(sizeof(char)*len);
            set(_str);
        }
    }

    void operator+=(const char &c) {
        chars = (char*)realloc(chars,sizeof(char)*(len+1));
        chars[len++]=c;
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
    void setstr(const string& value) {
        for(int64_t i = 0; i < len; i++)
            chars[i] = value.at(i);
    }

    CXX11_INLINE
    void set(const nString& value) {
        for(int64_t i = 0; i < len; i++)
            chars[i] = value.chars[i];
    }
};

/* native String Refrence */
class String {
public:
    int64_t id;
    nString value;
};


#endif //SHARP_STRING_H
