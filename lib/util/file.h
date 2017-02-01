//
// Created by bknun on 1/8/2017.
//

#ifndef SHARP_FILE_H
#define SHARP_FILE_H

#include "../../stdimports.h"

string read_alltext(string fn);
class file
{
public:

    static string read_alltext(const char *file);

    static int write(const char *file, string data);

    static bool empty(const char *file);

    static bool exists(const char *file);

    static bool endswith(string ext, string f);
};

#endif //SHARP_FILE_H
