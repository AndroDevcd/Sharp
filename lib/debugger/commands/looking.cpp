//
// Commands that look at the held program.
//
// They look at ONE call of ONE fiber: the fiber is `session.look` - the one that
// stopped, until `thread N` or `fiber N` picks another - and the call is the
// frame `session.frame` names: frame 0, where that fiber is, until `frame N`
// picks one of its callers. The next stop looks at the stopped fiber's frame 0
// again.
//

#include "commands.h"
#include "../prompt/prompt.h"
#include "../values/values.h"
#include "../../runtime/virtual_machine.h"

#include "../settings/settings.h"

#define LINES_AROUND (settings().listLines)     // how many lines `list` shows above and below ("list_lines" in sdb.json)

/** the frame the session looks at (it is always there: `frame N` only accepts one that is) */
static frame_view selected_frame(debug_session &session) {
    frame_view frame;
    if(!frame_at(session.look, session.frame, frame)) frame_at(session.look, 0, frame);
    return frame;
}

static command_result backtrace(debug_session &session, const string &rest) {
    std::vector<code_place> calls = call_stack_of(session.look);

    for(size_t i = 0; i < calls.size(); i++) {
        stringstream words;
        words << ((Int)i == session.frame ? "* " : "  ") << i << "  " << describe_place(calls[i]);
        say(words.str());
    }
    return COMMAND_STAY;
}

/** frame N: look at the N-th call of `backtrace` from here on */
static command_result select_frame(debug_session &session, const string &rest) {
    frame_view frame;

    if(!rest.empty()) {
        bool isNumber = rest.size() < 9 && rest.find_first_not_of("0123456789") == string::npos;
        if(!isNumber || !frame_at(session.look, atoll(rest.c_str()), frame)) {
            stringstream words;
            words << "frame which?  frame <number>, one of the " << call_stack_of(session.look).size()
                  << " that `backtrace` lists (0 is where the program is)";
            say(words.str());
            return COMMAND_STAY;
        }
        session.frame = frame.index;
    }

    frame = selected_frame(session);
    stringstream words;
    words << "frame " << frame.index << ": " << describe_place(frame.place);
    say(words.str());

    Int line = line_of(frame.place.function, frame.place.pc);
    if(line >= 1 && line <= source_line_count(frame.place.function->sourceFile)) {
        say(numbered_source_line(frame.place.function->sourceFile, line, true));
    }
    return COMMAND_STAY;
}

/**
 * list                 the lines around where the selected frame is
 * list 41              the lines around line 41 of the same file
 * list notebook.sharp:41
 */
static command_result list_source(debug_session &session, const string &rest) {
    code_place here = selected_frame(session).place;
    Int file = here.function->sourceFile;
    Int current = line_of(here.function, here.pc);      // the line with the arrow, when it is in view
    Int middle = current;

    if(!rest.empty()) {
        size_t colon = rest.find_last_of(':');
        string number = colon == string::npos ? rest : rest.substr(colon + 1);
        if(number.empty() || number.size() > 8 || number.find_first_not_of("0123456789") != string::npos) {
            say("list what?  list, list <line>, or list <file>:<line>");
            return COMMAND_STAY;
        }
        middle = atoll(number.c_str());

        if(colon != string::npos) {
            std::vector<Int> files = files_named(rest.substr(0, colon));
            if(files.empty()) {
                say("no source file named `" + rest.substr(0, colon) + "` in this program");
                return COMMAND_STAY;
            }
            if(files.size() > 1) {
                say("`" + rest.substr(0, colon) + "` fits more than one file - say more of its path:");
                for(Int one : files) say("  " + written_file(one));
                return COMMAND_STAY;
            }
            if(files[0] != file) current = -1;
            file = files[0];
        }
    }

    Int count = source_line_count(file);
    if(count == 0) {
        say("the program does not carry its source text: build it with -dbg");
        return COMMAND_STAY;
    }
    if(middle < 1 || middle > count) {
        stringstream words;
        words << "the file has " << count << " lines";
        say(words.str());
        return COMMAND_STAY;
    }

    Int from = middle - LINES_AROUND < 1 ? 1 : middle - LINES_AROUND;
    Int to = middle + LINES_AROUND > count ? count : middle + LINES_AROUND;
    for(Int line = from; line <= to; line++) {
        say(numbered_source_line(file, line, line == current));
    }
    return COMMAND_STAY;
}

/** every line of `text` (a message may have several), each as a line of its own */
static void say_lines(const string &text) {
    stringstream lines(text);
    string line;
    while(std::getline(lines, line)) say(line);
}

/** locals: every variable that can be named where the selected frame is, with what it holds */
static command_result show_locals(debug_session &session, const string &rest) {
    frame_view frame = selected_frame(session);
    std::vector<const local_data*> locals = locals_in_view(frame);

    if(locals.empty()) {
        if(frame.place.function->localCount == 0) {
            say("the names of this function's variables are not in the program: build it with -dbg and without -O");
        } else {
            say("no variables here");
        }
        return COMMAND_STAY;
    }

    // each name as written, with the name the executable carries beside it when that is another
    std::vector<string> names;
    size_t widest = 0;
    for(const local_data *local : locals) {
        string name = written_name(*local);
        names.push_back(name + carried_note(name, local->name));
        if(names.back().size() > widest) widest = names.back().size();
    }
    for(size_t i = 0; i < locals.size(); i++) {
        held_value value;
        string problem;
        string line = names[i] + string(widest - names[i].size(), ' ') + " = ";
        if(is_being_declared(frame, *locals[i])) line += "(not set yet: this line gives it its value)";
        else if(value_of_local(frame, *locals[i], value, problem)) line += text_of(value);
        else line += "(" + problem + ")";
        say(line);
    }
    return COMMAND_STAY;
}

/** print <path>: what the path names holds; an object's fields and an array's elements are listed under it */
static command_result print_value(debug_session &session, const string &rest) {
    if(rest.empty()) {
        say("print what?  a variable (`count`), a field (`book.title`), an element (`notes[3]`), or a mix (`book.notes.data[0]`)");
        return COMMAND_STAY;
    }

    held_value value;
    string problem;
    if(!value_of_path(selected_frame(session), rest, value, problem)) {
        say_lines(problem);
        return COMMAND_STAY;
    }

    // a variable on the very line that declares it: what its slot holds is not its value yet
    frame_view frame = selected_frame(session);
    for(const local_data *local : locals_in_view(frame)) {
        if(written_name(*local) == rest && is_being_declared(frame, *local)) {
            say(rest + " is not set yet: this line gives it its value");
            return COMMAND_STAY;
        }
    }

    string note = has_name_map() ? carried_note(rest, value.carried) : "";
    say(rest + note + " = " + text_of(value));
    for(const string &part : parts_of(value)) say("  " + part);
    return COMMAND_STAY;
}

/** set <path> = <value> */
static command_result set_variable(debug_session &session, const string &rest) {
    size_t equals = rest.find('=');
    string path = equals == string::npos ? "" : rest.substr(0, rest.find_last_not_of(" \t", equals - 1) + 1);
    size_t after = equals == string::npos ? string::npos : rest.find_first_not_of(" \t", equals + 1);
    string text = after == string::npos ? "" : rest.substr(after);

    if(path.empty() || text.empty()) {
        say("set what?  set <path> = <value>, as in `set count = 12`, `set ready = false`, `set name = \"bread\"`, `set book.owner = null`");
        return COMMAND_STAY;
    }

    held_value value;
    string problem;
    if(!value_of_path(selected_frame(session), path, value, problem)) {
        say_lines(problem);
        return COMMAND_STAY;
    }
    if(!set_value(value, text, problem)) {
        say("`" + path + "` was not set: " + problem);
        return COMMAND_STAY;
    }

    string note = has_name_map() ? carried_note(path, value.carried) : "";
    say(path + note + " = " + text_of(value));
    return COMMAND_STAY;
}

void add_looking_commands() {
    add_topic("looking", "seeing where the program is and what its variables hold");

    add_command("looking", "backtrace", "bt", {
        "backtrace",
        "The calls that led here: where the program is first, then each caller.",
        "    (sdb) backtrace\n"
        "    * 0  app#notebook.holds  notebook.sharp:27\n"
        "      1  app#notebook.add  notebook.sharp:19\n"
        "      2  app#fill  notebook.sharp:36\n"
        "      3  app#main  notebook.sharp:45\n"
        "\n"
        "Each line is a FRAME: one call that has not returned yet. Frame 0 is where the program\n"
        "is; the line of every other frame is the call it is waiting for.\n"
        "The `*` marks the frame that `print`, `locals` and `list` look at (see `frame`)."
    }, backtrace);

    add_command("looking", "frame", "f", {
        "frame [<number>]",
        "Look at one of the calls `backtrace` lists: its variables and its source.",
        "`print`, `locals` and `list` look at one frame - frame 0, where the program is, until\n"
        "`frame` picks another. It moves nothing: the program stays where it is.\n"
        "\n"
        "    (sdb) frame 1                 the function that called this one\n"
        "    frame 1: app#notebook.add  notebook.sharp:19\n"
        "    (sdb) locals                  ... its variables\n"
        "    (sdb) frame 0                 back to where the program is\n"
        "\n"
        "`frame` alone says which frame is looked at. The next stop makes it frame 0 again."
    }, select_frame);

    add_command("looking", "list", "l", {
        "list [<line> | <file>:<line>]",
        "The source around where the program is, or around a line.",
        "    list                      five lines above and below the current line (marked ->);\n"
        "                              \"list_lines\" in sdb.json says how many\n"
        "    list 80                   around line 80 of the same file\n"
        "    list notebook.sharp:12    around a line of another file\n"
        "\n"
        "The program carries its source only when it was built with -dbg."
    }, list_source);

    add_command("looking", "locals", "", {
        "locals",
        "Every variable that can be named here, and what it holds.",
        "`self`, the parameters, and the local variables declared above the current line whose\n"
        "block the line is in - what the code on this line could name.\n"
        "\n"
        "    (sdb) locals\n"
        "    self  = app#notebook\n"
        "    note  = \"milk\"\n"
        "    found = null\n"
        "\n"
        "An object shows as its class: `print <name>` lists its fields. An array and a list show\n"
        "their first 15 elements. A variable declared on the current line is `not set yet`.\n"
        "The names are in the program only when it was built with -dbg and without -O."
    }, show_locals);

    add_command("looking", "print", "p", {
        "print <path>",
        "What a variable, a field or an element of an array holds.",
        "A path names a value the way the program writes it:\n"
        "\n"
        "    print count               a local variable or a parameter\n"
        "    print self                the object the function was called on\n"
        "    print title               a field of that object\n"
        "    print book.title          a field of another object\n"
        "    print numbers[3]          an element of an array\n"
        "    print numbers[i]          ... the number taken from a variable\n"
        "    print book.notes.data[0]  any mix of the two\n"
        "    print limit               a static field of the function's class, or a variable of its module\n"
        "    print app#settings.limit  a static field of a named class\n"
        "    print app#visits          a variable of a named module\n"
        "\n"
        "An object is shown with its fields:\n"
        "\n"
        "    (sdb) print corner\n"
        "    corner = app#point\n"
        "      x = 3\n"
        "      y = 4\n"
        "      label = \"corner\"\n"
        "\n"
        "An array and a list are shown with their elements, the first 15 and then `...`:\n"
        "\n"
        "    (sdb) print numbers\n"
        "    numbers = _int64[4] [10, 20, 30, 40]\n"
        "    (sdb) print notes\n"
        "    notes = std#list<std#string> (2) [\"milk\", \"bread\"]\n"
        "    (sdb) print big\n"
        "    big = _int64[40] [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, ...]\n"
        "    (sdb) print big[27]           one that is further on\n"
        "\n"
        "A path only NAMES a value: it does no arithmetic and calls no function.\n"
        "It is looked up in the frame `frame` picked (frame 0, where the program is, by default)."
    }, print_value);

    add_command("looking", "set", "", {
        "set <path> = <value>",
        "Give a variable, a field or an element a new value.",
        "The path is written as for `print`; the program goes on with the new value.\n"
        "\n"
        "    set count = 12                a whole number\n"
        "    set ratio = 2.5               a decimal\n"
        "    set ready = false             a bool: true or false\n"
        "    set numbers[2] = 99           an element of an array\n"
        "    set corner.x = 0              a field\n"
        "    set name = \"bread\"          a text, in quotes (\\n \\t \\\" \\\\ as in a program)\n"
        "    set book.owner = null         an object can be set to null\n"
        "\n"
        "A text is given to a variable that holds a text (or null). sdb copies no objects and makes\n"
        "no others: `set a = b` is not possible.\n"
        "A number too large for the variable's type wraps round, as an assignment in the program\n"
        "would: an _int8 set to 300 holds 44.\n"
        "A variable on the line that declares it will be set again by that line."
    }, set_variable);
}
