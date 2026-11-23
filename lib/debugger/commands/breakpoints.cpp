//
// Commands about breakpoints.
//
//      break app#notebook.add      a function: by its whole name, or the end of it
//      break notebook.sharp:41     a line of a file: by the file's name, or the end of its path
//

#include "commands.h"
#include "../prompt/prompt.h"
#include "../settings/settings.h"
#include "../../runtime/virtual_machine.h"

/** `text` as a number of at most 8 digits, or -1 */
static Int number_in(const string &text) {
    if(text.empty() || text.size() > 8 || text.find_first_not_of("0123456789") != string::npos) return -1;
    return atoll(text.c_str());
}

/** the places of `break <function>`; empty (and the reason said) when there are none */
static std::vector<code_place> places_of_function(const string &name) {
    std::vector<code_place> places;
    bool ambiguous = false;
    std::vector<sharp_function*> functions = functions_named(name, ambiguous);

    if(functions.empty()) {
        say("no function named `" + name + "` in this program");
        return places;
    }
    if(ambiguous) {
        say("`" + name + "` fits more than one function - say which, with its module and class:");
        string last;
        for(sharp_function *function : functions) {
            if(name_of(function) != last) say("  " + name_of(function));
            last = name_of(function);
        }
        return places;
    }

    for(sharp_function *function : functions) {
        code_place place;
        place.function = function;
        places.push_back(place);
    }
    return places;
}

/** the places of `break <file>:<line>`; empty (and the reason said) when there are none */
static std::vector<code_place> places_of_line(const string &file, Int line) {
    std::vector<code_place> places;
    std::vector<Int> files = files_named(file);

    if(files.empty()) {
        say("no source file named `" + file + "` in this program");
        return places;
    }
    if(files.size() > 1) {
        say("`" + file + "` fits more than one file - say more of its path:");
        for(Int one : files) say("  " + written_file(one));
        return places;
    }

    Int nextLine = -1;
    places = places_at_line(files[0], line, nextLine);
    if(places.empty()) {
        stringstream words;
        words << "line " << line << " of " << shown_file(files[0]) << " holds no code";
        if(nextLine != -1) words << "; the next line that does is " << nextLine;
        say(words.str());
    }
    return places;
}

// ---------------------------------------------------------------------------------
// Keeping breakpoints in the settings file (settings/settings.h)
// ---------------------------------------------------------------------------------

// a breakpoint that is switched off is saved with this in front of it: "(off) notebook.sharp:41"
#define OFF_MARK string("(off) ")

// saved breakpoints that could not be set this time (the line holds no code any more, the function
// is gone): they are not set, and not thrown away either - a save writes them back as they were
static std::vector<string> not_set;

/** write the breakpoints there are (and the saved ones that could not be set) into the settings file */
static bool write_breakpoints(bool sayIt) {
    std::vector<string> all;
    for(breakpoint &point : sdb_breakpoints()) all.push_back((point.on ? "" : OFF_MARK) + point.written);
    for(string &kept : not_set) all.push_back(kept);

    string problem;
    if(!save_breakpoints(settings().file, all, problem)) {
        say(problem);
        return false;
    }
    if(sayIt) {
        stringstream words;
        words << all.size() << " breakpoint" << (all.size() == 1 ? "" : "s") << " saved in " << settings().file;
        say(words.str());
    }
    return true;
}

/** after a breakpoint was set or cleared: save at once, when the settings ask for that */
static void breakpoints_changed() {
    if(settings().saveBreakpoints) write_breakpoints(false);
}

static command_result set_breakpoint(debug_session &session, const string &rest);

void set_saved_breakpoints(debug_session &session) {
    for(const string &saved : settings().breakpoints) {
        bool off = saved.compare(0, OFF_MARK.size(), OFF_MARK) == 0;
        string place = off ? saved.substr(OFF_MARK.size()) : saved;

        size_t before = sdb_breakpoints().size();
        bool autoSave = settings().saveBreakpoints;
        settings().saveBreakpoints = false;             // (setting them is not a change to save)
        set_breakpoint(session, place);
        if(off && sdb_breakpoints().size() > before) {
            sdb_switch_breakpoint(sdb_breakpoints().back().number, false);
            say("  (it is switched off: `enable` switches it on)");
        }
        settings().saveBreakpoints = autoSave;

        if(sdb_breakpoints().size() == before) {
            say("  ^ the breakpoint `" + place + "` of " + settings().file + " was not set; it stays in the file");
            not_set.push_back(saved);
        }
    }
}

static command_result save_now(debug_session &session, const string &rest) {
    write_breakpoints(true);
    return COMMAND_STAY;
}

static command_result set_breakpoint(debug_session &session, const string &rest) {
    if(rest.empty()) {
        say("break where?  break <function> (`break app#notebook.add`), or break <file>:<line> (`break notebook.sharp:41`)");
        return COMMAND_STAY;
    }

    // `<file>:<line>` when what follows the last colon is a number
    std::vector<code_place> places;
    size_t colon = rest.find_last_of(':');
    Int line = colon == string::npos ? -1 : number_in(rest.substr(colon + 1));

    if(line != -1) places = places_of_line(rest.substr(0, colon), line);
    else places = places_of_function(rest);
    if(places.empty()) return COMMAND_STAY;

    int number = sdb_add_breakpoint(places, rest);
    stringstream words;
    words << "breakpoint " << number << " at " << describe_place(places[0]);
    if(places.size() > 1) words << "  (and " << places.size() - 1 << " more: `breaks` lists them)";
    say(words.str());
    breakpoints_changed();
    return COMMAND_STAY;
}

static command_result list_breakpoints(debug_session &session, const string &rest) {
    std::vector<breakpoint> points = sdb_breakpoints();
    if(points.empty()) {
        say("no breakpoints");
        return COMMAND_STAY;
    }

    for(breakpoint &point : points) {
        for(size_t i = 0; i < point.places.size(); i++) {
            stringstream words;
            if(i == 0) words << "  " << point.number << "  ";
            else words << "     ";
            words << describe_place(point.places[i]);
            if(!point.on) words << "   (off)";
            say(words.str());
        }
    }
    return COMMAND_STAY;
}

/** disable N / enable N (and `all`) */
static command_result switch_breakpoints(const string &rest, bool on) {
    const char *word = on ? "enable" : "disable";

    if(rest == "all") {
        for(breakpoint &point : sdb_breakpoints()) sdb_switch_breakpoint(point.number, on);
        say(on ? "every breakpoint is switched on" : "every breakpoint is switched off");
        breakpoints_changed();
        return COMMAND_STAY;
    }

    Int number = number_in(rest);
    if(number == -1) {
        say(string(word) + " which?  " + word + " <number> (as `breaks` lists them), or " + word + " all");
        return COMMAND_STAY;
    }
    if(sdb_switch_breakpoint((int)number, on)) {
        say("breakpoint " + rest + (on ? " is switched on" : " is switched off: the program does not stop at it"));
        breakpoints_changed();
    } else {
        say("there is no breakpoint " + rest + ": `breaks` lists them");
    }
    return COMMAND_STAY;
}

static command_result disable_breakpoints(debug_session &session, const string &rest) {
    return switch_breakpoints(rest, false);
}

static command_result enable_breakpoints(debug_session &session, const string &rest) {
    return switch_breakpoints(rest, true);
}

static command_result clear_breakpoints(debug_session &session, const string &rest) {
    if(rest == "all") {
        sdb_clear_all_breakpoints();
        say("every breakpoint is cleared");
        breakpoints_changed();
        return COMMAND_STAY;
    }

    Int number = number_in(rest);
    if(number == -1) {
        say("clear which?  clear <number> (as `breaks` lists them), or clear all");
        return COMMAND_STAY;
    }

    if(sdb_clear_breakpoint((int)number)) {
        say("breakpoint " + rest + " is cleared");
        breakpoints_changed();
    } else {
        say("there is no breakpoint " + rest + ": `breaks` lists them");
    }
    return COMMAND_STAY;
}

void add_breakpoint_commands() {
    add_topic("breakpoints", "saying where the program stops");

    add_command("breakpoints", "break", "b", {
        "break <function> | <file>:<line>",
        "Stop the program whenever a function is entered, or a line is reached.",
        "A FUNCTION is named as stack traces name it: module, `#`, class, `.`, function.\n"
        "\n"
        "    break app#notebook.add      the function `add` of class `notebook` in module `app`\n"
        "    break app#fill              a function written outside any class\n"
        "    break notebook.add          the end of the name is enough when only one function fits\n"
        "\n"
        "When a short name fits several functions (`add`), they are listed with their whole names:\n"
        "copy the one you mean. Functions that share one whole name (overloads) all get the\n"
        "breakpoint.\n"
        "\n"
        "A LINE is named by its file and its number.\n"
        "\n"
        "    break notebook.sharp:41\n"
        "    break shop/notes.sharp:5    with enough of the path, when two files share a name\n"
        "\n"
        "A line that holds no code (a comment, a blank line, a `}`) gets no breakpoint: sdb names\n"
        "the next line that does, and moves nothing by itself.\n"
        "\n"
        "Each breakpoint gets a number; `breaks` lists them, `clear` takes one away."
    }, set_breakpoint);

    add_command("breakpoints", "breaks", "", {
        "breaks",
        "List the breakpoints, each with its number and where it is.",
        "    (sdb) breaks\n"
        "      1  app#fill  notebook.sharp:35\n"
        "      2  app#notebook.add  notebook.sharp:22\n"
        "\n"
        "A breakpoint in several places (every overload of a function) shows a line for each."
    }, list_breakpoints);

    add_command("breakpoints", "clear", "", {
        "clear <number> | all",
        "Take a breakpoint away, by the number `breaks` shows - or every one.",
        "    clear 2       breakpoint 2 is gone\n"
        "    clear all     all of them are\n"
        "\n"
        "A number is never used again: after `clear 2` the next new breakpoint is not 2."
    }, clear_breakpoints);

    add_command("breakpoints", "disable", "", {
        "disable <number> | all",
        "Switch a breakpoint off: it stays in the list, but the program does not stop at it.",
        "For a breakpoint that is in the way now and wanted again later - `clear` would throw it away.\n"
        "\n"
        "    (sdb) disable 2\n"
        "    breakpoint 2 is switched off: the program does not stop at it\n"
        "    (sdb) breaks\n"
        "      1  app#fill  notebook.sharp:35\n"
        "      2  app#notebook.add  notebook.sharp:22   (off)\n"
        "    (sdb) enable 2\n"
        "\n"
        "A breakpoint that is off is saved as off:  \"(off) notebook.sharp:22\"  in sdb.json."
    }, disable_breakpoints);

    add_command("breakpoints", "enable", "", {
        "enable <number> | all",
        "Switch a breakpoint on again after `disable`.",
        "    enable 2      breakpoint 2 stops the program again\n"
        "    enable all    every one does"
    }, enable_breakpoints);

    add_command("breakpoints", "save", "", {
        "save",
        "Write the breakpoints into the settings file, so that the next run of sdb sets them again.",
        "The settings file is sdb.json in the folder sdb was started in (or the file -settings names).\n"
        "`save` puts the breakpoints there as its \"breakpoints\" list, each as it was typed:\n"
        "\n"
        "    \"breakpoints\": [\n"
        "        \"app#notebook.add\",\n"
        "        \"notebook.sharp:41\"\n"
        "    ]\n"
        "\n"
        "Only that list is written: everything else in the file stays as it is, character for\n"
        "character. A file that is not there is made.\n"
        "\n"
        "With  \"save_breakpoints\": true  in the file, every `break` and `clear` saves at once and\n"
        "`save` is not needed.\n"
        "\n"
        "A saved breakpoint that cannot be set any more (its line holds no code now) is reported at\n"
        "the start, is not set and is NOT moved to another line; it stays in the file until its\n"
        "line is right again or you take it out."
    }, save_now);
}
