//
// Commands that let the program go, or end sdb.
//
// The four that step say HOW FAR the stopped fiber goes (control.h, step_kind);
// the stepping itself happens in the hook.
//

#include "commands.h"
#include "../prompt/prompt.h"

/**
 * A step moves the fiber that stopped AT AN INSTRUCTION. After a pause that
 * found no thread running Sharp code there is none: says so, and gives true.
 */
static bool nothing_to_step(debug_session &session) {
    if(session.stop.atInstruction) return false;
    say("no thread was running Sharp code when the program was paused, so there is nothing to step: `continue` lets it run on");
    return true;
}

static command_result continue_program(debug_session &session, const string &rest) {
    session.step = STEP_NONE;
    return COMMAND_RUN_ON;
}

static command_result next_line(debug_session &session, const string &rest) {
    if(nothing_to_step(session)) return COMMAND_STAY;
    session.step = STEP_NEXT;
    return COMMAND_RUN_ON;
}

static command_result into_call(debug_session &session, const string &rest) {
    if(nothing_to_step(session)) return COMMAND_STAY;
    session.step = STEP_INTO;
    return COMMAND_RUN_ON;
}

static command_result out_of_function(debug_session &session, const string &rest) {
    if(nothing_to_step(session)) return COMMAND_STAY;
    session.step = STEP_OUT;
    return COMMAND_RUN_ON;
}

static command_result into_instruction(debug_session &session, const string &rest) {
    if(nothing_to_step(session)) return COMMAND_STAY;
    session.step = STEP_INTO_INSTRUCTION;
    return COMMAND_RUN_ON;
}

static command_result next_instruction(debug_session &session, const string &rest) {
    if(nothing_to_step(session)) return COMMAND_STAY;
    session.step = STEP_NEXT_INSTRUCTION;
    return COMMAND_RUN_ON;
}

static command_result quit(debug_session &session, const string &rest) {
    return COMMAND_QUIT;
}

void add_running_commands() {
    add_topic("running", "letting the program go on: to the next breakpoint, or a step at a time");

    add_command("running", "continue", "c", {
        "continue",
        "Let the program run until it reaches a breakpoint, or ends.",
        "Every thread runs again. The program stops at the next breakpoint any of them reaches.\n"
        "While it runs, Ctrl+C stops it wherever it is, as a breakpoint would.\n"
        "\n"
        "    (sdb) break notebook.sharp:41\n"
        "    (sdb) continue\n"
        "    breakpoint 1: app#notebook.add  notebook.sharp:41"
    }, continue_program);

    add_command("running", "next", "n", {
        "next",
        "Run to the next line. A function the line calls runs to its end; it is not gone into.",
        "The line with the arrow runs, whole, and the program stops on the line after it.\n"
        "\n"
        "    ->  45      count : _int64 = fill(book);\n"
        "    (sdb) next                                  fill(book) runs to its end\n"
        "    ->  46      println(\"\" + count + \" notes added\");\n"
        "\n"
        "A breakpoint inside a function the line calls still stops the program there.\n"
        "On a function's last line, `next` goes back to the line that called it.\n"
        "To go INTO the function a line calls, use `into`."
    }, next_line);

    add_command("running", "into", "i", {
        "into",
        "Run to the next line, going into a function the line calls.",
        "Like `next`, but when the line calls a function the program stops on that function's\n"
        "first line - also a function of the library.\n"
        "\n"
        "    ->  45      count : _int64 = fill(book);\n"
        "    (sdb) into\n"
        "    app#fill  notebook.sharp:35\n"
        "    ->  35      added : _int64 = 0;\n"
        "\n"
        "A line that calls nothing: `into` and `next` do the same.\n"
        "To come back out of a function you did not mean to enter, use `out`."
    }, into_call);

    add_command("running", "out", "o", {
        "out",
        "Run until the function the program is in has returned.",
        "The rest of the function runs, and the program stops in the function that called it,\n"
        "on the line of the call - that line is not finished yet: what the function gave back\n"
        "is still to be used.\n"
        "\n"
        "    app#notebook.add  notebook.sharp:22\n"
        "    (sdb) out\n"
        "    app#fill  notebook.sharp:37\n"
        "    ->  37      if(book.add(\"bread\")) { added++; }"
    }, out_of_function);

    // The two instruction steps are the two line steps, one instruction at a time:
    //      next  / nexti     stay in the function: what is called runs to its end
    //      into  / intoi     go into what is called
    add_command("running", "nexti", "ni", {
        "nexti",
        "Run one instruction of the VM. A function the instruction calls runs to its end.",
        "A source line is several instructions; `nexti` runs the next one only, and says which\n"
        "instruction of the function the program is at. It is `next`, an instruction at a time:\n"
        "when the instruction is a call, everything the called function does runs, and the\n"
        "program stops on the instruction AFTER the call - still in the same function.\n"
        "\n"
        "    app#fill  notebook.sharp:37  instruction 37        the next instruction calls book.add\n"
        "    (sdb) nexti\n"
        "    app#fill  notebook.sharp:37  instruction 38        ... which has run and returned\n"
        "\n"
        "A breakpoint inside the called function still stops the program there.\n"
        "On a function's last instruction, `nexti` goes back to the function that called it.\n"
        "To go INTO the function an instruction calls, use `intoi`."
    }, next_instruction);

    add_command("running", "intoi", "ii", {
        "intoi",
        "Run one instruction of the VM, going into a function the instruction calls.",
        "It is `into`, an instruction at a time: a call is one instruction, and after it the\n"
        "program is at the FIRST instruction of the function that was called.\n"
        "\n"
        "    app#fill  notebook.sharp:37  instruction 37        the next instruction calls book.add\n"
        "    (sdb) intoi\n"
        "    app#notebook.add  notebook.sharp:16  instruction 0\n"
        "\n"
        "An instruction that calls nothing: `intoi` and `nexti` do the same.\n"
        "These two are the only steps that work in code without line numbers."
    }, into_instruction);

    add_command("running", "quit", "q", {
        "quit",
        "End the program and sdb.",
        "The program is ended where it is: nothing more of it runs."
    }, quit);
}
