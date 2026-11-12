//
// Lazy compilation of library function bodies.
//
// A program uses a small part of the standard library, but the compiler used
// to compile the body of EVERY function in every library file: type-check each
// statement and build its operation tree. For a hello-world that was half of
// the whole compile time, spent on functions that are thrown away afterwards
// because nothing calls them.
//
// With lazy compilation (the default; `-eager` turns it off):
//
//   1. when the body of a function from a LIBRARY file (a file under a -L
//      folder) would be compiled, the compiler only writes down where it was
//      - defer_function_body() - and moves on;
//   2. once everything else is compiled, compile_reachable_function_bodies()
//      walks the dependency graph from the program's entry points, exactly as
//      the "mark what is used" step of the finalizer does, and compiles a
//      deferred body the moment the walk reaches its function. Compiling it
//      records what IT depends on, and the walk goes on from there.
//
// Functions that are never reached are never compiled - and never emitted,
// just as before, when they were compiled and then dropped as unused.
//
// The user's own files are always compiled in full, so every error in them is
// reported whether or not the code is called. The price is on the library
// side: an error inside a library function nobody calls goes unnoticed. When
// working on the library itself, compile with `-eager`.
//
// C++ comparison: a function template is only instantiated - and only then
// fully checked - when something uses it. This does the same for ordinary
// library functions.
//

#ifndef SHARP_LAZY_FUNCTIONS_H
#define SHARP_LAZY_FUNCTIONS_H

struct sharp_function;
struct sharp_file;
class Ast;

/**
 * Called at the top of compile_function(). Returns true when the body of
 * `function` was put aside for later (the caller then returns at once), false
 * when it has to be compiled now.
 */
bool defer_function_body(sharp_function *function, Ast *ast);

/** Compile the body of `function` now, if it was put aside. Returns true if it was. */
bool compile_deferred_function_body(sharp_function *function);

/** Step 2 above. Run once, after all files were compiled and before the finalizer. */
void compile_reachable_function_bodies();

/** Is this file part of a library (under one of the -L folders)? */
bool is_library_file(sharp_file *file);

/** For -timings: how many bodies were put aside / compiled after all. */
long deferred_function_count();
long deferred_function_compiled_count();

#endif //SHARP_LAZY_FUNCTIONS_H
