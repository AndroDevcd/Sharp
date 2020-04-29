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
    for(unsigned int i = 0; i < files.size(); i++) {
        if(files.get(i).id == sourceFile) {
            if(line >= 0 && line < files.get(i).lines.size()) {
                return  files.get(i).lines.get(line);
            } else
                return String();
        }
    }

    return String();
}

bool Meta::hasLine(uInt line, uInt sourceFile) {
    for(unsigned int i = 0; i < files.size(); i++) {
        if(files.get(i).id == sourceFile) {
            return line >= 0 && line < files.get(i).lines.size();
        }
    }

    return false;
}
