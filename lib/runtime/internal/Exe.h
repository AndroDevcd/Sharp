//
// Created by bknun on 2/18/2017.
//

#ifndef SHARP_EXE_H
#define SHARP_EXE_H

#include "../../../stdimports.h"
#include "../Manifest.h"
#include "../Meta.h"

extern stringstream sdf;
extern uint64_t jobIndx;

int Process_Exe(std::string);

Manifest& getManifest();

Meta& getMetaData();

void pushStackDump();

void updateStackFile(string status);

#endif //SHARP_EXE_H
