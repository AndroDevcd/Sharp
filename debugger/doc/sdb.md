# sdb, the Sharp debugger

`sdb` runs a Sharp program and lets you stop it, look at it and change it: where it is, how it got
there, what its variables hold, what every thread and fiber is doing.

```
sharpc notebook.sharp -dbg -o notebook      build the program with -dbg (and without -O)
sdb notebook                                 debug it
```

- [Starting](#starting)
- [The two views](#the-two-views)
- [The commands](#the-commands)
- [Naming things](#naming-things)
- [Threads and fibers](#threads-and-fibers)
- [Settings: sdb.json](#settings-sdbjson)
- [What the build of a program decides](#what-the-build-of-a-program-decides)
- [Driving sdb from a script or a test](#driving-sdb-from-a-script-or-a-test)
- [How sdb is built](#how-sdb-is-built)
- [Building and testing sdb](#building-and-testing-sdb)

## Starting

```
sdb [options] program [the program's arguments]
sdb [options]                                       the program that sdb.json names
```

| Option | Meaning |
|---|---|
| (none) | Load the program and stop at its `main` |
| `-run` | Start at once: stop only at a breakpoint, or at Ctrl+C |
| `-b <place>` | Set a breakpoint before the program starts; may be given more than once |
| `-x <file>` | Read commands from a file, then from the keyboard |
| `-map <file>` | The name map of a program built with `-obf` (default: `<program>.names` beside it) |
| `-settings <file>` | The settings file (default: `sdb.json` in the folder sdb is started in) |
| `-plain` | Text that scrolls, not the full-screen view |
| `-V`, `-h` | The version; this list |

The VM's own options (`-mem`, `-stack`, `-istack`, `-gclog`, `-gcstress`) may be given too.

`sdb -V` says which VM it is: `sdb 0.1.0 (sharp 4.0.6r19)`. A program is loaded by the VM inside
sdb, so sdb and the compiler that built the program must be of the same release.

## The two views

**The full-screen view** is what a terminal of at least 100 by 20 gets:

```
┌ output ────────────────────────────────────┐┌ long_run.sharp ─ app#spin ───────────────────────┐
│ spinning                                   ││      9  def spin(turns: _int64) : _int64 {       │
│                                            ││     10      total : _int64 = 0;                  │
│                                            ││     11      for(i : _int64 = 0; i < turns; i++) {│
│                                            ││ →   12          total += i % 7;                  │
│                                            ││     13      }                                    │
├ sdb ───────────────────────────────────────┤│ ●   14      return total;                        │
│ (sdb) break long_run.sharp:14              │├ locals ──────────────────────────────────────────┤
│ breakpoint 1 at app#spin  long_run.sharp:14││ turns = 4000000000                               │
│ (sdb) continue                             ││ total = 104796726                                │
│ paused: app#spin  long_run.sharp:12        ││ i     = 34932244                                 │
│ (sdb) █                                    │├ calls ───────────────────────────────────────────┤
│                                            ││ * 0  app#spin  long_run.sharp:12                 │
│                                            ││   1  app#main  long_run.sharp:23                 │
└────────────────────────────────────────────┘└──────────────────────────────────────────────────┘
 paused: app#spin  long_run.sharp:12   (thread 0, fiber 0)
```

- **Left:** what the program prints, above; what you type and what sdb answers, below.
- **Right:** the source around the current line (`→` the line about to run, `●` a breakpoint), the
  variables that can be named there, and the calls that led there. All three follow `frame`,
  `thread` and `fiber`.
- **Last line:** why the program stopped, or that it is running.

| Key | Does |
|---|---|
| Enter | Give the line |
| Tab | Finish a command typed in part (`con` → `continue`); when several fit, they are listed |
| Left, Right | Move in the line; what you type goes in at the cursor. Ctrl+Left / Ctrl+Right: a word at a time |
| Home, End (Ctrl+A, Ctrl+E) | To the start, the end of the line |
| Backspace, Delete | Take away the character before, under the cursor. Ctrl+U: all before it; Ctrl+K: all after |
| Up, Down | The lines given before |
| PageUp, PageDown | Scroll the sdb box |
| Ctrl+L | Draw the screen again |
| Ctrl+D | On an empty line: quit |
| Ctrl+C | While the program runs: stop it where it is. Twice, when nothing stops: end sdb |

While the program runs the keyboard is the program's. When sdb ends the terminal comes back as it
was, with the last of what the program printed.

**The plain view** is text that scrolls: one line for each stop, the same text every time. It is
what a pipe, a commands file (`-x`), a small terminal and Windows get, and what `-plain` or
`"view": "plain"` in `sdb.json` ask for.

```
at the start of the program: app#main  notebook.sharp:43   (thread 0, fiber 0)
->    43      println("loading notes...");
(sdb) break app#fill
breakpoint 1 at app#fill  notebook.sharp:35
(sdb) continue
loading notes...
breakpoint 1: app#fill  notebook.sharp:35   (thread 0, fiber 0)
->    35      added : _int64 = 0;
```

## The commands

`help` lists them by topic; `help <topic>` lists a topic's commands; `help <command>` says all
about one, with examples. The same texts, in short:

### Running

| Command | Short | Does |
|---|---|---|
| `continue` | `c` | Run to the next breakpoint, or the end |
| `next` | `n` | Run to the next line; a function the line calls runs to its end |
| `into` | `i` | Run to the next line, going into a function the line calls (the library's too) |
| `out` | `o` | Run until the function the program is in has returned |
| `nexti` | `ni` | One VM instruction; a function it calls runs to its end |
| `intoi` | `ii` | One VM instruction, going into a function it calls |
| `quit` | `q` | End the program and sdb |

`next` and `nexti` stay in the function; `into` and `intoi` enter what is called. A step moves the
fiber that stopped; every other thread runs freely meanwhile.

### Breakpoints

| Command | Does |
|---|---|
| `break <function>` | Stop when the function is entered: `break app#notebook.add` |
| `break <file>:<line>` | Stop when the line is reached: `break notebook.sharp:41`. `.sharp` may be left out, and a function may stand for its file: `break notebook.add:41` |
| `break <line>` | The number alone: that line of the file in view |
| `breaks` | List them, with their numbers |
| `clear <number>` / `clear all` | Take one away, or all |
| `disable <number>` / `enable <number>` (or `all`) | Switch one off and on; it stays in the list |
| `save` | Write them into `sdb.json` |

A line that holds no code gets no breakpoint: sdb names the next line that does and moves nothing
by itself.

### Looking and changing

| Command | Short | Does |
|---|---|---|
| `backtrace` | `bt` | The calls that led here: frame 0 is where the program is |
| `frame [<number>]` | `f` | Look at one of those calls from now on |
| `list [<line> \| <file>:<line>]` | `l` | The source around a line |
| `locals` | | Every variable that can be named here, and what it holds |
| `print <path>` | `p` | What a variable, a field or an element holds |
| `set <path> = <value>` | | Give it a new value |

A **path** names a value the way the program writes it, and computes nothing:

```
print count               a local variable or a parameter
print self                the object the function was called on
print title               a field of that object
print book.title          a field of another object
print numbers[3]          an element of an array or of a list
print numbers[i]          ... the number taken from a variable
print limit               a static field of the function's class, or a variable of its module
print app#settings.limit  a static field of a named class
print app#visits          a variable of a named module
```

What is shown:

```
count = 3                                              a number
ready = true                                           a bool
name = "milk\tand bread"                               a text
nothing = null
corner = app#point                                     an object: its fields follow, one a line
  x = 3
numbers = _int64[4] [10, 20, 30, 40]                   an array: its first 15 elements, then ...
notes = std#list<std#string> (2) ["milk", "bread"]     a list, the same
```

`set` takes a number, `true` or `false`, a text in quotes, or `null`:

```
set count = 12
set ready = false
set name = "bread"
set numbers[2] = 99
set book.owner = null
```

A text may only be given where a text is held. sdb copies no objects (`set a = b` is not possible).

A variable declared on the current line shows as "not set yet": the line has not run. A variable a
lambda uses and a `localized` variable are read where they really live, so they look like any
other; a `localized` one is the looked-at fiber's own.

### Threads and fibers

| Command | Short | Does |
|---|---|---|
| `threads` | | Every thread, what it was doing, the fiber it runs |
| `thread <id> [<command>]` | `t` | Look at a thread; with a command, for that one command only |
| `fibers [<word>] [<page>]` | | Every fiber, 20 to a page; a word narrows the list |
| `fiber <id> [<command>]` | | Look at a fiber; with a command, for that one command only |

### Help

`help`, `help <topic>`, `help <command>`, and `keys` for the keys of the full-screen view.

## Naming things

- **A function** is named as stack traces name it: `module#class.function`, as in
  `app#notebook.add`. A function written outside any class is `app#fill`.
- **The end of a name is enough** when only one thing fits: `notebook.add`, or `add`. When several
  fit, sdb lists them with their whole names: copy the one you mean.
- **A file** is named by its name, or by enough of its path when two files share a name:
  `notes.sharp`, `shop/notes.sharp`. sdb shows files the same way.

## Threads and fibers

A *thread* is an OS thread that runs Sharp code. A *fiber* is one flow of the program with its own
stack: each thread's own, and every coroutine. A fiber that waits is on no thread.

**When the program stops, every thread of the program stops.** The VM's own threads (the garbage
collector's, the idle thread) never do. The state shown is what the thread or fiber was doing at
that moment - holding the program writes nothing into them:

| A thread is | Meaning |
|---|---|
| `running` | It was executing Sharp code |
| `extern c++` | Inside the VM's C++ or a native function, waiting: the keyboard, a socket, a join |
| `extern c++ (busy)` | Inside C++ and still working: this one did not stop |
| `waiting for a fiber` | It has no fiber that can run |
| `suspended`, `not started`, `finished` | |
| `VM` | One of the VM's own |

| A fiber is | Meaning |
|---|---|
| `running on thread N` | |
| `ready` | It can run and waits for a thread |
| `sleeping, N ms left` | In `delay()` |
| `waiting for a lock that fiber N holds` | |
| `suspended`, `not started`, `finished` | |

- **The scheduler's time stands still while the program is held.** A `delay(1000)` with 400 ms to
  go still has 400 ms to go after you looked around for a minute.
- **A stepped fiber keeps its thread:** the scheduler does not switch it out in the middle of a step.
- **Two threads that stop together** are reported one after the other.
- **`where`** in the lists is the nearest function of your program, not the library function a
  sleeping fiber is really in. `thread <id>` and `fiber <id>` show every call.
- **`thread 4 backtrace`**, `fiber 12 locals`: one command, looking there, and nothing else changes.

## Settings: sdb.json

In the folder sdb is started in (or the file `-settings` names). It need not be there.

```json
{
    "program": "build/app",
    "arguments": ["first", "second"],
    "run": false,
    "map_file": "build/app.names",
    "view": "full",
    "list_lines": 5,
    "save_breakpoints": false,
    "breakpoints": ["app#notebook.add", "notebook.sharp:41", "(off) app#fill"]
}
```

| Setting | Meaning |
|---|---|
| `program`, `arguments` | The program to debug when the command line names none |
| `run` | `true`: the same as `-run` |
| `map_file` | The same as `-map` |
| `view` | `"full"` or `"plain"` |
| `list_lines` | How many lines `list` shows above and below |
| `save_breakpoints` | `true`: every `break`, `clear`, `disable` and `enable` writes the list at once |
| `breakpoints` | Set at the start. One saved with `(off) ` in front is switched off |

- The command line wins over the file.
- `save` (or every change, with `save_breakpoints`) replaces only the `breakpoints` list; every
  other character of the file stays as it is.
- A saved breakpoint that cannot be set any more is reported, not set, not moved, and kept in the file.
- A name sdb does not know is an error that lists the settings.

## What the build of a program decides

| Built | sdb can |
|---|---|
| with `-dbg`, without `-O` | Everything |
| without `-dbg` | Breakpoints, stepping, `backtrace`, static fields - but no local variables and no source text. Said once at the start |
| with `-O` | The same: the optimizer removes and merges variables. Said once at the start |
| with `-obf`, its name map found | Everything the rows above allow. Names are the written ones, with the program's own in brackets: `app#point (ad#br)` |
| with `-obf`, no name map | Nothing: sdb does not open the program |

With `-dbg` the compiler adds a section at the end of the executable that only sdb reads
(`compiler/backend/finalizer/output/debug_locals.sharp`): the name of every local variable,
where it lives, the lines it can be named on, and which fields are bools. The VM itself never asks
for it, and an executable built without `-dbg` and without `-obf` is byte for byte what it was.

One thing to know: the compiler leaves out a field that no code of the program ever reads. Such a
field is not in the executable, so `print` cannot show it.

## Driving sdb from a script or a test

The plain view is stable text, so a program can drive sdb:

```
sdb -x commands.txt program                  the commands of a file, then the keyboard
printf 'break app#fill\ncontinue\nlocals\nquit\n' | sdb program
```

- A command that did not come from a keyboard is printed after the prompt, so the output reads as
  a transcript.
- When the input ends, that is `quit`.
- sdb's exit code is the program's, or 0 after `quit`, or 1 when it refused to start.

sdb's own tests do exactly this, from Sharp: `debugger/tests/sdb_tests.sharp` starts sdb with
`std.system`'s `subprocess`, writes commands and checks the answers.

## Driving sdb from an editor: the machine mode

An editor's debugger does not read the plain view. It starts sdb in the machine mode:

```
sdb -machine <port> program [arguments]
```

- The editor listens on a port of this computer and gives sdb its number. sdb connects to it.
- The two exchange JSON, one object on a line each.
- The program's input and output stay the process's own streams: the editor's console shows them,
  and nothing of sdb's is mixed in.
- The editor can ask while the program runs: a new breakpoint, a pause.

The editor sends requests. Each has an `id` of the editor's choosing; the answer carries it back
with `"ok": true`, or `"ok": false` and `"why"`.

```
{"id": 7, "do": "break", "file": "/home/me/app/notebook.sharp", "line": 41}
{"id": 7, "ok": true, "number": 1, "line": 41}
```

| `do` | With | The answer holds |
|---|---|---|
| `start` | | (the program runs from its first instruction) |
| `break` | `file`, `line` | `number`, `line`; refused: `why`, and `next`, the next line with code |
| `clear` | `number` | |
| `continue` | `fiber` (optional) | |
| `next`, `into`, `out` | `fiber` (optional: the one that stopped) | |
| `pause` | | (a `stopped` event follows) |
| `threads` | | `threads`: `id`, `name`, `state`, `fiber`, `stopped` |
| `frames` | `fiber` | `frames`: `index`, `function`, `file`, `line` |
| `variables` | `fiber`, `frame` | `variables`: `name`, `value`, `kind`, `parts`, `path` |
| `children` | `fiber`, `frame`, `path`, `from`, `count` | `variables`, `total` |
| `set` | `fiber`, `frame`, `path`, `value` | `value`: what it holds now |
| `quit` | | (sdb ends, and the program with it) |

sdb sends events by itself:

| Event | Says |
|---|---|
| `ready` | The program is loaded and nothing has run: set breakpoints, then `start` |
| `stopped` | `reason` (`breakpoint`, `step`, `pause`, `start`), `number`, `thread`, `fiber`, `function`, `file`, `line`, `can_step` |
| `running` | The program runs again |
| `ended` | `code`: the program ran to its end |
| `say` | `text`: sdb's own words (the name map it found, a note about the build) |

- A `path` is what `print` takes. The editor hands back the path it was given, to open a variable
  or to set it.
- `parts` says whether a value can be opened. `children` gives an object's fields, or the elements
  `from` to `from + count` of an array.
- Lines are counted from 1. A file is named by its whole path.
- The frames, the variables and `set` only answer while the program is stopped.

The CLion plugin (`tools/clion-plugin`, package `sharp.ide.debug`) drives sdb this way.
`debugger/machine/machine.h` is the reference.

## How sdb is built

sdb is not a second VM. It is **the VM's own sources with `debugger` compiled in**:

- The VM includes one header of the debugger, `debugger/hooks.h`. It defines a few hooks; built
  with `-DSHARP_DEBUGGER_` they call into the debugger, built without they are empty. The plain
  `sharp` holds no debugger code.
- The hooks: before every instruction of the interpreter; the scheduler's clock; whether a thread
  must keep its fiber.
- sdb is built without the machine-code compiler: the interpreter runs everything, which is where
  the hooks are.

| Folder | Holds |
|---|---|
| `hooks.h` | The one header the VM includes |
| `main.cpp` | sdb's command line; then the VM's own start (`runtime/entry/main.cpp` is the VM's) |
| `control/` | Holding threads, breakpoints, steps, the scheduler's time, threads and fibers |
| `symbols/` | Functions, classes, lines, source text; the name map |
| `values/` | Finding, showing and setting values |
| `commands/` | The commands, one file per help topic |
| `prompt/` | The prompt's loop; where sdb's words go |
| `view/` | The full-screen view |
| `settings/` | `sdb.json` |
| `machine/` | The machine mode: an editor drives sdb |
| `tests/` | The test programs and `sdb_tests.sharp` |

A thread that must stop waits inside the hook, between two instructions. Nothing in the thread or
its fiber is written, and to the garbage collector it counts as a thread that touches no objects.
The prompt runs on a thread of its own.

## Building and testing sdb

| | Linux / WSL | Windows (Git Bash) |
|---|---|---|
| Build | `bash build-scripts/linux/build_sdb.sh` | `bash build-scripts/windows/build_sdb.sh` |
| Test | `bash debugger/tests/run.sh` (test 16 of `test.sh`) | `bash build-scripts/windows/sdb_tests.sh` |

A release builds sdb for both systems and installs it beside `sharp` (`build-scripts/release.sh`).

The machine mode is tested from the CLion plugin: `SdbClientWithRealSdbTest` holds a whole session
with the Windows `sdb.exe`. On Linux it has no automated test yet.

The full-screen view has no automated test. It is checked by running sdb inside `tmux`, sending
keys and reading the screen back (`tmux capture-pane -p`).

## Not there yet

- The full-screen view on Windows (sdb uses the plain view there).
- Assigning one object to another with `set`.
- Evaluating expressions, and conditions on breakpoints.
