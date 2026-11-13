//
// Closures: how a lambda reaches the variables of the function that made it.
//

#ifndef SHARP_CLOSURE_CAPTURE_H
#define SHARP_CLOSURE_CAPTURE_H

#include "../../../../stdimports.h"

struct sharp_class;
struct sharp_field;
struct sharp_function;
struct operation_schema;
struct context;
class Ast;

/**
 * A LAMBDA CARRIES WHAT IT CAPTURED
 *
 *      def make_counter() : ()(int) {
 *          total := new box();
 *          return { -> (int) total.value++; return total.value; };
 *      }
 *
 * The lambda uses `total`, a local of make_counter. This is how it gets it:
 *
 *   1. make_counter gets a "closure object": an instance of a class made for
 *      it (closure_<function>) with one field per captured variable. The
 *      object is created on EVERY CALL of make_counter and kept in a hidden
 *      local, `__closure`. Whenever make_counter writes `total`, the value is
 *      written to the closure object too.
 *
 *   2. The lambda gets one hidden LAST parameter, `__captured`. It is not part
 *      of the lambda's type - `()(int)` stays `()(int)` - and the lambda stays
 *      a static function. Inside the lambda `total` reads
 *      `__captured.total`.
 *
 *   3. The function value that make_counter builds for the lambda holds the
 *      closure object (function_value.captured), and a call through the value
 *      passes it as that last parameter (lib/runtime/memory/function_value.h).
 *
 * So the variables travel with the function value: to another fiber, out of
 * the function that made them, and each call of make_counter - also when it
 * calls itself - has its own set.
 *
 *      C++ comparison: a lambda that captures by reference a struct of the
 *      enclosing function's variables, where the struct is held by a
 *      shared_ptr so it outlives the function.
 *
 * (Before: one closure object per enclosing function PER FIBER, found through
 *  a fiber-local static slot. Another fiber found an empty one, and a function
 *  that called itself shared one between its calls.)
 *
 * A LAMBDA INSIDE A LAMBDA
 *
 *      def outer() {
 *          a := new box();
 *          run({ ->  run({ -> a.value = 1; }); });
 *      }
 *
 * The inner lambda receives the closure object of the lambda around it, and
 * that object has a `__parent` field: the closure object the middle lambda
 * received itself. `a` is `__captured.__parent.a`.
 *
 * `self` USED IN A LAMBDA is captured the same way, as the field `__@self`.
 */

/**
 * The function being compiled (ctx.functionCxt, a lambda) uses `local`, which
 * belongs to the enclosing function `owner`. Adds to `scheme` the steps that
 * reach the captured copy and returns the field of the closure class that
 * holds it (NULL after an internal error, which is reported).
 */
sharp_field* capture_enclosing_local(
        sharp_function *owner,
        sharp_field *local,
        context &ctx,
        operation_schema *scheme,
        Ast *ast);

/** the same for `self` of the enclosing instance function `owner` */
sharp_field* capture_enclosing_self(
        sharp_function *owner,
        sharp_class *selfClass,
        context &ctx,
        operation_schema *scheme,
        Ast *ast);

// names the generator looks for (finalizer/generation/function_generator.cpp)
#define closure_parent_field_name "__parent"
#define closure_self_field_name "__@self"

#endif //SHARP_CLOSURE_CAPTURE_H
