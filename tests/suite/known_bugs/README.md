# Known bugs

Every file in this folder is a reproduction of a bug that is still open. Each one
is **expected to fail** (a failed check, a compile error, a crash or a hang) and
is not run with the normal suite. When a bug is fixed, move its checks into
`../cases/` and delete it from this list.

Status below was checked on 2026-11-12 after the compiler bug-fix round (see
"Fixed" at the end). "All configurations" means with and without `-O`, with and
without the JIT.

To run one by hand:

```
sharpc tests/suite/known_bugs/<name>.sharp tests/suite/testing.sharp -o /tmp/bug
sharp /tmp/bug
```

## Needs a decision, not (only) a fix

| Behaviour | Where |
|---|---|
| Assigning to a non-nullable string variable/field calls `operator=` on the object it holds, so every other reference to that string changes too. (Null elements and nullable targets now get a new object.) | described in `lib_coroutines` report; no file |
| `override` only applies to interface / delegate methods; a plain base-class method is bound statically. Documented as the current design in `cases/inheritance.sharp`. | - |

## By design (not bugs)

- Enum members without an explicit value get a value that depends on when the
  enum is compiled; they do not count up from 0. Give members explicit values
  when the number matters.
- `override` only applies to interface / delegate methods.
- The operands of a binary operator are evaluated RIGHT TO LEFT: in
  `f() + g()` g runs first (decided 2026-11-12; `cases/operators_evaluation_order.sharp`).
  Write calls with side effects as separate statements when the order matters.
- Assigning a number to a field or variable that holds a number OBJECT writes
  the number into that object (it calls the object's `operator=`), so every
  holder of the same object sees it (decided 2026-11-12;
  `cases/lib_collections_language.sharp`). Assign `new int(n)` to replace the
  object for one holder only.
- Block comments nest: `/* a /* b */` is still open, the tokenizer counts
  every `/*` (decided 2026-11-12).
- `new coffee { water_left = 100 }` (inline initialization) is not part of the
  language. `examples/new_class.sharp` still shows it; the examples are left
  as they are, several are older than the language (decided 2026-11-12).
- A function value is an object (see "Fourth part" below). `f == g` is true
  when both are the same function with the same captured variables: `f == twice`
  after `f := twice`, but two closures made by two calls of one function are
  different.
- A closure captures objects, arrays and function values, not plain numbers
  (`count : decimal`): put the number in an object (`new int(0)`).

## Compiler

Found on 2026-11-12 porting `lib/compiler` to the current language:

| Bug | File |
|---|---|
| An error in the initializer of a global is reported in the files that IMPORT the module, with the importing file's text at that line; "could not locate main method" is reported against `std/io/atomic.sharp:0:0`. | not reduced |

Found on 2026-11-15 by random programs built with both compilers (the verifier
of the bootstrap's optimizer, `lib/compiler/doc/codegen.md` 13.6); the
bootstrap compiler gives the right answer in each:

| Bug | File |
|---|---|
| A `? :` with a CONSTANT arm, compared or negated, gives the wrong answer when the other arm is taken: with `five : _int64 = 5; wide : _int64 = 0;` the program prints 0 for `((11 == five) ? 7 : 0) == 0`, for `((11 == five) ? 123456789 : wide) == 0` and for `!((11 == five) ? 123456789 : wide)` (each is 1). With two variables as arms (`((11 == five) ? five : wide) == 0`) it is right, and so is the value when it is only printed (`n3 : _uint8 = 0; show((11 == five) ? 123456789 : n3)` prints 0). Seen with and without the JIT; not tried with `-O`. | not reduced further |

Found on 2026-11-15 while the bootstrap's driver was completed
(`lib/compiler/doc/codegen.md` 13.7); worked around in
`lib/compiler/configuration/settings_processor.sharp`, `project_path`,
by putting the object in a local first. The bootstrap compiler builds the
program as written:

| Bug | File |
|---|---|
| A `new` expression used directly as the receiver of a call inside a condition stops the code generator: `def pick(written: string) : string { if(new file(written).exists()) { return written; } return "none"; }` gives "sharp: <code-generator> fatal error: incorrect operation_step, found (1) when (31) was expected. note: in function: pick(std#string): std#string". With `candidate := new file(written); if(candidate.exists())` it compiles. | not in a file (the text here is the whole reproduction; it needs `import ( std.io )`) |

Not in a file (described only; not retested on 2026-11-12):
members added by one `mutate` file are not always visible inside another
`mutate` file of the same class. Also: when a generic class gets its `mutate`
members after the block was processed (a `list<T>` first used later), the
bodies are compiled in the file of the generic class, so a name that only the
mutating file imports may not resolve.

## Runtime and JIT

| Bug | File |
|---|---|
| Seen with the JIT that was replaced in November 2026: a crash when functions are first compiled while several worker threads are inside them (the coroutine tests warm up on one thread to avoid it). A bug of exactly this shape was found and fixed on 2026-11-14 (see "Fixed" below: a fiber continued in the machine code of a function compiled while it waited); the warm-ups were left in place. Re-tested without them on 2026-11-14 (`lib_coroutines_core`, `_channels`, `_sync` with the `warm_up()` call taken out, plain and `-O`, 25 runs each, and 8 runs each under the address sanitizer): no failure, neither with the runtime from before that fix nor with the one after. So the warm-ups no longer guard anything that can be reproduced; they cost nothing and stay. | `cases/lib_coroutines_resume_after_compile.sharp` |
| NOT REPRODUCED OUTSIDE THE BOOTSTRAP COMPILER: a plain `main` that `launch`ed one coroutine per source file (each read and parsed a file, then `send` its result on an unlimited channel) and then called `receive()` once per file hung about one run in three with 6 or more files, with and without the JIT; 4 or fewer files never hung. A small test of the same shape with a number loop instead of the file work ran 1,200 rounds without hanging, so the cause is something in the real work (file reads, large allocations during a collection) and is not found. `lib/compiler/main.sharp` now uses `run_blocking { coroutine_scope { launch_on(pool) } }` and a list, which has not hung. | first version of `parse_all` in `lib/compiler/main.sharp`, 2026-11-12 |
| The JIT exists for x86-64 Linux / Intel macOS only; on arm64 and Windows everything is interpreted. | `lib/runtime/jit/jit_compiler.cpp` |
| An integer constant above 2^53 written in source is loaded through a decimal constant (`ldc` + `d2i`) and loses its low bits. | needs an integer constant table |
| Deep recursion ends in SIGSEGV instead of a stack overflow exception. | described in `cases/recursion.sharp` |
| The directory call returns only regular files of a whole tree and skips dot names; no environment-variable call; read-file returns null for empty and missing alike. | worked around in `std/io/files` |

## Library

| Bug | File |
|---|---|
| Existing `read_line()` loops forever at end of input; `print_numbers(_int16[])` etc. call themselves. | `std/console.sharp` |
| `file.get_free_space()` returns <= 0 under `-O` (asm loads an int object). | `std/io/file.sharp` |

## Fixed (kept here so the history is easy to find)

Fixed on 2026-11-15 (runtime): a SIGSEGV at start-up, before `main`, in `build_reflect_function`
(reflection). The loader's table of native types had 14 entries, but it hands out entry 19 (`nil`)
as the result type of every function that returns nothing, so reflection read memory past the
table; when those bytes read as a class or function type it followed a garbage pointer. Whether
it crashed depended on the heap's layout - the length of the program's PATH changed it (the kostya
json program built by the bootstrap crashed as `/tmp/fullbench/boot/json`, ran as `./json`). Fixed
in `lib/runtime/exe/symbol_processor.cpp` (the table covers every type up to `nil`); found by the
bootstrap's optimizer stage while measuring the benchmarks (codegen.md 11.3, 13.6).

Fixed on 2026-11-15 (runtime, the kind-check build only, `-DSHARP_TYPE_CHECK`): `rstore`, `istore`,
`ipushl` and `distore` wrote the kind tag of the new stack top before growing the stack, one byte past
the tag array when that push grew it (heap corruption, then a hang); and the checker's signature of
interrupt 0x45 (`fiber_start`) still expected a number for the started function, which is an object
since the typed VM (false "holds a decimal" reports). Both found by the kind check of suite cases built
with the bootstrap's `-O` (codegen.md 13.6).

Fixed on 2026-11-15 (runtime), found by the bootstrap's completeness critics (codegen.md 13.8):
- An EMPTY command-line argument (`sharp prog a "" b`) killed every program with SIGSEGV before
  `main`: the VM makes no array of length 0 (`create_object` gives null), so filling the argument's
  text threw "string field is null!" during start-up. It is now an empty array object
  (`create_empty_array`, memory/sharp_object.cpp; used in multitasking/thread/stack_limits.cpp),
  and `memcopy` of 0 items does nothing whatever the arrays are (lib/Modules/std.io/memory.cpp:
  `string.append` of an empty array copies 0 items into a null `data`). `sharp ""` no longer
  aborts on `string("").at(0)` (main.cpp).
- What `def main() : int` returns never became the exit code of the process: `destroy()` copied
  the main fiber's exit value BEFORE `shutdown_thread()` read main's result into it, so every
  program ended with 0 (only `exit(n)` worked). `shutdown_thread()` now hands the value on
  (multitasking/thread/thread_controller.cpp), and an exception nobody caught on the main thread
  ends the process with 1, not 0.

Moved into `../cases/` after they started to pass: `threads.sharp` (scheduler
deadlock, `join()` and `start()` results), `logic_precedence.sharp`,
`variables_multiple.sharp`, `operators_on_new.sharp`, `casts_of_expressions.sharp`,
`new_in_condition.sharp`, `foreach_over_new.sharp`, `foreach_inherited_loopable.sharp`,
`thread_fiber_id.sharp`, `fiber_recursive_suspend.sharp`, `lock_exception_and_nesting.sharp`, `string_plus_empty_literal.sharp`, `arrays_sized_elements.sharp`,
`non_null_index*.sharp`, `generics_isolated_and_nested.sharp` (a generic class seeing the caller's locals; `box<box<var>>`), `init_blocks_order.sharp`, `init_blocks_locals.sharp`,
and the `*_fixes.sharp` files.

Fixed in the compiler / library round of 2026-11-12 (the files moved to `../cases/`):

- **Compiler crash on `"literal" + s` in a lambda** (`lib_*_lambda_concat`):
  `sharp_type` had no copy assignment, so `a = b` shared the list of unresolved
  items and the first to be destroyed freed it under the other. Also found
  with the address sanitizer: a context item made with `new` was released with
  `free()`.
- **`(a < b) + (c > d)` gave garbage** (`comparison_sum`): the left operand of
  the addition was read from `bmr`, but a comparison leaves its result in `cmt`.
- **`0.1 + 0.2 == 0.3` folded differently than at run time** (`comparison_fixes`):
  literals are folded with 64-bit doubles, like the VM computes.
- **`"n" + (2 + 3)` printed `n5.0000000000000000`** (`strings_basic_fixes`): two
  whole-number literals fold to a whole number. **`text + 4294967296` printed
  0**: a whole-number literal that does not fit in 32 bits is an `_int64`.
  **`s += s`, `s += 5`, `s += 2.5`** work (library).
- **`x == null` on a class with its own `==`** (`lib_io_language`, `std_misc_fixes`,
  `strings_basic_fixes`): it is always a test for "no object" and never calls
  the class's operator. `"" == null` is false; a null string can be compared.
- **`put(')')` and `return ';'`** (`lib_time_paren_char_literal`): a string or
  character literal never counts as punctuation, whatever its text.
- **`names : string[] = { "a", "b", "c" }`** makes three strings
  (`lib_time_array_initializer`), and **a string literal is no longer accepted
  for a `string[]`** (it was, and crashed at run time): now a compile error.
- **`def f(x: _int8)` called with 200** sees -56 (`casts_fixes`): an argument
  is cut to the parameter's size, as a cast would do.
- **`n : int?;` and `n : int[]?;`** start as null (`lib_collections_language`
  bug 2); only a plain `n : int;` is given an object.
- **A try/finally inside a finally block** no longer loses the pending
  exception (`finally_fixes`): the inner statement has its own hidden local.
- **A failing cast throws `class_cast_exception`** (`exceptions_cast_fixes`),
  which is now an `illegal_state_exception`, so old `catch` clauses still work.
- **`NaN != NaN` on `double` objects** is true (`lib_math_literals`):
  `number` had `operator==(double)` but no `operator!=(double)`.
- **Ambiguous calls with a number** (`functions_fixes`,
  `lib_time_implicit_number_to_class`): a plain number is passed to the
  overload that takes a plain number rather than to one that wraps it in an
  object (`backend/dependency/overload_preference.h`).
- `lib_coroutines_localized_arithmetic` and `lib_coroutines_jit_fiber_frames`
  pass since the typed VM and the new JIT.

Second part of the same round:

- **An interface method overridden in a subclass** is found through the class
  in between as well (`interfaces_fixes`, `interfaces_base_calls`): a call to a
  function that implements an interface method goes through that method (a
  run-time lookup) when a class derived from the function's class has its own
  version; `base->f()` stays a direct call. See "Direct call or run-time
  lookup?" in `compiler/expressions/expression.cpp`.
- **Properties** (`properties_fixes`): an assignment from outside no longer
  runs the getter (and is no longer lost under `-O`); `p.value += 5`,
  `p.value++`, `--p.value` call the getter and then the setter.
- **`"abc".size()`** (`lib_io_syntax`): a function can be called on a string
  literal; the literal becomes a `string` object first.
- **A number where a `string` is expected is a compile error**
  (`lib_text_literals`). Only a character ('a', an `_int8` / `_uint8`) still
  becomes a string by itself; write `"" + n` for the text of a number.

- **`mutate` on a generic class** (`lib_collections_mutate_generic`): a
  top-level `mutate list { ... }` was applied to the `list<T>` classes that
  existed at that moment and then forgotten, so a `list<card>` first used
  further down never got the members. The mutation is now recorded on the
  generic class and every later instance receives it (fields included).

Third part: the entries that had no reproduction file. Each got one; four
already worked (`dictionary_one_pair`, `negative_literal_in_condition`,
`generics_nested_lists`, `properties_getter_only_assignment` are now tests).

- **A class or enum written inside a class could not be named from outside**
  (`classes_nested_names`, `enums_inside_classes`): `holder.part.count`,
  `holder.inner.off`. A class found inside the class being compiled was marked
  "a value", not "a type", and nothing nested in it could be reached.
- **`fns[0](4)`** (`function_pointers_in_arrays`): a call on an array element
  was parsed as a call of the array's name and the index was dropped.
- **`flag ? "text" : name`** (`ternary_string_literal`): a string literal on
  one side of `?:` and a string object on the other.
- **`$` in a string** (`strings_dollar_sign`): only `$name`, `${...}` and `$$`
  mean something; `"cost: $5"` is the text it looks like.
- **`abs(3.5)`** (`lib_math_abs_plain_number`): `abs` has a version for a plain
  number.
- **`atomic`** (`lib_io_atomic_comparisons`): the functions named `operator<=`
  compared with `>=`; they are `operator>=` now, and a real `operator<=` exists.
- **`new result<T, E>(data, null)`** (`std_result_null_error`).

Fourth part: lambdas carry what they captured (`lambdas_capture_per_call`,
`lib_coroutines_closure_across_fibers`).

- **A captured variable was null on another fiber, and shared between the
  calls of a function that calls itself.** The captured variables of a
  function were kept in one object per function per fiber. Now a function
  value is an object (`platform.kernel#function_value`: which function, and
  the captured variables), the enclosing function makes a new set of captured
  variables on every call, and a lambda that captures receives them as a
  hidden last parameter. Every lambda is still a static function and slot 0
  of a frame is still only ever the instance of an instance function.
  Described in `lib/grammar/backend/dependency/closure_capture.h` and
  `lib/runtime/memory/function_value.h`.
- **`self` in a lambda** was "cannot access self from static context"; it is
  captured like a local now.
- A function value was a number: `callback != 0` is `callback != null` now,
  and an array of functions is an array of objects.

Found while doing the fourth part:

- **`x = x + n` on a fiber-local (`localized`) number stored a meaningless
  number** (`lib_coroutines_localized_arithmetic`). The value was loaded into
  `ebx`, the addition read the register it had asked for, and the copy between
  the two was behind a test written the wrong way round
  (`code_context.cpp`, `numeric_tls_field`). The test only passed before
  because that register happened to hold the right value.
- **A coroutine cancelled right after it was continued killed its worker
  thread, and whoever joined it waited for ever** (`lib_coroutines_core`,
  about one run in three with the JIT, never without). After a context switch
  the interpreter can be running a function whose caller is compiled code
  that has no machine frame yet. When that function threw and did not catch,
  "back to the compiled caller" ended in the thread's fiber loop, which
  reported the exception as unhandled although a `try` further down the stack
  catches it. The fiber loop now goes round again while the fiber still has
  functions on its stack (`thread_controller.cpp`), and a compiled function
  continued at its first instruction with a signal pending looks at the
  signal first instead of running that instruction again
  (`jit/x64/sections/entry.cpp`).

- **A fiber was sometimes switched out the moment it started**
  (`lib_coroutines_sync`, "waiters got the mutex in the order they arrived":
  `bcda`; one run in three with the JIT, about one round in 300 without). The
  scheduler's monitor thread reads the clock once per round and then tests
  every thread's time slice with `now - startedAt > slice`. A fiber that
  started after the clock was read has `startedAt > now`; both are unsigned,
  the difference wrapped around to an enormous number and the fiber was
  preempted at once (`scheduler.cpp`, the time slice loop). It looked like a
  JIT bug only because the JIT changed how often the two moments met.

Found running several bootstrap compilers at once (2026-11-14):

- **"(SIGSEGV) found" in about one run in 150** of `lib/compiler`,
  a different suite case each time, always passing when run again
  (`lib_coroutines_resume_after_compile`, and `lib_coroutines_resume_shapes`
  for the other mixes of frames). A fiber continued after a context
  switch gets the machine frames of its compiled functions back; the walk
  (`main_vm_loop`, `invoke_next_frame`) entered every function compiled at
  that moment, also one the interpreter had been running when it made its
  call and that another thread compiled since. Its still-interpreted callee
  ran in a nested interpreter loop, which after the callee returned ran the
  rest of the fiber, and the outer loop then dispatched past the end of
  `fiber_main`. A function is now continued in machine code only when the
  frame of its call says it called from machine code
  (`continues_in_machine_code`, `virtual_machine.cpp`). Found with a runtime
  built with `-fsanitize=address` (heap-buffer-overflow in `main_vm_loop`).

Found porting `lib/compiler` (2026-11-12):

- **`names[0] = other` on a list stopped the compiler** ("attempting to assign
  stack data!"), and with a literal it wrote into the object the list held
  (`list_index_assignment`). `object[index] = value` now stores the value: it
  is compiled as `object.put(index, value)` (`assign_expression.cpp`).
- **`cond ? a : b` with a `T` and a `T?`** was a "nullability mismatch"
  (`ternary_nullable_side`); the result is a `T?` now.

From the typed VM and the new JIT (November 2026):

- **`lib_coroutines_jit_fiber_frames.sharp`** (crash in `return_method()` when two
  sibling coroutines take turns and one yields twice) passes with the new JIT;
  the file is still here and can move to `../cases/`.
- **`std_serialization` failing with the address sanitizer and the JIT on** was a
  bug of the old JIT, which is gone.
- **`DUP` wrote one slot past the end of the fiber's stack** when the stack was
  exactly full: it was the one push without the "grow the stack" check
  (heap corruption in deep recursion under `-O`).
- **`copy()` of an empty range** copied into a null array (`std.io/memory.cpp`).
- **The optimizer's box removal and native-return passes had stopped firing**
  when number fields moved into the object (they matched `movn` + `iload`);
  they read `getf` / `putf` now.

Found with the address sanitizer while the tracing garbage collector was tested
(all three were in the runtime before it, and explain rare crashes):

- **Crash or hang inside `start_fiber`** (about 1 run in 12 of `lib_coroutines_sync`):
  the fiber start code read the new fiber's id after handing the fiber to the
  scheduler, and another thread could run it to its end and free it first.
- **Crash when a short thread is started or joined**: the scheduler freed the
  thread's struct while the thread that started it was still waiting on it
  (`hold_thread` / `release_thread` in `thread_controller.cpp`).
- **Memory corruption after a locked object is freed**: `linkedlist::delete_at`
  (`lib/util/linked_list.h`, used for the table of object locks) never removed
  the first entry and left `tail` pointing at freed memory when it removed the
  last one.
