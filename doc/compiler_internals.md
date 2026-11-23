# How the Sharp compiler and runtime work

A walk-through of the compiler (`lib/grammar`) and the parts of the runtime
(`lib/runtime`) that the language features depend on. It was written from three
read-only studies of the code on 2026-11-12 (commit `fc779490`); line numbers
are from that commit and will drift. Everything here was read in the code, not
measured or run. Where something is an inference rather than a fact it says so.

Contents

1. The compile pipeline, stage by stage
2. Parsing an expression
3. Expressions, schemes and steps
4. A call, step by step
5. Name lookup
6. Overload resolution and type matching
7. Types (`sharp_type`)
8. A function from declaration to bytecode
9. Constructors, init functions and static init
10. `new` expressions
11. Generic classes
12. Lambdas and the closure mechanism
13. Function pointers: compiler, bytecode, interpreter, JIT
14. Fibers and threads: how they get their entry function
15. The garbage collector
16. Lazy library compilation, the parse cache, the optimizer
17. From schemes to bytecode
18. Numbers: integers and decimals
19. Objects in memory
20. The instructions
21. The JIT

---

## 1. The compile pipeline

`compile()` (`lib/grammar/main.cpp`) lists and sorts the source files, then
runs one stage after the other. Each stage submits one task per file
(`submit_task` / `wait_for_tasks`); `execute_task`
(`taskdelegator/task_worker.cpp:183-247`) dispatches them. With `-j 1` the
calling thread runs the tasks itself. After each stage the file's `stage` is
advanced (enum in `sharp_file.h:15-28`).

| # | Stage | Function | What it does |
|---|---|---|---|
| 1 | tokenize | `tokenize_file` (task_worker.cpp) | Reads the file. Tries the parse cache first (`parse_cache_load`); otherwise runs the `tokenizer`. |
| 2 | parse | `parse_file` | `new parser(tok)` builds the syntax tree; the tree is stored in the parse cache. |
| 3 | pre-process | `pre_process` (`backend/preprocessor/class_preprocessor.cpp:20`) | Creates the module and its global class, declares classes, aliases, fields and enums. Generic classes go to `pre_process_generic_class`. Functions are skipped. |
| 4 | imports | `process_imports` (`postprocessor/import_processor.cpp`) | Builds `file->imports`. `std` and `std.io.console` are added to every file. |
| 5 | post-process | `post_process` (`postprocessor/class_processor.cpp:22-95`) | `process_class` resolves base classes and interfaces, creates the init functions, processes fields, aliases and functions (`process_function`), including nested mutations. Generic blueprints and top-level `mutate` are skipped here. |
| 6 | process generics | `process_generics` → `process_generic_class` (class_processor.cpp:97, 231) | For each blueprint, every clone made so far gets `pre_process_class` + `process_class` + its extension functions and mutations. |
| 7 | components | `compile_components` | Dependency injection. |
| 8 | delegates | `process_delegates` (`postprocessor/delegate_processor.cpp`) | Links `override` functions to interface / delegate functions. Calls `process_function_return_type`, so `:=` bodies are compiled here. |
| 9 | mutations | `compile_global_mutations` (`compiler/mutate_compiler.cpp:12-70`) | Top-level `mutate X { }`: pre-process + process + `compile_class` on X. For a generic blueprint only the clones that exist at that moment. |
| 10 | global members | `compile_global_members` (`compiler/compiler.cpp:27-71`) | Global fields and global functions. |
| 11 | classes | `compile_classes` → `compile_class` (`compiler/class_compiler.cpp`) | Fields, nested mutations, init blocks, functions, default constructor, lambdas, closure references. A blueprint compiles each of its clones instead. |
| 12 | finalize | `setup_core_functions`, `finalize_compilation` (`backend/finalizer/finalizer.cpp`) | `compile_reachable_function_bodies()` (lazy library bodies), `optimize()` (mark what is used, inject field initial values, assign addresses), `generate()` (bytecode, optional `-O` optimizer, listing, executable). |

`pre_compilation_state` is `classes_post_processed`; `compilation_ready(s)`
means `s >= class_delegates_processed` (sharp_file.h:30-31). Code that creates
things late (generic clones, lambdas) uses these to decide whether to process
or compile them at once.

Top-level `def`s do not float free: each module has a hidden global class
(`__srt_global`, flagged `flag_global`) that holds its functions and fields.

## 2. Parsing an expression

Token names to know: `LEFTBRACE` is `[`, `LEFTCURLY` is `{`.

`parser::parseExpression` (`frontend/parser/Parser.cpp`) handles a leading `-`,
`++`/`--`, then calls `parsePrimaryExpr`, then `!!`, `{`-literals, `?:`,
assignment operators, and finally the binary operators and `? :`.

Binary operators are parsed by precedence level, loosest first:
`||`, `&&`, `|`, `^`, `&` (`binaryLevel`), then `equality`, `comparason`,
`shift`, `addition`, `multiplication`, `exponent`, `unary`. Each level groups
from the left. An operand is parsed by `unary()`, which calls
`parsePrimaryExpr` and then `parseNonNullSuffix` (`x!!`, `x!![i]`).

`parsePrimaryExpr` builds an `ast_primary_expr` and tries these alternatives
in order, rewinding after a failed attempt (`enterProtectedMode` / `pass` /
`fail`):

1. `!expr` → `ast_not_e`
2. a literal → `ast_literal_e`
3. `self[->call]` → `ast_self_e`
4. `base[->call]` → `ast_base_e`
5. `get<T>(..)` → `ast_get_component`
6. any name or call → `ast_dot_not_e`
7. `new` → `ast_new_e`
8. a lambda `{ args -> ... }` → `ast_lambda_function`
9. `sizeof`
10. `( expr )` → `ast_paren_e`
11. `null`

Suffixes are reached with `goto` labels: `[..]` → `ast_arry_e`, `as T` →
`ast_cast_e`, `is T` → `ast_is_e`, `++` / `--`.

Type names: `ast_utype` → `ast_type_identifier` → a native type, a function
pointer type, or an `ast_refrence_pointer`. A reference pointer is a list of
`ast_reference_item`s (one identifier each) with `.`, `?.`, `!!.` between them;
`#` turns what was read so far into an `ast_module_reference`; `<..>` becomes
an `ast_generic_reference`. Generic arguments are parsed speculatively: a type
list is tried in protected mode and accepted only if `>` follows, otherwise the
parser rewinds and `<` is less-than. `>>` is one `SHR` token, which is why
`box<box<var>>` does not parse.

A `{` that is not a lambda is decided in `parseExpression`: `{ :`, `{ as` or
`{ expr :` is a dictionary literal, anything else a vector array literal.

Any call - `point(1, 2)`, `std#f(1)`, `a.b(1)`, `list<int>(5)` - parses to the
same shape: `ast_dotnotation_call_expr` → `ast_dot_fn_e` → { `ast_utype`,
`ast_expression_list` }.

## 3. Expressions, schemes and steps

The compiler does not emit bytecode while it type-checks. It builds a tree of
operations first.

- An `expression` (`backend/compiler/expressions/expression.h:13`) is a pair:
  `type` (a `sharp_type`) and `scheme` (an `operation_schema`).
- An `operation_schema` (`backend/operation/operation.h:437-485`) has a
  `schemeType` (enum `_operation_scheme`, e.g. `scheme_new_class`,
  `scheme_call_static_function`, `scheme_binary_math`), optional `field` /
  `fun` / `sc` pointers and a list of `operation_step`s.
- An `operation_step` has a `type` (enum `operation_type`, e.g.
  `operation_create_class`, `operation_call_instance_function`,
  `operation_get_value` or `operation_step_scheme` for a nested scheme) plus
  its data (class, function, integer, string, nested scheme, `_bool`,
  `nativeType`).
- `create_*_operation` helpers in `operation.cpp` append steps.

Two things to remember when changing this layer:

- Many scheme types have a FIXED step layout that their generator reads by
  position (`scheme_new_class`, `scheme_or`, `scheme_compound_assign_value`,
  `scheme_binary_math`, ...). Appending a step to such a scheme, or re-labelling
  it, breaks code generation, not type checking. Wrap it as one "get value"
  step first (`wrap_scheme_as_value`, `scheme_has_own_layout` in
  expression.cpp).
- `operation_schema::copy()` APPENDS; call `free()` first to replace.
  `List::addAll()` on the other hand EMPTIES the list first.

`compile_expression` (expression.cpp:23-52) picks a compile function by AST
node type; `compile_primary_expression` picks by child node.

## 4. A call, step by step

1. `compile_dot_notation_call_expression`
   (`compiler/expressions/primary/dot_notation_call_expression.cpp:13-55`) adds
   a label step and calls `resolve(ast)` (`backend/dependency/dependency.cpp:3201`).
2. `resolve` turns the type syntax into a list of `unresolved_item`s (normal,
   module, generic, operator, function pointer, function reference). It
   compiles every argument first and stores the argument types (after
   `expression_type_to_normal_type`: an integer literal becomes `_int32`, a
   decimal `var`, bool/char `_int8`, a string literal `_int8[]`) in
   `item.typeSpecifiers` and their code in `item.operations`.
3. If the last part of the path has generic arguments the compiler reports
   "generic functions are not supported." (dependency.cpp:3248-3250).
4. `resolve(...)` (3116) walks the items through `resolve_item`; each narrows
   the running result type and appends steps to a working scheme, which is then
   copied onto the caller's.
5. When the target function is found, `compile_function_call`
   (expression.cpp:267-329) matches each argument to its parameter: an exact
   type match first, then an implicit one with `constructor_only`. A constructor
   match wraps the argument in a new-object step. The call is emitted as a
   function-pointer call, a static call, a call on `self`, or a call on an
   object.
6. If the result is a function, `resolve` replaces it with the function's
   return type.
7. `compile_post_ast_expression` handles what follows (`.member`, `[i]`,
   `(args)` on a function-pointer value, `++`).

## 5. Name lookup

The compile context is PER FILE: `current_context` is
`currThread->currTask->file->ctx` and `current_file` is
`currThread->currTask->file` (`compiler_info.h:62-64`). `create_context` saves
the old context on `ctx.storedItems` but keeps `localFields`;
`create_context(fun)` appends the parameters (`backend/context/context.cpp`).

For a plain name, `resolve_normal_item` (dependency.cpp:1473-1665) checks:

1. generic type parameter of the primary class
2. labels
3. local aliases
4. locals (`resolve_local_field`)
5. class fields, class aliases, class enums, inner classes, the class itself
6. global field, alias, enum, class, generic class
7. function addresses (a function name used as a value)
8. the "did you mean" error

`resolve_local_field` (843-930) searches the current locals and then EVERY
saved context. A hit in another function becomes a closure capture (section
12). This is also the cause of a known bug: a generic class instantiated late
sees the locals of the function that triggered it.

For a call, `resolve_function_reference_item` (2542-2786) checks, for the first
part of the path:

1. a local function-pointer variable
2. a function-pointer field of the enclosing class
3. a method of the enclosing class
4. a global function-pointer field
5. a global function
6. otherwise `COULD_NOT_RESOLVE`

Classes are never looked up when the name is a call - which is why `point(1, 2)`
without `new` is "could not resolve" outside the class. After a module
(`std#x(...)`) only a function-pointer field and then a function are tried;
after a class (`outer.inner(...)`) only a function-pointer field and a method.

Global functions are searched through `file->imports` one module at a time and
the first module with a match wins, so overloads are never compared across
modules.

`get_primary_class` returns the nearest class context; `get_primary_function`
the outermost block function.

## 6. Overload resolution and type matching

`resolve_function(name, class, params, ...)` (dependency.cpp:608-686):

1. `locate_functions_with_name` (`types/sharp_class.cpp:347-378`) collects
   candidates from the class, its interfaces and (optionally) its base classes.
   For constructors of generic clones the lookup is by function kind, not name.
2. The FIRST candidate whose parameters all match explicitly wins - no
   ambiguity check.
3. Otherwise, if implicit matching is allowed, all implicit matches are
   collected; more than one is "call to method ... is ambiguous".
4. Untyped lambda arguments then get their types from the chosen function and
   are compiled (`fully_qualify_function`).

Match rules (`types/sharp_type.cpp:182-344`, flags in
`types/type_match_result.h`):

- Explicit: same `data_type`, same `isArray`, compatible nullability; for
  classes the same class; for functions equal parameters. A `type_any`
  parameter matches anything.
- Implicit: a class to a related class (base or interface); native numbers to
  one another; a class built from the value through a constructor
  (`match_constructor`), an `init(x)` function (`match_initializer`) or
  `operator=` (`match_operator_overload`).
- The masks passed around are EXCLUSIONS: `exclude_all`, `constructor_only`
  (excludes operator and init), `overload_only`.

Because a one-argument constructor makes its class implicitly constructible
from that argument, `takes_point(5)` silently builds a `point`. This is also
behind the "number literal is ambiguous between a `var` overload and a class
overload" bug.

## 7. Types

`sharp_type` (`types/sharp_type.h:23-323`) holds a `data_type`
(`lib/core/data_type.h`: the sized numbers, `type_function_ptr`, `var`,
`object`, `class`, `field`, `function`, `lambda`, `module`, `import_group`,
`null`, `nil`, `any`, `untyped`, `undefined`, the literal kinds, ...), pointers
(`_class`, `field`, `fun`, `module`, ...), `unresolvedType`, the flags
`isArray`, `nullable`, `nullableItems`, `arrayElement`, and literal values.

`type_function_ptr` sits INSIDE the numeric range of the enum (between
`type_uint64` and `type_var`), so `is_numeric_type` is true for it. A function
pointer type `(A)(R)` is a `type_function_ptr` whose `fun` is a synthetic
`blueprint_function`.

An expression's type is often `type_field` (it names a field); use
`get_real_type` / `get_class_type` to look through it.

## 8. A function from declaration to bytecode

- Parse. `parseMethodDecl` (Parser.cpp:219-262) builds `ast_method_decl`:
  flags, identifier, argument list, then `: type` + block, `: type = expr;`,
  `:= expr;` (inferred return) or `;` (a delegate). If the identifier is not
  followed by `(` the declaration becomes `ast_operator_decl` - extension
  methods (`def string.foo()`) take that path.
- Declare. `process_function` (`postprocessor/function_processor.cpp`) works
  out the name, resolves the parameters and calls `create_function`
  (`types/sharp_function.cpp:42-77`), which rejects a duplicate (same name and
  parameters). On a generic blueprint class it records the AST and processes it
  on every clone instead. The return type is explicit, `type_untyped` for `:=`,
  or `type_nil`.
- Inferred return. `process_function_return_type` compiles the expression once
  the file is far enough along and calls `validate_function_type`, which boxes
  inferred numbers into `std#int` and the other box classes.
- Body. `compile_function(function, ast)`
  (`compiler/functions/function_compiler.cpp`) first asks
  `defer_function_body` (lazy library compilation, section 16). Otherwise it
  pushes a block context and compiles the expression or block into
  `function->scheme`. Constructors also get the base constructor call, the
  static-init check and the call to the class's init function.
- Address and output. Only functions marked `used` get an address
  (`generate_address`: `functionAddressCounter++`). The executable records
  name, full name, owner class, type, flags and parameters.
  `fullName = owner->fullName + "." + name`.

## 9. Constructors, init functions and static init

- A constructor's name must equal the class name. `create_default_constructor`
  adds a public no-argument constructor if there is none.
- `create_class_init_functions` creates two private functions per class:
  `static_init<C>` (static) and `init<C>`. All `init { }` blocks of the class
  are compiled into `init<C>`; the fields' initial values are injected in front
  of that code by the finalizer (`field_injector.cpp`, `fieldInitPos`).
- A compiled constructor body is, in order: the base-class constructor call,
  the static-init check (a guard field, set on first run), a call to `init<C>`,
  then the constructor's own code. `init(x)` conversion functions get the same
  prefix.
- Constructor chaining is only `-> base(args)`. There is no `-> self(..)`.
- Calling a constructor on an existing object is rejected on purpose: "cannot
  explicitly call constructor" (dependency.cpp:3144-3149). Inside `class
  point`, a bare `point(1, 2)` finds the constructor and hits that error.

## 10. `new` expressions

Parser: `new` + a type (which may be generic), then one of `[n]` (array),
`[] { a, b }` (vector array) or `(args)` (class), wrapped in `ast_new_e`. There
is no `new T { ... }` form although an example file shows one.

Compiler: `compile_new_class_expression`
(`compiler/expressions/primary/new_expression.cpp:29-102`) compiles the
arguments, finds the constructor with `resolve_function(..., constructor_function, ...)`,
checks access, records dependencies on the class and the constructor (the lazy
compiler follows these), and builds `scheme_new_class`:
`create_new_class_operation`, `create_duplicate_operation`, then one nested
scheme holding the constructor call. The generator for that scheme reads the
three steps by position.

## 11. Generic classes

- Blueprint. `pre_process_generic_class` creates a class with `isGeneric`,
  `blueprintClass`, the type parameter names in `genericTypes`, full name
  `X<T>`, stored in `owner->generics` / `module->genericClasses`.
- Clone. `create_generic_class` (dependency.cpp:2830-2866, sharp_class.cpp:71-132)
  names the instance `X<std#int>`, looks it up (that is the cache), and on a
  miss creates an ORDINARY class whose `ast` is the blueprint's - shared, not
  copied - with `genericBuilder = blueprint` and
  `genericTypes[i] = { name, concrete type }`. The clone is added to
  `blueprint->genericClones`.
- Substitution. There is none. The same syntax tree is processed again with
  the clone as the primary class, and `T` resolves by name through
  `resolve_generic_type_param`. `base` constraints on parameters are checked in
  `process_generic_identifier`.
- Late clones. A clone created after post-processing is processed at once and
  compiled at once if `compilation_ready`. `current_file` is switched to the
  blueprint's file (`currModule` is not).
- Runtime. Clones are plain classes in the executable.
- Native type arguments work (`box<var>`), but `T?` with a native `T` is
  rejected, so library classes that need nullable `T` constrain it with
  `T base _object_`.
- `mutate` on a generic class is applied only to the clones that exist when
  the mutation is compiled and is not recorded for later clones - the
  "mutate members missing on user-only instantiations" bug.

## 12. Lambdas and the closure mechanism

Parsing. A `{` in primary position is tried as a lambda: `{ ->` or
`{ args ->` gives `ast_lambda_function` (arguments `name` or `name: Type`),
then an optional return type `(R)` / `(nil)` and a block of statements. The
parser never produces an expression-bodied lambda, although the compiler has a
path for one.

Compiling (`compiler/expressions/primary/lambda_expression.cpp`). Untyped
arguments get `type_any`. The lambda becomes a STATIC function named
`anonymous_func<id>` in the primary class. Its body is compiled later by
`compile_class_lambdas`, or at once if the file is already past
pre-compilation and the function is fully typed. The expression's type is
`type_lambda_function`; its value is a function value (section 13).

Typing an untyped lambda. `fully_qualify_function` copies parameter and return
types from the target function type and then compiles the body. It is called
from overload resolution, `compile_function_call`, and local / field
declarations.

Captures (`dependency/closure_capture.h`). When `resolve_local_field` finds a
name in a saved context that belongs to another function, and
`can_capture_closure` allows it (arrays, classes, `object` fields and function
values - a native number is refused), `capture_enclosing_local` does this:

1. the enclosing function gets a class `closure_<enclosingFn>` with one field
   per captured name, and a hidden local `__closure` that holds an instance of
   it. The instance is created at the START OF EVERY CALL
   (`generate_initial_closure_setup`): one set of captured variables per call;
2. the lambda gets a hidden LAST parameter `__captured` (`capturedParameter`
   on its `sharp_function`). It is kept out of `parameters` while types are
   matched - `{ a: int -> ... }` still matches `(int)()` - and added to the
   list when code generation starts (`generate_address`). The lambda stays a
   static function;
3. the original local gets `field->closure` (the field of the closure class)
   and `field->closureRef` (the `__closure` local);
4. inside the lambda an access is "`__captured`, then the field".

```
def make_counter() : ()(int) {          frame of make_counter
    total := new box();                   [ total ][ __closure ] ---> closure_make_counter { total }
    return { -> (int) ... total ... };                                   ^
}                                       function value { address, captured -+ }
                                        frame of the lambda:  [ __captured ] -> the same closure object
```

- the enclosing function keeps its own frame slot and copies every write into
  the closure object at code generation time (`scheme_assign_value.cpp`,
  `scheme_compound_assign_value.cpp`). A captured local therefore lives in two
  places, which only works because just objects are captured;
- captured PARAMETERS are copied at function entry;
- a lambda inside a lambda: every function between the lambda and the owner of
  the variable gets a closure object too, with a field `__parent` that holds
  the closure object it received. The access is `__captured.__parent.x`;
- `self` in a lambda of an instance function is captured as the field
  `__@self`, written at function entry from slot 0 (`self_expression.cpp`);
- slot 0 of a frame is the instance of an instance function and nothing else.
  The captured object is a parameter, never an instance.

(Until November 2026 there was one closure object per enclosing function PER
FIBER, found through a fiber-local static field and created in
`platform.tls_init`: recursion shared the captured variables, another fiber
read them as null.)

## 13. Function values

- Value. A function value is an OBJECT: an instance of
  `platform.kernel#function_value` (`lib/support/0.4.0/platform/kernel/function_value.sharp`)
  with a number slot `address` (the function's index in the method table) and
  an object slot `captured` (the closure object of the function that made a
  lambda, or null). `process_get_static_function_address` builds one wherever
  a lambda or the name of a function is used as a value; non-static functions
  are rejected. `lib/runtime/memory/function_value.h` has the full picture.
- Type. `type_function_ptr` sits after `type_class` in `lib/core/data_type.h`,
  outside the number range, so every `type <= type_var` test treats a function
  value as an object: object fields, object locals, object arrays.
- Comparing. `f == null` is a null test. `f == g` between two function values
  calls `function_value.same(f, g)`: the same function with the same captured
  object (`compile_function_value_comparison` in `binary_expression.cpp`).
- Call. `post_ast_expression.cpp` -> `compile_function_call(..., isFunctionPointer)`
  -> `create_dynamic_function_call_operation`: push the function value, push
  the arguments, `operation_call_dynamic_function` -> `calld <argument count>`.
- Interpreter. `CALLD n` calls `call_function_value(n)`: the function value is
  under the n arguments; the arguments move down over it, the captured object
  is pushed after them when there is one, and `prepare_method(address)` runs
  as for any static call.
- JIT. `CALLD` calls `jit_rt_call_dynamic`, which is the same C++ function.
- Fibers. `vm.start_fiber` pushes the function value (int 0x45); the runtime
  reads `address` from it. A thread's `main` field is copied to its first
  fiber as an object.

(Until November 2026 a function value was a NUMBER, the function's index, kept
in registers and number slots; `main != 0` tested it and `calld` took a
register.)

## 14. Fibers and threads

`fiber(name, main)` stores the user's function in the field `main`.
`vm.start_fiber` passes the static `fiber_main` to the runtime as a number
(`int 0x45`); the runtime stores a `sharp_function*`, and `fiber_main` calls
`fib.main(args)` through `CALLD`. Threads start at `_srt_thread_start`, named
through an asm symbol, which calls the thread's main function. So the runtime
itself only ever starts static, non-capturing functions; user lambdas are
always called from Sharp code.

When a thread continues a fiber after a context switch, the native side of the
fiber's call stack is rebuilt from the bottom so JIT compiled frames get their
machine frames back (`next_rebuild_frame`, by call depth). Only functions that
were RUNNING as machine code when they made their call are entered that way:
the frame of the call says so (`frame.native`). A function that was
interpreted then and has been compiled since - by another thread, while this
fiber waited - stays in the interpreter until it returns
(`continues_in_machine_code`, `virtual_machine.cpp`). Entering it as machine
code let a nested interpreter loop run the rest of the fiber, and the outer
loop dispatch past the end of `fiber_main` (fixed 2026-11-14,
`tests/suite/cases/lib_coroutines_resume_after_compile.sharp`).

Locks: every `lock(x) { }` block a fiber is inside of is recorded
(`fiber::held`); `catch_exception` releases the blocks an exception leaves, and
a lock counts re-entries by its holder (`memory/sharp_object.cpp`).

## 15. The garbage collector

It traces; it does not count references (it did until November 2026). The
code is `runtime/memory/tracing_collector.cpp`, and its header explains the
design in full. In short:

- A collection stops every thread that runs Sharp code at a safe point (the
  `state_check` of the interpreter and of compiled code, signal `tsig_gc`),
  marks everything reachable from the roots, frees the rest, and lets the
  threads go on. Blocked threads are inside a `gc_safe_region` and are not
  waited for.
- Roots (`mark_roots`): static fields, permanent objects (`tgc_make_permanent`),
  reflection objects, every thread's objects, every fiber's stack from the
  bottom up to `sp`, its exception and fiber objects, the objects it has locked
  or waits to lock, the object that contains `task->ptr`, and objects native
  code holds (`tgc_pin`).
- Stack slots above `sp` are set to null during a collection, so a leftover
  slot never points at a freed object.
- A collection never runs inside an allocation: allocating only asks for one,
  and it happens at the thread's next safe point. That is why C++ code may hold
  a new object in a local variable for the length of one instruction.
- Assigning a reference (`copy_object`) is a plain pointer store. Reference
  cycles are freed like everything else.
- Still true: a number slot holds no reference, so an object reachable only
  through a number (a handle stored as a number) is freed.
- Adding a place in the runtime that keeps a `sharp_object*` longer than one
  instruction means adding it to `mark_roots`. Check with `-gcstress <n>` and a
  runtime built with `-fsanitize=address`.

## 16. Lazy library compilation, the parse cache, the optimizer

- Lazy bodies (`compiler/functions/lazy_functions.*`). The body of a function
  from a library file (under `-L`) is put aside by `defer_function_body` and
  compiled only when the dependency walk from the entry points reaches the
  function (`compile_reachable_function_bodies`). Never deferred: functions
  containing a lambda, `:=` functions, initializers, `main`, the platform
  class. This is why every resolver must record `create_dependency(...)`.
  `-eager` turns it off.
- Parse cache (`frontend/parser/parse_cache.*`). Syntax trees are stored under
  `<home>/.sharpc/cache`, valid for the same source text (length + hash) and
  the same compiler executable. The file stores `ast_type` numbers: add new AST
  node types at the END of the enum and bump `PARSE_CACHE_FORMAT` when the tree
  shape changes.
- Bytecode optimizer (`backend/optimizer`, `-O`). Works on the generated
  bytecode. `CALLD` is an opaque call and is never inlined; nothing in it is
  specific to closures.

## 17. From schemes to bytecode

`process_scheme` (`finalizer/generation/code/scheme/scheme_processor.cpp`)
hands each scheme type to its own `scheme_*.cpp`; `process_step`
(`step_processor.cpp`) handles single steps. The generator tracks where the
current value is in `cc.machineData` (a constant, a register, a local slot, an
instance field through the object pointer, an object on the stack, ...);
`consume_machine_data` moves it where the next instruction needs it. Setting
machine data while the previous value was not consumed is the "previous machine
data not consumed" error; consuming when there is none is "attempting to
consume empty machine data".

Useful when a compile crashes in the generator: `SHARP_GEN_DEBUG=1` prints each
function before its code is generated, and `sharpc -d` writes a listing
(`.aout`) of the bytecode.

## 18. Numbers: integers and decimals

Since the November 2026 redesign the VM has two kinds of number, and the
compiler - not the VM - knows which one every value is.

| | in the language | in the VM | C++ picture |
|---|---|---|---|
| integer | `_int8` ... `_int64`, `_uint8` ... `_uint64`, `int`, `long`, `bool`, `char` ... | 64 bits, wraps around | `int64_t` |
| decimal | `decimal` (was `var`), `double` | 64-bit IEEE double | `double` |

- One VM value is one 8-byte cell: `union vm_cell { int64_t i; double d; }`
  (`stdimports.h`). Registers, stack slots, number fields and 64-bit array
  elements are all such cells. Nothing in the cell says which kind it holds.
- Moving a value (register to local, local to stack, field to register ...)
  copies the cell and is the same instruction for both kinds. Arithmetic and
  comparison are not: every such instruction exists in an integer form with the
  plain name (`ADD`, `LT`) and a decimal form with a `D` in front (`DADD`,
  `DLT`). `I2D` and `D2I` convert a register.
- `/` and `**` always work on decimals (`7 / 2` is `3.5`): `DDIV`, `DEXP`.
  `%`, shifts and the bit operators always work on integers.
- The compiler types every arithmetic step (`set_numeric_operation` in
  `backend/operation/operation.h`, `type_binary_operation` in
  `expressions/binary/binary_expression.cpp`) and inserts conversions where a
  value changes kind: `compiler/expressions/numeric_conversion.*`
  (`convert_number_for`, `convert_number_to_integer`,
  `convert_number_to_condition`). Places that build a value by hand instead of
  going through an expression must call these themselves.
- `decimal_to_integer()` (`stdimports.h`) is what `D2I` does: the fraction is
  dropped, a NaN becomes 0, a value that does not fit becomes the nearest end
  of the range.
- System interrupts (`INT`) read and write registers; what kind each one
  expects is the table in `runtime/interrupt_signatures.cpp`. An `asm` block
  that passes a number to an interrupt has to hold it in a local of the right
  kind.
- The kind checker. A runtime built with `-DSHARP_TYPE_CHECK` keeps a tag
  (integer / decimal) next to every register and stack slot and reports any
  instruction that reads the wrong kind (`runtime/memory/type_check.*`). It is
  the tool for finding a conversion the compiler forgot.

## 19. Objects in memory

`runtime/memory/sharp_object.h` has the full description. In short:

- A class instance is one block of 8-byte slots. A number field is stored IN
  its slot (`number_slot(o, i)`); a reference field's slot holds the reference.
  Each class knows which slots are numbers (`instanceNumberSlots`,
  `staticNumberSlots`, built by `build_number_slot_maps()`), which is how the
  collector, `sizeof` and the serializer skip them. There is no separate little
  object per number field any more.
- A number array stores its elements at their real size - one byte for
  `_int8[]`, eight for `_int64[]` and `decimal[]` (`element_size(type)`), in
  `sharp_object::data`. C++ code uses `integer_at` / `set_integer_at` /
  `number_at` / `set_number_at`.
- Runtime code that reads a field by name: `number_field(instance, name)` and
  `set_number_field` for numbers, `resolve_field` for references
  (`runtime/reflect/reflect_helpers.h`).

## 20. The instructions

An instruction is one 32-bit word, a few are two (the second word is a 32-bit
constant, marked `+k` below). `a b c` are the operands in the order the listing
prints them; `regs` are the ten VM registers (`adx cx cmt ebx ecx ecf edf ehf
bmr egx`), `fp` is the frame, `sp` the top of the stack, `ptr` the object
pointer (the address of a slot that holds a reference). Declared in
`lib/core/opcode/opcode.h`; the interpreter's handlers are in
`runtime/virtual_machine.cpp` (`main_vm_loop`) in the same order.

Integer arithmetic (decimal twin in brackets)

| instruction | does |
|---|---|
| `ADD SUB MUL` a, b -> c  [`DADD DSUB DMUL`] | `regs[c] = regs[a] op regs[b]` |
| `MOD` a, b -> c | remainder; a zero divisor raises an exception |
| `IADD ISUB IMUL` a `+k`  [`DIADD DISUB DIMUL`] | `regs[a] op= k` |
| `IMOD` a `+k` | `regs[a] %= k` |
| `INC DEC` a  [`DINC DDEC`] | `regs[a] += 1`, `-= 1` |
| `NEG` a, b  [`DNEG`] | `regs[a] = -regs[b]` |
| `DSQRT` a, b | `regs[a] = sqrt(regs[b])`, decimals |
| `NOT` a, b  [`DNOT`] | `regs[a] = regs[b] == 0` |
| `LT GT LTE GTE` a, b  [`DLT DGT DLTE DGTE`] | `cmt = regs[a] ? regs[b]` (0 or 1, an integer) |
| `TEST TNE` a, b  [`DTEST DTNE`] | `bmr = regs[a] == regs[b]`, `!=` |
| `CMP` a `+k`  [`DCMP`] | `cmt = regs[a] == k` |
| `AND` a, b | `bmr = regs[a] && regs[b]` |
| `UAND OR XOR` a, b | `bmr = regs[a] & \| ^ regs[b]` |
| `SHL SHR` a, b -> c | `regs[c] = regs[a] << regs[b]`, `>>` |
| `MOV8 MOV16 MOV32 MOV64 MOVU8 MOVU16 MOVU32 MOVU64` a, b | `regs[a] = (that integer type)regs[b]` |

Decimal only

| instruction | does |
|---|---|
| `DDIV` a, b -> c, `DIDIV` a `+k` | division |
| `DEXP` a, b -> c | `pow` |
| `I2D` a, b / `D2I` a, b | `regs[a] = (double)regs[b]` / `decimal_to_integer(regs[b])` |
| `LDC` a, n | `regs[a] =` decimal constant n of the executable |
| `DMOVI` a `+k` | `regs[a] = (double)k` |

Registers, locals, the stack (the cell is copied; both kinds)

| instruction | does |
|---|---|
| `MOVI` a `+k` | `regs[a] = k` (integer) |
| `MOVR` a, b | `regs[a] = regs[b]` |
| `LOADL` a, n / `SMOVR_2` a, n | `regs[a] = fp[n]` / `fp[n] = regs[a]` |
| `SMOVR_4` n `+m` | `fp[n] = fp[m]` |
| `ISTOREL` n `+k`  [`DISTOREL`] | `fp[n] = k` |
| `RSTORE` a / `LOADVAL` a | push `regs[a]` / `regs[a] =` pop |
| `ISTORE` `+k`  [`DISTORE`] | push k |
| `IPUSHL` n / `IPOPL` n | push `fp[n]` / `fp[n] =` pop |
| `SMOV` a, n / `SMOVR` a, n | `regs[a] = sp[n]` / `sp[n] = regs[a]` |
| `ISADD` n `+k`  [`DISADD`] | `sp[n] += k` |
| `RETURNVAL` a | `fp[0] = regs[a]`: the function's number result |
| `MOV_ABS` n / `LOAD_ABS` n | thread local n `= ebx` / `ebx =` thread local n |
| `ADDL SUBL MULL MODL ANDL ORL XORL` a, n  [`DADDL DSUBL DMULL DDIVL`] | `fp[n] op= regs[a]` |
| `IADDL ISUBL IMULL IMODL` n `+k`  [`DIADDL DISUBL DIMULL DIDIVL`] | `fp[n] op= k` |

Objects

| instruction | does |
|---|---|
| `MOVL` n / `MOVSL` n / `TLS_MOVL` n / `MOVG` class | `ptr =` address of local n's reference / stack slot / thread local / the class's static object |
| `MOVN` `+n` / `MOVND` a | `ptr =` address of reference slot n (or `regs[a]`) of the object at ptr |
| `GETF` a, slot / `PUTF` a, slot | `regs[a] =` number field of the object at ptr / the other way |
| `ALOAD8 ALOADU8 ALOAD16 ALOADU16 ALOAD32 ALOADU32 ALOAD64` a, b | `regs[a] = array[regs[b]]`, element of that size, signed or unsigned |
| `ASTORE8 ASTORE16 ASTORE32 ASTORE64` a, b | `array[regs[a]] = regs[b]` |
| `CHECKLEN` a | bounds check of `regs[a]` against the object at ptr |
| `SIZEOF` a / `CHECKNULL` a | `regs[a] =` its size / whether it is null |
| `PUSHOBJ` / `PUSHL` n / `PUSHNULL` / `DUP` | push the reference at ptr / of local n / null / the top one again |
| `POPOBJ` / `POPL` n / `POP` | reference at ptr `=` pop / local n `=` pop / drop |
| `POPOBJ_2` | `ptr =` address of the slot just popped |
| `SMOVR_3` n / `RETURNOBJ` / `DEL` | local n `=` reference at ptr / the function's object result / reference at ptr `= null` |
| `ITEST` a | `regs[a] =` the two popped references are the same object |
| `NEWCLASS` `+class`, `NEWARRAY` a, type, `NEWOBJARRAY` a, `NEWCLASSARRAY` a `+class`, `NEWSTRING` n | create and push |
| `CAST` class / `VARCAST` type / `IS` a `+type` | checked casts, type test |
| `LOCK` / `ULOCK` | the object lock of the object at ptr |
| `SWAP` | swap the two top stack slots |

Control flow and the rest

| instruction | does |
|---|---|
| `JMP` pc / `JE` pc / `JNE` pc | jump; if `cmt` is set; if it is not |
| `BRH` / `IFE` / `IFNE` | the same with the pc taken from `adx` |
| `SKIP` n / `SKPE` a, n / `SKNE` a, n | relative jumps (being phased out) |
| `LOADPC` a | `regs[a] =` this pc |
| `CALL` function / `CALLD` a / `INVOKE_DELEGATE` | call; call the function value under the `a` arguments on the stack (section 13); interface call |
| `RET` / `RET 1` | return; return with the exception on the stack |
| `THROW` | throw the object on the stack |
| `INT` n | system interrupt n (`runtime/interrupt_signatures.cpp`) |
| `PUT` a `DPUT` a `PUTC` a `GET` a `SLEEP` a `NOP` `HLT` | console output of an integer / a decimal / a character, key input, sleep |

Adding an instruction: `opcode.h` (number + builder), `opcode.cpp`,
`opcode_macros.h` (dispatch table, same order as the numbers), the handler in
`main_vm_loop`, the listing in `finalizer/generation/code/code_info.cpp`, the
row in `optimizer/instruction_set.cpp`, the `asm` mnemonic in
`frontend/parser/Parser.cpp` and `functions/statements/asm_statement.cpp`,
`runtime/telemetry/telemetry.cpp` (name), and the JIT (below).

## 21. The JIT

`lib/runtime/jit`. It compiles one function at a time to x86-64 machine code
(System V: Linux and Intel macOS; on other CPUs everything is interpreted). A
function is compiled when it has been called 10 times, or when a loop in it has
gone round often enough (the interpreter then continues the loop in the
compiled code).

| file | what is in it |
|---|---|
| `jit_compiler.h/.cpp` | what the VM sees: `jitCompiler->compileFunction`, the record of a compiled function (`jit_compiled_function`) |
| `jit_runtime.h/.cpp` | the C++ functions compiled code calls for everything slow or rare (`jit_rt_...`). None lets a C++ exception out: it becomes the thread's exception flag |
| `x64/x64_compiler.*` | the x86-64 compiler the VM calls; installs the result |
| `x64/x64_generator.*` | the code generator: walks the bytecode, and the building blocks every instruction uses. **Its header is the map of the JIT - start there** |
| `x64/sections/` | what every function has once: `entry`, `dispatch`, `exception`, `exits`, `slow_paths` |
| `x64/opcode/` | the instructions, one file per kind: `integer_math`, `decimal_math`, `stack_and_locals`, `objects`, `fields_and_arrays`, `allocation`, `casts_and_locks`, `control_flow`, `calls`, `direct_calls`, `console` |

The ideas, each explained at length in the file named:

- The VM's state stays where the interpreter keeps it (the fiber's stack and
  frames), so stack traces, exceptions, the collector and fibers work
  unchanged, and a function can switch between interpreter and machine code at
  any instruction. Machine code only keeps COPIES in CPU registers
  (`x64_compiler.h`).
- Six VM registers live in CPU registers - the same six in every function, so
  a call passes them along for free. C++ does not preserve those registers, so
  code is written for one of two states, "register mode" and "memory mode",
  with `save_vm_registers()` / `load_vm_registers()` between them
  (`x64_generator.h`).
- A bytecode instruction becomes one to four machine instructions:
  `add ecx, ebx -> ecx` is `add r9, rsi` (`opcode/integer_math.cpp`).
- A call from compiled code to compiled code does not leave machine code: the
  caller writes the VM's call frame itself (`opcode/direct_calls.cpp`).
- Every instruction has an address in the function's jump table, which is how
  a fiber is continued, an exception handler is entered and a hot loop is taken
  over (`sections/dispatch.cpp`).
- An instruction the JIT has no code for makes `compileFunction` return false;
  the function simply stays interpreted.

Run with `-nojit` to compare against the interpreter: the two must always
print the same thing.
