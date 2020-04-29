//
// Created by BraxtonN on 1/22/2018.
//

#include "File.h"
#include<stdio.h>
#include <fstream>

bool File::exists(const char *file)
{
    std::ifstream infile(file);
    return infile.good();
}

bool is_whitespace(char c)
{
    return ((c ==' ') || (c =='\n') ||
            (c =='\r') || (c =='\t') ||
            (c =='\b') || (c =='\v') ||
            (c =='\f')) ;
}

bool File::empty(const char *file)
{
    buffer data;
    read_alltext(file, data);
    for(long i = 0; i < data.size(); i++)
    {
        if(!is_whitespace(data.at(i)))
            return false;
    }

    return true;
}

int File::write(const char *f, buffer& data)
{
    try {
        FILE* fp=NULL;

        remove( f );
        fp = fopen(f,"wb+");
        if(fp == nullptr)
            return 1;  // could not open file

        if(data.size() > 0) {
            unsigned int p=0;

            do {
                fputc( data.at(p++), fp );
            }while(p<data.size());
        }

        fclose(fp);
        return 0;
    }
    catch(std::bad_alloc& ba){
        return -1;
    }
}

int64_t file_size(FILE *fp)
{
    int64_t len, cur;
    cur = ftell( fp );            /* remember where we are */
    fseek( fp, 0L, SEEK_END );    /* move to the end */
    len = ftell( fp );            /* get position there */
    fseek( fp, cur, SEEK_SET );   /* return back to original position */

    return ( len );
}

void File::read_alltext(const char *f, buffer& _out)
{
    if(!exists(f))
        return;

    try {

        FILE* fp=NULL;
        int64_t len;

        fp = fopen(f,"rb");
        if(fp == nullptr)
            return;  // could not open file


        len = file_size(fp);
        if(len == -1) 1;
        char c;

        do {
            c = getc(fp);
            if(len > 0) {
                _out << c;
            } else {
                break;
            }
        }while(len--);
        fclose(fp);
    }
    catch(std::bad_alloc& ba){
        return;
    }
    catch(std::exception& e){
        return;
    }

}

bool File::endswith(string ext, string file) {
    size_t ext_t = ext.length();
    stringstream extension;
    for(size_t i = file.length() - ext_t; i < file.length(); i++){
        extension << file.at(i);
    }

    return extension.str() == ext;
}

void File::buffer::_push_back(char _C) {
    if(_Data == NULL)
        begin();

    if(actualSize >= rawSize) {
        rawSize+=STREAM_CHUNK;
        void* tptr=realloc(_Data, rawSize * sizeof(char));
        if(tptr == NULL) {
            throw std::bad_alloc();
        }
        _Data=(char* )tptr;
    }

    _Data[actualSize++]=_C;
}

char File::buffer::at(stream_t _X) {
    if(_X >= actualSize || _X < 0) {
        stringstream _s;
        _s << "buffer::at() _X: " << _X << " >= size: " << actualSize;
        throw std::out_of_range(_s.str());
    }
    return _Data[_X];
}

string File::buffer::to_str() {
    stringstream ss;
    for(unsigned long i=0; i < size(); i++) {
        ss<<_Data[i];
    }
    return ss.str();
}

int File::write(const char *f, string data)
{
    try {
        FILE* fp=NULL;

        remove( f );
        fp = fopen(f,"wb+");
        if(fp == nullptr)
            return 1;  // could not open file

        unsigned int p=0;
        do {
            fputc( data.at(p++), fp );
        }while(p<data.size());

        fclose(fp);
        return 0;
    }
    catch(std::bad_alloc& ba){
        return -1;
    }
}
