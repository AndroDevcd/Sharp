//
// Created by bknun on 1/8/2017.
//
#include "file.h"
#include <sstream>
#include<stdio.h>
#include <fstream>

bool file::exists(const char *file)
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

bool file::empty(const char *file)
{
    string data = read_alltext(file);
    for(long i = 0; i < data.size(); i++)
    {
        if(!is_whitespace(data.at(i)))
            return false;
    }

    return true;
}

int file::write(const char *f, string stream)
{
    try {
        FILE* fp=NULL;

        fp = fopen(f,"wb+");
        if(fp == 0)
            return 1;  // could not open file

        unsigned int p=0;
        do {
            fputc( stream.at(p++), fp );
        }while(p<stream.size());
        fclose(fp);
    }
    catch(std::bad_alloc& ba){
        return -1;
    }
}

uint64_t file_size(FILE *fp)
{
    uint64_t len, cur;
    cur = ftell( fp );            /* remember where we are */
    fseek( fp, 0L, SEEK_END );    /* move to the end */
    len = ftell( fp );            /* get position there */
    fseek( fp, cur, SEEK_SET );   /* return back to original position */

    return ( len );
}

string file::read_alltext(const char *f)
{
    if(!exists(f))
        return "";

    try {

        FILE* fp=NULL;
        int64_t len;
        string s;

        fp = fopen(f,"rb");
        if(fp == 0)
            return "";  // could not open file


        len = file_size(fp);
        if(len == -1) 1;
        //printf("buf size %u\n", (unsigned int)len);
        unsigned int p=0;

        do {
            s+= getc(fp);
        }while(p++<(len));
        fclose(fp);
        return s;
    }
    catch(std::bad_alloc& ba){
        return "";
    }
    catch(std::exception& e){
        return "";
    }

}

bool file::endswith(string ext, string file) {
    size_t ext_t = ext.length();
    stringstream extension;
    for(size_t i = file.length() - ext_t; i < file.length(); i++){
        extension << file.at(i);
    }

    return extension.str() == ext;
}
