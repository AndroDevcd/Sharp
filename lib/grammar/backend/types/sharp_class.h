//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_SHARP_CLASS_H
#define SHARP_SHARP_CLASS_H

#include "../../../../stdimports.h"
#include "../dependency/dependancy.h"
#include "../../List.h"
#include "../access_flag.h"
#include "../../frontend/parser/Ast.h"
#include "../meta_data.h"
#include "sharp_module.h"
#include "generic_type_identifier.h"

enum class_type {
    class_normal,
    class_enum,
    class_interface
};

/**
 * Sharp classes
 *
 * Fields:
 * @field dependencies
 * This will hold a list of classes that the sharp class
 * will depend on. To be used later for excluding unused code.
 *
 */
struct sharp_class {
    sharp_class()
    :
        name(""),
        fullName(""),
        flags(flag_none),
        implLocation(),
        dependencies(),
        ast(NULL),
        module(NULL),
        baseClass(NULL),
        mut(),
        functions(),
        generics(),
        aliases(),
        fields(),
        interfaces(),
        type(class_normal),
        genericTypes()
    {
    }

    sharp_class(
            string name,
            sharp_class *owner,
            sharp_module* module,
            impl_location location,
            uInt flags,
            Ast *ast,
            class_type type)
    :
        name(name),
        flags(flags),
        implLocation(location),
        module(module),
        ast(ast),
        baseClass(NULL),
        dependencies(),
        owner(owner),
        children(),
        fullName(""),
        mut(),
        functions(),
        generics(),
        aliases(),
        interfaces(),
        fields(),
        type(type),
        genericTypes()
    {
        if(owner == NULL) {
            fullName = module->name + "#"
                       + name;
        } else {
            fullName = owner->fullName + "."
                        + name;
        }
    }

    sharp_class(
            string name,
            sharp_class *owner,
            sharp_class *baseClass,
            sharp_module *module,
            impl_location location,
            uInt flags,
            Ast *ast,
            class_type type)
    :
            name(name),
            flags(flags),
            implLocation(location),
            module(module),
            ast(ast),
            baseClass(baseClass),
            dependencies(),
            owner(owner),
            children(),
            fullName(""),
            mut(),
            functions(),
            generics(),
            aliases(),
            fields(),
            interfaces(),
            type(type),
            genericTypes()
    {
        fullName = module->name + "#"
                   + name;
    }

    sharp_class(const sharp_class &sc)
    :
         name(sc.name),
         flags(sc.flags),
         module(sc.module),
         baseClass(sc.baseClass),
         implLocation(sc.implLocation),
         dependencies(sc.dependencies),
         ast(sc.ast),
         owner(sc.owner),
         children(sc.children),
         fullName(sc.fullName),
         mut(),
         functions(sc.functions),
         generics(sc.generics),
         aliases(sc.aliases),
         fields(sc.fields),
         type(sc.type),
         interfaces(sc.interfaces),
         genericTypes(sc.genericTypes)
    {
    }

    ~sharp_class() {
        free();
    }

    void free();

    string name;
    string fullName;
    uInt flags;
    Ast* ast;
    sharp_class *owner;
    sharp_class *baseClass;
    sharp_module *module;
    impl_location implLocation;
    List<dependency> dependencies;
    List<sharp_class*> children;
    List<sharp_class*> interfaces;
    List<sharp_class*> generics;
    List<sharp_alias*> aliases;
    List<sharp_field*> fields;
    List<sharp_function*> functions;
    List<generic_type_identifier> genericTypes;
    class_type type;
    recursive_mutex mut;
};

void create_global_class();
sharp_class* create_class(sharp_file*, sharp_module*, string, uInt, class_type, bool, Ast*);
sharp_class* create_closure_class(sharp_file*, sharp_module*, sharp_function*, Ast*);
sharp_class* create_class(sharp_file*, sharp_class*, string, uInt, class_type, bool, Ast*);
sharp_class* create_generic_class(sharp_class*, List<sharp_type> &genericTypes, bool&);

bool locate_functions_with_name(
        string name,
        sharp_class *owner,
        Int functionType,
        bool checkBaseClass,
        List<sharp_function*> &results);

generic_type_identifier* locate_generic_type(
        string name,
        sharp_class *owner);

sharp_field* locate_field(
        string name,
        sharp_class *owner);

/**
 * For an explanation on explicit vs implicit matching please refer to
 * @file sharp_type.h
 *
 * @return Returns whether or not a class has an implicit or explicit match
 */
bool is_explicit_type_match(sharp_class*, sharp_class*);
bool is_implicit_type_match(sharp_class*, sharp_class*);

// check whether or not a class holds the base class of the class provided
bool is_class_related_to(sharp_class*, sharp_class*);

#endif //SHARP_SHARP_CLASS_H
