//
// Created by BraxtonN on 1/22/2018.
//

#ifndef SHARP_FILE_H
#define SHARP_FILE_H

#include "../../stdimports.h"

typedef uInt stream_t;
#define STREAM_BASE 0x200
#define STREAM_CHUNK 128

class native_string;

class File
{
public:

    class buffer {
    public:
        buffer()
        :
                _Data(NULL),
                rawSize(0),
                actualSize(0)
        {
            begin();
        }

        buffer(std::string _S)
                :
                actualSize(0)
        {
            begin();
            operator<<(_S);
        }

        void begin() {
            _Data=(char* )malloc(sizeof(char)*STREAM_BASE);
            rawSize=STREAM_BASE;
            actualSize=0;
        }

        ~buffer()
        {
            end();
        }

        void operator=(buffer _D)
        {
            if(_Data == NULL)
                begin();

            actualSize=0;
            for(stream_t i=0; i < _D.size(); i++){
                _push_back(_D._Data[i++]);
                actualSize++;
            }
        }

        buffer& operator<<(const char& _X)
        {
            _push_back(_X);
            return *this;
        }

        buffer& operator<<(const std::string& _X)
        {
            for(stream_t i=0; i < _X.size(); i++) {
                _push_back(_X.at(i));
            }
            return *this;
        }

        buffer& operator<<(buffer& buf)
        {
            for(stream_t i=0; i < buf.size(); i++) {
                _push_back(buf._Data[i]);
            }
            return *this;
        }

        buffer& operator<<(const long long& _X)
        {
            operator<<((char)_X);
            return *this;
        }

        buffer& operator<<(const int& _X)
        {
            operator<<((char)_X);
            return *this;
        }

        buffer& operator<<(const long& _X)
        {
            operator<<((char)_X);
            return *this;
        }

        bool empty() { return actualSize == 0; }
        CXX11_INLINE
        stream_t size() { return actualSize; }
        char at(stream_t _X);

        void end() {
            if(_Data != NULL) {
                std::free(_Data); _Data = NULL;
                rawSize=0;
                actualSize=0;
            }
        }

        string to_str();

        char* data() {
            return _Data;
        }

    private:
        void _push_back(char);
        char* _Data;
        stream_t rawSize;
        stream_t actualSize;
    };

    static void read_alltext(const char *file, buffer& out);

    static int write(const char *file, buffer& data);

    static bool empty(const char *file);

    static bool exists(const char *file);

    static bool endswith(string ext, string f);

    static int write(const char *f, string data);
};


#endif //SHARP_FILE_H
