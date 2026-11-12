# Known bugs

Every file in this folder is a reproduction of a bug that is still open. Each one
is **expected to fail** (a failed check, a compile error, a crash or a hang) and
is not run with the normal suite. When a bug is fixed, move its checks into
`../cases/` and delete it from this list.

Status below was checked on 2026-11-12 against commit `0d2ceadf` (all 124 files
in `../cases/` pass on that build, with and without `-O`). "All configurations" means with and without
`-O`, with and without the JIT.

To run one by hand:

```
sharpc tests/suite/known_bugs/<name>.sharp tests/suite/testing.sharp -o /tmp/bug
sharp /tmp/bug
```

## Needs a decision, not (only) a fix

| Behaviour | Where |
|---|---|
| Binary operands are evaluated right to left: `mark(1) + mark(2)` runs `mark(2)` first. | `operators.sharp` |
| Assigning to a non-nullable string variable/field calls `operator=` on the object it holds, so every other reference to that string changes too. (Null elements and nullable targets now get a new object.) | described in `lib_coroutines` report; no file |
| `override` only applies to interface / delegate methods; a plain base-class method is bound statically. Documented as the current design in `cases/inheritance.sharp`. | - |

## By design (not bugs)

- Enum members without an explicit value get a value that depends on when the
  enum is compiled; they do not count up from 0. Give members explicit values
  when the number matters.
- `override` only applies to interface / delegate methods.

## Compiler: wrong code or wrong result

| Bug | File |
|---|---|
| A sized parameter is not cut to its size: `def f(x: _int8)` called with 200 sees 200. (A callee-side fix was tried and reverted: the `long` box picks an `_int32` constructor for big literals and relied on this.) | `casts.sharp` |
| `0.1 + 0.2 == 0.3` differs between folded literals and run time. | `comparison.sharp` |
| An enum declared inside a class cannot be named from outside (`holder.inner.off`). | - |
| An interface method overridden in a subclass is ignored when called through the intermediate class type, or from a method of that class. | `interfaces.sharp` |
| A field with both `get` and `set`: assignments from outside are lost under `-O`; an assignment also runs the getter; `p.value++` gives a wrong number. | `properties.sharp` |
| A try/finally inside a finally body drops the pending exception. | `finally.sharp` |
| A lock is not released when an exception leaves the block; nested locks on one object are not counted. | `lib_coroutines_lock_exception.sharp` |
| `x = x + n` on a `localized` number stores garbage. | `lib_coroutines_localized_arithmetic.sharp` |
| `names : string[] = { "a", "b", "c" };` (no `new`) makes a 1-element array. | `lib_time_array_initializer.sharp` |
| A local `int?` / `int[]?` declared without a value is not null; `obj.int_field = 500` overwrites the number inside a shared `int` object. | `lib_collections_language.sharp` |
| `"" + (2 + 3)` prints `5.0000000000000000` although `"" + 5` prints `5`; `s += 5`, `s += s` and `s += <int object>` add nothing useful; comparing a null string with `null` calls `operator==` and throws. | `strings_basic.sharp` |
| `empty_string == null` is true (it calls `string.operator==`). `deferred_init<string>` throws for the same reason. | `lib_io_language.sharp`, `std_misc.sharp` |
| A number is silently accepted for a `string` parameter (it becomes a one-character string with that code). | `lib_text_literals.sharp` |
| `a != b` on two `double` objects does not compare the values (`number` has `operator==(double)` but no `operator!=`), so `nan != nan` is false. | `lib_math_literals.sharp` |
| A failing cast throws `illegal_state_exception`; `class_cast_exception` is never thrown. | `exceptions.sharp` |
| A string literal is accepted for a `string[]` parameter and crashes at run time. | `lib_text_literal_as_array.sharp` |
| A lambda that captures a variable is null on another fiber (closures are per function, held in fiber-local storage). | `lib_coroutines_closure_across_fibers.sharp` |
| Generic classes: `mutate` members are missing on instantiations that only user code makes (`list<card>`). | `lib_collections_mutate_generic.sharp` |

## Compiler: does not compile / compiler stops

| Bug | File |
|---|---|
| A number literal is ambiguous between a `var` overload and a class overload (`d(5)` with `d(var)` and `d(string)`); also hits constructors and operators. | `functions.sharp` |
| A class with a one-number constructor is implicitly convertible from a number, making `of(var, var, var)` / `of(var, month, var)` ambiguous. | `lib_time_implicit_number_to_class.sharp` |
| A generic class sees the locals of the function that instantiates it, and they hide its fields. `box<box<var>>` does not parse (`>>`). | `generics.sharp` |
| `p.value += 5` on a property, and assigning a getter-only field from outside: code generator fatal error. | `properties.sharp` |
| `new coffee { water_left = 100 }` (inline initialization, shown in `examples/new_class.sharp`) does not parse. | - |
| A lambda body containing `"literal" + s` (in some generic contexts): compiler segmentation fault. | `lib_collections_lambda_concat.sharp`, `lib_math_lambda_concat.sharp` |
| `f(')')`, `"x" + maybe!!)`, `"abc".size()` do not parse; a `')'` char literal as an argument is a parse error. | `lib_io_syntax.sharp`, `lib_time_paren_char_literal.sharp` |

Not in a file (they cannot be written as a failing check, or were only described):
a function pointer called straight out of an array (`fns[0](4)`); a ternary mixing
a string literal and a `string`; a one-pair dictionary literal; `$` cannot appear in
a string literal; block comments nest (`/*` inside a comment swallows the file);
`list<list<string> >` killed the compiler; members added by one `mutate` file are
not always visible inside another `mutate` file of the same class; `abs(3.5)` and
`sb.find("b")` are ambiguous calls; `atomic<var>` does not compile; a negative
literal in a longer condition (`c == -1 || x`) is read as `c == -(1 || x)`.

## Runtime and JIT

| Bug | File |
|---|---|
| A fiber switched out while one function is on its stack twice never continues (hang without the JIT, crash with it): `main_vm_loop` finds a frame by function, not by index. This is why coroutine scope functions cannot be nested directly. | `lib_coroutines_recursive_suspend.sharp` |
| JIT crash in `return_method()` when two sibling coroutines take turns and one yields twice. | `lib_coroutines_jit_fiber_frames.sharp` |
| JIT crash when functions are first compiled while several worker threads are inside them (the coroutine tests warm up on one thread to avoid it). Not reduced to a small program. | - |
| Deep recursion ends in SIGSEGV instead of a stack overflow exception. | described in `cases/recursion.sharp` |
| The directory call returns only regular files of a whole tree and skips dot names; no environment-variable call; read-file returns null for empty and missing alike. | worked around in `std/io/files` |

## Library

| Bug | File |
|---|---|
| `new result<T, E>(data, null)` does not compile (the constructor takes `E`, not `E?`). | `std_misc.sharp` (comment) |
| Existing `read_line()` loops forever at end of input; `print_numbers(_int16[])` etc. call themselves. | `std/console.sharp` |
| `file.get_free_space()` returns <= 0 under `-O` (asm loads an int object). | `std/io/file.sharp` |
| `atomic.sharp`: `<=` is implemented with `>=`. | `std/io/atomic.sharp` |
| The old `std.io.coroutines` and the new `std.coroutines` both define `job`, `deferred` and `channel`; importing both in one file is ambiguous. | - |

## Fixed (kept here so the history is easy to find)

Moved into `../cases/` after they started to pass: `threads.sharp` (scheduler
deadlock, `join()` and `start()` results), `logic_precedence.sharp`,
`variables_multiple.sharp`, `operators_on_new.sharp`, `casts_of_expressions.sharp`,
`new_in_condition.sharp`, `foreach_over_new.sharp`, `foreach_inherited_loopable.sharp`,
`thread_fiber_id.sharp`, `string_plus_empty_literal.sharp`, `arrays_sized_elements.sharp`,
`non_null_index*.sharp`, `init_blocks_order.sharp`, `init_blocks_locals.sharp`,
and the `*_fixes.sharp` files.
