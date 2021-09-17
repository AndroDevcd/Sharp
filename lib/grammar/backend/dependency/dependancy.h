//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_DEPENDANCY_H
#define SHARP_DEPENDANCY_H

#include "../../../../stdimports.h"
#include "../types/sharp_type.h"
#include "../../List.h"
#include "../../frontend/parser/Ast.h"

struct sharp_file;
struct sharp_class;
struct sharp_module;
struct sharp_function;
struct sharp_field;
struct sharp_alias;
struct import_group;
struct operation_scheme;
struct context;
struct stored_context_item;

enum dependency_type {
    no_dependency,
    dependency_file,
    dependency_class,
    dependency_function,
    dependency_field
};

/**
 *
 * File dependencies:
 *  - Files
 *
 * class dependencies: File Owner -> files holding each class
 *  - Classes
 *
 * Field dependencies: Class Owner -> classes holding each item
 *  - Classes
 *  - Fields
 *  - Functions
 *
 * Function Dependencies: Class Owner -> classes holding each item
 *  - Fields
 *  - Functions
 *  - Classes
 *
 */
struct dependency {
    dependency()
    :
        fileDependency(NULL),
        classDependency(NULL),
        functionDependency(NULL),
        fieldDependency(NULL),
        type(no_dependency)
    {}

    dependency(const dependency &d)
            :
            fileDependency(d.fileDependency),
            classDependency(d.classDependency),
            functionDependency(d.functionDependency),
            fieldDependency(d.fieldDependency),
            type(d.type)
    {}

    dependency(sharp_file *file)
            :
            fileDependency(file),
            classDependency(NULL),
            functionDependency(NULL),
            fieldDependency(NULL),
            type(dependency_file)
    {}

    dependency(sharp_class *sc)
            :
            fileDependency(NULL),
            classDependency(sc),
            functionDependency(NULL),
            fieldDependency(NULL),
            type(dependency_class)
    {}

    dependency(sharp_function *sf)
            :
            fileDependency(NULL),
            classDependency(NULL),
            functionDependency(sf),
            fieldDependency(NULL),
            type(dependency_function)
    {}

    dependency(sharp_field *sf)
            :
            fileDependency(NULL),
            classDependency(NULL),
            functionDependency(NULL),
            fieldDependency(sf),
            type(dependency_field)
    {}

    bool operator==(const dependency &d) {
        if(type == d.type) {
            switch(type) {
                case no_dependency:
                    return true;
                case dependency_file:
                    return d.fileDependency == fileDependency;
                case dependency_class:
                    return d.classDependency == classDependency;
                case dependency_function:
                    return d.functionDependency == functionDependency;
                case dependency_field:
                    return d.fieldDependency == fieldDependency;
                default: return false;
            }
        } else return false;
    }

    sharp_file *fileDependency;
    sharp_class *classDependency;
    sharp_function *functionDependency;
    sharp_field *fieldDependency;
    dependency_type type;
};

void create_dependency(sharp_class* depender, sharp_class* dependee);
void create_dependency(sharp_file* depender, sharp_file* dependee);
void create_dependency(sharp_function* depender, sharp_function* dependee);
void create_dependency(sharp_function* depender, sharp_class* dependee);
void create_dependency(sharp_function* depender, sharp_field* dependee);
void create_dependency(sharp_field* depender, sharp_function* dependee);
void create_dependency(sharp_field* depender, sharp_class* dependee);
void create_dependency(sharp_field* depender, sharp_field* dependee);

sharp_class* resolve_class(import_group*, string, bool, bool);
sharp_class* resolve_class(sharp_module*, string, bool, bool);
sharp_class* resolve_class(sharp_file*, string, bool, bool);
sharp_class* resolve_class(sharp_class*, string, bool, bool);
sharp_class* resolve_class(string, bool, bool);

import_group* resolve_import_group(sharp_file*, string);

sharp_function* resolve_function(
        string name,
        import_group *group,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_function* resolve_function(
        string name,
        sharp_file *file,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_function* resolve_function(
        string name,
        sharp_module *module,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_function* resolve_function(
        string name,
        sharp_class *searchClass,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_alias* resolve_alias(string, sharp_module*);
sharp_alias* resolve_alias(string, sharp_file*);
sharp_alias* resolve_alias(string, import_group*);
sharp_alias* resolve_alias(string, sharp_class*);

sharp_field* resolve_field(string, sharp_module*);
sharp_field* resolve_field(string, sharp_file*);
sharp_field* resolve_field(string, import_group*);
sharp_field* resolve_field(string, sharp_class*);

sharp_field* resolve_enum(string, sharp_module*);
sharp_field* resolve_enum(string, sharp_file*);
sharp_field* resolve_enum(string, import_group*);
sharp_field* resolve_enum(string, sharp_class*);

bool resolve_function_for_address(
        string,
        sharp_module*,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results);
bool resolve_function_for_address(
        string,
        sharp_file*,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results);
bool resolve_function_for_address(
        string,
        import_group*,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results);
bool resolve_function_for_address(
        string,
        sharp_class*,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results);

sharp_field* resolve_local_field(string, stored_context_item*);

enum resolve_filter {
    resolve_filter_local_field = 0x1,
    resolve_filter_class_field = 0x2,
    resolve_filter_class_enum = 0x4,
    resolve_filter_class_alias = 0x8,
    resolve_filter_alias = 0x10,
    resolve_filter_class = 0x20,
    resolve_filter_enum = 0x40,
    resolve_filter_field = 0x80,
    resolve_filter_function_address = 0x100,
    resolve_filter_inner_class = 0x200,
    resolve_filter_generic_type_param = 0x400,

    // quick resolve flag vars
    resolve_hard_type = 0x638, // searches for only class aliases, func ptr, genericType, global aliases, classes, and inner classes
    resolve_all = 0x7FF
};

void resolve(
        sharp_type &unresolvedType,
        sharp_type &resultType,
        uInt filter,
        Ast *resolveLocation,
        operation_scheme *scheme = NULL);

sharp_type resolve(
        Ast *resolveLocation,
        uInt filter = resolve_hard_type,
        operation_scheme *scheme = NULL);

#endif //SHARP_DEPENDANCY_H
