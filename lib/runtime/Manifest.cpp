//
// Created by BraxtonN on 2/15/2018.
//
#include "Manifest.h"

void Meta::free() {

    for(unsigned int i = 0; i < sourceFiles.size(); i++)
        sourceFiles.get(i).free();
    sourceFiles.free();
}

std::string Meta::getLine(long line, long sourceFile) {
    for(unsigned int i = 0; i < sourceFiles.size(); i++) {
        if(sourceFiles.get(i).id == sourceFile) {
            if(line >= 0 && line < sourceFiles.get(i).source_line.size()) {
                return  sourceFiles.get(i).source_line.get(line).str();
            } else
                return "";
        }
    }

    return "";
}

bool Meta::hasLine(long line, long sourceFile) {
    for(unsigned int i = 0; i < sourceFiles.size(); i++) {
        if(sourceFiles.get(i).id == sourceFile) {
            return line >= 0 && line < sourceFiles.get(i).source_line.size();
        }
    }

    return false;
}
