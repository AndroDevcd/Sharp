//
// Created by BNunnally on 9/8/2021.
//

#ifndef SHARP_UNRESOLVED_TYPE_H
#define SHARP_UNRESOLVED_TYPE_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../../frontend/parser/Ast.h"

struct unresolved_type;
struct sharp_type;
struct sharp_function;
struct operation_scheme;

enum reference_access_type {
    access_normal = 0,
    access_safe = 1,
    access_forced = 2
};

enum reference_type {
    normal_reference = 0,
    module_reference = 1,
    operator_reference = 2,
    generic_reference = 3,
    function_ptr_reference = 4,
    function_reference = 5
};

struct unresolved_item {
    unresolved_item()
    :
        name(""),
        accessType(access_normal),
        typeSpecifiers(),
        operations(),
        type(normal_reference),
        returnType(NULL),
        ast(NULL)
    {}

    unresolved_item(string name, reference_access_type accessType = access_normal)
    :
            name(name),
            accessType(accessType),
            typeSpecifiers(),
            operations(),
            type(normal_reference),
            returnType(NULL),
            ast(NULL)
    {}

    unresolved_item(
            string name,
            reference_type type = normal_reference,
            reference_access_type accessType = access_normal)
    :
            name(name),
            accessType(accessType),
            typeSpecifiers(),
            operations(),
            type(type),
            returnType(NULL),
            ast(NULL)
    {}

    unresolved_item(
            string name,
            List<sharp_type> &types,
            reference_access_type accessType = access_normal)
    :
            name(name),
            accessType(accessType),
            typeSpecifiers(types),
            operations(),
            type(generic_reference),
            returnType(NULL),
            ast(NULL)
    {}

    unresolved_item(const unresolved_item &item)
    :
            name(""),
            accessType(access_normal),
            typeSpecifiers(),
            operations(),
            type(normal_reference),
            returnType(NULL),
            ast(NULL)
    {}

    ~unresolved_item() {
        free();
    }

    void free();

    void copy(const unresolved_item &item);

    string name;
    reference_access_type accessType;
    reference_type type;
    List<sharp_type> typeSpecifiers;
    List<operation_scheme> operations;
    sharp_type *returnType;
    Ast *ast;
};

struct unresolved_type {
    unresolved_type()
    :
        items()
    {}

    unresolved_type(const unresolved_type &reference)
    :
            items(reference.items)
    {}

    ~unresolved_type() {
        free();
    }

    void free() {
        items.free();
    }

    List<unresolved_item> items;
};

string access_type_to_str(reference_access_type);

#endif //SHARP_UNRESOLVED_TYPE_H