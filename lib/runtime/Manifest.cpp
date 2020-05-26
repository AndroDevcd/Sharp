//
// Created by BraxtonN on 2/15/2018.
//
#include "Manifest.h"

void Meta::free() {
    for(unsigned int i = 0; i < files.size(); i++)
        files.get(i).free();
    files.free();
}

String Meta::getLine(Int line, Int sourceFile) {
    return files.get(sourceFile).lines.get(line);;
}

bool Meta::hasLine(Int line, Int sourceFile) {
    return line >= 0 && line < files.get(sourceFile).lines.size();;
}
