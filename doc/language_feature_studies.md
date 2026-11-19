# Language feature studies

Three read-only studies of the compiler and runtime, done on 2026-11-12
(commit `fc779490`). Each says how the feature would be built and what it would
cost. Nothing here is implemented. Line numbers are from that commit; the
machinery they refer to is described in `compiler_internals.md`.

Effort figures are estimates made from reading the code, not from doing the
work.

| Feature | C++ | Files | Risk |
|---|---|---|---|
| 1. Objects without `new` | 150-300 lines | 4-6 | Medium |
| 2a. Per-call closures | 1500-2500 lines (compiler), runtime optional | about 25 compiler, 0-6 runtime, about 6 library | High |
| 2b. Trailing lambdas and `it` | 350-600 lines | 7 + library | Medium-low |
| 3. Generic functions, explicit type arguments | 550-800 lines | about 15 | Medium (one high-risk step) |
| 3b. Generic functions, inferred type arguments | about 400 more | 2 | High |

---

## 1. Creating objects without `new`

`p := point(1, 2)` next to `p := new point(1, 2)`.

### What is true today

- `point(1, 2)`, `std#point(1, 2)`, `outer.inner(1)` and `list<int>(5)` already
  PARSE - as ordinary calls. No parser change and no new AST node are needed.
- Call resolution never looks at classes (dependency.cpp:2542-2786), so outside
  the class the call is "could not resolve"; `list<int>(5)` is "generic
  functions are not supported" (3248).
- Inside `class point`, a bare `point(1, 2)` finds the constructor and is
  rejected: "cannot explicitly call constructor" (3144-3149).
- There is no syntax that runs a constructor on an existing object.

### Calling a constructor explicitly

- `new T(...)` stays valid forever. It is the way to say "this is definitely
  construction", and the fix when a function shadows a class.
- A named form (`T.new(...)`) does not exist; `new` is a keyword, so it needs a
  small special case in `parseReferencePointer` and a resolver branch.
- Re-running a constructor on an existing object is blocked only by the error
  above, but a constructor body also runs the base constructor, the static-init
  check and every field initializer - it would reset the object. Not
  recommended.

### Ambiguities

- A function and a class with the same name: one case in the library -
  `coroutine_scope` is both a function (`scopes.sharp:112`) and a class. With
  functions winning and the class as a fallback, `coroutine_scope({ -> ... })`
  calls the function while `coroutine_scope()` would construct. Same spelling,
  different meaning by argument.
- Methods named like a class (30 declarations, 9 names: `iterator()`, `list(`,
  `error(`, `range(` ...) keep their meaning because methods are checked first.
- A local or field function pointer named like a class silently shadows it.
- `T(x)` cannot clash with a cast (casts are `x as T`).
- `list<int>(5)` adds no new parse ambiguity: `a < b > (c)` is already read as
  a generic call today.
- Implicit one-argument constructor conversion is unaffected.

### Plan

1. Move the class path of `compile_new_class_expression` into a shared helper.
2. In `resolve_function_reference_item`, where no function matched, look for a
   class and call the helper. Five code paths: plain global call, call through
   a module, `outer.inner(...)`, generic `list<int>(5)`, and the "inside the
   class of the same name" case (needs its own rule).
3. The new-class scheme must be NESTED as one step on the call path - its
   generator reads its steps by position, and the call path puts a label step
   first.
4. Keep the `create_dependency` calls (lazy compilation follows them).
5. Parse cache: no format change for bare `T(...)`.

Safe first step: global, non-generic `T(args)` only, tried after every
function lookup fails. This path is an error today, so no existing program can
change meaning. Generics, modules, nested classes, `T.new` and a "function and
class share a name" warning come after.

---

## 2. Closures and trailing lambdas

### 2a. Per-call closures

Today there is one closure object per fiber per enclosing function, reached
through fiber-local storage, and a lambda value is a plain number (a function
address). That is why captured values are null on another fiber, recursion
shares captures, captured parameters can crash, and native `var` cannot be
captured.

Recommended design - an environment per call plus "fat" function values:

- At the entry of a function that has captures, allocate its `closure_F`
  object into a hidden local. Captured locals and parameters live ONLY in that
  object, so the write-through code in the assign schemes disappears and
  capturing a native `var` becomes possible.
- A capturing lambda is compiled as an instance method of `closure_F`
  (`self` = the environment), so a captured access is an ordinary field access.
  Nested functions get a parent link.
- A function value becomes an object `{ address, environment }`.
- A call through it pushes the environment (or null), the arguments, and emits
  the EXISTING `calld`. No new opcode is strictly needed.
- A plain static function used as a value needs a small generated thunk; it
  and every non-capturing lambda use one cached function object. Direct calls
  `foo()` are untouched and cost what they cost today.

The rejected alternatives: keeping numbers and passing the environment through
a handle table (invisible to the reference counts: freed too early or leaked),
and lambda lifting (cannot cross a function-pointer boundary; only useful as an
optimization on top).

Main risks: `type_function_ptr` sits in the numeric range of the type enum and
is treated as a number in about 40 places; the calling convention for plain
functions versus lambdas; class fields of function type are pre-boxed as
numbers by the runtime; reference cycles (an environment holding a lambda that
holds the environment) are never freed; the cost of calls through a pointer.

Order:
1. Tests first (below).
2. Quick fix of the captured-parameter bug (static access to a fiber-local
   field in `function_generator.cpp` and `scheme_compound_assign_value.cpp`).
3. Fat function values with no captures: thunks, cached objects, field and
   array types, platform and vm fixes. Measure the cost.
4. Per-call environment, lambdas as instance methods, fiber-local reference
   removed.
5. `var`, parameter, `self` and nested capture.
6. Move the known bug into the suite; rewrite the coroutine "closure rule".
7. Optional fast-path opcode and JIT work.

Tests to write first: `make_counter()` called twice with independent counts;
recursion capturing a local; capture across fibers; capturing a parameter;
returning a lambda; a capturing lambda stored in a field and called after its
function returned; capturing and reassigning a `var`; two-level nesting; `self`
in a method's lambda; function pointer `== null`, equality, `fns[0](4)`; fiber
and thread entry points; many lambdas plus a GC pass; a micro-benchmark of
calls through a pointer - all with and without `-O` and the JIT.

### 2b. Trailing lambdas with `it`

`launch { delay(10) }`, `numbers.filter { it > 3 }.map { it * 2 }`.

- Hook: in `parseDotNotCallExpr`, after the optional `(...)`: if `{` follows,
  parse a lambda and append it to the call's argument list (creating the list
  when there were no parentheses), then continue the `.` chain.
- A bare `{ ... }` in trailing position is always a lambda; no `->` needed and
  the last expression without `;` is the result. A bare `{ x }` as an ordinary
  argument stays a vector array literal.
- No statement form conflicts: in `if`, `while`, `lock`, `foreach`, `catch`,
  `for`, `when`, method bodies and the rest, a `)` , keyword, `->` or `;` always
  stands between an expression and the following `{`. A lambda directly after a
  call is a parse error today, so it is free to claim. Exclude `new T(...) {`.
- `it`: mark implicit lambdas in the AST (parse cache format bump), scan the
  body for an unshadowed `it`, let matching accept 0 or 1 parameters (prefer 1
  when `it` is used), and let the existing `fully_qualify_function` add the
  parameter and copy the return type. Compilation of the body must wait until
  then.
- Library: `numbers.filter { }` needs instance methods; `map` to another type
  needs generic functions (study 3).
- `launch { hits.increment() }` with a captured local only works after 2a.

---

## 3. Generic functions

`def map<T, R>(items: list<T>, f: (T)(R)) : list<R>`, called as
`map<int, string>(xs, f)` and later `xs.map(f)`.

### What is true today

- The expression parser already reads `f<int>(x)` as a call; the compiler
  stops it on purpose ("generic functions are not supported.").
- Generic classes do not substitute type parameters into a copy of the code:
  the shared syntax tree is processed again per instance and `T` is looked up
  by name. The same mechanism works for functions.

### Design (monomorphisation, reusing the class machinery)

- Parser: in `parseMethodDecl`, when `<...>` followed by `(` comes after the
  name, parse a generic identifier list. No new node type is needed. Split the
  `>>` token in the two generic-argument checks. Bump the parse cache format.
- Data: a function blueprint with its AST and type parameter names, kept in a
  separate list on the class (out of `functions`, so normal lookup never sees
  it); instances named `map<std#int, std#string>` and cached on the blueprint.
- Instantiation, modelled on late class clones: take the global lock, isolate
  the compile context, switch to the blueprint's file and module, push the type
  bindings, run the existing function processing under the instance name,
  compile the body at once if the file is ready.
- Substitution: a stack of type bindings checked by `resolve_normal_item`
  before the class's own type parameters; pushed for instantiation, body
  compilation and inferred return types. Deferred library bodies keep working
  because the bindings live on the function.
- Call resolution: instead of the two errors, carry the generic arguments on
  the call item; find blueprints by name and arity, instantiate, then call the
  existing `resolve_function` - overloading, conversions and lambda typing are
  reused unchanged.

### Inference (second version)

Unify the blueprint's parameter syntax with the argument types: bare `T`,
`T[]`, `C<P...>` (walk the argument's class, base class and interfaces for a
clone of `C`), function types `(P...)(R)`. Two passes, plain arguments first,
then lambdas. Block lambdas have no known return type, so some calls will need
explicit type arguments. Literals have to be mapped to their box classes.

### Obstacles

1. A generic instance created late sees the locals of the function that
   triggered it (the context is per file and keeps its locals). BLOCKS generic
   functions; must be fixed first. Small (60-100 lines) but high risk, because
   closures rely on the same lookup through saved contexts.
2. `mutate` members are missing on generic clones created later. Blocks
   `xs.map(f)` written as a mutate on `list`; not free functions. Small fix.
3. `>>` in nested generics. Small parser fix; do it first.
4. Native type arguments: partial today (`T?` with a native `T` is rejected);
   does not block.
5. `==` on `T` behaves differently per instance; inherent, document it.
6. Threads: instantiation borrows another file's context; hold the global lock.

### Steps

| Step | C++ lines | Risk |
|---|---|---|
| 1. Context isolation for late generic class clones | 60-100 | High |
| 2. Split `>>` | 20-40 | Low |
| 3. Parse `def f<T>(...)`, store the blueprint | 80-120 | Low |
| 4. Explicit calls `f<int>(x)` for free functions | 250-350 | Medium |
| 5. Methods, methods in generic classes, `:=`, lambdas, lazy bodies | 80-150 | Medium |
| 6. Mutate fix, generic methods in `mutate` | 30-60 | Medium |
| 7. Inference | 350-450 | High |

### What it enables in the library

Five classes exist only to carry static generic helpers - `collections<T>`
(45 functions), `lists<T>` (29), `arrays<T>` (21), `transforms<T,R>` (9),
`async<t>` (6): 110 functions that could become ordinary generic functions and
methods.

---

## How the three fit together

- Study 3's first step (context isolation) and study 2a both change how a
  nested compile sees outer locals. Do them with the other in mind.
- Trailing lambdas (2b) are cheap and independent, but only pay off fully with
  per-call closures (2a) and, for `map`, generic functions (3).
- Objects without `new` (1) is independent of the other two.
