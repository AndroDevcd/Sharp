# Where the new parser differs from the C++ parser

The self-hosted parser (`frontend/syntax/`) follows conventional rules. This is
every place where a program is read differently, or accepted / rejected
differently, from the C++ compiler (`lib/grammar/frontend/parser/Parser.cpp`).
`grammar.md` in this folder describes what the C++ parser does.

Checked on 2026-11-12: the new parser reads all 270 files of the standard
library, the 176 files of `tests/suite`, and the bootstrap project and
benchmarks with no syntax error.

## The meaning of an expression changes

| Source | C++ parser | New parser |
|---|---|---|
| `2 ** 3 ** 2` | `(2 ** 3) ** 2` = 64 | `2 ** (3 ** 2)` = 512. `**` groups from the right, as in mathematics and in Python, Kotlin, JavaScript. |
| `!a as t` | `!(a as t)` | `(!a) as t`: a prefix operator takes the operand next to it. |
| `-a as t` | `(-a) as t` | the same. |
| `a ?: b + c` | `a ?: (b + c)` | the same. |
| `a + b ?: c` | syntax error | `(a + b) ?: c`. |
| `a * -b + c`, `x == -1 \|\| y` | were `a * -(b + c)`, `x == -(1 \|\| y)` | `(a * -b) + c`, `(x == -1) \|\| y`. **Also fixed in the C++ parser** on 2026-11-12, so both agree. |

Only the first two rows change what a program that compiles today computes.
Neither form appears in the library, the tests or the benchmarks.

## Accepted now, was a syntax error

- Assignment and `?:` anywhere their precedence allows, not only after the
  first operand: `a + b ?: c`.
- A prefix operator on any operand: `a + ++b`, `!-a`, `-a!!`.
- `as` / `is` after any operand and more than once: `s!! as string`,
  `a as x as y`.
- Calling a parenthesised value: `(f)(3)`.
- `g(a < b, c > d)`: `<` is a generic list only when the text up to the
  matching `>` can only be types AND a `(` or a `.` follows.
- An array or dictionary literal as an operand: `{1, 2}.size()`, `a == {1, 2}`.
- `for (i = 0; i < n; i++)`: the first part may be an expression.
- A comma after the last item of an array literal or an enum.
- `list<list<int>>` everywhere a type is written, including dictionary types.

## Rejected now, was accepted

- A block on the line AFTER a call is a block, not a trailing lambda:

  ```
  work(1)
  { ... }         // C++ parser: a lambda passed to work. New parser: a syntax error (missing `;`)
  ```

  A trailing lambda starts on the same line as the call.
- `get` / `set` after a LOCAL variable declaration are not accessors (they
  were, for every declaration).
- A native type or a function type written where a value is expected
  (`x := _int8;`): it parsed and was rejected later; now it is a syntax error.
- An expression that starts with `.` or `?.`.
- Stray `{ }` inside a class body.
- Modifiers in front of `mutate`, `component`, `obfuscate` (they were dropped
  silently).

## Strings

- A string with `$name` or `${...}` in it is ONE value. `"n=$n".size()` is the
  size of the finished text. The C++ tokenizer rewrote the string into
  `"n=" + n` before parsing, so the `.size()` went to `n` (and an operator
  tighter than `+` next to the string split it).
- `${ }` may contain anything an expression can, including a string with its
  own `${ }` (nesting was broken).
- A lone `$` is always a `$`. (It was dropped from any string that also had a
  backslash escape or a `$$`.)
- A backslash followed by something that is not an escape is an error
  (`"\q"`). It used to stand for the character itself. The escapes are
  `\n \t \r \0 \b \v \f \a \\ \" \'`, and `\0` is allowed in strings too.

## Numbers

- Hex digits are 0-9 and a-f (the C++ tokenizer also took g and h, and any one
  character right after `0x`). `0x` alone, and more than 16 hex digits, are errors.
- `1e2-1` is `1e2` minus `1` (it was one token with the value 100).
- Letters directly after a number are an error with an explanation (`10L`).

## `asm` blocks

- Parsed instruction by instruction against the same table the C++ parser
  uses (`frontend/syntax/assembly/asm_instructions.sharp`, generated from it). A wrong
  number or kind of operand, or an unknown instruction, is reported in the
  block, with the instruction's real form and the closest names.
- `@label + n` and `[name + n]` are not accepted (the C++ code for them
  always reported an error).

## Extension functions

- `def type.name(...)`, `def list<T>.name(...)` and `def type.operator+(...)`
  parse: the function node has the type as its first child. (The C++
  compiler stops with "internal error: unknown ast type" on these.)

## Same on purpose

- Operands still evaluate right to left; that is the code generator's
  business, not the parser's.
- Block comments nest.
- The body of `def` needs braces; the bodies of constructors, `init`, `get`,
  `set`, `if`, loops and clauses may be one statement.
- Keywords cannot be used as names.
