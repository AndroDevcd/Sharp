# Sharp performance work, night of 2026-11-19 to 2026-11-20

Everything here is in the working tree on `runtime-3.0.0_r8`, **not committed** (40 files changed, 5 new,
about 2,900 lines). The released 0.5.2 is the "before" in every table. The step-by-step log of every try, kept
or reverted, is `build/perf/LOG.md`; patch snapshots of each kept step are in `build/perf/snapshots/`.

Every benchmark program was compiled with `-O`, before and after.

## 1. Result

Typical program: **1.2 to 2.4 times faster** than 0.5.2. Code that is number loops over decimals, virtual
calls or lambda calls: **5 to 9 times**. Programs that mostly allocate objects or parse text: **almost
unchanged**.

### 1.1 Windows, native (installed 0.5.2 against this tree), seconds

| Program | 0.5.2 | Now | Faster by |
|---|---|---|---|
| brainfuck `bench.b` (kostya) | 5.91 | 4.32 | 1.37x |
| base64 (kostya) | 8.67 | 5.02 | 1.73x |
| matmul (kostya) | 11.75 | 5.00 | 2.35x |
| primes (kostya) | 0.74 | 0.69 | 1.07x |
| n-body | 2.17 | 1.56 | 1.39x |
| spectral norm | 8.50 | 0.96 | 8.85x |
| fannkuch | 4.59 | 2.88 | 1.59x |
| binary trees | 1.80 | 1.78 | 1.01x |
| recursive Fibonacci (42) | 3.61 | 2.98 | 1.21x |
| sieve | 2.46 | 1.33 | 1.85x |
| Mandelbrot | 2.65 | 1.96 | 1.35x |
| strings and lists | 1.27 | 0.97 | 1.31x |
| virtual calls | 1.97 | 0.41 | 4.80x |
| lambda calls | 5.18 | 1.08 | 4.80x |

The last ten were written tonight from memory as a held-out set (`build/perf/extra/`): no optimization was
tuned on them. They moved as much as the kostya programs did, which is the evidence that the work is general.

### 1.2 Linux (WSL), seconds

| Program | 0.5.2 | Now | Faster by |
|---|---|---|---|
| brainfuck `bench.b` | 5.48 | 3.71 | 1.47x |
| brainfuck `mandel.b` | 95.2 | 72.0 | 1.32x |
| base64 | 8.12 | 4.42 | 1.83x |
| json | about 7.0 | about 6.7 | 1.05x |
| matmul | 11.61 | 4.78 | 2.42x |
| primes | 0.55 | 0.51 | 1.07x |
| loop of decimals | 9.93 | 1.47 | 6.75x |
| loop over an array | 2.87 | 1.46 | 1.96x |
| loop over objects' fields | 1.16 | 0.77 | 1.50x |
| loop with remainder | 2.50 | 1.97 | 1.26x |
| 100 million small calls | 2.20 | 1.77 | 1.24x |
| n-body / spectral / fannkuch | 1.87 / 8.61 / 4.60 | 1.30 / 0.87 / 2.81 | 1.43x / 9.89x / 1.63x |
| trees / fib / sieve | 1.15 / 3.25 / 2.32 | 1.12 / 2.64 / 1.32 | 1.02x / 1.23x / 1.75x |
| mandel / strings / virtual / lambdas | 2.63 / 1.16 / 0.96 / 2.31 | 1.90 / 0.76 / 0.35 / 0.96 | 1.38x / 1.52x / 2.74x / 2.40x |

### 1.3 Against other languages (kostya programs, Linux, each run alone on a quiet machine), seconds

| Program | C | Go | Java | Node | LuaJIT | PyPy | Sharp 0.5.2 | Sharp now | Sharp now / Node |
|---|---|---|---|---|---|---|---|---|---|
| `bench.b` | 0.72 | 0.77 | 0.94 | 1.82 | 3.57 | 6.34 | 5.48 | 3.71 | 2.0x |
| `mandel.b` | 6.22 | 7.25 | 6.74 | 19.05 | 21.71 | 29.91 | 95.2 | 72.0 | 3.8x |
| base64 | 0.80 | 1.37 | 1.25 | 1.86 | - | 2.07 | 8.12 | 4.42 | 2.4x |
| json | - | 0.56 | - | 0.51 | - | 0.70 | 7.0 | 6.7 | 13x |
| matmul | 1.26 | 1.28 | 1.41 | 1.44 | - | 1.84 | 11.61 | 4.78 | 3.3x |
| primes | 0.06 (C++) | 0.08 | 0.15 | 0.24 | 0.31 | 0.69 | 0.55 | 0.51 | 2.1x |

Sharp is now ahead of PyPy on `bench.b` and level with LuaJIT there; it is 2 to 4 times behind Node on
computation and 13 times behind on json. (Earlier tonight I quoted Node figures taken while other programs
ran; these solo figures are faster, so the distance to Node is larger than I said then.)

The slower languages, also each run alone (`build/perf/results_others_2.txt`); a dash is a program that has
no version in that language or did not build or run here:

| Program | Rust | C# (Mono) | Lua | Ruby `--jit` | Python | Ruby | PHP | Tcl | Perl |
|---|---|---|---|---|---|---|---|---|---|
| `bench.b` | 0.73 | - | 24.56 | 31.54 | 37.00 | 53.85 | - | 109.87 | 142.40 |
| `mandel.b` | 9.86 | - | - | - | - | - | - | - | - |
| base64 | 0.50 | 4.21 | - | 1.11 | 1.83 | 0.98 | - | 1.97 | - |
| json | - | - | - | 1.59 | 1.02 | 1.28 | 0.79 | - | - |
| matmul | 1.24 | 7.61 | - | 97.52 | 91.33 | 126.52 | - | - | 112.19 |
| primes | 0.10 | - | 0.99 | 1.99 | 1.76 | 1.50 | - | - | - |

### 1.4 Against LuaJIT, program by program (Linux, each run alone), seconds

The held-out programs were ported to LuaJIT (`build/perf/extra_lua/`, same algorithm and sizes, same printed
results), plus matmul and base64; `bench.b` and primes are kostya's own LuaJIT versions. Rig:
`build/perf/lua_vs.sh`.

| Program | Sharp | LuaJIT | Sharp / LuaJIT | |
|---|---|---|---|---|
| sieve | 1.33 | 6.64 | 0.20 | Sharp 5x faster (a byte array against a Lua table of numbers) |
| base64 | 4.46 | 17.15 | 0.26 | Sharp 3.8x faster (byte arrays against Lua tables and strings) |
| binary trees | 1.13 | 2.69 | 0.42 | Sharp 2.4x faster |
| fannkuch | 2.81 | 3.21 | 0.87 | Sharp slightly faster |
| `bench.b` | 3.76 | 3.54 | 1.06 | level |
| virtual calls | 0.34 | 0.28 | 1.21 | LuaJIT slightly faster |
| primes | 0.63 | 0.36 | 1.75 | LuaJIT faster |
| spectral norm | 0.87 | 0.37 | 2.35 | LuaJIT faster |
| matmul | 4.78 | 1.76 | 2.71 | LuaJIT faster |
| n-body | 1.32 | 0.46 | 2.86 | LuaJIT faster |
| Mandelbrot | 1.87 | 0.56 | 3.33 | LuaJIT faster |
| recursive Fibonacci | 2.64 | 0.79 | 3.34 | LuaJIT faster |
| strings and lists | 0.76 | 0.12 | 6.33 | LuaJIT faster (interned strings, `table.concat`) |
| lambda calls | 0.98 | 0.07 | 14.0 | LuaJIT faster (it inlines the closure into the loop) |

Sharp wins where the program is arrays of bytes or allocation; LuaJIT wins by 2 to 3.5 times on floating-point
loops and recursion, and by much more where its tracing compiler removes a call entirely.

## 2. What was built

1. **Slot forms: 77 new VM instructions (0xa4 to 0xf0).** The compiler used to turn `d = a + b` into four
   instructions through VM registers (load, load, add, store). Now it is one instruction on the three frame
   slots. Covers arithmetic, bit operations, comparisons, array elements, fields, copies, returns; a comparison
   joins the jump that follows it; a small constant joins the comparison or the arithmetic. Implemented in the
   interpreter, the x64 JIT and both ARM JITs. Documented in `lib/compiler/doc/codegen.md` section 4.10.
2. **Variables in CPU registers (x64 JIT).** The four whole-number and four decimal slots a function uses most
   live in CPU registers. Every write still goes to memory too, so the interpreter, the collector and exceptions
   see the frame exactly as before.
3. **Calls (x64 JIT).** A lean return from compiled-to-compiled calls; virtual calls through a small cache at
   each call site (4 classes); lambdas and function values called directly instead of through the runtime.
4. **Smaller things.** Results computed directly in their register; conditional jumps go straight to their
   target; shorter null and overflow checks; `list` accessors can now be inlined (library).
5. **Fixes found on the way.** The kind-check VM did not compile (`tk_obj`); the optimizer's hand-off rule
   missed a case; the recursion limit of compiled code (below).

Tried and reverted: a higher inlining limit (no clear gain), six whole-number registers instead of four (calls
got slower).

## 3. What supports what

| | Windows x64 | Linux x64 | ARM64 | ARM32 |
|---|---|---|---|---|
| Slot forms (compiler, interpreter, JIT) | yes | yes | yes | yes |
| Direct calls, lean returns, virtual-call cache, direct lambda calls | yes | yes | yes | yes |
| Compare-and-jump as one comparison and one jump | yes | yes | yes | yes |
| Variables in CPU registers | yes (4 whole + 4 decimal) | yes | yes (4 whole + 4 decimal) | decimals only (4) |
| Results computed directly in the variable's register | yes | yes | yes | decimals only |

ARM64 has the same register work as x64, picked by the same rule. On 32-bit ARM a whole number takes two of
the 14 registers and none are spare, so whole numbers stay in memory there; decimals live in d8 to d11, which
the floating point unit has free.

## 4. Tests on the final code

- Linux, full test set (`test.sh full`, all 15 parts incl. self-hosting and both kind checks): **pass**. That
  run was before the last change (the 2 MB thread stack); after it, the gate was run again: suite 186 cases x 4
  ways and the short tests, **pass**.
- Windows suite: 186 cases x 4 ways = **744 of 744**, on the final code.
- ARM64 and ARM32 under qemu, with direct calls and the straight jumps: 185 of 186 in each of the 8 runs.
  The one failure is `lib_system_subprocess`, in every mode (with and without the JIT). Before direct calls
  the ARM64 JIT also failed `jit_calls` (its 20,000-deep recursion); it passes now. ARM speed was not
  measured: qemu's timings say little about a real Pi.
- ARM with the local homes (the last change): the suite again, 185 of 186 in each of the 8 runs; and ten
  benchmark programs on both ARMs print what the PC prints, with homes and without
  (`build/perf/arm_homes_check.sh`). Under qemu the ARM64 runs take 2 to 16% less time with homes, ARM32
  about the same: a hint only, a Pi has to say.
- Two suite cases were added: `jit_calls` (7 classes at one call site, lambdas, exceptions across direct calls,
  deep recursion) and `jit_constants` (constant comparisons incl. not-a-number, bit operations, nulls).
- Not run: the GUI examples by hand, the CLion plugin, real ARM hardware.

## 5. Things to know before committing

- **The default thread stack is now 2 MB (was 1 MB).** A call in compiled code takes 32 bytes of machine stack
  instead of 16, which halved how deep compiled code could recurse (16,000 passed, 32,000 did not; 0.5.2 passed
  32,000). With 2 MB it reaches what 0.5.2 did. Only addresses are reserved; memory is used as the stack
  grows. `-stack` still overrides it.
- **`-O` stays an option** (your decision). Without it the same programs run about 1.2 to 2 times slower, one
  string-heavy test 14 times slower, because the joined compare-and-jump instructions and inlining need the
  optimizer. Benchmarks should always be built with `-O`.
- The version numbers were not moved. These changes touch the VM, the compiler and the library, so a release
  would move the VM revision and the language patch number.
- A crash in `thread_exit` seen once on ARM64 under qemu earlier in the session (not reproduced in 20 runs)
  is still unexplained.

## 6. Where the remaining distance to Node is

| Gap | What closes it | Size |
|---|---|---|
| Loops, 2 to 4x | An optimizing tier for hot loops: keep values in registers without writing back, hoist repeated null and bounds checks, inline calls | weeks |
| json, 13x | A string type with spare capacity so appends do not reallocate, and a parser written against it | days |
| Allocation (trees, primes: unchanged tonight) | Fields inside the object, an allocator the JIT can inline | days to weeks |
| Calls, about 3 ns each | Not keeping the VM's separate frame record on every call | days |
| Unoptimized builds | Let plain builds join compare-and-jump (needs a liveness check outside `-O`) | a day |
| ARM | Measure on a Pi; a register plan for ARM32 that leaves room for whole-number variables | days |

## 7. Where things are

| What | Where |
|---|---|
| Log of every try | `build/perf/LOG.md` |
| Raw results | `build/perf/results.txt`, `results_extra.txt`, `results_others.txt` |
| Benchmark rigs | `build/perf/bench.sh`, `extra.sh`, `windows_bench.sh`, `others_alone.sh`, `depth_probe.sh` |
| Held-out programs | `build/perf/extra/*.sharp` |
| Snapshots (patch files) | `build/perf/snapshots/` (latest: 14) |
| Instruction reference | `lib/compiler/doc/codegen.md` 4.10 and 7.3 |
