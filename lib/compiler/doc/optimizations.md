# Optimizations: what the compiler does, and what we plan to add

This is the running list of optimizations for the Sharp compiler: what is
built, what is known to be missing, and what we have decided to do next. It is
a planning document. The description of how the optimizer works today is
`codegen.md` section 7 and section 13.6; this file does not repeat it.

**How to use it.** Every optimization has one row with a status. When a
decision is made (do it, drop it, do it later), change the status and add a
line to the decisions log at the bottom with the date and the reason. When an
optimization is built, record the measurement that shows what it bought.

Statuses: **built** - **planned** (we decided to do it) - **candidate** (on the
list, not decided) - **dropped** (decided against, with the reason) -
**needs VM** (cannot be done in the compiler alone).

"Value" is a judgement of how much it should help a bytecode VM with a JIT,
not a measurement, until the "measured" column is filled in.

---

## 1. Where things stand

Last updated 15 Nov 2026, after the thirteen approved optimizations (section
3a) were built and the full gates passed on them (all 14 steps: the suite
177 / 177 in plain, plain-nojit, O and O-nojit; both kind checks; self-hosting
17 / 17, the compiler built with `-O` included).

The optimizer runs with `-O` (`-O1`, `-O2`, `-O3` mean the same). Without it
nothing runs and programs are still correct.

### Benchmarks (seconds; the output is the same in every cell)

One round on a quiet machine, 15 Nov 2026 (the owner stopped the run after the
first of three rounds: single runs, so a few percent either way is noise).
"before" is `-O` with every optimization of section 3a switched off by its
`-n` name - it still has the one part that has no switch (box elimination
working on reused slots, built with native returns), so it understates what
the round gained: json measured 8.4 s before any of it.

| benchmark | C++ | C++ `-O` | Sharp | Sharp `-O` before | Sharp `-O` | Sharp `-O` against C++ `-O` |
|---|---|---|---|---|---|---|
| brainfuck `bench.b` | 15.8 | 8.1 | 13.4 | 9.4 | 7.3 | 9 % faster |
| base64 | 26.7 | 18.5 | 19.5 | 15.4 | 15.3 | 17 % faster |
| json | 10.2 | 4.8 | 9.5 | 5.4 | 4.4 | 9 % faster |
| matmul 1500 | 22.3 | 15.3 | 18.5 | 16.1 | 16.2 | 6 % slower |
| primes 5,000,000 | 4.3 | 2.1 | 3.7 | 2.0 | 2.2 | level (within noise) |
| brainfuck `mandel.b` | 151 | not measured | | | | |

**json, the one that had to be fixed, is fixed**: from 55 % slower than the C++
optimizer to ahead of it (native returns, box elimination on reused slots, and
common subexpressions do most of it; objects made in one run: 24.4 M -> 18.1 M,
C++ `-O` 19.5 M). `bench.b` gained about a fifth. base64, matmul and primes
did not move: their inner loops index with `i < n` for an `n` that is not
`sizeof` of the array, which the simple form of bounds check elimination does
not prove (range analysis, L11, is not approved), and they make no objects.

A program made of what the new passes are for shows more than these do: the
loop closed form test program runs in 3 ms instead of 189 ms, and a function
of three lambdas with captured variables compiles to plain arithmetic with no
object and no call.

### Other numbers worth watching

| | value | note |
|---|---|---|
| executable size, unoptimized | 80 to 110 KB for the benchmarks | the C++ compiler writes about 570 KB for the same programs |
| compile time added by `-O` | about 27 s for the compiler compiling itself (14 s before the thirteen optimizations of 3a); a small program with the library about 1 - 2 s | unoptimized code generation is about 0.1 s. The owner's decision (P2): compile time does not matter against the gains |

---

## 2. Built

| optimization | level | note |
|---|---|---|
| copy propagation, copy coalescing | `-O` | |
| constant folding and propagation, immediates | `-O` | |
| branch threading, jump threading | `-O` | |
| dead stores, dead code (not whole loops), unreachable code | `-O` | |
| register hand-off | `-O` | values kept in a register across two instructions only |
| loop rotation | `-O2` | |
| inlining of small functions | `-O2` | smaller budget than the C++ optimizer's |
| box elimination | `-O` | objects that stay inside one function; since 15 Nov 2026 also when the slot that holds the object is reused for other values (the object gets a slot of its own first) |
| native returns | `-O` | a function that returns a freshly made number object gets a twin `name$field` that returns the number; calls that only take the number out go to the twin (P1) |
| pure function detection | `-O` | for every function: changes nothing / reads nothing / never raises / always ends; an unused call of a function that does nothing else is removed, and the passes below use the facts (W9) |
| devirtualization | `-O` | a virtual call whose one body is known (one implementing class kept in the program, or the object made here) becomes a direct call - behind a null test when the object may be null (W1) |
| common subexpression elimination | `-O` | available expressions over the whole function, including field / static / element reads and calls of pure functions that give a number (L1) |
| loop-invariant code motion | `-O` | computed once in front of the loop; what may raise moves only when the loop begins with it or its object is known not null (O1) |
| null check elimination | `-O` | `x == null` / `x != null` with a known answer folded; the VM's own check inside a field read stays (C1) |
| bounds check elimination | `-O` | `a[i]` in a loop `for(i = 0 or more; i < sizeof(a); i++)` gets no `checklen` (C2, the simple loop form) |
| escape analysis | `-O` | which arguments of each function stay inside it; with it, objects that are filled and never read are not made (M1) |
| scalar replacement | `-O` | box elimination, now value-based: any object all of whose uses are field reads and writes in one function becomes slots, also when its slot is reused; objects only null-tested fall to it after C1 (M2 - see the limit in section 3a) |
| closure elimination | `-O` | a lambda made and called in the same function (also after its user was inlined) is called by name, inlined, and its function value and captured record removed (M5) |
| string concatenation flattening | `-O` | `a + b + c` and templates: one string, one copy of the first piece (M10) |
| loop closed form | `-O` | P7 |
| only what `main` reaches is written to the file | always | this is why the executables are small |

---

## 3. Parity gaps: what the C++ optimizer does and this one does not

These come first. They are the reason the Sharp optimizer trails on json and
primes, and "as good as the compiler it replaces" is the bar before going
further.

| id | optimization | value | status | note |
|---|---|---|---|---|
| P1 | **native returns** - a hidden second version of a function that returns the plain number instead of a number object, called where the caller only takes the number back out | high | **built** (15 Nov 2026) | passes/calls/native_returns.sharp, object_paths.sharp. Measured in section 6. The main cause of the json gap. The stage skipped it because the C++ design creates the new function after layout; here the twin can be made BEFORE layout, as an ordinary function, since every return type and call target is known by then. |
| P1b | larger inlining budget | high | dropped | Do with P1; re-measure json and primes after both. | Owner, 15 Nov 2026: no.
| P2 | **optimizer compile time** - the analyses are recomputed for a whole function after every change | high | dropped | Not a missing pass, but it will matter more than most of them once the compiler compiles itself with `-O`. Make the analyses incremental. | Owner, 15 Nov 2026: compile time does not matter; the gains are worth it.
| P3 | registers over longer stretches (linear scan as planned in codegen.md 7.3) | medium | candidate | Needed before P4 is worth doing. |
| P4 | loop registers - the number locals of an inner loop live in registers while it runs | medium | candidate | After P3. |
| P5 | dead loops - remove a whole loop whose results nobody uses | low | candidate | Needs control dependence (post-dominators). |
| P6 | loop unrolling - a loop with a small known count becomes straight code | low | **built** (15 Nov 2026, by the optimizer stage's verifier) | |
| P7 | loop closed form - a counting loop that only sums becomes its formula | low | **built** (15 Nov 2026) | Exact for every count (64-bit arithmetic that wraps), so no fallback loop as in the C++ pass. Counter `i < n`, `i++` only. |
| - | stack forwarding | - | dropped | Nothing for it to do: the Sharp generator does not produce the push-then-pop pattern it cleans up. |

(The order these were built in is section 3a. All thirteen approved items are built.)

---

## 3a. Planned: the owner's choice (15 Nov 2026)

Thirteen optimizations are approved, to be built AFTER the compiler workflow
(optimizer at parity, self-hosting, critics) has finished: the twelve the owner
named, and escape analysis (M1), which scalar replacement and closure
elimination cannot be done without and which the owner approved separately.
**Nothing else is to be added to this plan without the owner saying so.**

Order of work. Each group is built, then measured (section 5) before the next:

| step | optimizations | why here |
|---|---|---|
| 1 | P1 native returns | the json gap; parity with the C++ optimizer |
| 2 | W9 pure function detection | a fact the later ones use: a call to a pure function can be merged (L1), moved out of a loop (O1) or dropped |
| 3 | W1 devirtualization | turns virtual calls into direct ones, which the existing inlining can then take |
| 4 | L1 common subexpression elimination, O1 loop-invariant code motion | the two classic "do it once" passes; both use step 2 |
| 5 | C1 null check elimination, C2 bounds check elimination | cheap checks removed, mostly inside loops; C2 starts with counting loops over an array's own length |
| 6 | M1 escape analysis, M2 scalar replacement, M5 closure elimination | fewer objects for the collector; M1 first, the other two stand on it |
| 7 | M10 string concatenation flattening | `a + b + c` and string templates build one string |
| 8 | P6 loop unrolling, P7 loop closed form | the remaining loop passes of the C++ optimizer |

**All eight steps are built (15 Nov 2026).** What each one turned out to be, where it
deliberately stops, and what was left out because it was not approved:

- **M2 scalar replacement stops at the inlining budget.** An object is replaced by
  slots only when every use of it is a field read or write inside one function -
  so its constructor has to have been inlined. A constructor above the inliner's
  14-instruction budget stays a call, and its object stays an object (it is
  removed only when it is never read: dead objects). Replacing those would need
  the larger inlining budget the owner declined (W3 / P1b); nothing was added
  for it.
- **C2 bounds checks: the simple loop form only** - `i < sizeof(a)` on the same
  array, an index that counts up from 0 or more. `i < n` with another `n`
  (matmul's loops), `a[i + 1]`, a loop that counts down keep their checks: that
  needs range analysis (L11), not approved.
- **C1 null checks: the program's own tests.** The VM's check inside every field
  read is part of the instruction; removing it needs unchecked instructions in
  the VM and both JITs. Not done, not approved.
- **W1 devirtualization keeps null exact**: where the object may be null the
  direct call sits behind a null test with the virtual call as the other branch.
- **Every one of these leaves a function with a `try` alone** (except
  devirtualization's whole-program case, string flattening and the function
  facts): the analyses do not follow exceptions into handlers.

Chosen items and what each depends on:

| id | optimization | depends on |
|---|---|---|
| P1 | native returns | - |
| P6 | loop unrolling | - |
| P7 | loop closed form | - |
| W1 | devirtualization | the override families the analysis already records |
| W9 | pure function detection | - |
| M2 | scalar replacement | M1 escape analysis |
| M5 | closure elimination | M1 escape analysis; the closure records analysis already builds |
| M10 | string concatenation flattening | - |
| C1 | null check elimination | - |
| C2 | bounds check elimination | better with L11 range analysis (not chosen); the simple loop form needs none |
| L1 | common subexpression elimination | better with W9 |
| O1 | loop-invariant code motion | better with W9 |

Not chosen, but worth a decision because the chosen ones lean on them:

- **W3 cross-function inlining / P1b larger inlining budget: decided, not doing
  it.** The owner said no. The inliner keeps its current budget; devirtualization
  (W1) and native returns (P1) are built without it, and whatever part of their
  gain needs bigger inlining is left for a later decision.
- **P2 optimizer compile time: decided, not doing it.** The owner's answer:
  compile time does not matter, the gains are worth it. `-O` will get slower
  to run as passes are added; that is accepted. Record the compile time with
  each step all the same, so the cost is known.

---

## 4. Beyond parity: what this compiler can do that the C++ one could not

The C++ optimizer works on finished bytecode and has to guess what the
instructions meant. This compiler optimizes one level up, where it still knows
the exact function every call goes to, every override in the program, the real
type of every value, and what `main` can reach. Nothing in this section is
decided; it is the list to choose from.

### 4.1 Whole program

| id | optimization | what it does | value | status |
|---|---|---|---|---|
| W1 | devirtualization | a virtual call with only one reachable override becomes a direct call | high | **built** (15 Nov 2026) |
| W2 | guarded devirtualization | two or three overrides: test the class, call directly, fall back to virtual | medium | candidate |
| W3 | cross-function inlining | inline across classes and modules, after W1 | high | dropped |
| W4 | dead field removal | drop fields nothing reads | medium | candidate |
| W5 | interprocedural constant propagation | a parameter always given the same constant becomes that constant | medium | candidate |
| W6 | function specialization | a copy of a function tailored to a common argument | medium | candidate |
| W7 | dead argument removal | drop parameters no body uses | low | candidate |
| W8 | return value propagation | a function that always returns the same thing | low | candidate |
| W9 | pure function detection | mark functions without side effects, so calls can be merged, hoisted or removed | high (enables others) | **built** (15 Nov 2026) |
| W10 | whole-program type analysis | which classes can actually reach each variable | medium | candidate |
| W11 | identical function merging | two functions with the same body share one copy | low | candidate |
| W12 | global variable optimization | a global never written after start-up becomes a constant | medium | candidate |

### 4.2 Objects and memory

| id | optimization | what it does | value | status |
|---|---|---|---|---|
| M1 | escape analysis | find objects that never leave the function that made them | high (enables M2, M3, M5) | **built** (15 Nov 2026) |
| M2 | scalar replacement | such an object is never created; its fields become locals | high | **built** (15 Nov 2026) |
| M3 | stack allocation | such an object lives on the stack | medium | needs VM |
| M4 | unboxed generics | `list<_int64>` stores numbers, not number objects | high | needs VM (object layout) |
| M5 | closure elimination | a lambda called only where it is written needs no closure object | high | **built** (15 Nov 2026) |
| M6 | allocation sinking | move an allocation into the one branch that needs it | low | candidate |
| M7 | redundant load elimination | the same field read twice with no write between: read once | medium | candidate |
| M8 | store-to-load forwarding | reading a field just written: use the value written | medium | candidate |
| M9 | dead field stores | a field write overwritten before anyone reads it | medium | candidate |
| M10 | string concatenation flattening | `a + b + c` builds one string, not two temporaries | high for string templates | **built** (15 Nov 2026) |
| M11 | field reordering | pack fields to shrink objects | low | candidate |

### 4.3 Checks the language adds

| id | optimization | what it does | value | status |
|---|---|---|---|---|
| C1 | null check elimination | skip a null test on a value known non-null | high | **built** (15 Nov 2026) |
| C2 | bounds check elimination | skip an index test proven in range, especially in counting loops | high | **built** (15 Nov 2026) |
| C3 | cast check elimination | skip the run-time test of a downcast the types prove | medium | candidate |
| C4 | type test folding | `x is t` decided while compiling | low | candidate |
| C5 | lock elision | drop a lock on an object no other thread can see | low | candidate |
| C6 | lock coarsening | merge back-to-back locks on one object | low | candidate |

### 4.4 Classic, inside one function

| id | optimization | what it does | value | status |
|---|---|---|---|---|
| L1 | common subexpression elimination | `a * b` computed twice: compute once | high | **built** (15 Nov 2026) |
| L2 | global value numbering | the stronger form of L1, across branches | medium | candidate |
| L3 | partial redundancy elimination | compute once even when only some paths repeat it | low | candidate |
| L4 | strength reduction | `x * 8` becomes a shift, `x % 2` a mask | medium | candidate |
| L5 | algebraic simplification | `x + 0`, `x * 1`, `x - x` | medium | candidate |
| L6 | reassociation | reorder arithmetic so constants combine | low | candidate |
| L7 | if-conversion | a small `if` becomes a branch-free select | low | candidate |
| L8 | tail call elimination | a call in return position reuses the frame | medium | candidate |
| L9 | tail recursion to loop | self-recursion in return position becomes a loop | medium | candidate |
| L10 | sparse conditional constant propagation | constants and dead branches found together | medium | candidate |
| L11 | range analysis | track the possible range of each number (feeds C2) | medium | candidate |
| L12 | switch lowering | `when` on numbers becomes a jump table or binary search | high if `when` is common | candidate |
| L13 | code hoisting and sinking | move shared code out of both branches of an `if` | low | candidate |

### 4.5 Loops

| id | optimization | what it does | value | status |
|---|---|---|---|---|
| O1 | loop-invariant code motion | compute once outside what does not change inside | high | **built** (15 Nov 2026) |
| O2 | induction variable simplification | `i * 4` in a loop becomes a running add | medium | candidate |
| O3 | `foreach` lowering | iterate arrays by index with no iterator object | high | candidate |
| O4 | loop unswitching | move an unchanging `if` outside the loop | low | candidate |
| O5 | loop fusion and fission | merge or split loops over one range | low | candidate |
| O6 | loop peeling | handle the first round separately | low | candidate |
| O7 | loop interchange | swap nested loops for better memory order | low | candidate |
| O8 | vectorization | several array items per instruction | - | needs VM |

### 4.6 Code layout and the output file

| id | optimization | what it does | value | status |
|---|---|---|---|---|
| F1 | superinstructions | VM instructions for very common instruction pairs | high | needs VM |
| F2 | hot / cold splitting | move rarely run code (error paths) out of the way | low | candidate |
| F3 | function ordering | functions that call each other placed close together | low | candidate |
| F4 | constant pool sharing | identical strings and constants stored once | medium | candidate |
| F5 | lazy static initialization | a class's statics set up on first use | medium (start-up) | candidate |
| F6 | start-up snapshot | run static initializers while compiling, store the result | medium (start-up) | candidate |

### 4.7 Using measurements

| id | optimization | what it does | value | status |
|---|---|---|---|---|
| G1 | profile-guided inlining | inline what actually runs hot | medium | candidate |
| G2 | speculative devirtualization | guess the common class from a profile, guard it | medium | candidate |
| G3 | branch layout from profiles | the common path goes straight through | low | candidate |

### 4.8 Belongs in the JIT, not the compiler

Machine-level register allocation, instruction selection, inline caches for
virtual calls, on-stack replacement of hot loops, deoptimization. Listed so
they are not forgotten; they are VM work.

### Where to start, if the goal is the most speed for the least work

1. W1 + W3: devirtualization, then cross-function inlining. Everything else
   improves once calls disappear.
2. M1 + M2 + M5: escape analysis, scalar replacement, closure elimination.
   Fewer objects means less garbage collection.
3. C1 + C2 + O1: null and bounds checks, loop-invariant code motion.
4. L1 + M7: common subexpressions and repeated field reads.
5. M4: unboxed generics. Large for number-heavy collections, and the biggest
   change, since it touches how generic classes are laid out in the VM.

---

## 5. How to measure

- The kostya benchmarks: `benchmark/kostya/run.sh`, each program built four
  ways (C++, C++ `-O`, Sharp, Sharp `-O`), same VM, JIT on. Run on a quiet
  machine, three times each, and record the best.
- Once the compiler compiles itself: the time for it to compile its own source
  is the most useful single number, since it is a large, class-heavy Sharp
  program that is run constantly.
- Every optimization must leave all suite programs passing at every
  optimization level, with the JIT and with `-nojit`. A pass that cannot be
  made right is left out and listed here, not shipped.
- Record compile time as well as run time: an optimization that makes the
  compiler slow has a cost of its own.

---

## 6. Decisions log

| date | decision |
|---|---|
| 2026-11-15 | This document started. The owner's decision: finish the planned compiler workflow (optimizer at parity, self-hosting, critics) with nothing added, and decide which further optimizations to do afterwards. |
| 2026-11-15 | The owner chose twelve optimizations to build after the workflow: loop unrolling (P6), native returns (P1), devirtualization (W1), pure function detection (W9), scalar replacement (M2), closure elimination (M5), string concatenation flattening (M10), null check elimination (C1), bounds check elimination (C2), common subexpression elimination (L1), loop-invariant code motion (O1), loop closed form (P7). Escape analysis (M1) added as a prerequisite of M2 and M5. Order: section 3a. Larger inlining (W3 / P1b) and optimizer compile time (P2) were NOT chosen; flagged in 3a for a decision. |
| 2026-11-15 | Escape analysis (M1) approved by the owner. Optimizer compile time (P2): the owner does not care about it ("its worth it for the gains") - dropped as a work item. Larger inlining (W3 / P1b): still undecided, not in the plan. The approved plan is thirteen items and nothing is added to it without the owner's say. |
| 2026-11-15 | Larger inlining (W3 / P1b): the owner said no. Dropped. The approved plan stays at thirteen items. |
| 2026-11-15 | The optimizer stage was verified (Fable): no mistake found in its twelve passes (8 hand-written programs and 380 random programs agree between plain and `-O`, JIT and `-nojit`). The verifier ADDED loop unrolling (P6, with a loop analysis), so P6 is built ahead of the plan. Still not built: native returns (P1), loop closed form (P7), loop registers. json after verification: Sharp `-O` 7.6 s against C++ `-O` 5.2 s (one run). The verifier deliberately does not remove whole unused loops (the C++ pass does, assuming the loop ends): left as the owner's call. |
| 2026-11-15 | **P1 native returns built** (step 1 of 3a). Two parts: (a) the pass itself - twins added as ordinary functions of the executable, a call redirected when its result is only read for one number field; (b) box elimination now works on a VALUE instead of a slot (reaching definitions + an escape walk, shared in passes/calls/object_paths.sharp), because inlining and copy coalescing reuse slots and the old rule "the slot is written once" turned almost every box down - without (b) the twins still made their object. json 131072, objects made in one run: 24.39 M before, 20.20 M with (b) alone, 18.10 M with both; the C++ `-O` build makes 19.47 M. Run time on a noisy machine: json about 7.3-9.2 s with, 8.7-11.1 s without, C++ `-O` 6.6-6.9 s; primes, base64, bf within noise of before and ahead of C++ `-O`. Test: tests/codegen/cases/optimizer_native_returns.sharp. |
| 2026-11-15 | **Full gates PASS on all thirteen** (14 steps; step 14, the editor's files, is new: the IDE stream is joined). One real mistake was found on the way, by the self-hosting step: the not-null analysis took `text` and `new_array` for "never null", but on this VM an array of no elements is the null reference (`""`, `new T[0]`); the compiler built with `-O` then crashed in one of its own messages. Fixed (only `new_object` proves not-null) and added to tests/codegen/cases/optimizer_passes.sharp. Benchmarks: section 1. |
| 2026-11-15 | **Steps 2 - 8 of 3a built**, by hand, in order: pure function detection (W9); devirtualization (W1, with a null-test guard so null still raises as before); common subexpression elimination (L1) and loop-invariant code motion (O1), on a shared table of expressions; null check elimination (C1) and bounds check elimination (C2) - the bounds check turned out to be a separate VM instruction (`checklen`) the compiler emits, so no VM change was needed; escape analysis (M1: per-argument, interprocedural), dead object removal, closure elimination (M5); string concatenation flattening (M10: appending twins cut from `string.operator+` itself); loop closed form (P7: exact for every count thanks to wrap-around integers - no fallback loop). New `-n` names: pure-functions, devirtualization, common-subexpressions, null-checks, loop-invariants, bounds-checks, closure-elimination, escape-analysis, string-concatenation; loop-formula now exists. Tests: tests/codegen/cases/optimizer_native_returns, optimizer_passes, optimizer_strings, optimizer_loop_formula; tests/codegen/own/optimizer_closures (lambdas that capture numbers: the C++ compiler cannot build it). Nothing outside the approved thirteen was added; where an optimization would have needed something declined or not approved, it stops there (section 3a). |
| 2026-11-15 | Noted from the optimizer stage's own report: native returns, loop closed form, loop unrolling and loop registers were not built; stack forwarding was dropped as not applicable. The stage's independent verification had not run yet when this was written and may change some of these. |
