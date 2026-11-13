//
// Created by BNunnally on 9/1/2021.
//

#ifndef SHARP_SHARP_TYPE_H
#define SHARP_SHARP_TYPE_H

#include "../../../../stdimports.h"
#include "unresolved_type.h"
#include "type_match_result.h"
#include "get_component_request.h"
#include "../../../core/data_type.h"

void dispose_function_ptr(sharp_type*);

struct sharp_class;
struct sharp_field;
struct sharp_module;
struct import_group;
struct sharp_function;
struct sharp_label;

struct sharp_type {
    sharp_type()
    :
        _class(NULL),
        field(NULL),
        fun(NULL),
        module(NULL),
        group(NULL),
        componentRequest(NULL),
        label(NULL),
        unresolvedType(),
        type(type_untyped),
        isArray(false),
        nullable(false),
        nullableItems(false),
        arrayElement(false),
        integer(0),
        decimal(0),
        _string(""),
        _char(0),
        _bool(false)
    {}

    sharp_type(const sharp_type &st)
    :
           _class(NULL),
           field(NULL),
           fun(NULL),
           module(NULL),
           group(NULL),
           componentRequest(NULL),
           label(NULL),
           unresolvedType(),
           type(type_untyped),
           isArray(false),
           nullable(false),
           nullableItems(false),
           arrayElement(false),
           integer(0),
           decimal(0),
           _string(""),
           _char(0),
           _bool(false)
    {
        copy(st);
    }

    sharp_type(
            sharp_class *sc,
            bool nullable = false,
            bool isArray = false,
            bool nullableItems = false)
    :
            _class(sc),
            field(NULL),
            fun(NULL),
            module(NULL),
            group(NULL),
            componentRequest(NULL),
            label(NULL),
            unresolvedType(),
            type(type_class),
            isArray(isArray),
            nullable(nullable),
            nullableItems(nullableItems),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(
            unresolved_type &unresolvedType)
    :
            _class(NULL),
            field(NULL),
            fun(NULL),
            module(NULL),
            group(NULL),
            componentRequest(NULL),
            label(NULL),
            unresolvedType(unresolvedType),
            type(type_untyped),
            isArray(false),
            nullable(false),
            nullableItems(false),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(sharp_field *sf)
    :
            _class(NULL),
            field(sf),
            fun(NULL),
            module(NULL),
            group(NULL),
            componentRequest(NULL),
            label(NULL),
            unresolvedType(),
            type(type_field),
            isArray(false),
            nullable(false),
            nullableItems(false),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(get_component_request &componentRequest)
            :
            _class(NULL),
            field(NULL),
            fun(NULL),
            module(NULL),
            group(NULL),
            label(NULL),
            componentRequest(new get_component_request(componentRequest)),
            unresolvedType(),
            type(type_get_component_request),
            isArray(false),
            nullable(false),
            nullableItems(false),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(sharp_module *sm)
    :
            _class(NULL),
            field(NULL),
            module(sm),
            fun(NULL),
            group(NULL),
            componentRequest(NULL),
            label(NULL),
            unresolvedType(),
            type(type_module),
            isArray(false),
            nullable(false),
            nullableItems(false),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(import_group *group)
    :
            _class(NULL),
            field(NULL),
            module(NULL),
            fun(NULL),
            group(group),
            componentRequest(NULL),
            label(NULL),
            unresolvedType(),
            type(type_import_group),
            isArray(false),
            nullable(false),
            nullableItems(false),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(sharp_label *label)
    :
            _class(NULL),
            field(NULL),
            module(NULL),
            fun(NULL),
            group(NULL),
            componentRequest(NULL),
            label(label),
            unresolvedType(),
            type(type_label),
            isArray(false),
            nullable(false),
            nullableItems(false),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(
            sharp_function *fun,
            bool nullable = false,
            bool isLambda = false,
            bool isArray = false,
            bool nullableItems = false)
    :
            _class(NULL),
            field(NULL),
            module(NULL),
            group(NULL),
            componentRequest(NULL),
            label(NULL),
            unresolvedType(),
            fun(fun),
            type(isLambda ? type_lambda_function : type_function),
            isArray(isArray),
            nullable(nullable),
            nullableItems(nullableItems),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type(
            data_type type,
            bool nullable = false,
            bool isArray = false,
            bool nullableItems = false)
    :
            _class(NULL),
            field(NULL),
            fun(NULL),
            module(NULL),
            group(NULL),
            componentRequest(NULL),
            label(NULL),
            unresolvedType(),
            type(type),
            isArray(isArray),
            nullable(nullable),
            nullableItems(nullableItems),
            arrayElement(false),
            integer(0),
            decimal(0),
            _string(""),
            _char(0),
            _bool(false)
    {}

    sharp_type& copy(const sharp_type &st);

    /**
     * `a = b` copies everything `b` owns, like the copy constructor does.
     *
     * Without this C++ copies member by member: the list of unresolved items
     * (unresolvedType) then points at the SAME items from both types, and the
     * first of the two to be destroyed frees them under the other. That was
     * the compiler crash on `"literal" + s` inside a lambda: `type =
     * get_real_type(...)` kept the list of a temporary that was already gone.
     *
     * C++ comparison: the "rule of three" - a class that owns memory and
     * defines a copy constructor and a destructor needs this operator too.
     */
    sharp_type& operator=(const sharp_type &st) {
        if(this != &st) copy(st);
        return *this;
    }

    ~sharp_type() {
        free();
    }

    void free() {
        delete componentRequest;
        componentRequest = NULL;
    }

    bool operator==(data_type t) const {
        return type == t;
    }

    void operator=(data_type t) {
        this->type = t;
    }

    bool operator!=(data_type t) const {
        return type != t;
    }

    sharp_class *_class;
    sharp_field *field;
    sharp_module *module;
    import_group *group;
    sharp_function *fun;
    get_component_request *componentRequest;
    unresolved_type unresolvedType;
    data_type type;
    bool isArray;
    bool nullable;
    bool nullableItems;
    bool arrayElement;
    Int integer;
    string _string;
    char _char;
    bool _bool;
    long double decimal;
    sharp_label *label;
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
uInt is_explicit_type_match(sharp_type&, sharp_type&);
uInt is_implicit_type_match(
        sharp_type&,
        sharp_type&,
        uInt excludeMatches);
uInt is_implicit_type_match(
        sharp_type&,
        sharp_type&,
        uInt excludeMatches,
        sharp_function *&matchedFun);

sharp_type get_real_type(sharp_type&);
sharp_class* get_class_type(sharp_type&);
bool is_numeric_type(sharp_type&);

/**
 * A number in Sharp is one of two KINDS: a decimal (64-bit double: `var` and
 * fractional literals) or an integer (the sized types _int8 .. _uint64,
 * characters, booleans, whole literals, function pointers).
 *
 * The kind decides which instructions work on the value (ADD or DADD, LT or
 * DLT, ...) and when a conversion (I2D / D2I) is needed. The size of an integer
 * does not matter for that: every integer is 64 bits wide in a register and is
 * narrowed when it is stored.
 */
bool is_decimal_type(sharp_type&);
bool is_evaluable_type(sharp_type&);
bool is_object_type(sharp_type&);
data_type str_to_native_type(string&);
string type_to_str(sharp_type &t);
bool has_type(sharp_type &t);

#endif //SHARP_SHARP_TYPE_H
