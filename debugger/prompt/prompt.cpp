//
// See prompt.h.
//

#include "prompt.h"
#include "../control/control.h"
#include "../commands/commands.h"
#include "../settings/settings.h"
#include "../view/view.h"
#include "../machine/machine.h"
#include "../../runtime/virtual_machine.h"
#include "../../runtime/multitasking/thread/sharp_thread.h"
#include "../../runtime/multitasking/fiber/fiber.h"
#include "../settings/text_files.h"
#include <iomanip>
#ifdef _WIN32
#include <io.h>
#define input_is_a_keyboard() (_isatty(_fileno(stdin)) != 0)
#else
#include <unistd.h>
#define input_is_a_keyboard() (isatty(fileno(stdin)) != 0)
#endif

static std::vector<string> script;      // the commands of the file given with -x ...
static size_t script_at = 0;            // ... and how many of them were taken

static start_options start;

void prompt_start_with(const start_options &options) {
    start = options;
}

const start_options& prompt_options() {
    return start;
}

// what was said before the full view took the terminal (the name map, a note about the build):
// shown there too, in the sdb box
static std::vector<string> said_before_the_view;

void say(const string &line) {
    if(start.machinePort != 0) {
        machine_say(line);
        return;
    }
    if(view_is_full()) {
        view_say(line);
        return;
    }
    if(start.fullView) said_before_the_view.push_back(line);
    // the program prints through the same stream: flush, so the two stay in order
    cout << line << endl;
}

/** the full view takes the terminal (once), with what was said so far in its sdb box */
static void begin_the_full_view(debug_session &session) {
    if(!start.fullView || view_is_full()) return;
    view_begin(&session);
    for(const string &line : said_before_the_view) view_say(line);
    said_before_the_view.clear();
}

string numbered_source_line(Int file, Int line, bool current) {
    stringstream words;
    words << (current ? "-> " : "   ") << std::setw(5) << line << "  " << source_line(file, line);
    return words.str();
}

bool prompt_read_from(const string &file) {
    string text;
    if(!read_text_file(file, text)) return false;
    script = lines_of(text);
    script_at = 0;
    return true;
}

/** the next command; false when there are no more */
static bool next_command(string &line) {
    if(view_is_full()) return view_read_command(line);
    cout << "(sdb) " << flush;

    if(script_at < script.size()) {
        line = script[script_at++];
        cout << line << endl;
        return true;
    }

    if(!std::getline(cin, line)) {
        cout << endl;
        return false;
    }
    if(!input_is_a_keyboard()) cout << line << endl;
    return true;
}

/** why the program stopped, where, and the line it is about to run */
static void say_stop(const stop_report &stop, step_kind after) {
    code_place here = call_stack_of(stop.task).front();
    stringstream words;

    if(stop.reason == STOP_AT_BREAKPOINT) words << "breakpoint " << stop.breakpoint << ": ";
    else if(stop.reason == STOP_PAUSED && !stop.atInstruction) words << "paused; no thread was running Sharp code. The first fiber is in: ";
    else if(stop.reason == STOP_PAUSED) words << "paused: ";
    else if(stop.reason == STOP_AT_START) words << "at the start of the program: ";
    else if(stop.reason == STOP_LOADED) words << "before the first instruction: ";

    words << describe_place(here);
    bool hasLine = line_of(here.function, here.pc) >= 0 && !shown_file(here.function->sourceFile).empty();
    if((after == STEP_INTO_INSTRUCTION || after == STEP_NEXT_INSTRUCTION) && hasLine) {
        words << "  instruction " << here.pc;       // (the line alone would not show that it moved)
    }
    if(stop.atInstruction) words << "   (thread "<< stop.thread->id << ", fiber " << stop.task->id << ")";
    else words << "   (fiber " << stop.task->id << ")";
    say(words.str());

    if(view_is_full()) {
        // (the source box shows the line; the last line of the screen says why the program stopped)
        view_program_stopped(words.str());
        return;
    }
    Int line = line_of(here.function, here.pc);
    if(line >= 1 && line <= source_line_count(here.function->sourceFile)) {
        say(numbered_source_line(here.function->sourceFile, line, true));
    }
}

/**
 * The name map of a program built with -obf (symbols/names.h): the file given
 * with -map, or `<program>.names` when it is there - where the compiler writes
 * it. Says what it found; a program that was not obfuscated has no map and
 * nothing is said.
 */
static void load_names() {
    string file = start.mapFile.empty() ? start.program + ".names" : start.mapFile;
    int count = load_name_map(file);

    if(count >= 0) {
        stringstream words;
        words << "name map: " << count << " names from " << file << " - names are shown as written, with the program's own in brackets";
        say(words.str());
    } else if(!start.mapFile.empty()) {
        say("the name map `" + file + "` cannot be read");
    }
}

/**
 * What kind of build the program is, when that limits what sdb can do.
 *
 * NAMES REPLACED AND NO MAP: sdb does not open the program. Every name it
 * would show and take is one nobody wrote, so there is nothing to debug with -
 * and the program itself is not in doubt: it runs under `sharp` as it is.
 */
static void check_the_build() {
    if(vm.mf.obfuscated && !has_name_map()) {
        say("sdb cannot debug this program: its names were replaced (sharpc -obf) and its name map is not loaded.");
        say("  the map is the file the compiler wrote beside the program:  <program>.names");
        say("  sdb looked for " + (start.mapFile.empty() ? start.program + ".names" : start.mapFile));
        say("  give it with:  sdb -map <file> ...   or  \"map_file\" in " + settings().file);
        cout << flush;
        _exit(1);                           // (said before the full view has taken the terminal: it is read)
    }

    if(vm.mf.optimized) {
        say("note: the program was built with -O. sdb cannot show its local variables, and lines may run in another order.");
    } else if(!vm.mf.debug) {
        say("note: the program was built without -dbg. sdb cannot show its local variables or its source text.");
    }
}

/**
 * The program is loaded and has run nothing: read its name map, set the
 * command line's breakpoints, and say where the first stop is - the program's
 * own `main` (what runs before it is the library getting ready), or nowhere
 * with -run. False when there is no `main` to run to: then the prompt opens
 * right here, at the program's very first instruction.
 */
void prepare_the_program() {
    load_names();
    check_the_build();
}

static bool start_program(debug_session &session) {
    prepare_the_program();
    sdb_pause_on_interrupt();       // (now: the VM set up its own handling of signals when it started)
    set_saved_breakpoints(session);
    for(string &place : start.breaks) run_command(session, "break " + place);
    if(start.runAtOnce) return true;

    std::vector<code_place> starts;
    for(sharp_function *function : main_functions()) {
        code_place place;
        place.function = function;
        starts.push_back(place);
    }
    if(starts.empty()) {
        say("no function `main` was found: the program is stopped at its very first instruction - the library's start-up code.");
        return false;
    }

    sdb_stop_once_at(starts);
    return true;
}

void run_prompt() {
    debug_session session;

    while(sdb_wait_for_stop(session.stop)) {
        if(session.stop.reason == STOP_LOADED && start_program(session)) {
            begin_the_full_view(session);
            if(view_is_full()) view_program_runs();
            sdb_resume();
            continue;
        }
        step_kind stepped = session.step;
        session.step = STEP_NONE;
        session.look = session.stop.task;
        session.frame = 0;
        begin_the_full_view(session);
        say_stop(session.stop, stepped);

        for(;;) {
            string line;
            if(!next_command(line)) line = "quit";

            command_result result = run_command(session, line);
            if(result == COMMAND_QUIT) {
                view_end("");
                cout << flush;
                _exit(0);
            }
            if(result == COMMAND_RUN_ON) break;
            if(view_is_full()) view_refresh();      // (`frame 2`, `thread 3`, `set`: the boxes show something else now)
        }
        if(view_is_full()) view_program_runs();
        sdb_resume(session.step, session.stop.task);
    }

    stringstream words;
    words << "the program ended with exit code " << sdb_exit_code();
    if(view_is_full()) view_end(words.str());
    else say(words.str());
}
