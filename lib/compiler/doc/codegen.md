# Code generation, the optimizer and self-hosting

This is the plan for the second half of the bootstrap compiler, and the record
every stage after it keeps up to date. It starts where `doc/backend.md` ends:
with the `analysed_program`, an object in which every name is resolved, every
expression typed and every call bound (`backend.md` sections 6 and 12). It ends
with a compiler that compiles itself.

"Code generation" means turning that analysed program into a file the Sharp
virtual machine (`lib/runtime`) loads and runs. The VM and its file format are
FIXED: everything here is shaped by what the VM's loader reads and what its
instructions do, and both are described below exactly enough that nobody needs
to read the C++ to write the generator.

The oracle is strong: **a program compiled by the bootstrap compiler must
behave exactly like the same program compiled by the C++ compiler** - same
output, same exit code, same exceptions. The bytecode need not be the same.
Where the C++ compiler does something because the VM requires it, the bootstrap
does the same. Where it is an accident of how that compiler grew, the bootstrap
does the textbook thing and section 10 records the difference.

Contents

0. Map of the source (planned; every stage keeps it true)
1. Words used in this document
2. The pipeline: the stages of the build, and the steps of one compile
3. The executable file, byte by byte
4. The virtual machine as code generation sees it: registers, frames, calls,
   objects, exceptions, locks, fibers, function values, natives, the JIT;
   every instruction
5. The design of the generator: the intermediate form, slots, labels,
   packages, what is kept, how each stage is tested
6. The lowering: what every node of the typed tree becomes
7. The optimizer
8. Self-hosting
9. Feature inventory (every construct and every option, with its stage)
10. Deliberate differences from the C++ compiler's code generation
11. Traps in the VM's loader, and other findings
12. The questions, answered (probes of the C++ compiler and the VM)
13. The stages (one section each, filled in by that stage)

---

## 0. Map of the source

What exists (stage 1, "Output file and skeleton", built it: 13.1; stage 2,
"Expressions and calls", split generation into packages and added every
expression: 13.2; stage 3, "Statements and exceptions", added every statement,
the try records and their checks: 13.3; stage 4, "Objects, lambdas, the rest",
added closure records, `localized` locals, components and obfuscation: 13.4; stage 5, "Whole suite green",
added the suite runner, kind-check and programs steps: 13.5; stage 6, "Optimizer", added backend/optimizer: 13.6)
and what is still planned. The rules of `brief.md` apply: one package per folder, 3 to 8
files, the module named after the folder, a front-door file per package whose
header lists the files, dependencies one way (`finalizer` imports
`backend/types`, never the reverse; inside it: `ir` <- `layout` <-
`generation`, `bytecode` <- `verify`, `output`, `listing`; `generation` takes
the asm instruction table from `bytecode`). The packages of `generation` call
each other in a circle - an expression of one family contains expressions of
the others - which is the nature of a recursive walk (the analysis packages of
backend/compiler/expressions do the same).

```
lib/compiler/
  README.md                         for a user: how to build the compiler with itself and how to use it
  main.sharp                        THE DRIVER (module `main`, 13.7): one run of the compiler, from the command
                                    line to the exit code. `main`, and `exec_runtime`: every stage in order, timed
  source_folders.sharp              which files are compiled: the .sharp files below a folder, for `-L <folder>`
                                    and the project mode (one call of the runtime, sorted: 13.5 says why not
                                    file.walk()), the installed library, the files left out with -ignore
  left_out_modules.sharp            a module that is missing because `-ignore` / `-ignoredir` left its files out:
                                    the E0303 gets a note naming the file, and the way back (13.8)
  parsing.sharp                     every file parsed, several at a time; the results put back in the order given
  broken_declarations.sharp         the body stages on a program whose declarations have errors
  executable.sharp                  the options handed to the backend: generate_executable ->
                                    backend.finalizer#generate_program; the warning switches
  project.sharp                     --new-project; where the executable goes (`out`, or
                                    <project>/build/outputs/<output>); -run
  configuration/                    WHAT THE USER ASKED FOR (module `configuration`; 8.2). The bottom of the
                                    driver: it knows nothing of the stages
    configuration.sharp               front door; the compiler's name and version; `error`, `warning`,
                                      each also `_with_help` (a `= help:` line under it), `not_supported_yet`
    options.sharp                     the class `options`: every switch, with its default
    command_line.sharp                the options of the command line, one by one; the --h text
    settings.sharp                    the setters the command line and settings.json share
    settings_processor.sharp          settings.json of a project (the list of settings is at its top)
    platform.sharp                    the platforms -target can name
  cache/                            the build cache of a project (-cache <folder>, -cacheoff, --clean)
  backend/
    finalizer/                      CODE GENERATION (C++: backend/finalizer)
      finalizer.sharp               front door: generate_program (the steps of 2.2), the request from the command
                                    line, E0970 / E0972 / E0975 / E0978, -ir
      layout/                       numbers and names for everything
        layout.sharp                  front door: program_layout, lay_out_program and its steps for the classes:
                                      class addresses, the classes of closure records, kept component definitions
        program_functions.sharp       the steps for the functions: the class each belongs to, the init<C> /
                                      static_init<C> / component functions added, function addresses
        kept.sharp                    what is kept: the walk from the roots (5.5); the classes the VM names (3.11)
        program_order.sharp           the initialisation order: modules by imports, classes as written (6.15)
        class_layout.sharp            a class: field slots (instance after the base's, static, fiber-local),
                                      interfaces, what init<C> / static_init<C> run
        function_layout.sharp         a function: kind, VM flags, name in the executable
        function_types.sharp          the function-type table (3.5), VM type numbers (3.4)
        synthesized.sharp             __srt_global classes, @static_init_flag, init<C>, static_init<C>, the classes of
                                      closure records, a `localized` local's fiber field, a component's function (5.6)
        tables.sharp                  strings, decimal constants, file names; a decimal as exact text (3.6)
      ir/                           THE INTERMEDIATE FORM (5.1, 5.2)
        ir.sharp                      front door: kinds of instruction and of slot, operation codes
        ir_instruction.sharp          one instruction
        ir_function.sharp             one function: instructions, slot kinds, labels; ir_try: a `try`'s ranges as labels
        ir_builder.sharp              slots (reused per kind), labels, line marks, the emit helpers
        ir_printer.sharp              -ir
        function_code.sharp           what the bytecode step makes: words, line table, frame numbers; try_record (3.8)
        number_list.sharp             a growing list of plain numbers
      generation/                   TYPED TREE -> IR (section 6)
        generation.sharp              front door: generate_function, the generator (frame, loops, the `?.` receivers),
                                      "not generated yet"; the map of the packages below
        expressions.sharp             value_of: the dispatcher of every expression; held_over (5.3), has_no_effect
        values/                       the leaves and what is done to one value
          values.sharp                  front door: constants and literals (6.1)
          fields.sharp                  fields of every kind, enum values, `self` / `base` (6.1, 6.2)
          conversions.sharp             every CONV_ kind; conditions (6.3)
          type_tests.sharp              `is`, `sizeof` (6.7, 6.8)
        operators/                    the operators the VM does itself (6.4)
          operators.sharp               front door: binary operators on numbers and references (a chain in a loop)
          logical.sharp                 `&&`, `||`, `? :` (chains and ladders in a loop)
          unary.sharp                   `-a`, `!a`
        assignment/                   =, op=, ++, -- (6.4)
          assignment.sharp              front door: `=` and `op=`
          places.sharp                  a storage place - local, field, element, property - its parts evaluated once
          increments.sharp              `++`, `--`, prefix and postfix
        null_safety/                  (6.5)
          null_safety.sharp             front door: `!!`, `?:`
          safe_navigation.sharp         `?.`, chains, the receiver inside a chain
        calls/                        (6.6)
          calls.sharp                   front door: calls of declared functions (a chain of calls in a loop),
                                        emit_call (the dispatch), arguments
          dispatch.sharp                called_directly: when a virtual call can go straight to its body
          new_objects.sharp             `new c(...)`
          function_values.sharp         function values (named functions, lambdas; a lambda's record in `captured`), calld
        collections/                  (6.6, 6.7)
          arrays.sharp                  front door: elements, new arrays, array literals
          templates.sharp               string templates
          dictionaries.sharp            dictionary literals
        statements/                   (6.9 - 6.11, 6.17)
          statements.sharp              front door: block, expression statement, local declaration, return
          branches.sharp                if / else (an `else if` ladder in a loop), when
          loops.sharp                   while, do, for, foreach, break, continue
          exceptions.sharp              try / catch / finally (the layout of 6.11), throw
          locks.sharp                   lock
          exits.sharp                   return / break / continue out of a lock or a try with a finally: the exit stubs
          assembly.sharp                asm blocks
        closures/                     closure records (6.12)
          closures.sharp                front door: making a record (a function's prologue, an init block, a field's
                                        value), the record a lambda value carries
          records.sharp                 finding a record: by depth (record parameter, then @parent), or the very one
          variables.sharp               a local read and stored wherever it lives: frame slot, record, fiber; `self`
                                        inside a lambda
        setup/                        what the generator writes itself (6.14 - 6.16)
          setup.sharp                   front door: the constructor's prologue, init<C>, a field's start value
          static_parts.sharp            static_init<C>, enum values
          startup.sharp                 platform.static_init / tls_init, the `main` value
          components.sharp              `single` / `factory` definitions as functions; `get<t>()`, `inject`
      bytecode/                     IR -> VM INSTRUCTIONS (5.4)
        bytecode.sharp                front door: lower_function (two passes), frame numbers (4.4), try records from labels
        opcodes.sharp                 opcodes, registers, names, the operand form of every opcode
        encoding.sharp                classes E / D / C / B; the emitter; E0972 (4.2)
        selection.sharp               each IR instruction -> its VM sequence (the right column of section 6):
                                      the dispatcher and the sequences of one or two words
        selection_numbers.sharp       ... constants, copies, conversions, arithmetic, comparisons
        selection_objects.sharp       ... fields (instance, static, fiber-local) and array elements
        selection_calls.sharp         ... calls, calls of function values, asm instructions, stubs
        asm_table.sharp               every asm mnemonic and where its written operands go
      verify/                       THE VERIFIER (5.7)
        verify.sharp                  front door: decoding, jumps, slots, addresses, `ret` at the end
        stack_heights.sharp           the operand stack's height along every path
        slot_kinds.sharp              the kind of every slot an IR instruction touches (whole number, decimal, reference)
        try_records.sharp             the try records: ranges on whole instructions, handlers, slots, R1 and R2 (4.7)
      output/                       THE EXECUTABLE FILE (section 3)
        output.sharp                  front door: write_executable
        byte_buffer.sharp             bytes, big-endian int32, texts, flag chars
        sections.sharp                header, manifest, symbols, strings, constants, data, code, meta
      listing/                      -listing / -d: the file as text (one file: listing.sharp)
      obfuscation/                  -obf AND `obfuscate { }`: THE NAMES IN THE FILE REPLACED (6.17)
        obfuscation.sharp             front door: rename_for_obfuscation; what keeps its name; E0971
        modifiers.sharp               what each name is told: the default, then the rules, the last one wins
        names.sharp                   the generated names (a, b ... aa ...) and the map <output>.names
    optimizer/                      THE OPTIMIZER, -O (stage 6: 7.2, 13.6; C++: backend/optimizer)
      optimizer.sharp               front door: optimize_program, the order of the passes, the local passes' loop
      settings.sharp                -O, -O0, -n<name>, -Oreport; the names of the passes (the C++ compiler's, and four more)
      report.sharp                  what -Oreport prints: per pass the changes, the instructions removed, runs, time
      analysis/                     WHAT THE PASSES SHARE
        analysis.sharp                front door: function_analyses (the graph and the liveness, kept until a pass changes the code)
        instruction_facts.sharp       per IR instruction: the slots it reads and writes, may it raise, has it an effect
        control_flow.sharp            basic blocks, edges, the edges of a `try` (handlers), what can be reached
        liveness.sharp                live slots per block (with the handlers' slots at each instruction that may raise);
                                      the slots a block reads before it writes them
        slot_set.sharp                a set of slots as bits, and tables of them
        labels.sharp                  the labels a `try` needs where they are; the instruction that closes a range (R1)
        loops.sharp                   the loops of a function: dominators, back edges, natural loops
      passes/
        values/                       WHAT A SLOT HOLDS
          values.sharp                  front door
          copy_propagation.sharp        available copies: reads of a copy read the original (numbers; references = object-copies)
          copy_coalescing.sharp         a value made only to be copied is made in the copy's target
          constant_propagation.sharp    known values computed, branches on them decided
          branch_threading.sharp        a jump into a test known on the way in goes past it
          folding.sharp                 the arithmetic, exactly as the VM does it (what is never folded)
          immediates.sharp              `x op k` with a known 32-bit k: int_immediate (iaddl n, k); `x + 0`, `x * 1`, `x * 0`
        removal/                      WHAT NOBODY SEES
          removal.sharp                 front door
          dead_stores.sharp             a value nobody reads again (liveness)
          dead_code.sharp               values that only feed themselves (mark and sweep)
          unreachable_code.sharp        code control can never come to
        control/                      FEWER JUMPS
          control.sharp                 front door
          jump_threading.sharp          jump cleanup: jumps to jumps, to the next instruction, to a return; unused labels
          loop_unrolling.sharp          a loop that runs a small, known number of times becomes straight code
          loop_rotation.sharp           a loop's test to its bottom
        calls/                        CALLS AND THE OBJECTS THEY MAKE
          calls.sharp                   front door
          inline_calls.sharp            a small function's body in place of its call
          box_elimination.sharp         an object that never leaves the function becomes plain slots
        registers/                    WHERE VALUES LIVE (the two passes that run last)
          registers.sharp               front door
          slot_packing.sharp            values never alive at the same time share one frame slot: small frames (13.7)
          register_hand_off.sharp       a value read only by the next instruction stays in its register (the bytecode
                                        step drops the store and the load)
  tests/
    codegen.sh                      the code generation tests (a gates step since stage 1)
    codegen/
      print.sharp                   show(n), show_decimal(d): printing with asm, for the cases
      cases/*.sharp                 a program and the output it must print (`// output:`): compiled as a normal
                                    program, run JIT and -nojit, compared with the block AND with the C++-built program
      own/*.sharp                   the same without the C++ compiler: what it cannot build or builds wrongly
                                    (its bugs; the differences of section 10) - the output block worked out by hand
                                    (since stage 6 every cases/ and own/ program is also built with -O and must print
                                    its block; cases/optimizer.sharp gathers the shapes the passes rewrite, and
                                    optimizer_inlining, _exceptions, _copies, _unrolling what its verifier aimed at them;
                                    optimizer_frames what slot packing must keep - and the recursion it is for)
      suite_running.txt             the suite cases that run, built as normal programs: each must print RESULT PASS,
                                    JIT and -nojit; since stage 4 all 177
      obfuscation/*.sharp           built with -obf: the same output, and the `renamed:` / `kept:` names in the map
      refused/*.sharp               a program code generation must refuse (E0971, E0975): the `// expect:` codes, no file
      library_main.sharp            `def main() { }`: the whole library through layout and the writer
    suite_runner.sh                 the suite judged by its own runner (tests/suite/run.sharp, built by the C++ compiler)
                                    with the bootstrap as <sharpc> (-j 1 -L <library>), JIT and -nojit (stage 5, 13.5),
                                    and both again with -O (stage 6, 13.6)
    kindcheck.sh                    the executables the runner built, run on the kind-check runtime: no report (13.5);
                                    first builds that runtime again when lib/runtime is newer, and runs the probe
    kindcheck_probe.sharp           a program wrong on purpose (a decimal added as a whole number in asm): the
                                    kind-check runtime MUST report it, or its silence means nothing
    programs.sh                     benchmark/kostya and examples/ built by both compilers: the same output and
                                    the same exit code (13.5); the bootstrap's -O build of each as well (13.6)
    optimizer_fuzz.sh               by hand, not a gates step: random programs (optimizer_fuzz.py, one per seed) built
    optimizer_fuzz.py               plain and with -O, run JIT and -nojit - all four must print the same (13.6)
    driver.sh                       the command line: `-L <folder>` and `-j <n>` (nested and hidden folders, a file
                                    named twice, bad thread counts, the same bytes every time, compilers started
                                    side by side, 13.5); the output file, the exit code, -run, projects and
                                    settings.json, the build cache, an option that is accepted only (13.7); empty and
                                    misplaced option values, -o naming a source, a folder named x.sharp, -errlmt on
                                    syntax errors, -ir / -analysis with -L, -run's exit code, an empty settings.json,
                                    a project without the library (13.8)
    selfhost.sh                     SELF-HOSTING (13.7): stage 2 and stage 3 the same bytes, also on 4 threads and
                                    with -O; stage 2 as the compiler - the library, the frontend and backend tests,
                                    every refused program word for word, every suite case the same executable
```

---

## 1. Words used in this document

| word | meaning |
|---|---|
| bytecode | the instructions of the VM: 32-bit words in a function's code. |
| executable | the file the compiler writes and the VM loads (`sharp out`). The C++ calls it the exe. |
| address | an index: a class's address is its position in the VM's class table, a function's in the method table. Not a memory address. |
| slot | one place in a function's frame on the fiber's stack. A slot has two halves: a number (`var`) and a reference (`obj`). Local variables and the generator's temporaries live in slots. |
| register | one of the VM's ten number cells (`adx`, `cx`, `cmt`, `ebx`, `ecx`, `ecf`, `edf`, `ehf`, `bmr`, `egx`). Registers hold numbers only, never references. |
| operand stack | the part of the fiber's stack above the current frame: arguments are pushed there, results come back there. |
| ptr | the VM's "object pointer": the address of one reference half (a slot's, a field's, an array element's). Many instructions read or write through it. |
| IR | intermediate representation: the generator's own small instruction list (section 5.1), made from the typed tree and turned into bytecode by a separate step. |
| lowering | turning a higher form into a lower one: typed tree -> IR, IR -> bytecode. |
| label | a named position in a function's code; a jump names a label, and the real position is filled in later ("fix-up"). |
| basic block | a run of instructions entered only at the top and left only at the bottom. The optimizer works on them. |
| family root | the body-less function at the top of an override family (`sharp_function.family_root`); a virtual call names it. |
| reached | the program can come to it from `main` and the start-up code (`sharp_declaration.reached`). Only reached things are generated. |

---

## 2. The pipeline

### 2.1 The stages of this build, in order

Each stage leaves the gates green and fills in its own section 13.x. The names
are fixed; what goes in each is the plan below, and section 9 assigns every
construct to one of them.

| # | stage | what it builds | what "the gates pass" proves after it |
|---|---|---|---|
| 1 | **Output file and skeleton** | the layout (addresses, slots, names, the function-type table, what is kept), the IR, IR -> bytecode (encoding, labels, line table), the verifier, the file writer, `-listing`; just enough lowering to run straight-line code: literals, number locals, static calls, `return`, expression statements, and `asm` blocks (which give tests a way to print); the options `-o`, `-c`, `-v`, `-target`, `-entry` (a test option, removed in stage 4) | the VM loads every file the bootstrap writes; small programs started with `-entry` print what they should, with and without the JIT |
| 2 | **Expressions and calls** | every expression: values, fields, statics, conversions, casts, `is`, null tests, every operator, assignments and `++`, arrays, `sizeof`, `? :`, `?:`, `?.`, string templates, dictionaries; calls of every dispatch kind (static, direct, virtual, interface, base, native), `new` with the constructor's call and its `-> base(...)`; function values of named functions and of lambdas that capture nothing, and their calls (`calld`) | expression programs give the same output as the C++-built program; the suite cases whose code uses nothing else run when started at `main` (13.2) |
| 3 | **Statements and exceptions** | every statement: blocks, locals with their start values, `if`, `when`, `while`, `do`, `for`, `foreach` (arrays and `loopable`), `break`, `continue`, `return` through the things it leaves; `try` / `catch` / `finally`, `throw`, `lock` | control flow, exceptions and locks behave as with the C++ compiler, including exceptions the VM raises (null, bounds, cast) |
| 4 | **Objects, lambdas, the rest** | how objects and classes are set up: `init<C>`, `init { }`, `init(x)`, `static_init<C>` and its guard, enum values; the start-up (`platform.static_init`, `platform.tls_init`, the `main` function value) - an ordinary `def main()` runs already since stage 3 (13.3), when its program needs nothing of this row; lambdas, closure records, function values and their calls; components; `localized` fields; obfuscation; `-dbg` source text; the rest of the options that change the file | ordinary programs run through `srt_init`; a chosen set of suite cases (the smoke set) passes |
| 5 | **Whole suite green** | nothing new in the language: every difference the 177 suite cases show is found and fixed | every suite case compiled by the bootstrap passes on the VM, JIT and `-nojit`, and the kind-check runtime reports nothing |
| 6 | **Optimizer** | `-O` (section 7; ONE switch, as in the C++ compiler: `-O1` .. `-O3` mean the same, 7.2) | the suite passes at `-O` too, JIT and `-nojit`; the benchmarks run faster; `-O` never changes output |
| 7 | **Self-hosting** | the driver completed (section 8.2); the bootstrap compiles itself twice and the two results are identical | stage 2 and stage 3 of section 8.1 are byte-identical, and stage 2 passes the whole gates as the compiler |

### 2.2 The steps of one compile

What `generate_program` does, in order, once analysis ends with
`analysed.errors == 0`:

```
analysed_program
   |  1. layout       what is kept; addresses of classes, fields, functions; slot numbers of fields;
   |                  the function-type table; synthesized classes and functions; names
   v
program layout
   |  2. generation   for every kept function: typed tree -> IR (section 6)
   v
IR, one ir_function per function
   |  3. optimizer    only with -O (section 7); the IR in, the IR out
   v
   |  4. bytecode     IR -> instructions: frame slots numbered, instructions selected and encoded,
   |                  labels fixed up, line table and try table filled in, frame offsets computed
   |  5. verify       the verifier checks every function (section 5.7); a failure is an internal error
   v
code of every function
   |  6. output       the file of section 3 is written; -listing writes the text form next to it
   v
the executable
```

Each step is one pass over the whole program and gets an entry on the `time:`
line (`layout`, `obfuscation`, `generation`, `optimizer`, `bytecode`,
`write`). Obfuscation (stage 4, 6.17) runs right after layout: it changes only
the names layout gave, so everything after it sees the names the file will
carry. With `-c` the
steps do not run at all. A failure in steps 2-6 is a bug of the compiler, never
of the program: it is reported as an internal error (`E0970`, section 13.1)
naming the function, and no file is written.

`analysis complete: no errors` stays the line that says analysis found nothing
wrong (the test scripts look for it). Generation runs AFTER it is printed
(executable.sharp, `generate_executable`) and adds its own last line: `code
generation: n functions, m classes, written to <file>`, or its messages and
`code generation failed: n error(s), no file was written`.

C++ comparison: `finalize_compilation` in `backend/finalizer/finalizer.cpp` -
`optimize()` (mark what is used, inject field values, assign addresses), then
`generate()` (bytecode per function, `-O`, listing, native glue, executable).

---

## 3. The executable file, byte by byte

Read from `lib/runtime/exe/*.cpp` (the loader) and
`lib/grammar/backend/finalizer/generation/file_generator.cpp` (the C++ writer).
Constants are in `lib/core/exe_macros.h`.

### 3.1 How things are written

| what | how |
|---|---|
| int32 | 4 bytes, **big-endian** (high byte first). `put_int32`, read by `SET_i32(w, x, y, z)`. Negative numbers in two's complement (`-1` is `FF FF FF FF`). |
| text | the bytes, then one `00`. Text must not contain a `00` byte. |
| flag char | ONE ASCII character, `'1'` (0x31) or `'0'` (0x30). (The C++ writes `stream << (b ? 1 : 0)`, which prints the digit.) |
| number text | the number in decimal ASCII, then `00` (the file version, the target). |
| type | see 3.4 |

Between records the loader skips bytes `00`, `0A` and `0D` (the `PROCESS_SECTION`
macro, `exe_processor.h`), so the C++ writer's `'\n'` before each end marker is
harmless; the bootstrap writes the same bytes. Inside a record everything is
read by position.

### 3.2 The parts of the file, in order

```
header                      0F 'S' 'E' 'F'  00 x 15  07 1B 0C
manifest                    01  <13 entries>  0A 03
symbol section              05  <int32 count of function types>  <function-type records>  <class records>  0A 1D
string section              02  <string records>  0A 1D
constant section            02  <constant records>  0A 1D
[compressed marker]         5B, then everything below zlib-compressed (the bootstrap never writes it, 3.10)
data section                0E  <one function record per function, in address order>
code section                <one code record per function, in the same order>  0A 1D
meta section                06  <file records>  0A 1D
```

Each section marker is read with `check_section`, which reads exactly ONE byte:
no `00` / `0A` / `0D` may come between the end of one section and the marker of
the next. (Only the data section has no end marker of its own: it ends where
the first code record's `05` begins.)

### 3.3 Header and manifest

Header: `0F`, the three letters `SEF`, fifteen `00` bytes (`zoffset`), then the
signature `07 1B 0C`. The loader refuses anything else.

Manifest: the byte `01`, then thirteen entries, each a one-byte tag and a value,
in any order but each exactly once (the loader counts them: `HEADER_SIZE` is
13), then `0A 03`.

| tag | value | meaning | the bootstrap writes |
|---|---|---|---|
| `02` | text | application name | the output name (`-o`), as the C++ does (`options.out`) |
| `04` | text | version | `-v`, default `1.0` |
| `05` | flag char | debug: `'1'` means the meta section carries source text | `'1'` with `-dbg`, else `'0'` |
| `06` | int32 | entry function: the address of `platform.kernel#platform.srt_init` | (until stage 4 a test could name another with `-entry f`) |
| `07` | int32 | number of functions (records in the data section) | |
| `08` | int32 | number of classes | |
| `09` | number text | file version; must be 5 (`min_file_vers` .. `file_vers`) | `5` |
| `0C` | int32 | number of strings | |
| `0E` | number text | target platform; must be at most 3 (`alpha_cr4`) | `2` (`alpha`, the C++ default) or what `-target` says |
| `0F` | int32 | number of source files (records in the meta section) | |
| `1B` | int32 | number of fiber-local slots (`localized` fields and locals) | |
| `1C` | int32 | number of decimal constants | |
| `2A` | int32 | application id; native libraries check it (`snb`) | a fixed function of the program (10, G6) |

The VM passes the application name, the version and the target to `srt_init`
as its first `args` (`stack_limits.cpp`).

### 3.4 Types in the file

A type is an int32 `data_type` (`lib/core/data_type.h`), followed by one more
int32 for two of them:

| type | number | followed by |
|---|---|---|
| `_int8 _int16 _int32 _int64` | 0 1 2 3 | - |
| `_uint8 _uint16 _uint32 _uint64` | 4 5 6 7 | - |
| `decimal` (`type_var`) | 9 | - |
| `object` | 10 | - |
| a class | 11 | int32 class address |
| a function type | 12 | int32 index into the function-type table (3.5) |
| nil (no value: a function's result only) | 19 | - |

Anything else makes the loader stop ("invalid type found in symbol table").
Arrays are not part of the type: every type in a record is followed by a flag
char that says "array". Nullability is not written at all.

The loader also builds, from the field types, which slots of an object hold
numbers (`build_number_slot_maps`): a field whose type number is at most 9 and
that is not an array is a number slot; the collector skips those.

### 3.5 The symbol section

`05`, then an int32: the number N of FUNCTION-TYPE records. Then the records,
in any mix (the loader checks the counts):

**Function-type record** (one per distinct function type used by a kept field,
parameter or result; the k-th is referred to by type 12 with index k):

```
1F  int32 parameter count  { type  flag-char-array }*  type(result)  flag-char-array
```

The VM puts them in its method table after the real functions (index
`functions + k`) as `blueprint_function`s. The C++ lists the types of the kept
fields of each class and of the parameters and results of every kept function;
the bootstrap makes the same list, one entry per distinct type (by its text),
and ALSO lists a function type that appears only inside another one (the
parameter or result of a function type). The C++ writer forgets those: it
looks the inner type up, does not find it, and writes index -1, which the
loader turns into "the last real function" without a word. Every type-12 index
the bootstrap writes is checked to be in `0 .. N-1`.

**Class record** (one per kept class; `vm.classes[address]`, so any order):

```
2F  int32 address
    int32 owner address        the class it is nested in; for a class declared in a module, the address
                               of that module's `__srt_global` class; -1 only for the `__srt_global`
                               classes themselves
    int32 base address         -1 only for `std#_object_`, the `__srt_global` classes and the closure
                               classes: every other class, an interface and an enum included, has a
                               base (`std#_object_` or `std#_enum_` when it names none - analysis has
                               already put it in `base_class`)
    int32 guid                 must be UNIQUE: `is`, casts and `catch` compare classes by it
    text  name                 `string`, `list<std#string>`
    text  full name            `std#string`, `std#list<std#string>`, `std#map.entry` (nested)
    int32 static field count   its OWN static fields
    int32 instance field count its instance fields AND those of every class above it
    int32 method count         its functions AND those of every class above it
    int32 interface count      its interfaces AND those of every class above it
    field records              instance fields of the classes above (top first), its own instance
                               fields, its own static fields                     (count: static + instance)
    { 4C int32 function-address 00 }*        the methods: those of the classes above first     (method count)
    { 2E int32 class-address 00 }*           the interfaces: those of the classes above first  (interface count)
```

**Field record** (inside a class record):

```
22  text name  text full name (`std#string.data`)
    int32 slot              instance: its slot in the object (the fields of the classes above come
                            first: base's count + position among the kept own instance fields);
                            static: its position among the kept own static fields (a localized
                            field among them still takes a position, which stays unused);
                            localized: its fiber-local slot (3.3, tag 1B) - it is written in the
                            static part of the list and counted in the static field count
    int32 guid  int32 flags (3.5.1)
    flag-char array  flag-char localized  type
    int32 owner address     the class that DECLARES it (for an inherited field, the base)
    00 00
```

Global variables are the static fields of a hidden class per module,
`<module>#__srt_global` (name `__srt_global`); global functions are its methods.
The bootstrap emits one such class for every module that has a kept global, a
kept global function or a kept class (the classes of the module name it as
their owner). C++: the `flag_global` class every module has.

All of 3.3 - 3.9 was confirmed by decoding files the C++ compiler wrote with a
script that follows this section and nothing else (section 12, "how the
answers were found"): every byte of a 565 KB file was accounted for.

#### 3.5.1 Flags as the VM numbers them

The bootstrap's `FLAG_` numbers are not the VM's. The generator translates
(`lib/core/access_flag.h`):

| VM flag | value | from |
|---|---|---|
| public | 0x001 | `FLAG_PUBLIC` |
| private | 0x002 | `FLAG_PRIVATE` |
| protected | 0x004 | `FLAG_PROTECTED` |
| override | 0x008 | `FLAG_OVERRIDE` |
| excuse | 0x010 | `FLAG_EXCUSE` |
| local | 0x020 | `FLAG_LOCAL` |
| const | 0x040 | `FLAG_CONST` |
| static | 0x080 | `FLAG_STATIC` (every lambda, every global function and global variable: they are static members of their class) |
| stable | 0x100 | `FLAG_STABLE` |
| extension | 0x400 | an extension function |
| native | 0x800 | `FLAG_NATIVE` - the VM sets `nativeFunc` from it |
| global | 0x1000 | never written: the C++ compiler uses it for the `__srt_global` CLASS inside the compiler only. A global function or variable is written `public static` (0x81), as decoded from its output |

The VM reads `static` (in checked builds), `native`, and reflection reads all of
them. `@static_init_flag` is written `private excuse static` (0x92) with type
`_int8`, as the C++ compiler writes it.

### 3.6 String and constant sections

**Strings** (`02`, then per string, then `0A 1D`):

```
1E  int32 length  <length bytes, any values>
```

The string table holds every text a `newstring` names: text literals, the
names an enum value is made with, the texts the generator itself needs. Equal
texts share one entry. Index = position.

**Constants** (`02`, then per decimal constant, then `0A 1D`):

```
1C  <the number as text>  00
```

The loader converts the text with C's `strtod`. Only decimals live here (an
integer never does, see 4.3). The text must give back EXACTLY the double the
compiler means:

- for a literal written in the program the bootstrap writes the literal's own
  digits (without `_`): `strtod` of the same text is what the C++ compiler
  computed too, and the bootstrap's own lexer gets its value from the
  runtime's `strtod` (`frontend/syntax/parser/literals.sharp`), so all three
  agree;
- for a value the compiler computed (constant folding) it writes C99's
  hexadecimal floating form, `0x1.8p+1`, which `strtod` reads exactly. The
  digits are found with arithmetic only (scaling by powers of two is exact in
  binary floating point): `constant_text.sharp`. `inf`, `-inf`, `nan` and
  `-0x0p+0` cover the special values.

C++ comparison: it writes `std::scientific` with 16 digits after the point
(17 significant), which round-trips too; Sharp has no such formatting to rely
on, hence the hex form.

### 3.7 The data section: one record per function

`0E`, then for EVERY function, in address order (function k is the k-th record
and its address field must be k: the loader fills `vm.methods` by position):

```
4C  int32 address            = its position
    int32 guid
    text  name               `append`, `string`, `init<string>`, `static_init<string>`, `lambda@12`
    text  full name          owner's full name + "." + name: `std#string.append`
    int32 source file        index into the meta section's files (3.9)
    int32 owner class address
    int32 function kind      1 normal, 2 constructor, 3 initializer (init<C>, static_init<C>, init(x)),
                             4 operator, 5 delegate (body-less: an interface or abstract function),
                             7 native. Getters (`get_x`), setters (`set_x`), lambdas and global
                             functions are kind 1. (6 is the VM's own mark for a function-type record.)
    int32 stack size         number of frame slots (4.4); at least 1 when it returns a value
    int32 code size          number of 32-bit words in its code record (0 for kinds 5 and 7)
    int32 flags              3.5.1
    int32 delegate address   for a function in an override family that has a body: the address of
                             the family root; -1 otherwise
    int32 fp offset          } how the frame is placed over the arguments (4.4)
    int32 sp offset          }
    int32 frame stack offset }
    type  result  flag-char array
    int32 parameter count    { type  flag-char array }*     (a lambda's hidden record parameter included)
    int32 line count         { int32 pc  int32 line }*      ascending pc
    int32 try count          { try record }*                (3.8)
```

The "delegate address" is how virtual calls work (4.6): `invoke_delegate`
looks, in the object's class and the classes above it, for the method whose
delegate address is the root named by the call.

### 3.8 Try records

```
int32 try start pc   int32 try end pc        the protected range for `finally`: the try body AND its catch handlers
int32 block start pc int32 block end pc      the range whose exceptions the catch clauses take: the try body only
int32 catch count    { int32 handler pc  int32 exception slot (-1: none)  int32 class address }*
01 int32 finally start pc  int32 finally end pc  int32 exception slot      when there is a finally
00                                                                         when there is none
```

All ranges include BOTH ends (`pc >= start && pc <= end`), and a pc counts
WORDS: the end of a range is the last word of its last instruction (the second
word of a two-word instruction). Section 4.7 says what the VM does with them -
including the off-by-one that decides where a range may begin and end - and
6.11 how the bootstrap lays a `try` out. Records are in the order the `try`
statements START in the code: the VM searches from the last record to the
first, so an inner `try` (which starts later) is asked first. The byte after
the catch clauses is `01` for "a finally follows"; anything else means none
(the C++ writes `00`).

### 3.9 The code section and the meta section

Code: for every function, in the same order as the data section, `05` followed
by exactly "code size" int32 words (none for a delegate or native function).
Then `0A 1D`.

Meta: `06`, then one record per source file, then `0A 1D`:

```
2C  text file name       the absolute path, as the stack traces print it
    [int32 length  <the file's text>  00]      only when the manifest's debug flag is '1'
```

A function's "source file" is the file its body is written in; for a function
of a generic instance, the blueprint's file; for a synthesized function, the
file of its class. Only files some kept function points at are listed.

### 3.10 Compression

The C++ writer zlib-compresses everything from the data section on when it is
larger than 1 MB, and marks it with a `5B` byte. The loader decompresses only
when it sees that byte, and reads an uncompressed file of any size. The
bootstrap never compresses (10, G5): no zlib in Sharp, and the files load the
same.

### 3.11 What the VM looks up by name

The VM finds these classes by their full name when it starts
(`vm_initializer.cpp`) and needs them present with every field: `std#throwable`,
`std#runtime_exception`, `std#stack_overflow_exception`,
`std#thread_stack_exception`, `std#out_of_bounds_exception`,
`std#nullptr_exception`, `std#class_cast_exception`,
`std#out_of_memory_exception`, `std#invalid_operation_exception`,
`std#unsatisfied_link_error`, `std#illegal_state_exception`,
`std#incompatible_class_exception`, `std#object_import_error`, `std#string`,
`std#int`, `std#char`, `std#bool`, `std#short`, `std#byte`, `std#uint`,
`std#uchar`, `std#ushort`, `std#long`, `std#ulong`,
`platform.kernel#stack_state`, `std.io#thread`, `std#exception`, `std#error`,
`std#char_array`, `std.io.fiber#fiber`, `std.reflect#_class_`,
`std.reflect#_field_`, `std.reflect#_function_`, `std.reflect#data_type`,
`std.reflect#_function_param_`, `std.reflect#description`, and
`platform.kernel#function_value` (`memory/function_value.cpp`). Analysis's
`analysed.required_classes` is the C++ compiler's list, which leaves some of
them out (`std#bool`, `std#char`, `std#long`, `function_value` ...): layout
keeps every class of THIS list with all its fields (kept.sharp,
`vm_named_classes`) - without `std#bool.value` the VM fails at start-up
(13.1, found). The VM reads fields of them by NAME
(`data`, `name`, `message`, `stack_trace`, `frame_info`, `methods`, `pc`,
`main`, `value`, `ordinal`, `priority`, `id`, `native_handle`, `fib`,
`started`, `stack_size`): these names must survive obfuscation (E0971,
13.4). Native functions are linked by their full name (`snb_link_proc`).

---

## 4. The virtual machine as code generation sees it

### 4.1 Registers

Ten 64-bit cells, private to each fiber. A cell holds an integer or a decimal;
nothing in it says which, the compiler knows (section 4.3).

| # | name | role in the VM | the bootstrap's use |
|---|---|---|---|
| 0 | `adx` | index for `movnd`, `checklen` and array elements; `brh` jumps to it; array sizes | array index and size |
| 1 | `cx` | free | free (optimizer) |
| 2 | `cmt` | written by `lt gt lte gte` (and their `d` twins) and `cmp`; read by `je jne ife ifne` and `get` | comparisons, branch conditions |
| 3 | `ebx` | written by `load_abs`, read by `mov_abs` | first operand, results |
| 4 | `ecx` | free | second operand |
| 5 `ecf` 6 `edf` 7 `ehf` | free | free (optimizer) |
| 8 | `bmr` | written by `test tne and uand or xor` and their twins | results of those |
| 9 | `egx` | free | scratch |

Two facts shape everything:

- **Registers are not kept across a call.** They are shared by every function
  of a fiber (the JIT passes them from compiled function to compiled function
  for free, `x64_generator.h`), so a callee may change any of them. A value
  needed after a call must be in a slot.
- **Registers hold numbers only.** References live in slots, on the operand
  stack, in fields and array elements, and are reached through `ptr`.

The JIT keeps six of the ten in CPU registers ("homes": `egx ecx cx bmr ebx
cmt`); the other four (`adx ecf edf ehf`) stay in memory. Prefer the homes.

### 4.2 How an instruction is encoded

One 32-bit word, or two. The low 8 bits are the opcode. The operands follow in
one of four classes (`lib/core/opcode/opcode_macros.h`); every operand is
SIGN-AND-MAGNITUDE: a magnitude and a separate sign bit, 1 meaning positive
(`posNeg`; zero is written positive):

```
class E   [ 24 bits unused                         | opcode ]                 no operand
class D   [ a: 23 bits            | sa: 1          | opcode ]                 |a| <= 8,388,607
class C   [ b: 18 bits | a: 4 bits | sb: 1 | sa: 1 | opcode ]                 |a| <= 15, |b| <= 262,143
class B   [ c: 7 | b: 7 | a: 7     | sc | sb | sa  | opcode ]                 each |x| <= 127
             bits 25-31  18-24  11-17   10    9    8   0-7
```

So `D: word = op | sa<<8 | a<<9`, `C: op | sa<<8 | sb<<9 | a<<10 | b<<14`,
`B: op | sa<<8 | sb<<9 | sc<<10 | a<<11 | b<<18 | c<<25`. The second word, when
there is one, is a raw signed 32-bit number - except for `invoke_delegate`,
whose second word is a class-C word with opcode `FF` (4.6).

(The C++ builder's own range check for class D allows 0xFFFFFF, one bit more
than fits; the bootstrap checks 23 bits.)

Limits that follow: a function at most 8,388,607 words (jumps are absolute
class-D pcs); slots in a class-C `b` at most 262,143; decimal constants
reachable by `ldc` at most 262,143; field slots for `getf` / `putf` at most
262,143. A function or a program past one of these is `E0972`.

### 4.3 Numbers: integers, decimals, conversions

| kind | in the language | in a cell |
|---|---|---|
| integer | `_int8` .. `_int64`, `_uint8` .. `_uint64` (and everything that is one: `true`, `'c'`) | `int64_t`, wraps around |
| decimal | `decimal` | IEEE `double` |

- Moving a cell (register <-> slot <-> stack <-> number field) is the same
  instruction for both kinds.
- Arithmetic and comparison are not: the plain instruction (`add`, `lt`) works
  on integers, the `d` twin (`dadd`, `dlt`) on decimals. `/` and `**` exist for
  decimals only (`ddiv`, `dexp`); `%`, shifts and bit operators for integers
  only.
- `i2d a, b`: `regs[a] = (double)regs[b]`. `d2i a, b`: the fraction dropped, NaN
  becomes 0, a value out of range the nearest end (`decimal_to_integer`).
- Narrowing is explicit: `mov8 / mov16 / mov32` (signed) and `movu8 / movu16 /
  movu32` (unsigned) cut a register to that width and extend it back to 64
  bits; `mov64` and `movu64` change nothing.
- A comparison gives the integer 0 or 1. `!x` gives 0 or 1 (`not` / `dnot`).
- `movi r, k` loads a 32-bit signed k. A wider integer constant is built
  exactly: `movi r, high; movi egx, 32; shl r, egx -> r; movi egx, low;
  movu32 egx, egx; or r, egx` (result in `bmr`), `movr r, bmr`. (C++: from the
  decimal table through `ldc` + `d2i`, exact only up to 2^53 - 10, G3.)
- A decimal constant: `dmovi r, k` when it is a whole number that fits 32 bits,
  else `ldc r, index` from the constant section.
- `PUT` prints an integer register, `DPUT` a decimal, `PUTC` a character.

When a value is stored into a place of a narrower integer type - a local, a
field, a fiber local, an array element, a function's result - the C++
generator narrows it with `movN` first (array stores cut anyway), and it
narrows an argument in the CALLER before pushing it (`loadl egx, 0; mov8 egx,
egx; rstore egx` for an `_int64` handed to an `_int8` parameter). The
bootstrap narrows wherever a `CONV_NUMERIC` narrows and at every store into a
narrower place (6.2, 6.3). Probed (Q3): `take8(300)`, `return 300` from a
function `: _int8`, `x as _int8`, and stores into an `_int8` local, field and
array element all give 44.

### 4.4 Frames and the calling convention

A function's frame is a run of slots on the fiber's stack, addressed from `fp`.
Slot 0 is `self` for an instance function; then the parameters in order; then
everything else (locals, the generator's slots). Numbering of the bootstrap's
locals: `slot = (instance ? 1 : 0) + local_variable.index` - `locals` already
lists the parameters first (backend.md 12.2).

**A call.** The caller pushes, on the operand stack, `self` (instance
functions only, as a reference), then each argument in order (numbers into the
`var` half with `ipushl` / `istore` / `rstore`, references into the `obj` half
with `pushl` / `pushnull` / `pushobj`), and executes `call address`.
`prepare_method` then does, with P = the number of parameters (a lambda's
hidden record parameter included) and S = 1 for a static function, 0 for an
instance function:

```
fp = sp - fp_offset            so fp is the slot of self, or of the first argument
sp = sp + frame_stack_offset   room for the slots after the parameters
pc = the function's first word
```

**A return.** `returnval r` puts a number result into `fp[0].var`,
`returnobj` puts `*ptr` into `fp[0].obj`; then `ret`. The VM restores the
caller's `sp` to `sp_at_call - sp_offset` and continues after the `call`. For a
function that returns a value, that leaves exactly ONE slot on the caller's
operand stack - the result, where `self` or the first argument was; for one
that returns nothing, every argument (and `self`) is gone.

The three numbers of the function record follow from that (they are the C++
compiler's formulas, `code_context.cpp`, written out):

```
stack size          = max((1 - S) + number of locals (parameters included) + generator slots,
                          returns a value ? 1 : 0)
fp offset           = P - S
sp offset           = P - S + (returns nothing ? 1 : 0)
frame stack offset  = max(0, stack size - (1 - S) - P)
```

The `max(..., 1)` is the bootstrap's own: the C++ compiler writes stack size 0
for a static function without parameters or locals that returns a value
(`time.nano_time`, decoded). Its result slot `fp[0]` then lies one above `sp`,
in a slot `prepare_method` never made sure exists (it grows the stack by
"stack size" = 0). With stack size 1 the frame stack offset becomes 1 and the
result slot is inside the frame; everything else is unchanged.

Example: `def add(a: _int64, b: _int64) : _int64` (static, P = 2) with one more
local and three generator slots: stack size 6, fp offset 1, sp offset 1, frame
stack offset 4. The caller pushes 3 and 4 (`sp` on the 4); `fp = sp - 1` is the
3 (`a`), `fp+1` is `b`; slots 2-5 are made. After `ret` the caller's `sp` is
`sp - 1`: the top is `fp[0]`, the result. A static function with no parameters
that returns a value gets fp offset -1: its frame starts one ABOVE the
caller's top, and that first slot comes back as the result.

**New slots are not cleared.** Slots made by `frame_stack_offset` hold whatever
was there. Every local is given its start value explicitly when it is declared
(0, 0.0, null or a new object, 6.9). Only the reference halves of slots below
`sp` are roots of the garbage collector; a reference left in a slot keeps its
object alive until the slot is overwritten or the frame is gone - harmless.

### 4.5 Objects, arrays, fields

- `newclass C` pushes a new object of class C: one slot per instance field,
  numbers 0, references null. No constructor runs; calling one is the
  generator's job (6.6).
- A **field**: `ptr` is first put on the slot that holds the object (`movl n`
  for frame slot n, `movsl k` for a stack slot, `movg C` for the static object
  of class C, `tls_movl n` for a fiber local, or `movn k` / `movnd r` from
  another object). Then a NUMBER field k is `getf r, k` / `putf r, k` on the
  object in `ptr`; a REFERENCE field k is `movn k` (now `ptr` is on the field's
  slot), then `pushobj` / `popobj` / `smovr_3 n` (copy `*ptr` into frame slot
  n). All of them raise `nullptr_exception` when the object is null.
- **Static fields** live in one "static object" per class: `movg C` then as
  above. Globals are static fields of `<module>#__srt_global`.
- **Arrays**: `newarray r, type` (numbers, `r` holds the size), `newclassarray
  r, C` and `newobjarray r` (references) push a new array. An element: `ptr` on
  the array, the index in `adx`, `checklen adx` (bounds; also null), then for a
  number array `aloadN r, adx` / `astoreN adx, r` - the width and sign are the
  element type's (`_int8` `aload8`, `_uint8` `aloadu8` ... `_int64`, `_uint64`
  and `decimal` all `aload64` / `astore64`) - and for a reference array
  `movnd adx` then as for a reference field. `sizeof r` gives the length (0 for
  null: it does NOT raise).
- **Text**: `newstring k` pushes a new `_int8[]` holding string k. A `string`
  object is made from it by calling the `string(_int8[])` constructor.
- `cast C` checks the object in `ptr` is of class C or below (or implements
  C), else raises `class_cast_exception`; `varcast t, 1` the same for a number
  array of element type t. `is r, x` sets `r` to 1 or 0: x is a class address
  for an object, a `data_type` for a number array. The instruction does not
  say which of the two x is; the VM decides by what the OBJECT is, so a number
  array tested against the class at address 0 - 9 and an object tested against
  a number-array type can answer wrongly (11.6 says how the layout avoids it).
  `is` and `cast` follow base classes and interfaces (and the bases of
  interfaces), comparing class guids.
- `itest r` pops two references and sets `r` to 1 when they are the same
  object; `checknull r` sets `r` to 1 when `*ptr` is null.

### 4.6 Calls of every kind

| call | sequence |
|---|---|
| static function | push the arguments; `call f` |
| instance function, this very body | push `self`, the arguments; `call f` (no null check: a call on null runs until the callee touches a field of it - the C++ output does the same) |
| virtual (override family) | push `self`, the arguments; `invoke_delegate root, argc, 0` - the VM takes the object under the arguments (null: `nullptr_exception`), walks its class and the classes above it, last method first, and calls the first whose delegate address is `root` |
| native function (static) | push the arguments; `invoke_delegate f, argc, 1`: the VM links it by full name on first use and calls the bridge |
| a function value | push the value (a `function_value` object), the arguments; `calld argc`. The VM moves the arguments down over the value, pushes the value's `captured` object after them when it is not null, and calls `address` like a static function |
| constructor | `newclass C`, keep the object; push it, the arguments; `call constructor` (returns nothing, so the object remains wherever it was kept) |

`invoke_delegate a, argc, static` is two words: `[D: a | 0x64]` and
`[C: FF | static (in a) | argc (in b)]`. Its `argc` does not count `self`.

Three facts read from the VM that the table hides:

- **A function value must not hold address 0.** `calld` refuses an address
  `<= 0` ("invalid call to method"). The function at address 0 can therefore
  never be called through a value: layout gives address 0 to
  `platform.tls_init`, which nothing takes the value of (the C++ output has it
  there too).
- **`calld` pushes the record only when it is not null.** A function that
  `takes_record` must always be given a record, and one that does not must
  never get one: otherwise the frame is off by one slot.
- **Only static natives work.** With static = 0 `invoke_delegate` looks the
  address up as a family root, which a native function is not. The C++
  compiler passes the function's own static flag; the library has no native
  function at all, so nothing has ever run either path. A non-static native is
  `E0973` in the bootstrap until the VM can call one.

A body-less function at the top of a family is never called itself:
`invoke_delegate` only looks for functions whose delegate address is it. The
implementations in the method lists of the object's class and of the classes
above it are tried from the LAST method to the first, which is why the method
list is written base first: an override in a class below wins.

The result of a call that returns a value is the top slot: `ipopl n` /
`loadval r` (number) or `popl n` (reference) takes it; `pop` drops it.

### 4.7 Exceptions

`throw` pops an object and starts the search (a null object crashes the VM:
6.11 says what the bootstrap emits instead); the VM raises its own
(`nullptr_exception`, `out_of_bounds_exception`, `class_cast_exception`,
`stack_overflow_exception` ...) the same way. `catch_exception`
(`virtual_machine.cpp`) looks at the CURRENT function's try records, last to
first, with a `pc` that is NOT always the instruction that failed:

| the exception comes from | the `pc` the try records are asked with |
|---|---|
| an instruction of this function (`throw`, a null field access, `checklen` ...) | that instruction's first word |
| a function this one called, the interpreter unwinding it | the word AFTER the call (`call`: its pc + 1; `invoke_delegate`: its second word + 1) - `main_vm_loop` does `pc++` after `return_method` and searches again |
| a function this one called that returned through `ret 1` | the call itself |
| a function called from machine code (the JIT) | not checked; R1 and R2 below cover both answers |

So a call's exception is looked up one instruction late (run: a `try` that
begins directly after a call catches that call's exception; the C++-built
program prints the wrong line, with and without the JIT). The code the
bootstrap writes is right under either reading, by two rules (6.11):

- **R1** the last instruction inside every range is never a call: each range
  ends with the `jump` that leaves it, and that jump is inside the range;
- **R2** the instruction before the first word of every range is never a
  call: a `nop` stands in front of each `try`, outside its ranges.

With that `pc`:

1. if `pc` is in the record's BLOCK range, each catch clause in order: the
   first whose class is the thrown object's class, a class above it, an
   interface of it, or `throwable`, wins - the object is stored into the
   clause's exception slot (unless -1), the locks entered inside the block are
   given back, and execution continues at the handler pc;
2. else, if `pc` is in the record's TRY range and it has a finally: the locks
   entered inside the try range are given back, the object is stored into the
   finally's exception slot, and execution continues at the finally start pc.
   (Nothing tells the finally code that it was entered this way except that
   slot; the generated code re-throws at its end, 6.10.)
3. nothing in this function takes it: every lock the function holds is given
   back, the function's frame is dropped, and the search goes on in the caller
   (with the pc of the table above).

**The operand stack is not cleaned up.** When a handler is entered, `sp` is
wherever it was: values pushed for the instruction that failed are still
there, and an exception that came out of a function that returns a value
leaves that function's result slot (4.4). Frame slots are addressed from `fp`,
so this is harmless for the code of 6.11 - but nothing in a handler may be
addressed from `sp` across the entry, and a loop that catches an exception in
every round uses one or two stack slots more each round until the function
returns (the C++ output does the same; a finding, 11.3).

### 4.8 Locks, fibers, fiber locals, the JIT

- `lock` / `ulock` lock and unlock the object in `ptr` (re-entrant for the
  same fiber). A `lock` that has to wait gives up the thread and is retried
  later at the same instruction. The VM records each block (fiber, call depth,
  pc of the `lock`) and releases them itself when an exception leaves them
  (4.7); a normal exit and a jump out (`return`, `break`, `continue`) must
  `ulock` in the code.
- **Fiber locals** (`localized`): slot k of the BOTTOM of every fiber's stack
  (the manifest says how many; every fiber's frames start above them). A number
  is read with `load_abs k` (into `ebx`) and written with `mov_abs k` (from
  `ebx`); a reference through `tls_movl k`. `platform.tls_init` gives them
  their values in every new fiber (the library calls it: `fiber.sharp`,
  `thread_kernel.sharp`, `srt_init`).
- Fibers are started by the library (`vm.start_fiber`) with a function value;
  threads at `_srt_thread_start`, named in an `asm` block. Nothing in the
  generated code changes for them.
- **The JIT** compiles a function after 10 calls or a hot loop. What it
  assumes about the bytecode: any pc may be a jump target (every instruction
  has an entry in its jump table), so nothing special is needed for exception
  handlers or continuing a fiber; a loop must go back with a backward `jmp` /
  `je` / `jne` (that is where a hot loop is taken over); an instruction it does
  not know leaves the whole function interpreted - so the bootstrap never
  emits the old relative jumps (`skip`, `skpe`, `skne`) nor `ife` / `ifne`.
  A function continues after a context switch in the form it made its call in
  (`continues_in_machine_code`); ordinary calls need nothing special.

### 4.9 Every instruction

Layouts: E (no operand), D (a), C (a, b), B (a, b, c), `+k` a raw second word,
`D+C` see 4.6. "r" is a register, "n" a frame slot, "s" an offset from `sp`,
"pc" an absolute position in the function's code. The interpreter's handlers
are in `main_vm_loop`, in this order; the table mirrors
`lib/grammar/backend/optimizer/instruction_set.cpp`, which says the same for
each row. "Used" says whether the bootstrap's generator emits it (the
optimizer may use more).

| op | name | layout | operands | effect | used |
|---|---|---|---|---|---|
| 00 | nop | E | | nothing | yes |
| 01 | int | D | interrupt | system interrupt; reads/writes registers by `interrupt_signatures.cpp` | asm only |
| 02 | movi | D+k | r, k | `regs[r] = k` (32-bit, sign-extended) | yes |
| 03 | ret | D | 0 / 1 | return; 1: first pop the exception object and raise it in the caller | yes (0) |
| 04 | hlt | E | | stop the thread | asm only |
| 05 | newarray | C | r, type | push a new number array of `regs[r]` elements | yes |
| 06 | cast | D | class | check `*ptr` (4.5) | yes |
| 07-0e | mov8 mov16 mov32 mov64 movu8 movu16 movu32 movu64 | C | r1, r2 | `regs[r1] = (that width) regs[r2]` | yes |
| 0f | rstore | D | r | push `regs[r]` | yes |
| 10 11 12 | add sub mul | B | r1, r2 -> r3 | integer | yes |
| 13 | ddiv | B | r1, r2 -> r3 | decimal `/` | yes |
| 14 | mod | B | r1, r2 -> r3 | integer `%`; 0 divisor raises | yes |
| 15 16 17 | iadd isub imul | D+k | r, k | `regs[r] op= k` (integer) | optimizer |
| 18 | didiv | D+k | r, k | decimal `/= k` | optimizer |
| 19 | imod | D+k | r, k | `%= k` | optimizer |
| 1a | pop | E | | drop the top slot | yes |
| 1b 1c | inc dec | D | r | integer `+= 1`, `-= 1` | yes |
| 1d | movr | C | r1, r2 | `regs[r1] = regs[r2]` | yes |
| 1e | brh | E | | `pc = regs[adx]` | no |
| 1f 20 | ife ifne | E | | `if (cmt) / if (!cmt) pc = adx` | no |
| 21-24 | lt gt lte gte | C | r1, r2 | `cmt = regs[r1] ? regs[r2]` (integer) | yes |
| 25 | movl | D | n | `ptr = &fp[n].obj` | yes |
| 26 | movsl | D | s (signed) | `ptr = &sp[s].obj` | yes |
| 27 | sizeof | D | r | `regs[r] = ptr && *ptr ? length : 0` | yes |
| 28 29 | put putc | D | r | print integer / character | asm only |
| 2a | checklen | D | r | `0 <= regs[r] < length` else `out_of_bounds_exception`; null raises | yes |
| 2b | jmp | D | pc | jump | yes |
| 2c | loadpc | D | r | `regs[r] =` this pc | no |
| 2d | pushobj | E | | push `*ptr` | yes |
| 2e | del | E | | `*ptr = null` | no |
| 2f | call | D | function | 4.4 | yes |
| 30 | newclass | E+k | class | push a new object | yes |
| 31 | movn | E+k | field slot | `ptr = &(*ptr).field[k]` | yes |
| 32 | sleep | D | r | sleep `regs[r]` ms | asm only |
| 33 34 | test tne | C | r1, r2 | `bmr = regs[r1] == / != regs[r2]` (integer) | yes |
| 35 36 | lock ulock | E | | 4.8 | yes |
| 37 | movg | D | class | `ptr` = the class's static object | yes |
| 38 | movnd | D | r | `ptr = &(*ptr).slot[regs[r]]` | yes |
| 39 | newobjarray | D | r | push a new reference array | yes |
| 3a | not | C | r1, r2 | `regs[r1] = !regs[r2]` (integer, 0/1) | yes |
| 3b | skip | D | relative | (being phased out) | never |
| 3c | loadval | D | r | `regs[r] =` pop a number | yes |
| 3d 3e | shl shr | B | r1, r2 -> r3 | shifts (integer) | yes |
| 3f 40 | skpe skne | C | r, relative | (being phased out) | never |
| 41 | and | C | r1, r2 | `bmr = regs[r1] && regs[r2]` (0/1) | no (&& short-circuits) |
| 42 43 44 | uand or xor | C | r1, r2 | `bmr = regs[r1] & / \| / ^ regs[r2]` | yes |
| 45 | throw | E | | pop an object, raise it | yes |
| 46 | checknull | D | r | `regs[r] = ptr == null \|\| *ptr == null` | yes |
| 47 | returnobj | E | | `fp[0].obj = *ptr` | yes |
| 48 | newclassarray | D+k | r, class | push a new array of class references | yes |
| 49 | newstring | D | string | push a new `_int8[]` of that text | yes |
| 4a-4e | addl subl mull ddivl modl | C | r, n | `fp[n].var op= regs[r]` (ddivl decimal) | optimizer |
| 4f-53 | iaddl isubl imull didivl imodl | D+k | n, k | `fp[n].var op= k` | optimizer |
| 54 | loadl | C | r, n | `regs[r] = fp[n].var` | yes |
| 55 | popobj | E | | `*ptr =` pop (null `ptr` raises) | yes |
| 56 | smovr | C | r, s | `sp[s].var = regs[r]` | no |
| 57 58 59 | andl orl xorl | C | r, n | `fp[n].var op= regs[r]` (bits) | optimizer |
| 5a | smov | C | r, s | `regs[r] = sp[s].var` | no |
| 5b | returnval | D | r | `fp[0].var = regs[r]` | yes |
| 5c | istore | E+k | k | push the integer k | yes |
| 5d | smovr_2 | C | r, n | `fp[n].var = regs[r]` | yes |
| 5e | istorel | D+k | n, k | `fp[n].var = k` | yes |
| 5f | popl | D | n | `fp[n].obj =` pop | yes |
| 60 | pushnull | E | | push null | yes |
| 61 | ipushl | D | n | push `fp[n].var` | yes |
| 62 | pushl | D | n | push `fp[n].obj` | yes |
| 63 | itest | D | r | `regs[r] =` (pop == pop) | yes |
| 64 | invoke_delegate | D+C | function, static, argc | 4.6 | yes |
| 65 | get | D | r | read a key (echo when `cmt`) | asm only |
| 66 | isadd | D+k | s, k | `sp[s].var += k` | no |
| 67 68 | je jne | D | pc | `je`: jump when `cmt` is not 0; `jne`: jump when `cmt` is 0 (see the note below) | yes |
| 69 | ipopl | D | n | `fp[n].var =` pop | yes |
| 6a | cmp | D+k | r, k | `cmt = regs[r] == k` | optimizer |
| 6b | calld | D | argc | 4.6 | yes |
| 6c | varcast | C | type, 1 | check `*ptr` is a number array of that type | yes |
| 6d | tls_movl | D | k | `ptr = &stack[k].obj` (fiber local) | yes |
| 6e | dup | E | | push the top reference again | optimizer |
| 6f | popobj_2 | E | | pop; `ptr` = the popped slot | no |
| 70 | swap | E | | swap the two top slots | no |
| 71 | ldc | C | r, index | `regs[r] =` decimal constant | yes |
| 72 | smovr_3 | D | n | `fp[n].obj = *ptr` | yes |
| 73 | neg | C | r1, r2 | integer `-` | yes |
| 74 | dexp | B | r1, r2 -> r3 | `pow` (decimal) | yes |
| 75 | is | D+k | r, class / type | 4.5 | yes |
| 76 | mov_abs | D | k | `stack[k].var = ebx` | yes |
| 77 | load_abs | D | k | `ebx = stack[k].var` | yes |
| 78 | smovr_4 | D+k | n1, n2 | `fp[n1].var = fp[n2].var` | yes |
| 79 7a 7b | dadd dsub dmul | B | r1, r2 -> r3 | decimal | yes |
| 7c | dneg | C | r1, r2 | decimal `-` | yes |
| 7d 7e | dinc ddec | D | r | decimal `+= 1`, `-= 1` | yes |
| 7f-82 | dlt dgt dlte dgte | C | r1, r2 | `cmt =` decimal compare | yes |
| 83 84 | dtest dtne | C | r1, r2 | `bmr =` decimal `==`, `!=` | yes |
| 85 | dnot | C | r1, r2 | `regs[r1] = (regs[r2] == 0.0)` (integer 0/1) | yes |
| 86 | dcmp | D+k | r, k | `cmt = regs[r] == (double)k` | optimizer |
| 87 88 89 | diadd disub dimul | D+k | r, k | decimal `op= k` | optimizer |
| 8a 8b 8c | daddl dsubl dmull | C | r, n | `fp[n].var op= regs[r]` (decimal) | optimizer |
| 8d 8e 8f | diaddl disubl dimull | D+k | n, k | decimal `op= k` | optimizer |
| 90 | disadd | D+k | s, k | decimal `sp[s] += k` | no |
| 91 | distore | E+k | k | push `(double)k` | yes |
| 92 | distorel | D+k | n, k | `fp[n].var = (double)k` | yes |
| 93 | dmovi | D+k | r, k | `regs[r] = (double)k` | yes |
| 94 | dput | D | r | print decimal | asm only |
| 95 96 | i2d d2i | C | r1, r2 | 4.3 | yes |
| 97 98 | getf putf | C | r, field slot | number field of `*ptr` | yes |
| 99-9f | aload8 aloadu8 aload16 aloadu16 aload32 aloadu32 aload64 | C | r, index r | element of the number array at `ptr` | yes |
| a0-a3 | astore8 astore16 astore32 astore64 | C | index r, r | store (cut to the width) | yes |
| ff | ill | | | illegal; only as the opcode of `invoke_delegate`'s second word | - |

Note on `je` / `jne`: `je pc` jumps when `cmt` is NOT zero ("if true"), `jne`
when it IS zero ("if false"). Every "used" instruction must have its JIT
code; the stage that first emits one checks `lib/runtime/jit/x64/opcode`.

### 4.10 The slot forms (a4 - f0)

Added 2026-11-20. The instructions of 4.9 move every value through a register:
an IR instruction `d = a op b` became a load per operand, the operation and a
store - four instructions, and a comparison with a constant followed by a jump
was seven. The slot forms work on the frame slots themselves, so one IR
instruction is one VM instruction. Operands are frame slots (`fp + n`), three
of 7 bits each (layout B), so every slot must be 0 .. 127; a function with
more slots than that keeps the forms of 4.9 for the slots out of reach, and so
does everything that has no slot form (remainder, `**`, static and fiber-local
fields, a narrowing field store). Both are always right; the slot form is only
shorter - in the interpreter one dispatch instead of four, in machine code no
register traffic (and the slot may live in a CPU register: 7.3).

"d" is the slot written, "a" / "b" slots read, "k" a constant, "L" an absolute
pc in the second word. Selection: `bytecode/selection_slots.sharp`; handlers:
the end of `main_vm_loop`; machine code: `jit/x64/opcode/slot_forms.cpp`,
`jit/arm/opcode/slot_forms.cpp`.

| op | name | layout | operands | effect |
|---|---|---|---|---|
| a4-ab | add_s sub_s mul_s and_s or_s xor_s shl_s shr_s | B | d, a, b | `fp[d] = fp[a] op fp[b]` (whole numbers) |
| ac-af | dadd_s dsub_s dmul_s ddiv_s | B | d, a, b | the same for decimals |
| b0 b1 | i2d_s d2i_s | B | d, a | 4.3, slot to slot |
| b2-b7 | jlt_s jgt_s jle_s jge_s jeq_s jne_s | B+L | a, b, sense | `if((fp[a] ? fp[b]) == sense) goto L` |
| b8-bd | djlt_s ... djne_s | B+L | a, b, sense | the same for decimals (with "not a number" every comparison is false but `!=`) |
| be-c4 | aload8_s aloadu8_s aload16_s aloadu16_s aload32_s aloadu32_s aload64_s | B | d, array, index | `fp[d] = array[index]`; null and position checked by the instruction |
| c5-c8 | astore8_s astore16_s astore32_s astore64_s | B | array, index, value | store (cut to the width) |
| c9 ca | getf_s putf_s | B | d, object, field / object, field, value | number field of the object in a slot |
| cb | sizeof_s | B | d, array | its length, 0 for null |
| cc | agetobj_s | B | d, array, index | `fp[d].obj` = element of an array of references |
| cd | getobj_s | B | d, object, field | `fp[d].obj` = a reference field |
| ce cf | jnz_s jz_s | D+L | a | `if(fp[a] != 0)` / `if(fp[a] == 0) goto L` |
| d0 | jnull_s | B+L | a, 0, sense | `if((fp[a].obj is null) == sense) goto L` |
| d1 d2 | isnull_s not_s | B | d, a | `fp[d] = fp[a].obj is null` / `!fp[a]` |
| d3-d8 | mov8_s mov16_s mov32_s movu8_s movu16_s movu32_s | B | d, a | `fp[d] = fp[a]` cut to that width |
| d9 da | returnval_s returnobj_s | D | a | the function's result = `fp[a]` / `fp[a].obj` |
| db-dd | iadd_s isub_s imul_s | B+k | d, a | `fp[d] = fp[a] op k` |
| de | putobj_s | B | object, field, value | a reference into a field, with the write barrier (what `movl`, `movn`, `popobj` do together) |
| df | movobj_s | B | d, a | `fp[d].obj = fp[a].obj` |
| e0-e4 | iand_s ior_s ixor_s ishl_s ishr_s | B+k | d, a | `fp[d] = fp[a] op k` (a shift count is 0 .. 63) |
| e5-ea | jlt_k jgt_k jle_k jge_k jeq_k jne_k | B+L | a, k, sense | `if((fp[a] ? k) == sense) goto L`, k = -127 .. 127 |
| eb-f0 | djlt_k ... djne_k | B+L | a, k, sense | the same for a decimal `fp[a]` |

Three things the bytecode step joins into one of these (all need the
optimizer's hand-off marks, 7.2, which say a value is read once by the very
next instruction):

- a comparison and the jump on it (`jlt_s` ...; `is_null` + jump is `jnull_s`);
- a small constant, the comparison with it, and the jump (`jlt_k` ...);
- with `int_immediate` (the optimizer's, now also for `&`, `|`, `^`, `<<`,
  `>>`): the operation with its constant (`iadd_s` ... `ishr_s`).

The forms that look at an object set the object pointer as `movl` would
(nothing relies on what it was before); in machine code only on the way to an
exception.

---

## 5. The design of the generator

### 5.1 The decision: a small linear IR, lowered by a separate step

The C++ generator turns each expression into a tree of "schemes" with fixed
step layouts and then into bytecode while tracking where the current value is
("machine data": a register? a slot? an object on the stack?). That is the
part of the C++ compiler widely found hard to follow and to change: a value's
place is implicit state, and "previous machine data not consumed" is the error
when two pieces disagree about it.

The bootstrap uses the textbook structure instead (the "three-address code" of
the dragon book, chapter 6):

1. **Generation** walks the typed tree and produces, for each function, a flat
   list of IR instructions. Every value an instruction reads or makes lives in
   a numbered SLOT with a fixed KIND - `INT` (an integer cell), `DEC` (a decimal
   cell) or `REF` (a reference). Locals are slots; every intermediate value
   gets a fresh slot. Control flow is labels and jumps. Nothing is implicit.
2. **Lowering** turns each IR instruction, on its own, into a short fixed VM
   sequence: load the operands from their slots into scratch registers (or
   onto the stack), do the operation, store the result into its slot. No value
   stays in a register from one IR instruction to the next - which also makes
   "registers do not survive a call" (4.1) a non-issue.
3. The **optimizer** (stage 6) works on the IR between the two: it removes
   copies, folds constants, deletes dead code, inlines small functions, and
   leaves a value in its register when only the next instruction reads it
   (section 7). Without `-O` it does not run and the code is simply correct.

Why slots rather than an operand stack or virtual registers mapped onto the
ten VM registers: references can never be in registers (4.1), every call
destroys registers, and the VM's comparisons and bit operations write fixed
registers (`cmt`, `bmr`). A slot is the one place every kind of value can live
across anything. The cost is more `loadl` / `smovr_2` traffic in unoptimized
code; the JIT turns those into single memory moves, and `-O` removes most.

A worked example. The typed tree of

```
def add_twice(a: _int64, b: _int64) : _int64 {
    total := a + b;
    return total * 2;
}
```

becomes (slots: `%0` a, `%1` b, `%2` total, `%3 %4 %5` the generator's, all INT)

```
    int_binary   add  %3, %0, %1
    store_local  %2, %3                  (narrowed if total were narrower than 64 bits)
    int_constant %4, 2
    int_binary   mul  %5, %2, %4
    return_value %5
```

and lowering gives

```
    loadl ebx, 0          loadl ecx, 1          add ebx, ecx -> ebx     smovr_2 ebx, 3
    smovr_4 2, 3
    istorel 4, 2
    loadl ebx, 2          loadl ecx, 4          mul ebx, ecx -> ebx     smovr_2 ebx, 5
    loadl ebx, 5          returnval ebx         ret
```

with stack size 6, fp offset 1, sp offset 1, frame stack offset 4 (4.4). After
`-O`: `loadl ebx, 0  loadl ecx, 1  add ebx, ecx -> ebx  movi ecx, 2  mul ebx,
ecx -> ebx  returnval ebx  ret`.

### 5.2 The IR

`backend/finalizer/ir`. One `ir_function` per generated function:

| field | meaning |
|---|---|
| `function` | the `sharp_function` (or the synthesized one) it is the code of |
| `instructions : list<ir_instruction>` | in order |
| `slot_kinds : list<_int64>` | `SLOT_INT`, `SLOT_DEC`, `SLOT_REF` per slot; slots 0 .. are self, parameters, locals in `locals` order, then the generator's |
| `labels` | label k -> its position (filled by lowering) |
| `tries : list<ir_try>` | one per `try` statement, in the order they start (`ir_function.sharp`): its ranges as LABELS - `start`, `block_end`, `end` (one past the range they close), one handler label, exception slot and class address per catch clause, and for a finally `finally_start`, `finally_end`, `finally_slot` |
| `record_slot` | the slot holding this call's closure record (6.12), -1 when none |

An `ir_instruction` has a kind (`IR_...`), up to three slot operands, an
integer / decimal / text operand, a label, a declaration operand (class,
field, function) and, for a call, a list of argument slots. The kinds, by
family (their lowering is section 6):

| family | instructions |
|---|---|
| constants | `int_constant d, k` · `decimal_constant d, x` · `null d` · `text d, string` (an `_int8[]`) |
| moves | `copy d, s` (same kind) · `store_local d, s, narrow` |
| numbers | `int_binary op d, a, b` (add sub mul rem and or xor shl shr) · `int_immediate op d, a, k` (add sub mul with a 32-bit constant; only the optimizer makes it, 7.2) · `decimal_binary op d, a, b` (add sub mul div pow) · `int_compare op d, a, b` · `decimal_compare op d, a, b` (lt gt le ge eq ne) · `negate d, a` · `not d, a` · `convert how d, s` (int->decimal, decimal->int, narrow to 8/16/32 signed/unsigned, truth of a decimal) |
| references | `same_object d, a, b` · `is_null d, a` |
| fields | `get_field d, object, field` · `set_field object, field, s` · `get_static d, class, field` · `set_static class, field, s` · `get_fiber_local d, k` · `set_fiber_local k, s` |
| objects and arrays | `new_object d, class` · `new_array d, size, element` · `get_element d, array, index, element` · `set_element array, index, s, element` · `length d, array` · `cast d, s, class or number-array type` · `is d, s, class or type` |
| calls | `call d?, function, args, how` (how: direct, virtual through root, native) · `call_value d?, value, args` |
| control | `label L` · `jump L` · `jump_if c, L` · `jump_unless c, L` · `return` · `return_value s` · `throw s` · `lock s` · `unlock s` |
| regions | none: a `try` is the labels of its `ir_try` (above), placed like any other label; the bytecode step reads the try record off their pcs (an end label's range ends at the word before it). The design first had six marker instructions here; labels say the same and need nothing new in the bytecode step |
| other | `line n` (the next instruction starts source line n) · `asm` (one hand-written VM instruction, operands resolved, 6.17) · `nop` |

`-ir` prints the IR of the functions of the last file given, like `-analysis`
prints the typed tree.

### 5.3 Slots

- self, parameters and locals have fixed slots: `(instance ? 1 : 0) + index`.
- The generator asks the builder for a new slot of a kind for every value it
  makes. A slot is given back when the value has been used (the builder knows:
  values in an expression tree are used once, by their parent) and the next
  request of the same kind reuses it - so a function needs about as many
  generator slots as its deepest expression, not one per value.
- Hidden slots the generator adds: the closure record (6.12), the exception
  slot of each `finally` and the value of a `return` that passes a `finally`
  (6.10).
- A captured variable has a slot (analysis numbered it) that is not used; it
  lives in the closure record (6.12). A captured parameter's slot holds the
  argument until the prologue copies it into the record.
- A `localized` local has no frame slot either: it is a fiber-local slot of its
  own, numbered with the `localized` fields (C++: `generate_address`).

**When a local's slot may stand for its value.** Reading a local makes no
instruction: its slot is the value (6.1). That is only right while nothing
writes the local between the moment the expression is "evaluated" and the
instruction that uses it. `f(i, i++)` must pass the OLD `i` as the first
argument; `i++` as a value is the old `i`. The rule (`held_over` and
`held_over_all`, generation/expressions.sharp): when a node has several operands (a
call's object and arguments, the array and index of an element, the value and
the target of a store, the two sides of an operator), an operand whose value
is a local's own slot - a plain local, but also `(x = 5)`, whose value is `x` -
is copied into a fresh slot if an operand evaluated AFTER it writes that local
(an assignment or increment of the same variable anywhere inside it -
`tree_parts.sharp` lists a tree's expressions). So `(x = 5) + x` is 1 + 5 when
x was 1 (the right side runs first, 6.0), and `a[i++] = i` stores the old `i`.
The postfix value of `x++` on a local is always a copy. tests/codegen/cases/
order.sharp holds every such shape, each line checked against the C++-built
program. A call cannot write another function's
uncaptured local, and a captured one is read out of its record at the moment
of evaluation anyway, so nothing else needs this.

The stack size of 4.4 is the number of slots when the function is done.

**Slots cost recursion depth (11.8).** A fiber's whole stack is 4096 slots, so
the operands of an operator, a conversion and a call are given back BEFORE the
slot of the result is taken: the result may land in the slot of one of its
operands (every instruction reads all its operands before it writes its
result, 5.1).

### 5.4 Labels and fix-ups

A label is a number given out by the builder; `label L` places it. Lowering
first computes the length of every IR instruction's VM sequence (it is fixed
for each kind and operands), so the pc of every label is known in one pass,
and writes the code in a second. Jumps are absolute (`jmp pc`). The try
record's pcs and the line table come out of the same first pass. No search
and no list of pending jumps: linear in the size of the function.

### 5.5 What is kept and what is dropped

Analysis has already walked the program from `main`, `srt_init` and the
required classes (`analysed.reached_functions`, `reached_classes`,
`reached_fields`, `sharp_declaration.reached`). Layout keeps exactly those,
plus what the generator itself adds or needs:

- every function of a kept class that is the implementation of a kept family
  root (C++ `mark`: "a delegate function marks all its impls") - if the reached
  set does not already contain them, layout adds them;
- the start-up functions and the classes they touch (`platform.static_init`,
  `platform.tls_init` are reached through `srt_init`);
- the synthesized members of 5.6;
- the module global classes that have kept members.

A kept class keeps only its kept fields: the field slots and an object's
`sizeof` count them alone, exactly as in the C++ output (probed, Q10: a class
with two fields of which the program names one has `sizeof` 1). A required
class keeps all its fields (analysis already counts them reached).

**A field that is not kept is not given its value either - and that can be
seen.** Probed: with the C++ compiler `static v : _int64 = say("hello");`
prints nothing when no code names `v`, although its class is used (`x is c`,
`new c()`); the same for an instance field and for a global. A field that is
only WRITTEN (`c.v = 5;`) counts as named: its value runs. So "kept" for a
field must mean exactly what the C++ compiler's `used` means: some reached
code names it (reads or writes it), or the VM needs it. Analysis says "a class
reaches its initialisation lists" (backend.md, "Reachability, as built"); if
that marks every field with a value as reached, the bootstrap would run values
the C++ build skips. It does mark them, so `kept.sharp` does not use
`reached_fields`: it walks the program itself, and there a class does not keep
its fields - only a use does (Q14, answered in stage 1;
tests/codegen/cases/kept_fields.sharp checks it against the C++-built program).

Two more rules of the kept walk, both stage 1's: an override is kept when its
family root is kept AND its class is kept (the C++ compiler keeps every
override of a kept root, in classes no object is ever made of too: G22), and
the classes the VM looks up by name are kept whole (3.11).

### 5.6 What the generator adds

| added | for | what it is |
|---|---|---|
| `<module>#__srt_global` | every module with kept globals or global functions | a class: globals are its static fields, global functions its methods (flag global) |
| `@static_init_flag` (static number field) and `static_init<C>()` | every class (and module global class) with a static part: static field values, enum values, `static init { }` | the class's static initialisation, run once (6.15) |
| `init<C>()` | every class with instance initializers | the field values and `init { }` blocks of one object, in `instance_initializers` order (6.14) |
| a closure class per `closure_record` | lambdas that capture | one field per captured variable, `@parent`, `@self` (6.12) |
| `@sub_component_<n>` (static field), `@sub_component_init_flag_<n>` and `@sub_component_init_<n>()` | every kept `single` definition, in its module's `__srt_global` | the one object, made on first use (6.16) |
| `@sub_component_init_<n>()` | every kept `factory` definition | makes a new object at each call (no field) |
| a fiber-local field per `localized` local | every kept function with one | its own fiber-local slot, after the `localized` fields (6.17) |

The `@` makes a clash with a program's names impossible. The names of the
first four follow the C++ compiler's, so reflection and stack traces read the
same. Decoded from its output (Q7): a getter is `get_<field>`, a setter
`set_<field>`; every `init { }` block of a class is part of `init<C>`; a
lambda is `@anonymous_fun_<n>` with n from the compiler's random-offset id
counter (the bootstrap: `@anonymous_fun_<k>`, k counting the lambdas in
`analysed.lambdas` order); a closure class is `closure_<full name of the
function>` in the function's module, with no base class and no functions.

The C++ compiler gives EVERY class a `static_init<C>` and a
`@static_init_flag`, used or not, and every constructor the test of 6.14. The
bootstrap makes them only for a class with a static part: an empty function
and a test that always passes cannot be seen. (Reflection lists one method and
one field fewer for such a class; nothing in the library or the suite looks.)

### 5.7 The verifier

Before a function is written, `verify/` checks its code; a failure is `E0970`
naming the function and the instruction. It is the cheapest way to find a
generator bug without running anything (the JVM's bytecode verifier is the
model):

- every operand fits its encoding (4.2); every jump lands on the first word of
  an instruction inside the function; every slot is below the stack size;
- the operand stack has the same height on every path into each instruction,
  never goes below the frame, and is empty at every `line` mark and label
  that a jump reaches (values do not stay on the stack across statements). A
  handler pc (a catch clause, a finally) counts as height 0: whatever the VM
  left below is never touched (4.7), and no `movsl` / `smov` / `smovr` reaches
  below the values the instruction sequence itself pushed;
- R1 and R2 of 4.7 hold for every try record: the word before each range is
  not the last word of a `call`, `calld` or `invoke_delegate`, and the last
  instruction of each range is none of those;
- `calld n` takes the function value and n arguments and leaves a result when
  the function returns one - which its word does not say: the bytecode step
  notes the pc of every `calld` that leaves a result
  (`function_code.value_calls_returning`) and the stack-height walk reads it;
- no function value is made with address 0; every type-12 index is inside the
  function-type table; a function that returns a value has stack size >= 1;
- every IR instruction reads and writes slots of the KIND it works on
  (`verify/slot_kinds.sharp`, on the IR the words were made from): the three
  slots of `decimal_binary` hold decimals, a comparison's result is a whole
  number, the value stored into a field is of the field's kind, each argument
  of a call is of its parameter's kind and the result of the function's. A
  slot never changes kind and every VM sequence loads and stores whole slots
  (5.1), so this is the static form of the runtime's kind checker
  (`memory/type_check.h`): a decimal can never be read as a whole number. It
  was added by the verifier of stage 2 after `7 ** 2` printed the bits of
  49.0 - a decimal result written into a whole-number slot, which nothing
  had caught (13.2);
- a function ends with `ret`; try record pcs are ordered (`try start <= block
  start <= block end <= try end`) and inside the code;
- the loader traps of section 11 cannot happen.

### 5.8 Testing, stage by stage, before the whole suite can run

The suite cases need the platform start-up (`srt_init`), which uses string
templates, `asm`, `for`, `when`, casts and function values - nearly the whole
language. Until stage 3 the suite could not run, so the earlier stages tested
with small programs started directly. What that took, and what is left of it
since stage 4:

- **`-entry <name>` and `-stubs` (stages 1 - 3, removed in stage 4).** A test
  option made the manifest's entry a static function of the program instead
  of `srt_init`, whose prologue ran what srt_init would have (the
  `localized` values, every static part); the kept walk started at that
  function. With `-stubs` a function holding a node the stage had no
  lowering for was written as a STUB that printed `not generated yet:
  <function>` and returned 0 / null; without it the compile stopped with
  E0974. The count of stubs was each stage's progress measure: the library
  had none since stage 3, the suite none since stage 4. Stage 4 deleted both
  options, the stub instruction (`IR_STUB`), the test entry's prologue and
  E0974, E0976, E0977 as planned: every node of the typed tree has a lowering
  now, and a node the generator still meets without one is a bug of the
  compiler, `E0970` ("internal error: code generation has no code for ...").
- **Printing without the library**: `tests/codegen/print.sharp` has
  `show(n: _int64)` and `show_decimal(d: decimal)` written as `asm` blocks
  (`loadl ebx, [n]  put ebx`). Stage 1 lowered `asm` first for this reason;
  the cases still print with them, and with `println` where a text is shown.
- **`tests/codegen/cases/<name>.sharp`**: a program with a `main` and the
  output it must print in a comment block:

  ```
  // output:
  // 7
  // 3.5
  ```

  `tests/codegen.sh` compiles each case with the bootstrap as a normal program
  (since stage 4; started at srt_init like any program), runs it on the VM
  with and without the JIT, and compares the output with the block. It ALSO
  compiles the case with the C++ compiler and compares: the oracle, applied
  to every case. Only the OUTPUT is compared: the VM's exit code is 0 whether
  `main` returns 3, returns nothing or ends in an uncaught exception (probed,
  Q13), and an uncaught exception's stack trace prints function addresses and
  pcs, which differ by design.
- **`tests/codegen/own/<name>.sharp`**: the same, without the second half.
  For a program the C++ compiler cannot build, or builds wrongly (its bugs,
  and the deliberate differences of section 10), there is no oracle: the
  output block is worked out by hand from the rule of the language, the file
  says which rule and why the C++ compiler is not asked, and the bootstrap's
  program must print the block with and without the JIT. Kept small on
  purpose - a case belongs in `cases/` whenever the C++ compiler can check it.
- **`tests/codegen/obfuscation/<name>.sharp`** (stage 4): compiled with `-obf`;
  the program must print its block, JIT and `-nojit`, and the map of new names
  (`<output>.names`) must list each `// renamed: <full name>` line of the
  file and none of its `// kept: <full name>` lines.
- **`tests/codegen/refused/<name>.sharp`** (stage 4; it replaces `stops/`, the
  programs that had to stop with E0974): a program analysis accepts and code
  generation must refuse (E0971: an `obfuscate` block renaming what the VM
  finds by name). The codes of its `// expect:` line must be printed in that
  order, and no file written.
- A case for each loader trap and each limit.
- The gates' codegen step (tests/codegen.sh) compiles the whole library with
  `-o -eager`, so layout, the file writer, the verifier and the VM's loader
  meet all of it at once, and the VM must run the file to its end, JIT and
  `-nojit`: srt_init, the `main` of nothing, the shutdown - printing nothing,
  exit code 0. The same for the program a `main` of nothing really is, and
  for that program built with `-obf`. The gates' library step itself stays
  analysis-only. The verifier runs on every generated function of every
  compile. Without `-o` nothing is generated and the analysis-only runs of
  the gates stay exactly as they are.
- **The suite (from stage 2).** Every suite case is also compiled (with
  `testing.sharp`) and run, JIT and `-nojit`, as a normal program (since stage
  3; in stage 2 with `-entry main`). Since stage 4 every case must run and
  print `RESULT PASS` both ways - all 177 are listed in
  `tests/codegen/suite_running.txt`; until then a case could also WAIT (stop
  with E0974 at a construct of a later stage). A listed case that does not run
  fails the step, and one that runs and is not listed is reported to be added.
- From stage 5: the whole suite through the suite runner, with the bootstrap as
  `<sharpc>` (a two-line wrapper script that runs `sharp sharpc_boot "$@"`),
  JIT and `-nojit`, and on the kind-check runtime - done in stage 5:
  tests/suite_runner.sh, tests/kindcheck.sh (`~/sharp_work/sharp_tc_dev`),
  tests/programs.sh for the benchmarks and examples, and tests/driver.sh for
  the two options the runner calls the compiler with (13.5).

The gates grow with each stage (never shrink): stage 1 added "codegen tests",
stages 2 - 4 the suite cases compiled by the bootstrap and run (stage 4: every
one of them), stage 5 adds the suite run through the suite runner and the
kind-check runtime, stage 6 the same at `-O` (and every codegen case at `-O`), stage 7 the self-hosting check.

---

## 6. The lowering

For every node of the typed tree: the IR it becomes (left), and the VM code of
the IR instructions (right). `%x` are slots, `r1 r2` scratch registers
(`ebx`, `ecx` in practice), `->` "stored into". A node's "value" is the slot
its IR leaves the result in. Every node has `unit` and `node`; the first IR
instruction of each statement is preceded by `line <its line>`.

### 6.0 The order of evaluation

The order in which the parts of an expression run shows as soon as they have
effects (a call that prints, `i++`). It is a rule of the language - the suite
case `operators_evaluation_order.sharp` states "the operands of a binary
operator are evaluated RIGHT TO LEFT" - and the typed tree does not record it,
so the generator must follow this table. Every row was measured on the
C++-built program (section 12, Q6), each part a call that leaves a digit:

| construct | order | measured |
|---|---|---|
| a number operator `a op b` (`+ - * / % ** & \| ^ << >>`, `< > <= >=`, `== !=` on numbers) | **`b`, then `a`** | `m(1) < m(2)` gives 21; `-m(1) + m(2) * m(3) - m(4)` gives 4321 |
| `a && b`, `a \|\| b`, `c ? x : y`, `a ?: b` | left first; the rest only when needed | 12 |
| a call `o.f(x, y)`, `f(x, y)`, `new c(x, y)` | the object, then the arguments left to right | 123 |
| an operator that is a call (`a + b` on objects, `a == b` through `operator==`, text `+`) | as a call: `a`, then `b` | 12, 123 |
| a string template, an array literal, a dictionary | the pieces left to right | 123 |
| `a[i]` read | `a`, then `i` | 12 |
| `target = value`, `target op= value` | **`value` first**, then the parts of the target left to right (the object of a field; the array, then the index), then - for `op=` - the target's old value | `a(1)[m(2)] = m(3)` gives 312; `g(1).f += m(2)` gives 21 |
| `target++`, `target--` | the parts of the target left to right | 12 |
| a cast, a conversion, a unary operator | its operand | - |

A part that is a plain local needs the care of 5.3 ("when a local's slot may
stand for its value").

### 6.1 Constants and leaves

| node | IR | VM |
|---|---|---|
| any expression with `constant != null` (a number or a text) | the constant instead of the computation - but only when the operand parts have no effect (no call, `new`, assignment, increment); otherwise they are evaluated and dropped first | |
| `bound_literal` integer, char, bool | `int_constant %d, k` | `istorel d, k` (k fits 32 bits), else the wide sequence of 4.3 then `smovr_2` |
| `bound_literal` decimal | `decimal_constant %d, x` | `distorel d, k` (whole, fits 32 bits), else `ldc ebx, index; smovr_2 ebx, d` |
| `bound_literal` text (type `_int8[]`) | `text %d, s` | `newstring s; popl d` |
| text as a `string` | a `CONV_CONSTRUCT` of the text (6.3) | |
| `bound_null` | `null %d` | `pushnull; popl d` |
| `bound_local`, own and not captured | the variable's slot itself (no instruction) | |
| `bound_local`, captured | `get_field %d, <record>, <variable's field>` with the record found as in 6.12 | |
| `bound_local`, `localized` | `get_fiber_local %d, k` | `load_abs k; smovr_2 ebx, d` / `tls_movl k; smovr_3 d` |
| `bound_self` depth 0 | slot 0 | |
| `bound_self` / `bound_base` depth d > 0 | `get_field` of `@self` in the record d steps out (6.12) | |
| `bound_base` | as `bound_self`; only ever the object of a `CALL_BASE` | |
| `bound_receiver` | the slot the enclosing `?.` put its receiver in (6.5) | |
| `bound_type_name` | nothing (it is only the `on` of a static member or the operand of `is` / `as` / `sizeof`) | |
| `bound_functions`, `bound_pending`, `bound_error` | never in a program without errors: `E0970` | |

### 6.2 Fields and storage

| node | IR | VM |
|---|---|---|
| `bound_field`, instance, `on` given | `get_field %d, %on, f` | number: `movl on; getf ebx, slot; smovr_2 ebx, d` · reference: `movl on; movn slot; smovr_3 d` |
| `bound_field`, static (`on` null, `owner` a class) | `get_static %d, C, f` | `movg C;` then as above |
| `bound_field`, global (`owner` null) | `get_static %d, <module>#__srt_global, f` | as above |
| `bound_field`, `localized` | `get_fiber_local %d, k` | `load_abs k; smovr_2 ebx, d` / `tls_movl k; smovr_3 d` |
| `bound_enum_value` | `get_static %d, <the enum>, <the value's field>` | |
| a field with a getter, read outside its accessors | a `bound_call` (`AS_GETTER`) already: 6.6 | |

Storing (the target of `=`, `+=`, `++`; the `STORE_` kind says which):

| storage | IR | VM |
|---|---|---|
| `STORE_LOCAL`, own | `store_local %slot, %v, narrow` | number: `loadl ebx, v; [movN ebx, ebx]; smovr_2 ebx, slot` (or `smovr_4 slot, v` when nothing narrows) · reference: `pushl v; popl slot` |
| `STORE_LOCAL`, captured | `set_field <record>, <field>, %v` | number: `loadl ebx, v; [movN]; movl record; putf ebx, slot` · reference: `pushl v; movl record; movn slot; popobj` |
| `STORE_LOCAL`, `localized` | `set_fiber_local k, %v` | `loadl ebx, v; mov_abs k` / `pushl v; tls_movl k; popobj` |
| `STORE_FIELD` instance | `set_field %on, f, %v` | as for a captured local, with `on` |
| `STORE_FIELD` static / global | `set_static C, f, %v` | `movg C` in place of `movl` |
| `STORE_ELEMENT` | `set_element %array, %index, %v, element` | 6.7 |

"narrow" is the field's / local's own type when it is an integer narrower than
64 bits: `_int8` -> `mov8` ... `_uint32` -> `movu32`. The C++ generator narrows
at stores into locals, fields, static fields and array elements, at arguments
and at `return`; it does NOT narrow a store into a fiber local (probed, Q11:
`localized tiny : _int8; tiny = 300;` then reads 300). That is a slip of the
C++ generator, not a rule: the bootstrap narrows there like everywhere else
(10, G16).

### 6.3 Conversions (`bound_conversion`, `bound_cast`; `how.kind`)

| kind | IR |
|---|---|
| `CONV_IDENTITY`, `CONV_TO_NULLABLE`, `CONV_NULL`, `CONV_UPCAST`, `CONV_TO_OBJECT`, `CONV_RESULT_IGNORED` | nothing: the operand's slot is the value |
| `CONV_NUMERIC` integer -> narrower integer | `convert narrow_N %d, %s` -> `loadl ebx, s; movN ebx, ebx; smovr_2 ebx, d` |
| `CONV_NUMERIC` integer -> integer of the same or greater width, or between `_intN` and `_uintN` of 64 bits | nothing |
| `CONV_NUMERIC` integer -> decimal | `convert int_to_decimal` -> `i2d ebx, ebx` |
| `CONV_NUMERIC` decimal -> integer | `convert decimal_to_int` (-> `d2i`), then narrow when the target is narrower |
| `CONV_DOWNCAST`, `how.checked` = 1, to a class or interface | `cast %d, %s, C` -> `movl s; cast C` (the value stays in `s`: `d` is `s`) |
| `CONV_DOWNCAST` to an array of a class (`copy(...) as T[]`) | `cast %d, %s, C` with C the element's class: the C++ compiler checks the class and ignores the `[]` (probed: an object of the class itself passes too; an `_int64[]` raises class_cast_exception) |
| `CONV_DOWNCAST` to a number array (`args[0] as _int8[]`) | `cast %d, %s, type` -> `movl s; varcast type, 1` |
| `CONV_DOWNCAST` to `object[]` (`x as object[]`) | nothing: every array of objects is one, and the VM has no test that tells one from any other object. The C++ compiler emits nothing either (`create_cast_operation`: "ignore var and object casts"; probed) |
| `CONV_DOWNCAST` unchecked | nothing |
| `CONV_UNBOX` | `get_field %d, %s, how.field` (the box's `value`; a null box raises in `getf`, as in the C++ output) |
| `CONV_CONSTRUCT` | `how.inner` on the value first; then `new_object %o, how.function.owner`; `call -, how.function, [%o, %v], direct`; the value is `%o` |
| `CONV_ERROR`, `CONV_NONE` ("does not convert") | never: `E0970` |

A null operand of `cast` raises `nullptr_exception` (the instructions require
an object) - the C++ output does the same.

A CONDITION (of `if`, a loop, `&&`, `||`, `? :`, a `when` test) is an integer;
when analysis hands over a decimal, `convert truth` makes it one: `dnot ebx,
ebx; not ebx, ebx`. `-0.0` is false that way, as in the C++-built program
(probed, Q12); NaN is true.

### 6.4 Operators

| node | IR | VM |
|---|---|---|
| every `bound_binary` on numbers | `%r = right` FIRST, then `%l = left` (6.0), then the row below. A chain `a + b + c + d` gives the same instructions in the same order, written by a loop down its left side instead of one level of recursion per operator (13.3, "chains") | |
| `bound_binary`, `OPERANDS_INTEGER`: `+ - * % & \| ^ << >>` | `int_binary op %d, %l, %r` | `loadl ebx, l; loadl ecx, r; add ebx, ecx -> ebx; smovr_2 ebx, d` (`%`: `mod`; `&`: `uand ebx, ecx` then `smovr_2 bmr, d`; `\|`: `or`; `^`: `xor`; `<<`: `shl`; `>>`: `shr`) |
| `OPERANDS_INTEGER` comparisons `< > <= >=` | `int_compare` | `lt ebx, ecx; smovr_2 cmt, d` (gt, lte, gte) |
| `OPERANDS_INTEGER` `== !=` | `int_compare eq/ne` | `test ebx, ecx` / `tne`; `smovr_2 bmr, d` |
| `OPERANDS_DECIMAL` (also every `/` and `**`) | `decimal_binary` / `decimal_compare` | the `d` twins: `dadd dsub dmul ddiv dexp dlt dgt dlte dgte dtest dtne` |
| `**` of two WHOLE numbers (`count ** 2`: `OPERANDS_DECIMAL`, but the node's type is a whole number - backend.md 7.8.1, "calculated with pow and cut back") | `decimal_binary pow %t, %l, %r` into a decimal slot of its own, then `convert decimal_to_int %d, %t` | `dexp`, then `d2i`: `7 ** 2` is the whole number 49, as in the C++-built program |
| `OPERANDS_REFERENCE` `==` / `!=` | `same_object %d, %l, %r` (then `not` for `!=`) | `pushl l; pushl r; itest ebx; smovr_2 ebx, d` |
| `OPERANDS_NULL_TEST` | `is_null %d, %x` (then `not` for `!=`) | `movl x; checknull ebx; smovr_2 ebx, d` |
| `bound_logical` `a && b` | `%d = 0; c = a; jump_unless c, end; c = b; jump_unless c, end; %d = 1; end:` | |
| `bound_logical` `a \|\| b` | `%d = 1; c = a; jump_if c, end; c = b; jump_if c, end; %d = 0; end:` | the result is always 0 or 1, never an operand |
| `bound_unary` `-a` | `negate` | `neg ebx, ebx` / `dneg` |
| `bound_unary` `!a` | `not` | `not ebx, ebx` / `dnot` (always an integer result) |
| `bound_ternary` | `c = cond; jump_unless c, else; %d = when_true; jump end; else: %d = when_false; end:` | |
| `bound_elvis` | `%d = value; n = is_null %d; jump_unless n, end; %d = fallback; end:` | |
| an operator on an object (`a + b` with `a : money`), `o[i]`, `o[i] = v`, `s = x` through `operator=` | `bound_call`s already (`AS_OPERATOR`, `AS_INDEX`, `AS_INDEX_STORE`, `AS_ASSIGN_OPERATOR`): 6.6 | |

`bound_increment` (`target` is storage; its parts are evaluated ONCE):

```
parts of the target -> slots            (the object of a field; the array and the index of an element)
%old = read the target                  (a property: call the getter; else as 6.2)
%new = %old + 1                         (integer: add with a constant 1; decimal: dinc; - 1 for --)
store %new into the target (narrowed)   (a property with a setter: call the setter; else as 6.2)
value: %new, or %old for the postfix form
```

`bound_assign`:

```
=           %v = value (already converted); parts of the target; store %v; value: %v (as stored)
op=         %v = value; parts of the target; %old = read; %r = %old op %v (operands as in bound_binary);
            %r = store-conversion(%r) (assign.store); store %r; value: %r
```

The right side runs BEFORE the parts of the target (6.0; measured, Q6). A
storage kind `STORE_NONE` never reaches the generator (`E0970`).

### 6.5 Null safety: `?.` and `!!`

`bound_safe_navigation receiver rest`:

```
%recv = receiver
n = is_null %recv
jump_if n, is_null_label
%d = rest                   in which every bound_receiver of this navigation is %recv
jump end
is_null_label:  %d = null   (the type is nullable)
end:
```

A chain `a?.b?.c` is navigations nested through `enclosing`; each one jumps to
its own "is null" label, all of which give null. The generator keeps, while it
generates a navigation's `rest`, which slot holds its receiver
(`generator.navigations` / `receiver_slots`); a `bound_receiver` reads that
slot.

Two shapes have no null to give. A chain that ends in a plain NUMBER field
(`next?.value`, `value : _int64`; analysis warns, W1027) is read as if `!!.`
were written: no test, and a null receiver raises nullptr_exception at the
field read - which is what the C++-built program does. A chain used as a
statement (`listener?.notify();`) generates its rest as a statement: the
result of the call, if any, is dropped (`pop`).

`bound_not_null x!!`: the value of `x`, and nothing is emitted. The C++
compiler emits no test for `!!` either (`force_non_null_expression.cpp`): a
null shows when it is used (a field access, a call through
`invoke_delegate`). Probed (Q2): `y := h!!;` with a null `h` raises nothing.
DECISION: the bootstrap does the same, because the oracle is the C++-built
program. backend.md 12.3 said "`bound_not_null`: throw `nullptr_exception`
when null"; that line now says what really happens. Making `!!` a real test is
a change of the language for the owner to decide; it would be one place here
(`movl x; checknull cmt; jne ok; <new nullptr_exception, throw>; ok:`).

### 6.6 Calls (`bound_call`, `bound_value_call`, `bound_new`)

For every call: evaluate `on` (when there is one) and each argument, in order,
each into a slot; the arguments are already converted (backend.md 12.3).

| dispatch | IR `how` | VM |
|---|---|---|
| `CALL_STATIC` | direct | push each argument; `call f` |
| `CALL_STATIC`, `f` native (`FLAG_NATIVE`) | native | push each argument; `invoke_delegate f, argc, 1` |
| `CALL_DIRECT` | direct | push `%on`, the arguments; `call f` |
| `CALL_BASE` | direct | push `%on` (the object), the arguments; `call f` (the base's own body, never looked up) |
| `CALL_CONSTRUCTOR` (`-> base(...)`) | direct | push slot 0, the arguments; `call base_constructor` |
| `CALL_VIRTUAL`, `f` has a body and `f.overridden_by` is empty | direct | as `CALL_DIRECT`: nothing below can replace it (the C++ compiler's `dispatch_target` narrows the same way) |
| `CALL_VIRTUAL` otherwise | virtual | push `%on`, the arguments; `invoke_delegate f.family_root, argc, 0` |
| `written_as` (operator, getter, setter, index, iterate, text append) | - | only for the line table and `-ir`; a call is a call |

Pushing a slot: number `ipushl n`, reference `pushl n`. After the call: when
the function returns a value and the value is used, `ipopl d` / `popl d`;
when it is not used, `pop`.

`bound_value_call callee args`: `%f = callee`; arguments; `call_value` ->
`pushl f; <push each argument>; calld argc` (argc without the hidden record:
the VM adds it, 4.6). The result as above.

`bound_new class_of constructor arguments`:

```
%o = new_object C                   newclass C; popl o
arguments -> slots
call -, constructor, [%o, args]     pushl o; <arguments>; call constructor
value: %o
```

A constructor's own code begins with its prologue (6.14).

`bound_string_template start appends`: `%t = start`; for each append `a`:
`%t = call a.function on %t with a's one argument` (a direct call; the `on` of
an append is null because it is the result before). Value: `%t`.

`bound_dictionary`: `%n = <number of pairs>`; `%array = new_array %n of
pair_class`; for each i: `%p = new pair(keys[i], values[i])` through
`pair_constructor`; `set_element %array, i, %p`; then `%d = new
dictionary_class(%array)` through `dictionary_constructor`.

`bound_get_component definition`: `call %d, <the definition's function>, [],
direct` (6.16).

### 6.7 Arrays

| node | IR | VM |
|---|---|---|
| `bound_index` read | `get_element %d, %a, %i, element` | number: `movl a; loadl adx, i; checklen adx; aloadN ebx, adx; smovr_2 ebx, d` · reference: `movl a; loadl adx, i; checklen adx; movnd adx; smovr_3 d` |
| `bound_index` store | `set_element %a, %i, %v, element` | number: `loadl ebx, v; [movN]; movl a; loadl adx, i; checklen adx; astoreN adx, ebx` · reference: `pushl v; movl a; loadl adx, i; checklen adx; movnd adx; popobj` |
| `bound_new_array` with `size` | `new_array %d, %size, element` | `loadl adx, size;` then `newarray adx, type` (number elements) / `newclassarray adx, C` (class elements) / `newobjarray adx` (`object` and function values) `; popl d` |
| `bound_new_array` with `items`, `bound_array_literal` | `new_array` of the item count, then `set_element` for each item in order | |
| `bound_sizeof` of an array | `length %d, %a` | `movl a; sizeof ebx; smovr_2 ebx, d` (0 for null) |
| `bound_sizeof` with `constant` (a text literal) | `int_constant` | |
| `bound_sizeof` of an object | `length` too: the number of its kept instance fields | |

### 6.8 `is` and casts

| node | IR | VM |
|---|---|---|
| `bound_is` against a class or interface | `is %d, %x, C` | `movl x; is ebx, C; smovr_2 ebx, d` (null gives 0) |
| `bound_is` against a number array | `is %d, %x, type` | `is ebx, <data_type>` |
| `bound_is` against an array of a class (`x is shape[]`) | `is %d, %x, shape` | as against the class: the C++ compiler ignores the `[]` (`create_is_operation`) |
| `bound_is` with `constant` | 6.1; when the operand has an effect it is evaluated first, then the answer written (G25) | |
| `bound_is` of a plain number against a number type | always the constant 1 (analysis: backend.md 12.4.2) - as in the C++-built program; a number in a register carries no type | |
| `bound_is` of a plain number against anything else (`n is string`, `n is object`), or of an object against a plain number type (`thing is _int64`) | always the constant 0 (analysis: backend.md 12.4.3; 0 in the C++-built program too) | |
| `bound_is` against `object` or `object[]` | `is_null %d, %x; not %d, %d`: "is there an object". The constant 1 when the value cannot be null (analysis). `object` is every object and every array, and the VM has no test for "an array of objects" (G29) | `movl x; checknull ebx; smovr_2 ebx, d`, then `not` |
| `bound_cast` | its `how`, as 6.3 (a numeric `as` is a `CONV_NUMERIC`) | |

### 6.9 Statements: blocks, locals, expressions

| node | IR |
|---|---|
| `bound_block` | each statement in order |
| `bound_expression_statement` | the expression; a call's unused result is dropped (`pop`); with `is_result` (a lambda of one expression) `return_value` of it |
| `bound_empty` | nothing |
| `bound_local_declaration` with `value` | `store_local %slot, %value, narrow` (captured: into the record; `localized`: the fiber local) |
| ... with `default_constructor` (`n : int;`) | `%o = new_object C; call constructor on %o`, stored |
| ... `is_injected` | the value of `injected_from` (6.16), stored |
| ... with nothing | the start value, explicitly: `int_constant 0` (integer), `decimal_constant 0` (decimal), `null` (reference) - slots are not cleared by the VM (4.4), and a declaration inside a loop starts again every round |

### 6.10 Branches and loops

`bound_if c then else`: `c' = c; jump_unless c', else_label; then; jump end;
else_label: else; end:` (no `jump end` / `else` when there is no `else_part`).

`bound_when` (subject or not): the subject (when there is one) is stored into
`subject_variable` first. Then for each clause k: each test in order -
`c = test; jump_if c, body_k`; after its tests `jump next_k`; `body_k: body;
jump end; next_k:`. After the last clause: the `else_part`; `end:`. The tests
are finished conditions (already `subject == value`, possibly a call of
`operator==`).

| loop | IR |
|---|---|
| `bound_while c body` | `top: c' = c; jump_unless c', end; body; continue_label: jump top; end:` |
| `bound_do_while body c` | `top: body; continue_label: c' = c; jump_if c', top; end:` |
| `bound_for init c step body` | `init; top: [c' = c; jump_unless c', end]; body; continue_label: [step, result dropped]; jump top; end:` |
| `bound_foreach` | below |
| `is_endless` | the condition is left out (`while(true)`: no test at all) |

`break` jumps to the loop's `end`, `continue` to its `continue_label`. In a
`do` loop that is the CONDITION; the C++ compiler sends it to the top of the
body, past the condition (10, G31).

An `else if` ladder (an `if` that is the else part of an `if`) is followed in
a loop, every rung jumping to one `end` label: a ladder of any length costs
the generator no recursion (13.3, "chains").

`bound_foreach variable collection item_conversion array_variable index_variable`:

```
array_variable = collection                 (an array, or the call of get_elements())
index_variable = 0
top:    %len = length array_variable        (0 for a null array: the loop does not run - probed, Q4)
        c = index_variable < %len
        jump_unless c, end
        %item = get_element array_variable, index_variable
        variable = item_conversion(%item)    (stored as any local: narrowed, or into the record when captured)
        body
continue_label:
        index_variable = index_variable + 1
        jump top
end:
```

(C++: the index starts at -1 and is incremented at the top; the same rounds.)

### 6.11 Jumps out, try / catch / finally, lock

A `return`, `break` or `continue` lists in `leaves`, innermost first, the
`lock` statements and the `try` statements with a `finally` it passes on its
way out (backend.md 12.3). The generator handles them with EXIT STUBS: for
each `try` with a `finally` and each way out that leaves it, one small piece
of code placed right AFTER the try statement:

```
stub(try T, exit X):   <a copy of T's finally block>
                       <the rest of the way out after T: unlock the locks between T and the target,
                        then jump to the stub of the next try-with-finally on the way, or do the final jump>
```

A jump out therefore does: unlock every `lock` before the first
try-with-finally in `leaves` (`unlock %held`), then `jump stub(that try, X)`;
with no try-with-finally in the way it unlocks them all and does the final
jump (`jump end` of the loop, `jump continue_label`, or the return). A
`return value` that passes a `finally` first stores its value in a hidden slot
(the value is computed BEFORE any finally runs, as in the C++ output), and the
final return returns that slot.

`bound_try body catches finally_part`, laid out:

```
    nop                             (R2 of 4.7: a call just before the try must not fall into its ranges)
T.start:                            (try start pc = block start pc = the next word)
    body
    jump stub(T, normal)            (no finally: jump after)
T.block_end:                        (block end pc = that jump: R1 of 4.7 - a call that ends the body is
                                     looked up at the word after it, which must still be in the range)
handler 1:                          (handler pc; the exception slot: the catch variable's slot, or -1;
    [variable captured: copy the     a captured catch variable: the VM writes a hidden slot, which is
     slot into the record]           then stored into the record)
    catch body 1
    jump stub(T, normal)
... every catch clause ...
T.end:                              (try end pc = the last jump above: again a jump, never a call)
T.finally_start:                    (finally start pc; its exception slot: a hidden REF slot E)
    <copy of the finally block>     entered only by the VM, with the exception in E
    pushl E; throw                  re-raise it: the enclosing handlers get it next
T.finally_end:
stub(T, normal):  <copy of the finally block>; jump after
stub(T, X) for every other way out X that a jump inside T uses
after:
```

`T.start`, `T.block_end`, `T.end`, `handler k`, `T.finally_start` and
`T.finally_end` are the LABELS of the try's `ir_try` (5.2), placed like any
other label; an end label stands one past the range it closes, so the range's
last word is the word before it (`try_record_of`, bytecode.sharp).

A jump out of the body or of a catch handler (`return`, `break`, `continue`)
is a jump too, so wherever the body ends, the last instruction in a range is a
`jump`, a `throw` or the closing jump - never a call. The exception slot E
never gets a number whose low byte is 5 (11.1). The C++ compiler's layout,
decoded for comparison (`with_finally` of the probe): try range = body + catch
handlers + the `pushnull; popl E` in front of the one shared finally block;
block range = body up to its closing `jmp`; so its ranges end in a `jmp` as
well - but it puts no `nop` in front, hence the bug of 4.7.

Why this is right:

- an exception inside the body is offered to the catch clauses (block range),
  then to the finally (try range); one inside a catch handler only to the
  finally (handlers are inside the try range, outside the block range); one
  inside the finally copies or the stubs goes to the ENCLOSING handlers,
  because all of them lie outside T's ranges;
- a stub of an inner try lies inside the outer try's body, so a `finally`
  copy that raises is caught by the outer try, and the stub's onward jump to
  the outer try's stub is ordinary code inside the outer range;
- no indirect jump is needed: the C++ generator stores a return address in a
  hidden local and jumps back with `brh` (`try_catch_statement.cpp`,
  `return_statement.cpp`); here every path is a direct `jmp`, which the
  optimizer and the JIT see through (10, G2). The price is a copy of the
  finally block per way out, as `javac` does.

`bound_throw value`: `%v = value; throw %v` -> `pushl v; throw`. Unless
`value` is a `new` expression the lowering is `movl v; cast <C>; pushl v;
throw`, C the value's own class or the first class above it that is kept
(`throwable` always is): `cast` raises `nullptr_exception` for a null and
passes every real exception. Probed (Q5): the C++-built program that throws a
null CRASHES the VM (SIGSEGV - `catch_exception` reads the class of the
object); an exception the program can catch is the only sane meaning (10,
G15). The test does not look at the TYPE of the value: `x!!` tests nothing
(6.5), and a local or field of a type without `?` holds null until it is
given a value - `throw e!!;` and `throw never_set;` crashed the VM until the
verifier of stage 3 made the test unconditional (13.3).

`bound_lock locked held body`:

```
held = locked            (stored into the hidden slot, so the same object is unlocked)
lock held                movl held; lock
body
unlock held              movl held; ulock         (the normal end)
```

Jumps out unlock it (above); an exception out of the body is released by the
VM (4.7, 4.8). A jump that passes locks and finally blocks unlocks each lock
at its place in the nesting, so a finally block runs with exactly the locks
held that are written around it; the C++-built program holds all of them
until the last finally block is done (10, G30).

`bound_return value`: `return_value %v` -> number: `loadl ebx, v; [movN for
the function's narrower result type]; returnval ebx; ret` · reference: `movl
v; returnobj; ret`. `return;` -> `ret`. A function that returns nothing and
whose body `completes` gets a `return` at its end; every function's code ends
with `ret` (11.2).

`broken` jumps never occur in a program without errors.

### 6.12 Lambdas, closure records, function values

**The record class.** Every `closure_record` becomes a class (no base, no
functions) with, in this order: one field per `variables` entry (its name and
type: a number field or a reference field), `@parent` (a reference, when
`parent` is set) and `@self` (when `captures_self`). Name:
`closure_<full name of the function>` (the C++ compiler's:
`main#closure_main#__srt_global.make_counter`), for a field's record
`closure_<full name of the field>`; two records that would get the same name
(overloads) get `@2`, `@3` ... appended in `analysed.functions` order.

The C++ compiler can capture only objects ("cannot capture closure of local
field `count`. Only types of (arrays, classes, & objects) are allowed", met
while probing) and its record holds references. The bootstrap's analysis
accepts a captured number too (backend.md D10); its record then has a number
field. Nothing the C++ compiler accepts behaves differently.

**The record of a function F** (F has `closure`): F's prologue makes it, once
per CALL (recursion and fibers each get their own variables):

```
record_slot = new_object <F's record class>           newclass R; popl record_slot
[@parent = F's own record parameter]                  when the record has a parent (F is a lambda)
[@self = slot 0]                                      when captures_self and F is an instance function
[for every captured parameter p: record.p = slot of p]
```

**Finding a record** at a use in function G. The records G can reach are its
own (`record_slot`) and the chain through its record parameter (the last
parameter, when `G.takes_record`): that parameter holds the record of the
function G is written in; its `@parent` the one around that, and so on. A use
at `depth` d of a captured variable reads it from: d = 0, G's own record; d >
0, the record parameter, then `@parent` followed d - 1 times. `self` at depth
d the same way, field `@self`. For a `bound_lambda.record`, the generator finds
that record object among the ones G can reach (it is G's own or one of the
chain) and walks there.

**A function value.** `bound_lambda function record` and
`bound_function_reference function`:

```
%v = new_object platform.kernel#function_value            newclass function_value; popl v
%a = int_constant <address of function>                   istorel a, <address>
set_field %v, address, %a                                 loadl ebx, a; movl v; putf ebx, <address slot>
[set_field %v, captured, <the record>]                    when record is not null
```

A lambda is an ordinary static function (`FLAG_STATIC`), with one more
parameter when `takes_record` (a reference, last). A lambda written in the
value of an instance FIELD has a record that belongs to the field
(`closure_record.field`): `init<C>` makes it (with `@self = slot 0`) just
before it evaluates that field's value.

**Calling one**: 6.6 (`calld`). `f == g` between function values is already a
call of `function_value.same` in the typed tree.

**As built (stage 4, 13.4).** The frame of a lambda that takes a record is
`self`-less: its parameters, then the record parameter (the slot right after
them, where `calld` puts it; `function_layout.parameter_count` counts it and
the function record lists its type, the class of the record it is handed),
then its other locals. A captured local takes no frame slot at all; a
captured parameter keeps the slot it arrives in, and the prologue copies it
into the record. The record is made BEFORE a constructor's base call (which
may read a captured parameter). An `init { }` block generated inside
init<C> or static_init<C> makes its own record where it starts; the record of
a field (a lambda in an instance field's value that uses `self`) is made by
init<C> right before that value. A captured `catch` variable is caught into a
slot of its own (the VM stores into a frame slot, 4.7) and moved into the
record at the start of the handler. A captured variable is never also kept in
the frame (G11), so `x = 5` in the function and `x` in its lambda are one
store and one read of the same field.

### 6.13 Generics

Nothing special: every instance is its own class in `analysed.classes`
(`list<string>`), with its own functions (`analysed.functions`) whose types are
the real ones. A blueprint is never generated. Names: `std#list<std#string>`,
the arguments as the C++ compiler writes them (`_int32`, `std#int`, nested
instances in full), separated by `, ` - only reflection and stack traces see
them.

### 6.14 Objects: constructors, `init<C>`, `init { }`, `init(x)`

The code of a constructor of class C, and of an `init(x)` conversion function
(C++: `init_compiler.cpp`; backend.md 7.15.2):

```
1. the base constructor: call bound_base_call on slot 0          (nothing for a class with nothing above)
2. if C has a static part:                                       movg C; getf cmt, <@static_init_flag>;
       if @static_init_flag == 0: call static_init<C>            je skip; call static_init<C>; skip:
3. if C has instance initializers: call init<C> on slot 0
4. the body
```

`init<C>()` (an instance function, kind "initializer"): for each entry of
`C.instance_initializers`, in order - a field with `bound_initializer`: store
its value into the field of slot 0; a field with `default_constructor`: a new
object made with it; a field `injected_from`: the definition's value (6.16); an
`init { }` block: its statements, generated in place (they form one function,
as in the C++ compiler). Fields with none of these keep 0 / null from
`newclass`.

### 6.15 Static initialisation and the start-up of a program

`static_init<C>()` (static, kind "initializer"), for every class and every
module global class with a static part:

```
if @static_init_flag != 0: return
@static_init_flag = 1
for each entry of static_initializers (a module: global_initializers), in order:
    an enum value:   new std#_enum_(name, number) - `newstring` the name, the number as a decimal -
                     stored into the value's static field (the number: sharp_enum_value.constant,
                     else bound_value computed now, else a number the generator picks, 10 G7)
    a static field:  its value
    static init { }: its statements, in place
```

The generator writes the bodies of the two empty functions of
`platform.kernel#platform` (backend.md 12.4; `E0961` guarantees they are
empty):

```
platform.static_init():
    1. call static_init<E> for every kept enum E                              (initialisation order)
    2. platform.<main | main2 | main3 | main4> = a function value of `main`   (by analysed.main_form)
    3. call static_init<C> for every other kept class C with a static part,
       module global classes included                                         (initialisation order)

platform.tls_init():
    every kept localized field's value, classes in initialisation order, fields in declaration order
```

Only KEPT fields get their value (5.5): an entry of `static_initializers`,
`global_initializers`, `instance_initializers` or `fiber_initializers` whose
field is not kept is skipped, value and all. `init { }` blocks always run.
The order INSIDE each of those lists is analysis's (backend.md D70: a value
after the values it reads directly, the written order otherwise) and is
followed as it stands.

`srt_init` (library code) then calls `static_init()`, `tls_init()` and the
`main` field.

**The order (Q1, decided).** What the C++ compiler does, read and probed:
every enum's values are made first, inline in `platform.static_init`; then the
`main` value; then `static_init<C>` for each class in class ADDRESS order, a
module's `__srt_global` among them. A static value that reads a static of a
class later in that order reads 0 or null - nothing initialises on demand
except a constructor (6.14). Its addresses come from its list of source files,
which it sorts BY FILE SIZE (`main.cpp`, `isFileLarger`), then by the order
the generator happens to ask for them. That cannot be copied and should not
be: adding a comment to a file would change which static runs first.

The bootstrap keeps the three steps, but orders the classes by a rule a reader
can predict - the INITIALISATION ORDER, a list `class_layout.sharp` makes:

1. modules in DEPENDENCY order: a module comes after every module its files
   import. It is a depth-first search over the imports that STARTS at
   `platform.kernel` (program_order.sharp), so the modules the kernel imports
   come before it and the kernel before everything else. Modules that import
   each other (the `std` modules do), and modules with no import between
   them, keep the order of the files on the command line;
2. inside a module: its `__srt_global` class first (so a module's globals have
   their values before the statics of its classes), then its classes in the
   order they are declared (file by file in `analysed` order, top to bottom),
   a nested class right after the class around it, a generic instance where
   its blueprint is declared (instances in `analysed.classes` order);
3. closure classes last (they have no static part; they are in the list
   because class addresses are given from it).

Class ADDRESSES follow the same list with one change: every `__srt_global`
class is moved to the front, in list order (11.6 needs the addresses 0 - 9 to
belong to classes that no object is an instance of).

So the statics of an imported module are ready before the importing module's
run - the property a program can reasonably lean on, and the one the C++
order gave the probe by luck (module `aaa`, then `zzz` which imports it, then
`main`, whatever the order on the command line). Where a suite case or the
library turns out to lean on something else, stage 5 finds it; the fix is
then in the program's order of declarations or, if the owner prefers, a rule
added here. This is difference G13.

### 6.16 Components

For each kept `type_definition`: a `single` gets a static field, a flag and a
function in its module's global class (5.6) -

```
@sub_component_init_<n>() : <provides>
    if @sub_component_init_flag_<n> == 0:
        @sub_component_<n> = <definition.value>
        @sub_component_init_flag_<n> = 1
    return @sub_component_<n>
```

- a `factory` a function that returns `<definition.value>` each time. A
`bound_get_component`, an injected field and an injected local are calls of
that function (C++: `component_compiler.cpp` does the same for `single`, and
inlines a factory's expression at every use; one function is simpler and
behaves the same).

Which definitions are KEPT (stage 4): analysis records on every definition the
declarations whose code receives its value - a `get` or `inject` in them, or
in the value of a definition they receive (`type_definition.requested_by`,
program/usage `keep_uses`); a definition is kept when one of them is. Its
value's own uses are already among theirs (backend.md, usage), so everything
the value needs is kept too. `n` counts the kept definitions in the order the
components are written; the functions are the last methods of their module's
`__srt_global` class. A definition nothing kept asks for is not written, and
its value never runs - the same in the C++-built program
(cases/components.sharp).

### 6.17 `asm` blocks, natives, fiber locals, obfuscation, debug information

- **`asm`**: each `bound_asm_instruction` becomes one `IR asm` instruction and
  then exactly the VM instruction it names, with its operands: a register by
  number, a number as is, a text as a string-table index, `@label` the pc of
  the block's label (plus its offset, `@label+2`), `[local]` the variable's
  slot (a local that has none - captured by a lambda, or `localized` - is
  refused by analysis: `E0925`, backend.md 7.21), `[field]` / `$C[field]` the field's slot, `$C[function]` /
  `$m#__srt_global[name]` the function's address, `$C` the class's address.
  The operand order per mnemonic is the C++ compiler's
  (`asm_statement.cpp`). A label is an `IR label`.
- **natives**: a `native` function has no code (kind 7, flag native, code size
  0); calls use `invoke_delegate f, argc, 1` (6.6). The C++ compiler also
  writes C++ glue files for natives with `-nativedir` (`native_generator.cpp`);
  that is part of the driver (section 8.2).
- **fiber locals**: every `localized` field and local gets the next fiber-local
  slot (fields first, then the locals of every kept function); the count goes
  into the manifest; access as in 6.1 and 6.2. A field's value is given in
  `platform.tls_init`, once per new fiber; a LOCAL's where it is declared,
  every time the declaration runs, as for any local (the C++ compiler does the
  same: `local_variable_decl.cpp` gives it a fiber-local slot of its own).
- **obfuscation** (`-obf`, `obfuscate { }` blocks: `sharp_program.obfuscation_rules`):
  the names written into the file - class name and full name, field name and
  full name, function name and full name, the module part of full names - are
  replaced by generated ones (`a`, `b`, ... `z`, `aa` ..., C++:
  `generation/obfuscation/obfuscator.cpp`), numbered in a fixed order so two
  compiles give the same file (10, G9). A required class (3.11) or a field
  the VM reads by name renamed this way is `E0971`. The C++ obfuscator also
  replaces the FILE names of the meta section (stack traces then show the
  generated name) and module names, and honours the `keep` forms of an
  `obfuscate` block (a class kept "inclusive" keeps every member's name, one
  obfuscated "inclusive" renames them all: `should_obfuscate`); the bootstrap
  does all three. Text in
  the string table is never changed.

  As built (stage 4, package obfuscation/): without `-obf` and without a rule
  that renames (`obfuscate { }`, `-inclusive`), nothing changes - the
  library's own blocks only keep. Otherwise each class, field, function and
  module gets a modifier: the default (`-obf`: a class `inclusive`,
  everything else `obfuscate`; else none), then every rule naming it in the
  order written, the last one winning (an inclusive rule on a class reaches
  its fields and functions, and on a generic class as written every instance
  of it). Renamed are: a class told to be; a member when its class is not
  kept inclusive, the member is not told to keep its name (G35) and either it
  or its class says rename; a module, in the full names of its renamed
  classes; with `-obf` the source files. Never renamed: the classes the VM
  looks up by name (3.11) with every field and the classes above them, and
  every native function with its class; a rule asking for one is `E0971`.
  Enum values keep their names. The new names come from one counter in a
  fixed order - files, classes by address each with its own fields,
  functions by address (G9, G36) - and `<output>.names` lists what each
  stands for.
- **debug information**: the line table (a `line` before every statement, and
  before every call that starts on a later line than its statement); each
  function's source file; with `-dbg` the source text in the meta section.

---

## 7. The optimizer

### 7.1 What the C++ optimizer does

`lib/grammar/backend/optimizer` works on finished BYTECODE: it decodes each
function into `function_ir`, checks that encoding it again gives the same words,
runs passes until nothing changes (at most 16 rounds), and restores the
function if anything looks wrong. `-O` (any of `-O1 .. -O3`) turns everything
on; `-n<name>` leaves one pass out; `-Oreport` prints what each did.

| pass | what it does | level |
|---|---|---|
| stack forwarding | a push followed by a pop becomes a move, or nothing | 1 |
| copy propagation | register-to-register moves folded into their neighbours | 1 |
| object copies | a reference copied into another slot only to be read: the original is read | 1 |
| constant propagation | values known while compiling are computed; branches on known conditions decided | 2 |
| dead stores | computations and stores whose value is never read are removed | 1 |
| dead code | whatever contributes to nothing observable: variables that feed only themselves, ifs that guard nothing, loops whose results are unused | 2 |
| jump cleanup | jumps to the next instruction, jumps to jumps | 1 |
| unreachable code | code nothing can reach | 1 |
| loop closed form | a counting loop that only sums is replaced by the formula | loops |
| loop unrolling | a loop with a small known trip count becomes straight code | 3 |
| loop rotation | the test moves to the bottom: one jump per round instead of two | loops |
| loop registers | the number locals of a loop live in registers while it runs | loops |
| inline calls | small callees are copied into their callers | 3 |
| box elimination | number objects (`int`) that never leave the function become plain numbers | 3 |
| native returns | a call whose number-object result is only unwrapped goes to a hidden version that returns the number | 3 |

Its analyses (`analysis/`): control flow, liveness, loops, post-dominators,
known values, and a summary of the box classes' constructors.

### 7.2 What the bootstrap does (built in stage 6, 13.6)

The bootstrap optimizes its IR (5.2), where the structure is still plain -
slots with a kind, labels, calls as calls - instead of decoding finished
bytecode as the C++ optimizer does. **No pass is needed for correctness**:
without `-O` nothing here runs and the code is what the generator wrote.
Every pass is checked by the suite at `-O` (JIT and `-nojit`, through its own
runner), by every codegen case built with `-O`, by the whole library built
with `-eager -O` and run, and by the verifier (5.7) on every function after it.

**The switch is the C++ compiler's** (lib/grammar/main.cpp): `-O` - and
`-O1`, `-O2`, `-O3`, which mean the same - turns every optimization on; `-O0`
turns it off again; it is off by default; `-n<name>` leaves one out, with the
C++ compiler's names; `-Oreport` prints what each pass did and how long it
took. (The plan above had `-O` and `-O2` as two levels; the C++ compiler has
ONE switch, and the task said "exactly as in the C++ compiler". `-R` does not
turn it on: the C++ `-R` only takes the debug flag out.)

The order, as the C++ driver's (`bytecode_optimizer.cpp`):

```
0. string concatenation flattening, on the code as generated
1. every function alone: the local passes, again and again until none changes anything (at most 8 rounds)
2. across functions, up to 3 rounds: devirtualization, closure elimination, inlining; then native returns
3. pure function detection and escape analysis (facts for what follows); unused pure calls and dead objects go
4. common subexpressions; null tests whose answer is known
5. loops: invariants moved out (3 rounds), bounds checks, unrolling, closed form, rotation (+ jump cleanup)
6. slot packing: the frame of every function made small (after every pass that changes code)
7. the register hand-off (last: it describes the final order of the instructions)
(after each of 2 - 5 the local passes run again on the functions that changed)
```

Steps 0, 3, 4 and most of 2 and 5 have no counterpart in the C++ optimizer: they
are the optimizations approved on 15 Nov 2026 (doc/optimizations.md 3a), built
on what this compiler knows that a bytecode optimizer cannot - every override in
the program, the function each call goes to, the kind of every slot.

| pass (-n name) | file (backend/optimizer/...) | what it does | C++ pass |
|---|---|---|---|
| copy-propagation, object-copies | passes/values/copy_propagation.sharp | after `copy %a, %b`, reads of `%a` read `%b` ("available copies", a forward data-flow problem); numbers and references are its two halves, switched by the two C++ names | copy propagation, object copies |
| box-elimination | passes/calls/box_elimination.sharp, object_paths.sharp | an object whose slot is written once, by `new_object`, and read only as the object of its own field reads and writes becomes one slot per field ("scalar replacement", the simplest escape analysis). A slot that is reused for other values is split first: when the object provably stays in the function and its field reads and writes can see nothing else in the slot (reaching definitions), the object moves to a slot of its own | box elimination |
| constant-propagation | passes/values/constant_propagation.sharp, folding.sharp | values known while compiling are computed exactly as the VM computes them; `jump_if` on a known value becomes `jump` or goes | constant propagation |
| (part of constant-propagation) | passes/values/branch_threading.sharp | a jump into a block that only tests a slot known at the jump goes past the test (what `&&` / `\|\|` leave) | (in its constant propagation) |
| immediates | passes/values/immediates.sharp | `x + k`, `x - k`, `x * k` with a known 32-bit `k` become `int_immediate` (selected as `iaddl n, k` when the target is the operand: `i++` is one VM instruction); with `k` 0 or 1 the identities: `x + 0`, `x - 0`, `x * 1` are a copy of `x`, `x * 0` is 0 | (in its constant propagation) |
| copy-coalescing | passes/values/copy_coalescing.sharp | a value made only to be copied (`add %5, ...; copy %1, %5`) is made in the copy's target | - (the bootstrap's own) |
| dead-stores | passes/removal/dead_stores.sharp | a value written into a slot nobody reads again (liveness) is not made; a call's unused result is dropped | dead stores |
| dead-code | passes/removal/dead_code.sharp | "mark and sweep": values that only feed themselves are not computed | dead code (without its removal of whole loops) |
| jump-cleanup | passes/control/jump_threading.sharp | jumps to jumps, jumps to the next instruction, a test over a jump turned round, a jump to a `return` becomes the return, unused labels go | jump cleanup |
| unreachable-code | passes/removal/unreachable_code.sharp | blocks nothing can reach (handlers count as reached) | unreachable code |
| inline-calls | passes/calls/inline_calls.sharp | a direct call of a function of at most 14 IR instructions (about the C++ pass's 24 VM instructions), no loop, no `try`, `throw`, `lock` or `asm`, is replaced by its body; every place shares the same extra slots | inline calls |
| loop-unrolling | passes/control/loop_unrolling.sharp (analysis/loops.sharp) | a loop that is certain to run a small, known number of times (at most 16 rounds, 96 instructions) is RUN while compiling, with constant propagation's rules; the instructions passed on that run, without the jumps, replace it. Left alone: a jump that depends on an unknown value, a loop left by `return` or `throw`, a function with a `try` | loop unrolling (the same method and limits) |
| loop-rotation | passes/control/loop_rotation.sharp | a loop's test moves to its bottom: one jump per round instead of two | loop rotation |
| slot-packing | passes/registers/slot_packing.sharp | slots that are never alive at the same time get one number ("graph colouring" by liveness, the greedy way; only slots of the same kind share; `self` and the parameters stay). The frame of a function shrinks to what its values need at once: frame size is recursion depth on this VM (11.8) | - (the C++ generator keeps values on the way in registers) |
| register-hand-off | passes/registers/register_hand_off.sharp + bytecode/bytecode.sharp | a value read by the next instruction and by nothing after stays in its register: the store and the load both go | - (closest: stack forwarding) |
| stack-forwarding | - | not here: the IR has no operand stack to forward (the register hand-off is its counterpart) | stack forwarding |
| native-returns | passes/calls/native_returns.sharp, analysis/object_paths.sharp | a function whose every `return` gives an object made in it by `new_object` that goes nowhere else gets a TWIN, `name$field`, an ordinary function of the executable that returns the number of that field; a direct call whose result is only read for that one field goes to the twin. Run once, after inlining; the twins and the rewritten callers get the local passes again (in the twin box elimination then removes the object). Not for functions with a `try`, lambdas and closures | native returns |
| string-concatenation | passes/calls/string_concatenation.sharp | in `a + b + c` (and a string template) every `string.operator+` whose left side is a temporary of the same expression goes to a twin, `operator+$append`, cut from the code of that overload, which appends to its own object instead of to a copy of it: one string and one copy however long the chain. Runs first, on the code as generated | - |
| devirtualization | passes/calls/devirtualization.sharp | a virtual call (`invoke_delegate`, a search through the class's methods at run time) becomes a `call` when the one body that can run is known: only one kept class implements the function, or the object was made here and its class is known. Where the object may be null the direct call sits behind a null test with the virtual call as the other branch, so null raises exactly as before | - |
| closure-elimination | passes/calls/closure_calls.sharp | `call_value` of a function value that can only be the one made in this function becomes a `call` of the lambda's code with the captured record as its last argument; inlining then copies the body in, and box elimination removes the function value and the record | - |
| pure-functions | passes/calls/pure_functions.sharp | for every function: does it change anything, read anything, can it raise, does it come back (worked out together for the whole program; `sharp_declaration.optimizer_facts`). A call of a function that does none of it and whose result is unused is removed by the removal passes; the passes below ask the facts | - |
| escape-analysis | passes/calls/escape_analysis.sharp, passes/removal/dead_objects.sharp | for every function, which arguments do not escape it (`optimizer_kept_in`); with the facts above, an object that is made, filled (also by its constructor, when that is too big to inline) and never read is not made | - (the C++ pass removes dead number boxes only) |
| common-subexpressions | passes/values/common_subexpressions.sharp, analysis/expressions.sharp | "available expressions" over the whole function: an expression (arithmetic, a test, a field / static / element read, a call of a function that changes nothing and gives a number) computed on every way to a point, with nothing in between that could change it, is not computed again there | - |
| null-checks | passes/values/null_checks.sharp, analysis/not_null.sharp | `is_null` of a slot known not to hold null (just made, used before without raising, tested on the way) is 0; constant propagation then removes the branch | - |
| loop-invariants | passes/control/loop_invariants.sharp | an expression no instruction of the loop can change is computed once in front of the loop (innermost loop first, three rounds). One that may raise moves only when the loop begins with it, or its object is known not to be null | - |
| bounds-checks | passes/control/bounds_checks.sharp | `a[i]` reached from the passed test `i < sizeof(a)` of a loop whose `i` starts at 0 or more and only counts up is marked `in_range`: the bytecode step writes no `checklen` for it | - |
| loop-formula | passes/control/loop_closed_form.sharp | a counting loop (`i < n`, `i++`) whose body is straight whole-number arithmetic and whose every written slot is the counter, an accumulator `s += f(i)` (f up to `i * i`) or dead afterwards is replaced by the sums' formulas. Exact for every `n`: 64-bit arithmetic that wraps gives the loop's result, and the two divisions are done so that they wrap rightly too | loop closed form (exact below 2^52 only, with the loop kept as the fallback) |
| loop-registers | - | not built | loop registers |

The names of the passes that are not here are accepted by `-n<name>`, so a
command line written for the C++ compiler works with this one.

**New analyses for them** (backend/optimizer/analysis): `object_paths.sharp` -
reaching definitions and a small escape walk for ONE value, asked on demand
(box elimination, native returns, devirtualization, bounds checks);
`expressions.sharp` - the expressions of a function numbered, and what ends
each (a write of a slot it reads; for a read of memory, a write that may
alias it); `not_null.sharp` - which slots cannot hold null at each point, a
forward data-flow problem kept per edge so that the two sides of `x != null`
know different things. None of them answers for a function with a `try`:
handlers are entered from the middle of blocks, and the passes that ask leave
such functions as they are.

**The analyses** (backend/optimizer/analysis): the control-flow graph with the
edges a `try` adds, liveness (bit sets), the loops (dominators, back edges and
the natural loop of each: what unrolling asks; rotation still looks for the
generator's shape in the IR itself), which labels a `try` needs where they
are, and the facts of each IR instruction (what it reads and writes, whether
it may raise, whether it has an effect). The values known at each point - the
C++ optimizer's `known_values` - are not an analysis of their own here: they
are the state of constant propagation (`constant_state`, `evaluate`), which
loop unrolling uses too. Three rules about exceptions hold in
every pass: an instruction inside a `try` that may raise can go to a handler
with every slot as it was BEFORE it (liveness adds the handlers' live slots
there); a handler is entered from the middle of blocks, so the forward passes
know nothing at its start; and the labels of a `try`, the instruction that
closes one of its ranges (R1) and the `nop` before it (R2) are never removed.

**Added by the verifier of stage 7 (13.7): slot packing.** The passes above
only ever ADD slots - the slots of inlined bodies, one per field of a removed
box - and leave behind the ones they make unnecessary, so a function's frame
after `-O` was about 2.5 times the frame it was generated with
(`analyse_statement` of the compiler itself: 14 slots plain, 34 with `-O`).
On this VM that is recursion depth lost (11.8): the compiler built with `-O`
ran out of stack on code nested 28 blocks deep where the unoptimized one
managed 70, and a program that recursed 450 deep ran plain and overflowed
with `-O` (tests/codegen/cases/optimizer_frames.sharp). The pass renumbers
the slots of each function after all other passes: two slots INTERFERE when
one is written while the other is live, slots that do not interfere and have
the same kind share a number, taken greedily from 0 up. `self` and the
parameters keep their places and count as written on entry; the slot the VM
writes on entering a `catch` or `finally` counts as written at the top of the
handler (and a finally's slot still never ends in the byte 05: 11.1); a slot
nothing uses gets no number. After it `analyse_statement` has 10 slots, and
frames with `-O` are as small as or smaller than without. It costs 0.3 s of
the 14 s the optimizer takes on the compiler itself.

**Added in stage 7 (13.7): the static part of a class is never inlined.**
Every constructor begins with "flag set? else call `static_init<C>`". With
`static_init<C>` copied into it, the constructor of a number box (`int`: one
field store) grew from 4 instructions to 15, was itself too large to inline,
and so every `new int(n)` - which every `list.size()` makes - stayed a call
whose object box elimination could not remove. `static_init<C>` now stays a
call (it runs once per class in a whole run), the box constructors stay small,
and `i < items.size()` compiles to a field read and a compare, with no object
(passes/calls/inline_calls.sharp, `can_be_inlined`).

### 7.3 Slots in registers

Not built as planned. The plan was linear-scan register allocation over the
free registers. What the VM leaves for it is small: the selection uses `ebx`,
`ecx`, `egx`, `adx`, `cmt` and `bmr` itself, and of the four left (`cx`,
`ecf`, `edf`, `ehf`) only `cx` is one of the JIT's six "homes" in CPU
registers - the other three live in memory in machine code, where a slot is
just as fast. Built instead: the **register hand-off** (7.2), which keeps a
value in the register it is already in when only the next instruction needs
it - the commonest case by far (a comparison feeding its jump, a value
feeding the next operation). `-Oreport` shows how many values were handed on.
And **slot packing** (7.2), which does for the frame what register allocation
does for registers: the same liveness, the same colouring, with frame slots as
the colours. It does not make a value faster to reach, but it makes the frame
small, and that is what this VM is short of.

**Since 2026-11-20 the slots do live in registers - in machine code.** The
slot forms (4.10) took the registers out of the picture for the compiler: an
IR instruction works on its slots directly. The x86-64 JIT then gives the
slots a function uses most a CPU register each (four whole numbers, four
decimals; `lib/runtime/jit/x64/x64_generator.cpp`, "LOCAL NUMBERS IN CPU
REGISTERS"; the 64-bit ARM JIT does the same for four). Every write still
goes to memory as well, so the interpreter, the collector and exceptions see
the frame exactly as before - which is why this could be done in the JIT
alone, without the compiler knowing.

---

## 8. Self-hosting

### 8.1 The steps

```
stage 1:  the C++ compiler  compiles the bootstrap sources + the library  ->  sharpc_boot     (bootc.sh, today)
stage 2:  sharpc_boot       compiles the same sources                     ->  sharpc_stage2   (on the VM)
stage 3:  sharpc_stage2     compiles the same sources                     ->  sharpc_stage3
check:    sharpc_stage2 and sharpc_stage3 are byte-identical files
          sharpc_stage2 passes the whole gates as the compiler (the suite, the backend tests, the codegen tests)
```

Stage 2 and stage 3 are made by two compilers built from the SAME source; one
was made by the C++ compiler, the other by the bootstrap itself. If the
bootstrap compiles correctly, both behave the same, so their outputs for the
same input must be the same bytes. A difference means a compiled program
behaves differently from the source - a code generation bug - or the output
depends on something it should not.

That last part is a requirement on the generator: **the output must be
deterministic**.

- no random numbers: the application id is computed from the program (a hash
  of the output name and the number of functions), guids are addresses
  (10, G6);
- nothing in the output may depend on the order in which parallel work
  finishes: `parse_all` runs coroutines, so every list is ordered by the input
  (file position), never by completion;
- `hash_map` iteration order must not reach the output: layout orders every
  table by the analysed program's lists (modules, classes, functions in
  declaration order), never by iterating a hash map;
- paths written into the file (the meta section) are the absolute paths given
  on the command line; stage 2 and 3 are run with the same paths.

**As built (13.7).** All of the above holds and is checked by
tests/selfhost.sh, step 13 of the gates. One requirement of the list was not
met when the stage began and is the only thing that had to be fixed for the
fixed point: `parse_all` handed the files on in the order the coroutines
FINISHED, and the order of the files decides the order of a module's globals
and with it the numbers of everything after them. The parsed files are now put
back into the order they were given (`in_the_order_given`, parsing.sharp).

### 8.2 The driver, option by option

The command line is read in `configuration/command_line.sharp` into `options`
(configuration/options.sharp); `main.sharp` then does what `options` says. Every option of
the C++ compiler's `main.cpp` is accepted; the "stage" column says when it was
made to work. An option that is accepted but not acted on yet says so once in
a warning (`not_supported_yet`, configuration/configuration.sharp): today there is none (`-nativedir` became `-sni`, doc/sni.md).

| option | C++ meaning | bootstrap now | stage |
|---|---|---|---|
| `-o <file>` | the output file | done (stage 1): the file, and the application name in the manifest. A file being compiled is refused as the output (13.8: it was overwritten, as the C++ compiler does); in a project, settings.json's "output" wins as in the C++ compiler, and an `-o` it overrules is a warning | 1 |
| `-c` | analyse only, write nothing | done (stage 1) | 1 |
| `-v <version>` | the manifest's version | done (stage 1) | 1 |
| `-target <n>` | the manifest's target | done (stage 1); without it 2, as the C++ compiler writes | 1 |
| `-d` | debug mode of the compiler (the C++ compiler writes the `.aout` listing on every compile) | done (stage 1): `-d` and `-listing` write `<output>.listing` | 1 |
| `-dbg`, `-debug` | debug flag + source text in the file | done (stage 1): the manifest's flag and each file's text in the meta section; stack traces print the lines | 1 |
| `-R`, `-release` | no debug | done (stage 1): takes `-dbg` out again; the file is without source text unless `-dbg` is given, as in the C++ compiler | 1 (its optimizer half: 6) |
| `-obf` | obfuscate the program's own names | done (stage 4): 6.17; the map of new names in `<output>.names` | 4 |
| `-O0`, `-O`, `-O1`..`-O3`, `-n<opt>`, `-Oreport` | the optimizer | done (stage 6): as in the C++ compiler - one switch, every optimization; `-O0` off; `-n<name>` with the C++ names (an unknown name is refused, listing the names); `-Oreport` with the time of each pass (7.2, 13.6) | 6 |
| `-L <dir>` | a library folder: every `.sharp` file in it and below | done (stage 5): every `.sharp` file below the folder, at any depth, as absolute paths, sorted (`sharp_files_below`, source_folders.sharp: one call of the runtime, `file_natives.tree_files`, the same directory walk the C++ compiler makes - names that start with a dot are skipped, files and whole folders); a file also named on the command line is compiled once; the suite runner and the benchmark script build with it; tests/driver.sh | 5 |
| `-p`, `-P <path>`, `--new-project`, `--create-project` | project mode: sources of a folder, the settings file, the output to `build/outputs/<name>` (the C++ writer makes the folder) | done (stage 7): every `.sharp` file below the folder; `settings.json` read after the command line (its settings are listed at the top of settings_processor.sharp; the C++ names `output`, `debug`, `release`, `extern_lib`, `target_api` are read, and this compiler's earlier names as well); `-L` works in a project too; the executable goes to `<project>/build/outputs/<output>` (the folder is made); `--new-project` writes a settings.json that is JSON (the names were not quoted before) and, as in the C++ compiler, the compile goes on when files or `-p` were given as well; project.sharp, tests/driver.sh | 7 |
| (no `-o`) | the C++ compiler writes `out` | done (stage 7): `out`, in the current folder; `-c` writes nothing. (The test scripts that only analyse now pass `-c`.) | 7 |
| (bootstrap only) `-ir`, `-listing` | inspection options of code generation (5.2) | done (stage 1); the test options `-entry` and `-stubs` of stages 1 - 3 were removed in stage 4 (5.8). `-ir` (and `-analysis`) show the last file OF THE PROGRAM - the last named, or the project's - not the last library file `-L` added after it (13.8) | 1 |
| `-nativedir`, `-nd <dir>` | where native glue is written | replaced by `-sni <folder>`: `sni.h` and, per native library, `<library>_sni.h` and `<library>_sni.cpp` (doc/sni.md) |
| `-j <n>` | compiler threads | done (stage 5): the parallelism of the worker pool `parse_all` runs on (`dispatchers.set_parallelism`); a number below 1 or not a number is an error. `-g` still halves the threads (`throttle_max_threads`) | 5 |
| `-cache <dir>`, `-cacheoff`, `--clean` | the parse cache: its folder, off (the C++ cache holds syntax trees for every compile on the machine, in `<home>/.sharpc/cache`) | done (stage 7): the bootstrap's cache belongs to a project (`<project>/build/cache`: which files changed since the last build; cache/cache_controller.sharp says why it does not store syntax trees). `-cacheoff` switches it off (as `"cache_files": false`), `-cache <folder>` keeps it in that folder - and then it is also used without a project -, `--clean` forgets it (only the cache's own `files` folder is deleted, never the folder `-cache` named) | 7 |
| `-eager` | compile unused library bodies too | done (stage 1): code generation keeps every function and class analysis has (the gates' library check uses it) | 1 |
| `-magic`, `-g`, `-green` | not about the output | accepted: `-g` halves the threads, `-magic` does nothing in either compiler | 7 |
| `-w`, `-w<kind>`, `-werror`, `-errlmt`, `-V`, `-showversion`, `--h`, `-?`, `--hw`, `-ignore`, `-ignoredir`, `-timings` | messages and listing | done (`-errlmt` takes a whole number from 1 to 100000 - the C++ compiler refuses one below 10 - and limits the syntax errors as well as the later ones, 13.8) | - |
| `-a` | "aggressive" error reporting: also the errors its error manager filters out as follow-ons | accepted, and changes nothing: this compiler has no such filter - one mistake is one message, and every message is shown (up to `-errlmt`) | - |
| (bootstrap only) `-run` | run the program after compiling | done (stage 7): after a compile without errors `sharp <output>` is started (the runtime is looked up on the PATH), its output follows the compiler's and its exit code becomes the compiler's - what `main` returned, the n of `exit(n)`, 1 after an exception nobody caught (13.8: the runtime gave 0 for all of these but `exit(n)`); when `sharp` is not on the PATH that is said, with exit code 1 and the command to run by hand (project.sharp, `run_program`); with `-c` it is a warning (nothing is written) | 7 |
| (the exit code) | 1 when an option is refused; 0 after a compile, with errors or without | done (stage 7): 0 when the program compiled, 1 when the command line was refused, a file is missing, or the program has errors (G39: deliberate - a build script must be able to stop); with `-run`, what the program ended with. (What `main` returns is not the exit code of the process on this VM: the driver calls `exit(code)`.) | 7 |

A value-taking option checks its value (`value_after`, configuration/command_line.sharp, 13.8):
a missing, EMPTY or option-looking value (`-o ""`, `-o -c`, `-v -c`) is refused with a
message that names the option; the C++ compiler takes `-c` as the value and loses it.

Smaller things of stage 7: `-target` also takes the C++ name `alpha-cr4`;
`-R` no longer pretends to switch the optimizer on (it only takes the source
text out, as in the C++ compiler; `-O` optimizes); the standard library
installed on the machine (`/usr/include/sharp/<version>`, the version from
`"target_api"`) is added to the sources when it is there, as the C++ compiler
does - a machine without it passes the library with `-L`; an unknown
`"target_api"` is an error that lists the versions.

The suite runner calls its compiler as `<sharpc> <case> testing.sharp -o <exe>
-L <library>` (plus `-O`).

### 8.3 The performance budget

Measured on the gates' machine, the bootstrap (built by the C++ compiler, no
`-O`) analyses the whole library in about 2.1 s (`time:` line: bodies 0.9 s,
members 0.2 s, flow 0.2 s ...). The bootstrap's own sources are about 47,000
lines in 313 files, the library about 52,000.

- Code generation for a program the size of the library (everything from
  layout to the written file) takes at most as long as its analysis, about
  2 s; no step may be worse than linear in the size of the program (labels are
  fixed up in two linear passes, tables are hash maps, slots are reused from a
  free list).
- Stage 2 compiling the bootstrap (sources + library, about 100,000 lines)
  finishes in under 20 s on the VM.
- A bootstrap built by itself with `-O` is not slower than the one the C++
  compiler built (stage 1, no `-O`) on that same compile; without `-O` it is at
  most 1.5 times slower.
- `-O` adds at most half of the code generation time.

Each stage reports its numbers in its section; the `time:` line shows the new
entries. **Measured in stage 7 (13.7): every line of this budget holds** -
stage 2 compiles the bootstrap in 7.1 s; the compiler built by itself with
`-O` does it in 4.6 s against 7.2 s for stage 1; without `-O` it takes 7.1 s
against 7.2 s. Only the cost of `-O` itself is over (13.6).

---

## 9. Feature inventory

Every construct and every option that code generation must handle, with the
stage that does it. The final completeness check of stage 7 goes down this
list.

### 9.1 The file and the program

| feature | stage |
|---|---|
| header, manifest (all 13 entries) | 1 |
| class records: classes, interfaces, enums, generic instances, module global classes, synthesized classes | 1 |
| field records: instance slots after the base's, own static slots, fiber-local slots; flags translated; number slots | 1 |
| method and interface lists (base first) | 1 |
| function records: kinds, frame numbers, delegate address, parameters, line table, try table | 1 (try table: 3) |
| delegate (body-less) and native function records without code | 1 |
| function-type table | 1 |
| string table, constant table (literal text and hex form) | 1 |
| meta section: files; source text with `-dbg` | 1 |
| what is kept (reached + implementations + synthesized) | 1 |
| names as the VM and reflection read them, generic instance names | 1 |
| deterministic output (ids, order) | 1 |
| the verifier | 1 |
| `-listing` / `-d`, `-ir` | 1 |
| `-entry` (tests) | 1; removed in 4 |
| the loader traps avoided (section 11) | 1, 3 |
| class addresses and the initialisation order (6.15); `__srt_global` classes at the lowest addresses (11.6); `platform.tls_init` at function address 0 (11.5) | 1 |
| top-level classes owned by their module's `__srt_global` class; localized fields in the static part of the field list | 1 |
| function types inside function types listed (3.5) | 1 |
| stubs and the `-stubs` report (5.8) | 1; removed in 4 |
| obfuscated file and module names, `keep` rules (6.17) | 4 |
| default output name `out` when `-o` is missing | 5 |
| `platform.static_init`, `platform.tls_init`, the `main` function value; `main_form` | 4 |
| `required_classes` kept with every field; the VM's field names kept under obfuscation (E0971) | 1; 4 |
| obfuscation (`-obf`, `obfuscate { }`) | 4 |

### 9.2 Expressions (typed-tree nodes)

| node | stage |
|---|---|
| `bound_literal` (integer, wide integer, decimal, char, bool, text) | 1 (text and wide: 2) |
| `bound_null` | 2 |
| `bound_local` (own) | 1 |
| `bound_local` captured / `localized` | 4 |
| `bound_field` instance, static, global | 2 |
| `bound_field` `localized` | 4 |
| `bound_enum_value` | 2 (the values made: 4) |
| `bound_self` depth 0, `bound_base` | 2 |
| `bound_self` / `bound_base` depth > 0 | 4 |
| `bound_type_name` | 2 |
| `bound_call` `CALL_STATIC` | 1 |
| `bound_call` `CALL_DIRECT`, `CALL_VIRTUAL` (narrowed and through the root), interface calls, `CALL_BASE`, `CALL_CONSTRUCTOR`, natives; every `written_as` (operators, getters, setters, index, put, operator=, iterate, text append) | 2 |
| `bound_value_call` | 2 |
| `bound_new` | 2 (the constructor's full prologue: 4) |
| `bound_conversion`, `bound_cast`: every `CONV_` kind | 2 |
| `bound_is` (class, interface, array of a class, number array, `object` / `object[]`, constant) | 2 |
| `bound_not_null` | 2 |
| `bound_safe_navigation`, `bound_receiver`, chains | 2 |
| `bound_elvis`, `bound_ternary` | 2 |
| `bound_sizeof` | 2 |
| `bound_index` (read, store), `bound_new_array` (size, items), `bound_array_literal` | 2 |
| `bound_string_template` | 2 |
| `bound_dictionary` | 2 |
| `bound_binary` (`OPERANDS_INTEGER`, `_DECIMAL`, `_REFERENCE`, `_NULL_TEST`; every operator) | 2 |
| `bound_logical` | 2 |
| `bound_unary` | 2 |
| `bound_increment` (local, field, static, element, property; prefix and postfix; integer and decimal) | 2 |
| `bound_assign` (`=`, every `op=`, `store` conversion, properties) | 2 |
| `bound_function_reference` | 2 |
| `bound_lambda` | 2 (without a record: it captures nothing); 4 (with one) |
| `bound_get_component` | 4 |
| narrowing at stores, arguments and returns | 2 |
| conditions from decimals (truth) | 2 |
| the order of evaluation (6.0): right operand first, value before target; the copy of a local a later operand writes (5.3) | 2 |
| `CONV_NONE`, `CONV_ERROR`, `STORE_NONE`, `bound_error`, `bound_pending`, `bound_functions`: `E0970` | 1 |

### 9.3 Statements

| node | stage |
|---|---|
| `bound_block`, `bound_expression_statement`, `bound_empty` | 1 |
| `bound_local_declaration` with a value | 1 |
| ... with a start value (0, 0.0, null), `default_constructor` | 3 (built in 1; tested in 3) |
| ... `is_injected` | 4 |
| `bound_return` (no leaves) | 1 |
| `bound_return` through locks and finally blocks | 3 |
| the implicit `return` at the end of a body that `completes` | 1 |
| `bound_if` | 3 |
| `bound_when` (with and without a subject; `operator==` tests) | 3 |
| `bound_while`, `bound_do_while`, `bound_for` (endless forms) | 3 |
| `bound_foreach` over arrays and `loopable` (`get_elements`), typed item conversion | 3 |
| `bound_break`, `bound_continue` (with leaves) | 3 |
| `bound_throw`; the null test for a nullable value | 3 |
| `bound_try`, `bound_catch` (with and without variable), finally, exit stubs, nesting; the `nop` before a try and the jump inside each range (R1, R2 of 4.7) | 3 |
| `bound_when_clause`, `bound_loop` (the shared base of the loops: `is_endless`) | 3 |
| `bound_lock` | 3 |
| `bound_asm` (every mnemonic, every operand form) | 1 |
| line table entries | 1 |

### 9.4 Declarations and set-up

| feature | stage |
|---|---|
| frame layout: self, parameters, locals, hidden slots | 1 |
| constructors: base call, static check, `init<C>` call, body | 2 (base call and body); 4 (the rest) |
| `init<C>`: field values, default objects, injected fields, `init { }` blocks | 4 |
| `init(x)` conversion functions | 4 |
| `static_init<C>` with `@static_init_flag`: static field values, enum values, `static init { }` | 4 |
| module globals (`global_initializers`) | 4 |
| enum numbers the program does not fix | 4 |
| `localized` fields and locals, `fiber_initializers`, `tls_init` | 4 |
| closure records: classes, per-call creation, captured parameters, `@self`, `@parent`, field-value records | 4 |
| lambdas as functions with the hidden record parameter | 4 |
| function values (`function_value` objects) | 4 |
| components: `single` (storage, flag, function), `factory`, `inject` on fields, globals and locals | 4 |
| generic instances (classes and functions) | 1 (layout), 2 (calls) |
| extension functions (ordinary functions; on an instance: the instance's) | 2 |
| getters and setters (functions; reads and writes are calls) | 2 |
| interfaces, abstract functions (delegate records), implementations (delegate address) | 1 (records), 2 (calls) |
| native functions: records, calls | 1, 2; glue files: 7 |

### 9.5 Options

Section 8.2 lists every option with its stage; the ones that change the file:
`-o`, `-c`, `-v`, `-target`, `-d` / `-listing`, `-dbg`, `-R` (1; `-entry`, a test option of stages 1 - 3, removed in 4);
`-obf` (4); `-O*`, `-n<opt>`, `-Oreport` (6, done); `-L`, `-p`, `-P`,
`--new-project`, `-sni`, `-j`, `-cache`, `-cacheoff`, `-eager`, `-run`
(7).

---

## 10. Deliberate differences from the C++ compiler's code generation

The observable behaviour is the same in every case; the bytes are not.

| # | the C++ compiler | the bootstrap | why |
|---|---|---|---|
| G1 | "schemes" with fixed step layouts, lowered while tracking where the current value is ("machine data") | a linear IR whose values live in slots of a fixed kind; lowering per instruction; the optimizer on the IR (5.1) | the textbook structure; nothing implicit to get wrong |
| G2 | `finally` entered by a jump after storing a return address in a hidden local, left by `brh` back to it | a copy of the finally block per way out (exit stubs, 6.11); only direct jumps | direct control flow for the optimizer and the JIT; no hidden address local |
| G3 | an integer constant wider than 32 bits from the decimal table (`ldc` + `d2i`): wrong beyond 2^53 | built exactly with `movi`, `shl`, `movu32`, `or` (4.3) | correct for every 64-bit value |
| G4 | decimal constants written with 17 significant digits | the literal's own text, or the exact hexadecimal form (3.6) | exact without a float formatter |
| G5 | the data and meta sections zlib-compressed above 1 MB | never compressed (3.10) | the loader accepts both; no zlib in Sharp |
| G6 | random application id; guids from a randomly offset counter | a fixed function of the program; guids = addresses | deterministic output, needed for self-hosting (8.1) |
| G7 | an enum value without a fixed number gets an id-like number (16612 in one probe) | the smallest number not used by a fixed value of its enum, in declaration order | deterministic; the language promises only "different from its siblings" |
| G8 | the `.aout` listing written on every compile | only with `-d` / `-listing` | it is 2 MB for the library; compile time |
| G9 | obfuscated names from randomly offset ids | from a counter in a fixed order | deterministic |
| G10 | a `factory` definition's expression copied into every use | one function per definition, called | one copy of the code; same behaviour |
| G11 | `self` and captured variables also kept in the frame and copied into the closure object on every write | captured variables live only in the record (analysis D10) | any type can be captured |
| G12 | a function can be written that the loader misreads: a `finally` whose exception local is slot 5 makes a file the VM refuses (11.1, reproduced) | the generator never writes one | |
| G13 | static parts run in class address order, and the addresses follow the source files sorted by SIZE | modules in dependency order (imports first), a module's globals before its classes, classes as declared (6.15) | an order a reader can predict and that does not change when a comment is added |
| G14 | a `try` that begins right after a call catches that call's exception (4.7, reproduced) | a `nop` before every try; ranges end in a jump | the VM looks a call's exception up one word late; the C++ result is plainly wrong |
| G15 | `throw` of a null value crashes the VM (reproduced) - also a null the type does not show: `throw e!!;`, a local `e : oops;` never given a value, a field never set | raises `nullptr_exception` (6.11): every thrown value that is not a `new` object is tested | a crash is never wanted |
| G16 | a store into a `localized` `_int8` .. `_uint32` is not narrowed (reproduced: holds 300) | narrowed like every other store | a slip, not a rule |
| G17 | every class has `static_init<C>` and `@static_init_flag`, used or not | only a class with a static part (5.6) | less code; cannot be seen by a program |
| G18 | a function type written inside another function type gets index -1 in the file | listed and numbered (3.5) | the loader would read "the last function" |
| G19 | stack size 0 for a static function without parameters or locals that returns a value | at least 1 (4.4) | the result slot must be inside the frame |
| G20 | class addresses follow the files; `x is c` can be wrong for a number array when `c` has an address below 10 (11.6) | `__srt_global` classes take the lowest addresses | no object is ever an instance of one |

| G21 | an `init(x)` conversion runs only its base call and its body: no static check, no init<C> (decoded: `init<double>(decimal)`), so the fields of an object made by one keep 0 / null | the same prologue as a constructor (6.14) | an object made by a conversion is an object like any other |
| G22 | every override of a kept family root is kept ("a delegate function marks all its impls") | only the overrides in kept classes (5.5) | the textbook refinement (rapid type analysis): no object of a class that is not kept can exist |

| G23 | a field of a number class or `string` written without a value (`count : int;`) is NULL in the C++-built program (reproduced: `k.count == null` is 1; backend.md, "C++ compiler bugs found" of the initializers stage, item 3 - its `preInitScheme` is never run), although its own field compiler means it to start as an object, as a local `n : int;` does | the field starts as an object made by the constructor without parameters (`sharp_field.default_constructor`, set by analysis; `field_start_value`, generation/setup/setup.sharp) | the rule the language states, and the same for a field as for a local. EXCEPT a field WRITTEN with a type parameter (`value : T;` of a generic class): it starts null in every instance, also where `T` is `string`, as in the C++-built program - the library's `deferred_init<T>` tests it against null, and the suite case std_misc_fixes failed on the default object (13.3) |
| G24 | values on the way live in registers and on the operand stack; a frame holds little more than the locals | every value on the way has a frame slot until the optimizer (7.3) keeps it in a register | G1; the price is recursion depth, 11.8 |
| G25 | an `is` whose answer is known while compiling (`f() is shape` with `f() : square`) drops the code of its operand: `f()` is not called (`create_get_bool_constant_operation` replaces the scheme) | the operand is evaluated for its effects, then the answer written | an expression's parts run; the difference shows only when the operand has an effect |
| G26 | `+=` on a STATIC property (`box.count += 5`, getter and setter): the program reads back a wrong value (1 where 5 is right; reproduced, 13.2) | getter, add, setter, as for an instance property | a bug of the C++ compiler |
| G27 | `a?.f()` that ends in a CALL, with `a` null, does not give null (`z?.follow() == null` is 0; reproduced, 13.2) | the whole chain gives null (6.5) | a bug of the C++ compiler; the rule of the language is "the whole chain gives null" |
| G28 | `made().width += 3` on a property calls `made()` twice (once for the getter, once for the setter; reproduced, 13.2) | the parts of the target are evaluated once (6.4) | a bug of the C++ compiler; a plain field is evaluated once there too |
| G29 | `x is t` is answered from the DECLARED type when that settles it: `maybe_dog is animal`, `x is object`, `x is object[]` are 1 also when the value is null (probed: a null `dog?` `is animal` prints 1) | a value that may be null is tested when the program runs, and null is 0 for every `is`: against a class with the VM's `is`, against `object` / `object[]` with "is not null" (6.8). A value that cannot be null gives the same constant 1 as there | null is an instance of nothing; the `is` instruction of the VM says so itself |

| G30 | a `return` / `break` / `continue` that leaves locks AND finally blocks runs every finally block first and unlocks all the locks only at the final jump: in `lock(a) { try { lock(b) { return 1; } } finally { F } }` the block F runs with `b` still held (probed with a second fiber, the verifier of stage 3) | each lock is given back where its block is left, in nesting order (6.11): F runs with `b` free and `a` held. After the jump every lock is free in both | F is written outside `lock(b)`; holding `b` there is an accident of where the C++ compiler puts its unlocks (Java's `synchronized` behaves as the bootstrap does) |
| G31 | `continue` in `do { } while(c);` jumps to the top of the body, PAST the condition (`do_while_statement.cpp` hands the block the loop's begin label; probed: a round that continues never asks `c`, and `do { continue; } while(false);` never ends) | to the condition (6.10), as in every language with this loop | a slip of the C++ compiler; nothing in the library or the suite has a `continue` in a `do` |
| G33 | `base->f()` inside a lambda runs the OBJECT's own `f` (probed: a `dog`'s lambda calling `base->sound()` prints the dog's sound) | the base class's own body, as `base->f()` outside a lambda (6.12: the object is `@self` of the record, the call is `CALL_BASE`) | `base->` means the base's body wherever it is written |
| G34 | a `catch` variable a lambda captures is null in the lambda (probed: `nullptr_exception` when the lambda reads `e.get_message()`) | the caught object (6.12: moved into the record at the start of the handler) | a slip of the C++ compiler |
| G35 | under `-obf` a member a rule keeps (`obfuscate -keep { point.y }`) is renamed anyway when its class is renamed with everything in it (`should_obfuscate` asks only the class) | it keeps its name: the rule naming the member is the more particular one (6.17) | otherwise `-keep` on a member does nothing under `-obf` |
| G36 | obfuscated names from each declaration's id (random offset), and a spelling that repeats (26 gives `az`, 27 `aa`, ...) | a counter in a fixed order (files, classes with their fields, functions), spelled as spreadsheet columns (`z`, `aa`, `ab`) (G9) | deterministic output; every name its own |
| G32 | `lock(expression) { }` evaluates the expression TWICE: once for the lock, again for the unlock at the end of the block (probed: `lock(g.open()) { }` calls `open()` two times) | once, into the hidden local `held`; that object is locked and unlocked (6.11) | an expression that gives another object the second time unlocks the wrong one; a slip |
| G37 | `-O` optimizes the finished BYTECODE: decodes it, runs its passes, encodes it again and puts the generator's version back when the result does not verify (`bytecode_optimizer.cpp`) | `-O` optimizes the IR between generation and the bytecode step (7.2); the bytecode verifier checks every function after it as always, and a mistake it sees is E0970, never a function silently put back | the structure (slots of a kind, labels, calls) is still visible in the IR; a function put back hides an optimizer bug instead of reporting it |
| G38 | with `-O` a function inlined into its caller has no frame of its own: an uncaught exception raised inside it is reported at the caller's line ("VISIBLE DIFFERENCE", pass_inline_calls.cpp) | the same rule; but WHICH functions are inlined differs (the limit is 14 IR instructions here, 24 VM instructions there, and here a body may call others: 7.2), so the stack trace of an `-O` build can name other frames than the C++ `-O` build's. A frame's size changes with `-O` too (its share of inlined slots, fewer frames on the stack), so the recursion depth at which `stack_overflow_exception` comes (G24) is not the same as without `-O` | only stack traces and the depth of a stack overflow can show it; catching is unchanged (the body is inside the same `try` ranges the call was) |

| G39 | the compiler ends with exit code 0 after every compile, with errors or without (1 only for a refused option) | 1 when the program has errors, 0 when it compiled; with `-run` the program's own code (main.sharp) | `sharpc main.sharp -o app && sharp app` must not run an old `app`; every build tool reads the exit code |
| G40 | no nesting limit of its own: the parser recurses until the C++ stack ends (about 300 levels) | counted limits - 256 open statements and expressions in the parser (E0103), 150 statements deep in the analysis (E0992) - and, below them, the VM's stack (the same messages, with the hint to start the compiler with `-istack 1mb`); 13.7 | how far the stack reaches depends on how the compiler itself was built (stage 1, stage 2, `-O`) and started; a count is the same in every build, which self-hosting needs: every build accepts and refuses the same files |
| G41 | `settings.json`: `"release": true` obfuscates and takes the debug flag out; `"target_api"` picks the installed library | the same two, plus settings the C++ compiler does not have (`optimize_level`, `obfuscate`, `cache_files`, `output_file`, `extern_libs`, `ignore_extern_files`, `ignore_extern_folders`, and `version`, which `--new-project` writes but the C++ compiler never reads); a name that is not a setting is ignored there and is a warning with the nearest names here; a file that is not JSON stops the build here (13.7, the verifier's pass) | a project can say everything the command line can; a misspelt setting or a broken file must not silently build something else |

| G42 | a CONSTANT decimal out of the 64-bit range stored in a whole number (`n : _int64 = 1e30;`, `(1.0e30 as _int64)`, `2 ** 63`, `3 ** 40`) is folded with a C++ cast: -9223372036854775808 for every one, positive ones included - while the same conversion when the program runs (`d2i`, `decimal_to_integer`) gives the nearest end, so `pw(2, 63)` computed at run time gives 9223372036854775807 (probed by the completeness critics) | folded as `d2i` does it while the program runs: the nearest end (9223372036854775807 for a positive number); a constant and the same value computed agree. W1037 names the value stored (backend.md D97) | a fold must give what running the code gives; the C++ result is undefined behaviour of a C++ cast |
| G43 | a hexadecimal literal of 2^63 or more (`0x8000000000000000`, `0xFFFFFFFFFFFFFFFF`) goes through a decimal and saturates: 9223372036854775807 (and `>> 60` of it is 7) | the 64 bits written, as two's complement: `0xFFFFFFFFFFFFFFFF` is -1, `0x8000000000000000` is -9223372036854775808, `>> 63` of it is -1 (shifts are arithmetic); used as a decimal it is the decimal of that `_int64` (-1) (`integer_value`, literal_expression.sharp) | hexadecimal writes bits, as in C, Java and C#; it is the only way to write the top half of a `_uint64`. W1037's help for a big `_uint64` gives the hexadecimal and says the first compiler reads it differently |
| G44 | `foreach(d : decimal in nums)` over an `_int64[]`, or `foreach(b : _int8 in nums)`: the item is not converted: `d / 2` prints 0, 7.4e-322 and -nan for 1, 300 and -2 (the integer's bits read as a decimal), and the `_int8` loop prints 1, 0, 0 (probed again in the completeness stage) | the item is converted to the variable's type, as a declaration `d : decimal = nums[i];` converts it (backend.md 12.3, 6.10): 150 and -1 for the decimals, 44 and -2 for the `_int8` | a typed loop variable is a declaration |
| G45 | `asm`: `mov8`, `mov16`, `mov32`, `mov64`, `movu8` .. `movu64`, `d2i` and `i2d` with two registers encode the FIRST register twice (`asm_statement.cpp` asks `getSubAst(ast_assembly_register)` twice), and `dmovi` swallows the instruction after it (probed: `mov8 ecx, ebx` with ebx = 300 leaves ecx unchanged, and a `dput` after a `dmovi` is lost) | what is written (asm_table.sharp): `mov8 ecx, ebx` puts 44 in ecx, `dmovi` is one instruction | the instruction table of the VM; the library writes none of these ten with two different registers |
| G46 | a lambda's write to a captured object local is lost: `b := new box(1); run({ -> b = new box(2); });` leaves `b.n` 1 (the closure has a copy, G11); suite-like programs that accumulate in a lambda (`grand = grand + o.total()`, `biggest = o`) end with wrong totals or `nullptr_exception` | the variable is shared (it lives only in the closure record, G11, 6.12): `b.n` is 2 | the rule of closures in every language that has them |

One more difference shows through an ANALYSIS decision, not through the
generator: the C++ compiler types every whole-number result as `_int32`
(backend.md D13), so its `(big * big) as _int32` is a cast of an `_int32` to
`_int32` and cuts nothing - the program prints 4900000000 for a 64-bit `big`
of 70000. Here the product is an `_int64` and the cast cuts it to 32 bits
(605032704), which is what the cast says. The same decision (D13) shows in a
string template: `${n + 1}` with `n : _int64 = 3000000000` prints
-1294967295 from the C++-built program (the sum is an `_int32` there) and
3000000001 from the bootstrap's.

G13 - G16, G21, G23 - G35, G38 and G42 - G46 are the only ones a program can see (G35, G36 and G38 only through reflection and stack traces): G24 only as a
`stack_overflow_exception` at a smaller recursion depth (11.8); the others
only where the C++-built program reads a value that was never set, is wrong,
or crashes. If
the owner wants the C++ behaviour kept in one of them, it is a one-line change
at the place named.

---

## 11. Traps in the VM's loader, and other findings

Found by reading `lib/runtime` and then reproduced with programs the C++
compiler built (section 12 says which); each is a rule the generator follows
and the verifier checks, and a case in `tests/codegen`. Four of them are BUGS
OF THE VM that C++-built programs hit today (11.1, 11.4, 11.6 and the crash of
`throw null`, 6.11). The bootstrap steers around all of them, so none blocks
this build; the owner may still want them fixed in `lib/runtime`, and each
entry says where.

### 11.1 A finally slot numbered 5 ends the data section early (VM bug, reproduced)

`process_function_data` reads function records with the `PROCESS_SECTION`
macro: after each record it tests whether the LAST byte it read is the
section's end marker, `05`. That last byte is the low byte of the last int32 of
the record. For a function whose last try record has a `finally`, that int32 is
the finally's exception slot: if the slot is 5, 261, 517 ..., the loader thinks
the data section ended and fails with "data section may be corrupt" (for the
very last function it does not fail there but reads the code section one byte
off). Reproduced: `def five(a, b, c, d, e: _int64) { try { .. } finally { .. } }`
compiled by the C++ compiler - its hidden exception local gets slot 5 - gives a
file the VM refuses: "data section may be corrupt. Could not start the Sharp
virtual machine". The bootstrap never gives a finally's exception slot a number
whose low byte is 5 (the slot allocator skips it for that use). The fix in the
VM would be to count records instead of testing a byte
(`exe/data_processor.cpp`).

### 11.2 A code record must not end in the byte 1D

The code section's end marker is `1D`, tested the same way after each code
record against the low byte of the record's last word. Every function the
bootstrap writes ends with `ret` (whose low byte is `03`), so this cannot
happen.

### 11.3 Other findings

- `process_symbols` and `PROCESS_SECTION` skip `00 0A 0D` only BETWEEN records:
  a name may not contain a `00`, and a section marker must follow the previous
  section's end directly (3.2).
- The class `guid` decides `is`, casts and `catch` (`are_classes_related`,
  `is_class_or_subclass`): two classes with the same guid are "related". Guids
  must be unique (the bootstrap uses the address).
- `sizeof` of a null array is 0, not an exception (`SIZEOF` handler); what
  `foreach` over null does follows from it (Q4).
- The C++ opcode builder's range check for class-D operands allows one bit more
  than the encoding holds (4.2).
- Nothing reads a finally's end pc; it is written for completeness.
- The loader keeps a table of 14 "native types" (`symbol_processor.cpp`,
  `nativeSymbolCount`) and hands out `&nativeTypes[type]` for the type of a
  function that returns nothing - type 19 (`nil`), five entries past the end
  of the table. Every such function's result type points outside the table.
  The C++ compiler writes 19 and so must the bootstrap (the loader accepts
  nothing else for "no result"). **It did go wrong, and is FIXED (stage 6,
  13.6):** reflection reads that entry's `type` at start-up
  (`build_reflect_function`), and when the bytes past the table happened to
  read as `type_class` or `type_function_ptr` it followed a garbage pointer -
  a SIGSEGV before `main`, depending on the heap's layout, which the length
  of the program's path changes (the kostya json program built by the
  bootstrap crashed when started as `/tmp/fullbench/boot/json` and ran as
  `./json`). The table now has an entry for every type up to `type_nil`
  (lib/runtime/exe/symbol_processor.cpp).
- A handler is entered with whatever was on the operand stack (4.7): a function
  that catches in a loop grows its stack use by a slot or two per round until
  it returns. Same in the C++ output.
- Met while probing, a crash of the C++ COMPILER (not the VM):
  `x = n?.value ?: m(5);` with `value` a number field stops its code generator
  with "attempting to consume numeric data instead of object". The bootstrap's
  lowering of 6.5 has no such case: `%d` is simply an INT slot.

### 11.4 A call's exception is looked up one word late (VM bug, reproduced)

Section 4.7 has the table. Reproduced with the C++ compiler:

```
def before_try() {
    boom();                                   // throws
    try { quiet(); } catch(e: exception) { println("WRONG: the try after the call caught it"); }
}
```

prints the WRONG line, with and without the JIT: the `call boom` is the word
before the try's first word, and the interpreter searches with pc + 1. The
bootstrap's rules R1 and R2 make its own code right either way. The fix in the
VM would be in `main_vm_loop` after `return_method()` (`virtual_machine.cpp`,
label `catch_exception`): search the caller with the pc of the call, as the
`ret 1` path already does.

### 11.5 Function address 0 cannot be a function value

`call_function_value` refuses an address `<= 0` (`memory/function_value.cpp`).
Layout puts `platform.tls_init` at address 0 (4.6); the verifier checks that
no function value is made of the function at 0.

### 11.6 `is` cannot tell a class address from a number type (VM bug, read)

`is r, x` (`types/sharp_type.cpp`, `is_type`) looks only at the object: for a
number array it compares the array's element type with x, for an object it
takes x as a class address. The compiler knows which it meant; the VM does
not. So `value is c` answers 1 for an `_int8[]` when `c` is the class at
address 0 (an `_int64[]` and address 3, a `decimal[]` and address 9 ...), and
`value is _int8[]` answers 1 for an object of the class at address 0 or below
it. Both need a value typed `object`. The bootstrap gives the addresses 0 - 9
to `__srt_global` classes (6.15): nothing is an instance of one and no program
can name one after `is`, so both wrong answers become impossible. (A program
with fewer than ten modules fills the rest of 0 - 9 with closure classes and
then with ordinary classes, for which the flaw stays as it is in the C++
output; the library alone has thirteen modules.) `cast` is not affected: it
tests "is an object" first.

### 11.7 Fiber locals at shutdown when srt_init does not run (found in stage 1; gone since stage 3)

A file whose program has `localized` fields, started at a stubbed
`srt_init` (so `tls_init` never runs), loads and starts, but the VM's
shutdown then reads the fiber-local slots nobody set: with the JIT it hangs
after "Executed in ...", with `-nojit` it crashes; the address-sanitizer
runtime (`~/sharp_work/rt/vasan`) reports a use after free on thread T1. A
program whose srt_init runs sets them first and does not meet it. The
library check of tests/codegen.sh therefore reads only the first line of
that run. To look at again when srt_init is generated (stage 4): if it is
still there with tls_init run, it is a VM bug for lib/runtime.

Looked at again in stage 3, when srt_init came to be generated: with
tls_init run, the `-eager` library runs to its end with and without the JIT,
exit code 0, nothing printed - so it was not a VM bug, only the stub. The
library check of tests/codegen.sh now runs it to its end both ways.

### 11.8 The stack of a fiber is 4096 slots: frame size is recursion depth (found by the verifier of stage 1)

`lib/runtime/multitasking/thread/stack_limits.h`: `INTERNAL_STACK_SIZE` is 64
KB / 16 bytes = 4096 slots for ALL the frames of a fiber, and a call that
would pass it raises `stack_overflow_exception`. A frame takes "stack size"
slots (4.4), so how deep a program can recurse is 4096 divided by the frame
size of the recursing function. Measured with

```
def depth(n: _int64) : _int64 { if(n == 0) { return 0; } return 1 + depth(n - 1); }
```

| built by | frame of `depth` | deepest `depth(n)` that returns |
|---|---|---|
| the C++ compiler | 1 slot (values on the way are in registers) | 4000 runs, 6000 overflows |
| the bootstrap as stage 1 left it | 4 slots | 1000 runs, 1100 overflows |
| the bootstrap now (operands given back before the result's slot is taken, 5.3) | 3 slots | 1300 runs |
| the bootstrap with `-O` (slot packing, 7.2; measured by the verifier of stage 7) | 2 slots | 2000 runs |

So a program that recurses a few thousand deep through a small function runs
when the C++ compiler builds it and overflows when the bootstrap does: the
one place where G1 (every value on the way in a slot) can be SEEN (G24). For
a function with several locals the ratio is much smaller (the locals are in
the frame either way). What closes the gap is section 7.3 - keeping values in
registers - which is planned for the optimizer (stage 6) and so only with
`-O`; if stage 5 meets a suite case that recurses this deep, a cheaper first
step is to give an instruction its small constants as operands directly
instead of through a slot of their own. The owner may also simply raise the
VM's limit.

---

**What it means for the compiler itself (measured in stage 7).** The
compiler is a recursive program, so its own nesting depth is a matter of
frame size too - and of which compiler built it. The deepest code each build
accepts (nested `if` blocks / parentheses one inside the other):

| the compiler | default stack (64 KB) | `sharp -istack 1mb` |
|---|---|---|
| stage 1 (built by the C++ compiler) | 112 `if`s / 120 parentheses | 148 / 254 |
| stage 2 (built by itself) | 71 / 65 | 148 / 254 |
| stage 2 built with `-O`, before slot packing (7.2) | 28 / 41 | 148 / 254 |
| stage 2 built with `-O` | 100 / 130 | 148 / 254 |

With the default stack the stack runs out first, sooner the larger the
frames; the compiler says so (E0103 / E0992, never a crash) and names the
remedy. Inlining made the frames of `-O` code larger still, until the
optimizer got its last pass, slot packing (7.2): with it the compiler built
with `-O` - the one a user keeps - goes about as deep as stage 1 (deeper in
expressions, a little less deep in statements), where it used to stop at a
quarter of that. With a 1 MB
stack every build stops at the same counted limits (G40) and all three accept
and refuse the same files. The self-built compiler is therefore best
started as `sharp -istack 1mb -mem 4gb sharpc ...` (README.md); without it,
it still takes any code a person writes (100 blocks one inside the other).
For the UNOPTIMIZED self-built compiler the gap to stage 1 stays (71 against
112): its frames are what the generator makes, and no pass runs without `-O`.

## 12. The questions, answered

The design left thirteen questions "to settle by probing the C++ compiler".
The design review settled them all: each was put to the C++ compiler as a
small program, the program was run (with and without the JIT where it
mattered), and the file it wrote was decoded. Nothing here is left for a later
stage to find out, with the one exception marked OPEN (Q14), which needs the
bootstrap's own code generator to exist.

**How the answers were found.** The probe programs and the decoder are in the
scratch folder of the build, `.claude/jobs/c4f2c674/tmp/critic/` (`q1.sharp`
questions 2-4, 10, 12, 13; `q2.sharp` evaluation order; `o_main.sharp`,
`o_a.sharp`, `o_z.sharp` static order across modules; `h1.sharp` finally,
closures, virtual calls, boxes, fiber locals, `throw null`, kept fields;
`t1.sharp` the two VM bugs; `n1.sharp` narrowing and names; `decode.py` reads
a whole executable by section 3 and fails on any byte it cannot place). Stage
1 turns each probe into a `tests/codegen` case, so the answers stay checked.
Compile one with `~/sharp_work/sharpc_cl/sharpc <file> -o <out> -L
~/sharp_work/stdlib`; the C++ compiler writes its listing to `<out>.aout`.

| # | question | answer (measured) | decision |
|---|---|---|---|
| Q1 | In which order do the static parts of classes and the globals of modules run? | Enum values first, then the `main` value, then `static_init<C>` in class address order; the addresses follow the source files sorted by size. Nothing initialises on demand except a constructor: `static b = 1 + gamma.g` reads 0 when `gamma` comes later. The module order on the command line does not matter | 6.15: the same three steps; the bootstrap's own, predictable order (imports first). Difference G13 |
| Q2 | Does `y := x!!` with a null `x` raise anything? | No | 6.5: nothing emitted. backend.md 6.4 / 12.3 corrected |
| Q3 | Is an argument narrowed for an `_int8` parameter, and at other narrowing conversions? | Yes, everywhere: argument (in the caller), return, `as`, stores into locals, fields, array elements all give 44 for 300 | 4.3, 6.2, 6.3: narrow at every narrowing `CONV_NUMERIC` and store. No difference from the C++ compiler |
| Q4 | `foreach` over a null array? | No rounds, no exception | 6.10 as written. backend.md 12.3 corrected |
| Q5 | `throw null`? | The VM crashes (SIGSEGV) | 6.11: raise `nullptr_exception`. Difference G15 |
| Q6 | The order of evaluation where the parts have effects | 6.0 has the whole table. The two surprises: the RIGHT operand of a number operator runs first, and the VALUE of an assignment runs before the parts of its target | 6.0: follow it exactly; it is a rule of the language (suite case `operators_evaluation_order`) |
| Q7 | The names the C++ compiler gives getters, setters, lambdas, `init` blocks, closure classes | `get_x`, `set_x`; `@anonymous_fun_<n>` (n differs from build to build); `init { }` blocks are inside `init<C>`; `closure_<full function name>` | 5.6, 6.12: the same names, the lambda number deterministic |
| Q8 | Does a C++-built program whose finally has its exception local in slot 5 fail to load? | YES: "data section may be corrupt" | 11.1: a real VM bug; the bootstrap skips such slot numbers. Reported to the owner |
| Q9 | The owner the C++ writes for a top-level class, and the class records' order | The owner is the address of the module's `__srt_global` class; only those have -1. Records are in address order (any order loads) | 3.5 corrected |
| Q10 | Does `sizeof(object)` count only the fields the program uses? | Yes: 1 for a class with two fields, one of them never named | 5.5: kept fields only - and "kept" must equal the C++ compiler's "used" (Q14) |
| Q11 | Is a value stored into a `localized` `_int8` narrowed? | No: it then reads 300 | 6.2: the bootstrap narrows. Difference G16 |
| Q12 | Is `-0.0` true or false as a condition? | False | 6.3 as written |
| Q13 | What exit code does a program get? | 0 in all three cases: `main() : int` returning 3, `main()`, and an uncaught exception (which prints `Unhandled exception on thread Main` and a stack trace) | 5.8: the tests compare output only. (A VM bug, fixed in the completeness stage, 13.8: the runtime now ends with main's result, and with 1 after an uncaught exception - for C++-built programs as well) |
| Q14 | Does the bootstrap's `reached_fields` name exactly the fields the C++ compiler calls used? | No: analysis reaches every field of a reached class that has a value. The C++ side (5.5): a field no reached code names gets no value, even with side effects | stage 1: kept.sharp walks the program itself and keeps a field only when kept code names it; tests/codegen/cases/kept_fields.sharp gives the same output as the C++-built program |

Also learned while probing, and written where it belongs: the two VM bugs of
11.4 and 11.6, the address-0 rule (11.5), the table overrun (11.3), the crash
of the C++ code generator on `n?.value ?: x` (11.3), that the C++ compiler
captures objects only (6.12), that every class there has a `static_init<C>`
(5.6), that a function type inside a function type is written with index -1
(3.5), and that a top-level function is `public static`, not "global"
(3.5.1).

---

## 13. The stages

Every stage fills in its section: what it built (files), the rules it follows
and the codes it adds, the results of the probes it ran, the gates result, the
timing, what is not done, and what the next stage must know. Codes reserved for
code generation: `E0970` - `E0979` (internal errors and limits; stage 2 added
none). Given out so
far: `E0970` internal error (since stage 4 also a node the generator has no
code for), `E0971` an `obfuscate` block renames a name the VM finds by name
(13.4), `E0972` too large for the encoding, `E0975` no `main`, `E0978` a
function's body nests deeper than the generator can follow (13.3). Retired in
stage 4 with the test options: `E0974` not generated yet, `E0976` / `E0977` a
wrong `-entry` (13.1). Reserved and never given: `E0973` (a native function
that is not static, 4.6: analysis makes every native function static, so the
case does not arise). Next free: `E0979`.

### 13.0 The design and its review

status: done

The design stage wrote this document from the sources of the VM and the C++
compiler. The review stage then checked it against both, by decoding files
the C++ compiler wrote and by running probe programs (section 12), and
changed the document where it was wrong or silent. No compiler code was
written by either. What the review changed, so a reader of an earlier copy
knows where to look:

- 3.5, 3.5.1, 3.7, 3.8: the owner of a top-level class, which classes have no
  base, localized fields in the static list, the `global` flag (never
  written), function types inside function types, kinds of getters, setters
  and lambdas, try ranges counted in words.
- 4.4: stack size at least 1 for a function that returns a value.
- 4.5, 4.6, 4.7: the flaw of `is`; function address 0; static natives only;
  the pc a call's exception is looked up with and the rules R1 and R2; the
  operand stack at a handler.
- 5.3: when a local's slot may stand for its value. 5.5: fields that are not
  kept get no value. 5.6: names. 5.7: more checks. 5.8: what is kept with
  `-entry`, stubs, the gates' library step, what a test compares.
- 6.0 (new): the order of evaluation. 6.2 - 6.5, 6.10, 6.11, 6.12, 6.15,
  6.17: every "question Qn" replaced by the measured answer; the `try`
  layout; `throw` of a null; the initialisation order; obfuscated file names.
- 10: G12 confirmed, G13 - G20 added. 11: 11.1 reproduced, 11.4 - 11.6 new.
  12: the questions, answered.

For the owner to decide (nothing waits on it; the document says what is done
meanwhile):

1. Four VM bugs found (11.1, 11.4, 11.6, `throw null`): fix them in
   `lib/runtime`, or leave the VM as it is? The bootstrap's output avoids all
   four either way.
2. G13 - G16: in these four places the bootstrap's program behaves better
   than the C++-built one. Keep, or copy the C++ behaviour?
3. `!!` tests nothing (6.5). Make it a real test?
4. Still as the design left them: exit stubs instead of the C++ compiler's
   return-address scheme (G2), and never compressing the file (G5).

### 13.1 Output file and skeleton

status: done

**What a compile does now.** After `analysis complete: no errors`, when `-o`
or `-entry` is given (and not `-c`), main.sharp calls `generate_program`
(finalizer.sharp), which runs the steps of 2.2 - layout, generation,
bytecode, verify, write - and prints one line:

```
code generation: 58 functions, 76 classes, written to hello
time: layout 12 ms, generation 4 ms, bytecode 1 ms, write 2 ms          (with -timings)
```

or the messages and `code generation failed: n error(s), no file was
written`. Without `-o` / `-entry` nothing changes: every analysis-only run of
the gates is as before.

**The first programs run.** `tests/codegen/cases/hello.sharp` - a `main`
that prints a text and a number through the library's `println` - is
compiled by the bootstrap compiler and runs on the VM with the same output
as the C++ build, with the JIT and with `-nojit`. So do the other cases
(numbers, constants, control, arrays, objects, dispatch, calls, order,
statics, assembly, kept_fields). They are started with `-entry main`: the
platform's start-up `srt_init` uses string templates, `when`, casts and
function values (stage 2 - 4), so it is not generated yet; `main` itself then
begins with the static parts srt_init would have run (5.8).

**Built** (the map of section 0 has every file):

- **layout/**: the kept set (5.5) by its own walk; class addresses in the
  initialisation order with the `__srt_global` classes first (6.15, 11.6);
  instance slots after the base's, own static slots, fiber-local slots
  (3.5); interfaces from the top; what init<C> / static_init<C> run;
  function addresses class by class with `platform.tls_init` at 0 (11.5);
  names, kinds, VM flags (3.5.1); the delegate address of every override
  (3.7); the function-type table with the inner types (3.5, G18); the string,
  constant and file tables.
- **ir/**: the IR of 5.2 (35 instruction kinds), slots of a fixed kind reused
  from a free list per kind (5.3), labels, line marks, `-ir`.
- **generation/**: see "what is generated" below.
- **bytecode/**: selection (section 6's right column), the encoding (4.2) with
  E0972 for an operand that does not fit, two linear passes for the labels
  (5.4), the line table, the frame numbers (4.4, with G19); every function
  ends in `ret` (11.2); the whole asm instruction set (asm_table.sharp, read
  from the C++ compiler's asm_statement.cpp, code_context.cpp and opcode.cpp).
- **verify/**: decoding, jump targets, frame slots, function and class
  addresses, invoke_delegate's second word, `ret` at the end, and the
  operand-stack height along every path (5.7). Not yet: the kind of a
  register, try records (stage 3), function values of address 0 (stage 4).
- **output/**: every section of section 3, never compressed (G5); the
  application id is a fixed function of the output name and the number of
  functions (G6); guids are addresses.
- **listing/**: `-listing` (or `-d`) writes `<output>.listing`: every class
  (fields, methods, and the declared fields that were NOT kept) and every
  function (frame numbers, line table, the words decoded). It is written
  also when the verifier refuses the code, to show what it refused.

**What is generated** (the "first small set" grew to what the hello world
through the library needs, and a little more because each piece was small
once the IR was there): literals (integer, wide integer - G3 -, decimal with
the literal's own digits or the exact hexadecimal form, char, bool, text as
`_int8[]`), `null`, locals, `self`, fields (instance, static, global, fiber
local), every `CONV_` kind but `CONV_NONE` / `CONV_ERROR` (checked downcasts
to a class or a number array), `as`, `!!` (nothing, Q2), calls of every
dispatch (static, direct, `base->f()`, constructor, virtual through the
family root - narrowed to a direct call only when no kept class BELOW the
function's own has another body of the family -, static natives), `new`, the
operators on numbers (right operand first, 6.0), `&&` / `||`, `-` / `!`,
`==` on references and against null, `=`, every `op=`, `++` / `--` on locals,
fields, statics and elements (not on properties), arrays (`new t[n]`,
`new t[] { }`, array literals, elements, `sizeof` of an array), blocks,
locals with a value, a start object or the start value 0 / 0.0 / null,
expression statements, `return` (not through a `finally` / `lock`), `if`,
`while`, `do`, `for`, `foreach` over an array (the `get_elements()` of a
`loopable` included: analysis made it a call), `break` / `continue` that
leave nothing behind, `asm` blocks; the constructor's prologue (base call,
static check, init<C>), init<C> with `init { }` blocks in place, static_init<C>
with its guard and the enum values (`std#_enum_(name, ordinal)`, as the C++
compiler makes them; numbers the program does not fix: G7), platform.static_init
(enums first, the `main` value, then the other static parts) and
platform.tls_init.

**What stops with E0974 "not generated yet"**: string templates, `?.`, `?:`,
`? :`, `is`, dictionaries, function values and lambdas, calls of function
values, captured variables and closure records, `localized` locals,
components (`get`, `inject`), `sizeof` of an object, `op=` / `++` on a
property, `try` / `catch` / `finally`, `throw`, `lock`, `when`, jumps out of a
`finally` or a `lock`, enum values used as values (`bound_enum_value`),
`base` as a value (anything but the object of `base->f()`). Each names the
node and its place:

```
error[E0974]: not generated yet: string template at /home/.../platform/kernel/platform.sharp:20
```

**Stubs.** With `-stubs` such a function is written as a stub that prints
`not generated yet: <function>` and returns 0 / null - it does NOT stop the
program. (Stopping is not possible this early: `hlt` stops only the thread
and the VM waits for ever; the VM's exit interrupt waits for the thread
set-up srt_init does, which is itself a stub. A test that meets a stub fails
on its output.) The whole library with `-eager` has 11306 functions, 3097 of
them stubs.

**Codes given out**: E0970 internal error (the verifier refused a function,
or selection met an IR instruction or operation it has no VM sequence for -
it is said, never replaced by other code; names the function and what is
wrong), E0972 too large for the encoding, E0974 not
generated yet, E0975 no `main` (an executable starts there), E0976 `-entry`
names no function (with did-you-mean), E0977 `-entry` names a function the
VM cannot start at (it must be static and take nothing or one `object[]`).
E0971 and E0973 stay reserved for stage 4 (E0973: a non-static native is
"not generated yet" meanwhile). Next free: E0978 (taken in stage 3: nested too deeply, 13.3).

**Options** (8.2): `-o` (the file, and the application name in the manifest,
as the C++ compiler writes it), `-c` (no code generation), `-v`, `-target`
(without it the manifest says 2, the C++ default), `-entry`, `-stubs`,
`-eager` (keep everything analysis has: the C++ compiler's option of that
name), `-listing` / `-d`, `-ir`, `-dbg` / `-debug` (the manifest's debug flag
and the text of every source file in the meta section: stack traces then
print the lines, as with the C++ compiler's `-dbg`), `-R` / `-release` (takes
`-dbg` out again; its optimizer half is stage 6). Not done: `-run` (the C++
compiler has no such option; the bootstrap's is still only parsed), and there
is no "strip" in the file format to honour: line tables and names are always
written, by both compilers.

**Tests and gates.** `tests/codegen.sh` (a new gates step, "codegen tests"):

- `cases/*.sharp`, twelve programs, each compiled with `print.sharp` and the
  library (`-entry main`), run with the JIT and with `-nojit`; both outputs
  must be the case's `// output:` block, and the C++-built program (normal
  start-up) must print the same block - the oracle. hello, numbers (integer
  and decimal arithmetic, comparisons, bits, wide constants, narrowing),
  control (every loop, break, continue, if), arrays, objects (field values,
  `init { }`, a static part, `-> base(...)`, virtual calls through an
  interface), calls (recursion, the order of evaluation of 6.0, the copy rule
  of 5.3, an argument narrowed), assembly (labels, the operand orders),
  kept_fields (Q14, below); and, added by the verification pass: constants
  (decimals the compiler folds, written in the exact form), order (a part of
  an expression that changes a local another part reads, 5.3), statics
  (globals and static fields read before any object is made, 5.8), dispatch
  (overrides down a chain of classes, `base->f()`, hiding, interfaces,
  generic classes).
- `-dbg`: hello once more with `-dbg`; the file must carry the source text
  and print the same.
- `stops/template.sharp` (stage 2 replaced it by stops/throw.sharp, 13.2): a string template must stop the compile with E0974
  `... at <file>:<line>`, and no file may be written.
- the library: the whole library with `-eager -stubs` (11306 functions, 293
  classes) through layout, the verifier and the writer; the VM must load it
  and start it (it prints srt_init's stub line). And the library as a `main`
  of nothing reaches it (151 functions): loaded, started, exit code 0.

The C++ compiler now and then aborts under load (its own bug, also seen by
the help tests); codegen.sh compiles once more then and says so.

**What the verification pass found and changed** (an independent check of the
stage: experiments compiled by both compilers and compared; each fix has a
case now):

- A decimal the compiler folds (`0.1 + 0.2`, `0 - 7.9`) was written into the
  constant section as the text of its OPERATOR (`+`, `-`), which the VM reads
  as 0. Only a literal's own node gives its digits now (`literal_digits`,
  generation/values/values.sharp); anything computed gets the exact form.
- A local read by one part of an expression and changed by a part evaluated
  after it: `(x = 5) + x`, `x++ + x`, `(x *= 3) - x`, `a[i++] = i` gave the
  NEW value where the C++-built program gives the old. `held_over` (5.3).
- A virtual call was made direct whenever the called function itself had no
  `overridden_by` - but the overrides of a family are listed on its ROOT, so
  `an_animal.sound()` ran animal's body on a dog. `called_directly`
  (generation/calls/dispatch.sharp) searches the root's list for a body in a kept
  class below.
- `base->f()` stopped with "not generated yet: `base`".
- With `-entry` no static part ran before the test's `main`: a global or a
  static field with a value read 0 / null, silently. The entry function now
  begins with them (5.8).
- Selection chose `add` for any operation it did not know, and skipped an IR
  instruction it did not know. Both are E0970 now.
- `-dbg` writes the source text (it was left for stage 4; it is the file
  writer's, so it is here).
- Frames are one to two slots smaller (5.3, 11.8).
- Sixty functions had no comment of their own; every one has now.

**Q14 answered** (5.5, 12): kept fields are exactly the fields kept code
names - the walk of kept.sharp does not let a class keep its fields
(analysis's `reached_fields` does: "a class reaches its initialisation
lists"), so `kept_fields.sharp`'s unused field with a printing value prints
nothing, as in the C++-built program.

**Timing** (the gates' machine, `-timings`): the whole library with `-eager`,
11306 functions: layout 0.8 s, generation 0.5 s, bytecode and verify 0.4 s,
write 0.6 s - about 2.3 s, a little more than its analysis (2.1 s); the
budget of 8.3. A program like hello: about 20 ms for all of code generation.

**Changes outside backend/finalizer** (all additions but one fix):

- `sharp_declaration.kept`, `sharp_declaration.address` (layout's numbers)
  and `local_variable.slot` (the frame slot): new fields.
- A FIX in analysis, `backend/postprocessor/names/type_names.sharp`
  (`base_class_of`): the stage-3 lookup of a class's written base no longer
  overwrites the real base of a generic instance that resolve_bases already
  worked out. Before it, `number<_int8, bool>` had the BLUEPRINT
  `base_number<NT>` as its base, and its constructors called the blueprint's
  constructor (found because code generation has to call a real function;
  analysis printed `call base_number() (constructor)`). The gates show no
  other change.
- main.sharp: the options above; `generate_executable`.

**Found** (section 11 has them in full):

- `analysed.required_classes` (the C++ compiler's list) leaves out classes
  the VM looks up by name and makes objects of (`std#bool`, `std#char`,
  `std#long` ... and `platform.kernel#function_value`): an executable without
  `std#bool`'s `value` field makes the VM fail at start-up ("no number field
  `value` in the object"). Layout keeps the VM's whole list (kept.sharp,
  `vm_named_classes`; 3.11).
- With `srt_init` a stub, a file whose program has `localized` fields (the
  whole library: std.io.coroutines has two) loads and starts, but the VM's
  shutdown then reads the fiber locals tls_init never set: a hang with the
  JIT, a crash with `-nojit` (the address sanitizer runtime: a use after free
  on thread T1 after "Executed in ..."). Not seen once srt_init runs - it
  calls tls_init first; the library check therefore stops at the first line.
  To look at again in stage 4.
- In asm, `je` / `jne` take a number, not a label - in both compilers; a
  conditional jump to a label is `movi adx, @label` then `ife` / `ifne`.
- A fiber's stack is 4096 slots, so the bootstrap's larger frames overflow
  at a smaller recursion depth than the C++-built program's (11.8, G24).
- A box or `string` field without a value is null in the C++-built program
  and an object in the bootstrap's (G23): for the owner to decide.
- The VM's `class_cast_exception` names the class at address 0 as the target
  whatever the cast was to (`casting from _int64[] to <class 0>`); the two
  builds differ in which class that is (G13, G20), so that one line of an
  uncaught cast differs.
- Three places where ANALYSIS and the C++ compiler disagree, seen because the
  programs now run (not code generation's; for the leftovers stages):
  `x : c = 3;` with `init(d: decimal)` written before `init(n: _int64)` runs
  the decimal one in the C++-built program (the first that takes the value)
  and the `_int64` one here (the closer type); `f(3)` with `f(d: decimal)` and
  `f(n: _int64)` is "ambiguous" to the C++ compiler and accepted here; `x =
  2.5;` (an assignment, not a declaration) through an `init(x)` conversion is
  refused by the C++ compiler (S8026) and accepted here.

**Not done in this stage's scope** (each also in its stage's list):

- the probes of section 12 as tests: Q3, Q4, Q6 (calls, operators) and Q14
  are cases; Q1, Q2, Q5, Q8, Q11, Q12, Q13 need constructs of stages 2 - 4
  (`try`, `throw`, `localized`, the start-up through srt_init, string
  templates for their printing) and are not cases yet;
- the verifier does not check register kinds or try records;
- ordinary programs started through `srt_init` (stage 4: srt_init itself):
  a normal `-o` build of any program stops with E0974 until then, because
  the start-up code uses constructs of stages 2 - 4. So "program start-up"
  is laid out, written and tested only as far as `-entry` reaches it, and
  the bodies the generator writes for `platform.static_init` / `tls_init`
  run for real only as the test entry's prologue;
- a test started with `-entry` ends an uncaught exception without srt_init's
  frame in the trace (the exception and its line are the same);
- the deep-nesting limit of analysis is unchanged (backend.md 10.1).

**For the next stage.** Generation is `value_of(g, e)` -> a slot; every new
node kind is one function in the package of its family plus a line in
`value_of` / `generate_statement`; selection already has every IR
instruction of 5.2 but the try regions, `lock`, `throw`, function values.
Write a case in tests/codegen/cases for each construct (its output block is
checked against the C++-built program too). `-listing` shows the code;
`-ir` the IR. A construct without a lowering must call `g.stop(...)`, never
emit something approximate. Analysis leaves the integer side of a decimal
operation unconverted (`d * 4`: the 4 is still an `_int32`), so operators
convert their operands themselves (`as_operand`, operators.sharp).

### 13.2 Expressions and calls

status: done (the gaps are listed at the end)

**What a compile does now.** Every node of the typed tree that is an
expression has a lowering - the table of 9.2 - except `bound_get_component`
(a component, stage 4) and what needs a closure record (a captured variable,
`self` inside a lambda, a lambda that captures: stage 4). Code generation of
the whole library (`-eager`) went from 3097 functions written as stubs to
1237, and every one of those 1237 waits for a STATEMENT of stage 3 (650
`lock`, 565 `throw`, 20 `try`, 2 `when`). 66 of the 177 suite cases run,
started at their `main` (below), and print `RESULT PASS` with and without
the JIT; the other 111 stop with E0974 at a construct of a later stage (75
`try`, 19 `throw`, 8 a lambda that captures, 6 `lock`, 2 `when`, 1 a
`localized` local).

**The packages.** Stage 1 left `backend/finalizer/generation` as one folder
of eight files, and this stage would have doubled it, so it is now nine
packages (section 0): the front door (`generation.sharp`, and
`expressions.sharp` with `value_of`, the dispatcher), `values`,
`operators`, `assignment`, `null_safety`, `calls`, `collections`,
`statements`, `setup`. The code of stage 1 was MOVED into them, function by
function, without change, except: `element_code` of the loops and
`element_code_of` of the arrays were the same function (one is left); the
call instruction is made in one place (`emit_call`, calls.sharp) for
`call_value`, string templates and properties; and `set_main_value` makes its
function value with `make_function_value` (calls/function_values.sharp).

**Built** (each lowering is the row of section 6 it names):

- values/fields.sharp: enum values as values (6.2: the value's static slot
  of its enum), `self` and `base` as values (`base->legs`).
- values/type_tests.sharp: `is` (6.8: `is ebx, x` on the object; a class, an
  interface, an array of a class, a number array; the constant when analysis
  knows the answer) and `sizeof` of an object (its kept fields) as well as of
  an array.
- values/conversions.sharp: a checked downcast to an array of a class (6.3).
- operators/logical.sharp: `? :` (one result slot written by both paths; a
  whole number made a decimal when the result is one).
- assignment/places.sharp: PROPERTIES as storage places: `+=`, `-=` ...,
  `++` and `--` on a field with a getter and / or a setter read through the
  getter and write through the setter, on the object evaluated once; the
  value is cut to an `_int8` ... property's type in the caller before the
  setter gets it, as every argument is (4.3).
- null_safety/: `?.` with its receivers and chains, `?:`, `!!` (6.5).
- calls/function_values.sharp: a function named as a value
  (`bound_function_reference`), a lambda that captures nothing (its record is
  null: the value is the same as a named function's), and the call of a
  function value: the new IR instruction `call_value` -> `pushl value; <the
  arguments>; calld n` (4.6), whose result is taken like any call's.
- collections/templates.sharp: string templates - the start, then each
  append called on the result of the one before (6.6).
- collections/dictionaries.sharp: dictionary literals (6.6).
- ir: two instructions, `is` (IR_IS) and `call_value` (IR_CALL_VALUE), with
  their selection and their -ir text; bytecode: `is` and `calld`; verify: the
  operand stack across a `calld` (5.7: `function_code.value_calls_returning`).

**Rules followed, each checked against the C++-built program** (the cases
below hold every one):

- numbers: the 64-bit value of `_int8` / `_uint8` arithmetic until it is
  stored (`a + b` with two `_int8` 100s prints 200; stored into an `_int8`,
  -56); wrap-around; `_uint64` and `_uint32` compared signed, as the VM
  compares; `%` with negative operands; `>>` arithmetic, shift counts of 64
  and more as the VM takes them; `/` and `**` on decimals; decimals to whole
  numbers. Stage 1's code was already right on all of it - the case
  number_types.sharp now holds it.
- a plain number `is` any number type: 1 (backend.md 12.4.2) - an analysis
  fix this stage needed (the library's `number<T>.parse` asks `number is
  decimal`).
- a cast or `is` to an array of a class checks the class (6.3, 6.8).
- `p?.x` with `x` a plain number is read as `p!!.x` (W1027; 6.5).

**The test entry.** `-entry main` now also accepts `main(args: string[])`
(finalizer.sharp, `find_entry_function`), so that the suite's programs can be
started the way the cases are. Its `args` is what the VM hands over, not
`string` objects (srt_init makes those); none of the running cases reads it.

**Tests** (tests/codegen.sh, the gates' codegen step: 28 checks after the first pass, up from 16; 30 after the independent check below):

- eleven new cases, each run JIT and `-nojit` and compared with its
  `// output:` block and with the C++-built program: null_safety (`?.`,
  chains, a `?.` statement, `?:`, `!!`, `? :`, `&&` / `||` and which side
  ran), type_tests (`is`, casts, `sizeof`), templates (every kind of piece,
  the order of the pieces, `+` / `==` on strings), properties, function_values
  (named functions, static functions, lambdas, values in locals, fields,
  arrays, results and arguments, `==`), enums_and_dictionaries, assignments
  (every storage with `=`, every `op=`, `++` / `--` as statements and values,
  narrowing at each store), number_types, members (hiding, `base->`, a
  template calling methods), operator_functions (operators of a class,
  `operator[]`, the library's `int` / `long`), fields (inherited, chained,
  static through an object, globals, `localized` numbers and objects);
- stops/throw.sharp replaces stops/template.sharp (templates are generated
  now; `throw` is the construct that must stop the compile);
- the suite started at `main` (5.8): the 66 cases listed in
  `tests/codegen/suite_running.txt`; every listed case must keep running.

**C++ compiler bugs met** (none of them changes what the bootstrap does;
each is left out of the cases with a comment that says so):

1. `show(b.width++)` / `show(++b.width)` - `++` of a property used as a
   VALUE - stops its code generator: "attempting to consume empty machine
   data!". (The bootstrap generates it: getter, add, setter, the old or the
   new value. No case can compare it with the C++-built program.)
2. `box.count++;` on a STATIC property stops it: "attempting to set unknown
   label!".
3. `box.count += 5;` on a static property (`get { return count; }`, `set v
   -> { count = v; }`, starting at 0) then reads 1, not 5 (G26).
4. `z?.follow() == null` with `z` null is 0: a `?.` that ends in a CALL does
   not give null when the receiver is null (G27). A chain of fields
   (`z?.next?.next`) is right.
5. `made().width += 3` on a property calls `made()` twice (G28).
6. `b.on_press = { -> show(55); };` - a lambda without parameters given to
   a field - stops its code generator: "attempt to add invalid type to exe
   file".
7. `more := (count + 1).value;` with `count : int` crashes the C++ compiler
   (a segmentation fault, every time).

Two differences of ANALYSIS met on the way, for the leftovers stages (not
changed here): `(a?.next)!!.value` is accepted by the bootstrap and refused
by the C++ parser ("expected `)`"); `base.legs + 1` is refused by the C++
compiler ("expected object type ...") - `base->legs` is the form both accept.

**Timing** (the gates' machine, the library with `-eager`, 11306 functions):
layout 0.75 s, generation 0.53 s, bytecode and verify 0.34 s, write 0.52 s -
the same as after stage 1; there is no new pass. The codegen step of the
gates takes about 3.5 minutes, most of it the suite (177 compiles with the
library, 4 at a time).

**Not done in this stage's scope:**

- calls of NATIVE functions are generated (`invoke_delegate f, n, 1`) and
  reach the VM's linker (item 8 below), but none has run to its end: that
  needs a native library, and the library has no native function;
- `++` / `--` of a property used as a value, `++` and `+=` on a static
  property cannot be compared with the C++-built program (bugs 1 - 3 above);
  they are checked against numbers worked out by hand instead
  (tests/codegen/own/properties_as_values.sharp);
- `localized` LOCAL variables (one suite case waits for them), captured
  variables, `self` / `base` inside a lambda, lambdas that capture: stage 4
  (they need the closure records and the fiber-local numbering of 6.12,
  6.17);
- `get<t>()` of a component: stage 4;
- the verifier does not check the operand of `is` (a class address for an
  object, a VM type for a number array: the word does not say which);
- the deep-nesting limit of analysis is unchanged.

**The independent check of this stage** (a second pass over the work above,
by experiment: about 600 lines of probe programs built by both compilers and
run with and without the JIT, every one of the 111 waiting suite cases built
with `-stubs` and run, and each VM exception - a bad cast, an index outside
an array, a null object, a null function value, `%` by zero, a native
function that cannot be linked - raised in both builds and compared). What it
found and changed:

1. `**` of two whole numbers was wrong: `show_decimal(n ** 2)` printed
   4.63209295423891e+18 for n = 7. Analysis types `n ** 2` as a whole number
   that is CALCULATED on decimals; the generator wrote the decimal result
   into the whole-number slot. Fixed in operators.sharp
   (`whole_result_of_decimals`: `dexp`, then `d2i`; 6.4).
2. Nothing in the build could have caught that, so the verifier got the check
   it was missing: `verify/slot_kinds.sharp` (5.7) - every IR instruction's
   slots are of the kind it works on. It runs on every function of every
   compile (about 60 ms on the library's 11306 functions) and found nothing
   else in the library or the suite.
3. `x is object`, `x is object[]` and `x as object[]` stopped with E0974;
   `n is string` and `thing is _int64` stopped too ("`is` on a plain
   number"). The C++ compiler was probed for each and they are generated now
   (6.3, 6.8; backend.md 12.4.3): number against non-number is the constant
   0; `object` / `object[]` is "is there an object"; the cast checks nothing.
4. `is` on a value that holds NULL differs from the C++-built program, which
   answers from the declared type (1). That was already so for `maybe_dog is
   animal` and was not written down; it is G29 now, and the new `object`
   tests follow the same rule (null is 0).
5. The step of a `for` and an expression statement each had their own copy of
   "run this expression for what it does"; it is one function now
   (`effect_of`, statements.sharp), so a `?.` chain or a function-value call
   as the step of a `for` is dropped the same way as in a statement.
6. bytecode/selection.sharp had grown to 525 lines; it is four files by what
   the sequences work on (section 0). No sequence changed.
7. Tests: `**` of whole numbers in cases/number_types.sharp; numbers against
   classes, `object` and `object[]` in cases/type_tests.sharp; and a new kind
   of case, `tests/codegen/own/` (5.8), for what the C++ compiler cannot
   check: own/properties_as_values.sharp (`++` of a property as a value,
   `++` / `+=` / `*=` on a static property, `made().width += 3` - bugs 1 - 3
   and G28 above, each number worked out by hand) and
   own/null_and_known_tests.sharp (G25, G27, G29). The codegen step is 30
   checks.
8. A native function HAS run now, as far as it can without a native library:
   `static native def ticks(scale: _int64) : _int64;` called from `main`
   raises `unsatisfied_link_error (native function could not be linked)` at
   the same line in both builds - the call reaches the VM's linker with the
   right function. (No case holds it: a case cannot end in a stack trace,
   whose addresses differ by design.)

Probed and found the same in both builds, so nothing changed: narrowing at
every store, argument and return for every width; `++` / `--` / `op=` on
locals, fields, statics, globals, elements and properties, as statements and
as values; decimals as conditions; the order of evaluation of every row of
6.0 once more with objects, arrays and strings; operator functions including
unary `-` / `!`, `operator=`, `+=` on `string` / `int` / `double` fields,
elements and statics; conversions through constructors (`int`, `double`,
`string`, a class with one parameter) in arguments, results, arrays and
ternaries; generic classes and functions; base calls two levels up; `?.`
chains through fields, calls, arrays and statics; `?:` chains; function
values in fields, statics, arrays, arguments and results.

What the check could NOT do: programs that use `list`, `string.at` or
anything else of the library that can `throw` do not build until stage 3, so
the library's own collections were only exercised by the 66 running suite
cases.

**For the next stage.** Every expression goes through `value_of`
(expressions.sharp); a call through `emit_call`. The statements live in
generation/statements; `try` / `catch` / `finally`, `throw`, `lock`, `when`
and jumps out of a `finally` or a `lock` are what stops the library (the
numbers above) and the suite. When a statement makes more suite cases run,
add them to tests/codegen/suite_running.txt (codegen.sh names them). A value
call and a `?.` used as statements are handled in
`generate_expression_statement` (they may give nothing).

### 13.3 Statements and exceptions

status: done (the gaps are listed at the end)

**What a compile does now.** Every statement of the typed tree has a
lowering - the table of 9.3: blocks, locals with their start values and
default objects, expression statements, `if`, `when` (with and without a
subject), `while`, `do`, `for`, `foreach` (arrays and `loopable` objects),
`break`, `continue`, `return` with and without a value, `throw`, `try` /
`catch` / `finally` with nesting and every jump that LEAVES one, `lock`, and
`asm` blocks with every kind of operand. The consequences:

- the whole library with `-eager` (11305 functions) has NO stub any more -
  1237 before this stage, every one of them waiting for a statement;
- `platform.srt_init` is generated, so an ordinary program - no `-entry` -
  starts the way the C++-built one does: the VM runs srt_init, which sets up
  the platform and the main thread and calls `main` (stage 4 was planned to
  get here; nothing of its own was needed for it);
- of the 177 suite cases, **164 compile and print `RESULT PASS`** as normal
  programs, with the JIT and with `-nojit`. The other 13 stop with E0974 at a
  construct of stage 4 and nothing else:
  - 12 have a function whose variables a lambda captures (closure records,
    6.12): compound_assign_methods, lambdas_capture_per_call,
    lambdas_closures, lib_collections_functional, lib_coroutines_channels,
    lib_coroutines_closure_across_fibers, lib_coroutines_core,
    lib_coroutines_sync, lib_math_lambda_concat, lib_util_holders,
    lib_util_ordering, trailing_lambdas;
  - 1 has a `localized` local variable (6.17): variables.

  None of the 13 waits for anything else (a generic or a component of its
  own included: those cases run).

**Built** (each lowering is the row of section 6 it names):

- generation/statements/branches.sharp: `when` (6.10) - the subject once into
  its hidden local, each clause's tests left to right with `jump_if` to its
  body, the first clause that holds and no other, `else`.
- generation/statements/exceptions.sharp: `try` / `catch` / `finally` laid out
  as 6.11 says (the `nop` before it, the closing jump inside each range, the
  finally copy the VM enters, the normal copy, a stub per way out) and
  `throw` (with the `cast` that turns a null into nullptr_exception, G15).
- generation/statements/exits.sharp: the ways out (6.11). A `return`,
  `break` or `continue` walks its `leaves`: `unlock` for each lock, and for
  the first try with a finally a jump to that try's stub for this way out
  (`try_exits.stub_label`: one stub per kind of jump and target, made by the
  first jump that asks); the stub is the finally block, then the rest of the
  same walk; past the last one, the final jump. A returned value is computed
  first and kept in the function's hidden return slot
  (`generator.return_slot_for`).
- generation/statements/locks.sharp: `lock` (the object once into its hidden
  local, `lock`, the body, `unlock`).
- ir: `throw`, `lock`, `unlock` (IR_THROW, IR_LOCK, IR_UNLOCK); `ir_try` - a
  try's ranges as LABELS (5.2), made by `ir_builder.new_try` in the order the
  statements start; `ir_builder.finally_exception_slot` (never a number whose
  low byte is 05: 11.1); the -ir text of both.
- bytecode: their selection (`pushl; throw`, `movl; lock`, `movl; ulock`) and
  the try records read off the label pcs after the first pass
  (`try_record_of`; an end label is one past its range).
- function_code: `try_record` (3.8); output: `write_try_records`; listing:
  each function's try records.
- verify/try_records.sharp (5.7): ranges that nest, lie on whole
  instructions and inside the code, handlers and finally on instructions,
  slots inside the frame, a finally slot whose low byte is not 05, the classes
  of the clauses, R1 and R2 of 4.7. stack_heights.sharp starts every handler
  and finally block at height 0; slot_kinds.sharp checks that catch and
  finally slots hold references.

**Rules followed, each checked against the C++-built program** (probes of
about 500 lines and the cases below; JIT and `-nojit`):

- a clause takes its class and every class below it; the clauses of the
  inner try are asked before the outer; `catch(throwable)` takes everything;
  a clause without a variable;
- an exception in a catch handler goes to that try's finally and then to the
  handlers around it, never to the clauses of its own try; an exception
  thrown by a finally replaces the one it was running for; `throw e;` in a
  handler raises the same object;
- the finally runs on every way out - the end, an exception, `return`,
  `break`, `continue` - and the ways out run every finally they pass,
  innermost first, unlocking the locks between them in nesting order;
- `return x;` through a finally that changes `x` returns the OLD x (the value
  is computed before any finally runs), for whole numbers, decimals and
  objects;
- an exception several frames deep runs each frame's finally as it passes;
- a function that catches an exception in each of 1000 rounds of a loop
  runs (the VM leaves the failing call's values on the operand stack: 4.7);
- the exceptions the VM raises itself are caught by the right clause: a null
  object (`!!`, a field), `%` by zero, a cast that does not hold, an index
  outside an array, `lock` of null;
- locks: re-entered by the same fiber, left by `continue`, `return` from
  `lock(self)` and from a lock of a static object, released by the VM when an
  exception leaves the block (the next `lock` of the object does not wait);
- `when`: the subject evaluated once, the tests tried left to right and only
  until one holds, `operator==` for a `string` subject, decimals, a clause
  with `continue` / `break`, `when` without `else`;
- exceptions thrown and caught in a function the JIT compiled (a hot loop
  with try / catch / finally around a call that throws);
- the uncaught exception's report - the exception line and the source line of
  every frame - is the C++-built program's (only the addresses differ).

**Decisions of this stage.**

- The exit stubs of 6.11 (G2) and the `nop` before every try (G14) are built
  as designed. own/exception_rules.sharp holds the three places where the
  C++-built program is wrong: a try right after a call takes that call's
  exception (G14, 11.4); `throw` of null crashes the VM (G15); a finally
  whose exception slot is 5 makes a file the VM will not load (11.1, G12) -
  all three reproduced again in this stage.
- `throw` of a value that may be null is checked with `cast` to the value's
  class, or the first class above it that the program keeps (not always
  `throwable`, as 6.11 first said: any class above works, and the value's own
  is the obvious one).
- A field written with a type parameter (`value : T;`) gets no default
  object in any instance (G23, narrowed; backend.md 7.3 and the
  `default_constructor` row). The library's `deferred_init<T>` tests such a
  field against null to know whether its value has been computed; with a
  default `string` it never computed it, and the suite case std_misc_fixes
  failed. The C++-built program gives the field null. The change is in
  analysis (`set_default_object`, backend/compiler/initializers).
- The gates' suite step builds every case as a normal program (5.8): with
  `-entry main` two cases could not pass that pass started at srt_init
  (arrays_objects reads `sizeof(args)`, lib_system_process the platform's
  names, both set up by srt_init).

**Tests** (tests/codegen.sh, the gates' codegen step: 39 checks, up from 30; the last four are the verifier's, below):

- four new cases, each run JIT and `-nojit` and compared with its
  `// output:` block and with the C++-built program: exceptions (every rule of
  the first five bullets above, the VM's own exceptions), exits (every way
  out through finally blocks and locks, in `for`, `while(true)`, `do`,
  `foreach` over an array and over a list, from a `when` clause, from an
  endless loop; `lock(self)`, a static lock, re-entry, a lock an exception
  left), branches (`when` in every form, `if` chains), assembly_operands (the
  operands that name something: a text, a class, a field, a static field, a
  function by its name, through `$m#__srt_global` and `$__srt_global`);
- own/exception_rules.sharp (G14, G15, 11.1, above);
- stops/captured_variable.sharp replaces stops/throw.sharp (`throw` is
  generated now; a lambda that captures is what must stop the compile);
- the library check is stronger: no stub, and the file runs to its end with
  the JIT and with `-nojit`, printing nothing, exit code 0; the program a
  `main` of nothing reaches the same, compiled without `-stubs`;
- the suite step: 164 cases in tests/codegen/suite_running.txt (66 before).

**Differences of analysis met on the way** (for the leftovers stages; not
changed here): the C++ compiler refuses a `break` or `continue` inside a
loop that is itself inside a finally block ("control cannot leave body of
finally clause"), although such a jump does not leave the finally; the
bootstrap accepts it and the program does what it says (probed: the loop
stops at the `break`, then the code after the finally runs).

**Timing** (the gates' machine, the library with `-eager`, 11305
functions): layout 0.60 s, generation 0.53 s, bytecode and verify 0.45 s
(0.34 s before: 1237 more functions have real code now), write 0.53 s.

**The verifier's pass** (an independent agent went through the stage after
it was reported done; what it found and changed):

- *Chains ran the compiler out of stack.* The generator's walk called itself
  once per link of a chain, and the VM's stack is small (11.8): a sum of 200
  terms, a ladder of 600 `else if`, 300 `||`, 300 `? :` or a text put
  together from 300 pieces ended the COMPILER in an uncaught
  `stack_overflow_exception` - programs the C++ compiler builds and analysis
  accepts (backend.md D103 promises chains of any length). Each chain is now
  followed in a loop, writing the same instructions in the same order:
  `binary_value` and `logical_value` (generation/operators) down the left
  side, `ternary_value` down the "else" side, `call_value`
  (generation/calls) along the objects of a call chain, `generate_if`
  (statements/branches.sharp) along the else parts. 1000 links of each pass
  (cases/chains.sharp holds 300 of each).
- *Nesting too deep is a message, not a crash.* Real nesting (blocks in
  blocks, calls in arguments of calls) still costs a level each. Where the
  walk runs out of stack, `generate_function` catches the
  `stack_overflow_exception` and the function is reported: `E0978`, "the body
  of `f` is nested too deeply for this compiler to generate code for", at the
  function, with the same note and help as analysis's `E0992`. It is never
  written as a stub. Analysis and the parser stop earlier in most shapes
  (E0992, E0103); E0978 is for a body that passes them and is too deep here,
  where frames are larger.
- *`throw` of a null the type does not show crashed the VM* (G15 covered only
  values of a `?` type): `throw e!!;`, a local `e : oops;` never given a
  value, a field never set. Every thrown value that is not a `new` object is
  tested now (6.11); own/exception_rules.sharp has the three shapes.
- *Jumps out of a lock were tested by taking the lock again in the same
  fiber* - which succeeds whether or not the jump unlocked, because a fiber
  may enter a lock it holds. The generator was right; the test proved
  nothing. cases/lock_exits.sharp asks from ANOTHER fiber after every kind of
  jump (return, break, continue, through finally blocks, from a catch
  handler, from `foreach`, from `lock(self)`).
- *Three differences from the C++-built program nobody had seen*, all kept
  and written down: G30 (a finally block passed by a jump runs with the
  inner locks already given back), G31 (`continue` in a `do` loop goes to
  the condition) and G32 (the object of a `lock` is evaluated once).
  own/jump_rules.sharp states the three rules.
- *Measured, not changed:* the operand stack a handler inherits (4.7, 11.3).
  A function that catches an exception out of a call in every round of ONE
  loop runs out of the fiber's 4096 slots after about 4000 rounds (one slot a
  round: the failed call's result slot); from then on every round raises
  `stack_overflow_exception` instead. The C++-built program runs out sooner
  (before 3500 rounds, it leaves more behind). The VM has no instruction that
  sets `sp` back, so code generation cannot repair it; it is a VM matter for
  the owner (a server loop that catches would hit it).
- New cases: cases/lock_exits.sharp, cases/chains.sharp,
  cases/loops_and_locals.sharp (locals without a value in a loop, every form
  of `for`, `foreach` over each kind of array, a null and an empty one, a
  list and the characters of a text; `when` on an enum, a decimal, an object
  with `operator==`, nested), own/jump_rules.sharp.
- Probed and found the same as the C++-built program, JIT and `-nojit`
  (about 900 lines, `tmp/v3` of the build's scratch folder): a return from a
  catch handler through two finally blocks; a returned expression that
  throws inside its own try; `_int8` / `_uint16` results through a finally;
  a finally that throws, and one that catches, while a return is on its way;
  break / continue of inner and outer loops from nested tries; a clause for a
  class no object is made of; an exception thrown by an exit stub's finally
  copy; conditions and steps of loops that throw; try in a constructor, an
  `init` block, a static field's value, a property getter, an operator, a
  generic class and a lambda that captures nothing; 258 locals (hidden slots
  around 261, whose low byte is 05); 20000 rounds of try / catch / finally
  with break, continue and exceptions in a function the JIT compiles; asm
  operands naming a field and a function of the class the code is in; the
  source lines of the frames of an uncaught exception that passed finally
  blocks and a lock.
- *The C++ compiler as an oracle that is not always the same.* Three full
  gates runs of this pass failed once each on the C++ compiler alone, with
  the bootstrap's side right every time: it died without a message on a help
  test's program (tests/helps.sh), and its build of
  cases/enums_and_dictionaries.sharp once printed `green` for an enum value
  that is `blue` (eight builds of the same file alone were right; it numbers
  unfixed enum values from a randomly offset counter, G7). Both scripts now
  run the C++ compiler ONCE more in exactly that situation and print a
  `note:` line saying so - no file and no error message (helps.sh), a
  C++-built program that does not print the case's block (codegen.sh). What
  must hold is unchanged: the C++ compiler builds the program, and its
  program prints the block; the bootstrap's own runs are never repeated.
- Seen and left alone: for a statement that spans lines, the bootstrap's
  line table names the line of the CALL (`total +=` / `fail_here(i);` gives
  the second line, the C++-built program the first), and constructor frames
  get a line where the C++-built program prints `?` - both of stage 1 and 2's
  line marks (6.17), more exact, and only in stack traces.

**Not done in this stage's scope:**

- a catch variable that a lambda captures, and a `try` / `lock` inside a
  lambda that captures, wait for the closure records of stage 4 (the
  function stops with E0974 as a whole);
- the verifier (verify/) does not prove that every jump out of a lock
  unlocks it (the generator does it by construction, and
  cases/lock_exits.sharp asks a second fiber after each kind of jump);
- nesting has a limit (E0978, above; analysis: E0992): about 40 levels of
  `try { while { if {` one inside the other pass, 60 do not - the C++
  compiler takes more. Chains have none;
- `lock` of an object that another FIBER holds, contended: covered by the
  suite's lock cases (lock, lock_exception_and_nesting, lock_release_rules,
  threads), not by a codegen case of its own - a case cannot make the timing
  certain.

### 13.4 Objects, lambdas, the rest

status: done (the gaps are listed at the end)

**Where it stands.** Every node of the typed tree has a lowering, and every
row of the inventory (section 9) is generated: all 177 suite cases are
compiled by the bootstrap as normal programs and print `RESULT PASS`, with
the JIT and with `-nojit` (164 before this stage: the other 13 waited for
closure records and `localized` locals). The whole library compiles with
`-eager` into a file the VM runs to its end, also with `-obf`. The test
options of the first stages are gone (`-entry`, `-stubs`, E0974, E0976,
E0977; 5.8): a node the generator meets without code is now an internal
error, E0970.

Much of this row of 2.1 was built earlier than planned and is only checked
here: `init<C>`, `init { }` and `init(x)` (stage 1 - 2, 6.14), `static_init<C>`
with its flag and the enum values (stage 1 - 3, 6.15), the start-up
(`platform.static_init`, `tls_init`, the `main` value: generated since stage
3, run by every suite case), generic instances and functions, interfaces,
boxed numbers and the classes the VM names (stage 1 - 2), `localized` fields
(stage 2), `-dbg` (stage 1). What this stage built:

**Closure records** (6.12; new package generation/closures: closures.sharp,
records.sharp, variables.sharp)

- *Layout* (layout/synthesized.sharp `make_closure_class`, layout/layout.sharp
  `add_closure_class`): every kept function and field with a
  `closure_record` gets a class `closure_<full name>` (`@2`, `@3` ... for a
  second record of the same name), laid out after every other class: a field
  per captured variable (`local_variable.storage`; a second variable of one
  name gets `name@2`), `@parent`, `@self`. No base, no functions, no static
  part.
- *The frame* (generation.sharp `lay_out_frame`): `self`, the parameters, a
  lambda's hidden record parameter (`calld` pushes it right after the written
  ones; `function_layout.parameter_count` counts it and the function record
  lists its type), the other locals - a captured local takes NO slot, a
  captured parameter keeps the one it arrives in - and the slot of the
  function's own record.
- *The prologue* (`open_own_record`): `new closure_<f>`, `@parent` = the
  record parameter, `@self` = slot 0, every captured parameter copied in -
  before anything else, because a constructor's `-> base(x)` may already read
  a captured `x`. An `init { }` / `static init { }` block generated in place
  makes its record where it starts (`open_block_record`); the record of a
  FIELD whose value holds a lambda using `self` is made by init<C> right
  before that value (`open_field_record`).
- *Uses* (variables.sharp): a captured local at depth 0 is a field of the
  frame's own record, at depth d the record parameter followed by `@parent`
  d - 1 times (records.sharp, the textbook's access links); `self` / `base`
  inside a lambda is `@self` of the record at its depth. Reads, stores
  (declarations, `=`, `op=`, `++`, `foreach` items) and storage places all go
  through one place, so the three homes of a variable - its frame slot, its
  record, its fiber - are decided once. A captured `catch` variable: the VM
  stores the caught object into a frame slot (4.7), so the handler catches it
  in a slot of its own and moves it into the record first thing.
- *The value* (calls/function_values.sharp `lambda_value`): the function value
  of a lambda that captures carries its record in `captured`; the record is
  found among the frame's own or along the chain (`record_holding`). The two
  facts must agree - `calld` pushes a record only when the value carries one -
  and the generator stops with E0970 if analysis ever said otherwise.

**`localized` local variables** (6.17): each gets a fiber-local slot of its
own after the `localized` fields (layout.sharp; `make_fiber_local_field`), is
read and written like a `localized` field, and gets its value where it is
declared, every time the declaration runs - as the C++ compiler does
(`local_variable_decl.cpp`, `field_generator.cpp`; probed: a second fiber has
its own).

**Components** (6.16; generation/setup/components.sharp, layout): analysis now
records which declarations receive each definition's value
(`type_definition.requested_by`, filled by program/usage `keep_uses`); every
definition some kept code receives is kept, and gets `@sub_component_init_<n>()`
in its module's globals class - for a `single` also `@sub_component_<n>` and
`@sub_component_init_flag_<n>`. `get<t>()`, an injected field (in init<C> or
static_init<C>) and an injected local are calls of it. A definition no kept
code asks for is not in the file and its value never runs (as in the C++-built
program: cases/components.sharp).

**Obfuscation** (6.17; new package backend/finalizer/obfuscation:
obfuscation.sharp, modifiers.sharp, names.sharp; option `-obf`): after layout,
the names the file carries are replaced where the default (`-obf`: everything)
and the rules of the `obfuscate` blocks say so, the last rule naming a
declaration winning; classes (and the module part of their full names),
fields, functions, and with `-obf` the source files. The map back is written
to `<output>.names`. Never renamed: the classes the VM looks up by name with
all their fields and the classes above them, and native functions with their
classes; a rule that asks for one of them is `E0971` (new). Enum values keep
their names.

**Smaller things**

- a cast to an array of function values (`copy(...) as T[]` in
  `list<()(int)>`, which analysis accepts and the C++ compiler refuses) checks
  nothing, as for `object[]`; a cast to one function value checks
  `function_value` (values/conversions.sharp);
- `base->f()` inside a lambda passes the object kept in the record
  (calls/calls.sharp).

**Tests** (codegen step: 39 checks before, 46 now)

- cases/closures.sharp (closure records against the C++-built program: counters,
  captured parameters, three lambdas deep, recursion, `foreach` items, `self`
  one and two lambdas deep, an `init { }` block, a constructor),
  cases/components.sharp, cases/localized_locals.sharp (two fibers);
- own/closure_rules.sharp (captured numbers of every kind, `_int8` cut to its
  width, a field value using `self`, a static init block, a captured `catch`
  variable, `base->` in a lambda, try / lock inside a capturing lambda,
  lambdas kept in a list, recursion; the C++ compiler refuses or miscompiles
  each); own/extensions.sharp (extension functions - of a class, of `string`,
  of one generic instance, of `int` - and `mutate`: the C++ compiler refuses
  an extension function written at the top of a file, "unknown ast type: 16";
  `mutate` alone it builds the same, probed);
- obfuscation/renaming.sharp (`-obf`: the same output, the `.names` map);
- refused/renaming_vm_names.sharp (E0971 twice, no file);
- codegen.sh: cases and own programs are normal builds now; the library is
  compiled without `-stubs` and once more with `-obf`; every suite case must
  run (suite_running.txt lists all 177);
- the stops/ folder is gone: its one program (a captured number) runs in
  own/closure_rules.sharp.

**Found and written down** (section 10):

- G33: `base->f()` inside a lambda runs the object's own `f` in the C++-built
  program; here the base class's, as outside a lambda;
- G34: a captured `catch` variable is null in the C++-built program
  (`nullptr_exception` when the lambda reads it);
- G35: a member a rule keeps (`obfuscate -keep { point.y }`) keeps its name
  under `-obf` here; the C++ compiler renames it anyway;
- G36: obfuscated names are numbered in a fixed order and spelled as
  spreadsheet columns (`z`, `aa`, `ab`); the C++ compiler's come from random
  ids and repeat a spelling (26 gives `az`, 27 `aa`).

**Checked against the C++-built program** (tmp/s4 of the build's scratch
folder, JIT and `-nojit`): the 13 suite cases this stage unlocked; closure
probes (p1); components (p2); `localized` locals across fibers (p3); the same
three and lambdas_closures, lib_coroutines_core again with `-obf` on the
bootstrap's side (identical output); a non-static native function (the
bootstrap's program raises `unsatisfied_link_error`, the C++-built one
"delegate function has no subscribers": neither has a library to link).

**Timing.** The library with `-eager` (11305 functions, 293 classes): layout
0.61 s, generation 0.53 s, bytecode 0.79 s, write 0.19 s - the same as
before this stage within the noise of a shared machine. Obfuscation returns at
once when nothing renames (the library's own blocks only keep); with `-obf`
the program a `main` of nothing reaches has 146 names renamed.

**Not done in this stage's scope:**

- the native glue files (`-nativedir`: `function_mapping.h`,
  `native_mapping.cpp`, `function_mapping.cpp`) are C++ source for building a
  native library, not something the VM reads; they stay with the driver
  (8.2, stage 7). Native functions themselves are written and called (static
  ones; the library has none);
- a captured variable that is also `localized` lives in its fiber (the
  record's field for it is never used); analysis allows the pair, nothing in
  the library or the suite has it;
- the verifier does not check that a lambda value's record matches what its
  lambda takes (the generator checks it, `record_for_lambda`);
- obfuscation does not rename enum values, and keeps every field of a class
  the VM names - also fields the VM never reads;
- the line table and frame sizes are unchanged: G24 (frames larger than the
  C++ build's) still costs recursion depth.

**The verifier's pass over this stage** (an independent second agent; what it
checked and what it changed):

- *Checked by experiment* (scratch folder tmp/v4): programs written to stress
  this stage, built with both compilers where the C++ compiler can build
  them and compared (output, exit code, JIT and `-nojit`), by hand where it
  cannot (it refuses or crashes on a captured number, a captured function
  value, a lambda in a field value using `self`, nested function types,
  several `obfuscate` rules at once). Same output for: the start-up order of
  statics, globals, field values, `init { }` blocks, implied and written base
  calls, `init(x)` conversions, nested classes (a_objects); interfaces,
  enums with and without numbers and inside a class, `when` on an enum,
  generic classes and functions, boxed numbers, `is` / `as` (c_types);
  components with injected instance, static and local variables, `localized`
  globals and class fields across fibers, `launch` / `async`, `mutate`, an
  uncaught exception's trace (d_rest - the trace differs only in line
  numbers the C++ build prints as `?`). By hand: lambdas in constructors,
  getters, setters, operators, extension functions, generic functions and a
  static init block; captured numbers of every width; two variables of one
  name in one function; a variable three lambdas out; a captured parameter
  written by its lambda; lambdas that make lambdas; a captured loop
  variable used from 20 fibers; a variable both captured and `localized`
  (b_lambdas, e_own, f_gaps). All of them again with `-obf`, and c_types
  with five `obfuscate` rules of every form: the same output. Nothing the
  generator does wrongly was found.
- *Closed*: `[x]` in an `asm` block naming a local a lambda captures, or a
  `localized` local, was an internal error (E0970) from the generator. It is
  now a rule of analysis with a message of its own, `E0925` (backend.md
  7.21, D107; tests/backend/fail/lambdas.sharp).
- *Readability*: `lay_out_program` was one function of 206 lines. It is now
  six named steps (layout.sharp for the classes, the new
  program_functions.sharp for the functions); what it does is unchanged.
- *Tests added*: own/closure_rules.sharp also has two captured variables of
  one name, a local both captured and `localized`, and lambdas in a getter
  and an operator function; fail/lambdas.sharp has the three forms of E0925.
- *Nothing weakened*: the checks of gates.sh are as they were (one line of
  its printed description still spoke of "not generated yet" and was
  corrected); codegen.sh only got stricter (a suite case may no longer
  wait); the removed stops/ case runs in own/closure_rules.sharp.
- *A second flake of the C++ compiler* showed in the help tests
  (tests/helps.sh, step 4): under load it once refused the standard library
  itself (`loopable.sharp:4:29: could not resolve symbol t`, and six errors
  that follow from it) for a program it builds when run again. helps.sh
  already ran the C++ compiler once more after a crash; it now does the
  same when every error it reports is in a library file - such an error
  cannot come from the pasted help. An error naming the program is never
  retried, and a second failure fails the test.

### 13.5 Whole suite green

status: done (the gaps are listed at the end)

**Where it stands.** Every one of the 177 suite cases, compiled by the
bootstrap compiler with the standard library, runs on the VM and prints
`RESULT PASS`, with the JIT and with `-nojit` - judged by the suite's own
runner exactly as `tools/suite.sh` judges the C++ build. On the runtime built
with the kind checker (`-DSHARP_TYPE_CHECK`) all 177 run clean: not one
`[type check]` report. The repository's other programs - the kostya
benchmarks and the examples - build and print the same as the C++-built
ones. Stage 4 had already made every case print `RESULT PASS` through
`tests/codegen.sh`; what this stage found was in the DRIVER, not in code
generation: nothing in the suite, the benchmarks or the examples showed a
difference in what the generated programs do.

**What the runner needed** (main.sharp). The runner calls its compiler as
`<sharpc> <case>.sharp testing.sharp -o <exe> -j 1 -L <library>`, and so does
the benchmark script. Both options were broken in the bootstrap:

- `-j <n>` was "invalid option" - the compile stopped before it began. It now
  sets the parallelism of the worker pool `parse_all` runs on
  (`dispatchers.set_parallelism`), as the C++ compiler's `set_target_threads`
  sets its threads; a value that is not a whole number of at least 1 is an
  error.
- `-L <folder>` read one folder level, and as bare names (`file.list()`), so
  every library file was then "file `x.sharp` doesnt exist!". It now takes
  every `.sharp` file below the folder at any depth, as absolute paths,
  sorted, so the files come in the same order on every compile
  (`sharp_files_below`, new file source_folders.sharp; the project mode's
  folder is read by the same function).

**Compilers side by side** (found by the verifier of this stage). The first
version of `-L` listed the folders with the library's `file.walk()`. That
function lists each folder by running `ls` with its output sent to a
temporary file, and names the file after the clock (milliseconds) and a
counter that starts at the same number in every program
(`std/io/files/temp_files.sharp`, `new_path_in`: "is the name free?" is asked
before the file exists). Two compilers started in the same millisecond pick
the same name and read each other's listing. Measured, 8 compiles started at
once, 6 rounds: **18 of 48 wrote no file** - "file
`.../stdlib/std/data/csv/critical_section.sharp` doesnt exist!" (a name from
one folder under the path of another) - and some of them had printed
"parsed 260 file(s)" where the program has 271: library files silently
missing. The gates did not see it because the runner compiles one case at a
time; a build tool, or the editor checking a file while a build runs, starts
compilers side by side. Now the driver makes ONE call of the runtime
(`file_natives.tree_files`, interrupt 0x2a - `get_file_list` in
lib/Modules/std.io/fileio.cpp, the very function the C++ compiler's
`File::list` is): no process, no temporary file, and the same rule as the C++
compiler - a name that starts with a dot is skipped, a file or a whole folder
(`.git`, `.idea`; `walk()` took them). The runtime gives the paths in the
disk's order, so they are sorted. The race itself is in the standard library
and is still there for every program that lists folders with `file.walk()`
or `file.list()`; the library is not this build's to change, so it is in
this stage's report as a finding.

**The driver's own test** (new `tests/driver.sh`, a gates step, under a
minute): a small library of its own next to the standard one - files in the
folder, one and two folders down, a folder NAMED `folder.sharp`, a text
file, a broken file in `.hidden/` - must build and print the sum of the
three values; a trailing `/`, the same file named on the command line, a
folder given twice and a folder that does not exist (a warning) change
nothing; `-j 4` is accepted and `abc`, `0`, `-3`, `1.5` and a missing number
are refused by name; two compiles write the same bytes; and 4 compiles
started at the same moment, three times over, each write the bytes a compile
alone writes.

**The suite through its runner** (new `tests/suite_runner.sh`, a gates step).
It writes a two-line wrapper (`exec sharp sharpc_boot "$@"`) and hands it to
the runner as `<sharpc>`; the runner itself (`tests/suite/run.sharp`) is built
by the C++ compiler, so the judge is not the thing judged. Configurations
`plain` and `plain-nojit`, as the first two of suite.sh (`-O` is stage 6).
They run ONE AFTER THE OTHER: the first try ran them side by side and two
cases failed, `lib_io_tree` and `lib_data_json`, because each works in a fixed
folder under `/tmp` (`/tmp/sharp_suite_io_tree`, `/tmp/sharp_suite_io_json`)
and two runs at once deleted each other's files; alone each passes, six runs
of six. Crashes are counted on their own line - a program that died on a
signal, and a compile in which the bootstrap itself died - and are failures
like any other, never re-run until they pass (the runtime stage fixed the one
intermittent crash; a new one is a runtime bug).

**The kind check** (new `tests/kindcheck.sh`, a gates step): every executable
the runner built (`plain`) is run with `-nojit` on
`~/sharp_work/sharp_tc_dev/sharp`, a runtime built from the repository's
lib/runtime with `-DSHARP_TYPE_CHECK` (the old `~/sharp_work/sharp_tc` was
built before the runtime stage's fix). Each must print no `[type check]` line
and still pass. Result: 177 clean. Two things make its silence worth
something, and the script does both before it runs a case: when a source file
of the runtime is newer than the kind-check runtime it builds that runtime
again (an incremental build), so the check is never of an older VM; and it
builds and runs `tests/kindcheck_probe.sharp`, an `asm` block that loads a
decimal local and adds it as a whole number, which MUST give
`[type check] main#__srt_global.main pc 5 (opcode 0x10): register ebx holds a
decimal, an integer is needed` - a runtime built without the flag fails the
step instead of passing everything. This is the run-time side of the
verifier's slot-kind check (5.7), and it agrees with it.

**The repository's programs** (new `tests/programs.sh`, a gates step). Built
by both compilers the way the benchmark script builds (`-j 1 -L`), run with
small inputs, JIT and `-nojit`, and the output AND the exit code compared
with the C++-built program's, the times the programs measure taken out:

- kostya: brainfuck (a hello-world program written by the script: `bench.b`
  takes 15 s), base64, matmul, primes, json on a file `generate_json` wrote;
  `generate_json`'s own file must be the same bytes from both builds;
- examples/: `console_args` and `console_command` run the same;
  `console_input` is built and not run (it reads a password at a terminal:
  both builds wait there forever when the input is a pipe); the other twelve
  (`atomic`, `factorial`, `file_copy`, `function_timer`, `helloworld`,
  `list`, `new_class`, `random`, `string_to_int`, `task`, `thread`,
  `xor_neural_net`) are written for an older library or grammar and the C++
  compiler refuses every one of them - the check is that the bootstrap
  refuses each of them too (it does, with its own messages);
- benchmark/PerformanceTest1-14 and benchmark/forceField are in the old
  Java-like grammar (`def main(string[] args)`, `import std;`): the C++
  compiler refuses all of them, so there is nothing to compare; they are not
  in the script.

**Full-size run** (the kostya inputs of its README, the JIT, both builds run
one after the other on the same runtime `sharp_dev`, the same machine shared
with the other stream - one run each, so a few percent is noise). The
checked output is the same in every row; seconds are what the program
measures around its work.

| test | input | C++-built | bootstrap-built | peak memory C++ / bootstrap |
|---|---|---|---|---|
| brainfuck `bench.b` | - | 13.8 s | 14.3 s | 12 / 6 MB |
| brainfuck `mandel.b` | - | 148.2 s | 156.2 s | 13 / 7 MB |
| base64 | 131072 bytes x 8192 | 28.5 s | 20.4 s | 21 / 14 MB |
| json | 131072 entries | 8.5 s | 7.7 s | 837 / 868 MB |
| matmul | 1500 x 1500 | 21.9 s | 19.4 s | 80 / 74 MB |
| primes | up to 5000000 | 4.3 s | 4.5 s | 677 / 629 MB |

So, without `-O` on either side, the bootstrap's programs are within about 5 %
of the C++ compiler's on brainfuck and primes, and faster on base64, json and
matmul (the C++ code generator spends instructions the IR's slot reuse does
not; 7.3 plans registers for the rest). Nothing here is slower by more than
the noise of one run, except mandel.b at 5 %.

**File size and compile time.** The bootstrap's executables are much smaller:
it writes only what the program reaches from srt_init and main (5.5), where
the C++ compiler writes nearly the whole library.

| program | C++ file | bootstrap file | C++ compile | bootstrap compile |
|---|---|---|---|---|
| suite case arithmetic | 566 KB | 67 KB | 0.80 s | 2.62 s |
| suite case lib_data_json | 677 KB | 261 KB | 0.78 s | 2.71 s |
| kostya json | 596 KB | 150 KB | 0.69 s | 2.62 s |
| kostya bf / base64 / matmul / primes | 583 / 571 / 563 / 579 KB | 89 / 96 / 81 / 110 KB | | |
| all 177 suite cases | about 100 MB (estimated: 177 x 570 KB, not all built) | 17.5 MB (99 KB each on average) | | |

A compile takes the bootstrap about 3.3 times as long as the C++ compiler (the
bootstrap runs on the VM, built without `-O`; its parse cache was warm). The
whole compile of a suite case - library included - is under 3 s.

**Gates** (four new steps after the codegen step, ~/sharp_work/tools/gates.sh;
backups gates.sh.before_suite_green and, for the verifier's step,
gates.sh.before_s5_verify; nothing removed or loosened): the suite through
its runner, the kind check on what it built, the repository's programs, the
driver tests. They add about 20 minutes (the runner 18, the kind check
under 20 s with its probe, the programs a minute, the driver tests under a
minute). The full gates at the end of this stage: suite runner `ALL 177 TESTS
PASSED` both ways, 0 crashes; kind check 177 clean; programs 21 passed;
driver tests 15 passed; codegen 46 passed; `GATES: PASS`.

**What the verifier of this stage tried** (scratch programs, both compilers,
JIT and `-nojit`, output and exit code compared; stack traces compared
without their addresses):

- six programs written to stress what the suite does not: number types at
  their edges (wrap of every width, `%` and `/` by zero, decimals to whole
  numbers, compound assignment on fields, statics and array elements);
  `break` / `continue` / `return` through nested try / finally in loops,
  `when` in all its forms, an exception nobody catches (the same trace, the
  same exit code); interfaces, overrides, operators, generics, properties, a
  cast that fails, an index out of range that nobody catches; lambdas made
  in loops, kept in fields and arrays, calling each other, a stack overflow
  that is caught, `main`'s result and its arguments; static parts and
  globals in order, a tree walked by a lambda, coroutines, sorting. Nothing
  differed except what section 10 already lists: the recursion depth (G24,
  11.8: `depth(3000)` runs in the C++-built program and overflows here) and
  whole numbers above 32 bits in a text (`"${a + 1}"` - the C++ compiler
  types the sum `_int32`, backend.md D13);
- **the bootstrap compiled by itself** - not this stage's job (13.7), tried
  because it is the largest Sharp program there is: the C++-built bootstrap
  compiles the bootstrap's sources in 7 s (the file is 2.8 MB; the C++
  compiler's is 1.7 MB), the result compiles suite cases whose programs
  pass, and for a suite case it writes the same listing, byte for byte, as
  the C++-built bootstrap does. Stage 7 starts from there;
- the compiler started many times at once: the race described above. It also
  made the FIRST gates run of the verification fail (`functions` "did not
  compile": `.../stdlib/std/math/circle.sharp` doesnt exist) - a compile of
  the runner met one of the verifier's experiments in the same millisecond.

One thing seen and left for the driver stage (8.2): the bootstrap ends with
exit code 0 also when an option is refused (`-j abc`), where the C++
compiler ends with 1. For a program with errors BOTH end with 0, and the
runner tells success by whether the file was written.

**Not done in this stage's scope:**

- no default output name: without `-o` nothing is generated, where the C++
  compiler writes `out`. Every analysis-only run of the gates and of the test
  scripts (the library step, tests/backend.sh, tests/run.sh, the suite cases'
  analysis step) compiles without `-o`; with a default they would all start
  writing files into whatever folder they run in and pay for code generation.
  It belongs with the project mode's output folder in the driver stage (7),
  which must then give those scripts `-c`;
- `-O` configurations of the runner: stage 6;
- the kind check runs the suite only with `-nojit` (the checks are in the
  interpreter) and only for the `plain` executables (the `plain-nojit` ones
  are built from the same sources by the same compiler);
- `~/sharp_work/sharp_dev` (the runtime every other gates step uses) is not
  rebuilt by the gates: after a change to lib/runtime it is built by hand,
  as the runtime stage left it (`sharp_tc_dev` IS rebuilt when it is older
  than the sources, by tests/kindcheck.sh);
- the benchmark times are one run each on a shared machine.

### 13.6 Optimizer

status: done (the gaps are listed at the end)

**What was built.** `backend/optimizer` (the map of section 0): the analyses
and thirteen passes of 7.2 (twelve by the stage, loop unrolling by its
verifier: below), switched exactly as in the C++ compiler - `-O`
(`-O1` .. `-O3` the same) turns everything on, `-O0` off, off by default,
`-n<name>` leaves one pass out by the C++ compiler's names, `-Oreport` prints
what each pass did, how often it ran and how long it took. One more
option, `-Otrace <function>`, prints one function's IR after every pass that
changed it; it found the one wrong rewrite of this stage (below) in minutes and
is kept for the next. The optimizer runs between generation and the bytecode
step and has its entry on the `time:` line (`optimizer n ms`); `-ir` shows
the IR after it. Two things outside the package:

- the IR got one kind, `int_immediate` (ir.sharp; selected as `iaddl n, k`
  when the target is the operand, else `iadd`; the verifier's slot-kind check
  knows it), and one flag, `ir_instruction.handed_on`;
- the bytecode step (bytecode.sharp, `select_every_instruction`, `hand_on`)
  drops a store and the next instruction's load of the same register and slot
  when the flag allows it - and only when the words really are that pair, so a
  wrong flag can cost nothing but the saving.

**How it is kept right.** Correctness first, as the task said:

- the whole suite through its own runner at `-O`, JIT and `-nojit` (two new
  configurations of tests/suite_runner.sh, `O` and `O-nojit`, as suite.sh has
  for the C++ build), and the kind-check runtime on the `-O` executables;
- every codegen case and own case built once more with `-O` and run both ways
  (a new step of tests/codegen.sh), among them the new
  tests/codegen/cases/optimizer.sharp, which gathers the shapes each pass
  rewrites (folding at the 64-bit edges, every narrowing, -0.0, held-over
  copies, every loop shape, inlined getters and a getter of null inside a
  `try`, values a `catch` reads that were set right before something raised,
  immediates at the 32-bit edges, several boxes in one function) - checked
  against the C++-built program like every case;
- the options: `-nloop-rotation -ninline-calls` leaves exactly those out (and
  `-Oreport` says so), an unknown name is refused with the list, `-O -O0` is
  off;
- the whole library built with `-eager -O` (every one of its 11 305 functions
  through the optimizer and the verifier) and run to its end, JIT and `-nojit`;
- the bytecode verifier on every function, as always.

**What the suite found.** Two first versions were wrong and the suite at `-O`
said so before anything else did:

- box elimination replaced its boxes in index order, but replacing one puts
  its start values where it was made and so moves every later instruction: the
  second box's start values went where the first had been - in
  objects_without_new over a `dec_constant` that was an argument, so a check
  got the previous check's expected value (13 suite cases failed). Now the
  boxes are replaced from the last to the first. `-Otrace` showed the pass and
  the instruction; cases/optimizer.sharp has the shape now (`boxes()`);
- (before that, an out-of-range index in the same pass on the whole library:
  each replaced box adds slots, and the loop over the slots grew with them.)

**Speed of the programs.** The kostya set at the inputs of its README, built four ways, each
run once with the JIT on the fixed runtime (`sharp_dev`), two programs at a time on the machine
shared with the other stream (benchmark/kostya/run.sh; seconds are what the program measures
around its work; the output is the same in every cell):

| test | C++ | C++ `-O` | bootstrap | bootstrap `-O` |
|---|---|---|---|---|
| brainfuck `bench.b` | 14.7 | 11.3 | 15.2 | **8.5** |
| brainfuck `mandel.b` | 164.6 | **131.2** | 172.4 | 136.0 |
| base64 | 34.1 | 21.8 | 23.8 | **18.6** |
| json (131072 entries; run alone, three times for the bootstrap) | 8.9 | **5.4** | 8.3, 9.3, 9.0 | 9.5, 8.4, 8.4 |
| matmul 1500 | 23.4 | **17.2** | 21.1 | 17.7 |
| primes 5000000 | 5.1 | **3.2** | 4.9 | 4.2 |

So `-O` makes every program but json faster, by 14 % (primes) to 44 % (bench.b), and
the bootstrap's `-O` build is the fastest of the four on bench.b and base64, within 3 % of the
C++ `-O` build on matmul and mandel.b, and behind it on primes (30 %) and json (55 %). Where
it is behind, the C++ optimizer has passes this one does not: json and primes spend their
time making and reading number objects and strings in the library's hash maps and string
builders, which C++'s "native returns" (a version of a function that hands back the number,
not the box) and its larger inlining take apart. On json the bootstrap's `-O` is within
the noise of its plain build (it was 7.95 s once with `-nloop-rotation`; the runs spread by
a second). While the stage was built, primes alone on the machine went from 3.8 s (plain) to
3.4 s with leaf-only inlining, and to 2.25 s once inlined bodies could call other functions -
faster than the C++ `-O` build measured the same way (2.42 s); in the shared run of this table
it was 4.2 s, so the spread between runs is large.

Measured once more by the stage's verifier, with loop unrolling in, ONE program at a time
(the same builds and inputs, each run once with the JIT; mandel.b was not run again):

| test | C++ | C++ `-O` | bootstrap | bootstrap `-O` |
|---|---|---|---|---|
| brainfuck `bench.b` | 14.8 | 8.1 | 14.5 | **7.7** |
| base64 | 28.6 | 18.9 | 20.2 | **17.1** |
| matmul 1500 | 20.5 | 17.0 | 20.3 | **16.3** |
| primes 5000000 | 4.3 | 3.5 | 3.7 | **2.0** |
| json (131072 entries) | 8.2 | **5.2** | 9.0 | 7.6 |

Run alone, the bootstrap's `-O` build is the fastest of the four on every program but json,
where it gains 16 % over its plain build and stays 45 % behind the C++ `-O` build (native
returns, below). None of these programs has a loop with a small known trip count in its hot
path, so unrolling changes nothing here; it is for code like `for(k = 0; k < 3; k++)` over
the three coordinates of a point.

**Speed of the compiler.** 

| program | plain: file / compile | `-O`: file / compile | the optimizer's own time |
|---|---|---|---|
| suite case lib_data_json (438 functions) | 261 KB / 2.66 s | 377 KB / 3.85 s | 1.06 s |
| the library with `-eager` (11 305 functions) | 3.48 MB | 4.07 MB (IR 179 260 -> 253 414 instructions, 548 143 VM words) | 13.7 s |

The bootstrap's executables at `-O` are still smaller than the C++ compiler's (377 KB for
lib_data_json against 677 KB, 13.5); inlining is what makes them larger than the plain build.

**A runtime bug the measurements found, fixed.** The plain bootstrap build of kostya json
died with SIGSEGV before `main` - when started as `/tmp/fullbench/boot/json`, and not as
`./json`: the same bytes, another path. The VM's loader hands out entry 19 (`nil`) of a
14-entry table as the result type of every function that returns nothing (11.3), and
reflection read the bytes past the table; their content follows the heap's layout, which the
path's length moves. The table now covers every type up to `nil`
(lib/runtime/exe/symbol_processor.cpp, one line and its comment); the C++ compiler's own suite
passes on the new runtime in all four configurations, and the program runs from both paths.
It was not the optimizer: the program was built without `-O`, and C++-built programs write the
same 19.

**Two bugs of the kind-check runtime, fixed** (found by the new kind check of the `-O` builds;
both are in code that only the `-DSHARP_TYPE_CHECK` build compiles, so the release VM never had
them):

- `rstore`, `istore`, `ipushl` and `distore` set the kind tag of the new top of the stack
  BEFORE `grow_stack`; when the push was the one that grows the stack, the tag went one byte
  past the end of the tag array - a heap overflow that corrupted malloc's bookkeeping
  ("corrupted size vs. prev_size", then a hang) in suite case `loops` built with `-O`, which
  happens to grow its stack at such a push. The tag is now set after the stack has grown
  (lib/runtime/virtual_machine.cpp; found with a runtime built with both the address
  sanitizer and the kind checker, ~/sharp_work/rt/vasan_tc);
- the kind checker's signature of interrupt 0x45 (`fiber_start`) still said the started
  function is a number (`o i i o o`, from when a function value was an index); it is an
  object since the typed VM, so the checker read the number half of an object's slot - what
  an earlier value had left there - and reported "stack slot 13 holds a decimal" in the
  coroutine cases built with `-O` (lib/runtime/interrupt_signatures.cpp, now `o o i o o`).

After both, the kind check is clean on all 177 suite cases, plain and `-O`.

**The gates** (~/sharp_work/tools/gates.sh, backup gates.sh.before_optimizer; nothing
removed): the codegen step builds every case and own case again with `-O` and checks the
options and the library at `-eager -O` (tests/codegen.sh); the suite runner has the
configurations `O` and `O-nojit`; the kind check runs on the `-O` executables too. The full gates at the end of this stage: library 0 errors; 177 suite cases analysed, 0 failing, 0 VM crashes; frontend 17, backend 45, helps 58, codegen 91 passed (46 before); suite runner ALL 177 TESTS PASSED in `plain`, `plain-nojit`, `O` and `O-nojit`, 0 crashes; kind check 177 clean plain and 177 clean `-O`; programs 21, driver 15 passed; `GATES: PASS`. The first full run failed on the new kind check of the `-O` builds - the two runtime bugs above; nothing in the optimizer.

The full gates after the verifier's pass (loop unrolling in, four more cases, the programs also built with `-O`; backup gates.sh.before_optimizer_verify, only the description of step 11 changed): library 0 errors; 177 suite cases analysed, 0 failing; frontend 17, backend 45, helps 58, codegen 99 passed; suite runner ALL 177 TESTS PASSED in `plain`, `plain-nojit`, `O` and `O-nojit`, 0 crashes; kind check 177 clean plain and 177 clean `-O`; programs 21 (each now plain and `-O`), driver 15 passed; `GATES: PASS`. The library with `-eager -O`: optimizer 14.5 s in that run.

**What makes it slow, and what was done.** The compiler is a Sharp program on
the VM, built by the C++ compiler without `-O`, so an optimizer written the
plain way pays for every object it makes. Measured with `-Oreport`'s times:

- `list.size()` makes an `int` object at every call; in a loop condition that
  is one object per round. Hoisted out of every loop of the optimizer (a whole
  number taken once before the loop), the optimizer of a suite case went from
  1.7 s to 1.1 s; jump cleanup alone from 463 ms to 68 ms;
- the graph and the liveness are flat arrays (not a list per block), kept
  between passes until one changes the code (analysis.sharp), and a pass that
  found nothing is not run again until another pass changed something;
- constant propagation carries from block to block only the slots some block
  reads before writing them; copy propagation looks for the copies a write
  ends only when there are some; dead code uses a worklist;
- about a third of what is left is the garbage collector (two collections of
  about 0.18 s each during code generation of a suite case: `sharp -gclog`).

Inlining is most of the cost: it doubles the code the later passes work on,
and then they run again. It is also most of the gain (primes).

**The verifier's pass over this stage.** A second agent, which had not written
the optimizer, read every file of it and tried to break it:

- *by hand*: programs aimed at each pass's weak spot - a local of an inlined
  body set on one path only (the copied bodies of one caller share their
  slots, and a body in a loop runs in the same slots again), a body that
  changes its parameter, swaps and rotations through a third variable, an
  object that becomes slots inside a `try` whose handler reads it, values set
  right before a call, an element, `%`, a field of null or a cast raises,
  `finally` on the way out of nested `try`s, comparisons with NaN whose
  operands the hand-off swaps, the 64-bit and 32-bit edges of folding and of
  the immediates. Every one printed what the C++-built program prints, plain
  and `-O`, JIT and `-nojit`. Four of them are cases now
  (tests/codegen/cases/optimizer_inlining.sharp, optimizer_exceptions.sharp,
  optimizer_copies.sharp, optimizer_unrolling.sharp);
- *by machine*: a generator of random programs (nested loops with constant
  bounds, `break` / `continue`, `try` / `catch` / `finally` around calls that
  raise for some arguments, small helper functions, objects used only through
  their fields, narrow integers, decimals, every operator with constants mixed
  in so that there is something to fold). 380 programs of 80 to 200 printed
  lines each were built plain and with `-O` and run with the JIT and without:
  no difference in any of them. (The same programs built by the C++ compiler
  differed in about one of six, every one looked at traced to two of ITS
  slips: a `? :` with a constant arm that is compared or negated -
  `((11 == five) ? 7 : 0) == 0` is 0 there - and the cast of D13, section 10.
  The bootstrap's results are the right ones.) The generator is kept:
  `tests/optimizer_fuzz.sh <first seed> <last seed>` - run it after any change
  to backend/optimizer; `NO_TRY=1` writes programs without `try`, the only
  ones loop unrolling works on;
- no mistake was found in the twelve passes. What was missing or loose, and
  is done now:
  - **loop unrolling** (passes/control/loop_unrolling.sharp) and the analysis
    it needs, the natural loops of a function (analysis/loops.sharp:
    dominators by Cooper, Harvey and Kennedy's algorithm, back edges). It is
    the C++ pass's method - the loop is run while compiling - with its limits
    (16 rounds, 96 instructions); the random programs had well over a hundred
    loops it unrolled;
  - the identities `x + 0`, `x - 0`, `x * 1`, `x * 0` (immediates.sharp), which
    an unrolled body is full of (its counter is 0, then 1), and `copy %a, %a`
    as a dead store;
  - box elimination told two fields apart by their declaration OBJECT; it now
    also takes two declarations that name the same place of the object as one
    field (no program showed a difference; with two slots for one field it
    would have been a wrong value, silently);
  - constant propagation's sweeps have a limit (200: a guard, never reached);
  - `-Oreport` printed `-4219 instruction(s) fewer` for inlining and times as
    `10.9168540000000007 ms`: now `4219 instruction(s) more` and whole
    milliseconds;
  - tests/programs.sh builds every program of the repository (the kostya
    benchmarks, the examples) with `-O` as well, and that build must print
    what the C++-built program prints, JIT and `-nojit`: the gates had no
    `-O` run of the programs the optimizer is for.

**Not done in this stage's scope:**

- four of the C++ optimizer's passes are not here: **native returns** (a
  hidden version of a function that returns the number instead of the box: it
  needs new functions after layout - addresses, records, names - and inlining
  plus box elimination do the small cases), **loop formula** (a counting loop
  that only sums, replaced by the closed form: not attempted), **loop
  registers** (not attempted; it would have little to work with, 7.3, and the
  benchmarks above are within 3 % of the C++ `-O` build on the loop-heavy
  programs without it), and **stack forwarding** (nothing to do on an IR
  without an operand stack; the register hand-off is its counterpart). Their
  `-n` names are accepted;
- **dead code** does not remove whole loops whose results are unused. The C++
  pass does, with post-dominators, and says itself what that assumes: "a loop
  with no observable effect is assumed to END" - a loop that only counts is
  removed also when it would never end, and a delay loop loses its delay. Here
  a loop's jumps are effects, so such a loop stays: the program does what its
  source says. (An `if` that guards nothing does go: jump cleanup removes a
  jump to the next instruction, and its test is then a dead store.);
- **loop unrolling** leaves a function with a `try` alone (as the C++ pass
  does), and finds what is known on the way into a loop only in the one block
  in front of it (a counter set to a constant right before its loop: every
  `for(k = 0; ...)`), not across blocks;
- **slots in registers** across more than two instructions (7.3) is not built;
- no common subexpression elimination (not in the C++ optimizer either): a
  field read twice is read twice (`prime[n] = !prime[n]` reads `prime` twice);
- compile time: `-O` costs more than 8.3's budget ("at most half of the code
  generation time"): on a suite case it adds about 1.1 s to a 2.7 s compile (code generation without it is 0.1 s), and on the whole library 13.7 s. The cost is mostly the local passes running again on the code inlining made, plus the garbage collector; making the analyses incremental (not computed again for a whole function after each change) is the next step;
- the benchmark numbers are one run each on a machine shared with the other
  stream; mandel.b and bench.b were run once per build.

### 13.7 Self-hosting

status: done. The compiler compiles itself; the compiler that comes out
compiles itself again and writes the same file, byte for byte; and it gives
the same messages and the same executables as the compiler the C++ compiler
built. What is not done is listed at the end.

**What "self-hosting" proves, in plain words.** There are now three
compilers made from the one source text in this folder:

```
stage 1   the C++ compiler compiled the source                 (sharpc_boot; bootc.sh)
stage 2   stage 1 compiled the source                          (3.0 MB; 7.2 s)
stage 3   stage 2 compiled the source                          (must be stage 2 again)
```

Stage 1 and stage 2 are the same program translated by two different
translators. If the bootstrap translates Sharp correctly, the two behave
alike - so given the same input (the source again) they write the same
output: stage 3 is stage 2, bit for bit. One wrong instruction anywhere in
the 3,500 functions of the compiler that are on the path of a compile would
make stage 2 behave differently from stage 1 somewhere, and the two files
would differ. This is the strongest test the compiler has, and it is the
textbook one ("bootstrapping": the fixed point of a compiler applied to
itself).

**What was built**

- `tests/selfhost.sh`, step 13 of the gates. Its checks:
  - A. the fixed point: stage 2 and stage 3 are the same bytes; so are stage
    2's outputs on 4 threads and on the default number of threads (the
    parallel parse must not reach the output); stage 1 with `-O` and stage 2
    with `-O` write the same file (the optimized compiler), and THAT compiler
    compiling itself with `-O` writes it again;
  - B. stage 2 as the compiler, the messages: the standard library with every
    warning shown prints the same text; the frontend tests (17) and the
    backend tests (45) pass; each of the 40 programs that must be refused
    (tests/backend/fail, tests/parse/fail) gets, character for character,
    the messages stage 1 gives;
  - C. stage 2 as the compiler, the results: each of the 177 suite cases,
    compiled by stage 1 and by stage 2, is the same executable byte for byte
    and the same compiler output - once plain, once with `-O`. (Step 8 of
    the gates runs the files stage 1 writes and judges them; identical bytes
    behave identically.)
- the driver, completed (8.2): `command_line.sharp` (the options),
  `project.sharp` (the output file, `--new-project`, `-run`), main.sharp cut
  down to the run of the stages; tests/driver.sh grew from 15 to 35 checks.
- README.md: how to build the compiler with itself and how to use it.

**What stood in the way of the fixed point: one thing.** The first try gave
two files of different sizes. `-listing` on both and a `diff` showed the
globals of a module in a different order: `parse_all` runs one coroutine per
file and collected the results in the order they FINISHED, and stage 1 and
stage 2 do not finish in the same order (they are not equally fast). The
order of the files decides the order of a module's globals, the addresses of
classes and functions, the order of messages. The units are now put back in
the order the files were given (`in_the_order_given`, main.sharp). After that
one change stage 2 and stage 3 were identical at the first attempt - with
`-j 1`, `-j 4`, the default threads, and `-O`. No hash-map order and no
address reaches the output: layout already ordered everything by lists.

**What stood in the way of "the same messages": the stack.** Stage 2 gave
the same text as stage 1 for every program but two: the tests of code that
is nested too deeply. A compiler is a recursive program; how deep it can go
is how much stack one level of recursion takes, and stage 2's frames are
about twice the size of stage 1's (11.8: the C++ compiler keeps values in
registers, the bootstrap keeps every value in a frame slot). So with the
VM's default stack (64 KB) stage 2 ran out of stack on shallower code.
Two changes:

- the limits are now COUNTED where the recursion happens: 256 open
  statements and expressions in the parser (`PARSER_NESTING_LIMIT`: counted
  in `parse_statement` and `parse_expression`, E0103), 150 statements deep in
  the analysis (`ANALYSIS_NESTING_LIMIT`: `analyse_statement`, E0992). The
  counter raises the very exception the VM raises when the stack runs out,
  so nothing else changed; the message says which of the two it was. With
  enough stack every build of the compiler now stops at the same place with
  the same words;
- "enough stack" is `sharp -istack 1mb`: part B of selfhost.sh runs both
  compilers that way. With the default stack the stack still runs out first
  (the table in 11.8: 112 nested `if`s in stage 1, 70 in stage 2, 28 in
  stage 2 built with `-O`), which is reported as before, now with a note that
  names `-istack`. This is the one place where the self-built compiler is not
  the equal of stage 1 out of the box; README.md tells the user to start it
  with `-istack 1mb`, and the cure (smaller frames) is 7.3.

**The driver** (8.2 has the table). Found and fixed while completing it:

- nothing was written without `-o`; now `out`, as the C++ compiler;
- `-L` was ignored in project mode, the project's executable went to the
  current folder instead of `build/outputs`, and `--new-project` wrote a
  settings.json whose names had no quotes - which the compiler's own JSON
  reader then refused;
- settings.json was read with names the C++ compiler does not use
  (`output_file`, `extern_libs`); both spellings are read now, and
  `"target_api"` means what it means there (the library version), not the
  platform;
- the exit code was always 0, because what `main` returns is not the exit
  code of the process on this VM; the driver now calls `exit(code)`;
- the options were private variables of main.sharp, out of reach of
  settings.json; they live in `options` now.

**Speed** (the gates' machine, nothing else running, best of 3, wall time of
the whole process; "self" = the bootstrap's 383 files plus the library, about
100,000 lines; "hello" = a one-line program plus the library):

| the compiler | hello | self |
|---|---|---|
| the C++ compiler (`-j 1`) | 0.72 s | 3.21 s |
| stage 1: the bootstrap built by the C++ compiler | 2.31 s | 7.22 s |
| stage 2: the bootstrap built by itself | 2.12 s | 7.06 s |
| stage 2 built with `-O` | 1.42 s | 4.64 s |
| (stage 2 built with `-O`, before this stage's change to the inliner) | 1.63 s | 5.55 s |

Building the optimized compiler takes 19 s with stage 1 (the optimizer alone
11 to 14 s, 13.6) and 13 s when the optimized compiler builds itself. Sizes:
stage 1 1.8 MB, stage 2 3.0 MB, stage 2 with `-O` 4.1 MB. The budget of 8.3
holds. The self-built compiler with `-O` is 1.45 times the C++ compiler's
time on the large input and twice on the small one.

The `time:` lines of the self compile (stage 2 with `-O`): bodies 2.2 to 2.9
s, parsing 0.24 s, members 0.22 s, program checks 0.3 to 0.7 s, bytecode
0.25 s, generation 0.17 s; a stage that shows 0.7 s where another run shows
0.2 s had a garbage collection land in it.

**Where the time goes** - measured with a runtime built with
`-DSHARP_TELEMETRY` (lib/runtime/telemetry: it counts every call per
function), on stage 2 compiling itself:

| | stage 2 | stage 2 with `-O`, before | after |
|---|---|---|---|
| function calls | 163 million | 43 million | 37 million |
| objects made | 21.6 million | 15.2 million | 12.5 million |

1. **The boxes of numbers, not string keys.** The suspect before measuring
   was string-keyed lookups. It is not: the analysis stages had already
   moved the hot tables to `name_table`, whose hash is plain arithmetic
   (`name_hash`: 600,000 calls, no object); the library's `string.hash` runs
   11,000 times in a whole self-compile. What the count shows instead is
   `list.size()`: called 6.5 million times, and each call makes an `int`
   OBJECT through five constructors one inside the other (`int`, `number`,
   `base_number`, `_object_`, `set_value`) - 72 million of the 163 million
   calls and more than half of all objects, for `i < items.size()` at the top
   of every loop. Interning identifiers would not have touched it.
2. **Fixed, in the optimizer:** inlining plus box elimination is exactly the
   cure for a box that never leaves its loop, and it was failing on one
   detail - the static part of `int` had been inlined into `int`'s
   constructor and made it too large to inline in turn (7.2). With
   `static_init<C>` kept as a call, `i < items.size()` is a field read and a
   compare. Effect on the optimized compiler: 5.5 million `int` objects and 6
   million calls fewer, the self compile 5.55 s -> 4.64 s, hello 1.63 s ->
   1.42 s. It helps every program built with `-O`, not only the compiler. 60
   random programs (tests/optimizer_fuzz.sh, 30 with `try` and 30 without)
   print the same with and without `-O` after the change.
3. **Not fixed:** the unoptimized builds still make every box (the cure is
   the optimizer's). `string.operator==` of the library counts its loop with
   an `int` object: 1 million compares cost 4.2 million calls of
   `int.operator++`; the library is not this build's to change. The garbage
   collector's pauses are 1.7 s of the 7 s (20 collections, 17.8 million
   objects freed): fewer objects is the lever, and `-mem 4gb` (without it the
   same compile takes 11 s, because the collector runs more often).

**The gates** got step 13 (tests/selfhost.sh, about 25 minutes: 7 self
compiles, the tests with stage 2, 177 suite cases five times). Steps 2 and 3
and tests/backend.sh and tests/helps.sh now pass `-c`: they only analyse, and
a compiler that writes `out` by default would otherwise write it into the
folder the test runs in. Step 12 (tests/driver.sh) has 38 checks. Nothing was
removed or loosened. The final run:

```
library: 0 error(s), reached the end: yes   (full output: ~/sharp_work/out/boot/lib.txt)
suite cases: 177 run, 0 failing, vm crashes: 0   (outputs: ~/sharp_work/out/boot/cases/<name>.txt)
frontend tests: 17 passed, 0 failed
backend tests: 45 passed, 0 failed
help tests: 58 passed, 0 failed   (the programs with the helps pasted in: /home/braxton_nunnally/sharp_work/ou
suite cases: 177 of 177 run and pass (JIT and -nojit)
codegen tests: 99 passed, 0 failed
suite runner, plain: ALL 177 TESTS PASSED; vm crashes: 0 in programs, 0 in the compiler
suite runner, plain-nojit: ALL 177 TESTS PASSED; vm crashes: 0 in programs, 0 in the compiler
suite runner, O: ALL 177 TESTS PASSED; vm crashes: 0 in programs, 0 in the compiler
suite runner, O-nojit: ALL 177 TESTS PASSED; vm crashes: 0 in programs, 0 in the compiler
kind check: 177 clean, 0 with reports, 0 did not pass on the kind-check runtime
kind check: 177 clean, 0 with reports, 0 did not pass on the kind-check runtime
programs: 21 passed, 0 failed
driver tests: 35 passed, 0 failed   (work folder: /home/braxton_nunnally/sharp_work/out/boot/driver/work)
self-compiles (the bootstrap's 383 files + the library):
self-hosting: 14 passed, 0 failed   (work folder: /home/braxton_nunnally/sharp_work/out/boot/selfhost)
GATES: PASS
```

**A bug of the C++ compiler found** (worked around in
settings_processor.sharp, `project_path`): a `new` expression used directly
as the receiver of a call in a condition stops its code generator -

```
def pick(written: string) : string {
    if(new file(written).exists()) { return written; }
    return "none";
}
```

gives "sharp: <code-generator> fatal error: incorrect operation_step, found
(1) when (31) was expected. note: in function: pick". With the file in a
local first it compiles.

**The verifier's pass** (an independent check of this stage; what it found and
changed):

1. **The compiler built with `-O` ran out of stack four times sooner than
   stage 1** - the gap this section called "the one place where the self-built
   compiler is not the equal of stage 1". The cause was not the generator but
   the optimizer: its passes only ever add slots, so `-O` frames were 2.5
   times the generated ones. Cured with a new last pass, **slot packing**
   (7.2; backend/optimizer/passes/registers/slot_packing.sharp): frames of
   `-O` code are now as small as or smaller than unoptimized ones
   (`analyse_statement` 34 -> 10 slots, `parse_statement` 44 -> 13), and the
   optimized compiler follows 100 nested `if`s / 130 parentheses on the
   default stack where it stopped at 28 / 41 (stage 1: 112 / 120; the table
   in 11.8). It helps every program built with `-O`: `deep(450)` of
   tests/codegen/cases/optimizer_frames.sharp overflowed with `-O` only.
   Checked by the suite at `-O` (JIT and -nojit), the kind check of the `-O`
   builds, 120 random programs (tests/optimizer_fuzz.sh), the new codegen
   case, and the `-O` fixed point (the optimized compiler, which is itself
   packed, compiles itself and writes itself). One mistake of its own was
   caught on the way by the suite (`exceptions`, `lock`): the flow graph
   remembers the slot each handler is entered with, so after renumbering the
   graph must be made again, not only the liveness.
2. **The optimized compiler was only ever checked on itself.** It is the one
   README.md tells a user to keep. tests/selfhost.sh got part D: it compiles
   the library (same text as stage 1), every refused program (same messages)
   and every suite case (same bytes as stage 1 writes).
3. **The driver, for the reader.** main.sharp was 730 lines holding five
   subjects, in a folder of 8 files, and `cache` and `main` imported each
   other. Now: the options are a package of their own below everything
   (`configuration/`: options, command line, settings.json, `error` /
   `warning`), `cache` imports that and no longer `main`, and main.sharp (360
   lines: `main` and the run of the stages) has its other subjects in files
   of their own - parsing.sharp, broken_declarations.sharp, executable.sharp,
   and the gathering of files in source_folders.sharp. No behaviour changed.
4. **settings.json**: a file that is not JSON printed one line and the build
   went on WITHOUT the project's settings (so the ignored folders were
   compiled, the libraries were missing ...): it now stops the build. A name
   that is not a setting was silently ignored: it is now a warning with the
   nearest names (`unknown setting "ouptut" is ignored: did you mean
   "output"?`). `"version"`, which `--new-project` writes into every new
   settings.json and neither compiler read, now sets the version (`-v`).
5. **`-a`** was listed as "done"; it is accepted and changes nothing, because
   this compiler never hides an error (8.2 says so now, and `--h`).

Times after the verifier's pass (same machine, best of 3): the C++ compiler
0.69 s / 3.16 s (hello / self), stage 1 2.30 / 7.41, stage 2 2.19 / 6.82,
stage 2 built with `-O` 1.47 / 4.74 - slot packing changes frame sizes, not
speed, and costs 0.3 s of a 14 s optimizer run on the compiler itself.

The full gates were run twice by the verifier: once on the stage as it was
handed over (PASS), and once at the end, with everything above in:

```
library: 0 error(s), reached the end: yes
suite cases: 177 run, 0 failing, vm crashes: 0
frontend tests: 17 passed, 0 failed
backend tests: 45 passed, 0 failed
help tests: 58 passed, 0 failed
codegen tests: 101 passed, 0 failed
suite runner, plain / plain-nojit / O / O-nojit: ALL 177 TESTS PASSED (each)
kind check: 177 clean, 0 with reports (plain builds, and the -O builds)
programs: 21 passed, 0 failed
driver tests: 38 passed, 0 failed
self-compiles (the bootstrap's 389 files + the library):
        stage2, stage3, stage3_j4, stage3_threads: 3035051 bytes each
        stage2_O, stage3_O, stage4_O: 4122976 bytes each
        refused programs: 40, the messages of stage 2 differ in 0
        refused programs: 40, the messages of the optimized compiler differ in 0
        suite cases (plain): 177 compiled by stage 1 and by stage 2, 0 different
        suite cases (plain): 177 compiled by the optimized compiler too, 0 different from stage 1's
        suite cases (O): 177 compiled by stage 1 and by stage 2, 0 different
self-hosting: 17 passed, 0 failed
GATES: PASS
```

**Not done**

- The nesting depth with the VM's default stack: 71 nested `if`s in the
  UNOPTIMIZED self-built compiler against 112 in stage 1 (11.8); the one
  built with `-O` is level with stage 1 since slot packing (100 `if`s, 130
  parentheses). Started with `-istack 1mb` all three are the same. Two tests
  still need the larger stack to give stage 1's exact message in every build
  (200 `if`s one inside the other: the parser of a self-built compiler runs
  out of stack before the analysis' counted limit is reached), so part B and
  D of selfhost.sh run with `-istack 1mb`. What is left is the owner's call:
  a larger default stack in the VM, or lower counted limits.
- The counted limits cover statements and expressions in the parser and
  statements in the analysis. An expression that nests deeper than the
  analysis' stack holds but stays under the parser's 256 levels would still be
  stopped by the stack, at a depth that depends on the build; with a 1 MB
  stack no build gets there (254 parentheses pass in all three).
- Stage 2 is checked as the compiler on the library, the frontend and
  backend tests, the refused programs and the suite (same bytes); the
  optimized compiler on the library, the refused programs and the suite. The
  help tests, the codegen cases and the benchmark programs are not run a
  second time with a self-built compiler: every suite executable being
  identical was taken as enough, and the full gates would take twice as long.
- Speed: ON THE LARGE INPUT (the bootstrap and the library, 100,000 lines)
  the unoptimized compiler is as fast as stage 1 but 2.2 times the C++
  compiler's time; the optimized one 1.45 times. On a small program the
  ratio is worse, because about 2 s go to the library whatever the program:
  the completeness critics measured p10_generic_mix (240 lines plus the
  library) at 0.73 s with the C++ compiler, 2.67 s with stage 1 (3.7 times)
  and 1.68 s with stage 2 built with `-O` (2.3 times), and 2.3 to 3.8 s
  against 0.6 to 0.9 s for 15 such programs. That is the everyday case. What would move it
  further is in "Where the time goes": boxes in unoptimized code, the
  library's `string.operator==`, the collector. Lazy analysis of library
  bodies that the program never reaches (what the C++ compiler does: "library
  function bodies put aside") would cut the 2 s of a small program most, and
  was not attempted: this compiler checks the whole library on every compile.
- The build cache still only knows which files changed (cache/); nothing is
  skipped because of it.
- `-run` needs `sharp` on the PATH; it does not pass arguments to the
  program.
- With `-ide` or any tool that only wants the messages, pass `-c`: without
  it a compile without errors now writes `out`.

### 13.8 Completeness fixes

status: done (gaps below). Independent critics compared the bootstrap
compiler with the C++ compiler program by program and option by option and
listed 22 gaps. Each was reproduced first; this section says what each turned
out to be and what was done.

**Fixed in the compiler**

| # | gap | what it was | what was done |
|---|---|---|---|
| 6 | `E0908` "a circle" for three component definitions written in dependency order (config, repository, service); the reverse order compiled | analysing one definition's `get()` searches EVERY definition, so it analysed a later one whose own `get()` then waited for the first, still in progress; the waiting request was then called a circle whatever it really needed | backend.md 7.20 "analysed too early": a definition analysed inside another's analysis whose request waits is put back and analysed afresh later; a waiting request is a circle only when its answer needs, along `type_definition.needs`, the request's own definition (`path_of_needs`, components/requests.sharp); the message's places are that path. tests: backend pass/components.sharp, codegen own/component_chains.sharp (the C++ compiler crashes on every such program) |
| 5 | `movi ebx, 5000000000`: E0972 "`main` is too large", no place | the 32-bit raw word found too wide only when encoding | new `E0926` in analysis, at the number, with the value the C++ compiler would silently keep and a fix (asm_operands.sharp); E0972 (left for operands the encoder alone can see) says "does not fit the VM's instructions" and how to fix it. test: backend fail/statements.sharp |
| 16 | `E0975`'s help said "without -o", which now writes `out`; a `main` of the wrong form got W1015 and then "has no `main`" | - | the help names `-c`; with a global `main` of a wrong form E0975 points at it and says so (`report_no_main`, finalizer.sharp). test: codegen refused/main_of_wrong_form.sharp |
| 8 | W1037 said `18446744073709551615` "does not fit `_uint64`" | it fits; written in digits it is a decimal, stored as 9223372036854775807 | the headline names the value stored (exactly: `"" + n` is lossy in a C++-built compiler, D13, so the two ends are written out); the label says why; for a `_uint64` the help gives the bits in hexadecimal, worked out from the digits by long division with small numbers (`hexadecimal_of_digits`, the same in every build) |
| 2 | W1037's help sent users to `0xFFFFFFFFFFFFFFFF`, which the C++ compiler reads as 9223372036854775807 | hexadecimal literals of 2^63 and above differ (G43) | the help says so; G43 written down |
| 10, 17, 18 | an empty option value crashed (VM, below); `-o -c` and `-v -c` swallowed the `-c`; `-j 00` and `-j 99999999999999999999` were accepted; `-errlmt 99999999999999999999` said "limit of 0"; `-target` named no valid platform | value-taking options read the next argument blindly; numbers went through `int.parse` | one checked reader, `value_after` (configuration/command_line.sharp): missing, empty and option-looking values are refused naming the option; `whole_number` reads a count in digits (no sign, no point, at most 9 digits); `-target` lists `alpha` / `alpha-cr4` / 3 |
| 11 | `-ir` printed nothing and `-analysis` the last LIBRARY file whenever `-L` was used | "the last file given" was taken after the library's files were added | `options.inspected_file`: the program's last file, noted before the library is added (main.sharp, executable.sharp, finalizer.sharp) |
| 12 | `-errlmt` did not limit syntax errors | they were printed per file without a limit | printed through `print_within_limit` like the later stages' (main.sharp) |
| 13 | `-o` naming a source file overwrote it (the C++ compiler too) | - | refused before compiling, with the name to use instead (`check_output_is_not_a_source`, source_folders.sharp) |
| 14 | a FOLDER named `x.sharp` made both compilers loop for ever, using more and more memory | `check_source_files` tested only "exists" and ".sharp" | refused like a missing file, with the `-L` / `-P` that read a folder (source_folders.sharp); a missing file gets did-you-mean from its folder |
| 15 | `-run` ended with 0 for `main` returning 3 and after an uncaught exception; without `sharp` on the PATH it ended with 127 and let `sh: sharp: not found` through | the VM's exit code (below); `run_program` started the shell first | VM fixed; `run_program` looks the runtime up with `command -v` first and ends with 1 and its own message |
| 19 | driver messages: `library_dir` printed without `$`, "dosen't" / "doesnt" / "extion", "could not write file: X" with no reason, settings.json's wrong type without the type wanted, no did-you-mean for a missing file | - | all rewritten; `error_with_help` / `warning_with_help` (configuration.sharp) print a `= help:` line; an unwritable output says which: folder missing, a file, the output a folder, no permission (`why_not_written`, output/output.sharp) |
| 20 | an empty settings.json was a warning and the build went on without the library (8 errors); a setting given twice was silently the last | - | empty (or blank) is "not JSON" and stops the build, with a `{ }` to write; a member name written twice in the text is a warning (`warn_about_settings_given_twice`, settings_processor.sharp) |
| 21 | `-P demo -o custom` ignored `-o` without a word; `-run -c`, `-nativedir -c` did nothing silently | settings.json is read after the command line and wins (as in C++) | settings.json still wins; the overruled `-o` is a warning, and so are `-run` and `-nativedir` with `-c` |
| 22 | no library: E0962 plus E0400 `println`, E0301 `string`, four times over | every name of the library was missing too | with no module `std` and a global `main`, ONE E0962 right after the declarations are collected and the compile stops (`report_library_missing_first`, program/required_declarations.sharp); in a project its help names settings.json's `"extern_lib"` |

**Fixed in the runtime** (lib/runtime, lib/Modules; tests/suite/known_bugs/README.md "Fixed")

- An empty command-line argument killed EVERY program before `main`
  (SIGSEGV), so the compiler's own empty-argument check was dead code: the
  VM makes no array of length 0 and threw while filling the argument's text.
  An empty argument is now an empty array object (`create_empty_array`,
  memory/sharp_object.cpp, stack_limits.cpp). The library then appends it to
  a string with `memcopy` of 0 items into a null array; `memcopy` of 0 items
  now does nothing (lib/Modules/std.io/memory.cpp). `sharp ""` reports a
  missing file instead of aborting (main.cpp).
- `def main() : int`'s result never became the exit code: `destroy()` read
  the exit value before `shutdown_thread()` set it from main's result. Now
  `shutdown_thread()` hands it on, and an exception nobody caught on the main
  thread ends the process with 1 (multitasking/thread/thread_controller.cpp).
  `exit(n)` is unchanged. This changes C++-built programs too (Q13).
- The C++ compiler's suite passes all four configurations on the new runtime
  (tools/suite.sh, 177 cases).

**Written down, not changed** (the bootstrap is right; the C++ build differs)

- 1: backend.md D97 said "the same values" for `n : _int64 = 1e30;`. Wrong:
  the C++ fold gives -9223372036854775808 for every constant out of range,
  while `d2i` at run time gives the nearest end. The bootstrap folds as `d2i`
  does. D97 corrected, G42 added.
- 3: a typed `foreach` variable converts each item (G44).
- 4: asm `mov8` .. `movu64`, `d2i`, `i2d` with two registers, and `dmovi`
  (G45); asm_table.sharp's example `iaddl 5, [x]` corrected to `iaddl 5, 2`.
- 7: a lambda's write to a captured object local is kept here and lost in
  the C++ build (G46); `${n + 1}` above 32 bits is D13, now also in section 10.
- 9: the compile-time ratio of 13.7 was measured on the large input; on a
  small program it is 3.7 (stage 1) and 2.3 (stage 2 built with `-O`) times
  the C++ compiler's. 13.7's not-done list now says which input.
- G42 - G45 are stated as rules in tests/codegen/own/number_rules.sharp.

**Tests**: tests/driver.sh 38 -> 62 checks (items 10 - 22); codegen
own/component_chains.sharp, own/number_rules.sharp, refused/main_of_wrong_form.sharp;
backend pass/components.sharp (the chain), fail/statements.sharp (E0926).

**Gates**: one full run: every step passed except step 7, where the new own case
number_rules.sharp declared `show` / `show_decimal` a second time (tests/codegen/print.sharp
has them). After removing the copies, `gates.sh only 7` (a test file changes only its own
step): 106 passed, 0 failed. Library time line: bodies 1006 ms, members 235 ms, completeness
341 ms - this stage adds no pass.

**The verifier's pass** (an independent check of the above: every item probed
again, the gates run in full before and after)

1. *A lambda in a put-back definition ended in E0991.* The fix of item 6
   analyses a definition twice. `factory { new printer({ -> ... get<reader>() ... }) }`
   as the last link of a chain was refused with "internal: the compiler could
   not work out what this expression is": the lambda of the dropped analysis
   stayed in the program with a `get` that had no answer. The lambdas of a
   dropped analysis now leave with it (`type_definition.lambdas`,
   `forget_lambdas_of`, components/definitions.sharp).
2. *A `get` inside a lambda was an edge of the circle test.* A scheduler
   whose lambda asks for a worker, and a worker that takes the scheduler, was
   E0908 - though the lambda asks only when it is called, which is how a
   circle is broken. Such a request (`component_request.in_lambda`) is no
   edge now (backend.md 7.20); a lambda asking for its own definition stays
   E0908, with a note and help of its own.
3. *E0926's help was wrong.* It said to name the local with `[big]` in the
   instruction; `movi ebx, [big]` loads the local's PLACE in the frame (0),
   not its value. The help now gives `loadl ebx, [big]` (the register taken
   from the instruction), and the label no longer says "more than 32 bits"
   for 4294967295. New help test: tests/helps/asm_wide_number.sharp (the fix
   pasted in builds with both compilers).
4. *Item 22, second half, done:* an E0303 for a module that only files left
   out with `-ignore` / `-ignoredir` / "ignore_folders" declare gets a note
   naming the file and a help (new left_out_modules.sharp in the driver: the
   driver knows the options, analysis does not; only the `mod` line of the
   left-out files is read, and only when there is an E0303). tests/driver.sh.
5. The empty-value message no longer shows `-j "$OUT"` for every option.
6. Tests added: codegen own/component_chains.sharp (the lambda at the end of
   a chain; the lazy pair), backend fail/components.sharp (a lambda asking
   for its own definition), helps/asm_wide_number.sharp, driver.sh (the
   left-out module).

Checked and found right as reported: items 1 - 4, 7 (the samples of the
critics run again, JIT and -nojit, against the C++ build), 8, 10 - 21 (each
option by hand), the runtime changes (an empty argument, `main`'s result,
exit code 1 after an uncaught exception and 0 after a caught one, `exit(n)`;
both compilers' programs). gates.sh itself was not changed by the stage.

**Not done**

- 22: the follow-on `E0301` (the type of the left-out module) is still
  printed after the E0303; whether a missing type belongs to a left-out file
  cannot be told without parsing that file.
- `loadl ebx, 70000` in an `asm` block (a slot the frame does not have) is
  caught by the verifier as E0970 "internal error ... a bug of the compiler":
  it is the program's mistake and deserves a message of its own at the operand.
- 20, first half: "a fresh `--new-project` warns on its first build" did not
  reproduce: `--new-project` makes `build/` and the first `-P` build is
  quiet (checked by hand and in driver.sh). Nothing was changed.
- 19: the driver's messages still carry no code (E/W numbers are for the
  compiler's diagnostics); they now have a `= help:` line each.
- The did-you-mean for a missing source file looks only in that file's own
  folder, and only when the folder has fewer than 20,000 files below it.
- E0972 still has no place in the source: the encoder knows the function,
  not the node. The one case a program can write (an `asm` number) is E0926.
- 9: compile time itself is unchanged (lazy analysis of library bodies is the cure, 13.7).
