//
// WHAT THE PROGRAM IS MADE OF, AS THE DEBUGGER NEEDS IT: functions by name,
// which source line an instruction belongs to, which instructions a line
// begins, and the source text. Everything is read from the tables the VM loaded
// from the executable (vm.methods, each function's line table, vm.mdata.files).
//
// NAMES. A function is known by its full name, as stack traces print it:
// `module#class.function` (`app#notebook.add`). A function written outside any
// class is kept by the compiler in a class of its own, `__srt_global`; that
// name is the compiler's business, so here such a function is `app#fill`, both
// when it is shown and when a person types it.
//
// Every name here is the name AS WRITTEN in the source. For a program built
// with -obf that is only so when its name map is loaded (names.h); the name
// the executable carries is then shown beside it, in brackets: carried_note().
//

#ifndef SHARP_DEBUGGER_SYMBOLS_H
#define SHARP_DEBUGGER_SYMBOLS_H

#include "../../runtime/stdimports.h"
#include "../../runtime/types/sharp_function.h"
#include "../../runtime/types/sharp_class.h"
#include "names.h"
#include <vector>

/** a place in the program: an instruction of a function */
struct code_place {
    sharp_function *function = nullptr;
    Int pc = 0;                         // the instruction, counted in words from the function's first
};

// ---- functions

/** a function's name as a person reads and types it: `app#notebook.add`, `app#fill` */
string name_of(sharp_function *function);

/**
 * The functions a person means by `name`: the whole name (`app#notebook.add`),
 * or its end after a `.` or `#` (`notebook.add`, `add`). Functions that share
 * one name (overloads) all come back. When the name fits functions with
 * DIFFERENT names, `ambiguous` is set and they all come back, for the message.
 */
std::vector<sharp_function*> functions_named(const string &name, bool &ambiguous);

/**
 * Is the function part of the standard library (a module `std`, `std.*`,
 * `platform` or `platform.*`), not of the program being debugged?
 */
bool is_library_function(sharp_function *function);

/** the program's own `main` functions: where a person's code starts (empty when none is found) */
std::vector<sharp_function*> main_functions();

// ---- classes

/**
 * The classes a person means by `name`: the whole name (`app#notebook`) or its
 * end after a `.` or `#` (`notebook`). `ambiguous` is set when more than one
 * fits. The classes that hold a module's own functions and variables are not
 * among them: a module is named with globals_of_module().
 */
std::vector<sharp_class*> classes_named(const string &name, bool &ambiguous);

/** the class that holds the variables written outside any class in a module (null: no such module) */
sharp_class* globals_of_module(const string &module);

/** a class's name as a person reads it: `app#notebook` - or `module app`, for the holder of a module's variables */
string name_of(sharp_class *of);

// ---- lines

/** the source line of an instruction (-1: the function has no line table) */
Int line_of(sharp_function *function, Int pc);

/**
 * A source file's name as sdb shows it: without its folders (`notebook.sharp`),
 * or with as many of them as it takes to tell it from the program's other files
 * of that name (`shop/notes.sharp`). What it gives can be typed back.
 * "" when the executable does not say which file.
 */
string shown_file(Int file);

/**
 * ` (app#bh.hy)`: the name the executable carries, to stand after a name as
 * written - "" when the two are the same (no name map, or not renamed).
 */
string carried_note(sharp_function *function);
string carried_note(sharp_class *of);
/** the same for a short name (a field's, a local's): ` (bi)`, or "" when `carried` is what was written */
string carried_note(const string &written, const string &carried);

/** `app#notebook.add  notebook.sharp:41` (or `app#notebook.add  instruction 12` without lines) */
string describe_place(const code_place &place);

/**
 * The source files a person means by `name`: its whole path, or the end of it
 * (`notebook.sharp`, `io/file.sharp`). Each is a place in vm.mdata.files.
 */
std::vector<Int> files_named(const string &name);

/**
 * Where `line` of the file begins: in each function that has code on that
 * line, the first instruction of it. Empty when the line holds no code; then
 * `nextLine` is the first later line that does (-1: there is none).
 */
std::vector<code_place> places_at_line(Int file, Int line, Int &nextLine);

// ---- source text

/** how many lines of the file's text the executable carries (0: it was built without -dbg) */
Int source_line_count(Int file);

/** line `line` of the file (counted from 1) as it was written; "" when it is not there */
string source_line(Int file, Int line);

#endif //SHARP_DEBUGGER_SYMBOLS_H
