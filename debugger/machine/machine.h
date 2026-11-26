//
// THE MACHINE MODE: sdb driven by a program - an editor's debugger - not by a
// person.
//
//      sdb -machine <port> program [arguments]
//
// THE CHANNEL. The editor listens on a port of this computer and starts sdb
// with its number; sdb connects to it. Everything the two say to each other
// goes over that connection, one JSON object on a line each. The program's own
// input and output stay what they are - the process's streams - so the editor's
// console is the program's and nothing of sdb's is mixed into it; and the
// editor can talk to sdb WHILE THE PROGRAM RUNS (set a breakpoint, pause),
// which a prompt on the keyboard cannot.
//
// WHAT THE EDITOR SENDS: a request, with a number of its own choosing that the
// answer carries back.
//
//      {"id": 7, "do": "break", "file": "/home/me/app/notebook.sharp", "line": 41}
//
//      do            with                          answers (beside "id" and "ok")
//      ------------  ----------------------------  ---------------------------------------------
//      start                                       the program runs from its first instruction
//      break         file, line                    number, line   (ok false: why; next: the next line with code)
//      clear         number
//      continue      [fiber]
//      next, into,   [fiber]                       (the steps; the fiber that stopped when none is named)
//       out
//      pause                                       (a "stopped" event follows)
//      threads                                     threads: [{id, name, state, fiber, stopped}]
//      frames        fiber                         frames: [{index, function, file, line}]
//      variables     fiber, frame                  variables: [{name, value, kind, parts, path}]
//      children      fiber, frame, path,           variables: [...], total
//                    [from, count]                 (the fields of an object; elements from..from+count of an array)
//      set           fiber, frame, path, value     value: what it holds now
//      quit                                        (sdb ends, and the program with it)
//
//   ok false comes with "why". Looking at the program (frames, variables,
//   children, set) is only possible while it is stopped.
//
// WHAT sdb SENDS BY ITSELF: events.
//
//      {"event": "ready"}                    the program is loaded, nothing has run: set breakpoints, then "start"
//      {"event": "stopped", "reason": "breakpoint" | "step" | "pause" | "start", "number": 2,
//       "thread": 0, "fiber": 0, "function": "app#notebook.add", "file": "...", "line": 41, "can_step": true}
//      {"event": "running"}
//      {"event": "ended", "code": 0}         the program ran to its end
//      {"event": "say", "text": "..."}       sdb's own words: the name map it found, a note about the build
//
// A path (the `path` of a variable) is what `print` takes (values/values.h): the
// editor hands back the path it was given to ask for a variable's children or
// to set it. `kind` is the value's type in a word; `parts` says whether it can
// be opened. A file is named by its whole path, as the compiler was given it.
//
// Lines are counted from 1, as everywhere in sdb.
//

#ifndef SHARP_DEBUGGER_MACHINE_H
#define SHARP_DEBUGGER_MACHINE_H

#include "../../runtime/stdimports.h"

/** connect to the editor on this port of this computer; false when nobody listens there */
bool machine_connect(int port);

/** a line of sdb's own words, as a "say" event */
void machine_say(const string &line);

/**
 * The machine mode's whole life, on a thread of its own (where the prompt's
 * would be): events as the program stops and runs, requests answered as they
 * come, until the program ends or the editor says quit or goes away.
 */
void run_machine();

#endif //SHARP_DEBUGGER_MACHINE_H
