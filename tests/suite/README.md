# Sharp test suite

Tests for the Sharp compiler, runtime and standard library, written in Sharp. The whole suite is
ONE program: a project (`settings.json`, `main.sharp`) whose folders hold the tests.

```
tests/suite/
    settings.json    the project: its program is build/outputs/suite
    main.sharp       the program: calls the top folders, one group each
    language/  control/  functions/  classes/  generics/  arrays/  strings/
    exceptions/  concurrency/  jit/  library/          the groups, one folder each
    helpers/         what several groups need that is not a test (run_isolated ...)
    isolated/        tests that need a process of their own
    known_bugs/      behaviour that is known to be wrong today (not part of the program)
```

## Running it

With what is in the repository now - its VM, compiler and library, never the installed ones -
from the top of the repository:

```
bash test.sh suite                                      every test
bash test.sh suite suite.library                        a folder and everything in it
bash test.sh suite suite.library.collections.queues     one group
bash test.sh suite suite.jit suite.strings              several
bash test.sh suite O                                    built with -O
bash test.sh suite windows                              on the Windows VM too (from Git Bash)
```

By hand, with an installed Sharp:

```
sharpc -P tests/suite
cd tests/suite
sharp build/outputs/suite
sharp build/outputs/suite -test-only suite.library.collections
sharp -nojit build/outputs/suite
```

Run it with `tests/suite` as the current folder. `-test-only` may be given more than once; a name
that matches no group makes the run fail, so a typing mistake never passes with nothing run.

The program prints a line for every test (`  ok    ...`, `  FAIL  ...` with what went wrong), then
the full names of the failed ones under `failed:`, and last `all N tests passed` or
`K failed, P passed (N tests)`. The exit code is 0 when every test passed, 1 otherwise. The whole
of it runs in seconds.

A complete check is four runs: built plain and with `-O`, each run with the JIT and with `-nojit`.
`bash test.sh vm full` does that (test 8 of `build-scripts/test.sh`), and when a run dies without
its summary it runs it again one top-level group at a time, to name the group that crashes.

## Folders are groups

A group's name is its path, with dots: the folder `library/collections` is the group
`suite.library.collections`, and the file `queues.sharp` in it is the group
`suite.library.collections.queues`. Naming a group runs it and every group inside it.

Every folder has an `entry.sharp` that calls the groups in it, and `main.sharp` calls the top
folders. Nothing is picked up by itself: a file that no `entry.sharp` calls does not run.

A group's name is one word of letters, digits and `_`.

## Adding a group

1. A new file in the folder it belongs to, with the module of its path and one function, named
   after the file, that holds one `tests("<name>")`:

   ```
   mod suite.library.collections.stacks;

   import ( std.testing, std.collections )

   /** std.collections - stack: the last one in is the first one out */
   def stacks_tests() {
       tests("stacks") {
           s := new stack<int>();
           s.push(1); s.push(2);
           test("peek gives the last one pushed") { expect(s.peek().value).should_be(2); };
           test("pop takes it off") { s.pop(); expect(s.size()).should_be(1); };
       };
   }
   ```

2. Its module in the `import` list of the folder's `entry.sharp`, and the call inside that
   folder's `tests(...)`:

   ```
   stacks_tests();
   ```

A new folder is the same one level up: an `entry.sharp` with the module of the folder
(`mod suite.library.rings;`), a `def rings_tests()` with `tests("rings") { ... }` that calls the
groups in it, and the call in the `entry.sharp` above it (or in `main.sharp` for a top folder).

## Writing tests

- Tests are written with the standard testing library, `std.testing`, and nothing else: `test`,
  `expect(...).should_be(...)`, `expect_true`, `expect_null`, `fail_test` ... (its documentation
  lists them). There is no checking helper of the suite's own.
- Code that several groups need and that checks nothing - a way to start a child process, a
  shared fixture - goes in `helpers/` (`import ( suite.helpers )`).
- Write the expected value out by hand; do not compute it with the code under test.
- A test gives the same result every time: no clocks, no random numbers, no thread timing.
- A test that works with files does it in a folder of its own under /tmp and removes it at the
  end. (Two runs of the program at once would spoil each other's files: the test scripts never
  start it twice at the same moment.)

## Tests that need a process of their own

Some tests start the program again as a child, or check what happens outside any group. They live
in `isolated/`: each file has a `<name>_program(args)` with its own groups and a
`finish_tests("<name>")` at the end, and `isolated/entry.sharp` lists it. `sharp suite isolated
<name>` runs one of them and nothing else.

In the whole run each of them is one test, in the group it belongs to, that starts the program that
way with `run_isolated` (`suite.helpers`) and expects it to pass:

```
test("the subprocess tests pass, in a process of their own") {
    ran := run_isolated("subprocess");
    expect(ran.code).named("suite isolated subprocess said:\n" + ran.said).should_be(0);
};
```

(The ARM test under qemu leaves these groups out: qemu runs one program, and the child would be
an ARM program started without it.)

## Known bugs

`known_bugs/` holds what is still open, and what is the way it is on purpose: a bug with a
reproduction gets a file there, expected to fail, and not part of the program. When it is fixed,
its checks move into the group they belong to and it leaves the list. `known_bugs/README.md`
says what is in it.
