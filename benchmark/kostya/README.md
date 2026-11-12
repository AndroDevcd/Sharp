# kostya benchmarks in Sharp

Sharp versions of the programs from <https://github.com/kostya/benchmarks>:
brainfuck (`bench.b`, `mandel.b`), base64, json, matmul and primes.

```
SHARPC=<sharpc> SHARP=<sharp> LIB=<lib/support/0.3.0> OUT=<folder> ./run.sh plain
SHARPC=<sharpc> SHARP=<sharp> LIB=<lib/support/0.3.0> OUT=<folder> ./run.sh O -O
```

`run.sh` explains its options. Each line of the results file is
`name | input | checked output | seconds | peak memory (MB)`. The seconds are
what the program measures around the work itself, as in the original
benchmark. The json input has 131,072 entries (a quarter of the original).

## Results

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
