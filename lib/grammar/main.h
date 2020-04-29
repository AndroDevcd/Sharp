//
// Created by BraxtonN on 10/6/2019.
//

#ifndef SHARPC_MAIN_H
#define SHARPC_MAIN_H

#include "options.h"


#define progname "sharpc"
#define progvers "0.2.805"

int _bootstrap(int argc, const char* argv[]);
bool startsWith(string &str, string prefix);
bool ends_with(std::string const & value, std::string const & ending);

#define opt(v) strcmp(argv[i], v) == 0

#endif //SHARPC_MAIN_H
