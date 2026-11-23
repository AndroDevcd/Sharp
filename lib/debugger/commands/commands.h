//
// THE COMMANDS OF THE PROMPT.
//
// A command is a word, perhaps a short form of it, and what follows on the
// line. Each group of commands is a file of this folder; it is also a TOPIC of
// `help`, and adds its topic and its commands to one table:
//
//      running.cpp         continue, next, into, out, nexti, intoi, quit
//      breakpoints.cpp     break, breaks, clear, disable, enable, save
//      looking.cpp         backtrace, frame, list, locals, print, set
//      threads_and_fibers.cpp   threads, thread, fibers, fiber
//
// A command's function gets the session - what is known about the stop the
// prompt is at - and the rest of the line, and says what the prompt does next.
//
// HELP is made from what each command says about itself (commands.cpp):
//
//      help                the topics, and the commands of each
//      help <topic>        the commands of a topic, a line each
//      help <command>      all about one command, with examples
//

#ifndef SHARP_DEBUGGER_COMMANDS_H
#define SHARP_DEBUGGER_COMMANDS_H

#include "../../../stdimports.h"
#include "../control/control.h"

/** what is known while the prompt is open */
struct debug_session {
    stop_report stop;                   // where the program is held
    step_kind step = STEP_NONE;         // how far the stopped fiber goes when a command lets the program go
    fiber *look = nullptr;              // the fiber the looking commands look at: the stopped one, until `thread N` / `fiber N` picks another
    Int frame = 0;                      // which call of that fiber they look at (0: where it is)
};

enum command_result {
    COMMAND_STAY = 0,           // ask for the next command
    COMMAND_RUN_ON,             // let the program go
    COMMAND_QUIT
};

typedef command_result (*command_function)(debug_session &session, const string &rest);

/** what a command says about itself, for `help` */
struct command_words {
    const char *usage;          // how it is written: `break <function> | <file>:<line>`
    const char *summary;        // what it does, in one line
    const char *details;        // all of it: what it does, how to write it, examples. Lines separated by \n; "" for none
};

/** make a topic of `help` known: its name (one word) and what its commands are about */
void add_topic(const string &name, const string &about);

/** make a command known, under a topic. `shortName` may be "" (none). */
void add_command(const string &topic, const string &name, const string &shortName,
                 const command_words &words, command_function function);

/** set the breakpoints the settings file keeps (once, before the program starts); says which could not be set */
void set_saved_breakpoints(debug_session &session);

/** run one line as typed; an empty line does nothing, an unknown word says so */
command_result run_command(debug_session &session, const string &line);

// each group's topic and commands (called once, by run_command, before the first line)
void add_running_commands();
void add_breakpoint_commands();
void add_looking_commands();
void add_thread_commands();

#endif //SHARP_DEBUGGER_COMMANDS_H
