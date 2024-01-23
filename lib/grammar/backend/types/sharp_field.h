//
// Created by BNunnally on 9/1/2021.
//

#ifndef SHARP_SHARP_FIELD_H
#define SHARP_SHARP_FIELD_H

#include "../../../../stdimports.h"
#include "../meta_data.h"
#include "../../../core/access_flag.h"
#include "sharp_type.h"
#include "../dependency/dependancy.h"
#include "../dependency/injection_request.h"
#include "../../compiler_info.h"
#include "../../settings/settings.h"
#include "../compiler/obfuscate_compiler.h"

struct sharp_class;
struct code_info;

enum field_type {
    normal_field,
    localized_field
};

struct sharp_field {
    sharp_field()
    :
            name(""),
            fullName(""),
            owner(NULL),
            implLocation(),
            dependencies(),
            ci(NULL),
            flags(flag_none),
            fieldType(normal_field),
            closure(NULL),
            closureRef(NULL),
            uid(-1),
            staticClosure(false),
            closureSetup(false),
            used(false),
            hasConstValue(false),
            obfuscateModifier(options.obfuscate ? modifier_obfuscate : modifier_none),
            constValue(0),
            block(invalid_block_id),
            type(),
            ast(NULL),
            getter(NULL),
            setter(NULL),
            scheme(NULL),
            preInitScheme(NULL),
            request(NULL)
    {}

    sharp_field(const sharp_field &sf)
    :
            name(sf.name),
            fullName(sf.fullName),
            owner(sf.owner),
            implLocation(sf.implLocation),
            dependencies(sf.dependencies),
            type(sf.type),
            flags(sf.flags),
            obfuscateModifier(sf.obfuscateModifier),
            constValue(sf.constValue),
            uid(sf.uid),
            hasConstValue(sf.hasConstValue),
            ci(NULL),
            fieldType(sf.fieldType),
            closure(sf.closure),
            closureRef(sf.closureRef),
            staticClosure(sf.staticClosure),
            closureSetup(sf.staticClosure),
            used(sf.used),
            ast(sf.ast),
            block(sf.block),
            getter(sf.getter),
            setter(sf.setter),
            scheme(NULL),
            preInitScheme(NULL),
            request(NULL)
    {
        create_scheme(sf.scheme);

        if(sf.request)
            request = new injection_request(*sf.request);
    }

    sharp_field(
            string &name,
            sharp_class *owner,
            impl_location &location,
            sharp_type &type,
            uInt flags,
            field_type ft,
            Ast *ast)
    :
            name(name),
            fullName(""),
            owner(owner),
            implLocation(location),
            dependencies(),
            type(),
            ci(NULL),
            flags(flags),
            fieldType(ft),
            closure(NULL),
            closureRef(NULL),
            uid(-1),
            staticClosure(false),
            closureSetup(false),
            obfuscateModifier(options.obfuscate ? modifier_obfuscate : modifier_none),
            used(false),
            constValue(0),
            hasConstValue(false),
            block(invalid_block_id),
            ast(ast),
            getter(NULL),
            setter(NULL),
            scheme(NULL),
            preInitScheme(NULL),
            request(NULL)
    {
        set_full_name();
        this->type.copy(type);

        create_dependency(this, owner);
    }

    ~sharp_field() {
        free();
    }

    void free();

    void set_full_name();

    void create_scheme(operation_schema *);

    string name;
    string fullName;
    sharp_class *owner;
    sharp_function *getter;
    sharp_function *setter;
    impl_location implLocation;
    List<dependency> dependencies;
    code_info *ci;
    sharp_field* closure, *closureRef;
    operation_schema* scheme, *preInitScheme;
    injection_request *request;
    sharp_type type;
    field_type fieldType;
    Int constValue;
    bool hasConstValue;
    Int block;
    bool staticClosure;
    bool closureSetup;
    bool used;
    obfuscation_modifier obfuscateModifier;
    uInt uid;
    uInt flags;
    Ast* ast;
};

sharp_field* create_field(
        sharp_file*,
        sharp_module*,
        string,
        uInt,
        sharp_type,
        field_type,
        Ast*);

sharp_field* create_field(
        sharp_file*,
        sharp_class*,
        string,
        uInt,
        sharp_type,
        field_type,
        Ast*);

sharp_field* create_closure_field(
        sharp_class*,
        string,
        sharp_type,
        Ast*);

sharp_field* create_local_field(
        sharp_file*,
        context*,
        string,
        uInt,
        sharp_type,
        field_type,
        Ast*);

bool can_capture_closure(sharp_field*);


#endif //SHARP_SHARP_FIELD_H
