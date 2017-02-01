//
// Created by bknun on 1/8/2017.
//
#include "file.h"
#include <sstream>
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

int file::write(const char *file, string data)
{
    try {
        ofstream f (file);
        if (f.is_open())
        {
            f << data;
            f.close();
            return 0;
        }
        else
            return -1;
    }
    catch(std::bad_alloc& ba){
        return -1;
    }
}

string file::read_alltext(const char *file)
{
    if(!exists(file))
        return "file does not exist!";

    stringstream f;
    f << "";

    try {
        string tmp;
        ifstream input(file);

        while(!input.eof()) {
            tmp = "";
            getline(input, tmp);
            f << tmp;
            f << "\n";
        }
    }
    catch(std::bad_alloc& ba){
        return f.str();
    }
    catch(std::exception& e){
        return f.str();
    }

    return f.str();
}

bool file::endswith(string ext, string file) {
    size_t ext_t = ext.length();
    stringstream extension;
    for(size_t i = file.length() - ext_t; i < file.length(); i++){
        extension << file.at(i);
    }

    return extension.str() == ext;
}
