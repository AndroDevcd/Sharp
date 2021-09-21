//
// Created by bnunnally on 8/30/21.
//

#ifndef SHARP_SHARP_FILE_H
#define SHARP_SHARP_FILE_H

#include "../../stdimports.h"
#include "frontend/tokenizer/tokenizer.h"
#include "frontend/parser/Parser.h"
#include "backend/dependency/dependancy.h"
#include "backend/types/import_group.h"
#include "compiler_info.h"
#include "backend/context/context.h"

enum compilation_stage {
    not_compiled,
    tokenized,
    parsed,
    classes_preprocessed,
    classes_post_processed, // ready to compile after this stage
    class_delegates_processed,
    compiled
};

#define pre_compilation classes_post_processed

/**
 * Sharp file
 *
 * Fields:
 * @field dependecies
 * This will hold the files that this file depends on for analyzing a
 * much easier chain of dependencies for omitting unused code
 *
 * @field compilationFailed
 * This states whether or not the compilation has failed at any given point
 * during compilation. This will only stop the file from being compiled further if
 * it failed during tokenizing or parsing.
 *
 * @field stage
 * This represents the compilation stage that the file is currently at while it's being
 * processed. This helps the task delegator figure out when to transition the file to the next compilation
 * stage safely.
 *
 * @file imports
 * This represents a list of modules that the file wants to reference when resolving code
 */
struct sharp_file {
    sharp_file()
    :
        name(""),
        tok(NULL),
        p(NULL),
        errors(NULL),
        stage(not_compiled),
        compilationFailed(false),
        dependencies(),
        imports(),
        context(),
        importGroups()
    {}

    sharp_file(string filePath)
            :
            name(filePath),
            tok(NULL),
            p(NULL),
            errors(NULL),
            stage(not_compiled),
            compilationFailed(false),
            dependencies(),
            imports(),
            context(),
            importGroups()
    {}

    ~sharp_file() {
        free();
    }

    void free() {
        dependencies.free();
        imports.free();
        context.free();
        deleteList(importGroups);
    }

    string name;
    tokenizer *tok;
    parser *p;
    context context;
    List<import_group*> importGroups;
    compilation_stage stage;
    bool compilationFailed;
    List<dependency> dependencies;
    List<sharp_module*> imports;
    ErrorManager *errors;
};

extern List<sharp_file*> sharpFiles;

#endif //SHARP_SHARP_FILE_H
