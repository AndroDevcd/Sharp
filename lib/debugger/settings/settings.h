//
// sdb.json: THE SETTINGS OF A DEBUGGING SESSION, so that they need not be typed
// every time - as settings.json is for the compiler.
//
//      {
//          "program": "build/app",             the program to debug, when the command line names none
//          "arguments": ["first", "second"],   ... and what it is started with
//          "run": false,                       true: do not stop at `main` (the same as -run)
//          "map_file": "build/app.names",      the name map of a program built with -obf (the same as -map)
//          "view": "full",                     "full": the boxes (view/view.h), "plain": text that scrolls (the same as -plain)
//          "list_lines": 5,                    how many lines `list` shows above and below
//          "save_breakpoints": false,          true: every `break` and `clear` writes the list below at once
//          "breakpoints": ["app#notebook.add", "notebook.sharp:41"]
//      }
//
// WHERE: `sdb.json` in the folder sdb is started in, or the file -settings
// names. It need not be there. A file named in it (`program`, `map_file`) is
// looked for from the settings file's own folder.
//
// THE COMMAND LINE WINS: a program, -run, -map given there are taken instead of
// what the file says; -b breakpoints are set beside the saved ones.
//
// SAVING BREAKPOINTS (the `save` command, or every change with
// "save_breakpoints"): the file is READ AGAIN and only the list after
// "breakpoints" is replaced - every other character stays as it was, also what
// a person changed in the file while sdb ran. The settings sdb is running with
// are not touched by that reading. A file that is not there is made.
//
// THE FILE IS JSON, read by the small reader here: objects, lists, texts,
// numbers, true, false, null. A name it does not know is a mistake it reports -
// a setting spelled wrong should not pass for a setting left out.
//

#ifndef SHARP_DEBUGGER_SETTINGS_H
#define SHARP_DEBUGGER_SETTINGS_H

#include "../../../stdimports.h"
#include <vector>

struct sdb_settings {
    string file;                        // the file they are read from and breakpoints are saved to
    bool found = false;                 // the file was there

    string program;
    std::vector<string> arguments;
    bool run = false;
    string mapFile;
    Int listLines = 5;
    bool plainView = false;             // "view": "plain"
    bool saveBreakpoints = false;
    std::vector<string> breakpoints;
};

/**
 * Read a settings file. A file that is not there gives the defaults (`found`
 * stays false). False, and `problem` says what and where, when the file is
 * there but is not what it should be.
 */
bool read_settings(const string &file, sdb_settings &settings, string &problem);

/** the settings sdb runs with (set once, by sdb's main) */
sdb_settings& settings();

/**
 * Write `breakpoints` as the "breakpoints" list of the settings file, leaving
 * everything else in it as it is. False, and `problem` says why, when the file
 * cannot be read as JSON or cannot be written.
 */
bool save_breakpoints(const string &file, const std::vector<string> &breakpoints, string &problem);

#endif //SHARP_DEBUGGER_SETTINGS_H
