//
// Created by BraxtonN on 10/6/2019.
//

#ifndef SHARPC_MAIN_H
#define SHARPC_MAIN_H

#include "options.h"


#define progname "sharpc"
#define progvers "0.2.9"

int _bootstrap(int argc, const char* argv[]);
bool startsWith(string &str, string prefix);
bool ends_with(std::string value, std::string ending);
std::string to_lower(std::string s);

#define opt(v) strcmp(argv[i], v) == 0

#endif //SHARPC_MAIN_H
