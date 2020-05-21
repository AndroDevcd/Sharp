//
// Created by BraxtonN on 2/15/2018.
//
#include "Manifest.h"

void Meta::free() {
    for(unsigned int i = 0; i < files.size(); i++)
        files.get(i).free();
    files.free();
}

String Meta::getLine(uInt line, uInt sourceFile) {
    return files.get(sourceFile).lines.get(line);;
}

bool Meta::hasLine(uInt line, uInt sourceFile) {
    return line >= 0 && line < files.get(sourceFile).lines.size();;
}
