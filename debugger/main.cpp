//
// sdb - THE SHARP DEBUGGER.
//
//      sdb [options] program [the program's arguments]
//
// sdb is the Sharp VM with a prompt: it loads and runs the program inside
// itself, as `sharp` does, and holds it where a person asked (hooks.h says how
// the two are one code base). This file is sdb's `main` - it stands where the
// VM's own main.cpp stands in `sharp` - and does three things: reads sdb's
// options, starts the prompt on a thread of its own, and hands everything else
// (the VM's options, the program, its arguments) to the VM's own start.
//
// The machine-code compiler is not part of sdb: every instruction is run by the
// interpreter, where the hooks are.
//

#include "../runtime/main.h"
#include "../runtime/util/File.h"
#include "control/control.h"
#include "prompt/prompt.h"
#include "settings/settings.h"
#include "view/view.h"
#include "machine/machine.h"
#include <thread>

#define SDB_NAME "sdb"
#define SDB_VERSION "0.1.0"

static void sdb_help() {
    cout << "Usage: " << SDB_NAME << " [options] program [the program's arguments]" << endl;
    cout << "       " << SDB_NAME << " [options]                      the program that sdb.json names" << endl;
    cout << "The program must be built with sharpc; build it with -dbg and without -O to see the most.\n" << endl;
    cout << "[-options]\n" << endl;
    cout << "    -run                   start the program at once: stop only at a breakpoint" << endl;
    cout << "    -b <place>             set a breakpoint before the program starts (may be given more than once)" << endl;
    cout << "    -x <file>              read commands from a file, then from the keyboard" << endl;
    cout << "    -map <file>            the name map of a program built with -obf (default: <program>.names beside it)" << endl;
    cout << "    -settings <file>       the settings file (default: sdb.json in this folder, when it is there)" << endl;
    cout << "    -plain                 text that scrolls, not the full-screen view (which a terminal gets by itself)" << endl;
    cout << "    -machine <port>        be driven by an editor's debugger: JSON lines over this port of this computer" << endl;
    cout << "    -V                     print the version number and exit" << endl;
    cout << "    -h -?                  display this help message" << endl;
    cout << "\nThe VM's own options (-mem, -stack, -istack, -gclog, -gcstress) may be given too." << endl;
    cout << "\nsdb.json keeps what need not be typed every time - the program, its arguments, breakpoints:" << endl;
    cout << "    { \"program\": \"build/app\", \"arguments\": [], \"run\": false, \"map_file\": \"\"," << endl;
    cout << "      \"view\": \"full\", \"list_lines\": 5, \"save_breakpoints\": false, \"breakpoints\": [\"app#main\"] }" << endl;
    cout << "\nAt the prompt, `help` lists the commands." << endl;
}

static void sdb_error(const string &message) {
    cout << SDB_NAME << ":  error: " << message << endl;
    exit(1);
}

#define opt(v) (strcmp(argv[i], v) == 0)

int main(int argc, const char* argv[]) {
    std::vector<const char*> forVm;         // what the VM's own start gets: its options, the program, its arguments
    start_options options;
    forVm.push_back(argv[0]);

    // the settings first: what the command line says is then laid over them
    string settingsFile = "sdb.json";
    for(int k = 1; k + 1 < argc; k++) {
        if(strcmp(argv[k], "-settings") == 0) settingsFile = argv[k + 1];
    }
    string problem;
    if(!read_settings(settingsFile, settings(), problem)) sdb_error(problem);
    if(settingsFile != "sdb.json" && !settings().found) sdb_error("the settings file `" + settingsFile + "` does not exist");
    options.runAtOnce = settings().run;
    options.mapFile = settings().mapFile;
    bool plain = settings().plainView;
    bool scripted = false;

    int i = 1;
    for(; i < argc; i++) {
        if(opt("-V")) {
            cout << SDB_NAME << " " << SDB_VERSION << " (" << progname << " " << progvers << ")" << endl;
            return 0;
        }
        else if(opt("-h") || opt("-?")) {
            sdb_help();
            return 0;
        }
        else if(opt("-run")) {
            options.runAtOnce = true;
        }
        else if(opt("-b")) {
            if(i + 1 >= argc) sdb_error("a function or <file>:<line> is required after option `-b`");
            options.breaks.push_back(argv[++i]);
        }
        else if(opt("-x")) {
            if(i + 1 >= argc) sdb_error("a file is required after option `-x`");
            if(!prompt_read_from(argv[++i])) sdb_error("the commands file `" + string(argv[i]) + "` cannot be read");
            scripted = true;
        }
        else if(opt("-map")) {
            if(i + 1 >= argc) sdb_error("a file is required after option `-map`");
            options.mapFile = argv[++i];
        }
        else if(opt("-settings")) {
            if(i + 1 >= argc) sdb_error("a file is required after option `-settings`");
            i++;                            // (read above, before the other options)
        }
        else if(opt("-plain")) {
            plain = true;
        }
        else if(opt("-machine")) {
            if(i + 1 >= argc) sdb_error("a port is required after option `-machine`");
            options.machinePort = atoi(argv[++i]);
            if(options.machinePort <= 0) sdb_error("`" + string(argv[i]) + "` is not a port: -machine takes a number");
        }
        else if(opt("-nojit")) {
            // sdb never compiles to machine code
        }
        else if(opt("-mem") || opt("-maxlmt") || opt("-stack") || opt("-istack") || opt("-gcstress")) {
            // a VM option with a value: both go to the VM as they are
            forVm.push_back(argv[i]);
            if(i + 1 < argc) forVm.push_back(argv[++i]);
        }
        else if(argv[i][0] == '-') {
            forVm.push_back(argv[i]);       // the VM knows it, or says that it does not
        }
        else break;                         // the program: everything from here on is the VM's
    }

    if(i < argc) {
        options.program = argv[i];
        for(; i < argc; i++) forVm.push_back(argv[i]);
    } else if(!settings().program.empty()) {
        // no program on the command line: the one the settings name, with their arguments
        options.program = settings().program;
        forVm.push_back(settings().program.c_str());
        for(const string &argument : settings().arguments) forVm.push_back(argument.c_str());
    } else {
        sdb_help();
        return 1;
    }
    if(!File::exists(options.program.c_str())) sdb_error("the program `" + options.program + "` does not exist");

    // the full-screen view, where there is a terminal to draw it on and a person to look at it
    // (commands from a file are for reading back: they get the plain view)
    string whyNot;
    options.fullView = !plain && !scripted && options.machinePort == 0 && view_can_be_full(whyNot);
    if(options.machinePort != 0 && !machine_connect(options.machinePort)) {
        sdb_error("nothing listens on port " + std::to_string(options.machinePort) + " of this computer: -machine is for an editor that started sdb");
    }

    c_options.jit = false;
    prompt_start_with(options);
    sdb_hold_when_loaded();
    std::thread prompt(options.machinePort != 0 ? run_machine : run_prompt);

    int result = str_start((int)forVm.size(), forVm.data());

    sdb_program_ended(result);
    prompt.join();
    return result;
}
