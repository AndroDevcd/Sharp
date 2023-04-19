//
// Created by BNunnally on 9/6/2021.
//

#ifndef SHARP_SHARP_ALIAS_H
#define SHARP_SHARP_ALIAS_H

#include "../../../../stdimports.h"
#include "sharp_type.h"
#include "../dependency/dependancy.h"
#include "../../../core/access_flag.h"
#include "../meta_data.h"
#include "../operation/operation.h"
#include "../../compiler_info.h"

void set_full_name(sharp_alias*);

struct sharp_alias {
    sharp_alias()
    :
            name(""),
            fullName(""),
            owner(NULL),
            type(),
            flags(flag_none),
            dependencies(),
            block(invalid_block_id),
            location(),
            scheme(NULL),
            ast(NULL)
    {}

    sharp_alias(
            string name,
            sharp_class *owner,
            uInt flags,
            impl_location location,
            Ast *ast)
    :
            name(name),
            owner(owner),
            type(),
            flags(flags),
            location(location),
            block(invalid_block_id),
            scheme(NULL),
            ast(ast)
    {
        set_full_name(this);
    }

    sharp_alias(const sharp_alias &alias)
    :
            name(alias.name),
            type(alias.type),
            owner(alias.owner),
            fullName(alias.fullName),
            dependencies(alias.dependencies),
            block(alias.block),
            flags(alias.flags),
            location(alias.location),
            scheme(new operation_schema(*alias.scheme)),
            ast(alias.ast)
    {}

    sharp_alias(
            string name,
            sharp_class *owner,
            uInt flags,
            sharp_type type,
            impl_location location,
            Ast *ast)
    :
            name(name),
            owner(owner),
            dependencies(),
            type(type),
            flags(flags),
            location(location),
            block(invalid_block_id),
            scheme(NULL),
            ast(ast)
    {
        set_full_name(this);
    }

    ~sharp_alias()
    {
        free();
    }

    void free();

    string name;
    string fullName;
    sharp_class *owner;
    sharp_type type;
    uInt flags;
    Int block;
    operation_schema *scheme;
    impl_location location;
    List<dependency> dependencies;
    Ast *ast;
};

sharp_alias* create_alias(sharp_file*, sharp_class*, string, uInt, Ast*);
sharp_alias* create_alias(sharp_file*, sharp_module*, string, uInt, Ast*);

sharp_alias* create_local_alias(
        sharp_file*,
        context*,
        string,
        uInt,
        Ast*);


#endif //SHARP_SHARP_ALIAS_H
