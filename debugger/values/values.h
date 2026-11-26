//
// THE VALUES OF THE HELD PROGRAM: finding a variable, a field, an element of an
// array, and saying what it holds.
//
// A value is found by a PATH, the way it is written in the program:
//
//      count               a local variable or a parameter
//      self                the object the function was called on
//      title               a field of that object, as the function itself may write it
//      book.title          a field of an object
//      notes[3]            an element of an array, or of a list (std#list<...>)
//      notes[i]            the same, the number taken from a variable
//      book.notes.data[0]  any mix of the two
//      limit               a static field of the function's own class - or, in a
//                          function written outside any class, a variable of its module
//      app#settings.limit  a static field of a named class
//      app#limit           a variable of a named module
//
// A LOCAL VARIABLE LIVES in a slot of its function's frame - or, when a lambda
// uses it, in the function's closure record (an object made at each call),
// which the lambda is handed too - or, when it is `localized`, at the bottom of
// its fiber's stack: every fiber has its own, and the one shown is the looked-at
// fiber's. (A `localized` variable of a module or class is found there too.) The executable says which (local_data, in
// runtime/types/sharp_function.h), so a variable is read the same way wherever
// it lives, and a lambda can name the variables of the functions around it.
//
// A path names things; it computes nothing and calls nothing.
//
// held_value says what was found AND WHERE IT LIVES (the slot, the cell, the
// array and index): `print` only reads it, and a later `set` can write there.
//
// Everything here reads the VM's own structures where they are - a fiber's
// stack, the objects - and may only be used while the program is held.
//

#ifndef SHARP_DEBUGGER_VALUES_H
#define SHARP_DEBUGGER_VALUES_H

#include "../../runtime/stdimports.h"
#include "../symbols/symbols.h"
#include "../../runtime/memory/sharp_object.h"
#include "../../runtime/memory/vm_stack.h"

class fiber;

/** one call of a held fiber: frame 0 is where it is, frame 1 its caller, and so on (`backtrace` numbers them) */
struct frame_view {
    fiber *task = nullptr;
    Int index = 0;
    code_place place;
    stack_item *slots = nullptr;        // the frame's slots: slot n is slots[n]
};

/** frame `index` of a fiber; false when it has no such frame */
bool frame_at(fiber *task, Int index, frame_view &frame);

enum value_kind {
    VALUE_INTEGER = 0,
    VALUE_UNSIGNED,             // an _uint64: the same 64 bits, read without a sign
    VALUE_DECIMAL,
    VALUE_BOOL,
    VALUE_OBJECT                // an object, an array, or null
};

struct held_value {
    value_kind kind = VALUE_OBJECT;
    int64_t integer = 0;                // VALUE_INTEGER, VALUE_UNSIGNED, VALUE_BOOL
    double decimal = 0;                 // VALUE_DECIMAL
    sharp_object *reference = nullptr;  // VALUE_OBJECT (null: the value is null)

    // where it lives - one of the three:
    vm_cell *cell = nullptr;            // a number in a frame slot or in a field
    object *slot = nullptr;             // a reference in a frame slot, a field or an object array
    sharp_object *array = nullptr;      // a number array ...
    Int element = 0;                    // ... and which element of it
    sharp_object *owner = nullptr;      // the object `slot` is part of (null: a slot of a fiber's stack) - the collector
                                        // must be told when a reference is stored into an object

    bool boolElements = false;          // VALUE_OBJECT: an array whose elements are bools (the VM keeps them as bytes)
    bool holdsText = false;             // VALUE_OBJECT: the variable is one for a text (a `string`), whatever it holds now
    int type = 0;                       // a number: the VM's type of it (data_type: type_int8 ... type_var), which says how wide it is

    // the path that reached it, spelled with the names the EXECUTABLE carries (`bh.bi`): the same as
    // what was typed unless a name map is loaded (symbols/names.h). Set by value_of_path().
    string carried;
};

/**
 * The variables of a frame's function that can be named where the frame is:
 * `self`, the parameters, and the locals that are declared above the line and
 * whose block the line is in. Of two with one name the inner one is kept.
 * A variable declared ON the line is among them too: is_being_declared() says
 * so, because its slot still holds whatever was there before.
 * Empty when the executable carries no names (built without -dbg, or with -O).
 */
std::vector<const local_data*> locals_in_view(const frame_view &frame);

/** is the frame on the very line that declares this local (so the line has not given it its value yet)? */
bool is_being_declared(const frame_view &frame, const local_data &local);

/** what a local holds; false (and `problem` says why) when it cannot be read from the frame */
bool value_of_local(const frame_view &frame, const local_data &local, held_value &value, string &problem);

/** the value a path names, seen from a frame; false (and `problem` says why) when it names nothing */
bool value_of_path(const frame_view &frame, const string &path, held_value &value, string &problem);

/**
 * Give what a path named a new value, written where it lives:
 *
 *      a number        `12`, `-3`, `2.5` - a whole number for a whole-number variable; a number
 *                      too large for the variable's type wraps round, as an assignment in the
 *                      program would
 *      a bool          `true`, `false`
 *      a text          `"milk"` (with \n \t \" \\ as in the program) - for a variable that holds a
 *                      text, an `_int8[]`, or null. The text is a NEW object, made on one of the
 *                      program's own threads (control.h, sdb_run_on_held_thread)
 *      an object       `null`; sdb copies no objects and makes no others
 *
 * `value` is brought up to date. False (and `problem` says why) when `text`
 * is not something the value can take.
 */
bool set_value(held_value &value, const string &text, string &problem);

/**
 * A value in one line: `5`, `true`, `"milk"`, `null`, `app#notebook`.
 * An array and a list (std#list<...>) show their elements on the line, the
 * first 15 and then `...`:  `_int64[4] [10, 20, 30, 40]`,
 * `std#list<std#string> (2) ["milk", "bread"]`.
 */
string text_of(const held_value &value);

/** one part of a value: a field of an object, or an element of an array or list */
struct value_part {
    string name;                // how it is named after the value's own path: `.title`, `[3]`
    string shown;               // how it is shown beside its value: `title`, `[3]`
    held_value value;
};

/**
 * The parts of a value, as values - what an editor shows when a variable is
 * opened: the fields of a class instance, or the elements [from, from + count)
 * of an array or a list. `total` is how many parts there are in all. Nothing
 * for a number, a text and null.
 */
std::vector<value_part> parts_between(const held_value &value, Int from, Int count, Int &total);

/** has the value parts to open? */
bool has_parts(const held_value &value);

/** what kind of thing a value is, in a word: `_int64`, `decimal`, `bool`, `std#string`, `app#point`, `_int64[4]`, `null` */
string kind_of(const held_value &value);

/**
 * What an object is made of, one line each: the fields of a class instance
 * (`title = "shopping"`). Empty for everything text_of() says whole on its
 * line: a number, a text, null, an array, a list.
 */
std::vector<string> parts_of(const held_value &value);

#endif //SHARP_DEBUGGER_VALUES_H
