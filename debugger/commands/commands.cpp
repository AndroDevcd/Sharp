//
// See commands.h: the table of commands, and `help`.
//

#include "commands.h"
#include "../prompt/prompt.h"
#include <algorithm>

struct command {
    string topic;
    string name;
    string shortName;
    command_words words;
    command_function function;
};

struct topic {
    string name;
    string about;
};

static std::vector<topic> topics;
static std::vector<command> commands;

void add_topic(const string &name, const string &about) {
    topic made;
    made.name = name;
    made.about = about;
    topics.push_back(made);
}

void add_command(const string &topic, const string &name, const string &shortName,
                 const command_words &words, command_function function) {
    command made;
    made.topic = topic;
    made.name = name;
    made.shortName = shortName;
    made.words = words;
    made.function = function;
    commands.push_back(made);
}

// ---------------------------------------------------------------------------------
// help
// ---------------------------------------------------------------------------------

/** `text` and spaces after it, up to `width` characters */
static string padded(const string &text, size_t width) {
    return text.size() >= width ? text : text + string(width - text.size(), ' ');
}

/** help: every topic, what it is about, and its commands */
static void help_overview() {
    say("sdb's commands, by topic. `help <topic>` lists a topic's commands; `help <command>` says all about one.");

    size_t widest = 0;
    for(topic &one : topics) {
        if(one.name.size() > widest) widest = one.name.size();
    }
    for(topic &one : topics) {
        say("");
        say("  " + padded(one.name, widest) + "   " + one.about);

        string names;
        for(command &each : commands) {
            if(each.topic != one.name) continue;
            if(!names.empty()) names += ", ";
            names += each.name;
        }
        say("  " + string(widest, ' ') + "   " + names);
    }
}

/** help <topic>: its commands, a line each */
static void help_topic(const topic &wanted) {
    say(wanted.name + ": " + wanted.about);
    say("");

    size_t widest = 0;
    for(command &one : commands) {
        if(one.topic == wanted.name && strlen(one.words.usage) > widest) widest = strlen(one.words.usage);
    }
    for(command &one : commands) {
        if(one.topic != wanted.name) continue;
        say("  " + padded(one.words.usage, widest) + "   " + one.words.summary);
    }
    say("");
    say("`help <command>` says all about one of them, with examples.");
}

/** help <command>: everything it says about itself */
static void help_command(const command &wanted) {
    say(wanted.words.usage);
    if(!wanted.shortName.empty()) say("short form: " + wanted.shortName);
    say("");
    say(wanted.words.summary);

    if(wanted.words.details[0] != 0) {
        say("");
        stringstream lines(wanted.words.details);
        string line;
        while(std::getline(lines, line)) say(line);
    }
}

static command_result help(debug_session &session, const string &rest) {
    if(rest.empty()) {
        help_overview();
        return COMMAND_STAY;
    }

    for(topic &one : topics) {
        if(one.name == rest) {
            help_topic(one);
            return COMMAND_STAY;
        }
    }
    for(command &one : commands) {
        if(one.name == rest || (!one.shortName.empty() && one.shortName == rest)) {
            help_command(one);
            return COMMAND_STAY;
        }
    }

    say("no topic and no command `" + rest + "`: `help` lists them all");
    return COMMAND_STAY;
}

static command_result keys(debug_session &session, const string &rest) {
    say("In the full-screen view, while sdb has the keyboard:");
    say("  Enter                 give the line");
    say("  Backspace             take the last character away");
    say("  Up, Down              the lines given before");
    say("  PageUp, PageDown      scroll the sdb box back and forward");
    say("  Ctrl+L                draw the screen again");
    say("  Ctrl+D                on an empty line: quit");
    say("While the program runs:");
    say("  Ctrl+C                stop it where it is (twice, when it does not stop: end sdb)");
    say("  anything else         goes to the program");
    return COMMAND_STAY;
}

static void add_help_command() {
    add_topic("help", "finding a command");
    add_command("help", "keys", "", {
        "keys",
        "The keys of the full-screen view.",
        "The full-screen view shows the program's output and sdb's answers on the left, and on the\n"
        "right the source around the current line, the variables and the calls. It is what a\n"
        "terminal gets; `sdb -plain` (or \"view\": \"plain\" in sdb.json) gives text that scrolls.\n"
        "`keys` lists what each key does there."
    }, keys);
    add_command("help", "help", "h", {
        "help [<topic> | <command>]",
        "The commands by topic; or the commands of one topic; or all about one command.",
        "    help              every topic, with the names of its commands\n"
        "    help looking      the commands of the topic `looking`, a line each\n"
        "    help print        all about `print`, with examples\n"
        "    help p            the same: a command's short form names it too"
    }, help);
}

// ---------------------------------------------------------------------------------
// Running a line
// ---------------------------------------------------------------------------------

static string trimmed(const string &text) {
    size_t from = text.find_first_not_of(" \t\r\n");
    if(from == string::npos) return "";
    size_t to = text.find_last_not_of(" \t\r\n");
    return text.substr(from, to - from + 1);
}

/** every group's commands into the table, the first time they are asked for */
static void know_the_commands() {
    if(!commands.empty()) return;
    add_running_commands();
    add_breakpoint_commands();
    add_looking_commands();
    add_thread_commands();
    add_help_command();
}

std::vector<string> completions_of(const string &text, size_t &from) {
    know_the_commands();
    std::vector<string> found;

    size_t start = text.find_first_not_of(" \t");
    if(start == string::npos) start = text.size();
    from = text.find_last_of(" \t");
    from = from == string::npos ? 0 : from + 1;

    bool first_word = from <= start;
    bool after_help = false;
    if(!first_word) {
        string before = trimmed(text.substr(0, from));
        after_help = before == "help" || before == "h";
    }
    if(!first_word && !after_help) return found;
    if(first_word) from = start;

    string begun = text.substr(from);
    for(command &one : commands) {
        if(one.name.compare(0, begun.size(), begun) == 0) found.push_back(one.name);
    }
    if(after_help) {
        for(topic &one : topics) {
            if(one.name.compare(0, begun.size(), begun) == 0
               && std::find(found.begin(), found.end(), one.name) == found.end()) found.push_back(one.name);
        }
    }
    return found;
}

command_result run_command(debug_session &session, const string &line) {
    know_the_commands();

    string text = trimmed(line);
    if(text.empty()) return COMMAND_STAY;

    size_t space = text.find_first_of(" \t");
    string word = space == string::npos ? text : text.substr(0, space);
    string rest = space == string::npos ? "" : trimmed(text.substr(space));

    for(command &one : commands) {
        if(word == one.name || (!one.shortName.empty() && word == one.shortName)) {
            return one.function(session, rest);
        }
    }

    // a word that begins like a command, or goes on after one (`breakpoint`): those are named
    string alike;
    for(command &one : commands) {
        if(one.name.compare(0, word.size(), word) == 0 || word.compare(0, one.name.size(), one.name) == 0) {
            alike += (alike.empty() ? "`" : ", `") + one.name + "`";
        }
    }
    if(!alike.empty()) say("no command `" + word + "`: did you mean " + alike + "?   (`help` lists them all; Tab finishes a word)");
    else say("no command `" + word + "`: `help` lists them");
    return COMMAND_STAY;
}
