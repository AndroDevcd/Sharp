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
#include "backend/context/context.h"

enum compilation_stage {
    not_compiled=0,
    tokenized,
    parsed,
    classes_preprocessed,
    imports_processed,
    classes_post_processed, // ready to compile after this stage
    generics_processed,
    components_processed,
    class_delegates_processed,
    mutations_compiled,
    members_compiled,
    compiled
};

#define pre_compilation_state classes_post_processed
#define compilation_ready(state) ((state) >= class_delegates_processed)

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
        module(NULL),
        used(false),
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
            module(NULL),
            used(false),
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

    void free();

    string name;
    tokenizer *tok;
    parser *p;
    context context;
    sharp_module *module;
    List<import_group*> importGroups;
    List<sharp_field*> fields;
    List<sharp_function*> functions;
    List<sharp_class*> classes;
    List<sharp_alias*> aliases;
    compilation_stage stage;
    bool compilationFailed;
    bool used;
    List<dependency> dependencies;
    List<sharp_module*> imports;
    ErrorManager *errors;
};

extern List<sharp_file*> sharpFiles;



#endif //SHARP_SHARP_FILE_H
