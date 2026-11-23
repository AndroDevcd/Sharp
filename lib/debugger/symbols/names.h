//
// THE NAMES AS THEY WERE WRITTEN, FOR A PROGRAM WHOSE NAMES WERE REPLACED.
//
// `sharpc -obf` gives every class, field, function, local variable, module and
// source file a short new name (`a`, `b` ... `bh`), and writes the map back to
// the old ones beside the program, `<program>.names`, a line each:
//
//      class  bh  app#point
//      field  bi  app#point.x
//      function  hy  app#point.length_squared
//      local  hz  app#point.length_squared.total
//      module  c  app
//      file  f  /home/me/app/values.sharp
//
// (the compiler: backend/finalizer/obfuscation/names.sharp). The executable
// then carries only the new names. With the map loaded, everything here gives
// the name AS WRITTEN - so a person reads and types `app#point.x` - and the
// name the executable CARRIES is there to show beside it.
//
// Without a map, or for a thing that was not renamed, the written name is the
// carried one.
//
// HOW A NAME IS FOUND BACK. A new name is used once in the whole map, so it
// finds its line - but a thing that was NOT renamed may happen to be called
// what some other thing was renamed to (a function really named `at`). So a
// line is only taken when what stands in front of the name fits too: a class's
// module or outer class, a function's or field's class.
//

#ifndef SHARP_DEBUGGER_NAMES_H
#define SHARP_DEBUGGER_NAMES_H

#include "../../../stdimports.h"
#include "../../runtime/types/sharp_function.h"
#include "../../runtime/types/sharp_class.h"
#include "../../runtime/types/sharp_field.h"

/**
 * Read a name map. Call it once the program is loaded and before anything
 * asks for a name. Gives how many names it holds, or -1 when the file cannot
 * be read.
 */
int load_name_map(const string &file);

/** was a map loaded? */
bool has_name_map();

// the name as written (the whole name: `app#point`, `app#point.length_squared`)
string written_name(sharp_class *of);
string written_name(sharp_function *function);
/** a field's own name as written (`x`); `of` is the class it is a field of */
string written_name(sharp_field &field, sharp_class *of);
/** a local variable's name as written */
string written_name(const local_data &local);
/** the path of source file `file` (its place in vm.mdata.files) as written */
string written_file(Int file);

#endif //SHARP_DEBUGGER_NAMES_H
