//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_STRING_H
#define SHARP_STRING_H

#include <string>
#include "../../../stdimports.h"

class native_string {
public:
    native_string(string value)
            :
            len(0)
    {
        if(value == "") {
            chars = NULL;
        } else {
            len=value.size();
            chars = (char*)malloc(sizeof(char)*len);
            assign(value);
        }
    }

    native_string()
    {
        init();
    }

    void init() {
        len=0;
        chars=NULL;
    }

    ~native_string()
    {
        free();
    }

    void operator=(const string &str) {
        free();

        if(!str.empty()) {
            len = str.size();
            chars = (char*)malloc(sizeof(char)*len);
            assign(str);
        }
    }

    void operator+=(const string &str) {

        if(str != "") {
            len += str.size();
            for(char c : str) {
                operator+=(c);
            }
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

    bool operator==(const native_string &str) {
        if(str.len != len) return false;
        for(int64_t i = 0; i < len; i++) {
            if(str.chars[i] != chars[i])
                return false;
        }
        return true;
    }

    bool operator!=(const string &str) {
        return !operator==(str);
    }

    void operator=(const native_string &_str) {
        free();

        if(_str.len>0) {
            len = _str.len;
            chars = (char*)malloc(sizeof(char)*_str.len);
            std::memcpy(chars, _str.chars, sizeof(char)*len);
        }
    }

    void operator+=(const char &c) {
        if(len == 0) {
            len=1;
            chars=(char*)malloc(sizeof(char)*len);
            chars[0]=c;
        } else {
            chars = (char*)realloc(chars,sizeof(char)*(len+1));
            chars[len++]=c;
        }
    }

    CXX11_INLINE
    string str() {
        if(len==0) return "";
        return string(chars,len);
    }

    const char* c_str() {
        return chars;
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
    void assign(const string &value) {
        for(int64_t i = 0; i < len; i++)
            chars[i] = value.at(i);
    }

    CXX11_INLINE
    void set(const native_string& value) {
        for(int64_t i = 0; i < len; i++)
            chars[i] = value.chars[i];
    }
};

/* native String Refrence */
class String {
public:
    int64_t id;
    native_string value;
};

#endif //SHARP_STRING_H
