//
// Created by bnunnally on 8/30/21.
//

#ifndef SHARP_SHARP_FILE_H
#define SHARP_SHARP_FILE_H

#include "../../stdimports.h"
#include "frontend/tokenizer/tokenizer.h"
#include "frontend/parser/Parser.h"

enum compilation_stage {
    not_compiled,
    tokenized,
    parsed
};

struct sharp_file {
    sharp_file()
    :
        name(""),
        tok(NULL),
        p(NULL),
        stage(not_compiled),
        compilationFailed(false)
    {}

    sharp_file(string filePath)
            :
            name(filePath),
            tok(NULL),
            p(NULL),
            stage(not_compiled),
            compilationFailed(false)
    {}

    string name;
    tokenizer *tok;
    parser *p;
    compilation_stage stage;
    bool compilationFailed;
};

extern List<sharp_file*> sharpFiles;

#endif //SHARP_SHARP_FILE_H
