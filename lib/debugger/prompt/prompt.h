//
// THE PROMPT: what a person (or a script) types, and what sdb says back.
//
// TWO VIEWS. The PLAIN one: every answer is a line of text that scrolls, the
// same text every time - no colours, nothing redrawn - so a script and a test
// can read it. The FULL one (view/view.h): the terminal in boxes, for a person.
// Everything sdb says goes through say(), which puts it where the view in use
// shows it; a command never knows which one that is.
//
// WHERE COMMANDS COME FROM: the file given with -x first, then the keyboard
// (or whatever is piped in). When both have run out, that is `quit`.
// A command that did not come from a keyboard is printed after the prompt, so
// what a script did can be read back.
//

#ifndef SHARP_DEBUGGER_PROMPT_H
#define SHARP_DEBUGGER_PROMPT_H

#include "../../../stdimports.h"
#include <vector>

/** one line of sdb's own words */
void say(const string &line);

/**
 * A line of source the way sdb shows it: its number, then the text; `current`
 * puts an arrow in front (the line the program is about to run).
 */
string numbered_source_line(Int file, Int line, bool current);

/** read commands from this file before the keyboard; false when it cannot be opened */
bool prompt_read_from(const string &file);

/** what the command line asked for */
struct start_options {
    string program;                 // the executable
    std::vector<string> breaks;     // -b: breakpoints to set before the program starts
    bool runAtOnce = false;         // -run: do not stop at the program's `main`
    string mapFile;                 // -map: the name map of an obfuscated program; "": look for <program>.names
    bool fullView = false;          // the full-screen view (view/view.h); false: text that scrolls
    int machinePort = 0;            // -machine: an editor drives sdb through this port of this computer (machine/machine.h)
};

/** what the command line asked for */
const start_options& prompt_options();

/**
 * The program is loaded and nothing has run: read its name map and look at
 * what kind of build it is (the prompt and the machine mode both start so).
 * Ends sdb when the program cannot be debugged at all: names replaced and no
 * name map.
 */
void prepare_the_program();

void prompt_start_with(const start_options &options);

/**
 * The prompt's whole life, on a thread of its own: wait for the program to
 * stop, say where, take commands until one lets the program go, and again -
 * until the program ends or a person quits.
 */
void run_prompt();

#endif //SHARP_DEBUGGER_PROMPT_H
