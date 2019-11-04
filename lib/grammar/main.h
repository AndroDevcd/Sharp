//
// Created by BraxtonN on 10/6/2019.
//

#ifndef SHARPC_MAIN_H
#define SHARPC_MAIN_H

#include "options.h"
#include "../runtime/oo/string.h"

class main {

};


#define progname "sharpc"
#define progvers "0.2.587"

int _bootstrap(int argc, const char* argv[]);

extern options c_options;
#define opt(v) strcmp(argv[i], v) == 0

#endif //SHARPC_MAIN_H
