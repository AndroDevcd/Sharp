//
// Generic functions.
//
//      def largest<T>(a: T, b: T, bigger: (T, T)(var)) : T {
//          if(bigger(a, b)) { return a; }
//          return b;
//      }
//
//      x := largest<int>(p, q, { a, b -> return a.value > b.value; });
//
// How it works - the same way generic CLASSES work in this compiler, by
// "monomorphisation": there is no code for `largest<T>` itself. Each
// combination of types it is used with gets its own ordinary function,
// `largest<std#int>`, `largest<std#string>`, ..., made on first use.
// (C++ templates work like this; Java generics do not.)
//
//   1. DECLARATION. When the post-processor meets a function declaration with
//      a type parameter list it does not create a function. It stores a
//      blueprint on the owning class: name, type parameter names, and the
//      syntax tree of the declaration (register_generic_function).
//
//   2. CALL. `largest<int>(a, b, f)` is renamed at the call site to a call of
//      the function `largest<std#int>` (the "typed name"), and the type
//      arguments are remembered under that name (note_generic_call).
//
//   3. INSTANCE. Function lookup asks for `largest<std#int>` in some class. If
//      the class has no such function but has a blueprint `largest` with one
//      type parameter, the instance is made right there
//      (instantiate_generic_function_for_call): the ordinary function
//      processing runs on the blueprint's syntax tree under the typed name,
//      while T is "bound" to int.
//
//   4. BINDINGS. Nothing is substituted in the syntax tree. While an instance
//      is processed or compiled, a list { T -> std#int } is on a stack, and
//      resolving the plain name `T` looks there first
//      (resolve_generic_function_type_param). The list is stored on the
//      function (sharp_function::genericBindings) and pushed again whenever
//      its body or its inferred return type is compiled, also when that
//      happens much later (lazy library functions) - see generic_binding_scope.
//
// An instance is processed in a fresh, isolated context (isolated_context_scope)
// as if it stood where the blueprint was declared, so it never sees the locals
// of the function whose call triggered it.
//
// Limits of this first version: the type arguments have to be written
// (`largest<int>(...)`, no inference from the arguments), and a function
// pointer to a generic function cannot be taken.
//

#ifndef SHARP_GENERIC_FUNCTIONS_H
#define SHARP_GENERIC_FUNCTIONS_H

#include <string>
#include "../../../List.h"
#include "../../../../core/function_type.h"

struct sharp_class;
struct sharp_function;
struct sharp_type;
class Ast;

/** Does this function declaration have a type parameter list? */
bool is_generic_function_decl(Ast *ast);

/** True while an instance of THIS declaration is being processed: it must then be processed normally, not registered again. */
bool processing_generic_function_instance(Ast *ast);

/** Step 1: keep the declaration as a blueprint on `owner`. */
void register_generic_function(sharp_class *owner, std::string &name, function_type type, Ast *ast);

/** Step 2: remember the type arguments of the call that was renamed to `typedName` (`name<std#int>`). */
void note_generic_call(const std::string &typedName, const std::string &baseName, List<sharp_type> &types);

/** The base name and type arguments noted for `typedName`; false if there are none. */
bool generic_call_info(const std::string &typedName, std::string &baseName, List<sharp_type> &types);

/** Step 3: make the instance `typedName` in `owner` (or a base class of it) if a blueprint fits and it does not exist yet. */
void instantiate_generic_function_for_call(const std::string &typedName, sharp_class *owner);

/** Step 4: is `name` a type parameter of the generic function being processed? Then `result` is the type bound to it. */
bool resolve_generic_function_type_param(const std::string &name, sharp_type &result);

/** A lambda written inside a generic function uses the same type parameters. */
void inherit_generic_bindings(sharp_function *lambda);

/**
 * Puts the type bindings of `function` on the stack for as long as this
 * object lives. For a function that is not a generic instance the list is
 * empty - which still matters: while that function is compiled, the type
 * parameters of whatever generic function happens to be further out mean
 * nothing.
 */
struct generic_binding_scope {
    explicit generic_binding_scope(sharp_function *function);
    ~generic_binding_scope();

    bool pushed;
};

/**
 * No type bindings at all for as long as this object lives. Used while a
 * generic CLASS is instantiated in the middle of a generic function: the
 * class has type parameters of its own, possibly with the same names
 * (`def swap<A, B>(p: pair<A, B>) : pair<B, A>` makes pair<B, A>, whose own A
 * is the function's B).
 */
struct generic_binding_isolation {
    generic_binding_isolation();
    ~generic_binding_isolation();
};

#endif //SHARP_GENERIC_FUNCTIONS_H
