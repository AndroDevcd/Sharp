//
// THE FULL-SCREEN VIEW: sdb takes the whole terminal and keeps it in boxes.
//
//      ┌ output ──────────────────────┐┌ notebook.sharp ─ app#notebook.add ──┐
//      │ what the program prints      ││   40      found := store.find(note); │
//      │                              ││ → 41      if(found != null) {        │
//      ├ sdb ─────────────────────────┤├ locals ─────────────────────────────┤
//      │ (sdb) print found            ││ note  = "milk"                      │
//      │ found = null                 ││ found = null                        │
//      │                              │├ calls ──────────────────────────────┤
//      │ (sdb) █                      ││ * 0  app#notebook.add  ...:41       │
//      └──────────────────────────────┘└─────────────────────────────────────┘
//       breakpoint 2: app#notebook.add  notebook.sharp:41   (thread 0, fiber 0)
//
//   left    what the PROGRAM prints, above; what is typed to sdb and what sdb
//           answers, below
//   right   the source around the line the looked-at frame is on, the variables
//           that can be named there, the calls that led there - drawn again at
//           every stop and after every command, so `frame 2` or `thread 3`
//           changes all three
//   last    why the program stopped, or that it is running
//
// HOW IT IS POSSIBLE AT ALL
//   - sdb owns the terminal, so NOTHING ELSE MAY WRITE TO IT: what the program
//     prints (also what C code it calls prints) is taken at the source - the
//     output and error streams of the process are turned into a pipe, and a
//     thread reads the pipe into the output box. sdb itself draws through a
//     copy of the terminal it kept.
//   - the keyboard is the program's while the program runs (it may read it;
//     what is typed shows on the last line) and sdb's while the program is
//     held. Ctrl+C pauses, as in the plain view.
//   - the terminal's own second screen is used (as an editor does), so what
//     was on the terminal before is there again afterwards.
//
// THE PLAIN VIEW STAYS: text that scrolls, for a script, a pipe, a test, a
// terminal that is too small - and by choice (`-plain`, or "view": "plain" in
// sdb.json). Everything sdb says goes through say() (prompt.h) in both; a
// command does not know which view shows its words.
//
// KEYS, while sdb has the keyboard: Enter gives the line. Left and Right move
// in it (Ctrl+Left / Ctrl+Right a word at a time, Home / End or Ctrl+A / Ctrl+E
// to its ends) and what is typed goes in where the cursor is; Backspace and
// Delete take the character before and under it, Ctrl+U everything before it,
// Ctrl+K everything after. Tab finishes a command that is typed in part (and a
// command or topic after `help`); when several fit, it writes as far as they
// agree and lists them. Up and Down walk through the lines given before;
// PageUp and PageDown scroll the sdb box back and forth; Ctrl+L draws the
// screen again; Ctrl+D on an empty line is `quit`.
//
// Linux and other POSIX systems for now (termios); on Windows sdb uses the
// plain view.
//

#ifndef SHARP_DEBUGGER_VIEW_H
#define SHARP_DEBUGGER_VIEW_H

#include "../../runtime/stdimports.h"

struct debug_session;

/** can the full view be used here: both ends a terminal, and room for the boxes? `why` says why not */
bool view_can_be_full(string &why);

/** take the terminal: from now on the view is the full one. `session` is what the right-hand boxes are drawn from */
void view_begin(debug_session *session);

/** is the full view on? */
bool view_is_full();

/** a line of sdb's own words, into the sdb box */
void view_say(const string &line);

/** the next command typed at the prompt; false when the input has ended (Ctrl+D) */
bool view_read_command(string &line);

/** the program is held: `why` goes on the last line, and the right-hand boxes are drawn from the session */
void view_program_stopped(const string &why);

/** the right-hand boxes again, after a command that may have changed what is looked at */
void view_refresh();

/** the program was let go: the keyboard is its own until it stops */
void view_program_runs();

/**
 * Give the terminal back as it was. `lastWords` (the program's end, say) is
 * shown and waited on first when it is not empty, and printed again on the
 * terminal that comes back.
 */
void view_end(const string &lastWords);

#endif //SHARP_DEBUGGER_VIEW_H
