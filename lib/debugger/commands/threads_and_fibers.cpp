//
// Commands about the program's threads and fibers (control/threads.h says what
// the two are and what their states mean).
//
// `thread N` and `fiber N` also choose which fiber the looking commands look
// at (`session.look`): its calls, its variables. Stepping is not theirs to
// change - `next`, `into`, `out` and `nexti` always move the fiber that
// stopped.
//

#include "commands.h"
#include "../prompt/prompt.h"
#include "../control/threads.h"
#include "../../runtime/multitasking/fiber/fiber.h"
#include <algorithm>

#define FIBERS_ON_A_PAGE 20

/** `text` and spaces after it, up to `width` characters */
static string padded(const string &text, size_t width) {
    return text.size() >= width ? text : text + string(width - text.size(), ' ');
}

static string number_text(Int number) {
    stringstream words;
    words << number;
    return words.str();
}

/** `text` as a number of at most 9 digits, or -1 */
static Int number_in(const string &text) {
    if(text.empty() || text.size() > 9 || text.find_first_not_of("0123456789") != string::npos) return -1;
    return atoll(text.c_str());
}

static string lower(string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)tolower(c); });
    return text;
}

// ---------------------------------------------------------------------------------
// Looking at a fiber
// ---------------------------------------------------------------------------------

/** all about one fiber: its state, the thread it is on or may run on, the calls that led to where it is */
static void say_fiber(debug_session &session, const fiber_row &row) {
    stringstream words;
    words << "fiber " << row.id;
    if(!row.name.empty()) words << " \"" << row.name << "\"";
    words << ": " << row.state;
    if(row.boundTo >= 0) words << "; only thread " << row.boundTo << " may run it";
    else words << "; any thread may run it";
    say(words.str());

    std::vector<code_place> calls = call_stack_of(row.task);
    if(calls.empty()) {
        say("  it has not started: it has no calls and no variables yet");
        return;
    }
    for(size_t i = 0; i < calls.size(); i++) {
        stringstream line;
        line << "  " << i << "  " << describe_place(calls[i]);
        say(line.str());
    }
}

/**
 * `thread 4 backtrace`, `fiber 12 locals`: run one command looking at that
 * fiber, and look where one looked before again.
 */
static command_result run_looking_at(debug_session &session, const fiber_row &row, const string &command) {
    if(call_stack_of(row.task).empty()) {
        say("fiber " + number_text(row.id) + " has not started: it has no calls and no variables yet");
        return COMMAND_STAY;
    }

    fiber *looked = session.look;
    Int frame = session.frame;
    session.look = row.task;
    session.frame = 0;
    command_result result = run_command(session, command);
    session.look = looked;
    session.frame = frame;
    return result;
}

/** the id a `thread` / `fiber` command starts with (-1: none), and what follows it on the line */
static Int id_and_rest(const string &line, string &rest) {
    size_t space = line.find_first_of(" \t");
    rest = space == string::npos ? "" : line.substr(line.find_first_not_of(" \t", space));
    return number_in(space == string::npos ? line : line.substr(0, space));
}

/** from now on the looking commands look at this fiber; false (and why is said) when there is nothing to look at */
static bool look_at(debug_session &session, const fiber_row &row) {
    if(call_stack_of(row.task).empty()) return false;

    session.look = row.task;
    session.frame = 0;
    if(row.task != session.stop.task) {
        say("`backtrace`, `frame`, `locals`, `print` and `list` now look at fiber " + number_text(row.id)
            + "; stepping still moves fiber " + number_text(session.stop.task->id) + ", the one that stopped");
    }
    return true;
}

// ---------------------------------------------------------------------------------
// threads, thread N
// ---------------------------------------------------------------------------------

static command_result list_threads(debug_session &session, const string &rest) {
    std::vector<thread_row> rows = threads_now();

    size_t names = 4, states = 5;
    for(thread_row &row : rows) {
        if(row.name.size() > names) names = row.name.size();
        if(row.state.size() > states) states = row.state.size();
    }

    say("  " + padded("id", 5) + padded("name", names + 2) + padded("state", states + 2) + padded("fiber", 7) + "where");
    for(thread_row &row : rows) {
        string mark = row.task != nullptr && row.task == session.look ? "* " : "  ";
        string fiber = row.fiberId >= 0 ? number_text(row.fiberId) : "-";
        say(mark + padded(number_text(row.id), 5) + padded(row.name, names + 2) + padded(row.state, states + 2)
            + padded(fiber, 7) + row.place);
    }
    return COMMAND_STAY;
}

static command_result select_thread(debug_session &session, const string &line) {
    string command;
    Int id = id_and_rest(line, command);
    string rest = number_text(id);
    if(id < 0) {
        say("thread which?  thread <id>, one that `threads` lists - or thread <id> <command>, as in `thread 4 backtrace`");
        return COMMAND_STAY;
    }

    for(thread_row &row : threads_now()) {
        if(row.id != id) continue;

        if(!command.empty()) {
            if(row.task == nullptr) {
                say("thread " + rest + " runs no fiber right now: there is nothing to look at");
                return COMMAND_STAY;
            }
            for(fiber_row &fiber : fibers_now()) {
                if(fiber.task == row.task) return run_looking_at(session, fiber, command);
            }
            return COMMAND_STAY;
        }

        say("thread " + number_text(row.id) + (row.name.empty() ? "" : " \"" + row.name + "\"") + ": " + row.state);
        if(row.task == nullptr) {
            say(row.own ? "  it is one of the VM's own threads: it runs no Sharp code"
                        : "  it runs no fiber right now: there is nothing to look at");
            return COMMAND_STAY;
        }
        for(fiber_row &fiber : fibers_now()) {
            if(fiber.task == row.task) {
                say_fiber(session, fiber);
                look_at(session, fiber);
            }
        }
        return COMMAND_STAY;
    }

    say("there is no thread " + rest + ": `threads` lists them");
    return COMMAND_STAY;
}

// ---------------------------------------------------------------------------------
// fibers [word] [page], fiber N
// ---------------------------------------------------------------------------------

static command_result list_fibers(debug_session &session, const string &rest) {
    // the last word is the page when it is a number; what stands before it is looked for
    string word = rest;
    Int page = 1;
    size_t space = rest.find_last_of(" \t");
    string last = space == string::npos ? rest : rest.substr(space + 1);
    if(number_in(last) >= 1) {
        page = number_in(last);
        word = space == string::npos ? "" : rest.substr(0, rest.find_last_not_of(" \t", space) + 1);
    }

    std::vector<fiber_row> all = fibers_now();
    std::vector<fiber_row> rows;
    for(fiber_row &row : all) {
        // looked for in its id, name and state, and in every call that led to where it is
        string text = number_text(row.id) + " " + row.name + " " + row.state + " " + row.calls;
        if(word.empty() || lower(text).find(lower(word)) != string::npos) rows.push_back(row);
    }

    if(rows.empty()) {
        if(word.empty()) say("the program has no fibers");
        else say("no fiber has `" + word + "` in its id, name, state or calls (" + number_text(all.size()) + " fibers in all)");
        return COMMAND_STAY;
    }

    Int pages = ((Int)rows.size() + FIBERS_ON_A_PAGE - 1) / FIBERS_ON_A_PAGE;
    if(page > pages) {
        say("there are " + number_text(pages) + " pages");
        return COMMAND_STAY;
    }
    size_t from = (page - 1) * FIBERS_ON_A_PAGE;
    size_t to = std::min(rows.size(), from + FIBERS_ON_A_PAGE);

    size_t names = 4, states = 5;
    for(size_t i = from; i < to; i++) {
        if(rows[i].name.size() > names) names = rows[i].name.size();
        if(rows[i].state.size() > states) states = rows[i].state.size();
    }

    say("  " + padded("id", 7) + padded("name", names + 2) + padded("state", states + 2) + "where");
    for(size_t i = from; i < to; i++) {
        fiber_row &row = rows[i];
        string mark = row.task == session.look ? "* " : "  ";
        say(mark + padded(number_text(row.id), 7) + padded(row.name, names + 2) + padded(row.state, states + 2)
            + (row.place.empty() ? "(not started)" : row.place));
    }

    stringstream foot;
    foot << "fibers " << from + 1 << " to " << to << " of " << rows.size();
    if(!word.empty()) foot << " with `" << word << "` (" << all.size() << " in all)";
    if(pages > 1) {
        foot << ", page " << page << " of " << pages;
        if(page < pages) foot << ": `fibers " << (word.empty() ? "" : word + " ") << page + 1 << "` shows the next";
    }
    say(foot.str());
    return COMMAND_STAY;
}

static command_result select_fiber(debug_session &session, const string &line) {
    string command;
    Int id = id_and_rest(line, command);
    string rest = number_text(id);
    if(id < 0) {
        say("fiber which?  fiber <id>, one that `fibers` lists - or fiber <id> <command>, as in `fiber 12 locals`");
        return COMMAND_STAY;
    }

    for(fiber_row &row : fibers_now()) {
        if(row.id != id) continue;
        if(!command.empty()) return run_looking_at(session, row, command);
        say_fiber(session, row);
        look_at(session, row);
        return COMMAND_STAY;
    }

    say("there is no fiber " + rest + ": `fibers` lists them");
    return COMMAND_STAY;
}

void add_thread_commands() {
    add_topic("threads", "the program's threads and fibers: what each was doing when the program stopped");

    add_command("threads", "threads", "", {
        "threads",
        "Every thread, what it was doing, and the fiber it runs.",
        "A thread is an OS thread that runs Sharp code. When the program stops, EVERY thread stops;\n"
        "the state shown is what the thread was doing at that moment:\n"
        "\n"
        "    running               it was executing Sharp code\n"
        "    extern c++            inside the VM's C++ or a native function, waiting: the keyboard, a socket, a join\n"
        "    extern c++ (busy)     inside C++ and still working: this one did not stop\n"
        "    waiting for a fiber   it has no fiber that can run (they sleep, or wait for a lock)\n"
        "    suspended             the program suspended it\n"
        "    not started, finished\n"
        "    VM                    the VM's own: the garbage collector's thread, the idle thread - never stopped\n"
        "\n"
        "    (sdb) threads\n"
        "      id   name    state                 fiber  where\n"
        "    * 0    main    running               0      app#main  workers.sharp:41\n"
        "      1    gc      VM                    -\n"
        "      3    worker  waiting for a fiber   -\n"
        "\n"
        "`where` is the nearest function of YOUR program the thread's fiber is in (`thread <id>`\n"
        "shows every call). The `*` marks the thread whose fiber `print`, `locals` and `backtrace`\n"
        "look at.\n"
        "`thread <id>` looks at another one."
    }, list_threads);

    add_command("threads", "thread", "t", {
        "thread <id> [<command>]",
        "Look at a thread: its state, the fiber it runs and the calls that led there.",
        "With a command after the id, that ONE command looks at the thread, and nothing else changes:\n"
        "\n"
        "    thread 4 backtrace            the calls of thread 4\n"
        "    thread 4 locals               its variables\n"
        "    thread 4 print total          one of them\n"
        "\n"
        "Without a command, `backtrace`, `frame`, `locals`, `print` and `list` look at that thread's\n"
        "fiber from then on.\n"
        "\n"
        "    (sdb) thread 3\n"
        "    thread 3 \"worker\": running\n"
        "    fiber 7: running on thread 3; only thread 3 may run it\n"
        "      0  app#work  workers.sharp:18\n"
        "      1  app#worker_main  workers.sharp:30\n"
        "    (sdb) locals                  the variables of app#work, in thread 3\n"
        "\n"
        "It only changes what is LOOKED at: `next`, `into`, `out` and `nexti` still move the fiber\n"
        "that stopped. The next stop looks at the fiber that stopped again."
    }, select_thread);

    add_command("threads", "fibers", "", {
        "fibers [<word>] [<page>]",
        "Every fiber and what it was doing - 20 to a page; a word narrows the list.",
        "A fiber is one flow of the program with its own stack: each thread's own, and every\n"
        "coroutine. A fiber that waits is on no thread at all, so `threads` does not show it.\n"
        "\n"
        "    running on thread N   (the thread's state says the rest)\n"
        "    ready                 it can run and waits for a thread to take it\n"
        "    sleeping, N ms left   in delay(); the time stands still while the program is stopped\n"
        "    waiting for a lock that fiber N holds\n"
        "    suspended             by fiber.suspend(), until it is woken\n"
        "    not started, finished\n"
        "\n"
        "    fibers                the first 20\n"
        "    fibers 3              page 3\n"
        "    fibers sleeping       only the fibers with `sleeping` in their line\n"
        "    fibers download 2     page 2 of the fibers with `download` in their line\n"
        "\n"
        "`where` is the nearest function of YOUR program the fiber is in: a fiber that sleeps is\n"
        "always somewhere inside the library, which says little. `fiber <id>` shows every call.\n"
        "\n"
        "The word is looked for in a fiber's id, name and state and in EVERY call that led to where\n"
        "it is, in capitals or not - so it finds fibers by what they do (`lock`, `sleeping`), by any\n"
        "function or file they are in (`app#fetch`, `net.sharp`) or by what they are called."
    }, list_fibers);

    add_command("threads", "fiber", "", {
        "fiber <id> [<command>]",
        "Look at a fiber: its state, which thread may run it, and the calls that led to where it is.",
        "With a command after the id, that ONE command looks at the fiber, and nothing else changes:\n"
        "\n"
        "    fiber 12 backtrace            the calls of fiber 12\n"
        "    fiber 12 locals               its variables\n"
        "\n"
        "Without a command, `backtrace`, `frame`, `locals`, `print` and `list` look at that fiber\n"
        "from then on - also one that is asleep or waits for a lock: its stack is its own, whatever\n"
        "thread it last ran on.\n"
        "\n"
        "    (sdb) fiber 12\n"
        "    fiber 12 \"download\": sleeping, 400 ms left; any thread may run it\n"
        "      0  std.io#delay  ...\n"
        "      1  app#fetch  net.sharp:52\n"
        "    (sdb) frame 1\n"
        "    (sdb) locals                  the variables of app#fetch, in fiber 12\n"
        "\n"
        "It only changes what is LOOKED at: stepping still moves the fiber that stopped."
    }, select_fiber);
}
