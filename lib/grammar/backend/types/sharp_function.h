//
// Created by BNunnally on 9/1/2021.
//

#ifndef SHARP_SHARP_FUNCTION_H
#define SHARP_SHARP_FUNCTION_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../dependency/dependancy.h"
#include "../meta_data.h"
#include "../access_flag.h"
#include "sharp_type.h"
#include "function_type.h"

struct sharp_class;
struct sharp_field;

void set_full_name(sharp_function*);

struct sharp_function {
    sharp_function()
    :
        name(""),
        fullName(""),
        owner(NULL),
        implLocation(),
        dependencies(),
        flags(flag_none),
        ast(NULL),
        parameters(),
        returnType(),
        type(undefined_function)
    {}

    sharp_function(const sharp_function &sf)
            :
            name(sf.name),
            fullName(sf.fullName),
            owner(sf.owner),
            implLocation(sf.implLocation),
            dependencies(sf.dependencies),
            flags(sf.flags),
            ast(sf.ast),
            parameters(),
            returnType(sf.returnType),
            type(sf.type)
    {
        copy_parameters(sf.parameters);
    }

    sharp_function(
            string name,
            sharp_class *owner,
            impl_location location,
            uInt flags,
            Ast *ast,
            List<sharp_field*> &parameters,
            sharp_type &returnType,
            function_type type)
    :
            name(name),
            fullName(""),
            owner(owner),
            implLocation(location),
            dependencies(),
            parameters(),
            flags(flags),
            ast(ast),
            returnType(),
            type(type)
    {
        this->returnType.copy(returnType);
        copy_parameters(parameters);
        set_full_name(this);
    }

    ~sharp_function() {
        free();
    }

    void free();
    void copy_parameters(const List<sharp_field*> &params);

    string name;
    string fullName;
    sharp_class *owner;
    impl_location implLocation;
    List<dependency> dependencies;
    List<sharp_field*> parameters;
    sharp_type returnType;
    function_type type;
    uInt flags;
    Ast* ast;
};

bool is_fully_qualified_function(sharp_function*);
bool function_parameters_match(List<sharp_field*>&, List<sharp_field*>&, bool);


bool create_function(
        sharp_class *sc,
        uInt flags,
        function_type type,
        string &name,
        bool checkBaseClass,
        List<sharp_field*> &params,
        sharp_type &returnType,
        Ast *createLocation);

void create_default_constructor(sharp_class*, uInt, Ast*);

string function_to_str(sharp_function*);

#endif //SHARP_SHARP_FUNCTION_H
