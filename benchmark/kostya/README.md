# kostya benchmarks in Sharp

Sharp versions of the programs from <https://github.com/kostya/benchmarks>:
brainfuck (`bench.b`, `mandel.b`), base64, json, matmul and primes.

```
SHARPC=<sharpc> SHARP=<sharp> LIB=<lib/support/0.4.0> OUT=<folder> ./run.sh plain
SHARPC=<sharpc> SHARP=<sharp> LIB=<lib/support/0.4.0> OUT=<folder> ./run.sh O -O
```

`run.sh` explains its options. Each line of the results file is
`name | input | checked output | seconds | peak memory (MB)`. The seconds are
what the program measures around the work itself, as in the original
benchmark. The json input has 131,072 entries (a quarter of the original).

`./run.sh c` runs the C programs that sit next to the Sharp ones (`bf.c`,
`base64.c`, `json.c`, `matmul.c`, `primes.c`): the same work on the same
input, timed the same way, without a runtime. C has no base64, JSON or hash
map of its own: the first two are plain loops written in the file, and a
node of the primes tree keeps its children in a ten-slot array.

## Results, 2026-11-23 (later): Sharp 0.5.9 (VM 4.0.10r19), and C

The same machine, one run each, the same checked output everywhere.

| test | 0.5.8, `-O` | 0.5.9, `-O` | 0.5.8, no `-O` | 0.5.9, no `-O` | C (gcc -O3) | Sharp `-O` / C |
|---|---|---|---|---|---|---|
| brainfuck `bench.b` | 3.59 s | 3.00 s | 8.53 s | 7.91 s | 0.72 s | 4.2 x |
| brainfuck `mandel.b` | 70.88 s | 69.70 s | 100.11 s | 107.66 s | 5.08 s | 13.7 x |
| base64 | 4.36 s | 4.52 s | 7.93 s | 8.01 s | 0.76 s | 6.0 x |
| json | 0.36 s | 0.35 s | 0.62 s | 0.59 s | 0.12 s | 2.9 x |
| matmul | 4.55 s | 4.56 s | 13.04 s | 7.38 s | 1.26 s | 3.6 x |
| primes | 0.54 s | 0.57 s | 1.00 s | 0.87 s | 0.06 s | 10.1 x |

Over the six, Sharp `-O` is 5.8 times C (5.9 with 0.5.8) and Sharp without
`-O` 10.2 times (11.6). What 0.5.9 changed - floor, ceil, abs and the other
math functions as instructions, `%` in machine code, decimals copied from
register to register, a text's hash in whole numbers - shows where a program
copies decimals without `-O` (matmul) and hardly anywhere else here: these six
use none of the math functions and no text keys. With the six programs of
benchmark/game, twelve in all, Sharp `-O` is 3.2 times C and Sharp without
`-O` 5.6 times.

## Results, 2026-11-23: Sharp 0.5.8 (VM 4.0.9r19), and C

x86-64 Linux under WSL2 (AMD Ryzen 7 9700X, 15 GB), one run each. Every
program printed the same checked output: Sharp with and without `-O`, and C.

| test | input | 2026-11-12, `-O` | now, `-O` | faster by | now, no `-O` | C (gcc -O3) | Sharp `-O` / C |
|---|---|---|---|---|---|---|---|
| brainfuck `bench.b` | - | 8.5 s | 3.59 s | 2.4 x | 8.53 s | 0.73 s | 4.9 x |
| brainfuck `mandel.b` | - | 121.2 s | 70.88 s | 1.7 x | 100.11 s | 5.12 s | 13.8 x |
| base64 | 131072 bytes x 8192 | 20.8 s | 4.36 s (encode 1.73, decode 2.63) | 4.8 x | 7.93 s | 0.76 s | 5.7 x |
| json | 131072 entries | 6.6 s | 0.36 s | 18.6 x | 0.62 s | 0.13 s | 2.8 x |
| matmul | 1500 x 1500 | 16.4 s | 4.55 s | 3.6 x | 13.04 s | 1.29 s | 3.5 x |
| primes | up to 5000000 | 2.8 s | 0.54 s | 5.2 x | 1.00 s | 0.05 s | 11.5 x |

Over the six, Sharp `-O` is 5.9 times C (geometric mean); in November's
numbers it was about 26 times these C times.

Peak memory with `-O`, November -> now: 12 -> 8 MB (`bench.b`), 13 -> 10 MB
(`mandel.b`), 21 -> 33 MB (base64), 824 -> 225 MB (json), 81 -> 76 MB
(matmul), 526 -> 319 MB (primes). C: 1, 2, 2, 168, 70 and 62 MB.

What the month between the two columns changed: slot-form instructions,
direct calls and numbers kept in registers by the JITs (0.5.3); json four
times faster, text literals shared, files read in one block (0.5.4);
per-thread allocation (after 0.5.4); the x64 JIT's register fixes (0.5.8).
The two tests furthest from C are the ones made of calls on small objects
(`mandel.b`: a recursive interpreter) and of hash maps (primes: C uses an
array where Sharp uses `hash_map`).

## Results, 2026-11-12

x86-64 Linux under WSL2 (3 GB of memory), one run each, 2026-11-12. The
checked output is the same in every column.

| test | input | original runtime, no `-O` | now, no `-O` | now, `-O` |
|---|---|---|---|---|
| brainfuck `bench.b` | - | 176.7 s | 14.6 s | 8.5 s |
| brainfuck `mandel.b` | - | 1646.8 s | 150.5 s | 121.2 s |
| base64 | 131072 bytes x 8192 | - | 32.0 s (encode 13.8, decode 18.2) | 20.8 s (encode 10.3, decode 10.5) |
| json | 131072 entries | - | 9.8 s | 6.6 s |
| matmul | 1500 x 1500 | - | 21.7 s | 16.4 s |
| primes | up to 5000000 | - | 4.6 s | 2.8 s |

Peak memory with `-O`: 12 MB (`bench.b`), 13 MB (`mandel.b`), 21 MB (base64),
824 MB (json), 81 MB (matmul), 526 MB (primes).

"original runtime" is the build before November 2026: reference counting,
every number an 80-bit value, the old JIT. Only the two brainfuck programs were
ever run to the end on it at full size.

How `bench.b` got here, without `-O`: 176.7 s -> 107.9 s when the tracing
garbage collector replaced reference counting -> 14.6 s with typed numbers,
number fields stored in the object, and the new JIT (the interpreter alone,
`-nojit`, takes 89.2 s).

base64, json and primes were 82.6 / 13.2 / 13.7 s without `-O` (70.3 / 8.5 /
12.0 s with it) on the same runtime before the standard library was gone
over: counts and indexes as whole numbers instead of decimals, base64 writing
into a byte array instead of calling a function per character, a small
default table and an object-free bucket index in `hash_map`, and a `list.at`
that takes a plain number.

For comparison, the figures of the original project (2026-07-19, Intel Xeon
E-2324G, a different machine): Python takes 61.0 s for `bench.b`, 3.1 s for
base64, 0.94 s for the full-size json, 136.3 s for matmul and 1.7 s for
primes; C (gcc) 1.1 s, 1.1 s, 0.13 s, 3.0 s, and C++ 0.06 s for primes.
