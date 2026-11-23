# The Benchmarks Game: Sharp and the others

2026-11-23. AMD Ryzen 7 9700X 8-Core Processor, 16 threads, 15 GB, Linux 6.18.58.1-microsoft-standard-WSL2. Sizes: full; the best of 1 run(s); the whole run is measured, start to exit.

## How many times slower than the fastest program

| language | geometric mean | median | best | worst | tests |
|---|---|---|---|---|---|
| Java | 1.0 x | 1.0 x | 1.0 x | 1.2 x | 6 of 6 |
| Rust | 1.1 x | 1.0 x | 1.0 x | 2.0 x | 6 of 6 |
| C (gcc) | 1.3 x | 1.0 x | 1.0 x | 2.8 x | 6 of 6 |
| Go | 1.3 x | 1.1 x | 1.0 x | 3.7 x | 6 of 6 |
| JavaScript (Node) | 1.4 x | 1.2 x | 1.0 x | 3.7 x | 6 of 6 |
| Lua (LuaJIT) | 2.3 x | 2.1 x | 1.1 x | 10.0 x | 6 of 6 |
| Sharp -O | 2.3 x | 2.1 x | 1.3 x | 5.5 x | 6 of 6 |
| Python (PyPy) | 2.6 x | 2.2 x | 1.4 x | 10 x | 6 of 6 |
| C# (Mono) | 2.7 x | 3.3 x | 1.1 x | 5.4 x | 6 of 6 |
| Sharp -O (before) | 3.5 x | 3.7 x | 1.7 x | 6.5 x | 6 of 6 |
| Sharp | 4.6 x | 4.9 x | 2.3 x | 9.8 x | 6 of 6 |
| Sharp (before) | 6.1 x | 6.8 x | 2.6 x | 11 x | 6 of 6 |
| Lua | 15 x | 19 x | 5.1 x | 33 x | 6 of 6 |
| PHP | 17 x | 15 x | 9.3 x | 37 x | 6 of 6 |

## nbody 50000000

| language | seconds | times the fastest | memory |
|---|---|---|---|
| Java | 1.92 | 1.0 x | 45 MB |
| C (gcc) | 1.96 | 1.0 x | 1 MB |
| Rust | 1.98 | 1.0 x | 2 MB |
| Go | 2.43 | 1.3 x | 1 MB |
| JavaScript (Node) | 2.85 | 1.5 x | 54 MB |
| Lua (LuaJIT) | 3.98 | 2.1 x | 2 MB |
| Python (PyPy) | 5.26 | 2.7 x | 67 MB |
| Sharp -O | 6.49 | 3.4 x | 7 MB |
| C# (Mono) | 6.97 | 3.6 x | 17 MB |
| Sharp | 12.06 | 6.3 x | 7 MB |
| Sharp -O (before) | 12.39 | 6.5 x | 7 MB |
| Sharp (before) | 16.96 | 8.8 x | 7 MB |
| PHP | 52.23 | 27 x | 18 MB |
| Lua | 59.36 | 31 x | 2 MB |

## spectralnorm 5500

| language | seconds | times the fastest | memory |
|---|---|---|---|
| C (gcc) | 0.89 | 1.0 x | 2 MB |
| Rust | 0.91 | 1.0 x | 2 MB |
| Go | 0.94 | 1.1 x | 1 MB |
| Lua (LuaJIT) | 0.95 | 1.1 x | 2 MB |
| Java | 0.96 | 1.1 x | 43 MB |
| JavaScript (Node) | 1.18 | 1.3 x | 52 MB |
| Python (PyPy) | 1.24 | 1.4 x | 62 MB |
| Sharp -O | 2.15 | 2.4 x | 7 MB |
| Sharp -O (before) | 2.19 | 2.5 x | 7 MB |
| C# (Mono) | 4.84 | 5.4 x | 17 MB |
| Sharp | 8.68 | 9.8 x | 7 MB |
| Sharp (before) | 9.57 | 11 x | 7 MB |
| Lua | 29.75 | 33 x | 3 MB |
| PHP | 33.14 | 37 x | 18 MB |

## fannkuchredux 12

| language | seconds | times the fastest | memory |
|---|---|---|---|
| Java | 20.35 | 1.0 x | 44 MB |
| Rust | 20.38 | 1.0 x | 2 MB |
| Go | 21.14 | 1.0 x | 1 MB |
| JavaScript (Node) | 22.35 | 1.1 x | 51 MB |
| C (gcc) | 25.83 | 1.3 x | 1 MB |
| Sharp -O | 32.68 | 1.6 x | 7 MB |
| C# (Mono) | 34.73 | 1.7 x | 16 MB |
| Sharp -O (before) | 35.39 | 1.7 x | 7 MB |
| Python (PyPy) | 39.03 | 1.9 x | 67 MB |
| Lua (LuaJIT) | 48.30 | 2.4 x | 2 MB |
| Sharp | 50.56 | 2.5 x | 7 MB |
| Sharp (before) | 52.22 | 2.6 x | 7 MB |
| Lua | 215.69 | 11 x | 2 MB |
| PHP | 262.77 | 13 x | 18 MB |

## binarytrees 21

| language | seconds | times the fastest | memory |
|---|---|---|---|
| Java | 2.52 | 1.0 x | 1689 MB |
| Rust | 5.15 | 2.0 x | 258 MB |
| C (gcc) | 7.06 | 2.8 x | 257 MB |
| C# (Mono) | 9.09 | 3.6 x | 418 MB |
| JavaScript (Node) | 9.20 | 3.7 x | 1338 MB |
| Go | 9.29 | 3.7 x | 201 MB |
| Sharp -O | 13.90 | 5.5 x | 1148 MB |
| Sharp -O (before) | 14.60 | 5.8 x | 1103 MB |
| Sharp | 18.64 | 7.4 x | 1259 MB |
| Sharp (before) | 18.95 | 7.5 x | 1259 MB |
| Lua (LuaJIT) | 25.11 | 10.0 x | 1624 MB |
| Python (PyPy) | 25.28 | 10 x | 733 MB |
| PHP | 42.11 | 17 x | 738 MB |
| Lua | 66.87 | 27 x | 1044 MB |

## mandelbrot 16000

| language | seconds | times the fastest | memory |
|---|---|---|---|
| JavaScript (Node) | 9.59 | 1.0 x | 54 MB |
| Rust | 9.68 | 1.0 x | 2 MB |
| Java | 9.74 | 1.0 x | 43 MB |
| C (gcc) | 9.82 | 1.0 x | 1 MB |
| Go | 9.92 | 1.0 x | 1 MB |
| Lua (LuaJIT) | 11.19 | 1.2 x | 2 MB |
| Sharp -O | 17.16 | 1.8 x | 7 MB |
| Sharp -O (before) | 20.74 | 2.2 x | 8 MB |
| Python (PyPy) | 24.62 | 2.6 x | 67 MB |
| C# (Mono) | 28.03 | 2.9 x | 16 MB |
| Sharp | 34.07 | 3.6 x | 7 MB |
| Sharp (before) | 43.39 | 4.5 x | 8 MB |
| Lua | 80.18 | 8.4 x | 2 MB |
| PHP | 127.73 | 13 x | 18 MB |

## fasta 25000000

| language | seconds | times the fastest | memory |
|---|---|---|---|
| Rust | 2.23 | 1.0 x | 2 MB |
| C (gcc) | 2.31 | 1.0 x | 1 MB |
| Go | 2.44 | 1.1 x | 1 MB |
| C# (Mono) | 2.44 | 1.1 x | 16 MB |
| JavaScript (Node) | 2.60 | 1.2 x | 52 MB |
| Java | 2.65 | 1.2 x | 45 MB |
| Sharp -O | 2.84 | 1.3 x | 7 MB |
| Python (PyPy) | 3.81 | 1.7 x | 65 MB |
| Lua (LuaJIT) | 4.87 | 2.2 x | 2 MB |
| Sharp | 5.21 | 2.3 x | 8 MB |
| Sharp -O (before) | 11.08 | 5.0 x | 17 MB |
| Lua | 11.35 | 5.1 x | 2 MB |
| Sharp (before) | 13.44 | 6.0 x | 18 MB |
| PHP | 20.83 | 9.3 x | 18 MB |

## Versions

- C (gcc): gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Rust: rustc 1.75.0 (82e1608df 2023-12-21) (built from a source tarball)
- Go: go version go1.22.2 linux/amd64
- Java: openjdk version 21.0.13.1 2026-11-17
- C# (Mono): Mono JIT compiler version 6.8.0.105 (Debian 6.8.0.105+dfsg-3.6ubuntu2 Sun Mar 31
- JavaScript (Node): v18.19.1
- Python (PyPy): Python 3.9.18 (7.3.15+dfsg-1build3, Apr 01 2024, 03:12:48)
- Lua (LuaJIT): LuaJIT 2.1.1703358377 -- Copyright (C) 2005-2023 Mike Pall. https://luajit.org/
- Sharp -O: sharp 4.0.9r19 build-3
- Sharp: sharp 4.0.9r19 build-3
- PHP: PHP 8.3.6 (cli) (built: Nov 18 2026 23:42:07) (NTS)
- Lua: Lua 5.4.6  Copyright (C) 1994-2023 Lua.org, PUC-Rio
- Sharp -O (before): sharp 4.0.8r19
- Sharp (before): sharp 4.0.8r19
