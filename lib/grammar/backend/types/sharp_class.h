//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_SHARP_CLASS_H
#define SHARP_SHARP_CLASS_H

#include "../../../../stdimports.h"
#include "../dependency/dependancy.h"
#include "../../List.h"
#include "../../../core/access_flag.h"
#include "../../frontend/parser/Ast.h"
#include "../meta_data.h"
#include "sharp_module.h"
#include "generic_type_identifier.h"
#include "unresolved_extension_function.h"
#include "../../settings/settings.h"

struct code_info;

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
            ci(NULL),
            module(NULL),
            baseClass(NULL),
            genericBuilder(NULL),
            staticInit(NULL),
            used(false),
            genericProcessed(false),
            functions(),
            generics(),
            aliases(),
            fields(),
            interfaces(),
            uncompiledLambdas(),
            genericClones(),
            type(class_normal),
            genericTypes(),
            extensionFunctions(),
            mutations(),
            blueprintClass(false),
            injected(false),
            typesProcessed(false),
            uid(-1),
            obfuscateModifier(options.obfuscate ? modifier_obfuscate : modifier_none)
    {
    }

    sharp_class(
            string &name,
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
            ci(NULL),
            baseClass(NULL),
            genericBuilder(NULL),
            staticInit(NULL),
            used(false),
            genericProcessed(false),
            typesProcessed(false),
            dependencies(),
            owner(owner),
            children(),
            fullName(""),
            functions(),
            generics(),
            aliases(),
            interfaces(),
            uncompiledLambdas(),
            genericClones(),
            fields(),
            type(type),
            genericTypes(),
            extensionFunctions(),
            mutations(),
            blueprintClass(false),
            injected(false),
            uid(-1),
            obfuscateModifier(options.obfuscate ? modifier_obfuscate : modifier_none)
    {
        if(owner == NULL || check_flag(owner->flags, flag_global)) {
            fullName = module->name + "#"
                       + name;
        } else {
            fullName = owner->fullName + "."
                        + name;
        }

        create_dependency(this, owner);
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
            used(false),
            genericProcessed(false),
            typesProcessed(false),
            ci(NULL),
            genericBuilder(NULL),
            staticInit(NULL),
            children(),
            fullName(""),
            functions(),
            generics(),
            uncompiledLambdas(),
            aliases(),
            fields(),
            interfaces(),
            genericClones(),
            type(type),
            genericTypes(),
            extensionFunctions(),
            mutations(),
            blueprintClass(false),
            injected(false),
            uid(-1),
            obfuscateModifier(options.obfuscate ? modifier_obfuscate : modifier_none)
    {
        fullName = module->name + "#"
                   + name;

        create_dependency(this, owner);
    }

    sharp_class(const sharp_class &sc)
    :
            name(sc.name),
            flags(sc.flags),
            module(sc.module),
            baseClass(sc.baseClass),
            genericBuilder(sc.genericBuilder),
            implLocation(sc.implLocation),
            dependencies(sc.dependencies),
            staticInit(sc.staticInit),
            ast(sc.ast),
            used(sc.used),
            owner(sc.owner),
            genericProcessed(sc.genericProcessed),
            ci(NULL),
            injected(sc.injected),
            children(sc.children),
            fullName(sc.fullName),
            functions(sc.functions),
            uncompiledLambdas(sc.uncompiledLambdas),
            generics(sc.generics),
            obfuscateModifier(sc.obfuscateModifier),
            aliases(sc.aliases),
            typesProcessed(sc.typesProcessed),
            fields(sc.fields),
            type(sc.type),
            uid(sc.uid),
            interfaces(sc.interfaces),
            genericClones(sc.genericClones),
            genericTypes(sc.genericTypes),
            extensionFunctions(sc.extensionFunctions),
            blueprintClass(sc.blueprintClass),
            mutations(sc.mutations)
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
    sharp_class *genericBuilder;
    sharp_module *module;
    impl_location implLocation;
    List<dependency> dependencies;
    List<sharp_class*> children;
    List<sharp_class*> interfaces;
    List<sharp_class*> generics;
    List<sharp_alias*> aliases;
    List<sharp_field*> fields;
    List<sharp_class*> genericClones;
    List<sharp_function*> functions;
    List<sharp_function*> uncompiledLambdas;
    List<Ast*> mutations;
    code_info *ci;
    sharp_function *staticInit;
    List<unresolved_extension_function> extensionFunctions;
    List<generic_type_identifier> genericTypes;
    class_type type;
    uInt uid;
    obfuscation_modifier obfuscateModifier;
    bool injected;
    bool blueprintClass;
    bool typesProcessed;
    bool genericProcessed;
    bool used;
};

sharp_class* get_top_level_class(sharp_class *sc);
string get_simple_name(sharp_class *);
void create_global_class();
sharp_class* create_class(sharp_file*, sharp_module*, string, uInt, class_type, bool, Ast*);
sharp_class* create_closure_class(sharp_file*, sharp_module*, sharp_function*, Ast*);
sharp_class* create_class(sharp_file*, sharp_class*, string, uInt, class_type, bool, Ast*);
sharp_class* create_generic_class(sharp_class*, List<sharp_type> &genericTypes, bool&, Ast *ast);
impl_location* get_real_impl_location(sharp_class*);

bool locate_functions_with_name(
        string name,
        sharp_class *owner,
        Int functionType,
        bool checkBaseClass,
        bool ignoreInterfaceFunctions,
        List<sharp_function*> &results);

bool locate_functions_with_type(
        sharp_class *owner,
        Int functionType,
        bool checkBaseClass,
        List<sharp_function*> &results);

bool locate_functions_pointer_fields_with_name(
        string name,
        sharp_class *owner,
        bool checkBaseClass,
        List<sharp_field*> &results);

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
bool is_implicit_type_match(sharp_class*, sharp_class*, bool objBaseClass = false);

// check whether or not a class holds the base class of the class provided
bool is_class_related_to(sharp_class*, sharp_class*, bool objBaseClass = false);
bool inherits_generic_class(sharp_class *comparer, sharp_class *generic);

#endif //SHARP_SHARP_CLASS_H
