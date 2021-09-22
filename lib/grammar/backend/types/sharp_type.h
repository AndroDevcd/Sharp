//
// Created by BNunnally on 9/1/2021.
//

#ifndef SHARP_SHARP_TYPE_H
#define SHARP_SHARP_TYPE_H

#include "../../../../stdimports.h"
#include "unresolved_type.h"
#include "type_match_result.h"

enum native_type {
    type_int8,
    type_int16,
    type_int32,
    type_int64,
    type_uint8,
    type_uint16,
    type_uint32,
    type_uint64,
    type_function_ptr,
    type_var,
    type_object,
    type_class,
    type_field,
    type_function,
    type_lambda_function,
    type_module,
    type_import_group,
    type_null,
    type_nil,
    type_any,
    type_untyped,
    type_undefined
};

void dispose_function_ptr(sharp_type*);

struct sharp_class;
struct sharp_field;
struct sharp_module;
struct import_group;
struct sharp_function;

struct sharp_type {
    sharp_type()
    :
        _class(NULL),
        field(NULL),
        fun(NULL),
        module(NULL),
        group(NULL),
        unresolvedType(),
        type(type_untyped),
        isArray(false),
        nullable(false)
    {}

    sharp_type(const sharp_type &st)
    :
           _class(NULL),
           field(NULL),
           fun(NULL),
           module(NULL),
           group(NULL),
           unresolvedType(),
           type(type_untyped),
           isArray(false),
           nullable(false)
    {
        copy(st);
    }

    sharp_type(
            sharp_class *sc,
            bool nullable = false,
            bool isArray = false)
    :
            _class(sc),
            field(NULL),
            fun(NULL),
            module(NULL),
            group(NULL),
            unresolvedType(),
            type(type_class),
            isArray(isArray),
            nullable(nullable)
    {}

    sharp_type(
            unresolved_type &unresolvedType)
    :
            _class(NULL),
            field(NULL),
            fun(NULL),
            module(NULL),
            group(NULL),
            unresolvedType(unresolvedType),
            type(type_untyped),
            isArray(false),
            nullable(false)
    {}

    sharp_type(sharp_field *sf)
    :
            _class(NULL),
            field(sf),
            fun(NULL),
            module(NULL),
            group(NULL),
            unresolvedType(),
            type(type_field),
            isArray(false),
            nullable(false)
    {}

    sharp_type(sharp_module *sm)
    :
            _class(NULL),
            field(NULL),
            module(sm),
            fun(NULL),
            group(NULL),
            unresolvedType(),
            type(type_module),
            isArray(false),
            nullable(false)
    {}

    sharp_type(import_group *group)
    :
            _class(NULL),
            field(NULL),
            module(NULL),
            fun(NULL),
            group(group),
            unresolvedType(),
            type(type_import_group),
            isArray(false),
            nullable(false)
    {}

    sharp_type(
            sharp_function *fun,
            bool nullable = false,
            bool isLambda = false,
            bool isArray = false)
    :
            _class(NULL),
            field(NULL),
            module(NULL),
            group(NULL),
            unresolvedType(),
            fun(fun),
            type(isLambda ? type_lambda_function : type_function),
            isArray(isArray),
            nullable(nullable)
    {}

    sharp_type(
            native_type type,
            bool nullable = false,
            bool isArray = false)
    :
            _class(NULL),
            field(NULL),
            fun(NULL),
            module(NULL),
            group(NULL),
            unresolvedType(),
            type(type),
            isArray(isArray),
            nullable(nullable)
    {}

    void copy(const sharp_type &st) {
        _class = st._class;
        field = st.field;
        fun = st.fun;
        type = st.type;
        for(Int i = 0; i < st.unresolvedType.items.size(); i++) {
            unresolvedType.items.add(new unresolved_item(*st.unresolvedType.items.get(i)));
        }

        isArray = st.isArray;
        nullable = st.nullable;
        module = st.module;
        group = st.group;
    }

    ~sharp_type() {
        free();
    }

    void free() {
        unresolvedType.free();
    }

    bool operator==(native_type t) const {
        return type == t;
    }

    sharp_class *_class;
    sharp_field *field;
    sharp_module *module;
    import_group *group;
    sharp_function *fun;
    unresolved_type unresolvedType;
    native_type type;
    bool isArray;
    bool nullable;
};

/**
 * When it comes to comparing types in the language we have 2 ways of checking
 * if they are compatible types, "explicitly" or "implicitly"
 *
 * Explicit type comparing:
 *  - Explicit types are very clearly compatible such as described below:
 *
 *  def foo() {
 *      i : int = 10;
 *      j : int = i; // fields j & i are explicitly compatible
 *  }
 *
 *  As show above explicitly compatible because they share the exact same type
 *
 *  Implicit type comparing:
 *   - Implicit type are more complicated and will be considered
 *     implicitly compatible based on the following criteria:
 *       - The type being assigned holds a shared base class of the other
 *       - The type being assigned holds a 'operator=' function that
 *         holds that exact type or a shared base class
 *       - The type being initialized holds a constructor that holds
 *         that exact type or a shared base class
 *       - The type being initialized holds a 'init(..){}' function that
 *         holds that exact type or a shared base class
 *
 *  class a base c {}
 *  class b base c {}
 *  class c {
 *      c(num: int){ .. }
 *
 *      init(num: int){ .. }
 *
 *      def operator=(num: int){ .. }
 *
 *      num: int;
 *  }
 *
 *  def printNum(c: c) {
 *     println("${c.num}");
 *  }
 *
 *  def foo() {
 *      i : int = 10;
 *      n64 : _int64 = 10;
 *      n32 : _int32 = 100;
 *      a : a = new a();
 *
 *      printNum(i); // implicit call to 'printNum' via c class constructor 'c(num: int){}'
 *
 *      c: c = i;   // implicit call to initialize local var "c" via 'init(num: int){}' initializer
 *
 *      c = i;      // implicit call to 'operator=(num: int){}' via operator overload (note: var "c" is not
 *                     being reassigned. Operator overloads are simply treated as another function call)
 *
 *      c = a;     // implicit reassignment of "c" var via inheritance of the 'c' class inside of the "a" var
 *
 *      n64 = n32; // implicit reassignment od "n64" var since "n32" var are both numbers
 *  }
 *
 *
 * @field excludeMatches
 * exclude the type of matches performed on an implicit type match, this can be used to prevent
 * stack-overflow exceptions in the compiler from over-analyzing classes
 *
 * @return Returns wether or not the types matched explicitly or implicitly
 */
type_match_result is_explicit_type_match(sharp_type, sharp_type);
type_match_result is_implicit_type_match(
        sharp_type,
        sharp_type,
        uInt excludeMatches);
type_match_result is_implicit_type_match(
        sharp_type,
        sharp_type,
        uInt excludeMatches,
        sharp_function *&matchedFun);

sharp_type get_type(sharp_type&);
native_type str_to_native_type(string&);
string type_to_str(sharp_type &t);

#endif //SHARP_SHARP_TYPE_H
