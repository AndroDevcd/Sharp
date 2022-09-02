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
#include "../../settings/settings.h"

struct code_info;
struct sharp_class;
struct sharp_field;

void set_full_name(sharp_function*);

struct sharp_function {
    sharp_function()
    :
            name(""),
            fullName(""),
            owner(NULL),
            baseConstructor(NULL),
            implLocation(),
            dependencies(),
            flags(flag_none),
            ast(NULL),
            closure(NULL),
            scheme(NULL),
            delegate(NULL),
            ci(NULL),
            uid(-1),
            used(false),
            obfuscate(options.obfuscate),
            returnProtected(false),
            parameters(),
            returnType(),
            locals(),
            impls(),
            aliases(),
            labels(),
            type(undefined_function),
            directlyCopyParams(false)
    {}

    sharp_function(const sharp_function &sf)
            :
            name(sf.name),
            fullName(sf.fullName),
            owner(sf.owner),
            implLocation(sf.implLocation),
            dependencies(sf.dependencies),
            impls(sf.impls),
            flags(sf.flags),
            uid(sf.uid),
            returnProtected(sf.returnProtected),
            baseConstructor(sf.baseConstructor),
            ast(sf.ast),
            delegate(sf.delegate),
            obfuscate(sf.obfuscate),
            parameters(),
            locals(),
            aliases(),
            labels(),
            used(sf.used),
            returnType(sf.returnType),
            type(sf.type),
            closure(sf.closure),
            directlyCopyParams(sf.directlyCopyParams),
            scheme(NULL),
            ci(NULL)
    {
        copy_parameters(sf.parameters);
        copy_locals(sf.locals);
        copy_scheme(sf.scheme);
    }

    sharp_function(
            string &name,
            sharp_class *owner,
            impl_location location,
            uInt flags,
            Ast *ast,
            List<sharp_field*> &parameters,
            sharp_type &returnType,
            function_type type,
            bool directlyCopyParams = false)
    :
            name(name),
            fullName(""),
            owner(owner),
            implLocation(location),
            dependencies(),
            parameters(),
            flags(flags),
            ast(ast),
            uid(-1),
            returnType(),
            type(type),
            locals(),
            impls(),
            aliases(),
            labels(),
            used(false),
            returnProtected(false),
            directlyCopyParams(directlyCopyParams),
            scheme(NULL),
            baseConstructor(NULL),
            obfuscate(options.obfuscate),
            closure(NULL),
            delegate(NULL),
            ci(NULL)
    {
        this->returnType.copy(returnType);
        if(!directlyCopyParams)
            copy_parameters(parameters);
        else this->parameters.addAll(parameters);
        set_full_name(this);
    }

    ~sharp_function() {
        free();
    }

    void free();
    void copy_parameters(const List<sharp_field*> &params);
    void copy_scheme(operation_schema *operations);
    void copy_locals(const List<sharp_field*> &params);

    string name;
    string fullName;
    sharp_class *owner;
    impl_location implLocation;
    List<dependency> dependencies;
    List<sharp_field*> parameters;
    List<sharp_field*> locals;
    List<sharp_alias*> aliases;
    List<sharp_label*> labels;
    operation_schema *scheme;
    sharp_field* closure;
    sharp_function* delegate;
    List<sharp_function*> impls;
    sharp_type returnType;
    sharp_function* baseConstructor;
    function_type type;
    bool used;
    bool obfuscate;
    bool returnProtected;
    code_info *ci;
    uInt flags;
    uInt uid;
    Ast* ast;
    bool directlyCopyParams; // we need to do this for get() expressions to preserve the resolved type definition found
};

bool is_fully_qualified_function(sharp_function*);
void fully_qualify_function(sharp_function*, sharp_function*);
bool function_parameters_match(List<sharp_field*>&, List<sharp_field*>&, bool, uInt excludedMateches = 0);


bool create_function(
        sharp_class *sc,
        uInt flags,
        function_type type,
        string &name,
        bool checkBaseClass,
        List<sharp_field*> &params,
        sharp_type &returnType,
        Ast *createLocation);

bool create_function(
        sharp_class *sc,
        uInt flags,
        function_type type,
        string &name,
        bool checkBaseClass,
        List<sharp_field*> &params,
        sharp_type &returnType,
        Ast *createLocation,
        sharp_function *&createdFun);

void create_default_constructor(sharp_class*, uInt, Ast*);

string function_to_str(sharp_function*);
string parameters_to_str(List<sharp_field*> &);

#endif //SHARP_SHARP_FUNCTION_H
