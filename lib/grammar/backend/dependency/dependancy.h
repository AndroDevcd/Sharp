//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_DEPENDANCY_H
#define SHARP_DEPENDANCY_H

#include "../../../../stdimports.h"
#include "../types/sharp_type.h"
#include "../../List.h"
#include "../../frontend/parser/Ast.h"
#include "../meta_data.h"

struct sharp_file;
struct sharp_class;
struct sharp_module;
struct sharp_function;
struct sharp_field;
struct sharp_alias;
struct import_group;
struct operation_schema;
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

void create_dependency(sharp_field* dependee);
void create_dependency(sharp_function* dependee);
void create_dependency(sharp_class* dependee);

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

sharp_field* resolve_function_pointer_field(
        string name,
        import_group *group,
        List<sharp_field*> &parameters,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_field* resolve_function_pointer_field(
        string name,
        sharp_file *file,
        List<sharp_field*> &parameters,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_field* resolve_function_pointer_field(
        string name,
        sharp_module *module,
        List<sharp_field*> &parameters,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_field* resolve_function_pointer_field(
        string name,
        sharp_class *searchClass,
        List<sharp_field*> &parameters,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_field* resolve_local_function_pointer_field(
        string name,
        stored_context_item *context,
        List<sharp_field*> &parameters,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck);

sharp_alias* resolve_alias(string, sharp_module*);
sharp_alias* resolve_alias(string, sharp_file*);
sharp_alias* resolve_alias(string, import_group*);
sharp_alias* resolve_alias(string, sharp_class*);

sharp_field* resolve_field(string, sharp_module*, bool checkBase = false);
sharp_field* resolve_field(string, sharp_file*, bool checkBase = false);
sharp_field* resolve_field(string, import_group*, bool checkBase = false);
sharp_field* resolve_field(string, sharp_class*, bool checkBase = false);

sharp_field* resolve_enum(string, sharp_module*);
sharp_field* resolve_enum(string, sharp_file*);
sharp_field* resolve_enum(string, import_group*);
sharp_field* resolve_enum(string, sharp_class*);

sharp_label* resolve_label(string name, stored_context_item *context);
sharp_field* resolve_local_field(string name, stored_context_item *context, bool ignoreBlockId = false);
sharp_alias* resolve_local_alias(string name, stored_context_item *context);

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

sharp_field* resolve_local_field(string, stored_context_item*, Int);

void check_access(
        string &type,
        string &name,
        uInt flags,
        context &ctx,
        bool inPrimaryClass,
        sharp_class *owner,
        impl_location &location,
        Ast *ast);

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
    resolve_filter_generic_inner_class = 0x800,
    resolve_filter_generic_class = 0x1000,
    resolve_filter_class_function = 0x2000,
    resolve_filter_function = 0x4000,
    resolve_filter_label = 0x8000,
    resolve_filter_local_alias = 0x10000,

    // special case for complete unrestricted access to everything (used for deep type resolving)
    resolve_filter_un_restricted = 0x20000,

    /**
     * quick resolve flag vars
     */
    resolve_generic_type = resolve_filter_generic_class
            | resolve_filter_generic_inner_class,

    resolve_hard_type = resolve_filter_class_alias
            | resolve_filter_alias
            | resolve_filter_class
            | resolve_filter_inner_class
            | resolve_filter_generic_type_param
            | resolve_filter_local_alias
            | resolve_generic_type,

    resolve_all = resolve_filter_local_field
            | resolve_filter_class_field
            | resolve_filter_class_enum
            | resolve_filter_class_alias
            | resolve_filter_alias
            | resolve_filter_class
            | resolve_filter_enum
            | resolve_filter_field
            | resolve_filter_function_address
            | resolve_filter_inner_class
            | resolve_filter_generic_type_param
            | resolve_filter_generic_inner_class
            | resolve_filter_generic_class
            | resolve_filter_class_function
            | resolve_filter_function
            | resolve_filter_label
            | resolve_filter_local_alias,

    resolve_inner_class_type = resolve_filter_class_field
            | resolve_filter_class_enum
            | resolve_filter_class_alias
            | resolve_filter_class_function,

    resolve_unrestricted = resolve_filter_un_restricted
};

void resolve(
        sharp_type &unresolvedType,
        sharp_type &resultType,
        bool ignoreInitialType,
        uInt filter,
        Ast *resolveLocation,
        operation_schema *scheme = NULL);

sharp_type resolve(
        Ast *resolveLocation,
        uInt filter = resolve_hard_type,
        operation_schema *scheme = NULL,
        sharp_class *with_class = NULL);

#endif //SHARP_DEPENDANCY_H
