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
        baseClass(NULL)
    {
    }

    sharp_class(
            string name,
            sharp_class *owner,
            sharp_module* module,
            impl_location location,
            uInt flags,
            Ast *ast)
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
        fullName("")
    {
        fullName = module->name + "#"
                + name;
    }

    sharp_class(
            string name,
            sharp_class *owner,
            sharp_class *baseClass,
            sharp_module *module,
            impl_location location,
            uInt flags,
            Ast *ast)
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
            fullName("")
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
         fullName(sc.fullName)
    {
    }

    ~sharp_class() {
        free();
    }

    void free() {
        dependencies.free();
    }

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
};

void create_global_class();
sharp_class* create_class(sharp_file*, sharp_module*, string, uInt, Ast*);

#endif //SHARP_SHARP_CLASS_H
