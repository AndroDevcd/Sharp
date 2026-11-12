# Sharp test suite

Tests for the Sharp compiler and runtime, written in Sharp.

```
tests/suite/
    run.sharp        the runner: compiles and runs every test, prints PASS / FAIL
    testing.sharp    the checks the tests use (expect, expect_str, finish, ...)
    cases/           one Sharp program per area of the language
    known_bugs/      tests for behaviour that is known to be wrong today (not run by default)
```

## Running it

Build the runner once, then point it at the compiler and runtime you want to test:

```
sharpc tests/suite/run.sharp -o run_tests
mkdir -p /tmp/suite
sharp run_tests sharpc sharp tests/suite /tmp/suite
```

Anything after the work folder is passed to every compile, so the same tests
check the optimizer, and the runtime can be given options in quotes:

```
sharp run_tests sharpc sharp           tests/suite /tmp/suite -O
sharp run_tests sharpc "sharp -nojit"  tests/suite /tmp/suite
sharp run_tests sharpc "sharp -nojit"  tests/suite /tmp/suite -O
```

A complete check is those four runs: with and without `-O`, with and without the JIT.

The last line is `ALL n TESTS PASSED` or `k OF n TESTS FAILED`, and the runner
exits with 0 or 1. For a failed test it prints the checks that failed; the full
output is in the work folder (`<test>.out.txt`, `<test>.compile.txt`).

## Writing a test

A test is an ordinary program that imports `testing`:

```
mod main;

import ( testing )

def main(args: string[]) {
    expect("17 % 5", 17 % 5, 2);
    expect_str("concatenation", "ab" + "cd", "abcd");
    finish("my_test");
}
```

- every `expect...` is one check; write the expected value out by hand, do not compute it with the code under test
- `finish(name)` must be the last call: it prints the `RESULT PASS` / `RESULT FAIL` line the runner looks for
- a test must give the same output every time (no clocks, no random numbers, no thread timing)
- put a new file in `cases/` and it is picked up automatically

`testing.sharp` lists all the checks.
