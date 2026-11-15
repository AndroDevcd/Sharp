# The backend of the self-hosted Sharp compiler

This is the plan and the record of the backend of the bootstrap compiler
(`lib/support/bootstrap/backend`). Every stage of the build works from it and
updates it; it is the document a review is held against.

"Backend" here means everything between the parser and code generation: the
part of a compiler that works out what a program MEANS. It ends with every
function body, field value and expression fully analysed - every name
resolved, every expression given a type, every call bound to the exact
function it calls, every rule of the language checked - and the result held in
objects a code generator can walk without asking another question. There is no
bytecode and no optimizer in this document.

The reference for what the language means is the C++ compiler
(`lib/grammar/backend`). Where its behaviour is a RULE of the language the
bootstrap follows it. Where it is an ACCIDENT of how that compiler was built,
the bootstrap does the textbook thing instead and the difference is written
down here (section 5, "Deliberate differences", and marked **[differs]** where
the rule is stated).

Contents

0. Map of the source (the folders, one line per package)
1. Words used in this document
2. The pipeline
3. The stages that exist
4. Decided designs: generic classes, the typed tree, function values
5. Deliberate differences from the C++ compiler
6. The object model
7. The rules of the language
8. Feature inventory (features, the C++ compiler's errors, every node kind)
9. Error codes and how diagnostics are written
10. The stages still to build (one section each; 10.0 says what a green gate
    proves at each stage; 10.7 is the review this plan went through;
    10.10 the leftovers of the analysis build: rules)
11. Decisions on what the first draft left open
12. Handing over to code generation: how to walk the analysed program

---

## Map of the source

Every folder is one PACKAGE and one module (the module name is the folder
path with dots: `backend/compiler/names` is `mod backend.compiler.names;`).
Each package has a front-door file named like its folder (for the stage
packages it also holds the stage's driver) whose header lists the package's
files with a line each. Keep this map in step with every move.

```
lib/support/bootstrap/
  main.sharp                        the driver: runs the stages, times them, ends with the exit code
  source_folders.sharp, left_out_modules.sharp, parsing.sharp, broken_declarations.sharp, executable.sharp, project.sharp
                                    the driver's parts: which files, the note for a module whose files were
                                    left out, the parallel parse, the body stages after declaration errors, the
                                    options handed to the backend, projects
  configuration/                    the command line and settings.json, read into `options`
                                    (doc/codegen.md, section 0, 8.2 and 13.7)
  frontend/
    lexer/                          source text -> tokens
    syntax/
      tree/                         the syntax tree (flat arrays), node kinds, parse_state, source_unit;
                                    token_spans (the first / last token of a subtree, the text between two tokens)
      parser/                       tokens -> tree: declarations, statements, expressions, types, literals
        habits/                     habits of other languages (`and`, `(t) x`, `function f()` ...) and keywords with a
                                    slip (`retrun x;`): reported once, then parsed as the Sharp form (10.11). One file
                                    per family: operator_, value_, declaration_, loop_habits, misspelt_keywords
      assembly/                     `asm { }` blocks: the instruction table and their parser
      printing/                     -parsetree, -parsestats, printing a file's parse errors
    diagnostics/                    the diagnostic object, the renderer, did-you-mean, the order the
                                    messages are printed in (message_order.sharp: the program text's)
  backend/
    types/                          THE OBJECTS (pure data; knows nothing of the stages)
      declarations/                 sharp_declaration, sharp_class, sharp_field, sharp_function, local_variable,
                                    closure_record; sharp_component (a component, a `single` / `factory`
                                    definition, what a `get` / `inject` asks for); name_table (the
                                    hash table of names every symbol table uses, 10.6)
      program/                      sharp_module, sharp_file, sharp_program; well-known library classes; node
                                    helpers; obfuscation_rule (one name of an `obfuscate` block, resolved);
                                    analysed_program: THE object handed to code generation    (stage 10)
      type_system/                  sharp_type (written), resolved_type (what a value is), type_environment,
                                    same_type, constant_value
      typed_tree/                   the bound_* nodes of the typed tree, and `conversion`          (stage 6)
        statements/                 the statements of the typed tree: block, local, if, when, loops,
                                    jumps, try / lock, asm                                          (stage 8)
    preprocessor/                   stage 1: the trees become objects (declare.sharp)
    postprocessor/
      imports/                      stage 2: which modules a file sees; import groups; the `mod` line
      names/                        stage 3: type names -> declarations (type_names.sharp; the messages for a
                                    name that is not found: type_name_messages.sharp); may the type be named
                                    there (private / protected / local); mutate blocks; an extension
                                    function's own names for its class's type parameters
                                    (renamed_type_parameters.sharp)
      inheritance/                  stage 4: base classes, interfaces, overrides; stage 5: real bases, override families,
                                    functions that only hide one above (W1041)
      generics/                     stage 5: resolve_written_type, generic instances, member types
      members/                      stage 5: the driver; member lookup; constructors, accessors, extension functions ...
      checks/                       declaration rules: duplicates, modifiers, default access, name conflicts ...
    compiler/                       THE BODY STAGES (6 and on)
      driver/                       analyse_bodies: every class, instance and global; one function body;
                                    one field value; types on demand; unused_blueprints.sharp: generic
                                    code nothing instantiates, analysed as stand-in instances (W1039)
      context/                      analysis_session, analysis_context, local_scope (the scope chain),
                                    jump frames (where break / continue / return go), types written in
                                    bodies, reporting (the diagnostics sink), closures (a capture:
                                    the variable and the closure records on the way)
      overloads/                    overload resolution: candidates, applicability, ranking, generic
                                    instances for a call (type arguments worked out), the E0501 / E0503
                                    messages, the E0515 - E0518 messages for type arguments
      names/                        what a plain name / a member name means; access rules; the "there is no ..."
                                    and "... has no member ..." messages
      conversions/                  the implicit table, casts, classes up and down, number classes,
                                    convert(), the type of `x := value`, the messages for a value that does not fit
      expressions/                  analyse_expression (the dispatcher), analyse_value / _converted / _condition
        literals/                   literals, string templates
        primary/                    names, module#name, self, base; reading a field; a type named in an expression
          members/                  a.b, a->b: members of a value and of a class; the null rule of `.`; ?. chains
        operators/                  as, is, !!, ?:, ? :, sizeof
        collections/                a[i] (also operator[] on an object), new t[n], { a, b }, an item written
                                    into a new array, { "k" : v } dictionaries
        calls/                      f(x), a.f(x), module#f(x), function values, new t(x) and t(x), a
                                    constructor's `-> base(x)`; binding the chosen function (dispatch,
                                    W1022, E0704)
        arithmetic/                 the operators: numbers, operator functions, == on references, && ||,
                                    -a !a, constant folding
        assignment/                 what can stand on the left of `=` (E0711); = and += into storage,
                                    setters, operator=, put, ++ --, const (W1021)
      statements/                   analyse_statement (the dispatcher), blocks, local variables, local
                                    aliases, expression statements (W1018)                          (stage 8)
        branches/                   if, when (a value as "subject == value"; E0616, W1020)
        loops/                      while, do-while, for, foreach; what can be looped over (E0609, E0610)
        jumps/                      return, break, continue; what a jump passes on the way out (E0602 - E0607)
        exceptions/                 throw, try / catch / finally, lock (E0612 - E0615, W1019)
        assembly/                   asm blocks: labels and operands (E0920 - E0926); the globals of a
                                    module (`$m#__srt_global[name]`), overloaded addresses (W1042)
      flow/                         the flow analysis, a pass of its own: can execution go on after each
                                    statement; E0605 (with the path), W1002                         (stage 8)
      lambdas/                      lambdas: their written form, their type from what is expected
                                    (E0800 - E0803), the lambda as a function of its own and its body,
                                    its result worked out from its returns; lambdas and function names
                                    as call arguments; a function named as a value (E0804, E0805,
                                    W1011); where a lambda may stand (E0808)                        (stage 9)
      initializers/                 how objects, classes and the program are set up: the implied
                                    `-> base()` and the constructors the compiler adds (W1024), enum
                                    numbers (E0940, W1030), field values in reading order (W1031),
                                    the initialisation lists of every class and module; a pass of
                                    its own (`initializers` on the time: line)                      (stage 9)
      components/                   the dependency-injection system: a definition analysed on demand
                                    (E0905), the search for the definition a request means, `get<t>()`
                                    as an expression and as a waiting argument (E0902, W1014), `inject`
                                    on fields, globals and locals (E0906), the messages of failed
                                    requests (E0903, E0904, E0908, E0909, W1013); components and
                                    definitions declared twice (E0900, E0901). Its pass after the
                                    bodies is `components` on the time: line                        (stage 9)
      obfuscation/                  `obfuscate` blocks: the option (E0950) and every listed name looked
                                    up - a module, class, field or function (E0951, W1016)         (stage 9)
      program/                      the whole-program checks, a pass of its own (`program checks` on the
                                    time: line): `main` (E0930, W1015), the platform's start-up functions
                                    (E0961), the modules and classes the VM itself needs (E0962), fields
                                    nothing gives a value (W1032), the warning switches (-w, -w<kind>,
                                    -wextra, -werror, -errlmt); builds the analysed program          (stage 10);
                                    completeness.sharp: the self-check that no expression is left
                                    without a type and without a message (E0991), a pass of its own
                                    (`completeness` on the time: line)                            (10.8)
                                    (driver/unused_blueprints.sharp: generic code nothing instantiates,
                                    analysed as stand-in instances - W1039, `unused generics` on the
                                    time: line)                                                   (10.8)
        usage/                      what every body uses (def-use), what is reached from `main`; unused
                                    locals and private members, a local hiding a field (W1034 - W1036)
    reports/                        -declarations, -analysis (analysis_printer: the walk, the headings,
                                    every local with its slot, the initialisation lists, components;
                                    expression_printer; statement_printer; program_printer: the analysed
                                    program in a few lines), -analysis-stats
    finalizer/                      CODE GENERATION (doc/codegen.md, its own map in section 0): layout, ir,
                                    generation, bytecode, verify, output, listing. Not part of analysis: it
                                    reads the analysed program and writes the executable
    optimizer/                      THE OPTIMIZER, -O (doc/codegen.md 7.2, 13.6): analysis, passes/{values,
                                    removal, control, calls, registers}; on code generation's IR, between
                                    generation and the bytecode step. Not part of analysis either
  cache/                            the parse cache
  tests/                            run.sh (frontend), backend.sh (backend: pass/ and fail/), helps.sh
                                    (helps/: the text each tested help proposes, pasted in, must compile, 10.11),
                                    codegen.sh (codegen/: programs compiled and run on the VM, codegen.md 13.1)
  doc/                              this document, the grammar; codegen.md: the plan of code generation, the
                                    optimizer and self-hosting (the VM's file format and instructions, the lowering)
```

Which way the packages depend on each other: `frontend` knows nothing of
`backend`; `backend/types` knows nothing of the stages; a later stage imports
an earlier one, never the reverse. Three groups are circular INSIDE, by the
nature of what they hold, and are best read as one unit each:

- the four `backend/types` packages are one data model (a class has fields,
  a field has a type, a type names a class; a function owns its typed tree);
- the stage 5 packages `generics` / `inheritance` / `members` call each other
  because instances are made lazily (naming `list<string>` needs its base
  class, which needs its members, which may name another instance);
- `compiler/expressions` and its four families, with `compiler/driver`: an
  expression contains expressions, and the type of `x := value` is worked
  out by analysing `value` on demand - a recursive walk;
- `compiler/statements` and its five packages: a statement contains
  statements (the body of a loop is analysed by the front door's
  `analyse_part`);
- `compiler/lambdas` with the expression and statement packages: a lambda is
  an expression whose body is statements whose expressions may be lambdas.
  `compiler/initializers` belongs to the same unit through the driver (the
  driver asks it for the implied base call of a constructor; it binds that
  call with the calls package, which asks the driver for result types on
  demand). `compiler/components` is part of the walk in the same way: a
  `get<t>()` is an expression, and the expression of a definition is analysed
  by the expression packages. `compiler/obfuscation` and `compiler/program`
  (with `program/usage`) only use the others; `program` imports `usage`, never
  the reverse.

One trap for whoever writes Sharp here: the C++ compiler resolves the type
of a field declared `name := new list<x>()` in the file that USES the field.
A file that reads such a field must import the module of `x` too, or it gets
"could not resolve symbol". The body-stage files therefore import all four
`backend.types.*` packages.

---

## 1. Words used in this document

| word | meaning |
|---|---|
| syntax tree | what the parser built: one node per piece of TEXT (`frontend/syntax`). Flat arrays; never changed after parsing. |
| declaration | something a program declares by name: a class, a field, a function ... One object each (`backend/types`). |
| written type | a type as it stands at one place in the source: `list<string>?`. A `sharp_type`. |
| resolved type | what a value IS, wherever it came from: "an object of class `list<string>`, may be null". A `resolved_type` (section 6.1). |
| typed tree (bound tree) | the result of analysing a function body: one object per expression and statement, each carrying what was worked out about it (its type, the function it calls ...). Separate from the syntax tree. "Binding" is the textbook word for tying a name to what it means. |
| scope | a place names are declared in. Scopes nest: a block inside a function inside a class inside a module. Looking a name up means asking each scope in turn, nearest first (the "scope chain"). |
| symbol table | the textbook name for the data structure behind a scope: name -> what it means. |
| blueprint | a generic class or function as written: `class list<T>`. Not a real class: `T` is not known. |
| instance (of a generic) | the real class made from a blueprint for particular types: `list<string>`. |
| overload | one of several functions with the same name and different parameters. Overload resolution picks the one a call means. |
| conversion | turning a value of one type into another. Implicit: the compiler does it without being asked (`_int32` to `decimal`). Explicit: the programmer wrote `as`. |
| error type | the type given to an expression that was already reported as wrong. It fits everywhere, so one mistake gives one message. |
| lvalue | something that can stand on the left of `=`: a variable, a field, an array element. |
| reachability | whether execution can get to a statement at all. `return; x = 1;` - the second statement is unreachable. |

---

## 2. The pipeline

`exec_runtime` in `main.sharp` runs the stages in this order. Each takes the
whole program and adds to the objects the stage before it left. A stage never
changes the syntax tree.

| # | stage | entry point | takes | produces | status |
|---|---|---|---|---|---|
| 0 | Tokenize and parse | `frontend/lexer`, `frontend/syntax` | source text | one `source_unit` per file with its flat syntax tree | done |
| 1 | Declarations | `pre_process` (`backend/preprocessor/declare.sharp`) | the trees | `sharp_program`: modules, classes, fields, functions, aliases, components, and a written `sharp_type` for every type in a declaration. Nothing looked up. | done |
| 2 | Imports | `process_imports` (`backend/postprocessor/imports/imports.sharp`) | the program | for every file the modules it can see (`sharp_file.imports`) | done |
| 3 | Type names | `resolve_type_names` (`backend/postprocessor/names/type_names.sharp`) | the program | `mutate` blocks carried out; every written type name tied to its declaration (`sharp_type.declaration`); `sharp_program.all_classes` | done |
| 4 | Class checks | `check_classes` (`backend/postprocessor/inheritance/inheritance.sharp`) | the program | base classes and interfaces as objects, duplicates reported, `override` functions linked to what they override | done |
| 5 | **Generics and members** | `resolve_members` (`backend/postprocessor/members/members.sharp`) | the program | a `resolved_type` for every field, parameter and result; generic classes instantiated lazily (a copy per use); real base classes and exact override families; default constructors; extension functions attached; modifiers and member-level rules checked; member lookup ready | done (3.5) |
| 6 | **Expression foundations** | `analyse_bodies` (`backend/compiler/driver/driver.sharp`) - the driver all body stages share | members | the analysis context, scopes and local variables; the typed tree for names, literals, member access, conversions, casts, null-safety operators, arrays; constants; access checks; the value of every field checked against the field's type. Calls, operators, lambdas are still "pending" nodes | done (10.2) |
| 7 | **Calls and operators** | same driver | stage 6 | calls of every form bound to their function (overload resolution), `new`, constructors and a written `-> base(args)`, operator overloading, assignment and `++`, string templates, dictionaries, generic function calls | done (10.3) |
| 8 | **Statements and flow** | same driver; then `check_flow` (`backend/compiler/flow/flow.sharp`), a pass of its own | stage 7 | every statement as a typed-tree object; block scopes; `break`/`continue`/`finally` rules; `foreach`, `try`, `when`, `lock`, `asm`; then reachability and "every path returns" over the finished statements | done (10.4) |
| 9 | **Lambdas and initializers** | same driver; then `order_initializers` (`backend/compiler/initializers/initializers.sharp`), a pass of its own | stage 8 | lambdas typed and their captures recorded (closure records); function names used as values; the implied base call of every constructor and the constructors the compiler adds; enum numbers; the order a class, an object and the program are set up in (`sharp_class.instance_initializers`, `static_initializers`, `fiber_initializers`, `sharp_module.global_initializers`); the components: every definition typed, every `get<t>()` and `inject` tied to the definition it receives (`check_components`, `backend/compiler/components`, a pass of its own for what needs the whole program); the names of `obfuscate` blocks (`check_obfuscate_blocks`, `backend/compiler/obfuscation`); the self-check that no expression is left without a type | done (10.5) |
| 10 | **Whole-program checks** | `check_program` (`backend/compiler/program/program.sharp`), a pass of its own | the fully analysed program | the entry point `main` (E0930, W1015); the platform's start-up functions stay empty (E0961); the modules and classes the VM needs are there (E0962); fields nothing gives a value (W1032); unused locals and private members, a local hiding a field (W1034 - W1036); what every body uses and what the program reaches from `main` (`sharp_declaration.uses`, `reached`); the warning switches (-w, -w<kind>, -wextra, -werror, -errlmt); the `analysed_program`, the one object handed to code generation (section 12) | done (10.6) |
| - | (later, not in this build) | | | code generation, optimizer | |

Stages 6 to 9 are ONE pass over the bodies, built up in four steps. They share
the driver `analyse_bodies`: it walks every class (instances of generic
classes included, also the ones that appear while it runs - a work list) and
every global, and analyses each field value and function body exactly once.
After stage 6 the driver already runs over the whole library; what a stage has
not built yet becomes a `bound_pending` node (section 6.4) that has the error
type and reports nothing. After stage 9 no expression is pending any more
(`get<t>()`, the last one, is `backend/compiler/components`); the node is
kept only as the placeholder of an argument that waits for its parameter
(6.8). What "the library gives 0
errors" proves at each stage, while later stages are still missing, is spelled
out in section 10.0.

A clean build prints, last, exactly `analysis complete: no errors`.

The timing line (`-timings`) gets one entry per stage: `members`, `bodies`
(stages 6 - 9, one walk), `flow` (the flow analysis of stage 8),
`unused generics` (the stand-in instances of generic code nothing
instantiates, 4.1 and 10.8), `components` (what the component system and the `obfuscate` blocks need the
whole program for, stage 9), `initializers` (the ordering pass of stage 9),
`completeness` (the self-check below, a pass of its own since 10.8),
`program checks` (stage 10). After it
`-timings` prints the result of the self-check (10.5, 10.8):

```
unknown: 0 expression(s) without a type and without a message
```

- every expression of every typed tree whose type is the error type, in a
file where no error was reported. The gates require 0 for the library. Since
10.8 the self-check runs on every build, not only with `-timings`, and the
first such expression of each file is REPORTED (`E0991`, internal), so a
program the analysis did not fully understand never ends with
`analysis complete: no errors`.

When stages 1-5 report errors, the members stage (if it has not run), the
body stages, the flow analysis and the unused-generics pass still run (10.8,
D94). Everything they report is added except a REPEAT - a message with the
same code at the same place as one already given (the body stages read the
type names written in bodies again). What a broken declaration makes
uncertain stays quiet: a class declared twice, or one whose base class or
interface was not found or was refused, is marked `members_uncertain`, and
neither "has no member named ..." (`E0402`) nor "does not define ..."
(`E0341`) is said about it, nor "there is no ... named ..." (`E0400`) about a
plain name inside it (it may be a member of the base that was not found - so
a name that really does not exist there shows only once the base is fixed). Components, initializers and the program checks
wait for a program whose declarations are sound.

---

## 3. The stages that exist

Brief and as built. The code of each stage opens with a longer explanation.

### 3.1 Declarations (`backend/preprocessor/declare.sharp`)

Walks every file's tree once and builds the objects of `backend/types`:

| object | file | what it holds |
|---|---|---|
| `sharp_declaration` | `sharp_declaration.sharp` | what all declared things share: `kind` (`D_CLASS` ... `D_SETTER`), `name`, `flags` (`FLAG_PUBLIC` ...), `unit` + `node` (where it is declared), `owner` (the class it is in), `module`, `members` (a `scope`: name -> list of declarations) |
| `sharp_class` | `sharp_class.sharp` | class, interface or enum: `type_parameters`, `base_type` / `interface_types` (written), `base_class` / `interfaces` (objects, set by stage 4), `base_bindings` / `interface_bindings`, `fields`, `functions`, `constructors`, `init_blocks`, `classes`, `aliases`, `enum_values` |
| `sharp_type_parameter`, `sharp_enum_value` | `sharp_class.sharp` | the `T` of `class box<T base number>` with its `constraint`; one value of an enum with `value_node` |
| `sharp_field`, `sharp_alias` | `sharp_field.sharp` | a field or global variable: `type` (written, null for `:=`), `initializer` (tree node), `getter`, `setter`, `inject_node`; an alias with its `target` |
| `sharp_function`, `sharp_parameter`, `sharp_component` | `sharp_function.sharp` | function, constructor, init block, getter, setter: `type_parameters`, `parameters`, `return_type`, `returns_nil`, `body` / `expression` / `base_call` (tree nodes), `is_operator`, `extends` (extension function), `field`, `overrides`, `implementations` |
| `sharp_type` | `sharp_type.sharp` | a written type: `form` (`TYPE_NAMED`, `TYPE_NATIVE`, `TYPE_FUNCTION`, `TYPE_NIL`), `declaration`, `arguments`, `native_kind`, `parameters`, `result`, `is_array`, `is_nullable`, `has_nullable_items` |
| `sharp_module`, `sharp_file`, `sharp_program`, `pending_mutation` | `sharp_module.sharp` | a module (all files with one `mod` name), a file with its `imports`, the program |

Function bodies and field values are NOT turned into objects here; the objects
point at their tree nodes. Errors: `E0302` (a type declared twice in one place).

### 3.2 Imports (`backend/postprocessor/imports/imports.sharp`)

For each file, the list of modules it sees: its own module first, then `std`
and `std.io.console` (always), then its `import` lines in order. `import (a.*)`
takes every module whose name starts with `a.`. Error: `E0303` (no such module).

### 3.3 Type names (`backend/postprocessor/names/type_names.sharp`, `type_name_messages.sharp`, `declared_types.sharp`, `mutations.sharp`)

1. Carries out the `mutate x { }` blocks (`mutations.sharp`): their members
   become members of `x` (flag `FLAG_MUTATED`). Done first, so later stages see
   one complete class.
2. Looks up every type name. Order, nearest first: type parameters of the
   function; the enclosing classes (their type parameters, the types declared
   in them and in their base classes); the file's module; the imported
   modules. `module#name` looks only in that module; `outer.inner` finds
   `outer` by these rules and `inner` inside it.
3. Checks the type names inside function bodies too (every `N_TYPE_NAME` of
   the tree, `sharp_file.checked` stops double work), so an unknown type in a
   body is already reported - but those names are not stored anywhere yet.

Errors: `E0301` unknown type (did-you-mean, missing import, other languages'
names), `E0303` unknown module in `a.b#t`, `E0304` module has no such type,
`E0305` no such type inside a class.

### 3.4 Class checks (`backend/postprocessor/inheritance/inheritance.sharp` and the files it calls)

| step | file | does | errors |
|---|---|---|---|
| `check_aliases` | `class_checks.sharp` | no alias stands for itself | `E0310` |
| `link_base_classes` | `base_classes.sharp` | `base_class` + `base_bindings`; a class without `base` gets `std#_object_`, an enum `std#_enum_`; `class mixin<T> base T` is only marked (`base_is_parameter`) - its base is known per instance (stage 5) | `E0320` base is not a class, `E0321` base is an interface, `E0322` base is an enum, `E0323` inherits from itself |
| `link_interfaces` | `interfaces.sharp` | `interfaces` + `interface_bindings`. The same generic interface with DIFFERENT type arguments (`: comparable<int>, comparable<string>`) is two interfaces; `: T` (a type parameter) is marked (`inherits_from_parameter`) and decided per instance | `E0324` not an interface, `E0325` listed twice with the same arguments |
| `check_duplicates` | `duplicates.sharp` | nothing declared twice in one place | `E0330` field / value, `E0331` function with the same parameters, `E0332` constructor |
| `link_overrides` | `overrides.sharp` | functions without a body and the `override` functions that define them: `overrides`, `implementations` | `E0340` global function without body, `E0341` class does not define what it must, `E0342` `override` overrides nothing, `E0343` other result type, `E0344` `override` missing |

Types are compared here with `same_type` (`backend/types/type_system/type_compare.sharp`),
which works on WRITTEN types and a `type_bindings` ("T stands for money"). It
stays the tool for declaration-against-declaration checks on blueprints; from
stage 5 on, values are compared as resolved types (section 6.1).

### 3.5 Generics and members (`backend/postprocessor/members/members.sharp` and the files it calls)

Runs only when stages 1 to 4 reported no error (a class that was just
reported as broken would be reported again in other words). `resolve_members`
does, in this order:

| step | file | does | errors / warnings |
|---|---|---|---|
| `find_well_known_classes` | `types/program/well_known.sharp` | `sharp_program.well_known`: `_object_`, `_enum_`, `string`, `throwable`, `loopable`, `pair`, `dictionary`, `function_value`, the eleven boxes; `box_of(type)`, `is_box(class)` | `E0395` |
| `check_file_rules` | `file_rules.sharp` | the `mod` line is first and there is one | `E0390`, `E0391` |
| `check_modifiers` | `modifiers.sharp` | the words in front of every declaration (7.24): allowed, not twice, order, `local` not in a class | `E0360`-`E0364`, `W1007`, `W1009` |
| `attach_extension_functions` | `extension_functions.sharp` | `def string.shout()` becomes a member of `string` (`owner`, `extension_of`); for a generic class, of the blueprint, so every instance has it; `def box<string>.f()` becomes a member of that one instance | `E0373`, `E0374`, `E0331` |
| `apply_default_access` | `default_access.sharp` | the flags a declaration has without saying so: `public` by default (an alias in a class: `private`), `static` for globals, `native` functions, aliases and `localized` variables; enum values `public static const excuse`; an enum `ext` | - |
| `check_constructors` | `constructors.sharp` | constructor names; `init(x)` forms; adds the constructor without parameters where none is written (`is_synthesized`), sets `sharp_class.default_constructor` | `E0370`-`E0372` |
| `check_interface_members` | `interface_members.sharp` | an interface declares functions WITHOUT a body (and aliases) only: no field, constructor, `init`, function with a body, class inside | `E0379` |
| `check_accessors` | `accessors.sharp` | getters / setters / `inject` as declared (7.17); one getter and one setter per field | `E0375`-`E0378`, `E0369` |
| `register_generic_functions` | `generic_functions.sharp` | `sharp_program.generic_functions`; provides `instantiate_function` for the calls stage; a generic function has a body and is never `override` | `E0358`, `E0359`, `E0345` |
| `check_function_forms` | `function_forms.sharp` | a `native` function has no body; an operator function belongs to a class | `E0365`, `E0366` |
| `check_operator_functions` | `operator_functions.sharp` | how many parameters an operator function can be used with (7.8.5) | `W1025` |
| `check_name_conflicts` | `name_conflicts.sharp` | a field, function, alias and class sharing a name in one place; a member named like a class of its module | `W1001` |
| `number_enum_values` | `enum_values.sharp` | `green = 5`: the numbers a declaration fixes (`has_number`, `number`); an enum has at least one value | `E0367` |
| `record_components` | `components.sharp` | `sharp_program.components`; one `type_definition` per `single` / `factory` (no value yet) | - |
| `ensure_members` for every written class, `resolve_global_types` | `member_types.sharp`, `postprocessor/generics/resolve_written_type.sharp` (messages: `postprocessor/generics/type_use_messages.sharp`), `generic_classes.sharp` | `value_type` / `result_type` of every member; generic instances are made as their names are met | `E0350`-`E0357`, `E0368` (a class inside a generic class named from outside it), `E0396` (internal: a type the compiler could not follow) |
| `attach_instance_extensions` | `extension_functions.sharp` | the extension functions written for one instance | `E0374`, `E0331` |
| `link_families` for every written class | `class_bases.sharp`, `override_families.sharp` (messages: `override_family_messages.sharp`) | `base_class` / `interfaces` of every real class become REAL classes (`list<string>`, not the blueprint `list`); for `class mixin<T> base T` / `: T` the checks of stage 4 are made here, per instance; override links recomputed with resolved types: `overrides`, `overridden_by`, `family_root` | `E0341`, `E0342`, `E0343` (only what the exact comparison adds); `E0320`-`E0325` for an instance |
| `complete_generic_instances` | `members.sharp`, `instance_duplicates.sharp` | the work list: every instance gets its members, is checked for functions / constructors that fall together with its type arguments, and gets its override families; new instances met on the way are appended and handled too | the same, plus `E0331`, `E0332` for an instance - each with the note "found while checking `box<_int64>`" and the place that uses that instance |

Two things were added to EARLIER stages, because that is where they are
needed:

- stage 2 (`imports.sharp`, new `import_groups.sharp`): `import (a, b) as g`
  is recorded as a group (`sharp_file.import_groups`) and no longer makes its
  modules visible by plain names (that is the C++ rule; stage 2 used to import
  them like any other). `E0393`, `W1008`. Stage 3 resolves `g#name` through
  the group (`E0304` when no module of the group has the name);
- stage 3 (`mutations.sharp`, split out of `type_names.sharp`): `mutate` is
  refused for something that is not a class (`E0380`), for a `stable` class
  (`E0381`) and with type arguments that pick one instance (`mutate box<int>`,
  `E0383`); `mutate c base b : i { }`
  gives the class its base (`E0382` when it already has one) and interfaces;
  inside `def box<T>.second() : T` type names are also looked up in `box`.
  Stage 4 no longer reports `E0340` for an extension function without a body
  (it is `E0374`).

**Objects.**

| object | file | what it is |
|---|---|---|
| `resolved_type` | `types/type_system/resolved_type.sharp` | 6.1. One object per type: the built-in ones in `sharp_program.types` (`type_table`: `error_type`, `nil_type`, `null_type`, `object_type`, `decimal_type`, `int8` ... `uint64`, `open`), a class type in `sharp_class.plain_type` (`type_of_class`), a type parameter's in `sharp_type_parameter.plain_type`. `t[]`, `t?`, `t[]??` are made from the plain type by `with_marks` and kept in it (`plain`, `marked_forms`), so the same type is the same OBJECT; only function types are compared part by part (`same_resolved_type`). `is_open()`: a type parameter somewhere in it is not known |
| `type_environment` | `types/type_system/type_environment.sharp` | 6.1, plus `instance`: the class this is the environment of. `lookup(parameter)`, `class_made_from(written class)` |
| `well_known_classes` | `types/program/well_known.sharp` | the library classes the language needs |
| `found_member` | `postprocessor/members/member_lookup.sharp` | the answer of `find_member`: `declaration` (a field, enum value, alias or class of the nearest class that has the name), `functions` (every overload, nearest first, overridden ones hidden), `distance` |
| `import_group` | `types/program/sharp_module.sharp` | `import (...) as g`: `name`, `modules` |
| `type_definition` | `types/declarations/sharp_component.sharp` | 6.9; made here with `owner`, `name`, `is_factory`, `unit`, `node`, `value_node`; `value`, `provides`, `needs` are stage 9's (`backend/compiler/components`) |

New fields on the existing objects (6.10 has the whole table):
`sharp_class`: `written` (the class as it stands in the source: itself, or for
an instance / a class nested in one the class it was made from), `blueprint`,
`type_arguments`, `instance_key`, `is_generic_instance`, `environment`,
`instances`, `instance_list`, `first_use` (the written type that made the
instance: where messages about it point as well), `base_is_parameter`,
`inherits_from_parameter` (set by stage 4), `nesting_depth`, `failed_constraint`,
`members_resolved`, `bases_resolved`, `families_linked`, `plain_type`,
`default_constructor`; `is_instance()`, `is_open()` (a blueprint or a class
inside one). `sharp_field`: `value_type`, `type_state`, `origin`.
`sharp_parameter`: `value_type`. `sharp_function`: `result_type`,
`type_state`, `origin`, `extension_of`, `is_synthesized`, `overridden_by`,
`family_root`, `blueprint`, `type_arguments`, `environment`, `instances`.
`sharp_enum_value`: `value_type`, `has_number`, `number`, `origin`.
`sharp_type`: `resolved`, `is_closed` (the cache of 6.1). `sharp_program`:
`types`, `well_known`, `generic_instances`, `instances_completed`,
`pending_functions`, `generic_functions`, `components`, `problems`.

**What the later stages call.**

| function | file | for |
|---|---|---|
| `resolve_written_type(prog, written, environment, problems)` | `postprocessor/generics/resolve_written_type.sharp` | any written type -> resolved type; makes generic instances |
| `find_member(prog, class, name)`, `find_field`, `find_functions` | `member_lookup.sharp` | a member by name, inherited ones included (7.4.3). Works on any real class; makes the members of an instance first if need be |
| `ensure_members(prog, class)` | `generic_classes.sharp` | before reading `fields` / `functions` of a class that may be an instance |
| `resolve_bases(prog, class)`, `real_base_of`, `inherits_from(prog, class, wanted)` | `class_bases.sharp` | the real base chain; "is this class that one or below it" |
| `link_families(prog, class)` | `override_families.sharp` | before reading `overrides` / `family_root` of a class met late |
| `complete_generic_instances(prog)` | `members.sharp` | after analysing something that may have named new instances |
| `instantiate_function(prog, blueprint, arguments)` | `generic_functions.sharp` | the instance of a generic function for a call |
| `type_of_class(class)`, `function_type_of(parameters, result)` | `types/type_system/resolved_type.sharp` | making types |

`-declarations` shows all of it: default flags, `-> resolved type` after
every member, for each class its real base and interfaces and what it
`reaches` through them (each inherited name asked through `find_member`),
every instance under its blueprint with the types its members have there,
synthesized constructors, override links, extension functions, enum numbers,
component definitions, and a summary line.

---

## 4. Decided designs

These three are decided. Stages implement them; they do not reopen them.

### 4.1 Generic classes: a copy per use, made lazily

```
class box<T> { item : T;  def get() : T { return item; } }

a : box<string>;        // first use: the class box<string> is made
b : box<_int64>;        // another class, box<_int64>
c : box<string>;        // the same box<string> as above
```

`box<T>` is a blueprint, not a class: nothing can be laid out or checked while
`T` is unknown. Each distinct list of type arguments gets its own real
`sharp_class`, an INSTANCE, made the first time that use is seen. (Textbook
name: monomorphisation. C++ templates work this way; Java generics do not.)
Why: the VM stores a number field in its slot and an object field as a
reference, so `box<_int64>` and `box<string>` are laid out differently and
their code uses different instructions. It is also what the C++ compiler does
with its "generic clones", so the meaning of the language is unchanged.

What follows from it (stated here because a reader may expect otherwise): a
body is checked only as part of an instance. A generic class or a generic
function that NOTHING instantiates - library-style code nobody uses yet -
would have its bodies analysed by no stage (the C++ compiler never looks at
them). So that such code is checked too, a pass after the flow analysis makes
one **stand-in instance** of each (`compiler/driver/unused_blueprints.sharp`,
`unused generics` on the time: line):

```
class box<T> { def show() { prnt(item); } }     // nothing writes box<...>
        -> box<?> is made and analysed: `prnt` does not exist
```

- every type argument of a stand-in is the error type ("not known": it fits
  everywhere and reports nothing), so whatever depends on `T` is quiet and
  whatever does not is checked like any other code - names, calls,
  statements, and the flow analysis (`check_flow_from`);
- what is wrong there is the **warning `W1039`**, not an error: it carries the
  message, label and helps the mistake would have anywhere, and a note with
  the error code it becomes once something uses the class. A warning because
  the program is complete without the unused class - the C++ compiler builds
  and runs it (D99);
- a stand-in is not part of the program. `stand_in_marks` remembers how long
  `generic_instances`, `pending_functions`, `analysed_functions` and
  `unsettled_requests` were before the pass; `forget_stand_ins` cuts them
  back and removes the stand-ins from their blueprints, so components,
  initializers, completeness, the program checks and a code generator never
  see a class with an unknown type in it;
- which blueprints get one: a generic class without instances (one nested in
  another generic class is reached through the stand-in of the outer one); a
  generic function without instances - global, in an ordinary class, or in a
  generic class (there the copy in the first instance of the class, when no
  instance of the class has an instance of that function);
- what stands above a stand-in is a stand-in too: for `class kid<T> base
  parent<T>` the base of `kid<?>` is `parent<?>` (`stand_in_above`,
  inheritance/class_bases.sharp), and the same for its interfaces - the
  members it inherits are known whatever `T` is. Where what is written is no
  generic class (`base T` itself), the stand-in is marked `members_uncertain`
  and no name is reported missing in it;
- an override check cannot be made for a stand-in (the parameter types above
  it are unknown), so `link_families` says nothing about one
  (`has_unknown_type_argument`).

On the standard library the pass finds a real mistake in a class nothing
uses: `atomic_ref<t>.write` calls `self->_unlock()`, which does not exist
(both overloads; std/io/atomic_ref.sharp lines 25 and 35).

One rule had to change for the stand-ins, and it holds everywhere: an
argument of the error type fits every parameter, so with several overloads
the "winner" was merely the first one. When another overload could take the
arguments too and returns ANOTHER type, the call is now a tie that is not
reported and has the error type (`is_a_guess`, overloads/overloads.sharp) -
otherwise `abs(key.hash() % n).value` in a stand-in, or `pick(nope).length()`
in any program, was judged by the result type of a guessed overload.

**What an instance is.** An ordinary `sharp_class` with

| field (added to `sharp_class`) | meaning |
|---|---|
| `blueprint : sharp_class?` | the generic class it was made from; null for an ordinary class |
| `type_arguments : list<resolved_type>` | what each type parameter stands for, in order |
| `instances : hash_map<string, sharp_class>` | on a BLUEPRINT: its instances by key (below) |
| `instance_list : list<sharp_class>` | on a blueprint: the same, in the order they were made |
| `environment : type_environment?` | on an instance: "T is string", chained to the environment of the class it is nested in (section 6.1) |
| `members_resolved : _int64` | 1 once stage 5 gave its members their resolved types |

Its name is the blueprint's with the arguments: `box<string>`; `full_name()`
is `main#box<std#string>`.

**The key.** The canonical text of the arguments, comma separated, each
written as `resolved_type.key()` - the full name of the class, the native
keyword, `[]` and `?` included: `std#string`, `_int64[]`, `std#list<std#string>?`.
Two uses are the same instance when their keys are equal. Aliases and type
parameters never appear in a key: they are looked through before it is built.

**What is copied.** `instantiate(blueprint, arguments)`
(`backend/postprocessor/generics/generic_classes.sharp`) makes the new class and, for
each member of the blueprint, a new member object of the same kind:

- a new `sharp_field` / `sharp_function` / `sharp_parameter` per field,
  function, constructor, init block, getter, setter - `owner` is the instance,
  `unit` and `node` are the blueprint member's (the SAME tree nodes), and
  `origin` points at the blueprint's member;
- the WRITTEN types (`sharp_type`) are shared, not copied: a written type is a
  place in the source and stays one. What differs per instance is the RESOLVED
  type of each member, computed from the written type under the instance's
  environment;
- classes declared inside a generic class (`list<T>.node`) are instantiated
  with it: `list<string>.node` is a class of its own whose environment chains
  to `list<string>`'s;
- members a `mutate` block added to the blueprint are members like any other
  (stage 3 has already put them there), so every instance has them **[differs:
  the C++ compiler loses them for instances made later]**;
- extension functions written for the generic class (`def list<T>.shout()`)
  were attached to the blueprint by stage 5 and are copied the same way.

Base class and interfaces of an instance are resolved under its environment:
`class names<T> base list<T>` - `names<string>` has the base `list<string>`,
which is instantiated on the spot if it does not exist. The override links
(`overrides`, `implementations`) are made again for the instance's own
function objects (`link_overrides_of`, with reporting off: the blueprint was
already checked in stage 4).

**Lazily, and where.** An instance is made inside `resolve_written_type`
(section 6.1) - the one function that turns a written type into a resolved
type - whenever it meets a generic class with arguments. So it happens for
`a : box<string>` in a declaration (stage 5), for `new box<string>()`,
`box<string>.make()`, `x as box<string>` in a body (stages 6 to 9), and for the
dictionary literal's `pair<k, v>` / `dictionary<k, v>`. A new instance is
appended to `sharp_program.generic_instances`, a list that only grows; each
pass over the classes keeps the index it has reached (stage 5:
`instances_completed`) and walks the list to its end, so an instance that
first appears in the middle of a body is handled like every other class.
(As built; the first plan had a list that is emptied, `pending_instances`.
A list that keeps every instance is also what `-declarations` and a code
generator need.)

**Making an instance never touches the syntax tree, and never touches the
blueprint.** Everything an instance needs that differs from the blueprint is
in objects of its own: its member objects, their resolved types, their typed
trees, its environment. What is the SAME for every instance is shared and
read-only: the tree nodes, and the written types - those of declarations
(made by stage 1) and those of types written inside bodies, which are made
once per tree node and kept in a table beside the tree
(`sharp_file.body_types`, section 6.1), not on it. The test for "is this
right": deleting an instance object must leave the program exactly as it was
before the instance was made.

**The members of an instance are created in two steps**, so that an instance
is cheap until it is used. `instantiate` makes the class object, its
environment, and registers it; the member objects are made by
`ensure_members(instance)` the first time anything asks the instance for a
member (`find_member`, the body driver, `-declarations`). `members_resolved`
records that this has happened. Nothing outside `generic_classes.sharp` reads
`fields` / `functions` of a class that may be an instance without calling
`ensure_members(prog, class)` first (`find_member` does it for its callers).

**The body of a blueprint is never analysed.** Only instances are: inside
`box<T>`, `item.size()` means nothing until `T` is known. (Sharp generics are
"duck typed" like C++ templates: a body may use whatever the actual type
happens to have.) Consequences, all intended:

- a generic class that is never used gets its declarations checked (stages 1
  to 5) but not its bodies - same as the C++ compiler;
- a mistake in a generic body would be reported once per instance. The
  diagnostics sink drops a diagnostic whose code and source range it has
  already seen (section 9), so it is reported once; when the mistake depends
  on the type argument the message gets the note "while checking `box<_int64>`";
- every body of every instance is analysed, used or not. This is what the
  C++ compiler does for the user's own generic classes, and for library
  classes when it is run with `-eager`; by default it puts library function
  bodies aside and compiles only the ones the program reaches
  (`compiler/functions/lazy_functions.cpp`), so a library function that does
  not fit one particular instance goes unnoticed there. The bootstrap does
  not copy that: an analysis that skips bodies is not "fully analysed". The
  evidence that the library allows it is in decision 3 of section 11.

**What is checked at instantiation** (stage 5): the number of arguments
(`E0350`), each `T base x` constraint - the argument must be `x` or inherit
from it, and must not be nullable or an array (`E0351`), runaway nesting such
as `class a<T> { x : a<a<T>>; }` (a depth limit of 32 levels, `E0352`),
arguments given to something that is not generic (`E0353`) and a generic class
used without arguments where a real type is needed (`E0354`).

**What only an instance can show**, and is therefore checked for each
instance when the work list reaches it:

| what | example | code |
|---|---|---|
| a type that is fine for most `T` | `value : T?` in `optional<_int64>` | `E0355`-`E0357` |
| two functions or constructors that fall together | `def put(a: A)` and `def put(b: B)` in `either<int, int>` | `E0331`, `E0332` (`instance_duplicates.sharp`) |
| the same interface twice | `class both<A, B> : source<A>, source<B>` as `both<int, int>` | `E0325` |
| an `override` that fits `T` but not what `T` stands for; a required function that only looked defined; another result type | section 3.5 | `E0341`-`E0343` |
| a base class that is a type parameter | `class mixin<T> base T` as `mixin<_int64>`, `mixin<an_interface>`, `mixin<an_enum>`, or so that it inherits from itself | `E0320`-`E0323`; `: T` with something that is not an interface: `E0324` |

Every such message stands inside the generic class - which is fine as written
- so it gets a note naming the instance ("found while checking `either<int,
int>`") and a second place: where that instance is first used
(`sharp_class.first_use`, shown as "`either<int, int>` is used here"). One
mistake in the text of a generic class is reported once, however many
instances show it (`sharp_type.instance_problem_reported`,
`sharp_function.exact_problem_reported`); a mistake that belongs to ONE
instance (its base class, its interfaces) is reported for that instance.

**A class inside a generic class** (`list<T>.node`) can only be named from
inside that class (also from a `mutate` of it or an extension function of
it): there the name means the copy that belongs to the instance at hand.
Anywhere else - `x : list.node`, `x : list<string>.node`, or `node` reached
through a base class `base list<int>` - nothing says which instance is meant,
and it is `E0368`. The C++ compiler cannot resolve these either ("could not
resolve symbol `node`").

**A type parameter as base class or interface** - `class mixin<T> base T`,
`class tagged<T> : T` - is allowed, as in the C++ compiler (whose base class
step runs on every generic clone). The blueprint has `_object_` as its base;
each instance gets its real base in `resolve_bases`, with the checks above.
`override` functions in such a class cannot be checked before an instance
exists, so stage 4 leaves them alone and stage 5 reports an `override` that
finds nothing in the instance (`E0342`).

**Generic functions** (`def largest<T>(a: T, b: T) : T`) work the same way,
per function: the declaration is a blueprint (`sharp_function.type_parameters`
not empty); a call `largest<int>(p, q)` makes - or finds - the instance
`largest<std#int>`, a copy of the function object with its own environment and
its own typed tree (`sharp_function.blueprint`, `type_arguments`, `instances`).
Type arguments must be written at the call; they are not worked out from the
arguments (`gen_f(5)` is "could not resolve symbol" in the C++ compiler). A
lambda inside a generic function uses the function's environment.
The registry is built in stage 5, instances are made by stage 7 (calls). A
new function instance is put on `sharp_program.pending_functions`, the
function counterpart of `generic_instances`, and the body driver handles both
lists to their ends before it finishes. A generic function inside a generic class is
instantiated per class instance first (it is a member like any other, still a
blueprint) and per call second; its environment chains to the class
instance's.

### 4.2 The typed tree is separate and owned by what it belongs to

The syntax tree is never changed and never annotated. One tree node means
different things in different instances of a generic class - in `box<string>`
the node `item` has the type `string`, in `box<_int64>` it is a number - so
nothing can be stored "on the node". Instead analysis BUILDS a second tree,
the typed tree (section 6.4), and each tree belongs to the declaration it was
built for:

| owner | field | holds |
|---|---|---|
| `sharp_function` (also constructors, init blocks, getters, setters, lambdas, instances of generic functions) | `bound_body : bound_block?` | its statements |
| | `bound_base_call : bound_expression?` | a constructor's `-> base(args)`: a `bound_call` of the base class's constructor with dispatch `CALL_CONSTRUCTOR` (a `bound_pending` while a lambda argument still decides which constructor); null when none is written - the implied call of the constructor without parameters is added by stage 9 (10.5) |
| | `locals : list<local_variable>` | parameters first, then every local in the order declared |
| | `closure : closure_record?` | the variables its lambdas capture (4.3) |
| `sharp_field` | `bound_initializer : bound_expression?` | its value, already converted to the field's type |
| | `default_constructor : sharp_function?` | for a field without a value whose type is a box or `string`: the constructor that makes its default object (7.3). Not for a field WRITTEN with a type parameter (`value : T;` in a generic class): in every instance it starts null, also where `T` is `string` (codegen.md 13.3, G23) |
| `sharp_class` | `instance_initializers : list<sharp_declaration>` | what runs for every new object, in order: the instance fields that have a value or a default object, and the `init { }` blocks, as written (7.15.2 step 3) |
| | `static_initializers : list<sharp_declaration>` | the same for the class itself: static fields with values, enum values, `static init { }` blocks (step 2) |
| `sharp_enum_value` | `constant : constant_value?` | its number, when the program fixes it (7.16) |
| | `bound_value : bound_expression?` | the expression written after `=`, if any |
| `sharp_component` | `definitions : list<type_definition>` | each `single` / `factory` with its typed expression |

A constructor the compiler adds (7.7.5) is an ordinary `sharp_function` with
`is_synthesized = 1`, `node` = the class's node, an empty `bound_body` and a
`bound_base_call` like any other constructor. A code generator treats it like
a written one.

Every typed-tree object keeps the `unit` and `node` it came from, for messages
and later for line numbers. `box<string>.get` and `box<_int64>.get` are two
`sharp_function` objects (4.1), so each has its own typed tree.

A code generator will walk `sharp_program` -> classes -> functions ->
`bound_body` and find, on each object, everything it needs: the exact
function of a call, the conversion to apply to each argument, which variable a
name is, where a `break` goes.

**The test of the typed tree** - a stage that adds a node asks it of the
node: could code be generated for this node from its own fields, the fields of
the objects it points at, and nothing else? In particular without looking at
the syntax tree, without looking a name up again, without comparing two types
to find out which conversion is meant, and without knowing a rule from
section 7. Whatever fails the test is a missing field. Three consequences
that shaped section 6.4:

- sugar is removed where the meaning is a call: `a + b` on objects, `o[i]`,
  `o[i] = v`, a property read or write, `x += y` on an object are all
  `bound_call`s of the function that does the work, with a mark saying how
  they were written (for messages only);
- what is evaluated once is said once: `a?.b.c` is a `bound_safe_navigation`
  holding the receiver and "the rest", a `when` subject is a hidden local;
- nothing in a finished tree is changed later. A fact that only the whole
  program knows (is this function overridden anywhere? is this variable
  captured by a lambda further down?) lives on the DECLARATION
  (`sharp_function.overridden_by`, `local_variable.is_captured`), where it can
  still change until analysis ends, and the node points at the declaration.

### 4.3 Function values are objects that carry what they captured

(From `doc/compiler_internals.md` sections 12 and 13, which describe the C++
compiler and the runtime. The runtime side is fixed; the compiler side below
is the bootstrap's design of the same thing.)

A function value - a lambda, or the name of a function used as a value - is an
object of `platform.kernel#function_value` with two slots: `address` (which
function) and `captured` (an object holding the variables the function uses
from around it, or null). Calling it (`CALLD`) passes `captured` as a hidden
last argument. Its TYPE is a function type, `(int, int)(decimal)`: the hidden
argument is not part of it.

```
def make_counter() : ()(int) {
    total := 0;                                 // captured: lives in the closure record
    return { -> (int) total++; return total; }; // function value { address, captured -> record }
}
```

**In analysis** (stage 9) this becomes three things:

1. *Lambda as a function.* Each lambda is a `sharp_function` of its own
   (`kind = D_FUNCTION`, `FLAG_STATIC`, `is_lambda = 1`, `name` `lambda@line`,
   `owner` the class it is written in, `enclosing : sharp_function` the function
   it is written in). It has parameters, a result type, `bound_body`, `locals`
   like any function. The expression is a `bound_lambda` whose type is the
   function type.
2. *Captures.* When name lookup (section 7.4) walks out of a lambda's scopes
   into the scopes of the enclosing function and finds a local or parameter
   there, that variable is CAPTURED: `local_variable.is_captured = 1`. Each
   function that has captured variables gets one `closure_record`
   (the object: `backend/types/declarations/closure_record.sharp`; the
   bookkeeping when a capture is found: `backend/compiler/context/closures.sharp`):

   | field | meaning |
   |---|---|
   | `function : sharp_function` | the function whose variables these are |
   | `variables : list<local_variable>` | the captured ones, in order of first capture |
   | `captures_self : _int64` | 1 when a lambda inside uses `self` or an instance member (allowed only when `function`, or the function around it, is an instance function) |
   | `parent : closure_record?` | for a lambda that itself has lambdas inside: the record it received |

   Whether a variable is captured is a fact about the VARIABLE, and it is
   only final when the whole function has been analysed: a lambda near the
   end of a function can capture a local that was used twenty lines earlier,
   when nobody knew. So a `bound_local` does not say "captured"; it points at
   its `local_variable` (whose `is_captured` the code generator reads when
   analysis is over) and records only what is known at that moment and never
   changes: `depth : _int64`, how many function boundaries lie between this
   use and the function the variable belongs to (0: the function's own
   variable; 1: used in a lambda directly inside it; 2: a lambda in that
   lambda ...). A use with `depth > 0` is what MAKES the variable captured.
   Where the variable lives then follows without further questions: not
   captured - the stack frame; captured, used at depth 0 - the record of its
   own function; used at depth `d > 0` - the record the lambda was handed
   (that of the function directly around it), then `parent` followed `d - 1`
   times. (The first draft said `d` times; the record handed to a lambda is
   already one step out.) A captured PARAMETER is copied into the record
   when the function is entered (`is_parameter` and `is_captured`). One
   record object is created per CALL of the function, so recursion and
   fibers each have their own variables.

   As built (stage 9): a lambda that is handed a record has
   `sharp_function.takes_record = 1` - that is the hidden last parameter;
   `bound_self.depth` and `bound_base.depth` count lambdas the same way, and
   the object is kept in the record of the outermost function
   (`captures_self`). A lambda written in the VALUE of an instance field has
   no function around it: the record it is handed belongs to the field
   (`closure_record.field`, `sharp_field.closure`), made when the object's
   fields are set up. Every lambda is listed in `sharp_program.lambdas` and
   in the `lambdas` of the function (or field) it is written in.
3. *Any type can be captured* **[differs]**. A captured variable lives ONLY in
   the record (the textbook "closure conversion": the variable is moved from
   the stack frame to a heap record that outlives the call). The C++ compiler
   keeps the variable in the frame as well and copies every write, which only
   works for references - hence its error "cannot capture closure of local
   field ... Only arrays, classes and objects are allowed". The bootstrap has
   no such restriction: a captured `_int64` is a number field of the record.

**A function's name as a value** (`f := twice;`) is a `bound_function_reference`
with nothing captured. Only functions that need no object qualify: global
functions and `static` functions (`E0804` otherwise - the help writes the
lambda that keeps the object: `{ -> d.bark() }`).

**Comparing.** `f == null` asks "is there one". `f == g` between two function
values is the call `function_value.same(f, g)` (`!=`: `different`): the same
function with the same captured record.

**A lambda that captures nothing** has no record: `bound_lambda.record` is
null and so is the function value's `captured`. A lambda inside a lambda
needs a record of the OUTER function only if it (or a lambda inside it) uses
one of that function's variables; the middle lambda then passes its own
record's `parent` on. `closure_record.parent` is set only when that is so.

**What is NOT decided here** and belongs to code generation: the layout of the
record class and the instructions. Analysis hands over the `closure_record`s,
`is_captured` on every variable and the `depth` of every use.

---

## 5. Deliberate differences from the C++ compiler

Each is marked **[differs]** where its rule is stated. "Wider" means the
bootstrap accepts everything the C++ compiler accepts and more; "stricter"
means it rejects something the C++ compiler lets through.

**The rule for "stricter"** (it settles decisions 1 and 2 of section 11).
A program the C++ compiler accepts AND GIVES A MEANING TO is accepted by the
bootstrap. Where the C++ compiler accepts something that a sound language
would refuse, but the program still runs as written - assigning a `const`,
calling a `private` function from outside, giving a base-class value to a
derived-class variable - the bootstrap accepts it too and reports a WARNING
(`W1017`, `W1021`, `W1022`), so the places are visible and can be made errors
in language 2.0 by changing one line each (the severity is decided in one
function, as built: `report` in `backend/compiler/context/reporting.sharp`, where a code that starts with `W` is a warning).
The bootstrap is stricter with an ERROR only where what the C++ compiler
accepts has no meaning at all: an expression it gives no type to, code that
can only raise `nullptr_exception`, an object of an interface. Each such case
is in the table with the probe that showed it. Because of this rule no stage
ever has to choose between a false error on the library and editing the
library: if a "stricter" error fires on the library or the suite, the code
there has a meaning the stage has not understood, and the stage is wrong.

**How the rules were checked.** Section 7 was written from the C++ sources
and then tested with about thirty small programs run through the C++ compiler
(and the VM, where the question was what a program prints). One thing to know
when doing that: the C++ compiler drops an error whose TEXT it has already
printed, wherever it occurs. Two wrong lines that produce the same sentence
show as one error, and the second line looks accepted. Probe one mistake per
file, or make sure the names in the messages differ.

| # | topic | C++ compiler | bootstrap | kind |
|---|---|---|---|---|
| D1 | analysis result | "schemes" (lists of code generation steps) built while type checking, read by position | a typed tree; no code-generation detail | design |
| D2 | context | one mutable context per file with saved copies; a late generic instance can see the locals of the function that triggered it | an `analysis_context` object per body; an instance is analysed on its own | design, fixes a bug |
| D3 | overload resolution | the first exact match wins; else exactly one match after conversions; else "ambiguous" unless the plain-number rule singles one out | the same three steps first; where they give up, every applicable candidate is ranked argument by argument and the best wins (section 7.6). Every call the C++ compiler resolves gets the same function (one exception, on purpose: D13) | wider |
| D4 | global functions | modules searched in import order, the first module with a match wins; overloads never compared across modules | the same: a function in a nearer module hides the name in a module further away (7.6 step 1). (The first draft of this plan ranked all modules together, which would have changed which function an existing call means.) | same |
| D5 | user conversions | which of constructor / `init(x)` / `operator=` is tried depends on the place (argument, declaration, assignment ...); in a declaration with a value an `init(x)` runs even when a constructor takes the value too (checked by running) | one rule everywhere (section 7.5.4), except that a DECLARATION with a value prefers an `init(x)` that takes it, as there (10.10), so a program both accept runs the same code | wider |
| D6 | base to derived | accepted silently EVERYWHERE a value is given to a typed place - `d : dog = an_animal;` and also `takes_dog(an_animal)` (`is_implicit_class_type_match` in `sharp_class.cpp` is symmetric); nothing is checked | accepted: a conversion of its own (`CONV_DOWNCAST`, ranked below every safe conversion), marked as needing a run-time check, with the warning `W1017` ("write `an_animal as dog`") | same language, plus a warning |
| D7 | `const` | parsed and stored, never enforced (assigning a const field, global or local compiles and runs) | the assignment is accepted with the warning `W1021` (section 7.10) | same language, plus a warning |
| D8 | private functions and constructors | `private` is checked for fields and for classes, not for a function (instance or static) or a constructor | accepted with the warning `W1022`; private fields and classes are errors as in the C++ compiler (`E0700`) | same language, plus a warning |
| D9 | operator on a class without that operator | a BINARY operator (`a + b`, `a < b`) compiles and the expression has no type: using its value is an error somewhere else, not using it is silent. `==` `!=` fall back to "same object". Unary `-a`, `!a` and `a += 1` are errors | `==` / `!=` fall back to "same object"; every other operator is an error where it stands (`E0511`) | stricter: the C++ result has no type |
| D10 | captures | numbers cannot be captured by a lambda | anything can (4.3) | wider |
| D11 | lambda result | a lambda without a written `(t)` is "not fully qualified" unless the place it is used gives the type | also inferred from its `return` statements when neither is there | wider |
| D12 | "every path returns" | eight flags per block; misses `while(true)`, `try`/`finally`, nested blocks (`def f() : _int64 { while(true) { } }` is "not all code paths return a value") | a flow analysis (section 7.13) | wider |
| D13 | type of integer arithmetic | `_int32` whatever the operands are. The label is not harmless: it picks the overload. With two `_int64` variables holding 5000000000 and 1, `"" + (big + one)` calls `string.operator+(_int32)` and prints `705032705`, and `k := big + one` makes `k` an `_int32` holding the same wrong number | `_int64` (`_uint64` when both are) when either operand is a 64-bit integer, otherwise `_int32` exactly as before. Operands of 32 bits or less, and literals that fit 32 bits, are typed as the C++ compiler types them, so those programs call the same overloads | fixes a bug; decision 6 in section 11 |
| D14 | truth values | number comparisons and `&&` `\|\|` give `_int32`; `!x`, null tests, "same object" tests and `is` give `_int8` | the same. Making them all one type looks tidy and would change programs: `"" + (a < b)` prints `1` because the result is an `_int32`; as an `_int8` it would be appended as the CHARACTER with code 1 | same |
| D15 | enum numbering | a value without `=` takes the next number of a counter that starts, for each enum, at a program-wide number depending on compile order (`red` was 16612 in a probe); after `green = 5` the next value is 6. The source says the start is unspecified on purpose: such values "are only meant to be told apart" | the same rule, stated: a value written `= n` is `n`; a value after it is one more; a value before any `=` has NO number a program may rely on, only that it differs from the other values of its enum. `sharp_enum_value.constant` is set only where the program fixes the number; code generation picks the rest | same language; design |
| D16 | `mutate` on a generic class | applied only to instances that exist at that moment | part of the blueprint, so in every instance | fixes a bug |
| D17 | member lookup in a base class | written out separately for fields, aliases, functions, function-typed fields | one `find_member` (section 7.4.3) | design |
| D18 | no `main` | an error in every build | not reported by analysis: a library has none. It belongs to building an executable (code generation) | design |
| D19 | follow-on errors | one unknown name gives up to five messages | the error type: one message | design |
| D20 | name conflicts | field / class / function with one name: a warning per pair | the same warning (`W1001`), once | same |
| D21 | repeated messages | once per generic instance - and the opposite fault: a second error with the same TEXT anywhere in the program is dropped | once per place (code + file + range, section 9) | design |
| D22 | cast between a class and an interface | only when the class is known to implement it | always allowed (a subclass may implement it); checked at run time | wider |
| D23 | unknown name | "could not resolve symbol" + sometimes a class with a similar name | did-you-mean over everything visible, "exists but ..." explanations (section 9) | design |
| D24 | `x?.member` where the member is a plain number | a field read gets the type `_int64?`, which nothing accepts (not a variable, not a parameter, not `+`); compared with `==` it compiles and raises `nullptr_exception` when `x` is null. A call (`x?.count()`) is the error "cannot be nullable" | an error for both (`E0725`); the help gives the ways out: `x!!.member`, or `x == null ? 0 : x!!.member` | stricter: the C++ result cannot be used |
| D25 | items of a `t[]??` array | the source means an item to be a `t?` (`array_expression.cpp:46`, `for_each_statement.cpp:85`), but the mark is lost when the array is a variable or a field: `take(xs[0])` and `foreach(s in xs) { s.size(); }` compile | an item read from a `t[]??`, and a `foreach` variable over one, is a `t?` (7.11). Neither the library nor the suite declares such an array (searched), so nothing that exists changes | stricter: what the C++ source intends |
| D26 | `new` of an interface or an enum | accepted | `E0507`: an interface has no bodies; an enum's values are made by the compiler | stricter: no meaning |
| D27 | `null` for a function-typed variable | `f : ()()? = null;` is "types do not match" in a declaration (a slip: the same field left without a value is null) | accepted like any other nullable type | wider |
| D28 | `_int64?` written directly | accepted in a declaration (the check "cannot be nullable" only runs for a type reached through a NAME: an alias, a type parameter, a member) and then unusable | `E0355` wherever it is written or arises. Neither the library nor the suite has one (searched) | stricter: no meaning |
| D29 | interface with a field, a constructor or `init` | the C++ PARSER refuses the field ("unexpected variable declaration"); the bootstrap parser accepts all three | reported by stage 5 (`E0379`) | same language |
| D30 | modifiers on a constructor | a constructor is processed as a function and may be written `override`, `static`, `native`, `local` | only `public` / `private` / `protected` (`E0360`) | stricter: no meaning |
| D31 | which generic bodies are checked | every body of every instance for the user's classes; library function bodies only when the program reaches them, unless run with `-eager` | every body of every instance, always (4.1) | design; decision 3 in section 11 |
| D32 | a lambda parameter or local named like a local of the function around it | allowed: inside the lambda the name means the lambda's own | the same (7.4.2). (The first draft of this plan made it an error; the C++ compiler accepts it.) | same |
| D34 | a generic class named without type arguments outside itself (`a : box?;`) | accepted in a declaration; the variable then has the blueprint as its type, which no value has | `E0354` | stricter: no meaning |
| D35 | a generic class that refers to itself with a GROWING type argument (`class e<T> { deeper : e<e<T>>?; }`, used as `e<int>`) | accepted: it makes generic clones only as far as something walks | `E0352`: every instance gets its members (decision 2 of section 11: every body of every instance is analysed), and here there is always one more instance. The limit is 32 levels | stricter, by design; **for the owner to confirm** (10.1) |
| D36 | `mutate` of a `stable` class | refused only for a `mutate` written inside a class body; at the top of a file (where every `mutate` in the library stands) the check is never reached | `E0381` wherever it is written - the rule the C++ source states | stricter: the C++ behaviour is a slip |
| D37 | flags of a function | `public` is added to EVERY function, private ones too (which is why D8 exists) | a private function is `FLAG_PRIVATE` only | design |
| D38 | an operator function with a number of parameters its operator cannot have | not checked | `W1025` | same language, plus a warning |
| D39 | a generic function without a body, or marked `override` (`class a { def f<T>(x: T); }`, `override def f<T>(x: T) { }`) | accepted; the words are then ignored - a call `b.f<int>(x)` is bound like any generic call, and nothing ever checks that a class below defines `f` | `E0345`: a generic function is made per call, so there is no one function for a class to replace (7.19) | stricter: no meaning |
| D40 | `mutate box<int> { }` / `mutate box<T> { }` on `class box<T>` | `mutate box<int>` is accepted and what it adds is not found on any other instance; `mutate box<T>` is "could not resolve symbol `T`"; `mutate box` is the form that works | `mutate box` and `mutate box<T>` (the class's own parameter names) change the class as a whole; any other type arguments are `E0383`, with the help "for one instance only: `def box<int>.name()`" | wider for `box<T>`, stricter for `box<int>` (it read as something it did not do) |
| D41 | a `native` function with a body; an operator function at the top of a file with no class before its name | the compiler stops with an internal error ("could not resolve function", "unknown ast type: 16") | `E0365`, `E0366` | same rule, a real message |
| D42 | a second `get` / `set` on a field; an enum without values; a function WITH a body or a class inside an interface | refused by the PARSER ("unexpected get declaration", "expected identifier", "unexpected method declaration", "expected delegate declaration") | the bootstrap parser reads them; stage 5 reports `E0369`, `E0367`, `E0379` with what to write instead | same language |
| D43 | a name declared twice as a type in one place when one of them is an alias or a type parameter (`alias int as n; alias string as n;`, `class a<T> { class T { } }`) | accepted (the second silently wins or loses) | `E0302`, by stage 1 | stricter: no meaning |
| D44 | a function a class declares again with the same parameters as one of its base class, WITHOUT `override` (the one above has a body) | a call on the lower class runs the lower one when the argument is exact (`new string("x")`); when it needs a conversion (`"x"` for a `string`, a class for its interface) the call is "ambiguous" (checked, 10.9). A call through a variable of the base class runs the base's function (functions with a body are never replaced) | `find_member` returns only the nearest of the two (7.4.3), so every call on the lower class runs the lower one; the declaration gets `W1041`, which says the function only hides the other and how to get run-time choice. The same for two `static` functions (`leaf.make("x")` is ambiguous there too, checked) and when the one above is `private` (checked) | wider |
| D33 | `object` and null | a variable of type `object` (no `?`) takes `null` and takes a `t?` value without complaint | the same: `object` is the one type that is nullable without the mark (7.5.1) | same |

| D45 | `base.name` (a dot after `base`) | means `base` alone: `s : string = base.name;` is "cannot assign ... type `circle`" (a parser accident) | the member `name` of the base class, like `base->name` | wider |
| D47 | where type parameters are looked for when a plain name is resolved | first | last (after locals, the class, the modules) | design: no program names a local like a type parameter, and it saves a tree walk per name |
| D48 | `s : string = c;` with `c : _int8`; `b : long = an_int;` (a conversion through the class in a declaration) | refused in a declaration, accepted as an argument | accepted everywhere (the one rule of 7.5.4, D5) | wider |
| D49 | a field declared `half := width;` whose value names another instance field by its plain name | "could not resolve symbol `width`" - although `half : decimal = width;` and `half := self->width;` compile (the value of an untyped field is looked at once without an object) | accepted: the value of an instance field has a `self`, typed or not | wider |
| D50 | `{ maybe }` / `{ sure, maybe }` with `maybe : node?` | the array literal copies the first item's `?` onto the ARRAY (`{ maybe }` cannot be given to a `node[]`), and later items are compared without their `?` | the array is a `node[]`; every item that may be null is `W1026` (7.7.5) | wider for `{ maybe }`, plus a warning |
| D51 | a local used in its own declaration: `n : _int64 = n + 1;` | accepted; the variable is read before it has a value | `E0407` "used in its own declaration" | stricter: no meaning |
| D52 | `x as t?` for an `x : t`; `anything!!` / `anything ?: y` for an `object` | warnings ("redundant cast", "redundant use of `!!`", "found to be non nullable") | `W1003` for the cast; nothing for `object`, which may hold null without the `?` (D33) | same language |
| D53 | a generic function called without its type arguments (`first_or(names, "-")`) | "could not resolve symbol": the type arguments must be written | worked out from the arguments by unification (10.3); `E0517` only when one cannot be | wider |
| D54 | `p?.x` read as a plain number where a number is wanted (`expect("...", a.next?.value, 4)`, suite case `nullability.sharp`) | accepted; the read throws when `p` is null | accepted with `W1027`, typed as the plain number (the first plan made it `E0725`; the suite case shows it has a meaning). A CALL through `?.` that gives a number stays `E0725`, as in the C++ compiler | same language, plus a warning |
| D55 | `x % 0` with a variable `x` | accepted (only two literals are folded and checked) | `E0427` whenever the right side is the constant 0 | stricter: no value |
| D56 | `5++`, `f()++` on a number that is no variable | accepted, changes nothing | `E0711` | stricter: no effect |
| D57 | a call with several fitting one-parameter constructors for a value (`x : int = small;`) | the first explicit match, else "ambiguous" | the same order, then ranks and number grades (7.5.4); constructors before `init(x)` | wider |
| D58 | assigning through `?.`: `maybe?.age = 5;` | accepted; it stores like `maybe!!.age = 5` and the program throws `nullptr_exception` when `maybe` is null (checked by running it) - the opposite of what `?.` promises | `E0711`, with the help to write `!!.` or to test for null first | stricter: the accepted meaning contradicts the operator |
| D59 | an object target that is no place: `make() = other;`, `(a ?: b) = c;` | a class-typed left side is never checked for being a place: the value is stored into a temporary and lost | `E0711` - unless the class of the object has an `operator=` that takes the value (`title() = "x";` changes the string the call gave back, in both compilers, 7.10) | stricter: no effect |
| D60 | a type argument that breaks `T base x` on a generic FUNCTION, written out: `only_animals<table>(t)` | accepted (the constraint is checked for generic classes only) | `E0518` | stricter: the constraint is what the body relies on |
| D61 | `break;` / `continue;` outside every loop | the C++ compiler crashes (segmentation fault: `retrieve_loop_block` returns null) - checked | `E0602` / `E0603` | stricter: no meaning |
| D62 | `lock(items) { }` with `items` an array held in a variable or parameter | "attempt to lock non-lockable object of type `_int64[]`" (it tests the variable's type, which is not marked as an array) - checked | accepted: every object has a lock, an array too (7.12.2) | wider |
| D63 | an `asm` label defined twice (`.here: .here:`), or jumped to and never defined (`jmp @nowhere`) | accepted silently: a label is made on its first mention - checked | `E0921`, `E0922` | stricter: no meaning (the jump has no place, or two) |
| D65 | a loop INSIDE a `finally` block, left with its own `break` / `continue`: `finally { for(;;) { break; } }` | "control cannot leave body of finally clause" (it asks "is there a finally block anywhere around the jump", not "between the jump and its loop") - checked | accepted: the jump stays inside the finally block. Only a jump that would LEAVE the block is `E0604` | wider |
| D66 | a `bool?` as the condition of a statement: `if(flag)` with `flag : bool?` | accepted (the condition is asked for the class `std#bool` by name; the `?` is not looked at); null throws when it runs - checked | accepted with the warning `W1028`, read as `flag!!` (7.12.1) | same language, plus a warning |
| D67 | an ARRAY of exceptions thrown: `throw new oops[2];` | accepted (the class of the array's items is tested) - checked | `E0612`: only one object can be thrown, and no `catch` can name an array (`E0613` in both compilers) | stricter: no meaning |
| D68 | a clause after the `else` of a `when`, or a second `else` | refused by the PARSER ("expected `}`") | the bootstrap parser reads the clauses in any order; stage 8 reports `E0619` with what to move | same language |
| D69 | a function value passed as an ARGUMENT whose result differs from the parameter's function type (`new thread("w", worker)`, `worker : (object[])(decimal)` for `(object[])(int)`; `fiber_main` returning nothing for `(object[]?)(decimal)`) | accepted: only the parameters of an argument's function type are compared (`is_explicit_type_match`); in a declaration the result is compared - checked | accepted with `W1029` (the conversion `CONV_RESULT_IGNORED`, nothing at run time); refused in a declaration as there | same language, plus a warning |
| D70 | the order field values run in | as written: `a := b + 1; b := 5;` (globals) compiles and stops with `nullptr_exception` when the program starts - checked | each value after the values it reads directly (a topological order, the written order otherwise); a circle keeps the written order with `W1031` | wider: the C++ order has no meaning when it reads a value not set yet |
| D71 | overloads that differ only in the function type an untyped lambda argument would get | the first in candidate order (an untyped lambda is a "direct match") | the same - not the first plan's `E0503` | same |
| D72 | a lambda's result neither written nor expected (`f := { x: int -> return x.value * 2; };`); a one-expression body after an arrow (`{ x -> x * 2 }`); calling a lambda where it is written (`{ -> (int) return 7; }()`) | "not a fully qualified lambda expression"; "expected `;`" - checked | worked out from the body (D11); accepted | wider |
| D73 | two values of one enum with the same number | accepted silently - checked | accepted with `W1030` (they compare equal) | same language, plus a warning |
| D74 | `lock(a_function)` | accepted (a function value is an object) - checked by stage 8 | accepted (the stage 8 plan made it `E0615`) | same |
| D75 | a `get` / `inject` that names no component | the definition of the LAST component declared, or "No defined type found" when that one has none (the loop of `get_type_definition` overwrites its result) - checked | the first definition found, in the order the components are declared; `W1013` when another answers too | wider: programs the C++ compiler refuses by accident |
| D76 | what a definition `single { 8080 }` / `{ "text" }` provides | not established (the C++ search fails before the type matters, D75) | as `x := value;`: an `int`, a `string` | - |
| D77 | a lambda as a definition whose types are not all written | "lambda must be fully qualified to be injected" | typed like any lambda with nothing expected: `E0800` for a parameter without a type; a result that is not written is worked out (D72) | wider, as D72 |
| D78 | `[name]` in `asm` inside a lambda, naming a local of the function around it | not probed | `E0924` | stricter: the variable is not in the lambda's frame |
| D79 | a field value that reads its own field (`z : _int64 = z + 1;`) | accepted silently | accepted with `W1031` | same language, plus a warning |
| D80 | a field listed in an `obfuscate` block (`obfuscate -keep { point.x }`, `{ total }`) | the compiler crashes (segmentation fault) - checked | resolved like any name | wider |
| D64 | unreachable code | warning S607 "unreachable statement" only right after a `return` / `throw` / `break` / `continue` in the same block | `W1002` also after a loop that never ends, and after an `if` / `when` / `try` whose every way ends in a jump - except the `return` the C++ compiler demands after an endless loop (7.13) | same language, more warnings |
| D81 | a global `main` that is not one of the four forms | "main method might not be executed" only when the parameters fit and the result does not (`def main() : _int64`); `def main(n: int)` is silently not the entry point - checked | `W1015` for both | same language, one more warning |
| D82 | a non-nullable object field nothing gives a value | `initialization_validator.cpp` would say "field `x` must be initialized", but it never fires: a field no constructor sets, a global without a value, a static field read before it is set - all compile (checked), the last two stop with `nullptr_exception` | the warning `W1032` (off unless -wextra: it fires 37 times in the library), with the same rule: the first use in every constructor that can run, or an `init` block, is an assignment | same language, plus a warning |
| D83 | `base->f()` where `f` has no body in the class above | compiles and calls `f` VIRTUALLY - which runs the very function that made the call: `stack_overflow_exception` (checked) | `E0510` | stricter: the C++ result has no meaning |
| D84 | `new c()` where `c` still has a function without a body (declared in `c` or above it, defined by nothing on the way down) | compiles and runs until the function is called (checked) | the warning `W1033` | same language, plus a warning |
| D85 | warnings the C++ compiler does not have | - | `W1034` unused local, `W1035` unused private member (both off unless -wextra), `W1036` a local named like a field of its class that the code could have meant | more warnings |
| D86 | the warning switches | applied where each warning is made (`create_new_warning`) | applied once to the finished list of diagnostics, just before printing (`backend/compiler/program/warning_options.sharp`): every stage reports everything; -analysis-stats still counts what was switched off | design |
| D87 | `def main() := start();` - a `main` whose result is worked out from its expression | looked at before the result is known: "main method might not be executed", and then no `main` is found (checked: `def main() := println("x");` does not build) | the entry point when the result turns out to be nothing or an `int` | wider: the function is what it says |
| D88 | calling a `static` function of a class that has no body (`class t { static def f(); }` ... `t.f();`) | compiles; the VM stops with "attempting to call non-native static delegate function" (checked), also when a class below "overrides" it | `E0510` | stricter: the C++ result has no meaning |
| D89 | the modules and classes the VM needs (`std#throwable`, `std.io#thread` ...; `validate_required_dependencies`) | one fatal line per missing name, without a place, when the executable is written; a class the program does not USE counts as missing; a name an `obfuscate` block renamed counts as missing | one `E0962` at `main` listing everything missing, in the whole-program checks; the classes are made roots of the reachability walk instead of being required to be used; the obfuscation half is left to code generation | design; same programs accepted |
| D90 | two parameters with one name (`def f(a: _int64, a: string)`) | accepted; the body's `a` is the last one (checked) | `E0600` at the second | stricter: the first parameter could never be read |
| D91 | `==` / `!=` between two arrays | number arrays of different types and `_int64[] == object` refused; EVERY two class arrays refused ("mismatched array types", even `k[] == k[]`) - checked | refused only when no array can be both (`E0511`): different number types, a number and a class, unrelated classes. `k[] == dog[]` with `dog` below `k`, and an array against `object`, are accepted | same language where C++ has a meaning; wider for related class arrays |
| D92 | `local` / `private` / `protected` on a function declared in a module, called from another file or module | not checked (checked) | `W1022`, as for the functions of a class | same language, plus a warning |
| D93 | `o as T[]` in a generic instance where `T` is a function type (`list<(decimal)(decimal)>.to_array()`) | refused when that function is compiled ("cast expression of type `*(decimal)(decimal)[]` not allowed"); written directly (`o as fn[]`) refused too | written directly: `E0412` as in C++. In an instance body, where the cast names a type parameter: accepted, tested when the program runs - every body of every instance is analysed (decision 2), so refusing it would make every list of lambdas an error, used or not | wider in instances |
| D94 | a mistake in a declaration (stages 1-5) | reports everything it finds, bodies included | the members and body stages, flow and the unused-generics pass run as well; everything they report is kept except a repeat (same code, same place); a class declared twice or with a base that was not found is `members_uncertain` and gets no `E0402` / `E0341`; components, initializers and program checks wait for a sound program | design: one compile shows the mistakes of the bodies too |
| D95 | a lambda item without types in `new t[] { ... }` | "lambda must be fully qualified" | the item type gives it its types (the same rule as any place that expects a function type, D11) | wider |
| D96 | what a text definition of a component provides (`single("greeting") { "hello" }`) | an `_int8[]` (checked: `get<string>("greeting")` is "Type mismatch on type definition ... `_int8[]`"; `get<_int8[]>` crashes the C++ compiler) | a `string`, as `x := "hello";` makes a string (7.20) | design: what the text is everywhere else |
| D97 | a decimal out of the 64-bit range stored in a whole-number type - a constant (`n : _int64 = 1e30;`, a decimal integer literal too big for 64 bits, `(1.0e30 as _int64)`, `2 ** 63`); a decimal literal past 1.8e308 | silent. A CONSTANT is folded with a C++ cast, which gives the smallest `_int64` (-9223372036854775808) for every number out of range, positive ones too; the same conversion while the program runs (`d2i`, `decimal_to_integer` in stdimports.h) gives the nearest end (9223372036854775807 for a positive one), so its program disagrees with itself (`pw(2, 63)` gives ...807, `2 ** 63` ...808); `1e400` is infinity | the constant is folded as the VM converts it while the program runs - the nearest end - so a constant and the same value computed agree; with `W1037` (which names the value stored) / `W1038`. (Corrected in the completeness stage: this row said "the same values" - they differ for positive numbers, codegen.md G42) | same language for values in range; the C++ fold is an accident (undefined behaviour of a C++ cast) |
| D98 | `f := twice<int>;` (a generic function named with type arguments, as a value) | "could not resolve symbol twice" | the parser reads the name with its type arguments when `;`, `)` or `,` follows the `>` (no comparison ends that way), and the backend says `E0805` with the lambda to write | same programs accepted, a better message |
| D99 | the body of a generic class or function nothing instantiates | never looked at: any mistake in it goes unseen until the first use | analysed once as a stand-in instance with unknown type arguments (4.1); a mistake that does not depend on the type arguments is the warning `W1039` | same programs accepted; the mistake is shown before the first user meets it |
| D100 | `maybe?.x ?: -1` with a number `x` | accepted; the program stops when `maybe` is null (the `?:` never runs) | `W1027` alone, saying that the `?:` never runs and giving `maybe != null ? maybe!!.x : -1`; no `W1006` on top | one mistake, one message |
| D101 | `a := get("main", runtime);` (a `get` with name and component, no type, as the value of a `:=` declaration) | refused: "cannot resolve type from get() expression" (checked) | accepted: `a` gets the type the definition provides. `W1014`'s help for `a := get<settings>("main", runtime)` therefore gives the form both compilers accept, `a : settings = get("main", runtime);` | wider |
| D102 | `private alias ...` at module level used from another module; `public alias ...` in a class used from outside | the words are dropped by the parser: the first is accepted, the second refused (checked) | the same: who may name an alias follows from where it is declared (preprocessor `alias_flags`); the written word is `W1040` | same |
| D103 | very deep nesting: 150 parentheses one inside the other, 200 nested `if` blocks; long chains: a 400-term `a + a + ...`, a 300-call builder chain | compiles all of them | chains of any length - `a + b + ...`, `a \|\| b \|\| ...`, `x.f().g()...`, `a ? 1 : b ? 2 : ...`, `if .. else if .. else if ..` - because the analysis and every pass after it follow them in a loop (code generation too, since the verifier's pass of its stage 3: codegen.md 13.3). NESTING deeper than the VM's stack can follow is `E0103` (parser), `E0992` (a later pass of analysis) or `E0978` (code generation), at the place, instead of the crash it was; about 120 parentheses, 130 `if` blocks, 100 calls inside each other's arguments and 120 `a = b = c ...` or `a ?: b ?: ...` are fine (the C++ compiler takes 300 of each) | stricter (a limit) |
| D104 | `class job_failed : exception` (a class listed after `:`) | "is not an interface", and the class has no base, so its constructor's `base(message)`, its `throw` and `catch` give more errors (3 for two such classes, checked) | `E0324` alone: the rest of the program is checked as if `base exception` were written (error recovery by repair), when the class writes no `base` and taking it makes no circle | stricter in nothing; fewer follow-ons |
| D105 | `mutate i { def f() { ... } }` on an INTERFACE: a function with a body added to it | accepted; `f` is called through any `i` (checked by running); `new c().f()` on a class `c : i` is "could not resolve" | accepted the same way (it is then like an extension function of `i`, not asked of the classes that implement it); `new c().f()` is accepted too, since a `c` is an `i` (10.10) | wider |
| D106 | the order the messages are printed in | per stage, in the order each stage met them | in the order of the program text: file by file, and by place inside a file (10.10) | same messages |
| D107 | `[name]` in `asm` naming a local of the SAME function that a lambda captures, or a `localized` local | accepted: a captured variable is kept in the frame as well as in the closure object, so `[name]` names the frame's copy (which a lambda's writes never reach); a captured NUMBER it refuses anyway (D10) | `E0925` | stricter: here such a variable is not in the frame at all (codegen.md G11, 6.17), so there is no slot to name. Added by the verifier of code generation's stage 4, in place of an internal error |

---

## 6. The object model

### 6.0 Where everything lives

What EXISTS is in the "Map of the source" at the top of this document: one
package per folder, one front-door file per package. This section lists what
the stages still to come are expected to ADD, in the layout the map shows
(the first draft of this plan put everything in a few flat folders; the
owner asked for packages, and stage 6 moved the existing files - only
locations, `mod` lines and imports changed).

```
backend/
  types/typed_tree/
    bound_calls.sharp             + call of a function value, new, function reference, lambda        (stages 7, 9)
    bound_operators.sharp         + binary, logical, unary, increment, assign                        (stage 7)
    bound_collections.sharp       + dictionary                                                       (stage 7)
  compiler/
    conversions/
      class_conversion.sharp      + the tie between several fitting constructors; operator=          (stage 7)
    names/
      overload_resolution.sharp   call_candidate, ranking, the winner                                (stage 7)
      overload_messages.sharp     "no overload fits": why each candidate does not                    (stage 7)
    expressions/
      calls/                      call_expression, new_expression, value calls, generic function calls (stage 7)
      operators/                  + binary (arithmetic, comparison, equality, logical, bitwise),
                                  unary, increment, assign, operator overloads                       (stage 7)
      collections/                + dictionary_expression; a[i] on an object                         (stage 7)
      literals/string_template    + the `string.operator+` chosen for each piece                     (stage 7)
      constant_folding.sharp      2 + 3 while compiling                                              (stage 7)
    lambdas/                      (built: see the Map of the source; closures are context/closures.sharp)  (stage 9)
    initializers/                 (built) implied base calls, added constructors, enum values, the order a
                                  class is initialised in                                            (stage 9)
    checks/                       stage 10: field initialization, main, completeness
    components/                   stage 9 (built by its review): components, inject, get
    obfuscation/                  stage 9 (built by its review): obfuscate blocks
  reports/
    analysis_printer.sharp        grows with the tree (it is the `-bound` printer of the first draft)
```

### 6.1 Types during analysis: `resolved_type`

**Decision: a new object, not an extension of `sharp_type`.** A `sharp_type`
is a PLACE where a type is written: it has a file and a node, it may be an
alias or a type parameter that has to be looked through, and one written `T`
means different types in different instances. Analysis needs something else:
the type a VALUE has, wherever the value came from - `a + b` has a type though
nobody wrote one - that can be compared quickly and exists once per meaning.
Mixing the two (as the C++ `sharp_type` does, with its "unresolved", "field",
"literal" and "function" states) is the main reason that compiler is hard to
follow. So:

```
written type  (sharp_type)   --resolve_written_type(written, environment)-->   resolved type (resolved_type)
   `T?` in class box<T>                 environment: T is string                  class std#string, nullable
```

`backend/types/type_system/resolved_type.sharp`:

| field | meaning |
|---|---|
| `kind : _int64` | `RT_ERROR` already reported, fits everywhere - `RT_NIL` "no value" (a function that returns nothing) - `RT_NULL` the type of the word `null` - `RT_INTEGER` - `RT_DECIMAL` - `RT_OBJECT` the built-in `object` - `RT_CLASS` an object of a class, interface or enum - `RT_FUNCTION` a function value - `RT_TYPE_PARAMETER` a `T` nothing is known about (only in blueprint signatures; never in a body) |
| `bits : _int64`, `is_signed : _int64` | for `RT_INTEGER`: 8, 16, 32, 64; `_uint8` is 8 / 0 |
| `class_of : sharp_class?` | for `RT_CLASS`: the class - for a generic, the INSTANCE (`list<string>` is one class object, so there is no argument list here) |
| `parameters : list<resolved_type>`, `result : resolved_type` | for `RT_FUNCTION`; `result` is the nil type for "returns nothing" |
| `parameter : sharp_type_parameter?` | for `RT_TYPE_PARAMETER` |
| `is_array`, `is_nullable`, `has_nullable_items` | `t[]`, `t?`, `t[]??` |

Functions (all in `resolved_type.sharp`): `same_resolved_type(a, b)` (kind,
bits/sign, the same class OBJECT, the same function shape, the same three
flags), `to_text()` for messages (`list<string>?`), `key()` for instance keys
(4.1), `is_number()` (integer or decimal, not an array), `is_reference()`
(everything a `null` can stand for: class, object, function, any array),
`can_be_null()` (written `?`, or the type `object`, which takes null without
the mark - 7.5.1), `element_type()` (an array's item: `is_array` off, nullable
when `has_nullable_items`), `as_nullable()`, `as_non_null()`, `array_of()`.

A `resolved_type` is never changed after it is made (`as_nullable()` and the
others return another object). That is what allows them to be shared.

The native types, the error type, the nil type and the null type exist ONCE
each (`types.int64`, `types.decimal`, `types.error` ... in a `type_table`
hanging off `sharp_program`), so most comparisons are one pointer test. Class
types are small objects made on demand.

**`type_environment`** (`type_environment.sharp`) - what the type parameters
in scope stand for:

| field | meaning |
|---|---|
| `parameters : list<sharp_type_parameter>` | of ONE generic class or function |
| `arguments : list<resolved_type>` | what each stands for |
| `outer : type_environment?` | the environment around it: of the class a nested class is in, of the class a generic function is in |

`lookup(parameter)` walks the chain. An ordinary class has `environment = null`.

**`resolve_written_type(prog, written, environment, problems) : resolved_type`**
(`resolve_written_type.sharp`) is the only way from one to the other:

1. `declaration == null` on a named type (stage 3 reported it): the error type.
2. look through aliases; a type parameter becomes what the environment says
   (none says anything: `RT_TYPE_PARAMETER`);
3. a generic class with arguments: resolve the arguments, `instantiate` (4.1);
4. apply `[]`, `?`, item nullability, and check them (`E0355` to `E0357`);
5. a function type: resolve parameters and result.

Each `sharp_type` caches its result when `environment` is null (the common
case); under an environment the caller caches (the member object of the
instance holds it).

**Types written inside a body** (`backend/compiler/context/body_types.sharp`, stage 6).
`new list<T>()`, `x as shape`, `n : box<string>;`, `e is circle`, a lambda
parameter's type: these are `N_TYPE` nodes in a body, and stage 1 made no
`sharp_type` for them. `written_type_at(context, node) : sharp_type` makes
one - with the lookup rules of stage 3 (`type_names.sharp`), seeing also the
type parameters of the function - and keeps it in `sharp_file.body_types`, an
array with one slot per tree node of the file, so a node is looked up once
however many instances of a generic class share it. The array lies BESIDE the
tree; the tree is not touched. The caller then resolves it under ITS
environment. One kind of name is not in that table: a LOCAL alias
(`alias t as name;` in a block) is looked for first, in the scope chain, and
answers with a resolved type directly (it can differ per instance).

Resolved types on declarations, filled by stage 5 (`member_types.sharp`):
`sharp_field.value_type`, `sharp_parameter.value_type`,
`sharp_function.result_type` (the nil type for "returns nothing"),
`sharp_alias.target_type`. For `x := value` and `def f() := value` they stay
null until the value is analysed (section 6.6).

### 6.2 The error type and "one mistake, one message"

`types.error` (`RT_ERROR`) is given to any expression that could not be
analysed and HAS BEEN REPORTED. The rules every analysis function follows:

- an operand with the error type makes the result the error type, silently;
- the error type converts to and from every type (no "cannot convert");
- a member looked up on the error type is a `bound_error`, silently;
- a call with an error-typed argument that fits no overload is not reported;
- a condition, an index, a `throw` operand of the error type is accepted;
- a local declared `x := <error>` gets the error type and is declared anyway,
  so later uses of `x` are not "unknown name";
- a local whose WRITTEN type refused its value (`ages : _int64 = { ... };`,
  E0410) is READ as the error type afterwards (`local_variable.value_refused`,
  set by `written_type_refused` in conversions/convert.sharp, applied
  where a name becomes a `bound_local`, primary/found_names.sharp; a FIELD
  declared that way is read as the error type too, `field_value_refused` in
  driver/lazy_types.sharp, 10.11): whether the
  type or the value is the mistake is not known, so no use is judged by it. A
  local whose value was itself broken (`n : _int64 = nothing_here;`) keeps its
  written type.

`bound_error` is the tree node; `bound_pending` is its sibling for "a later
stage builds this" (counted, never reported).

### 6.3 Constants

`backend/types/type_system/constant_value.sharp` - a value the compiler knows:

| field | meaning |
|---|---|
| `kind` | `CONST_INTEGER`, `CONST_DECIMAL`, `CONST_TEXT` |
| `integer : _int64`, `number : decimal`, `text : string` | the value |

`bound_expression.constant` is set for literals and for operations on
constants (`constant_folding.sharp`): `+ - * / % ** & | ^ << >>`, the
comparisons, `&& || !`, unary minus, on integer and decimal constants, with
the SAME result the VM gives (64-bit wrap-around integers, IEEE doubles, `/`
always decimal). Used for: enum values, `x % 0` (`E0427`), a `when` clause
written twice, `while(true)`, the value of a `const` field read from another
file, `is` tests known while compiling. `true` / `false` are the integer
constants 1 / 0; a character is its code.

Literals keep a mark of what they were written as (`bound_expression.literal`:
`LIT_NONE`, `LIT_INTEGER`, `LIT_DECIMAL`, `LIT_CHAR`, `LIT_BOOL`, `LIT_TEXT`),
which survives folding of `+ - *` on two whole-number literals (`2 + 3` is
still `LIT_INTEGER`; that is all the C++ compiler folds into a literal, see
`compile_binary_integer_expression`). A literal has an ordinary type like
every expression (7.2: `5` is an `_int32`); the mark matters in exactly two
places: `x := 5` makes `x` an `int` OBJECT, not an `_int32` (7.3), and a text
literal becomes a `string` where an object is needed (7.2).

### 6.4 The typed tree

`backend/types/typed_tree/` (the first draft said `backend/compiler/bound`; the declarations own their trees, and `types` may not import the compiler - 10.2 lists the other differences between this table and the tree as built). Every class has a `kind` so code can branch on it
and cast (`node as bound_call`) - the same pattern as `sharp_declaration`.
Section 4.2 gives the test every node has to pass.

**Base classes** (`bound_node.sharp`)

| class | fields |
|---|---|
| `bound_node` | `kind : _int64` (`BX_...` / `BS_...`), `unit : source_unit`, `node : _int64` (the syntax node it was built from; for a node the compiler added - an implicit `self`, a conversion - the node of the expression it belongs to) |
| `bound_expression base bound_node` | `type : resolved_type`; `constant : constant_value?`; `literal : _int64`; `storage : _int64` - what kind of lvalue it is: `STORE_NONE` (a value only), `STORE_LOCAL`, `STORE_FIELD`, `STORE_ELEMENT` (an item of a real array). Only these three can be assigned to; a property and `o[i]` on an object are not storage, they are calls (below) |
| `bound_statement base bound_node` | `completes : _int64` - can execution continue after it (section 7.13) |

**Expressions**

| class (`BX_` kind) | file | fields beyond the base | built by |
|---|---|---|---|
| `bound_error` (`BX_ERROR`) | bound_node | - | 6 |
| `bound_pending` (`BX_PENDING`) | bound_node | `what : _int64` (the kind of syntax node it stands for), `parts`. Since stage 9: only the placeholder of an ARGUMENT that waits for its parameter (an untyped lambda, a function name, a `get()` in a component definition) until the call is bound; it stays only in a call that failed and was reported | 6 |
| `bound_literal` (`BX_LITERAL`) | bound_values | (value in `constant`) | 6 |
| `bound_null` (`BX_NULL`) | bound_values | - | 6 |
| `bound_local` (`BX_LOCAL`) | bound_values | `variable : local_variable`; `depth : _int64` (function boundaries between this use and the variable's function, 4.3; 0 for an ordinary use) | 6, 9 |
| `bound_field` (`BX_FIELD`) | bound_values | `object : bound_expression?` (null: static or global; an implicit `self` is made explicit as a `bound_self`); `field : sharp_field`. Always the STORED value: a read through a getter is a `bound_call` | 6 |
| `bound_enum_value` (`BX_ENUM_VALUE`) | bound_values | `value : sharp_enum_value` | 6 |
| `bound_self` (`BX_SELF`) | bound_values | `depth : _int64` (0 in the function itself; in a lambda, how many records out the object is, 4.3) | 6, 9 |
| `bound_base` (`BX_BASE`) | bound_values | `class_of : sharp_class` (the base class named, `base@type`); `depth` (as for `bound_self`; added by stage 9); only legal in front of `->` / `.` | 6, 9 |
| `bound_type_name` (`BX_TYPE_NAME`) | bound_values | `named : resolved_type` - a class written where a value is expected; only legal in front of `.` or `(` (anything else is `E0404`), so it never reaches a code generator | 6 |
| `bound_call` (`BX_CALL`) | bound_calls | `function : sharp_function`; `object : bound_expression?` (null for a static / global call); `arguments : list<bound_expression>` (each already converted to its parameter); `dispatch : _int64` (below); `written_as : _int64` - `AS_CALL`, `AS_OPERATOR` (`a + b`), `AS_INDEX` (`o[i]`), `AS_INDEX_STORE` (`o[i] = v`, a call of `put`), `AS_GETTER` (`o.size`), `AS_SETTER` (`o.size = 3`), `AS_ASSIGN_OPERATOR` (`o = v` / `o += v` through `operator=` / `operator+=`), `AS_ITERATE` (the `get_elements()` of a `foreach`); for messages and the printer only | 6 (getter), 7 |
| `bound_value_call` (`BX_VALUE_CALL`) | bound_calls | `callee : bound_expression` (a function value); `arguments` | 7 |
| `bound_new` (`BX_NEW`) | bound_calls | `class_of : sharp_class`; `constructor : sharp_function`; `arguments`; `without_new : _int64` (`point(1, 2)`) | 7 |
| `bound_function_reference` (`BX_FUNCTION_REFERENCE`) | bound_function_values | `function : sharp_function` (a global or static function; the overload the expected type chose); its type is the function's type | 9 |
| `bound_lambda` (`BX_LAMBDA`) | bound_function_values | `function : sharp_function` (the lambda: `is_lambda`, its parameters, result, `bound_body`, `locals`, `closure`); `record : closure_record?` (the record it is handed: that of the function it is written in, or of the field whose value it is; null when it captures nothing) | 9 |
| `bound_binary` (`BX_BINARY`) | bound_operators | `operator : _int64` (token kind); `left`, `right`; `operands : _int64` - `OPERANDS_INTEGER`, `OPERANDS_DECIMAL` (how it is CALCULATED, 7.8.1), `OPERANDS_REFERENCE` (same object?), `OPERANDS_NULL_TEST`. Only numbers and references: an operator on an object is a `bound_call` | 7 |
| `bound_logical` (`BX_LOGICAL`) | bound_operators | `is_or`; `left`, `right` (the right side runs only when needed) | 7 |
| `bound_unary` (`BX_UNARY`) | bound_operators | `operator`; `operand` (a number) | 7 |
| `bound_increment` (`BX_INCREMENT`) | bound_operators | `target : bound_expression` (storage of a number; its object / index parts are evaluated once); `is_decrement`; `is_postfix` (the value is the old one); `getter`, `setter : sharp_function?` - set when the target is a property: read through `getter`, write through `setter`, either may be null (then the slot is used directly) | 7 |
| `bound_assign` (`BX_ASSIGN`) | bound_operators | `target` (storage); `value` (converted to the target's type); `operator : _int64` (0 for `=`; else the arithmetic operator of `+=` on NUMBERS, with `operands` as in `bound_binary` and `store : conversion`, the conversion of the result back to the target's type); `getter`, `setter : sharp_function?` as in `bound_increment`, for a compound assignment to a property. A plain `=` to a property with a setter, `o[i] = v`, and an assignment through `operator=` / `operator+=` are `bound_call`s, not this node | 7 |
| `bound_conversion` (`BX_CONVERSION`) | bound_operators | `operand`; `conversion : conversion` (6.5) - every implicit conversion is an explicit node in the tree | 6, 7 |
| `bound_cast` (`BX_CAST`) | bound_operators | `operand`; `conversion : conversion` (number to number, upcast, or `CONV_DOWNCAST`); `checked : _int64` (needs a run-time test) | 6 |
| `bound_is` (`BX_IS`) | bound_operators | `operand`; `tested : resolved_type` (`constant` set when known while compiling) | 6 |
| `bound_not_null` (`BX_NOT_NULL`) | bound_operators | `operand` | 6 |
| `bound_safe_navigation` (`BX_SAFE_NAVIGATION`) | bound_operators | `receiver : bound_expression` (the `x` of `x?.a.b()`; evaluated once and tested); `rest : bound_expression` (the WHOLE chain after it, `<x>.a.b()`, in which the receiver appears as a `bound_receiver`). Its value is null when the receiver is null, else the value of `rest`; its type is `rest`'s type made nullable | 6 |
| `bound_receiver` (`BX_RECEIVER`) | bound_operators | `of : bound_safe_navigation` - "the receiver of that node, already evaluated and known not to be null" | 6 |
| `bound_elvis` (`BX_ELVIS`) | bound_operators | `value`; `fallback` (converted to the result type) | 6 |
| `bound_ternary` (`BX_TERNARY`) | bound_operators | `condition`; `when_true`; `when_false` (both converted to the result type) | 6 |
| `bound_sizeof` (`BX_SIZEOF`) | bound_operators | `operand` | 6 |
| `bound_index` (`BX_INDEX`) | bound_collections | `array`; `index` (an integer) - an element of a real array. (`a[i]` on an object is a `bound_call` of `operator[]`.) | 6 |
| `bound_new_array` (`BX_NEW_ARRAY`) | bound_collections | `element : resolved_type`; `size : bound_expression?`; `items : list<bound_expression>?` (`new t[] { a, b }`, each converted) | 6 |
| `bound_array_literal` (`BX_ARRAY_LITERAL`) | bound_collections | `element`; `items` (each converted) | 6 |
| `bound_dictionary` (`BX_DICTIONARY`) | bound_collections | `pair_class`, `dictionary_class : sharp_class`; `pair_constructor`, `dictionary_constructor : sharp_function`; `keys`, `values : list<bound_expression>` (converted) | 7 |
| `bound_string_template` (`BX_STRING_TEMPLATE`) | bound_collections | `start : bound_expression` (the first piece of text as a `string` object); `appends : list<bound_call>` - one call of a `string.operator+` per further piece, in order, each with its `object` left null: it is the result of the one before | 7 |
| `bound_get_component` (`BX_GET_COMPONENT`) | bound_calls | `definition : type_definition` | 9 |

`dispatch` says which function body runs, and is settled when the call is
bound, from the declarations alone (7.7.3): `CALL_STATIC` (no object),
`CALL_DIRECT` (this very function), `CALL_VIRTUAL` (the function belongs to an
override family - it has no body, or it is an `override` - so the object's
class decides at run time), `CALL_BASE` (`base->f()`, never looked up),
`CALL_CONSTRUCTOR` (a base constructor call, `-> base(...)`).

**Statements** (the package `backend/types/typed_tree/statements`, module
`backend.types.typed_tree.statements`, all built by stage 8; the base
`bound_statement` stays in `bound_node.sharp` because `sharp_function.bound_body`
is one). The `BS_` kinds are in the package's front door, `statements.sharp`.

| class (`BS_` kind) | file | fields |
|---|---|---|
| `bound_block` | bound_simple_statements | `statements : list<bound_statement>`. (The plan gave it its `local_scope`; scopes belong to the compiler, which `types` may not import, and a code generator needs only each variable's slot, `local_variable.index`) |
| `bound_local_declaration` | bound_simple_statements | `variable : local_variable`; `value : bound_expression?` (converted; null: none); `default_constructor : sharp_function?` (`n : int;` starts as an `int` made by `int()`, 7.3); `is_injected` (`inject x : t;`, resolved by stage 10). `a : int = 1, b = 2;` is two of these in a row |
| `bound_expression_statement` | bound_simple_statements | `expression`; `is_result : _int64` (the value of a lambda written without `return`, 7.14; set by stage 9) |
| `bound_empty` | bound_simple_statements | - (`;`, a local alias, a statement the parser gave up on) |
| `bound_if` | bound_branches | `condition` (a plain number); `then_part`; `else_part : bound_statement?` |
| `bound_when` | bound_branches | `subject : bound_expression?`; `subject_variable : local_variable?` (a hidden local holding the subject: evaluated once); `clauses : list<bound_when_clause>`; `else_part : bound_statement?` |
| `bound_when_clause` (`base bound_node`, not a statement) | bound_branches | `tests : list<bound_expression>` (each a finished condition, a plain number: "`subject_variable` == value" bound like the operator `==`, or the written condition when there is no subject); `body` |
| `bound_loop` (the base of the four loops) | bound_loops | `body`; `is_endless` (no condition or a literal true); `left_by_break`, `continued` (a reachable `break` / `continue` acts on it: set by the flow analysis) |
| `bound_while` | bound_loops | `condition` |
| `bound_do_while` | bound_loops | `condition : bound_expression?` (analysed after the body) |
| `bound_for` | bound_loops | `initializers : list<bound_statement>` (local declarations, or an expression statement); `condition : bound_expression?`; `step : bound_expression?` |
| `bound_foreach` | bound_loops | `variable : local_variable`; `collection` (ALWAYS an array: for a `loopable` object, the `bound_call` of its `get_elements()`, `AS_ITERATE`); `item_type : resolved_type`; `item_conversion : conversion` (item to the variable's type; identity when no type is written; a checked `CONV_DOWNCAST` for an `object` item given to a class variable); `array_variable`, `index_variable` (hidden locals: the array evaluated once, the position) |
| `bound_return` | bound_jumps | `value : bound_expression?` (converted to the function's result; a value of type nil in a function that returns nothing is a call made for its effect); `leaves` (below); `broken` (below) |
| `bound_break`, `bound_continue` | bound_jumps | `loop : bound_loop?` (the loop it leaves / repeats; null only after `E0602` / `E0603`); `leaves : list<bound_statement>` - the `bound_lock` and the `bound_try` with a finally it passes on the way out, innermost first; `broken` |
| `bound_throw` | bound_jumps | `value` |
| `bound_try` | bound_handlers | `body`; `catches : list<bound_catch>`; `finally_part : bound_statement?` |
| `bound_catch` (`base bound_node`, not a statement) | bound_handlers | `caught : sharp_class?` (null only after `E0613`); `variable : local_variable?` (null for `catch(some_exception)`); `body` |
| `bound_lock` | bound_handlers | `locked` (the object); `held : local_variable` (hidden: the object locked, so the same one is unlocked); `body` |
| `bound_asm` | bound_assembly | `instructions : list<bound_asm_instruction>`; `labels : list<string>` (an `ASM_LABEL` operand is a position here) |
| `bound_asm_instruction` (`base bound_node`) | bound_assembly | `mnemonic`; `is_label` (`.name:`); `operands : list<bound_asm_operand>` |
| `bound_asm_operand` (plain class) | bound_assembly | `kind` - `ASM_REGISTER` (`register`), `ASM_NUMBER` (`number`), `ASM_TEXT` (`text`), `ASM_LABEL` (`label`), `ASM_LOCAL` (`variable`), `ASM_FIELD` (`field`), `ASM_FUNCTION` (`function`), `ASM_CLASS` (`class_of`) |

A `return` records `leaves` like `break` does, so the code generator knows
which `finally` blocks and locks to run. A jump that was reported as wrong
(`E0602` - `E0604`) is `broken = 1`: it has no meaning to generate, and the
flow analysis treats it as going on, so that the one mistake does not also
make the code after it "unreachable". A local alias (`alias t as name;`)
leaves nothing in the tree: it only adds a name to the scope.

The hidden locals (`local_variable.is_hidden`) are in the function's
`locals` like any other, so they have slots; no name in any scope reaches
them. What `try` / `finally` needs to run (the exception being passed on, the
address to come back to after a finally block - the C++ compiler's
`exception_object` and `return_address` locals) is left to the code
generator: it can be derived from the tree, and would only be a slot here.

### 6.5 Conversions

`backend/types/typed_tree/conversion.sharp` (the rules: `backend/compiler/conversions/`). A `conversion` says HOW a
value of one type becomes another; `convert(value, to, context, place)` in
`implicit_conversion.sharp` either wraps the expression in a `bound_conversion`
(or returns it unchanged for an identity) or reports why it cannot.

| field | meaning |
|---|---|
| `kind` | `CONV_NONE` (does not convert), `CONV_IDENTITY`, `CONV_NUMERIC` (number to number: widen, narrow, integer <-> decimal), `CONV_NULL` (`null` to a nullable type or `object`), `CONV_TO_NULLABLE` (`t` to `t?`), `CONV_UPCAST` (to a base class or interface), `CONV_TO_OBJECT`, `CONV_DOWNCAST` (to a class below, or from an interface to a class: implicit with `W1017`, or written `as`), `CONV_UNBOX` (an `int` object to its number: the field `value`), `CONV_CONSTRUCT` (through a one-argument constructor or `init(x)`), `CONV_LITERAL_TEXT` (a text literal to a `string` object: `CONV_CONSTRUCT` under the name messages use), `CONV_ERROR` (the error type: nothing to do) |
| `from`, `to : resolved_type` | the two types (the generator needs both for a number conversion) |
| `rank : _int64` | how good it is, for overload resolution (section 7.6): 0 identity ... 5 construct |
| `checked : _int64` | 1 when the VM has to test at run time (`CONV_DOWNCAST`) |
| `function : sharp_function?` | the constructor / `init(x)` for `CONV_CONSTRUCT` and `CONV_LITERAL_TEXT` |
| `field : sharp_field?` | the `value` field for `CONV_UNBOX` |
| `inner : conversion?` | the conversion applied to the value before constructing (`_int32` to a class built from a `decimal`) |
| `problem : _int64` | for `CONV_NONE`, why: `WHY_UNRELATED`, `WHY_NULLABLE` (only the `?` is in the way), `WHY_ARRAY` (array against non-array), `WHY_NO_VALUE` (nil), `WHY_NUMBER_TO_OBJECT` (a plain number where an object is wanted and no constructor takes it), `WHY_OBJECT_TO_NUMBER` (the reverse: help `.value`) |

`find_conversion(from, to, allow_user)` is the pure question (no reporting);
overload resolution asks it for every argument of every candidate. The
identity, error, to-nullable and to-object conversions for the native types
are shared objects (made once in the `type_table`), so the common cases
allocate nothing.

### 6.6 Local variables, scopes, the analysis context

**`local_variable`** (`backend/types/declarations/local_variable.sharp`)

| field | meaning |
|---|---|
| `name`, `type : resolved_type`, `unit`, `node` | |
| `function : sharp_function` | the function (or lambda) it belongs to |
| `index : _int64` | its position in `function.locals` |
| `is_parameter`, `is_const`, `is_localized` (one per fiber), `is_hidden` (made by the compiler: the `it` of a lambda) | |
| `is_captured : _int64` | a lambda inside uses it (4.3) |
| `reads`, `writes : _int64` | counted, for later warnings |

**`local_scope`** (`backend/compiler/context/local_scope.sharp`) - one per block,
`for`, `foreach`, `catch`, lambda and function body. The textbook scope chain:

| field | meaning |
|---|---|
| `outer : local_scope?` | the scope around it; null for the outermost scope of a function that is not a lambda (a lambda's outermost scope continues into the scope it is written in) |
| `function : sharp_function` | whose scope it is - lookup that crosses from one function to another has found a capture |
| `variables : hash_map<string, local_variable>` | declared HERE |
| `aliases : hash_map<string, resolved_type>` | local `alias t as name;` |
| (`statement`, `in_finally`) | planned for `break`, `continue` and `leaves`; built instead as the context's stack of jump frames (below), which a scope does not need to know about |

**`analysis_context`** - everything an analysis function needs to know about
where it is. One per body being analysed; created by the driver, passed to
every function (no global state):

| field | meaning |
|---|---|
| `prog : sharp_program`, `file : sharp_file` | (the file is the one the body is WRITTEN in - for an instance, the blueprint's) |
| `class_of : sharp_class?` | the class the body belongs to (an instance for generic code); null in a global function |
| `function : sharp_function?` | the function being analysed; null for a field value |
| `field : sharp_field?` | the field whose value is being analysed |
| `environment : type_environment?` | type parameters in force |
| `is_static : _int64` | no `self` here |
| `scope : local_scope?` | the innermost scope |
| `expected_result : resolved_type?` | what `return` must give: the result type, the nil type for "nothing", the field's type in a getter (set by `driver/function_body.sharp`). A function whose result is inferred (`def f() := value;`) has an expression body, never a `return` |
| `jumps : list<jump_frame>` | the loops, locks and try statements around the statement being analysed, innermost last (`context/jump_frames.sharp`: `FRAME_LOOP`, `FRAME_LOCK`, `FRAME_GUARDED` - the body or a catch of a try with a finally - and `FRAME_FINALLY`). `enter_frame` / `leave_frame`; a jump walks it from the top to find its loop and what it passes |
| (`returned`) | planned "every `return` seen"; not needed: no function body infers its result from `return`s (only lambdas would, stage 9) |
| `tree : syntax_tree`, `unit : source_unit` | shortcuts |
| `problems : diagnostics_sink` | where diagnostics go (section 9) |

Entering a lambda makes a NEW context for the lambda's function whose first
scope has the enclosing scope as `outer`. That one link is the whole capture
mechanism: lookup walks `outer`, and when `scope.function` changes on the way
it has left the lambda.

**Types on demand** (`compiler/driver/lazy_types.sharp`). `total := price * 2;` as a
field, and `def area() := w * h;`, have no written type; their type is the
type of the value, and another body may need it before the driver gets there.
`type_of_field(field)` / `result_of_function(function)` analyse the value at
that moment, in a fresh context for that declaration, and store the result
(`value_type` + `bound_initializer`, or `result_type` + `bound_body`), so it is
done once. A state on the declaration (`TYPE_UNKNOWN`, `TYPE_IN_PROGRESS`,
`TYPE_KNOWN`) catches circles - `a := b; b := a;` is `E0408` at both, and both
get the error type.

### 6.7 What a name was found to be: `lookup_result`

`backend/compiler/names/lookup_result.sharp`. Name lookup never builds tree nodes
itself; it answers with one of these and the expression code decides what to
do with it (a field becomes a `bound_field`, a class a `bound_type_name` ...).

| field | meaning |
|---|---|
| `kind` | `FOUND_NOTHING`, `FOUND_LOCAL`, `FOUND_FIELD`, `FOUND_ENUM_VALUE`, `FOUND_FUNCTIONS` (one name, possibly several overloads), `FOUND_TYPE`, `FOUND_MODULE` (the left of `#`) |
| `variable : local_variable?`, `captured_from : sharp_function?` | for a local; `captured_from` when it belongs to an enclosing function |
| `field : sharp_field?`, `value : sharp_enum_value?` | |
| `functions : list<sharp_function>` | every function of that name in the scope it was found in, own class before base classes |
| `type : resolved_type?`, `module : sharp_module?` | |
| `owner : sharp_class?` | the class the member was found in |
| `through_self : _int64` | found as a member of the class the code is in (needs `self`, or is static) |
| `hidden : list<sharp_declaration>` | things with that name that exist but cannot be used from here (private, another module not imported, an instance member seen from static code) - for the message |

### 6.8 Choosing an overload: `call_argument`, `call_candidate`, `overload_result`

`backend/compiler/overloads/call_candidate.sharp` (stage 7). `resolve_overload`
(`overloads.sharp`) takes the functions of ONE scope and the arguments and
gives back an `overload_result`; it reports nothing - the caller decides
whether to ask the next scope or to report.

| `call_argument` | meaning |
|---|---|
| `value : bound_expression`, `node` | the argument, analysed, and where it is written |
| `is_lambda : _int64` | 1: a lambda or a function name the lambda stage has not typed yet; it fits any function-typed parameter with the right number of parameters |
| `lambda_parameters : _int64` | how many parameters the lambda was written with; -1 for the `{ it }` form and for a function name |

| `call_candidate` | meaning |
|---|---|
| `function : sharp_function` | for a generic function: its INSTANCE for this call |
| `conversions : list<conversion>` | one per argument, when applicable |
| `applicable : _int64` | every argument converts and the count is right |
| `failed_argument : _int64`, `reason : string` | when not applicable: which argument (or -1 for the count or a type argument) and the sentence for the message ("argument 2 is `string`, but parameter `count` is a `decimal`") |
| `type_argument_code : string` | `E0515` .. `E0518` when the TYPE arguments are what is wrong (7.19), "" otherwise |
| `distance : _int64` | how many classes above the object's class the function is declared (step 7b of 7.6) |
| `waits_for_lambda : _int64` | a lambda argument was accepted by its parameter count only |

| `overload_result` | meaning |
|---|---|
| `best : call_candidate?` | the winner |
| `tied : list<call_candidate>` | the equally good best ones when there is no single winner (`E0503`) |
| `rejected : list<call_candidate>` | everything that did not fit, for the message (`E0501`) |
| `applicable_count : _int64` | how many candidates could take the arguments |
| `waits_for_lambda : _int64` | the candidates left want different function types from a lambda argument: the call stays pending until stage 9 |
| `type_argument_problem : string` | the first `type_argument_code` met, "" when none |

The algorithm is section 7.6.

### 6.9 Components: `sharp_component`, `type_definition`, `component_request`

`backend/types/declarations/sharp_component.sharp`. A `sharp_component` is
one `component name { }` block (a block without a name belongs to the MAIN
component, and there may be several such blocks); `sharp_program.components`
lists them in the order declared.

One `type_definition` per `single { }` / `factory("name") { }` (7.20):

| field | meaning |
|---|---|
| `owner : sharp_component` | the component block it is written in |
| `name : string?` | the name given in `single("name")`, or null |
| `is_factory : _int64` | 1: a new object for every `get`; 0: one object, made on first use |
| `unit`, `node`, `value_node` | where it is written (`N_COMPONENT_TYPE`), and its expression |
| `state` | `TYPE_UNKNOWN` / `TYPE_IN_PROGRESS` / `TYPE_KNOWN`: a definition is analysed the first time something asks for its type |
| `value : bound_expression?` | the typed expression that makes the object |
| `provides : resolved_type?` | the type of `value`: what a `get` / `inject` of that type receives. The error type when the expression was reported as wrong; the nil type when it has no value (`E0905`) |
| `needs : list<type_definition>` | the definitions its own `get`s were resolved to: the edges of the dependency graph the circle test (`E0908`) walks |
| `put_back : _int64` | 1 while the definition, analysed too early inside another one's analysis, waits to be analysed afresh (below) [completeness fixes] |
| `lambdas : list<sharp_function>` | the lambdas written in its expression, as its analysis made them; taken out of the program again when that analysis is dropped (below) [completeness fixes] |

A `component_request` is what one `get` or `inject` asks for - `type`, `name`,
`component_name`, each of which may be missing - with `inside` (the component
it is written in, when it is written in a definition) and where it stands.
It exists while the request is resolved; a request that has to wait for a
definition in progress is kept on `analysis_session.unsettled_requests` with
`waiting_on` (the chain of definitions in progress then), `asked_in` (the
definition whose value makes it) and `in_lambda` (1: written inside a lambda
of that value) until every definition is known.

ANALYSED TOO EARLY [completeness fixes]. A search looks at every definition
in reach, so analysing `single { new repository(get()) }` also analyses a
later `factory { new service(get()) }`, whose own `get()` then asks for the
repository still in progress. That used to be `E0908` ("a circle") for a
chain written in dependency order - config, repository, service - while the
reverse order compiled. Now a definition analysed INSIDE another one's
analysis whose request had to wait is put back: its result is dropped, its
waiting requests are taken back, it counts as "in progress" until the
outermost analysis ends, and it is analysed afresh when next asked for. Only
the outermost definition keeps a waiting request, and when every definition
is known that request is a circle only if its answer NEEDS the request's own
definition along `needs` (a depth-first walk of the dependency graph,
`path_of_needs` in components/requests.sharp); the message's related places
are that path. If the answer does not need it, the request only waited by
accident of the order, and its definition is analysed once more. The C++
compiler crashes (SIGSEGV) on every program of this shape.

Two things follow from analysing a definition twice, both found by the
verifier of that stage. (1) The lambdas of the dropped analysis leave the
program with it (`forget_lambdas_of`, components/definitions.sharp): each was
already a function in `sharp_program.lambdas` and in the list the flow pass
and the completeness check walk, with a `get` that had no answer yet, and
`factory { new printer({ -> ... get<reader>() ... }) }` at the end of a chain
ended in `E0991`. (2) A `get` INSIDE A LAMBDA of a definition is carried out
when the lambda is called, not while the object is made:

    single { new scheduler({ -> (string) w : worker = get<worker>(); return w.name; }) }
    single { new worker(get()) }            // worker(s: scheduler)

is no circle - the scheduler is made without a worker - and is the usual way
to break one. Such a request (`component_request.in_lambda`) adds nothing to
`needs` and is not tested for a circle; only a lambda that asks for its OWN
definition is still `E0908`, because the type of that definition is worked
out from the expression the lambda is part of (the help: ask through a
function that states the type).

What the code generator reads: a `bound_get_component` (`BX_GET_COMPONENT`,
`typed_tree/bound_calls.sharp`) points at the `definition` it was resolved
to, and so do an injected field (`sharp_field.injected_from`) and an injected
local (`bound_local_declaration.injected_from`): it never searches.

`backend/types/program/obfuscation_rule.sharp`: one name of an `obfuscate`
block, resolved - `mode` (`OBFUSCATE_PLAIN`, `_KEEP`, `_KEEP_INCLUSIVE`,
`_INCLUSIVE`), and either the `module` or the `declaration` (class, field,
enum value, function) it names. `sharp_program.obfuscation_rules` holds them
in the order written (7.25).

### 6.10 Fields this plan adds to the existing objects, in one place

| object | field | set by | meaning |
|---|---|---|---|
| `sharp_program` | `types : type_table` | 5 | the shared native / error / nil / null types and conversions |
| | `well_known` | 5 | the library classes and functions the compiler itself needs |
| | `generic_instances`, `instances_completed`, `pending_functions`, `generic_functions` | 5, 7 | every class made from a generic class, in order (a work list: 4.1); instances of generic functions; the generic functions as written |
| | `problems` | 5 | where work done on demand (an instance met late) reports |
| | `components` | 5 | every component block, in the order declared |
| | `obfuscation_rules` | 9 | every name of an `obfuscate` block, resolved (6.9) |
| | `main_function` | 10 | the entry point the whole-program checks found (also `analysed_program.main_function`) |
| `sharp_file` | `import_groups` | 5 | `import (...) as g` |
| | `body_types : sharp_type?[]` | 6 | written types of type nodes in bodies, by node (6.1) |
| `sharp_class` | `written`, `blueprint`, `type_arguments`, `instance_key`, `is_generic_instance`, `instances`, `instance_list`, `first_use`, `environment`, `members_resolved`, `bases_resolved`, `families_linked`, `plain_type`, `default_constructor`, `nesting_depth`, `failed_constraint` | 5 | 4.1, 3.5 |
| | `members_uncertain` | 1, 4 | 1 when a declaration error means its list of members cannot be trusted - declared twice, or a base class / interface that was not found or was refused; `E0402` and `E0341` are then not said about it, nor `E0400` inside it (10.8); also set on a stand-in whose base is a type parameter (4.1) |
| | `base_is_parameter`, `inherits_from_parameter` | 4 | `class mixin<T> base T` / `: T`: the base or an interface is a type parameter, known per instance (4.1) |
| | (`conversion_cache`) | - | planned for stage 7, NOT built: too few conversion questions for it to matter (7.6, "Cost") |
| | `instance_initializers`, `static_initializers`, `fiber_initializers` | 9 | 4.2, 7.15.2 (`fiber_initializers`: `localized` static fields with a value, run once per fiber) |
| `sharp_module` | `global_initializers`, `fiber_initializers` | 9 | the globals with a value or a default object, in reading order; the `localized` ones |
| `sharp_program` | `lambdas` | 9 | every lambda, as a function |
| `sharp_field` | `value_type : resolved_type?`, `type_state` | 5, 6 | its type; `TYPE_UNKNOWN` / `TYPE_IN_PROGRESS` / `TYPE_KNOWN` for inferred ones (6.6) |
| | `origin : sharp_field?` | 5 | on a member of an instance: the blueprint's member |
| | `bound_initializer` | 6 | 4.2 |
| | `default_constructor` | 9 | 4.2 (set by the ordering pass, `initializers.sharp`, `set_default_object`) |
| | `closure`, `lambdas` | 9 | the record a lambda in the field's value is handed when it uses `self` (4.3); the lambdas written in the value |
| | `injected_from` | 9 | the definition an `inject` field receives (6.9); such a field is in the initialisation lists like one with a value |
| `sharp_parameter` | `value_type` | 5 | |
| `sharp_function` | `result_type`, `type_state`, `origin`, `blueprint`, `type_arguments`, `instances`, `environment` | 5, 6, 7 | as for fields and classes |
| | `overridden_by : list<sharp_function>`, `family_root : sharp_function?` | 5 | 7.7.3 |
| | `is_synthesized`, `is_lambda`, `enclosing : sharp_function?`, `enclosing_field : sharp_field?`, `takes_record`, `lambdas` | 5, 9 | an added constructor; a lambda, the function (or field value) it is written in, whether it is handed a closure record (its hidden last parameter), the lambdas written in a function |
| | `bound_body`, `bound_base_call`, `locals`, `closure` | 8, 9 | 4.2, 4.3; `bound_base_call` of every constructor and `init(x)`: written (stage 7) or implied (stage 9) |
| `sharp_declaration` | `reached`, `used_anywhere`, `uses`, `usage_stamp` | 10 | reached from `main` and the start-up function; named by any analysed code; what its typed tree names (functions, fields, classes, enum values, constructors), each once - the def-use information of `backend/compiler/program/usage` |
| `sharp_declaration` | `kept`, `address` | code generation | set by code generation, not by analysis (codegen.md 5.5, 13.1): in the executable; its number there (a class's or function's address, a field's slot) |
| `local_variable` | `slot` | code generation | its slot in the frame of the generated function (codegen.md 4.4) |
| `local_variable` | `storage` | code generation | for a variable that does not live in the frame: its field in its function's closure record class (captured), or its fiber-local field (`localized`) (codegen.md 6.12, 6.17) |
| `closure_record` | `record_class`, `parent_field`, `self_field` | code generation | the class `closure_<function>` the record becomes, and its hidden fields `@parent` / `@self` (codegen.md 6.12) |
| `sharp_field` | `name_in_executable` | code generation | the name obfuscation gave it in the file; null when it keeps its own (codegen.md 6.17) |
| `type_definition` | `requested_by` | 10 | the declarations whose code receives the definition's value (a `get` / `inject` in them, or in a definition they receive): filled in by `program/usage` (`keep_uses`), so that code generation makes the definition's function only when one of them is kept |
| `type_definition` | `function`, `storage`, `made_flag` | code generation | `@sub_component_init_<n>()`, and for a `single` its static field and made-flag (codegen.md 6.16) |
| `sharp_module` | `reached` | 10 | something of the module is reached: its global values run at start |
| `sharp_class` | `promise_checked`, `unkept_promise` | 10 | a function without a body the class still has (W1033) |
| `sharp_type` | `last_environment`, `last_resolved` | 10 | a one-entry cache: the answer of `resolve_written_type` under the environment asked last (speed, 10.6) |
| `local_scope` | `variables : list`, `hashes : _int64[]` | 10 | was a `hash_map<string, ...>` per block (speed, 10.6) |
| `sharp_enum_value` | `value_type`, `has_number`, `number`, `origin` | 5 | its enum's type; the number a declaration fixes (`green = 5` and what follows it) |
| | `constant`, `bound_value` | 9 | 7.16 (`initializers/enum_values.sharp`) |
| `sharp_alias` | (no `target_type`: an alias inside a generic class means a different type per instance, so its target is resolved where it is used, under the user's environment; the written target caches the answer when it does not depend on a type parameter) | 5 | |

---

## 7. The rules of the language

Extracted from the C++ compiler (the file each rule comes from is named) and
checked with small programs where the code was not clear ("checked" in the
text means: a program was compiled, and where it says so run, with the C++
compiler). **[differs Dn]** / **[Dn]** marks an entry of section 5. Nothing
is left marked "to be confirmed": every detail the first draft left open was
probed and is stated as the rule. A stage that finds the C++ compiler doing
something else than this section says corrects the section and, if the
difference is on purpose, adds it to section 5. `S` is the type of the value,
`T` the type wanted.

### 7.1 Types

| type | values | notes |
|---|---|---|
| `_int8 _int16 _int32 _int64 _uint8 _uint16 _uint32 _uint64` | integers | one 64-bit cell in the VM; the type says how a value is cut down when stored |
| `decimal` (old name `var`) | a 64-bit IEEE double | |
| `object` | any object or array, or null | nothing can be done with it but pass it on, compare it, cast it, lock it. It is the one type that may hold null without being written `object?` (7.5.1) |
| a class, interface or enum | an object of that class (or a class below it) | |
| `(a, b)(r)`, `()()` | a function value | `()()` and `(a)(nil)` return nothing |
| `t[]` | an array | one dimension only: no `t[][]` (`E0357`; C++: "array arrays are not supported") |
| `t?` | `t` or `null` | not for a plain number: `_int64?` is `E0355` (C++: "cannot be nullable" - reported there only when the type is reached through a name, **[differs D28]**). `_int64[]?` is fine: the ARRAY may be null |
| `t[]??` | an array whose items may be null | only for arrays (`E0356`), not for arrays of numbers (C++ `ast_parser.cpp`: "cannot specify non-nullable integer array items as nullable", "nullable array items on non-array type") |
| `nil` | no value | only as a result type |

There is no `bool` type: truth values are numbers, 0 is false. `true` and
`false` are literals. `bool`, `int`, `long`, `char`, `byte`, `short`, `uint`,
`ulong`, `ushort`, `uchar`, `double` and `string` are CLASSES of the library
(`std#int` ...) - "boxes" that hold a number in a field `value` (7.5.6).

### 7.2 Literals (`expressions/primary/literal_expression.cpp`)

| literal | type | constant | mark |
|---|---|---|---|
| `12`, `0xff`, `1_000` | `_int32`, or `_int64` when it does not fit 32 bits | integer | `LIT_INTEGER` |
| `1.5`, `1e3` | `decimal` | decimal | `LIT_DECIMAL` |
| `'a'`, `'\n'` | `_int8` | its code | `LIT_CHAR` |
| `true`, `false` | `_int8` | 1, 0 | `LIT_BOOL` |
| `"text"` | `_int8[]` (an array of characters, NOT a `string` object) | text | `LIT_TEXT` |
| `null` | the null type | - | - |

These are the types a literal has as an ARGUMENT, which is where it shows:
with `def f(a: _int32)` and `def f(a: _int64)`, `f(5)` calls the first and
`f(5000000000)` the second; with `def f(a: _int64)` and `def f(a: decimal)`,
`f(2)` is ambiguous (checked: `expression_type_to_normal_type` in
`dependency.cpp`). A literal converts to any other number type like any
number does.

A text literal becomes a `string` object (`new string(literal)`,
`CONV_LITERAL_TEXT`) wherever an object is needed: when a member is used on it
(`"abc".size()`), as the LEFT operand of `+ == !=` (`"abc" + 5` is text;
`5 + "abc"` is `E0511`), in `x := "abc"`, in a ternary whose other side is a
`string`, as an item of an untyped array literal. `sizeof("abc")` is its
length.

### 7.3 Variables and inferred types

`name : type = value;` - the value must convert to the type (7.5).
`name : type;` - no value: a number is 0, a nullable type is null; a box class
or `string` that is neither nullable nor an array gets an object from its
no-argument constructor (`n : int;` holds an `int` with value 0 -
`pre_initialize_class` in `expression.cpp`); any other non-nullable object
type is "not given a value yet" (for fields see 7.15.4).
`name := value;` - the type is worked out from the value
(`validate_field_type` in `field_processor.cpp`):

| the value is | the variable's type |
|---|---|
| marked `LIT_INTEGER` (`5`, `2 + 3`) | `std#int` - also when the number needs 64 bits: `big := 3000000000 * 4;` holds -884901888 in the C++ compiler. The bootstrap keeps the type and adds `W1023` ("does not fit an `int`; write `big : _int64 = ...` or `new long(...)`") |
| marked `LIT_DECIMAL` | `std#double` |
| marked `LIT_CHAR` | `std#char` |
| marked `LIT_BOOL` | `std#bool` |
| marked `LIT_TEXT` | `std#string` |
| `null` | `object?` (C++ says `object`; the bootstrap adds the `?`, since null is what it holds) |
| any other value (a variable, a call, arithmetic on variables, `new`) | the value's type, nullability included: `k := a + b;` with two `_int64` is an `_int64` (7.8.1; **[differs D13]**), `h := a / b;` a `decimal`, `t := a < b;` an `_int32` |
| a lambda | its function type; every parameter must have a type (`E0800`) |
| no value (`nil`, a call that returns nothing) | `E0406` |
| a type name (`x := string;`) | `E0404` |
| a circle of fields or globals (`a := b; b := a;`) | `E0408` |
| itself, for a LOCAL (`x := x;`, and also `n : _int64 = n + 1;`) | `E0407` "used in its own declaration": the local does not exist until its declaration is complete. (The C++ compiler accepts `n : _int64 = n;` and reads the variable before it has a value - no meaning, so refused: **D51**) |

The same table gives the type of a field `name := value;`. The result of
`def f() := value;` follows a DIFFERENT table (found by the calls stage, checked
with the C++ compiler, `validate_function_type` in `function_processor.cpp`):
a literal stays a plain number (`5` an `_int32`, `1.5` a `decimal`, `'a'` and
`true` an `_int8`, `"x"` an `_int8[]`, `null` `object`); a variable or field
named by itself keeps its type (`def width() := w;`); any OTHER plain number
(`w * 2`, `sizeof(data)`, a call, a cast) becomes its number class - `def
size() := sizeof(data);` returns an `int` OBJECT, which is why the library writes
`s.size().value`; a value of type nil makes the function return nothing
(`def sleep(ms: decimal) := time.sleep(ms);`).

Several names in one statement, `a : int = 1, b = 2;`, share the type of the
first (checked; `variables_multiple.sharp` in the suite).

`n : int;` and `s : string;` were run: they hold an `int` 0 and an empty
`string`, not null.

### 7.4 Names

#### 7.4.1 A plain name used as a value (`resolve_normal_item`, `dependency.cpp`)

Looked for in this order; the first place that has the name wins.

1. type parameters of the generic function, then of the class (the name then
   means a TYPE);
2. local aliases, innermost scope outward;
3. local variables and parameters, innermost scope outward. Leaving a lambda
   for the function around it continues the search there: a hit is a CAPTURE
   (4.3);
4. the class the code is in, and its base classes upward (`find_member`,
   7.4.3): a field; then an alias; then an enum declared inside it; then a
   class declared inside it (a generic one with its arguments); then the
   class's own name;
5. the modules the file sees, in order (own module, `std`, `std.io.console`,
   the imports): a global variable; an alias; an enum; a class (a generic
   class with its arguments, `list<int>`);
6. a function of that name used as a value (9): in the class, then in the
   modules;
7. not found: `E0400` (section 9.3).

Notes.

- Code in a class declared inside another class does NOT see the outer class's
  fields and functions by their plain names, static ones included (only its
  TYPES, stage 3) - checked: `total` and `help()` of the outer class are
  "could not resolve symbol" in the inner one. The message for a name that
  exists in an outer class says so and offers `outer.name`. (Decision 10,
  section 11.)
- `module#name` looks in that module only (steps 5 and 6); `E0403` when absent.
- An enum value is reached through its enum: `color.red`. Its PLAIN name
  works too wherever the enum is visible (probed in stage 6: `e : color =
  red;` in a global function compiles): it is looked for last, after
  everything that could shadow it, in the enums of the modules the file sees.
- In front of a `.`, a class wins over a FUNCTION of the same name that
  lookup met first (`build.is_windows()` inside a class with a function
  `build`; the C++ compiler looks for fields and classes before functions).
- `base` alone is a value - the object, seen as its base class (`x : a =
  base;` compiles). `base->name` reaches a member of the base class; the
  bootstrap reads `base.name` the same way (in the C++ compiler `base.name`
  is `base` alone, a parser accident).
- The C++ compiler also looks for "labels" (step 2 of its list). Labels and
  `goto` were never finished there and the parser has neither; the bootstrap
  has none outside `asm` blocks.

#### 7.4.2 Shadowing

| new name | against | result |
|---|---|---|
| local | a local or parameter of the SAME function in the same or an enclosing block | `E0600` "already declared" (C++: "local field is already defined in function") |
| local | a local of a block that has ended (a sibling block, an earlier loop) | fine: that name is gone |
| local or parameter | a field of the class, a global, a function | fine: the local wins; the field is `self->name` |
| lambda parameter or local in a lambda | a local of the enclosing function | fine: the lambda is a function of its own and its name wins inside it (checked: `x : _int64 = 1; f := { x: _int64 -> (_int64) return x; };` compiles). The outer `x` is then simply not captured |
| field | a field of a base class | fine: the nearest wins (7.4.3) |

A local exists from its declaration to the end of its block. Using it earlier
in the block is `E0407` ("declared further down, on line n"), not "unknown" -
unless a field or global of that name exists, which is then what the earlier
use means (the lookup of 7.4.1 simply does not see the local yet).

How `E0600` is found without searching every enclosing scope for every
declaration: the scope chain is walked from the new local's scope outward and
stops at the first scope whose `function` is another function. Blocks nest a
handful deep, so this is a few hash lookups per declaration.

#### 7.4.3 The members of a class: `find_member(class, name)`

One function for every kind of member (`postprocessor/members/member_lookup.sharp`)
**[differs D17]**. It looks in the class, then in its base class, and so on up
to `std#_object_`; for functions also in the interfaces of each (`locate_functions_with_name`).
The first class that has ANYTHING of that name ends the search for fields,
aliases, classes and enum values. For functions the search goes all the way up
and collects every overload, nearest class first. A function hides every
function FURTHER UP that takes exactly the same parameters: the one it
overrides (`overrides` link), and one it merely repeats - `def area()` in
`circle` and in `shape`, without `override` because shape's has a body: on a
circle, `area()` is circle's (D44: the C++ compiler agrees when the argument
is exact, and calls the use ambiguous when it needs a conversion). Such a
function is not an override - a function with a body is never replaced at run
time, so through a `shape` variable shape's runs - and gets `W1041` when it has
parameters (inheritance/hidden_functions.sharp; two `static` functions with the
same parameters likewise). So the list never
holds two functions a call could not choose between. Generic functions are
not compared this way (their parameter types are not real types yet).

`a.name` where `a` is

| `a` | `name` may be |
|---|---|
| a value of a class type | an instance field, a static field (allowed; a `localized` one gets `W1012`), a function |
| a class name (`bound_type_name`) | a static field, an enum value, an alias, a class declared inside, a static function; an INSTANCE member here is `E0703` |
| an enum type | one of its values |
| a module (`std.io#x`) | handled before, by 7.4.1 |
| a number, an array, `object`, a function value | nothing: `E0405` ("`count` is an `_int64`; it has no members"). Arrays have no members; their length is `sizeof(a)` |
| a nullable value | see 7.11 first |

For a CALL `a.name(args)` see 7.7.

### 7.5 Conversions

#### 7.5.1 The table of implicit conversions (`is_explicit_type_match`, `is_implicit_type_match` in `sharp_type.cpp`)

"Implicit" = allowed wherever a value is given to something that has a type:
an argument to a parameter, a value to a declared variable or field, the right
of `=`, a `return` value, an array item, a `foreach` item to its variable.
Rank is used by overload resolution (7.6); lower is better.

| from `S` | to `T` | allowed | rank | kind |
|---|---|---|---|---|
| any | the same type | yes | 0 | identity |
| `t` | `t?` | yes | 0 | to nullable |
| `t?` | `t` | NO - `E0721` ("may be null"; help: `value!!` or `value ?: fallback`) | | |
| `t?` | `object` | yes: `object` may hold null without the `?` (checked: `o : object = maybe_animal;`) | 3 | to object |
| `null` | any nullable class, a function type `?`, an array `?`, `object` and `object?` | yes (checked: `o : object = null;`; for the function type see **D27**) | 1 | null |
| `null` | any other non-nullable type | NO - `E0720` | | |
| `null` | a plain number | NO - `E0720` (help: numbers are never null) | | |
| integer or decimal (not an array) | any other integer type or `decimal` | yes, always - there is no "narrowing" error; the value is cut down or its fraction dropped (`big : _int8 = 300;` holds 44) | 1 | numeric |
| a number array `n[]` | the SAME `n[]` only | yes | 0 | identity |
| class `C` | a base class of `C`, an interface `C` (or a base of `C`) implements | yes | 2 | upcast |
| class or interface `B` | a class `C` BELOW `B` (or one that implements `B`) | yes, with `W1017` - the C++ compiler's class match is symmetric, in arguments too **[D6]**. The value is tested at run time (`conversion.checked`) | 4 | downcast |
| class `C` | a class that is neither above nor below it (`c : cat = a_dog;`) | NO - `E0410` | | |
| interface | `object` / the class `_object_` | yes | 3 | to object |
| class, interface, enum, any array of objects | `object` | yes | 3 | to object |
| a number array `n[]` | `object` (not `object[]`) | yes | 3 | to object |
| a function value | `object` | yes | 3 | to object |
| `C[]` | `B[]` where `C` converts to `B` by upcast | yes (C++ treats the array like its element); `B[]` to `C[]` likewise with `W1017` | 2 / 4 | upcast / downcast |
| `C[]` | `object[]` | yes | 3 | to object |
| a plain number | `object` | NO (wrap it: `new int(n)`) | | |
| function type | the same function type: same number of parameters, each the same type, the same result | yes | 0 | identity |
| function type | a different function type | NO - function types are compared exactly | | |
| a box object (`int`, `bool` ...) | a plain number | only in the places of 7.5.6 (checked: `m : _int64 = an_int;` is an error; help `.value`) | 1 | unbox |
| an enum value | a plain number | NO (checked; help `.ordinal`, 7.16) | | |
| `S` | class `C` that can be built from one `S` | yes (7.5.4) | 5 | construct |
| `_int8[]` text literal | `std#string` | yes (a case of the line above; named for messages) | 5 | literal text |
| a value of type nil | anything | NO - `E0406` | | |
| the error type | anything, and back | yes, silently | 0 | error |
| array | non-array (or the reverse), other than the `object` lines | NO | | |

Nullability is checked LAST: when everything but the `?` fits, the error is
the nullability one (`E0721`), not "cannot convert" (C++:
`indirect_match_w_nullability_mismatch`). A value that may be null is still
accepted where a constructor of the target takes a nullable value
(`return name;` with `name : _int8[]?` in a function returning `string` -
`std#_enum_.to_string` - works because `string` has a constructor from
`_int8[]?`).

#### 7.5.2 Numbers

- All number types convert to each other implicitly, at rank 1. A literal is
  no exception: `5` IS an `_int32` (7.2), identical to an `_int32` parameter
  and one conversion away from every other number type.
- Within rank 1, when a call would otherwise be a tie, conversions are
  graded (7.6 step 7a): to the same kind of number without losing anything
  (an integer to an integer at least as wide; `decimal` to `decimal`) is
  better than integer to `decimal`, which is better than a conversion that
  can lose something (to a narrower integer; `decimal` to an integer). The
  C++ compiler has no grades: more than one number overload that is not the
  exact type is "ambiguous" there.
- A number is NOT text: `string` has a constructor from one character, and
  only a CHARACTER-sized value (an `_int8` or `_uint8` variable, a char
  literal) is accepted for a `string` through it - checked: `takes_text('a')`
  and `takes_text(c)` with `c : _int8` run, `takes_text(300)` is an error
  (`sharp_type.cpp`, the "A number is not text" comment).
- A number does not become a box where a choice would have to be made:
  `bx('c')` with `bx(int)` and `bx(string)` is ambiguous in both compilers.
- An array of a class is never built from one value: `string[]` does not
  accept `"x"`.

#### 7.5.3 Classes

`C` "is a" `B` when `B` is `C`, a base class of `C` at any depth, or an
interface listed by `C` or by any of its base classes (or a base interface of
one of those). Generic instances are different classes: `list<dog>` is not a
`list<animal>`.

#### 7.5.4 Conversions through the class: constructor, `init(x)`, `operator=` **[differs D5]**

A value of type `S` converts implicitly to a class `C` (not an array of `C`)
when exactly one of these exists, tried in this order:

1. a constructor of `C` with ONE parameter that takes `S` by a conversion of
   rank 0 to 4 (never by another construction: conversions do not chain);
2. an `init(x: P)` conversion function of `C` (7.15.3) whose `P` takes `S` the
   same way.

The result is `new C(value)`. This is how `"text"` becomes a `string`, `5`
becomes an `int`, and `takes_point(5)` builds a `point` when `point` has a
constructor from a number (C++: `match_constructor`, `match_initializer`).
If several one-parameter constructors accept `S`, the best by rank is taken;
a tie is `E0503`.

For ASSIGNMENT to an existing variable, `target = value` (7.10): when `C` has
`operator=(P)` and `P` takes `S`, the operator function is called on the
object the target holds - unless the target may be null or is an array
element, in which case a new object is constructed and stored (rule 1 / 2;
`assign_by_construction` in `assign_expression.cpp`).

The C++ compiler tries a different subset in each place (arguments:
constructor only; a declared local with a value: `init(x)` only; `=`:
`operator=` only). The bootstrap uses the one rule above everywhere; it
accepts everything the C++ compiler accepts. One exception keeps the
MEANING the same where both accept a program: in a DECLARATION with a value
(a local, a global or a field: `v : c = 5;`) an `init(x)` that takes the
value runs, even when a constructor takes it too - the C++-built program
prints "init" there and "ctor" for an argument or a returned value (checked
by running; `with_init_block_preferred` in conversions/class_conversion.sharp,
10.10). `-analysis` names the function a construction runs:
`conversion (construct, init block init(_int64))`.

A value that already IS of the class (or of a class below it) is never
built into a new object: `w : item?` given where an `item` is wanted is the
null rule's (`E0501` / `E0721` with the `w!!` help), even when a constructor
of `item` takes an `object`, which takes null (10.10).

#### 7.5.5 Explicit conversion: `value as T` (`cast_expression.cpp`)

`T` must be a class, `object`, a number type, or an array of those (`E0412`
for a function type or nil).

| from | to | result |
|---|---|---|
| the same type | | allowed, `W1003` "redundant cast" |
| `t` | `t?` | allowed, `W1003` too (C++: "redundant cast from `a: animal` to `animal?`") |
| a text literal | `string` | `E0412` - a cast never BUILDS an object (checked: `"abc" as string` is "cannot cast `string_literal`"). Help: `new string("abc")` |
| number | number | converts (integer <-> decimal, cut to size); never fails |
| `object` | a class, an array | allowed, checked at run time |
| a class | a class above it, `object` | allowed, nothing to check |
| a class | a class below it | allowed, checked at run time |
| a class or interface | an interface, or an interface to a class | allowed, checked at run time **[differs D22]** |
| a class | an unrelated class | `E0412` ("`circle` is never a `string`") |
| a number | a class, or a class to a number | `E0412` (help: `new int(n)` / `.value`) - checked: `an_int as _int64` is "cannot cast `std#int` to `_int64`" |
| a nullable value `s?` | `t` | allowed when `s` to `t` is: the result has exactly the type written after `as`, so `maybe_animal as dog` is a `dog`, not a `dog?` (checked: it is then accepted where a `dog` is wanted). To keep the `?`, write it: `x as dog?` |

The result type is always the type as written. `as` binds tighter than the
binary operators: `a + b as t` is `a + (b as t)`.

#### 7.5.6 The number classes ("boxes")

`std#bool byte short char int long ushort uchar uint ulong double` each hold a
number in the field `value`. A non-nullable, non-array value of one of them is
used as its number, without being asked, in exactly these places
(`extract_value_field_from_expression`): the condition of `? :` (the
condition of a STATEMENT takes only a `bool` object, 7.12.1); both sides of `&&` and `||`; the RIGHT operand of
arithmetic, comparison and bit operators when the left is a plain number; an
array index; the size in `new t[n]`. Everywhere else a box is an ordinary
object, and operators on it are its operator functions (7.8). Checked:
`if(a_bool) { }`, `5 + an_int` use the number; `m : _int64 = an_int;` and
`an_int as _int64` are errors. In the typed tree each use is a
`bound_conversion` of kind `CONV_UNBOX`.

The other way round - a plain number where a box is wanted - is the
constructor conversion of 7.5.4.

### 7.6 Overload resolution **[differs D3]**

Given a name, the argument expressions (already analysed) and the place of the
call. The textbook shape - collect candidates, keep the applicable ones, pick
the best by ranking the conversions each needs - with the C++ compiler's own
three steps as its first three, so that every call that compiler resolves
gets the same function here and ranking only decides where it gives up.

1. **Candidates.** Every function of that name in ONE scope (7.7 says which
   scopes are asked, in which order; a scope with no applicable candidate
   hands on to the next). In a class: the class's own functions, then each
   base class's, then the interfaces', in that order; a function hidden by an
   override of it is left out. For global functions the scope is ONE module:
   the modules the file sees are asked in import order (own module first) and
   the first module with an applicable candidate decides - a function in a
   nearer module hides the same name further away, whatever its parameters
   (`resolve_function(name, file, ...)` in `dependency.cpp`).
2. **Applicable.** A candidate is applicable when it has as many parameters as
   there are arguments and every argument converts implicitly to its parameter
   (7.5.1, constructions included). An untyped lambda argument fits any
   function-typed parameter with the same number of parameters (7.14).
   None applicable in any scope: `E0501`, listing every candidate of every
   scope with the reason it does not fit (section 9.3). No function of that
   name at all: `E0500`.
3. **Exact.** If some applicable candidate takes every argument at rank 0
   (the identical type, or the same type with a `?`), the first such candidate
   in candidate order wins. (C++: "the first exact match".)
4. **Only one.** If exactly one candidate is applicable, it wins.
5. **The plain-number rule** (`overload_preference.h`). If exactly one
   applicable candidate takes EVERY plain-number argument as a plain number
   (not as an object built from it), it wins.
   Steps 3 to 5 are the whole of the C++ compiler's `resolve_function`; where
   they do not decide, it reports "ambiguous".
6. **Ranking.** Candidate A is better than B when no argument of A has a worse
   rank than the same argument of B and at least one has a better rank (the
   rule of C++ and Java). If one candidate is better than every other, it
   wins. The ranks, from 7.5.1:

   | rank | conversion |
   |---|---|
   | 0 | identical; `t` to `t?` |
   | 1 | number to number; `null` |
   | 2 | to a base class or an interface |
   | 3 | to `object` |
   | 4 | base to derived (`W1017`) |
   | 5 | through a constructor or `init(x)` |

7. **Ties.** Asked in order, each only among the candidates still tied, until
   one is left:

   a. *the grades of number conversions* (7.5.2): one that keeps its kind and
      loses nothing, before integer to `decimal`, before one that can lose
      something (among those, the wider target first);
   b. *nearer class*: a candidate declared in a class nearer to the object's
      class beats one further up.

   Still tied: `E0503`, listing the tied candidates and what to write to
   choose (a cast on the argument).

Checked against the C++ compiler with probes:

| call | candidates | C++ compiler | here |
|---|---|---|---|
| `lit(5)` | `(_int32)`, `(_int64)` | `(_int32)`, exact | the same (step 3) |
| `lit(5000000000)` | the same | `(_int64)`, exact | the same |
| `mix(5)` | `(_int32)`, `(decimal)` | `(_int32)`, exact | the same |
| `ch('a')`, `ch(true)` | `(_int8)`, `(_int64)` | `(_int8)`, exact | the same |
| `nul(a_string)` | `(string)`, `(string?)` | the one declared first | the same (step 3) |
| `two(an_animal)` | `(animal)`, `(dog)` | `(animal)`, exact | the same |
| `one(an_animal)` | `(dog)` only | `(dog)`, base to derived | the same (step 4), with `W1017` |
| `of(2024, 2, 29)` | `(decimal, decimal, decimal)`, `(decimal, month_like, decimal)`; `month_like` has a constructor from a number | the first, by the plain-number rule | the same (step 5) |
| `dd(2)` | `(_int64)`, `(decimal)` | ambiguous | `(_int64)` (step 7a) - wider |
| `obj(new dog())` | `(object)`, `(animal)` | ambiguous | `(animal)` (step 6: rank 2 before 3) - wider |
| `three(an_animal)` | `(cat)`, `(dog)` | ambiguous | ambiguous (both rank 4) |
| `bx('c')` | `(int)`, `(string)` | ambiguous | ambiguous (both rank 5) |
| `ar(big + one)`, both `_int64` | `(_int8)`, `(_int32)`, `(_int64)` | `(_int32)`: the sum is labelled `_int32` | `(_int64)`: the sum is an `_int64` **[D13]** - the one place a winner differs, and on purpose |

After the winner is known: each argument is wrapped in its conversion; an
untyped lambda argument gets its parameter and result types from the winning
parameter and is analysed then (7.14).

**While lambdas are still pending (stage 7).** A lambda argument takes part
by its number of parameters only. If the candidates that remain after step 2
differ in what they want from a lambda argument, the call is not decided and
not reported: it becomes a `bound_pending` (waiting for stage 9) with the
error type. Stage 9 removes this case.

**Cost.** Candidates are found by name in a hash map (`scope`), so a call
looks at the overloads of ONE name, not at the class. A first, quick pass
asks each candidate with the right number of parameters whether it takes the
arguments WITHOUT building objects from them; the first that takes every
argument exactly is the answer (step 3), and the costly question "could a
constructor of the parameter's class take this?" is never asked of the other
overloads (`number.operator==` has twenty). What the quick pass found is
kept, so step 2 asks again only the candidates that did not fit. The cache of
(class, class) conversions the first plan had here
(`sharp_class.conversion_cache`) was not built: the library asks 66,000
conversion questions in all, too few for it to matter.

### 7.7 Calls

#### 7.7.1 `name(args)` - what is tried, in order (`resolve_function_reference_item`)

1. a LOCAL variable or parameter called `name` (a capture included): it must
   have a function type; the call is a `bound_value_call` (7.7.4). A local
   that is not a function value: `E0504`;
2. inside class `C`, `name` equal to `C`'s own name: construct, as `new C(args)`;
3. in a class: a field of the class (or a base class) with a function type:
   `bound_value_call` on the field;
4. in a class: the functions called `name` of the class, its base classes and
   interfaces - overload resolution (7.6). If the name exists there but
   nothing is applicable, the search goes on with 5 and 6, and the `E0501`
   message, if it comes to that, lists these candidates too;
5. a global variable with a function type, in the visible modules;
6. the global functions called `name`: the modules the file sees are asked
   one after the other, and the first module with an applicable function
   decides (7.6 step 1);
7. a class called `name`: construct it - `point(1, 2)` is `new point(1, 2)`
   (`resolve_class_construction`). With type arguments, `list<int>()`, the
   instance is made first;
8. `E0500`: nothing callable has that name.

A function found in step 4 that is not `static`, called from static code, is
`E0704`. A constructor cannot be called as a function on an existing object:
`p.point(1, 2)` - the class's own name used as a member - is `E0508` (C++:
"cannot explicitly call constructor"), reported where a missing member would
be (`names/no_member.sharp`). A generic function
called without its type arguments (`gen_f(5)`) is `E0517`, not "no such
function".

A trailing lambda (`run { ... }`, `repeat(4) { ... }`, `apply(5) { it * 2 }`,
`N_CALL` with an `N_LAMBDA` child after `N_ARGUMENTS`) is simply the LAST
argument of the call; `run { }` has an `N_ARGUMENTS` with no children.

`module#name(args)`: steps 5, 6, 7 in that module only.

#### 7.7.2 `value.name(args)` and `type.name(args)`

On a value of class `C` (after the null rules, 7.11): a function-typed field
called `name` (step 3), else the functions called `name` of `C` (step 4).
Nothing else. On a type name: the same, and the function must be `static`
(`E0704`); a class declared inside the type that is called `name` is
constructed. `E0402` when `C` has nothing of that name (did-you-mean over
`C`'s members). On a number, an array, `object`: `E0405`.

`self->name(args)` and `base->name(args)` are calls on the current object;
`base` names the base class's function on purpose: the call is `CALL_BASE` and
is never looked up at run time. `base` in a class that names no base class
still has `_object_` (7.4.3). `base@type` must name a class the current class
inherits from (`E0422`). `self` and `base` in static code: `E0421`.

#### 7.7.3 Which function runs: `dispatch`

Only `override` functions take part in run-time lookup: a plain function of a
base class is never replaced by one of the same name below it (checked:
`b : base_one = a_derived; b.id()` runs `base_one.id` although `derived_one`
declares an `id` too; and `override` on a function that overrides no
body-less function is `E0342`).

`dispatch` is set when the call is bound, from the declarations alone:

| the call | `dispatch` |
|---|---|
| a global or `static` function | `CALL_STATIC` |
| `base->f()` | `CALL_BASE`: the base class's function, never looked up |
| on an object; the function has no body, or is an `override` (it belongs to an "override family": `overrides` is set or it is body-less) | `CALL_VIRTUAL`: the object's class decides at run time, through the body-less function at the top of the family (`sharp_function.family_root`) |
| on an object; any other function | `CALL_DIRECT` |

The C++ compiler narrows `CALL_VIRTUAL` further (`dispatch_target` in
`expression.cpp`): a call of an `override` function is direct when no class
BELOW overrides the same function. That needs the whole program, gives the
same result, and is an optimisation: the code generator can make it by
looking at `sharp_function.overridden_by` (the functions that override this
one, filled in as override links are made, generic instances included), which
is complete when analysis ends. The typed tree is not rewritten for it.

#### 7.7.4 Calling a function value (`post_ast_expression.cpp`)

`f(args)` where `f` has the type `(p1, p2)(r)`: as many arguments as
parameters (`E0505`, "expected 2 but 3 were found"), each converting to its
parameter (7.5, constructions included). The result is `r`. Anything else in
front of `(`: `E0504`. A function value that may be null (`f : ()()?`) may be
called without `!!` (7.11). Any expression may be called this way, not only a
name: `handlers[i](x)`, `make_adder(2)(3)`, `(pick())()`.

#### 7.7.5 `new` (`new_expression.cpp`)

| form | rule |
|---|---|
| `new C(args)` | `C` is a class - not an `ext` class, a type parameter that is not a class; not an interface or an enum **[differs D26]** (`E0507`). Overload resolution among `C`'s constructors only (never a base class's); none fits: `E0506`. A private CLASS used outside its class is `E0700`; a private CONSTRUCTOR used outside is `W1022` **[D8]** (7.9) |
| `new t[n]` | `t` a number type, a class, `object`, or a function type (`E0417`); `n` a number or a box (7.5.6), turned into an integer. Result `t[]` |
| `new t[] { a, b }` | every item converts to `t`, construction allowed (`E0410`); a lambda item takes its parameter and result types from the item type `t` when it does not write them (the C++ compiler wants it fully typed: "lambda must be fully qualified" - **[differs D95]**). Result `t[]`. An item that MAY BE NULL (`new node[] { sure, maybe }`) is accepted with `W1026`: the C++ compiler compares item types without looking at the `?`, and a slot of an object array can hold null anyway (`new node[3]` is three nulls) - the rule of section 5 for what the old compiler accepts and gives a meaning to. `null` written out as an item is `E0720` (C++: "cannot assign array item of type `unknown`") |
| `{ a, b }` | an array literal: the item type is the FIRST item's type without its `?` (a literal's as in 7.2; a text literal first makes it `string`; a box literal does not happen: `{ 1, 2 }` is an `_int32[]`), and every later item must convert to it, construction allowed (`E0410`) - checked: `{ 1, 2.5 }` is an `_int32[]` holding 1 and 2. An item that may be null: `W1026`, as above. No items, or `null` first: `E0418` - an empty `{ }` is refused also where an array type is expected (checked: `a : _int64[] = { };` is "could not determine the type of the array"; help: `new _int64[0]`) (`vector_array_expression.cpp`) |
| `{ k : v, ... }`, `{ as <k, v> : }` | a dictionary: `std#dictionary<k, v>` built from `std#pair<k, v>` objects; key and value types from the first pair or from `as <k, v>`; every pair must fit (`E0419`) (`dictionary_expression.cpp`) |

Every class has a constructor without parameters: when none is written the
compiler adds a public one (`create_default_constructor` - also when the class
has OTHER constructors). So `new point()` always exists, and a class whose
base class only declares `base_one(n: _int64)` may still leave out
`-> base(...)` (checked).

### 7.8 Operators

#### 7.8.1 Which rule applies (`binary_expression.cpp`: by the LEFT operand)

| left operand | operators | rule |
|---|---|---|
| a plain number (literal or not) | `+ - * / % ** & \| ^ << >> == != < > <= >= && \|\|` | the right must be a plain number or a box (7.5.6); anything else `E0511` (`5 + "abc"`: help "put the text first: `"abc" + 5`, or `"" + 5 + "abc"`"). Result: table below |
| a text literal | `+ == !=` | the literal becomes a `string` object, then as a class. Other operators: `E0511` |
| a class | `== null`, `!= null` | "is there an object" - never the class's own `operator==` (it cannot be called on null) |
| a class | any operator | the class's function `operator<op>` with one parameter, by overload resolution (7.6) on the right operand: a `bound_call` with the function's own result type (`t == "abc"` on a `string` is a `bool` OBJECT, `"a" + 1` a `string`). None: for `==` `!=` "the same object"; for anything else `E0511` **[differs D9]** |
| `object`, an array | `== !=` | "the same object" / null test; arrays must both be arrays, and one array must be able to be both: items of the same number type, or classes one of which is the other or below it (`_int64[] == _int32[]`, `k[] == string[]`: `E0511`, the C++ "expected object type" / "mismatched array types"; `object[]` and `object` compare with any array) **[differs D91]**. Any other operator: `E0512` for `object` ("cast it first: `(x as t) + ...`"), `E0513` for an array |
| a function value | `== !=` | against `null`: null test; against a function value: `function_value.same` / `different` (4.3) |
| a value of type nil | any | `E0406` |

A nullable class on the left (`maybe + other`) is accepted by the C++ compiler
without `!!` (checked) and fails at run time when there is no object; the
bootstrap accepts it too (decision 5, section 11). A nullable RIGHT operand is
an argument like any other: it must fit the operator function's parameter.

Result types for numbers **[differs D13]**. The type is not decoration: it
decides which overload a result is passed to (`"" + x` has nine).

| operator | calculated as | result type |
|---|---|---|
| `/` | decimal, always (`7 / 2` is `3.5`, also for two `_int64`) | `decimal` |
| `+ - * **` | decimal when either side is a decimal, else integer (`**` is calculated with pow and cut back) | `decimal` when either side is; else `_int64` when either side is a 64-bit integer (`_uint64` when both are unsigned); else `_int32` |
| `% & \| ^ << >>` | integer (a decimal operand is cut to an integer first) | `_int64` / `_uint64` when either side is a 64-bit integer, else `_int32` |
| `== != < > <= >=` | decimal when either side is | `_int32` (0 or 1) |
| `&& \|\|` | each side is "not zero"; the right side runs only when needed | `_int32` |
| `== !=` against `null`, between references | - | `_int8` |

Two LITERALS fold to a literal, typed as the C++ compiler types it (checked
in the calls stage: `x := 1 < 2;` makes `x` a `double`, and `"" + (1 < 2)`
prints `1.0000000000000000`): `+ - *` without a decimal literal, and
`% & | ^ << >> && ||`, give a whole-number literal (`_int32`, or `_int64` when
it does not fit); `/ ** == != < > <= >=`, and `+ - *` with a decimal literal,
give a DECIMAL literal. The table above is for operands that are not both
literals. Other constant operands are folded too (6.3), but the result is an
ordinary value of the type in the table. `x % 0` with a constant 0 is `E0427`
(also for a variable `x`: D55).

#### 7.8.2 Unary and `++ --` (`unary_expression.cpp`)

| form | operand | rule |
|---|---|---|
| `-x` | number | the same type (`-big` is an `_int64`; a literal: folded, still a literal) |
| `!x` | number | `_int8`; on a literal `W1010` ("could be written `false`") |
| `-x`, `!x` | class | `operator-` / `operator!` with NO parameter (told apart from binary `-` by the parameter count); none: `E0511` (as in the C++ compiler: "does not have any qualified overloads") |
| `++x`, `--x` | a number that can be assigned (local, field, array element, property) | the new value; type unchanged |
| `x++`, `x--` | the same | the OLD value |
| `++x` / `x++` | class | `operator++` with no parameter (prefix) / with one `decimal` parameter that is given 1 (postfix) |
| any | `object` | `E0512` |
| any | array, text, nil, `++` on `true` | `E0514` |
| `++ --` | not assignable (`5++`, `f()++`) | `E0711` |

On a property (7.17) `p.count++` is getter, add, setter.

#### 7.8.3 Index `a[i]` (`array_expression.cpp`)

On an array: one index (`E0416` for `a[i, j]`; C++: "Multi-dimensional arrays
are not supported"), a number or box turned into an integer (`E0415`); the
result is the item type, nullable when the array is a `t[]??` **[D25]**. An
array that may itself be null (`numbers : decimal[]?`) is indexed WITHOUT
`!!` (the suite relies on it: `non_null_index.sharp`, "index without !!");
`numbers!![1]` is allowed too.
On a class: `operator[]` by overload resolution on the index (`E0414` when the
class has none) - a `bound_call`. On anything else: `E0414`.

Storing, `a[i] = v`: into an array directly. Into an object
(`assign_through_index_operator` in `assign_expression.cpp`; decided
2026-11-12, `list_index_assignment.sharp`): when the class has an instance
function `put(index, value)` that fits, `names[0] = x` IS `names.put(0, x)`.
When it has none, the old meaning stays: `operator[]` is called and `=` is
applied to the object it returned, through that object's `operator=`
(`E0711` when the returned type has no `operator=` that takes the value).
Compound assignment `o[i] += v` on an object is always the second form:
`operator+=` on what `operator[]` returned (`E0712` when there is none).

#### 7.8.4 Others

| form | rule |
|---|---|
| `c ? a : b` (`inline_if_expression.cpp`) | `c` a condition (7.12.1). Type: `a`'s type when `b` converts to it without construction - `flag ? 1 : 2.5` is an `_int32` and gives 1 or 2 (checked); else `b`'s type when `a` converts to it. When the two differ only in `?` (or one is `null`) the result is the nullable type (`flag ? node : null`). A text literal next to a `string` becomes a `string`. Otherwise `E0420` |
| `a ?: b` | 7.11 |
| `a!!` | 7.11 |
| `sizeof(e)` (`sizeof_expression.cpp`) | `e` a class value, `object`, an array or a text literal: `_int32`. A plain number or `null`: `E0413` (checked: "cannot get sizeof from expression of type `null`") |
| `e is t` (`is_expression.cpp`) | `_int8`. Known while compiling: constant 1 when `e`'s class is `t` or below it and `e` cannot be null, when `e` is a plain number and `t` a number type (`n is decimal` is 1 for an `_int64` n too: 12.4.2), and when `t` is `object` / `object[]` and `e` cannot be null; constant 0 when exactly one of the two is a plain number (`n is string`, `thing is _int64`: 12.4.3); otherwise tested at run time. `t` any type. Allowed on a nullable value |
| `(e)` | `e`; no node of its own in the typed tree |

#### 7.8.5 Declaring operators

`def operator+(other: t) : r` in a class. Any binary operator, `!`, unary `-`
(no parameter), `++` `--`, `[]` (one parameter per index), `=` and the compound
assignments (`+=` ...) can be declared; overloads by parameter type are
allowed. They are ordinary instance functions with a special name
(`sharp_function.is_operator`): `operator+`, `operator==`, and `operator[]`
for the index (`function_name` in `declare.sharp`). An operator function is
always a member of a class - written in it, or added with
`def money.operator+(...)`; at the top of a file with no class before the
name it has no `self` and is `E0366` (D41).

### 7.9 Access (`check_access` in `dependency.cpp`)

Checked wherever a member, a class or a constructor is USED by name: in an
expression by the body stages (`compiler/names/access_rules.sharp`), and for
a type written where a type is expected (`x : vault.plate;`, `new plate()`,
`x as plate`, a field's type, a base class) by the type-names stage
(`postprocessor/names/type_access.sharp`), which visits every type name of
the program once.

| modifier | may be used from | when it is not |
|---|---|---|
| `public` (the default for everything) | anywhere | |
| `private` on a field, a class, an alias, an enum INSIDE A CLASS | code of the SAME class (a class declared inside it does not count, nor does a class below it - both checked) | `E0700` |
| `private` on a function or a constructor | the same rule | `W1022`: the C++ compiler does not check these at all - `obj.secret()`, `counter.hidden_helper()`, `new thing(3)` with a private constructor and a derived class calling a private function of its base all compile and run (checked) **[D8]** |
| `protected` on a member of a class | code of the same class; of a class above or below it; and any code in a file of the same MODULE as the code's class | `E0701` |
| `private` on something declared in a MODULE (a global variable, a class, an alias, an enum outside any class) | the module's own GLOBAL code only: its global functions and the values of its globals, in any file of the module. NOT the classes of the module (checked in the review of stage 6: a private global read inside a class of its own module is "cannot access private field `m#__srt_global.x`"; from a global function in another file of the module it compiles). The globals of a module behave like the members of one hidden class per module | `E0700` |
| `protected` on something declared in a module | any code of the same module, classes included; no other module (checked) | `E0701` |
| `local` (only on things declared in a module, not in a class) | the same FILE | `E0702` |
| `local`, `private`, `protected` on a FUNCTION declared in a module | the same table as for a global variable (`local`: the same file; `private`: the module's global code; `protected`: the same module) | `W1022`, as for the functions of a class: the C++ compiler checks neither (`other#helper()` of a `local def helper()` in another file compiles and runs) **[D92]** |

A class may always name ITSELF, whatever its access (`private class plate {
next : plate?; def deep() := plate.depth; }`); a class declared inside it
may not name it when it is private (both checked in the review of stage 6).

Getters and setters have their own modifiers; a getter's must equal its
field's (7.17). An enum value has its enum's access.

`leniency_severity(code)` (section 5) is the one place that says `W1022` is a
warning; the check itself is written once, for every kind of member.

**Static against instance.**

| use | from | result |
|---|---|---|
| instance field or function by plain name | a `static` function, a field value of a static field, a lambda in static code | `E0703` / `E0704` (help: "make `f` static, or call it on an object") |
| instance member through a type name `point.x` | anywhere | `E0703` / `E0704` |
| static member through an object `p.count` | anywhere | allowed; for a `static localized` field `W1012` (C++: "accessing static localized field through instance") |
| `self`, `base` | static code | `E0421` |

Everything declared in a module (not in a class) is static; `static` written
there is `W1007`. Aliases and `native` functions are static by default too.

### 7.10 Assignment and `const`

`target = value` (`assign_expression.cpp`). What the target is decides what
the assignment means:

| target | `=` | compound (`+= -= *= /= %= &= \|= ^=`; there is no `**=`, `<<=`, `>>=`) |
|---|---|---|
| local, parameter | value converts to its type | numbers: as the binary operator, result converted back to the target's type; a class: its `operator+=` function (`E0712` when it has none) |
| field | the same | the same |
| array element | the same | the same |
| property with a setter (7.17) | the setter is called with the converted value | getter, operator, setter |
| property with a getter only | the value is stored in the field directly - `g.value = 50;` works from anywhere (`properties_getter_only_assignment.sharp` in the suite) | getter, operator, store. (The C++ code generator stops with a fatal error on this form; the meaning follows from the two rows above.) |
| object index `o[i]` | `o.put(i, value)`, or `operator=` on what `operator[]` returns (7.8.3) | `operator+=` on what `operator[]` returns |
| a class-typed target and a value that only fits through the class | 7.5.4: `operator=`, or construct when the target may be null / is an array element | |
| an OBJECT that an expression gave back, whose class has an `operator=` that takes the value: `title() = "x";` | that `operator=` is called on the object (a `string` changes its contents - checked by running it with the C++ compiler) | its `operator+=`, as for any object |
| anything else: a call, a literal, `self`, a type, an enum value, the name of a function, `a + b`, a `?.` chain **[D58]**, `(a ?: b)` **[D59]** | `E0711` "cannot be assigned to" | `E0711` |

Only something WRITTEN as a place is one - a name, `a.b`, `a[i]`, `module#name`,
in parentheses or not (`(count) = 2;` is accepted by both compilers).
`(count ?: 1) = 2;` is a value although analysing `count ?: 1` gives back the
variable `count` when it can never be null.

The value of an assignment expression is the target's new value (type: the
target's): `x := (a.count = 5);` (checked).

**`const`** **[D7: the C++ compiler does not enforce it at all]**

| declared `const` | may be assigned |
|---|---|
| local | by its declaration only |
| field with a value | never again |
| field without a value | in a constructor or `init` block of its own class (instance field), in a `static init` block (static field) |
| global | by its declaration only |

Any other assignment is `W1021` ("`limit` is `const`"), with the declaration
shown; `++`, `--` and compound assignment count. It is a warning, not an
error, in this build (section 5); the assignment is analysed and bound like
any other. `const` says the VARIABLE is not assigned again; the object it
refers to can still change. Two things ARE errors because they have no
meaning: assigning to an enum value (`E0711`: it is not a variable), and a
setter on a `const` field (`E0375`, as in the C++ compiler).

### 7.11 Null safety

Types are non-null unless written `t?` (7.1). There is NO flow analysis for
null: after `if(x != null)` `x` is still `t?` and needs `x!!` (as in the C++
compiler; decision 5, section 11).

What the C++ compiler really checks is narrower than it looks, and each row
below was probed one mistake per file (see section 5 for why that matters).
The rule: giving a `t?` to something that wants a `t`, and reaching INTO a
`t?` object with a plain `.`, are errors; everything that merely USES a value
that may be null is left to the run time, which raises `nullptr_exception`.

| form | on a nullable `t?` | on a non-nullable `t` |
|---|---|---|
| `x.member`, `x.f()` - also after a call, `maybe().name`, and with an implicit `self`, `other.show()` where `other` is a nullable field | `E0722` ("`x` may be null": help `x?.member`, `x!!.member`) | fine |
| assigned / passed / returned to a `t` | `E0721` | fine |
| `x(args)` where `x` is a function value | allowed (`f : (_int64)(_int64)? = ...; f(1)` compiles) | fine |
| `x[i]` on an array that may be null | allowed (`non_null_index.sharp`) | fine |
| `x + y`, `-x` ... : an operator function called on `x` | allowed | fine |
| `foreach(i in x)`, `lock(x)`, `throw x`, `sizeof(x)`, `x is t`, `x as t` | allowed | fine |
| `x?.member`, `x?.f()` | the member is read only when `x` is not null; otherwise the WHOLE chain to its end gives null: in `a?.b.c.d()` nothing after `?.` needs another `?.` on account of `a` (checked: `i.other?.other.weight` wants nothing more for the first `other`; the library writes `base_field?.type.type_class?.get_field(...)`). Result type: the chain's, made nullable. A chain that ends in a plain number: `E0725` **[differs D24]** | `W1005` "unnecessary `?.`" |
| `x!!.member`, `x!!` | asserts: type `t`; null raises an exception at run time | `W1004` "redundant `!!`"; on a plain number `E0723` |
| `x ?: fallback` (`elvis_expression.cpp`) | `fallback` must convert to `x`'s type (construction allowed), else `E0724`. Result: `x`'s type, non-null when `fallback` is non-null | `W1006`; the result is `x` |
| `x == null`, `x != null` | `_int8` | allowed (always false / true) |

In the typed tree `x?.rest` is a `bound_safe_navigation` (6.4); `x!!` a
`bound_not_null`.

Elements of an object array start out null although the array's type says
they are not (`string[]`): reading one gives the item type as declared (kept as
in the C++ compiler). Declare the array `t[]??` to have the compiler insist:
an item read from it is a `t?`, and so is a `foreach` variable over it that
has no written type **[differs D25]** (decision 8, section 11).

### 7.12 Statements

#### 7.12.1 Conditions

`if`, `else if`, `while`, `do while`, the middle of `for`, a `when` test: a
plain number (0 is false; a decimal is compared with 0), or a non-null
`bool` OBJECT, whose `value` is taken. Of the number classes ONLY `bool` is
accepted here: `if(an_int)` and `while(a_double)` are errors in the C++
compiler too (`compile_cond_expression` asks for `std#bool` by name; probed
in stage 6). The condition of `? :` is wider: any number object (7.5.6).
Anything else - another object, an array, text, null, nil - is `E0411`
(help: `x != null`, `sizeof(x) > 0`, `an_int.value != 0`).

A `bool?` - a bool object that may be null - is accepted in a statement's
condition, because the C++ compiler accepts it and the program runs while
the object is there (D66): it is read as `flag!!` (a `bound_not_null` in the
typed tree, then the unboxing), and `W1028` says that null throws there and
how to write the test (`flag!!`, or `flag != null && flag!!`).

#### 7.12.2 Each statement

| statement | requires | errors |
|---|---|---|
| `{ }` | a new scope | |
| `name : t = v;` `name := v;` | 7.3; no name clash (7.4.2) - also for the variable of a `for`, a `foreach` and a `catch` (checked: "local field is already defined"); modifiers only `const`, `excuse`, `localized` (others `E0360`, stage 5); `inject` 7.20: an injected local has a written type and no value | `E0600`, `E0410`, `E0406`, `E0617` (an injected local given a value), `E0618` (neither type nor value) |
| `alias t as name;` | a type; the name is visible to the end of the block | `E0600` |
| expression`;` | any expression; a call, assignment, `++` or `new` is expected - `a + b;` and a bare name `pick;` compile silently in the C++ compiler and are `W1018` "has no effect" here (for a function name: "did you mean `pick()`?") | |
| `if` / `else` | condition | `E0411` |
| `while`, `do while` | condition; a loop for `break` / `continue` | `E0411` |
| `for(init; cond; step)` | `init`: declarations (visible in the whole `for`) or an expression; `cond` optional; `for(;;)` is endless. A start expression or a step that changes nothing (`for(...; ...; i + 1)`) is `W1018`, as for an expression statement | `E0411` |
| `foreach(x in e)`, `foreach(x: t in e)` (`for_each_statement.cpp`) | `e` an array, or an object whose class implements `std#loopable<t>`: then `e.get_elements()` (an instance function returning an array) is looped over. `x` without a type: the item type (a `t?` over a `t[]??`, D25). `x: t`: the item must convert to `t` (construction allowed; an `object` item may go to a class variable, checked at run time); `t` not an array. `e` is evaluated once; it may be a nullable array (7.11) | `E0609` not iterable, `E0610` item does not fit |
| `return;` `return e;` | 7.13 | `E0606`, `E0607`, `E0410`, `E0604` |
| `throw e;` | ONE object of a class that is `std#throwable` or below (it may be a `t?`, 7.11); not an array of them (D67) | `E0612` |
| `break;` `continue;` | inside a loop of the SAME function (not through a lambda: `run({ -> break; })` inside a loop makes the C++ code generator stop with a fatal error, so there is no meaning to keep); not out of a `finally`. A `when` is not a loop: `break` in a clause leaves the loop around the `when`. Outside every loop the C++ compiler crashes (D61) | `E0602`, `E0603`, `E0604` |
| `lock(e) { }` | a class value, `object`, an array or a function value (nullable or not; a function value is an object when the program runs, and the C++ compiler accepts it - D74); not a number or `null`. `e` is evaluated once (the C++ compiler evaluates it again to unlock, and refuses an array held in a variable: D62) | `E0615` |
| `try { } catch(e: t) { } finally { }` (`try_catch_statement.cpp`) | at least one `catch` or a `finally` (the parser already insists: `E0100`); each `t` a class (not an array) that is `throwable` or below; no class caught twice; a clause for a class that an EARLIER clause already covers is `W1019`; the catch variable is a local of the clause | `E0613`, `E0614` |
| `when(e) { a, b -> s  else -> s }` (`when_statement.cpp`) | with a subject: each value is compared with it by `==` exactly as the binary operator would (operator overload included); the result must be a condition. Without a subject: each value is a condition. The first clause that fits runs; no fall-through. The same constant (or enum value) twice: `W1020`. The subject is any value (`E0406` when it has none); a `bool` object subject is taken as its number and a text literal becomes a `string`, as on the left of `==`. The `else` clause is the last clause and there is one at most (D68) | `E0616`, `E0411`, `E0619` |
| `asm { }` | 7.21 | |
| `;` | nothing | |

Nothing may jump out of a `finally` block: `return`, `break`, `continue` in
one are `E0604` (C++: "control cannot leave body of finally clause"). `throw`
is allowed, and so is a `break` / `continue` whose loop is itself inside the
finally block: it does not leave it (D65).

### 7.13 Return and reachability **[differs D12]**

**What a function returns** (stage 5 knows it, except the inferred case):

| declared | result type | `return` |
|---|---|---|
| `: t` | `t` | `return e;` with `e` converting to `t` (construction allowed; `E0410`, or `E0721` when only the `?` is in the way). `return;` is `E0607` |
| `: nil`, or nothing | nil | `return;` only. `return e;` is `E0606` |
| `:= e;` (no type) | inferred from `e` by the table of 7.3 | - |
| `: t = e;` | `t`; `e` must convert | - |
| constructor, `init`, setter | nil | `return;` only |
| getter | the field's type | as `: t` |
| lambda | 7.14 | |

**Reachability** is a textbook flow analysis over the typed tree
(`functions/reachability.sharp`). For every statement: can execution continue
after it (`completes`)?

| statement | completes |
|---|---|
| `return`, `throw`, `break`, `continue` | no |
| expression, declaration, empty, `asm` | yes |
| block | when each statement in it does; a statement after one that does not is UNREACHABLE: `W1002`, reported for the first such statement of a block only |
| `if` without `else` | yes |
| `if` / `else` | when either part does |
| `while(c)`, `for(;c;)` | yes - unless `c` is absent or the constant true AND no `break` leaves the loop |
| `do { } while(c)` | when the body does (or a `continue` reaches the condition) and `c` is not constant true; or a `break` leaves it |
| `foreach` | yes |
| `lock` | as its body |
| `try` | (the body or any `catch` completes) and the `finally`, if any, completes |
| `when` without `else` | yes |
| `when` with `else` | when any clause or the `else` does |

A function whose result is not nil and whose body COMPLETES has a path without
a `return`: `E0605` ("not every path returns a value"), marked at the closing
brace, with the note saying which statement lets execution through (an `if`
without `else`, a `when` without `else`, a loop that can end). Only literal
constants count for "constant true"; `if(true)` is not special.

As built (10.4) the analysis is a pass of its own over the finished typed
tree (`backend/compiler/flow`). Three details:

- A `break` or `continue` makes its loop end / reach its condition only when
  it can itself be reached: `while(true) { return 1; break; }` never ends.
- A jump reported as wrong (`E0602` - `E0604`, `bound_return.broken`) is
  treated as going on, so one mistake gives one message.
- `W1002` is not given for the `return` that is the last statement of a
  function body right after an endless loop (`while(true) { ... } return 0;`):
  the C++ compiler refuses the function without it (D12), so a program that
  compiles with both cannot remove it. Everywhere else - after an `if`, `try`
  or `when` whose every way ends in a jump, which the C++ compiler sees too -
  the dead `return` gets its warning.

### 7.14 Lambdas and function values (`lambda_expression.cpp`, `closure_capture.h`)

**Forms.**

```
{ a: int, b: int -> (int) return a + b; }    parameters with types, result type, statements
{ a, b -> return a.value + b.value; }        untyped: types come from where it is used
{ -> work(); }                               no parameters
xs.each { println(it); }                     no `->` at all, after a call: one parameter `it` if the body uses `it`, none otherwise
xs.map { it * 2 }                            a body that is ONE expression without `;`: its value is the result
```

(How to tell the last two from `{ -> ... }` in the tree: the token after the
lambda's `{` is not `->` and the `N_PARAMETERS` node has no children. The
single-expression body is an `N_EXPRESSION_STATEMENT` that is the only
statement and has no `;` before the `}`; it becomes a
`bound_expression_statement` with `is_result = 1`. The C++ parser makes this a
hidden `return` only for the no-arrow form; the bootstrap accepts it for any
lambda. When the lambda returns nothing the expression is just evaluated.)

**Typing.** A lambda is analysed against the function type EXPECTED at the
place it stands - a parameter (after overload resolution, 7.6), a declared
variable or field, a `return`, an array item:

| | with an expected type `(p...)(r)` | without (`f := { ... };`) |
|---|---|---|
| number of parameters | must be equal (`E0801`); `it` exists only when the expected type has exactly one (`E0809`) | as written |
| a parameter with a type | must be the same type as expected (`E0802`) | as written |
| a parameter without a type | gets the expected type | `E0800` ("give `a` a type, or give the variable a function type") |
| result | written `(t)`: must be the same as `r` (`E0803`; as an argument of a call `W1029`, see below). Not written: `r` | written, or inferred from the `return` statements: none / all without a value: nil; otherwise the FIRST `return` with a value decides (a text literal makes it a `string`, `null` an `object`) and every other value is converted to it (`E0410`; a `return;` then is `E0607`) **[differs D11]**. A body of one expression without `;` gives the expression's type |

A lambda with every parameter typed and a written result does not need an
expected type and is checked against it like any other value (function types
must be the same, 7.5.1).

The no-arrow form (`{ println(it); }`) has no parameter list of its own, so
its parameters come entirely from the expected type: none when the expected
type has none; one hidden parameter named `it` (`local_variable.is_hidden`)
when it has one - whether or not the body uses it; more than one is `E0801`
("write the parameters: `{ a, b -> ... }`"). The lookup of the name `it`
(7.4.1 step 3) finds that parameter; in a lambda that has none, `it` is an
ordinary unknown name with the help "`it` exists only in a lambda that takes
exactly one argument" (`E0809`). Without an expected type the no-arrow form
is `E0800`.

A lambda may stand as a trailing argument (7.7.1), as an ordinary argument,
as a value in a declaration, an assignment, a `return`, an array item, a
`single` / `factory` definition, and as an operand of `==` / `!=`. Anywhere
else (the operand of `+`, a condition, the object of a `.`) it is `E0808`
("a lambda cannot be used here").

For overload resolution a lambda whose types are not all written WAITS: it
fits a function-typed parameter with as many parameters as it is written
with (the form without `->`: none or one) whose parameter types are the ones
written, where any are; the body is analysed against the parameter once the
winner is known (`describe_waiting_lambda`, `finish_waiting_argument` in
`backend/compiler/lambdas/lambdas.sharp`). A function NAME passed as an
argument waits the same way and fits when one of its overloads has the
parameters. A lambda with every type written is analysed at once and takes
part like any value. When several candidates remain that differ only in the
function type they want from a waiting lambda, the FIRST of them wins, as in
the C++ compiler, where an untyped lambda is a "direct match" for every such
parameter (`is_explicit_type_match` in `sharp_type.cpp`); the first plan's
`E0503` would have refused calls the C++ compiler resolves. (Checked against
the library and the suite: no call there depends on it.)

**The result of a function passed as an ARGUMENT is not compared**
**[differs D69]**. The C++ compiler compares only the parameters of two
function types when it matches an argument - `new thread("w", worker)` with
`worker` returning a `decimal` where `(object[])(int)` is expected, and
`vm.start_fiber(..., fiber_main)` with `fiber_main` returning nothing, both
compile, and the suite and the library rely on them (checked). The bootstrap
accepts them with `W1029` and the conversion `CONV_RESULT_IGNORED` (nothing
changes at run time). In a declaration or an assignment the result IS
compared, in both compilers (`k : (decimal)(decimal) = gives_text;` is
`E0410`), and a lambda's written result against the expected type there is
`E0803`.

**Body.** A lambda is a static function of the class it is written in. Inside:

- its own parameters and locals;
- locals and parameters of the enclosing function(s): CAPTURED, any type
  **[differs D10]**, read and written through the closure record (4.3). An
  assignment in the lambda is seen by the enclosing function and the other way
  round;
- `self`, fields and functions of the object: allowed when the function the
  lambda is written in (outermost) is an instance function - `self` is
  captured. In static code: `E0703` / `E0421`;
- `return` returns from the LAMBDA; `break` / `continue` cannot leave it
  (`E0602` / `E0603`);
- the type parameters of a generic function or class around it.

**A function's name as a value** (`resolve_primary_class_function_address`):
`f := twice;`, `run(worker)`. The function must be global or `static`
(`E0804`); not a generic function (`E0805`). Its type is its parameters and
result. With several overloads: the one whose function type is the same as
the expected type; without an expected type the first declared, with `W1011`
("`twice` has 2 overloads; `twice(_int64)` was taken"). A name is tried as a
function value LAST (7.4.1 step 6), so a variable or class of the same name
wins.

### 7.15 Constructors, `init`, base calls

#### 7.15.1 Declaring

A constructor is named like its class (for a generic class, the name without
arguments) - `E0370` otherwise. Overloads by parameter types. No result. When
a class has no constructor without parameters one is added (public, empty) -
see 7.7.5. An interface declares functions WITHOUT a body only (and aliases):
a field, a constructor, an `init`, a function with a body or a class inside
one is `E0379` (the C++ parser refuses each of them - "unexpected variable
declaration", "unexpected method declaration", "expected delegate
declaration"; the bootstrap parser lets them through, so stage 5 reports them
and says what to write instead) **[D29, D42]**. A function added to an
interface from outside (`def printable.twice() { }`) has a body by nature and
is allowed.

#### 7.15.2 What runs when `new C(args)` is evaluated, in order (`function_compiler.cpp`, `init_compiler.cpp`)

1. the BASE constructor: the one named by `-> base(args)` - overload resolution
   among the constructors of the class directly above, exactly as for `new`
   (`expressions/calls/base_constructor_call.sharp`, bound by the calls stage);
   `E0509` when none fits. A class that names no base class has `_object_`
   above it, whose only constructor takes nothing: `-> base()` is fine there,
   `-> base(5)` is `E0509` with the help to remove it (checked: C++ "could not
   resolve base class constructor in class `_object_`"). When nothing is
   written, the base's constructor without parameters runs;
2. once per class, before its first object or static use: the static part -
   the enum values, the static field values, then the `static init { }`
   blocks in order (`sharp_class.static_initializers`);
3. the class's instance initialisation: the values of its instance fields
   (and the default objects of `count : int;` fields), then its `init { }`
   blocks in the order written (all of them form one function;
   `sharp_class.instance_initializers`);
4. the constructor's own body.

Field values - instance, static and global alike - run in an order in which
each comes after the values it reads directly, the written order wherever
that says nothing **[differs D70]**: the C++ compiler runs them as written,
so `a := b + 1; b := 5;` reads `b` before it has its object and stops with a
null-pointer exception (checked with two globals). Values that read each
other in a circle are placed as ONE unit - after everything they read from
outside the circle - and keep the written order among themselves, as the
C++ compiler runs them, with `W1031` (the message lists the circle: "`x`
reads `y`, `y` reads `x`", and stands at the member written first, which
finds the others not yet set). A value that reads its own field
(`z : _int64 = z + 1;`) is `W1031` too. A lambda in a value reads nothing yet:
its body runs when it is called. An injected field (7.20) stands in the same
lists.

There is no `-> self(args)`: one constructor cannot hand over to another.
The arguments of `-> base(...)` are analysed in the constructor's scope, as
instance code: its parameters are visible, and so are `self` and the fields
of the object. They run BEFORE step 3, so a field read there still holds its
start value - checked: with `w : _int64 = 4;` and `-> base(n + w)`, the base
constructor receives `n + 0`. The bootstrap accepts this like the C++ compiler
and adds `W1024` when an argument reads an instance field of the class itself
("`w` has not been given its value yet when the base constructor runs").

A class whose base class has only constructors WITH parameters may still omit
`-> base(...)`: every class has the added constructor without parameters
(7.7.5), and that is the one called (checked).

#### 7.15.3 `init` forms

| form | meaning | rules |
|---|---|---|
| `init { }` | part of step 3 | no parameters, no modifiers but `static` |
| `static init { }` | part of step 2 | no parameters (`E0372`) |
| `init(x: t) { }` | a CONVERSION function: `c : C = value_of_t;` (7.5.4). Runs steps 1 to 3, then its body, like a constructor | exactly one parameter (`E0371`); not `static` (`E0372`); may have `-> base(args)` |

#### 7.15.4 A field must have a value (`initialization_validator.cpp`) - stage 10

**[differs D82: a warning, `W1032`, off unless -wextra.]** The C++
compiler's check never fires in practice (every probe compiled), so a program
that leaves such a field without a value is a program the C++ compiler builds
and runs; refusing it would be stricter (section 5). The rule below is the
C++ compiler's, as `backend/compiler/program/required_values.sharp` applies
it; `E0960` is retired with its number kept.

A field whose type is a class, `object`, a function type or an array, and is
NOT nullable, must be given a value before it is read: `W1032` unless one of
these holds -

- it has a value where it is declared, or is injected (7.20);
- it is marked `excuse` ("I know; trust me");
- its class is a box or `string` (it gets a default object, 7.3 - unless it is
  written with a type parameter, `value : T;`, which starts null in every
  instance: codegen.md 13.3);
- instance field: EVERY constructor of the class that can run assigns it
  before reading it - the first use of the field in the order the code is
  written (branches included, as in the C++ compiler: `if(x) { f = ...; }`
  counts) is an assignment - or an `init { }` block does. The constructor the
  compiler adds counts only when some code calls it;
- static field: a `static init` block assigns it before reading it;
- global: none of the above apply; it needs a value or `excuse`.

Number fields start at 0 and nullable fields at null; they need nothing. A
field with a getter or a setter is a property and is not asked about. This
is a simple form of "definite assignment" on the typed tree of the
constructors (the first use in written order, not every path).

### 7.16 Enums (`enum_compiler.cpp`)

`enum color { red, green = 5, blue }`. An enum is a class below `std#_enum_`
(stable, cannot be mutated or inherited from, `E0322`); its values are public
static constants of the enum's type. An enum has at least one value (`E0367`,
D42). Numbering **[D15]**:

- a value written `= e`: `e` must be a plain NUMBER expression (`E0940` for
  anything else; a decimal is cut to an integer). When `e` is a constant the
  number is known while compiling (`sharp_enum_value.constant`); when it is
  not, the C++ compiler evaluates it when the program starts, and so does the
  bootstrap (`bound_value` holds the expression, `constant` stays null);
- a value after one with a known number is that number plus 1: with
  `green = 5`, `blue` is 6 (checked);
- any other value - before the first `=`, or after a `= e` that is not a
  constant - has no number the program may rely on, only one that differs
  from the other values of its enum (in the C++ compiler `red` was 16612 in
  one probe, and the first value of the next enum 16614). `constant` is null;
  the code generator numbers them.
- A constant is what folding gives (literals, `+ - * / ...` on them), and a
  `const` field whose value is a constant counts as one (`= top + 1` with
  `const top : _int64 = 10;` is 11): reading such a field carries its value
  (`bound_field.constant`, `field_access.sharp`). Checked with the C++
  compiler: `= "text"` and `= shade.light` are refused ("expected numeric
  type for enum field"), `= limit * 2` with a plain global compiles. Two
  values with the same number are accepted there; here they get `W1030`
  (they compare equal).

Each value is an OBJECT, built when the program starts by
`std#_enum_(name: _int8[], ordinal: decimal)` (`well_known.enum_constructor`)
and stored in a static field of the enum's type; the initialisation is part of
the enum's `static_initializers` (4.2). Its number is read as
`color.blue.ordinal` (a `decimal` property with a private setter) and its
name as `.name`; it does not convert to a number implicitly
(`i : _int64 = color.red;` is `E0410`, help: `.ordinal`).

Values are compared with `==` / `!=` - these are the operator functions of
`_enum_` (`operator==(e: _enum_) : bool`, `operator==(num: int) : bool`), so
the result is a `bool` object - and used in `when` (`when(c) { color.red ->
... }` compares the same way). Enum values cannot be assigned (`E0711`).
`new color()` is `E0507` **[D26]**.

### 7.17 Getters and setters (`field_processor.cpp`, `assign_expression.cpp`)

```
width : decimal;
    get { return width * scale; }
    set value -> { width = value / scale; }
```

- only on fields of a class or globals, not on locals;
- the getter is a function without parameters returning the field's type; the
  setter a function with one parameter of the field's type returning nothing.
  The setter's parameter has no written type (it is the field's);
- INSIDE its own getter or setter the field's name means the stored value;
  everywhere else, the class's other functions included, reading the field
  calls the getter and assigning it calls the setter; `+=`, `++` call both.
  Run to be sure: with `get { return value + 1; }` and a setter that stores,
  `c.value = 10;` then `c.value` is 11 - also from a function of the class -
  and `c.value += 1;` leaves 13 to be read (get 11, add, set 12, get 13);
- a field with a getter and NO setter is assigned directly, from anywhere
  (7.10); a field with a setter and no getter is read directly;
- in the typed tree a read through a getter and a `=` through a setter are
  `bound_call`s (`AS_GETTER`, `AS_SETTER`); only inside the accessor itself is
  the field a `bound_field`;
- a getter's modifiers must be the field's (`E0377`); a setter may have its
  own (a public field with a private setter);
- one getter and one setter per field: a second is `E0369` (D42);
- no setter on a `const` field (`E0375`); neither on a `localized` field
  (`E0376`);
- a setter is not used when the value only fits through `operator=` of the
  field's class (`E0713`; C++: "Conflicting assignment ... Cannot assign value
  via setter when `operator=` function is required").

### 7.18 Extension functions and `mutate`

`def string.shout() : string { ... }` (`function_processor.cpp`,
`process_extension_class`) declares, outside the class, a function that
BECOMES a member of the class: an instance function with `self`, found by
member lookup like any other, subject to the same duplicate rule (`E0331`).
The type in front must be a class (`E0373`); the function must have a body
(`E0374`); `def list<T>.first() : T` extends the generic class - every
instance gets it. The type parameters may be given names of the function's
own - `def box<X>.first() : X` is the same function, `X` being box's `T`
(names/renamed_type_parameters.sharp) - but they are all written, in order:
`def pair<B, A>.f()` and `def pair<string, B>.f()` for `class pair<A, B>` are
neither every pair nor one instance (`E0384`); `def box<string>.f()`, with
real types, is a function of that one instance. `def t.operator+(...)` adds an
operator. It sees the class's private members (it is a member). A function
that is refused (`E0331`, `E0384`) stays tied to its class and its body is not
analysed until it is put right - analysed as a global function, every `self`
and field in it would be a message about the one mistake. (The C++ PARSER fails on these forms -
`parser_differences.md` - so no library code uses them; the rule is taken from
the C++ backend.)

`mutate t { members }` (stage 3; `mutation_processor.cpp`) adds members, a
base class or interfaces to an existing class. Rules: `t` must be a class
(`E0380`) and not `stable` (`E0381`); a generic class is mutated as a
blueprint (D16) - written `mutate box { }` or `mutate box<T> { }`, never with
other type arguments (`E0383`, D40); what is added must not clash with what
is there (`E0330` ... `E0332`). A class marked `ext` cannot be instantiated (`E0507`).

### 7.19 Generics

Section 4.1 has the mechanism. The rules:

- `class c<T, U base x>`: `U` must be `x` or below it for every use (`E0351`).
  "Below" is exact, as in the C++ compiler: `x?` and `x[]` are not an `x`
  (the class uses its `U` as an `x` without asking whether it is null);
- a type parameter may be the base class or an interface of the class:
  `class mixin<T> base T`, `class tagged<T> : T` (4.1). What `T` may then be
  is checked per instance (`E0320`-`E0324`);
- the same generic interface may be listed more than once with different
  arguments (`: comparable<int>, comparable<string>`): two interfaces, two
  functions to define. The same arguments twice is `E0325` - also when it
  only happens in one instance (`both<int, int>`);
- two functions of a generic class that differ only by type parameters
  (`put(a: A)`, `put(b: B)`) are fine until an instance makes them the same
  (`E0331`, `E0332` for constructors; reported for that instance);
- a class declared inside a generic class is named from inside it only
  (`E0368`, 4.1);
- a use names every argument: `list<int>`; `list` alone where a type is needed
  is `E0354`, `list<int, int>` is `E0350`, `string<int>` is `E0353`;
- an argument may be any type, numbers included (`box<_int64>`). `T?` inside
  the class is then `_int64?`, which is `E0355` - reported at the use, with
  the note "while checking `box<_int64>`"; library classes that need `T?`
  constrain `T base _object_`;
- instances are unrelated classes (7.5.3);
- inside the class its own name without arguments means "this instance":
  in `class gen<T>`, `def me() : gen { return self; }` is `gen<T>`. (The C++
  compiler accepts the declaration and then makes the name the BLUEPRINT,
  which no value converts to - "returning `gen<string>` from a function
  returning `gen<T>`" - so nothing that compiles there changes meaning);
- a type argument may be a plain number type, and the library does it:
  `std#number<_int32, std#int>`. Inside such an instance `T` IS `_int32`: a
  field `value : T` is a number field, `value > other.value` is number
  arithmetic, `T?` is `E0355`;
- static members exist per instance: `box<int>.count` and `box<string>.count`
  are two fields;
- generic functions: `def f<T>(...)`; the call writes the arguments,
  `f<int>(x)` (`E0517` when they are missing, `E0515` wrong number, `E0516`
  on a function that is not generic); two generic functions of one name in one
  place must differ in the NUMBER of type parameters (`E0358`); a generic
  function cannot be used as a value (`E0805`), and it cannot be `override`
  or be left without a body in a class (`E0345`, **[differs D39]**): which
  function runs for an overridden function is decided at run time from one
  table entry per function, and a generic function is a new function for
  every `f<...>(...)` that is written. A body-less generic function is also
  not something a class below is asked to define (it is skipped by `E0341`).

### 7.20 Components, `inject`, `get` (`di/component_compiler.cpp`, `get_expression.cpp`, `component_manager.cpp`) - stage 9

```
component app {
    single { new database("main") }             one object, made on first use
    factory("worker") { new worker(get()) }     a new object every time, with a name
}
inject db : database;                           a field filled from a component
w := get<worker>("worker", app);
```

**Components and definitions.**

- a component has a name; a block without one adds to the MAIN component and
  may be repeated. A NAMED component is declared once in the whole program
  (`E0900`; component names are global, as in the C++ compiler);
- each `single` / `factory` is a DEFINITION: an expression, optionally with a
  name. The expression is analysed like the value of a global (no object, no
  locals); its type - worked out as for `x := value;`, so `{ 8080 }` is an
  `int` and `{ "text" }` a `string` (the C++ compiler: an `_int8[]`, D96) - is what the definition PROVIDES. An
  expression without a value (a call that returns nothing, `null`) is
  `E0905`. A lambda there is typed like any lambda with nothing expected
  (`E0800` when a parameter has no type: the plan's `E0907` is not used);
- named and unnamed definitions are two lists: a request with a name looks
  only at the named ones, a request without one only at the unnamed ones.
  Two unnamed definitions of one component that provide the same type, or
  two with the same name: `E0901` (the blocks of one component count
  together);
- "provides a `database`" is EXACT: the same type; a request for `database?`
  accepts a `database`. A definition of a class below `database` does not
  answer a request for `database`.

**`get`.**

| written | finds |
|---|---|
| `get<t>()` | an unnamed definition that provides `t` |
| `get<t>("name")` | the definition named "name"; it must provide `t` (`E0904`) |
| `get<t>(component)` | an unnamed definition of that component |
| `get("name", component)` | the definition of that name there; the type is what it provides. With a type as well: `W1014` (it is checked, not needed). As the whole value of `x := ...` the C++ compiler refuses it ("cannot resolve type from get() expression"); accepted here (D101), and W1014's help there writes `x : t = get(...)` |
| `get()`, `get(component)` - only INSIDE a definition | an unnamed definition that provides the type EXPECTED where the `get` stands: the parameter it is passed to |
| `get("name")` - only inside a definition | the definition of that name |

- outside a component a `get` names the type, or both name and component
  (`E0902`, as in the C++ compiler); inside a definition `get()` with nothing
  expected either (`single { get() }`) is `E0902` too;
- WHERE it looks: in the component it names (`E0909` when there is none of
  that name); else first in the component the request is written in, then in
  every component in the order declared. The first definition found is the
  answer; when another component answers as well and none was named: `W1013`;
- nothing answers: `E0903`. The message looks at what exists: the type under
  a name ("ask for it by name: `get<worker>("fast")`"), the right definition
  in another component, a name one slip away, or the list of what the
  components provide;
- a `get()` passed as an ARGUMENT waits for its parameter like an untyped
  lambda (7.6): it fits a parameter whose type a definition in reach
  provides, so `new worker(get())` picks the constructor the components can
  serve. (A definition that cannot be asked yet - one being analysed - lets
  the argument through; a candidate that needs that benefit of the doubt
  loses to one that does not.) Among several that fit, the first wins;
- definitions are analysed on demand, so they may be written in any order.
  Definitions that need each other in a circle - `single { new engine(get()) }`
  where an engine takes a car and the car's definition takes an engine, or a
  definition that asks for what it makes itself - are `E0908`, with the chain.
  A `get` written inside a lambda of a definition runs when the lambda is
  called and is not part of the circle (unless it asks for its own definition).

**`inject`.** `inject x : t;` / `inject(component) x : t;` on a field, a
global or a local: the value is what `get<t>()` / `get<t>(component)` gives -
found by type among the unnamed definitions; an `inject` cannot ask by name.
It cannot also have a value (`E0378` fields, `E0617` locals). Nothing
provides it: `E0906` (`E0909` for an unknown component, `W1013` as for
`get`). The field is given its value when the object (a static field: the
class; a global: the program) is set up - it stands in the initialisation
lists (7.15.2); a local when its statement is reached.

**[differs D75]** In the C++ compiler a search through ALL components
returns what the LAST component has - `get_type_definition(manager, type,
ast)` in `component_manager.cpp` overwrites its result in every round of its
loop - so `get<database>()` and `inject db : database;` fail with "No
defined type found" unless the database is defined in the component declared
last (checked). The bootstrap takes the first definition found.

### 7.21 `asm` blocks (`asm_statement.cpp`)

The parser has already checked each instruction's name and the number and
kind of its operands. Analysis resolves the operands that are names:

| operand | must be | error |
|---|---|---|
| `[name]` | looked up like a plain name (7.4.1): a local or parameter, else a field or function (its address) of the class or a class above it, else a global variable or function of the modules the file sees | `E0920` |
| `$type` | a class (a name that is no type at all is `E0301` from the type-name stage, which visits the types written in asm blocks too) | `E0923` (a type that is not a class: `$_int64`) |
| `$type[member]` | a field or function of that class or a class above it | `E0923` |
| `$module#__srt_global[name]` | a global variable (its slot) or global function (its address) of that module: `__srt_global` is the C++ compiler's name for what a module declares outside any class; the library uses it for the thread entry point (`platform/kernel/vm.sharp`). The function is then reached from main like a called one | `E0923` (no such global; `$m#__srt_global` alone) |
| `.name:` | a label, once per block | `E0921` |
| `@name` | a label of the same block | `E0922` |

An operand that names an OVERLOADED function (`[h]` with `h(_int64)` and
`h(string)`) is the address of the first declared, as in the C++ compiler,
with `W1042` saying which (the C++ compiler also reads `[h + 1]` as the second;
the bootstrap's parser has no offsets). A GENERIC function has no address
(`E0805`, as for one used as a value; the C++ compiler: "attempt to get
address of a member ... when none were found"); beside overloads that are
not generic it is passed over. `$__srt_global[name]` without a module is the
globals of the module the code is in, as in the C++ compiler. Outside an asm
operand `__srt_global` is not a type (`E0301` / `E0304`).

An `asm` block completes normally for reachability, and reads / writes of
locals inside it are not tracked. Inside a LAMBDA, `[name]` may name the
lambda's own locals and parameters only: a local of the function around the
lambda is not in the lambda's stack frame (it is reached through the closure
record, which asm does not do) - `E0924`, with the help to copy it into a
local of the lambda first.

In the function ITSELF, `[name]` may name only a local that lives in the
stack frame. Two kinds do not: a local a lambda of the function captures
(its only home is the closure record, 4.3) and a `localized` local (it lives
in its fiber). Either is `E0925`, with the same help: copy it into a plain
local and name that. The asm block and the lambda may come in either order:
the operand remembers the first `[name]` on the variable
(`local_variable.asm_operand`), and a capture met later reports it at that
place (`note_local_capture`, context/closures.sharp).

### 7.22 String templates

`"total: ${a + b} for $name"` is ONE value of type `string`
(`N_STRING_TEMPLATE`: text pieces and expressions alternating). It means
`"total: " + (a + b) + " for " + name`, built left to right starting from a
`string`: each embedded expression must be something `string.operator+`
accepts (7.8.1; `E0423`: "`shape` cannot be put into a string", help: "call a
function that returns its text"). A value of type nil is `E0406`.

This is exactly what the C++ compiler does, one step earlier: its TOKENIZER
rewrites the string into `"total: " + (a + b) + " for " + name` before the
parser sees it. So the overload of `string.operator+` picked for each piece
is the one a hand-written `+` chain would pick: a number is appended as text
by its own overload (an `_int8` as a CHARACTER), a `string`, an `_int8[]` and
any `printable` object by theirs. Every class is `printable` through
`_object_` (checked: `"${c}"` with a plain class compiles), so in practice
`E0423` is for function values and for numbers arrays other than `_int8[]`.
`$$` is a literal `$`.

### 7.23 `main` (`check_main_method_signature`) - stage 10

A GLOBAL function called `main` is the program's entry point when it is one of

```
def main(args: string[]) : int      def main(args: string[])
def main() : int                    def main()
```

Two such functions in the program: `E0930` (the first is shown). A global
`main` with other parameters or another result: `W1015` ("will not be run";
the C++ compiler warns only about the result, D81). `int` is the class
`std#int`, not `_int32` or `_int64`; the parameter is any `string[]` (also
`string[]?`: the C++ compiler compares parameters without the `?`); the
result may be written `int?` (it compares the class alone, and runs such a
`main` - checked). `def main() := e;` counts with the result its expression
gives (D87). A `private` `main` is an entry point like any other (checked).
A function called `main` inside a class is an ordinary function, and so is a
generic one. No `main` at all is not an analysis error (D18). The one found
is `analysed_program.main_function` with its form (`main_form`:
`MAIN_ARGS_INT` ... the C++ compiler's `main_method_sig` 0 - 3), which tells
the code generator which of the platform's four function values (`main`,
`main2`, `main3`, `main4` of `platform.kernel#platform`) to fill in. Built by
stage 10: `backend/compiler/program/main_function.sharp`.

### 7.24 Modifiers (`parse_access_flags` in `ast_parser.cpp`) - stage 5

| declaration | allowed |
|---|---|
| class, interface, enum | `public private protected local stable ext` |
| field, global variable | `public private protected static excuse local const`; `localized` in front of the name |
| function | `public private protected override local static native` |
| constructor | `public private protected` (the C++ compiler runs a constructor through the function rules and so also lets `override local static native` through; they mean nothing on a constructor) **[D30]** |
| `init` | `static` |
| getter, setter | `public private protected local static` |
| alias | `static` (`W1007`). `public private protected local` are accepted and IGNORED, as the C++ parser drops them: an alias in a class is always private, an alias in a module always public; the word gets `W1040` **[D102]** |
| local variable | `excuse const`; local alias: `const` |

- a word not in the list: `E0360`; the same word twice: `E0361`; `public`
  with `private` (two of the three): `E0364`;
- the words must come in this order: access (`public private protected`),
  `override`, `excuse`, `local`, `static`, `const`, `stable`, `ext`, `native`
  (`E0362`, with the corrected order in the help);
- `local` only on something declared in a module, not inside a class (`E0363`);
  with `public` / `private` / `protected` it wins: `W1009`;
- `static` where it is implied (globals, aliases, `native`): `W1007`.

### 7.25 `obfuscate` blocks (`obfuscate_compiler.cpp`) - stage 9

```
obfuscate -keep { "std.io", point, point.x, point.move(decimal, decimal), start(string[]), std#string.at(int) }
```

- the option after `-` is `keep`, `keep_inclusive` or `inclusive`, or there is
  none (`E0950` for any other word);
- a name in quotes is a MODULE (`E0951` when the program has none of that name);
- any other name is read from the left: parts that name a class (and classes
  inside it), then at most one member. It must be a class, an enum value, a
  field (also a global) or a function; a function is written WITH its
  parameter types, also when it has none, and they must be exactly those of
  one overload (`E0951` otherwise, with did-you-mean or the overloads that
  exist). `point(decimal)` names a constructor; `mod#name` names something of
  a module that is not imported. For a function of a generic class as written
  (`list.add(T)`) only the number of parameters is compared;
- a generic class with type arguments (`list<int>`) is that instance; without
  them it means every instance in a `-keep_inclusive` / `-inclusive` block,
  and nothing at all in the others (`W1016`).

Each name becomes an `obfuscation_rule` (6.9). Nothing else is checked; the
block only matters to code generation.

### 7.26 Smaller rules

| thing | rule | stage |
|---|---|---|
| `import (a.b, c.*) as g` | `g` names the GROUP of those modules for this file: `g#name` looks in them only, in the order written (C++ `import_group`). The same alias twice in a file: `E0393`. A module imported twice: `W1008`. Stage 2 does not record the alias yet; stage 5 adds `sharp_file.import_groups` and teaches the type lookup of stage 3 and the name lookup of stage 6 that the left of `#` may be a group | GM, EF |
| `mod` | must be the first declaration of the file (`E0390`) and appear once (`E0391`); the parser accepts both mistakes | GM |
| `std.io#name`, `std.io#f(x)`, `std.io#type` (`N_MODULE_NAME`) | the name is looked up in that module only (7.4.1 steps 5 and 6, 7.7.1 steps 5 to 7); no such module `E0303`, module without that name `E0403` | EF, CO |
| `alias t as name;` | a second name for a TYPE, in a module, a class or a block. It is never a value: used as one it is `E0404` unless followed by `.` or `(` (then it is the type it stands for) | done (declared), EF |
| `native def f(x: t) : r;` | a function the VM implements: no body (`E0365` when it has one, D41), `static`, never `override`, not part of any override family. A call is `CALL_STATIC` | GM, CO |
| `static_init` and `tls_init` of `platform.kernel#platform` | the two functions the compiler fills in itself (static field values, enum values, thread-local values). In the library they must be written EMPTY: `E0961` otherwise (C++: "platform level function does not allow statements or expressions inside"). `srt_init` of the same class is where the VM starts (`analysed_program.startup_function`) | WP (done) |
| the modules and classes the VM needs | the VM makes objects of some classes itself and finds them by name: the modules `std`, `std.io`, `std.io.fiber`, `platform.kernel`, and the classes `throwable`, `exception`, `error`, the thirteen exceptions and errors the VM throws, `_enum_`, `string`, `char_array`, `int`, `number`, `platform.kernel#stack_state`, `std.io#thread`, `std.io.fiber#fiber` and the six `std.reflect` classes (the list of the C++ `validate_required_dependencies`, in `program/required_declarations.sharp`). A program WITH a `main` that lacks one: `E0962`, once, at `main`, listing them (D89). Those present are `analysed_program.required_classes`, roots of the reachability walk | WP |
| a `static` function of a class without a body | may be declared (a class below may declare its own), but CALLING it is `E0510`: a static call is never virtual (D88) | WP |
| `localized x : t;` | a variable with one value per thread / fiber (`FLAG_LOCALIZED`, `local_variable.is_localized`). No getter or setter (`E0376`). Read and assigned like any other; reaching a static one through an object is `W1012` | GM, EF |
| `excuse x : t;` | "do not check that this is given a value" (7.15.4). Nothing else | WP |
| `stable class` | cannot be the target of `mutate` (`E0381`) | GM |
| `ext class` | cannot be made with `new` (`E0507`); the compiler itself makes objects of `ext` classes (`_enum_`) | CO |
| a declaration the parser recovered from (`N_ERROR` anywhere) | the parser has reported it. In a body it becomes a `bound_error` (an `N_ERROR` statement: a `bound_empty`), silently | EF, SF |
| a type or a function type written as an expression (`_int8;`, the parser allows it) | `E0404` | EF |

---

## 8. Feature inventory

The checklist every later stage and the final completeness check work from.
Part A lists every FEATURE of the language the backend must understand (A2:
the ones the first draft missed); part C every KIND OF NODE the parser can
produce and the stage that handles it; part B
every ERROR and WARNING the C++ compiler can report in these areas (all of the roughly 380
`create_new_error` / `createNewError` / `create_new_warning` call sites under
`lib/grammar/backend`, grouped - many sites report the same thing), each with
the bootstrap's code.

Stage column: **done** (stages 1 to 4), **GM** Generics and members, **EF**
Expression foundations, **CO** Calls and operators, **SF** Statements and
flow, **LI** Lambdas and initializers, **WP** Whole-program checks, **later**
(code generation / building an executable; not in this build), **n/a** (an
internal error of the C++ compiler that has no counterpart). A stage ticks its
rows (`[x]`) when the feature is built AND has a test.

### 8.A Features

| # | feature | rule | stage | ok |
|---|---|---|---|---|
| A1 | resolved types for fields, parameters, results, aliases | 6.1 | GM | [x] |
| A2 | generic class instance per use, lazily; nested classes; base / interfaces under the environment | 4.1 | GM | [x] |
| A3 | type parameter constraints `T base x` | 7.19 | GM | [x] |
| A4 | generic function blueprints and instances | 4.1, 7.19 | GM (registry), CO (instances at calls) | [x] GM part |
| A5 | default constructor for every class | 7.7.5 | GM | [x] |
| A6 | extension functions attached to their class | 7.18 | GM | [x] |
| A7 | `mutate` members in every generic instance | 7.18 | GM | [x] |
| A8 | modifiers: allowed words, order, defaults | 7.24 | GM | [x] |
| A9 | getters / setters as declared (types, modifiers) | 7.17 | GM | [x] |
| A10 | `find_member` through base classes and interfaces; override hiding | 7.4.3 | GM | [x] |
| A11 | well-known library classes found once (`string`, the boxes, `_object_`, `_enum_`, `throwable`, `loopable`, `pair`, `dictionary`, `function_value`) | - | GM | [x] |
| A12 | override links for instances of generic classes | 4.1 | GM | [x] |
| A20 | analysis context, block scopes, local variables | 6.6 | EF | [x] |
| A21 | literals: integer, hex, decimal, char, bool, text; `null` | 7.2 | EF | [x] |
| A22 | plain names: locals, parameters, fields, globals, enum values, types, modules | 7.4.1 | EF | [x] |
| A23 | `a.b`, `a?.b`, `a!!.b`, `self->b`, `base->b`, `module#name`, `outer.inner.x` | 7.4.3, 7.11 | EF | [x] |
| A24 | `self`, `base`, `base@type` | 7.7.2 | EF | [x] |
| A25 | implicit conversions (all rows of 7.5.1 except construction) as tree nodes | 7.5 | EF | [x] |
| A26 | `as`, `is`, `sizeof`, `!!`, `?:`, `? :`, parentheses | 7.5.5, 7.8.4, 7.11 | EF | [x] |
| A27 | array element `a[i]`, `new t[n]`, `new t[] { }`, `{ a, b }` | 7.7.5, 7.8.3 | EF | [x] |
| A28 | constants and folding | 6.3 | EF | [x] literals, converted numbers, `sizeof("text")`, `is`; operators are folded (`x := 2 + 3 * 4;` is the literal 14, checked in 10.10) |
| A29 | error type; pending nodes; `-analysis-stats` | 6.2 | EF | [x] |
| A30 | access rules for fields, classes, aliases, enums; static against instance (functions and constructors: A128) | 7.9 | EF | [x] |
| A31 | types on demand for `x := v` fields and `def f() := v` | 6.6 | EF | [x] |
| A32 | local declarations walked so names resolve (full statement rules come in SF) | 7.3 | EF | [x] |
| A33 | properties: reading a field with a getter is a `bound_call` of the getter | 7.17 | EF | [x] |
| A34 | box used as a number in conditions and indexes | 7.5.6 | EF | [x] |
| A40 | calls `f(a)`, `o.f(a)`, `t.f(a)`, `module#f(a)`, `self->f(a)`, `base->f(a)` | 7.7 | CO | [x] |
| A41 | overload resolution with ranking | 7.6 | CO | [x] |
| A42 | conversions through constructor / `init(x)` / `operator=` | 7.5.4 | CO | [x] |
| A43 | `new c(a)`, `c(a)` without `new`, `list<int>()` | 7.7.5 | CO | [x] |
| A44 | calling a function value; a function-typed field or local | 7.7.4 | CO | [x] |
| A45 | binary operators on numbers; result types; short-circuit `&& \|\|` | 7.8.1 | CO | [x] |
| A46 | operator functions: binary, unary, `++ --`, `[]`, `=`, `+=` ... | 7.8 | CO | [x] |
| A47 | text literal operands; string templates | 7.2, 7.22 | CO | [x] |
| A48 | assignment to locals, fields, elements, properties, `o[i] = v` | 7.10 | CO | [x] |
| A49 | `const`: assignment after the declaration is `W1021` | 7.10 | CO | [x] |
| A50 | `++ --` prefix and postfix on every kind of lvalue | 7.8.2 | CO | [x] |
| A51 | comparing references, null tests, function values | 7.8.1, 4.3 | CO | [x] |
| A52 | dictionary literals | 7.7.5 | CO | [x] |
| A53 | generic function calls `f<int>(x)` | 7.19 | CO | [x] |
| A54 | trailing lambda as the last argument (typed in LI) | 7.14 | CO | [x] |
| A60 | every statement as a typed-tree object | 6.4, 7.12 | SF | [x] |
| A61 | local variables: typed, inferred, several in one statement, `const`, `localized`, default objects for boxes | 7.3 | SF | [x] |
| A62 | local aliases | 7.12 | SF | [x] |
| A63 | `if`, `while`, `do while`, `for` (three forms), conditions | 7.12 | SF | [x] |
| A64 | `foreach` over arrays and `loopable` objects | 7.12 | SF | [x] |
| A65 | `return` with conversion; `throw` | 7.13 | SF | [x] |
| A66 | `break`, `continue`, what they leave | 7.12 | SF | [x] |
| A67 | `try` / `catch` / `finally`; `lock` | 7.12 | SF | [x] |
| A68 | `when` with and without a subject | 7.12 | SF | [x] |
| A69 | reachability; every path returns | 7.13 | SF | [x] |
| A70 | `asm` operands | 7.21 | SF | [x] |
| A71 | function bodies: parameters as locals, expression bodies `: t = e` | 7.13 | SF | [x] |
| A80 | lambdas: all forms, typing from the expected type, `it`, single-expression result | 7.14 | LI | [x] |
| A81 | captures; closure records; `self` in a lambda; lambdas in lambdas | 4.3 | LI | [x] |
| A82 | function names as values | 7.14 | LI | [x] |
| A83 | (moved to EF: A31 and A111. Field values are expressions; nothing about them waits for lambdas except a lambda value itself, which is pending until LI) | 7.3 | EF | - (the row moved) |
| A84 | inferred function results `def f() := e` | 7.13 | LI | [x] |
| A85 | constructors: base call, implicit base call, order | 7.15 | LI | [x] |
| A86 | `init { }`, `static init { }`, `init(x)` | 7.15.3 | LI | [x] |
| A87 | enum values | 7.16 | LI | [x] |
| A88 | getter and setter bodies; the field inside its own accessor | 7.17 | LI | [x] |
| A89 | lambdas and generic code: environment inherited | 4.1 | LI | [x] |
| A90 | fields that must be given a value | 7.15.4 | WP | [x] `W1032` (D82) |
| A91 | `main` | 7.23 | WP | [x] |
| A92 | components, `single`, `factory`, `get`, `inject` | 7.20 | LI | [x] |
| A93 | `obfuscate` blocks | 7.25 | LI | [x] |
| A94 | (moved to CO: A122. `dispatch` is set when a call is bound; nothing is rewritten at the end) | 7.7.3 | CO | - (the row moved) |
| A95 | `-bound` printer; zero pending nodes; completeness check of this section | - | WP | [x] the printer is `-analysis` (every local with its slot, `[reached from main]`, the analysed program); no pending node is left (stage 9); 10.6 has the walk-through |

### 8.A2 Features the first draft of the inventory left out

Found by reading `syntax_kinds.sharp` kind by kind, the suite's file list and
the library; each is assigned to a stage like the rows above.

| # | feature | rule | stage | ok |
|---|---|---|---|---|
| A100 | `import (...) as g` groups: `g#name`; alias twice; module imported twice | 7.26 | GM (record), EF (use) | [x] GM part |
| A101 | `mod` first and once | 7.26 | GM | [x] |
| A102 | interface members: functions only | 7.15.1 | GM | [x] |
| A103 | `native` functions; `stable` / `ext` classes; `localized`, `excuse` | 7.26 | GM | [x] |
| A104 | members of an instance made on first use (`ensure_members`) | 4.1 | GM | [x] |
| A105 | generic function inside a generic class: per class instance, then per call | 4.1 | GM, CO | [x] GM part |
| A106 | a plain number type as a type argument (`number<_int32, int>`) | 7.19 | GM | [x] |
| A107 | `sharp_function.overridden_by`, `family_root` for every override link, instances included | 7.7.3 | GM | [x] |
| A110 | types written in bodies: `written_type_at`, `sharp_file.body_types`; local aliases | 6.1 | EF | [x] |
| A111 | field values: every `x : t = v` checked against `t`; default objects for box / `string` fields | 7.3, 4.2 | EF | [x] values checked; default objects set (`-analysis` shows "(a default object)", checked in 10.10) |
| A112 | `x?.a.b()` as one `bound_safe_navigation`; a chain ending in a number | 7.11 | EF | [x] |
| A113 | `object` holds null; nullable to `object` | 7.5.1 | EF | [x] |
| A114 | implicit base-to-derived conversion with `W1017` | 7.5.1 | EF | [x] |
| A115 | items of `t[]??`; indexing an array that may be null | 7.8.3, 7.11 | EF | [x] |
| A116 | a type name, an alias or a function type where a value is needed; `x : a = a;` | 7.26 | EF | [x] |
| A117 | `module#name` as a value, a call, a type | 7.26 | EF, CO | [x] |
| A118 | `leniency_severity`: `W1017`, `W1021`, `W1022` in one place | 5 | EF | [x] |
| A119 | the `diagnostics_sink` (no message twice for one place) | 9.3 | EF | [x] |
| A120 | `o[i] = v` as `put`, else `operator=` on what `operator[]` returns; `o[i] += v` | 7.8.3 | CO | [x] |
| A121 | assignment to a property that has only a getter (direct store); compound assignment to one | 7.10 | CO | [x] |
| A122 | `dispatch` of every call, set when bound | 7.7.3 | CO | [x] |
| A123 | calling any expression that is a function value: `a[i](x)`, `f(1)(2)` | 7.7.4 | CO | [x] |
| A124 | result types of number operators (64-bit rule; `_int32` comparisons; `_int8` null tests) | 7.8.1 | CO | [x] |
| A125 | global functions: nearer module hides the name | 7.6 | CO | [x] |
| A126 | the plain-number rule; number conversion grades | 7.6 | CO | [x] |
| A127 | a call left pending because only a lambda argument could decide | 7.6 | CO (LI removes it) | [x] |
| A128 | private function / constructor used outside: `W1022` | 7.9 | CO | [x] |
| A129 | enum values compared with `==`, used as `when` values | 7.16 | CO, SF | [x] |
| A130 | expression statements with no effect; `N_ERROR` statements | 7.12.2, 7.26 | SF | [x] |
| A131 | hidden local for a `when` subject; `leaves` of `return` / `break` / `continue` | 6.4 | SF | [x] |
| A132 | `break` / `continue` / `return` and lambdas; `finally` | 7.12.2 | SF | [x] |
| A133 | `inject` on a local | 7.20 | SF (declared), LI (resolved) | [x] declared and checked by stage 8 (`bound_local_declaration.is_injected`, `E0617`); resolved by stage 9 (`injected_from`, `E0906`) |
| A140 | the no-arrow lambda and `it`; a lambda where none can stand | 7.14 | LI | [x] |
| A141 | `local_variable.is_captured` final per function; `depth` of every use; captured parameters | 4.3 | LI | [x] |
| A142 | `instance_initializers` / `static_initializers` of every class; enum values in them | 4.2, 7.15.2 | LI | [x] |
| A143 | `-> base(...)` arguments as instance code; `W1024` | 7.15.2 | LI | [x] |
| A144 | enum values: constant, counted on, computed at start, or unnumbered | 7.16 | LI | [x] |
| A150 | `static_init` / `tls_init` of the platform class are empty | 7.26 | WP | [x] checked by hand on a copy of the library (no test file: a second `platform.kernel#platform` cannot be declared beside the library's) |
| A151 | completeness: no `bound_pending`, no node kind without a handler (8.C) | 8.C | WP | [x] the `unknown:` self-check is 0 on the library, the suite and the bootstrap's own source; every N_ kind of 8.C has its handler |

### 8.B Errors and warnings of the C++ compiler

"C++ source" names the file (under `lib/grammar/backend`) and the message, or
its gist.

**Declarations and members**

| # | C++ message | C++ source | code | stage | ok |
|---|---|---|---|---|---|
| B1 | module declaration must be first in every file | preprocessor/class_preprocessor, postprocessor/class_processor, di/component_compiler | `E0390` | GM | [x] |
| B2 | file module cannot be declared more than once | same | `E0391` | GM | [x] |
| B3 | class / child class / alias / field already defined | types/sharp_class, sharp_alias, sharp_field | `E0302`, `E0330` | done | [x] |
| B4 | function already defined | types/sharp_function | `E0331`, `E0332` | done | [x] |
| B5 | declared X conflicts with field / alias / function / class (warnings) | preprocessor/class_preprocessor | `W1001` | GM | [x] |
| B6 | import alias already defined in the file | types/import_group | `E0393` | GM | [x] |
| B7 | module could not be found; modules under prefix not found | postprocessor/import_processor | `E0303` | done | [x] |
| B8 | module has already been imported (warning) | postprocessor/import_processor | `W1008` | GM | [x] |
| B9 | base class errors (not a class, interface, enum, cyclic) | postprocessor/base_class_processor | `E0320`-`E0323` | done | [x] |
| B10 | support class for objects / enums not found; class `std#string` not found; wrapper class missing; internal "cannot locate ..." | base_class_processor, field_processor, enum_compiler, literal_expression, dictionary_expression, function_compiler | `E0395` | GM | [x] |
| B11 | interface errors | postprocessor/interface_processor | `E0324`, `E0325` | done | [x] |
| B12 | delegate not implemented; override without delegate | postprocessor/delegate_processor | `E0341`-`E0344` | done | [x] |
| B13 | delegate function missing enclosing class | postprocessor/function_processor | `E0340` | done | [x] |
| B14 | generic class: wrong number of type arguments | types/sharp_class:129 | `E0350` | GM | [x] |
| B15 | generic type must contain base type | postprocessor/class_processor:219 | `E0351` | GM | [x] |
| B16 | generic class declaration error (the same type parameter name twice) | preprocessor/generic_class_preprocessor:81 | `E0359` | GM | [x] |
| B17 | generic function already declared with the same number of type parameters | functions/generic_functions | `E0358` | GM | [x] |
| B18 | array arrays are not supported | dependency:3328 | `E0357` | GM | [x] |
| B19 | expression of type X cannot be nullable (a number) | dependency:3340 | `E0355` | GM | [x] |
| B20 | cannot specify non-nullable integer array items as nullable; nullable array items on non-array type | astparser/ast_parser | `E0356` | GM | [x] |
| B21 | member type only allows access specifiers (...); invalid access specifier | astparser/ast_parser | `E0360` | GM | [x] |
| B22 | duplicate access flag | astparser/ast_parser | `E0361` | GM | [x] |
| B23 | invalid access specifier ... make sure the order is correct | astparser/ast_parser:539 | `E0362` | GM | [x] |
| B24 | `local` can only be used at global scope | astparser/ast_parser | `E0363` | GM | [x] |
| B25 | `local` trumps public / private / protected (warning) | astparser/ast_parser | `W1009` | GM | [x] |
| B26 | `static` is added by default on aliases / global fields / global functions / native functions (warnings) | alias_preprocessor, field_preprocessor, function_processor | `W1007` | GM | [x] |
| B27 | constructor must match holding class name | function_processor, function_compiler | `E0370` | GM | [x] |
| B28 | invalid amount of parameters on initializer function; expected (1) parameter for initializer | function_processor, init_compiler | `E0371` | GM | [x] |
| B29 | static initializer functions are not allowed | init_compiler | `E0372` | GM | [x] |
| B30 | extension function must be extended by a class | function_processor | `E0373` | GM | [x] |
| B31 | extension function cannot be a delegate | function_processor | `E0374` | GM | [x] |
| B32 | cannot apply setter to constant field | field_processor | `E0375` | GM | [x] |
| B33 | cannot apply setter / getter to localized field | field_processor | `E0376` | GM | [x] |
| B34 | getter cannot have differing access types from the field | field_processor | `E0377` | GM | [x] |
| B35 | injected field cannot be assigned a value | field_processor, local_variable_decl | `E0378` | GM (fields), SF (locals) | [x] fields |
| B36 | local field cannot have setter / getter | local_variable_decl | parser (`E01xx`) | done | [x] |
| B37 | cannot mutate stable class | mutation_processor | `E0381` | GM | [x] |
| B38 | expected mutate host to be of type class | mutation_processor, mutate_compiler | `E0380` | GM | [x] |
| B39 | invalid module accessor, did you mean `.` | astparser/ast_parser:250 | parser / `E0303` | done | [x] |

**Names and expressions**

| # | C++ message | C++ source | code | stage | ok |
|---|---|---|---|---|---|
| B50 | could not resolve symbol X (+ "did you possibly mean") | dependency (15 sites) | `E0400` | EF | [x] |
| B51 | could not resolve X after non class type; field X must be of type class; expected `class` type but X was found; illegal use of non-class field as class type | dependency, post_ast_expression | `E0405` | EF | [x] |
| B52 | could not resolve X in field Y (no such member) | dependency | `E0402` | EF | [x] |
| B53 | cannot access inner class through field | dependency | `E0405` | EF | [x] |
| B54 | illegal use of field X as a type | field_processor, function_processor | `E0428` | EF | [ ] not a message of its own: `x : v.money` is "there is no type named `v`" (`E0301`, stage 3) |
| B55 | cannot assign hard type as value; must resolve to a class, object, lambda, or number value; field cannot have a method representing the type | field_processor, function_processor | `E0404` | EF | [x] |
| B56 | cannot assign field to its-self | field_processor | `E0408` | EF | [x] |
| B57 | cannot be assigned type X due to invalid type assignment format | field_processor, function_processor | `E0409` | EF | [~] no program is known that reaches it; the code was retired by stage 8 (10.4): what the C++ message covers is `E0404` (a type as a value) or `E0406` (no value) |
| B58 | expression of type X must evaluate to a value | dot_notation_call, array_expression, post_ast, dependency | `E0406` | EF | [x] |
| B59 | cannot use operator on expression that returns nil | binary_expression, unary_expression | `E0406` | CO | [x] |
| B60 | cannot access private X outside of class | dependency:956 | `E0700` | EF | [x] |
| B61 | cannot access protected X outside of class | dependency:965 | `E0701` | EF | [x] |
| B62 | cannot access local X outside of home file; cannot access local enum from outside file | dependency:948, 1355 | `E0702` | EF | [x] |
| B63 | cannot access instance field from static context; cannot capture closure / value of instance field from static context | dependency | `E0703` | EF | [x] |
| B64 | cannot access instance function from static context; cannot capture closure from instance function ... `self->` is required | dependency:2325 | `E0704` | CO | [x] |
| B65 | cannot access self from static context | self_expression | `E0421` | EF | [x] |
| B66 | class X is not a base class of primary class; unexpected type instead of class; class does not contain a base class | base_expression | `E0422` | EF | [x] |
| B67 | accessing static localized field through instance; unusual access of alias / static function through field; unnecessary access type on statically accessed field / alias / function / inner class (warnings) | dependency | `W1012`, `W1005` | EF | [~] `W1012` for a static localized field, `W1005` for `?.` after a class name; the "unusual access" of an alias or a static function through an object is accepted without a warning |
| B68 | accessing class / nullable field without `?.` or `!!.`; unsafe use of nullable type | dependency, post_ast_expression | `E0722` | EF | [x] |
| B69 | unnecessary use of `?.` / `!!.` on non nullable type (warning) | post_ast_expression | `W1005` | EF | [x] |
| B70 | cannot use operator `!!` with type X | force_non_null_expression | `E0723` | EF | [x] |
| B71 | redundant use of operator `!!` (warning) | force_non_null_expression | `W1004` | EF | [x] |
| B72 | elvis: expression was found to be non nullable (warning) | elvis_expression | `W1006` | EF | [x] |
| B73 | elvis: expressions are not compatible | elvis_expression | `E0724` | EF | [x] |
| B74 | cannot cast X to Y | cast_expression | `E0412` | EF | [x] |
| B75 | cast expression of type X not allowed | cast_expression | `E0412` | EF | [x] |
| B76 | redundant cast (warning) | cast_expression | `W1003` | EF | [x] |
| B77 | cannot get sizeof from expression of type X | sizeof_expression | `E0413` | EF | [x] |
| B78 | inline if: expression of type X is not equal to that of type Y (two forms) | inline_if_expression | `E0420` | EF | [x] |
| B79 | inline if: condition must evaluate to true or false; expression must evaluate to a numeric value, true, or false | inline_if_expression, expression | `E0411` | EF | [x] |
| B80 | field `X.value` was not found (a box without `value`) | expression | `E0395` | EF | [ ] a box class without `value` is not used as a number (the place then reports `E0411` / `E0415`); no message of its own |
| B81 | Multi-dimensional arrays are not supported | array_expression | `E0416` | EF | [x] |
| B82 | type X must be either a class or array type (index) | array_expression | `E0414` | EF | [x] |
| B83 | cannot create array of type X | new_expression, vector_array_expression | `E0417` | EF | [x] |
| B84 | type of `nil` cannot be defined as array element | new_expression | `E0406` | EF | [x] |
| B85 | cannot assign array item of type X to array of type Y; expected array item of type X but found Y | new_expression, vector_array_expression | `E0410`, `E0418` | EF | [x] |
| B86 | could not determine the type of the array | vector_array_expression | `E0418` | EF | [x] |
| B87 | lambda must be fully qualified (array item) | new_expression | not an error: the item type gives the lambda its types (D95) | LI | [x] |
| B88 | remainder of a division by zero | binary_expression | `E0427` | CO | [x] |
| B89 | expression could be simplified to true / false (warning) | unary_expression | `W1010` | CO | [x] |
| B90 | the class `std#string` is needed ...; no constructor of `std#string` takes a string literal | literal_expression | `E0395` | CO | [x] |

**Calls, constructors, operators**

| # | C++ message | C++ source | code | stage | ok |
|---|---|---|---|---|---|
| B100 | could not resolve symbol `name(types)` (no such function / no overload) | dependency:2785 ff. | `E0500`, `E0501`, `E0402` | CO | [x] |
| B101 | call to method X is ambiguous (+ candidate list on stdout) | dependency:679 | `E0503` | CO | [x] |
| B102 | call to function pointer X is ambiguous | dependency:596 | `E0503` | CO | [x] |
| B103 | call to field of type X cannot be called with parameters; Parameter type mismatch; Too many parameters for function call | dependency:534, post_ast_expression | `E0505` | CO | [x] |
| B104 | call to field of type X is not a function pointer; expected `function pointer` type | dependency:539, post_ast_expression | `E0504` | CO | [x] |
| B105 | cannot call non-fully qualified function | post_ast_expression | `E0504` | CO | [x] |
| B106 | cannot explicitly call constructor | dependency:3316 | `E0508` | CO | [x] |
| B107 | cannot find constructor X for class Y | new_expression, dependency:2635, dictionary_expression | `E0506` | CO | [x] |
| B108 | cannot instantiate non-class type; cannot instantiate extension class | new_expression | `E0507` | CO | [x] |
| B109 | no default constructor found for class X | field_compiler, local_variable_decl | `E0506` | SF, LI | [x] |
| B110 | could not resolve base class constructor; class does not inherit a base class | function_compiler | `E0509` | CO (was LI) | [x] |
| B111 | generic functions are not supported (as a signature / value) | dependency:3465 | `E0805` | LI | [x] |
| B112 | expected function call | dependency:3441 | `E0504` | CO | [x] |
| B113 | use of operator X does not have any qualified overloads with class Y | expression:277 | `E0511` | CO | [x] |
| B114 | unqualified use of operator X with type Y and Z; expected numeric type; expected object type | binary_expression | `E0511` | CO | [x] |
| B115 | expression must be of type class or numeric to use operator (arrays); mismatched array types | binary_expression, unary_expression | `E0513` | CO | [x] |
| B116 | expressions of type object must be casted before using operator | binary_expression, unary_expression | `E0512` | CO | [x] |
| B117 | unqualified use of operator X on bool / string expression; with (field of) type Y (unary) | unary_expression | `E0514` | CO | [x] |
| B118 | cannot apply operator to untyped expression (internal) | binary_expression, unary_expression | n/a (error type) | - | [x] |
| B119 | expected dictionary key / value of type X but found Y; could not determine the type of the dictionary | dictionary_expression | `E0419` | CO | [x] |
| B120 | could not find / create generic classes `pair` and `dictionary` | dictionary_expression | `E0395` | CO | [x] no test (needs a library without `pair` / `dictionary`) |
| B121 | getting address from ambiguous function reference (warning) | dependency:1145, 1411 | `W1011` | LI | [x] |
| B122 | cannot get address of non-static function | dependency:1150, 1416 | `E0804` | LI | [x] |

**Assignment, const, null**

| # | C++ message | C++ source | code | stage | ok |
|---|---|---|---|---|---|
| B130 | expressions are not compatible, assigning X to (field / expression) of type Y; cannot assign field X type Y, as types do not match | assign_expression, field_compiler, local_variable_decl | `E0410` | CO, SF, LI | [x] |
| B131 | assigning null to non nullable type | assign_expression | `E0720` | CO | [x] |
| B132 | assigning nullable type of X to non nullable type; returning nullable type to non nullable; cannot assign 'nullable' to non nullable type as return value | field_compiler, local_variable_decl, return_statement, function_processor | `E0721` | CO, SF, LI | [x] |
| B133 | expressions of type X is not assignable | assign_expression:556 | `E0711` | CO | [x] |
| B134 | expressions of type X cannot be assigned via operator Y | assign_expression:561 | `E0712` | CO | [x] |
| B135 | Conflicting assignment on field ... Cannot assign value via setter when `operator=` is required | assign_expression:372 | `E0713` | CO | [x] |
| B136 | cannot assign field X of type Y to expression of type Z (through a setter) | assign_expression:400 | `E0410` | CO | [x] |
| B137 | (not reported by the C++ compiler) assignment to `const` | - | `W1021` | CO | [x] |
| B138 | (not reported) base class value given to a derived class variable or parameter | - | `W1017` | EF | [x] |
| B139 | (not reported, and not an error: a property without a setter is assigned directly, 7.10) | - | none | CO | [x] |
| B140 | (not reported) private function or constructor used outside its class | - | `W1022` | CO | [x] |
| B141 | (not reported) `x?.member` giving a plain number | dependency:3340 reports the call form only ("cannot be nullable") | `E0725` | EF | [x] |
| B142 | (not reported) `new` of an interface or enum | - | `E0507` | CO | [x] |
| B143 | (fatal error in the C++ code generator) compound assignment to a property that has only a getter; `break` / `continue` inside a lambda that is inside a loop | - | accepted (7.10) / `E0602`, `E0603` | CO, SF | [x] |
| B144 | (not reported) an integer literal that does not fit the `int` it is inferred to be | - | `W1023` | EF | [x] |
| B145 | (not reported) `-> base(...)` argument reads a field that has no value yet | - | `W1024` | LI | [x] |

**Statements and flow**

| # | C++ message | C++ source | code | stage | ok |
|---|---|---|---|---|---|
| B150 | local field X is already defined in function; alias already defined in function | types/sharp_field:94, sharp_alias:73 | `E0600` | SF | [x] |
| B151 | if / else if / while / do while / for condition must evaluate to true or false | if_statement, while_statement, do_while_statement, for_statement | `E0411` | SF | [x] |
| B152 | attempt to call `break` / `continue` outside of an enclosing loop | break_statement, continue_statement | `E0602`, `E0603` | SF | [x] |
| B153 | control cannot leave body of finally clause | break, continue, return statements | `E0604` | SF | [x] |
| B154 | not all code paths return a value | function_compiler:414 | `E0605` | SF | [x] |
| B155 | returning X from a function returning Y; return value of type X is not compatible | return_statement, function_compiler:363 | `E0410`, `E0606`, `E0607` | SF | [x] |
| B156 | unreachable statement (warning) | statement_compiler | `W1002` | SF | [x] |
| B157 | foreach expression must be an array type | for_each_statement | `E0609` | SF | [x] |
| B158 | could not call static function / support function must return an array / could not locate `get_elements()` (loopable) | for_each_statement | `E0609` | SF | [x] |
| B159 | type assigned to field X cannot evaluate to an array (foreach, catch) | for_each_statement, try_catch_statement | `E0610`, `E0613` | SF | [x] |
| B160 | incompatible types, cannot convert X to Y (foreach) | for_each_statement | `E0610` | SF | [x] |
| B161 | throw: must inherit `std#throwable`; must evaluate to a class | throw_statement | `E0612` | SF | [x] |
| B162 | catch: type must be of type class; handling class cannot be an array; must inherit `std#throwable` | try_catch_statement | `E0613` | SF | [x] |
| B163 | class X has already been caught | try_catch_statement | `E0614` | SF | [x] |
| B164 | attempt to lock non-lockable object | lock_statement | `E0615` | SF | [x] |
| B165 | when: (reported through the binary operator and condition rules) | when_statement | `E0616`, `E0411` | SF | [x] |
| B166 | unknown ast type (internal); unexpected AST in if statement | statement_compiler, if_statement | n/a | - | [x] |
| B167 | asm: offsets are not allowed with field members | asm_statement | parser (not accepted) | done | [x] |
| B168 | asm: attempt to get address of a member X when none were found (+ resolve errors) | asm_statement:118, 121, 156 | `E0920`, `E0923` | SF | [x] |
| B169 | asm: expected asm literal; unknown opcode | asm_statement | parser | done | [x] |
| B170 | label X is already defined | types/sharp_label | `E0921` | SF | [x] |
| B171 | (not reported) asm label used and never defined | - | `E0922` | SF | [x] |

**Lambdas**

| # | C++ message | C++ source | code | stage | ok |
|---|---|---|---|---|---|
| B180 | expression is not a fully qualified lambda expression; cannot assign non fully qualified function to field | field_processor, function_processor, field_compiler, local_variable_decl | `E0800` | LI | [x] |
| B181 | cannot capture closure of local field X. Only arrays, classes and objects | dependency:916, 2143 | none: allowed (D10) | LI | [x] |
| B182 | cannot find the function around the lambda (internal) | closure_capture | n/a | - | [x] |
| B183 | (not reported; a lambda that does not fit simply "does not resolve") parameter count / type / result of a lambda against the expected function type | - | `E0801`, `E0802`, `E0803`, `E0809` | LI | [x] |

**Enums, components, main, the rest**

| # | C++ message | C++ source | code | stage | ok |
|---|---|---|---|---|---|
| B190 | expected numeric type for enum field | enum_compiler | `E0940` | LI | [x] |
| B191 | component X has already been defined | component_manager | `E0900` | LI | [x] |
| B192 | type definition of type X / with name X has already been defined | component_manager | `E0901` | LI | [x] |
| B193 | cannot call `get()` outside of a component block without explicitly declaring type; cannot get type definition from generic `get()`; cannot resolve type from `get()` expression | get_expression, component_compiler, field_processor | `E0902` | LI | [x] |
| B194 | No defined type found; Could not find type definition; could not find type definition for given parameters | get_expression, component_compiler | `E0903` | LI | [x] |
| B195 | Type mismatch on type definition | get_expression | `E0904` | LI | [x] |
| B196 | type of `nil` cannot be defined in components | component_compiler | `E0905` | LI | [x] |
| B197 | cannot inject field X, no type found in components | field_compiler | `E0906` | LI | [x] |
| B198 | lambda must be fully qualified to be injected | component_compiler | `E0907` | LI | [x] not a code of its own: `E0800` (D77) |
| B199 | cannot set type of X as a type definition (after all retries) | component_compiler | `E0908` | LI | [x] |
| B200 | multiple components found with type X; multiple type definitions with name X (warnings) | component_manager | `W1013` | LI | [x] |
| B201 | unnecessary explicit type declaration for `get()` (warning) | get_expression | `W1014` | LI | [x] |
| B202 | main method with the same or different signature already exists | function_compiler | `E0930` | WP | [x] |
| B203 | main method might not be executed (warning) | function_compiler | `W1015` | WP | [x] |
| B204 | field X must be initialized | finalizer/optimization/initialization_validator | `W1032` (D82) | WP | [x] |
| B205 | platform level function does not allow statements or expressions inside | function_compiler:320 | `E0961` | WP | [x] no test (see A150) |
| B206 | obfuscate: unknown obfuscation modifier | obfuscate_compiler | `E0950` | LI | [x] |
| B207 | obfuscate: cannot locate module; invalid type found in obfuscation block (two forms) | obfuscate_compiler | `E0951` | LI | [x] |
| B208 | obfuscate: non-created generic class (warning) | obfuscate_compiler | `W1016` | LI | [x] |
| B211 | missing required module dependency X; missing required class dependency X (fatal) | finalizer/optimization/dependency_runner (`validate_required_dependencies`) | `E0962` (D89); the "may be obfuscated" half: later | WP | [x] no test file (the library is compiled with every test); checked by hand by compiling a `main` without the library |
| B212 | (not reported; the VM stops) a static function without a body is called | - | `E0510` (D88) | WP | [x] |
| B209 | could not locate main method; starter class and its `__srt_init` / static init / tls init requirements (15 messages) | finalizer/finalizer.cpp, optimizer.cpp | - | later | - |
| B210 | internal errors: "could not resolve function X in Y", "could not locate initializer function", "expected ast of utype", unknown declaration kinds | function_compiler, init_compiler, dependency, class_processor, preprocessors | n/a | - | [x] |

### 8.C Every node kind and who handles it

`frontend/syntax/tree/syntax_kinds.sharp` is the contract with the parser. Every
`N_*` kind is in this table once; "done" means stages 1 to 4 already read it.
The dispatch functions (`analyse_expression`, `analyse_statement`) end in a
branch that reports an INTERNAL error naming the kind (`E0990`, "the analysis
has no rule for a `<kind>` node here"): a kind that is never handled is
then found by the first program that uses it instead of being skipped
silently. Stage 10 adds a test that walks a file containing every construct.

| kinds | what | handled by |
|---|---|---|
| `N_FILE`, `N_MODULE`, `N_NAME` | file, `mod` line | done; GM (`E0390`, `E0391`) |
| `N_IMPORT`, `N_IMPORT_ITEM`, `N_STAR` | imports | done; GM (the alias child, `E0393`, `W1008`) |
| `N_MODIFIERS`, `N_MODIFIER` | modifier words | done (flags); GM (7.24) |
| `N_CLASS`, `N_INTERFACE`, `N_ENUM`, `N_MEMBERS`, `N_BASE`, `N_INTERFACES` | type declarations | done; GM (`E0379`) |
| `N_GENERIC_PARAMETERS`, `N_GENERIC_PARAMETER` | `<T, U base x>` | done; GM (instances, `E0351`, `E0358`, `E0359`) |
| `N_MUTATE` | `mutate` | done (stage 3); GM (`E0380`, `E0381`) |
| `N_ENUM_VALUE` | an enum value and its `= e` | done (declared); LI (the value, 7.16) |
| `N_FUNCTION`, `N_PARAMETERS`, `N_PARAMETER`, `N_RETURN_TYPE`, `N_OPERATOR_NAME` | functions | done; GM (resolved types, extension functions, generic functions); SF (the body) |
| `N_EXPRESSION_BODY` | `def f() := e;` / `: t = e;` | EF (inferred result, on demand); SF (as a body) |
| `N_CONSTRUCTOR`, `N_BASE_CALL` | constructors | done; GM (`E0370`, default constructor); SF (body); LI (base call) |
| `N_INIT` | `init { }`, `static init`, `init(x)` | done; GM (`E0371`, `E0372`); SF (body); LI (order, conversion function) |
| `N_VARIABLE` (in a module or class), `N_INITIALIZER` | fields and globals | done; GM (type); EF (value, inferred type) |
| `N_VARIABLE` (in a block or `N_FOR_INIT`) | locals | EF (declared so names resolve); SF (the statement) |
| `N_GETTER`, `N_SETTER` | accessors | done; GM (7.17 declaration rules); SF (body); LI (the field inside its own accessor) |
| `N_ALIAS` | `alias t as name;` | done (module, class); SF (in a block) |
| `N_INJECT` | `inject` on a field or local | GM (`E0378`); SF (a local: `E0617`, `E0618`); LI (resolved: `backend/compiler/components/injections.sharp`) |
| `N_COMPONENT`, `N_COMPONENT_TYPE` | components | GM (declared); LI (`backend/compiler/components`) |
| `N_OBFUSCATE`, `N_OBFUSCATE_ELEMENT` | `obfuscate` blocks | LI (`backend/compiler/obfuscation`) |
| `N_TYPE`, `N_TYPE_NAME`, `N_MODULE_PATH`, `N_TYPE_PART`, `N_NATIVE_TYPE`, `N_FUNCTION_TYPE`, `N_ARRAY_SUFFIX`, `N_NULLABLE` | a written type | done (in declarations); GM (resolved, `E0350`-`E0357`); EF (in bodies: `written_type_at`) |
| `N_BLOCK`, `N_EXPRESSION_STATEMENT`, `N_EMPTY_STATEMENT` | | EF (walked); SF |
| `N_RETURN`, `N_IF`, `N_WHILE`, `N_DO_WHILE`, `N_FOR`, `N_FOR_INIT`, `N_FOR_CONDITION`, `N_FOR_STEP`, `N_FOREACH`, `N_THROW`, `N_BREAK`, `N_CONTINUE`, `N_LOCK`, `N_TRY`, `N_CATCH`, `N_FINALLY`, `N_WHEN`, `N_WHEN_SUBJECT`, `N_WHEN_CLAUSE`, `N_WHEN_ELSE` | statements | EF (walked for their expressions and the locals they declare); SF (typed tree, rules) |
| `N_ASM`, `N_ASM_INSTRUCTION`, `N_ASM_LABEL`, `N_ASM_REGISTER`, `N_ASM_NEGATIVE`, `N_ASM_LABEL_REF`, `N_ASM_MEMBER`, `N_ASM_TYPE` | `asm` blocks | SF (7.21); EF skips the block |
| `N_LITERAL`, `N_NULL`, `N_SELF`, `N_BASE_VALUE`, `N_IDENTIFIER`, `N_MODULE_NAME`, `N_PAREN` | values and names | EF |
| `N_MEMBER`, `N_NOT_NULL`, `N_CAST`, `N_IS`, `N_SIZEOF`, `N_TERNARY`, `N_ELVIS` | | EF |
| `N_INDEX`, `N_NEW_ARRAY`, `N_ARRAY_LITERAL` | arrays | EF (real arrays); CO (`operator[]`) |
| `N_CALL`, `N_ARGUMENTS`, `N_NEW` | calls, objects | CO (pending in EF) |
| `N_BINARY`, `N_UNARY`, `N_POSTFIX`, `N_ASSIGN` | operators | CO (pending in EF) |
| `N_DICTIONARY`, `N_DICTIONARY_TYPE`, `N_DICTIONARY_PAIR`, `N_STRING_TEMPLATE` | | CO (pending in EF) |
| `N_LAMBDA` | lambdas | LI (pending in EF and CO; counted as an argument in CO) |
| `N_IDENTIFIER` naming a function, used as a value | function reference | LI (pending before) |
| `N_GET_COMPONENT` | `get<t>(...)` | LI (`backend/compiler/components`; pending before) |
| `N_ERROR` | where parsing failed | EF (`bound_error`), SF (`bound_empty`); never reported again |

---

## 9. Error codes and diagnostics

### 9.1 Ranges

| range | area |
|---|---|
| `E01xx` | parser (taken, `frontend/`) |
| `E030x`-`E034x` | declarations and class checks (taken: `E0301`-`E0305`, `E0310`, `E0320`-`E0325`, `E0330`-`E0333`, `E0340`-`E0346`) |
| `E035x`-`E039x` | generics and members |
| `E04xx` | names and expressions |
| `E05xx` | calls, overloads, constructors, operators |
| `E06xx` | statements and control flow |
| `E07xx` | access, const, null-safety, assignment |
| `E08xx` | lambdas, closures, function values |
| `E09xx` | everything else: components / inject, asm, main, enums, obfuscate, field initialisation |
| `W1xxx` | warnings |

A code has ONE meaning. A stage that needs a code not listed here takes the
next free number of its range and adds it to this table in the same change.

### 9.2 Every code

Existing: section 3. New (the stage that reports it in brackets):

| code | meaning |
|---|---|
| `E0320`-`E0325` | (existing, stage 4) also reported by stage 5 for ONE INSTANCE of `class mixin<T> base T` / `class tagged<T> : T` / `class both<A, B> : i<A>, i<B>`: the type argument is not a class, is an interface, is an enum, makes the instance inherit from itself, is not an interface, gives the same interface twice [GM] |
| `E0331`, `E0332` | (existing, stage 4) also for one instance: two functions / constructors whose parameters are the same types there [GM] |
| `E0341`-`E0343` | (existing, stage 4) also what the comparison with resolved types adds [GM] |
| `E0345` | a generic function without a body in a class, or marked `override` [GM] |
| `E0350` | wrong number of type arguments for a generic class [GM] |
| `E0351` | a type argument does not satisfy `T base x` [GM] |
| `E0352` | generic instances nest without end [GM] |
| `E0353` | type arguments given to something that is not generic [GM] |
| `E0354` | a generic class used without type arguments [GM] |
| `E0355` | a plain number type cannot be nullable (written, or arising from `T?` with `T` a number) [GM, EF] |
| `E0356` | nullable items on something that is not an array of objects [GM] |
| `E0357` | an array of arrays [GM] |
| `E0358` | two generic functions with the same name and number of type parameters [GM] |
| `E0359` | a type parameter named twice / malformed type parameter list [GM] |
| `E0360` | a modifier that this kind of declaration cannot have (also `static` / `override` / `native` / `local` on a constructor) [GM] |
| `E0361` | the same modifier twice [GM] |
| `E0362` | modifiers in the wrong order [GM] |
| `E0363` | `local` inside a class [GM] |
| `E0364` | two of `public` / `private` / `protected` [GM] |
| `E0365` | a `native` function with a body [GM] |
| `E0366` | an operator function declared outside any class [GM] |
| `E0367` | an enum without values [GM] |
| `E0368` | a class declared inside a generic class, named where its instance is not known (outside that class) [GM] |
| `E0369` | a second getter or a second setter on one field [GM] |
| `E0370` | a constructor not named like its class [GM] |
| `E0371` | `init(x)` must have exactly one parameter [GM] |
| `E0372` | `static init` with parameters / `static` on `init(x)` [GM] |
| `E0373` | an extension function's target is not a class [GM] |
| `E0374` | an extension function without a body [GM] |
| `E0375` | a setter on a `const` field [GM] |
| `E0376` | a getter or setter on a `localized` field [GM] |
| `E0377` | a getter's modifiers differ from its field's [GM] |
| `E0378` | an injected variable with a value [GM, SF] |
| `E0379` | an interface that declares a field, a constructor, `init`, a function WITH a body, or a class / interface / enum inside it [GM] |
| `E0380` | `mutate` of something that is not a class [GM, reported by stage 3] |
| `E0381` | `mutate` of a `stable` class [GM, reported by stage 3] |
| `E0382` | `mutate ... base x` on a class that already has a base class [GM, reported by stage 3] |
| `E0383` | `mutate` with type arguments that are not the class's own parameter names (`mutate box<int>`) [GM, reported by stage 3] |
| `E0384` | an extension function for a generic class mixes the class's type parameters with other types, or writes them out of order (`def pair<B, A>.f()`, `def pair<string, B>.f()`) (7.18) [LR] |
| `E0390` | the `mod` line is not first in the file, or the file has none [GM] |
| `E0391` | two `mod` lines in a file [GM] |
| `E0393` | an import alias defined twice [GM, reported by stage 2] |
| `E0395` | a class the compiler itself needs is missing from the library, or lacks the member it needs [GM and later] |
| `E0396` | internal: a declared type of a real class still depends on a type parameter after stage 5. Never expected (0 on the library and every test); it exists so that such a type is never passed on silently as "fits everything" [GM] |
| `E0400` | unknown name [EF] |
| `E0402` | the class has no member of that name [EF, CO] |
| `E0403` | the module has no such name [EF] |
| `E0404` | a type (or a function) written where a value is needed [EF] |
| `E0405` | member access / qualifier on something that has no members [EF] |
| `E0406` | an expression without a value used as a value [EF] |
| `E0407` | a local used before its declaration, or inside its own declaration (`x := x;`) [EF] |
| `E0408` | a type that depends on itself (`x := x`, a circle of fields) [EF] |
| `E0409` | (retired by stage 8, the number kept: the one place that reported it - a local with neither a type nor a value - is `E0618`. No value has no type: a value without one is `E0406`) |
| `E0410` | type mismatch: a value does not convert to the type wanted (declaration, assignment, return, array item). A call that fits no function is `E0501` even with one candidate; an argument of a function VALUE is `E0505` [EF and later] |
| `E0411` | a condition that is not a number [EF, SF] |
| `E0412` | a cast that can never work (a text literal to `string` included) / a type that cannot be cast to [EF] |
| `E0413` | `sizeof` of something without a size: a plain number, `null` [EF] |
| `E0414` | `[ ]` on something that is neither an array nor has `operator[]` [EF, CO] |
| `E0415` | an array index that is not a number [EF] |
| `E0416` | more than one index on an array [EF] |
| `E0417` | an array of a type that arrays cannot have [EF] |
| `E0418` | array literal: cannot tell the type (`{ }`, `{ null, ... }`, an array as first item). An item that does not fit is `E0410` [EF] |
| `E0419` | dictionary literal: key or value does not fit / cannot tell the type [CO] |
| `E0420` | `? :` branches of unrelated types [EF] |
| `E0421` | `self` or `base` in static code [EF] |
| `E0422` | `base@t` where `t` is not a base class [EF] |
| `E0423` | a value in a string template that cannot become text [CO] |
| `E0424` | (not used: a whole-number literal that needs more than 64 bits is a `decimal`, as in the C++ compiler - 10.2) |
| `E0427` | division remainder by constant zero [CO] |
| `E0428` | a value written where a type is needed [EF] |
| `E0500` | nothing callable has that name [CO] |
| `E0501` | no overload fits these arguments [CO] |
| `E0503` | ambiguous call [CO] |
| `E0502` | (not used; kept free so `E0501` / `E0503` stay a pair) |
| `E0504` | calling something that is not a function [CO] |
| `E0505` | a function value called with the wrong arguments [CO] |
| `E0506` | no constructor fits [CO] |
| `E0507` | cannot make an object of this (interface, enum, `ext` class, not a class) [CO] |
| `E0508` | a constructor used as a member of an object: `p.point(1, 2)` [CO] |
| `E0509` | `-> base(...)`: no constructor of the base class takes the arguments [CO] |
| `E0510` | a function without a body called where no class of an object can supply one: `base->f()` (D83), or a `static` function of a class (D88) [WP, reported where the call is bound] |
| `E0511` | operator not defined for these operand types [CO] |
| `E0512` | operator on `object`: cast first [CO] |
| `E0513` | operator on an array [CO] |
| `E0514` | `- ! ++ --` on something they do not apply to [CO] |
| `E0515` | wrong number of type arguments in a generic function call [CO] |
| `E0516` | type arguments on a function that is not generic [CO] |
| `E0517` | a generic function called without its type arguments [CO] |
| `E0518` | a type argument of a generic function call does not satisfy `T base x` [CO] |
| `E0600` | a local declared twice; also two parameters with one name in one list - of a function, a constructor, a function without a body, a lambda (D90) [SF, CP] |
| `E0602` | `break` outside a loop [SF] |
| `E0603` | `continue` outside a loop [SF] |
| `E0604` | `return` / `break` / `continue` leaves a `finally` [SF] |
| `E0605` | not every path returns a value [SF] |
| `E0606` | `return value;` in a function that returns nothing [SF] |
| `E0607` | `return;` in a function that returns a value [SF] |
| `E0609` | `foreach` over something that cannot be looped over [SF] |
| `E0610` | the `foreach` variable cannot hold the items [SF] |
| `E0612` | `throw` of something that is not a `throwable` [SF] |
| `E0613` | `catch` of a type that is not a `throwable` class [SF] |
| `E0614` | the same class caught twice [SF] |
| `E0615` | `lock` on something that is not an object [SF] |
| `E0616` | a `when` value that cannot be compared with the subject [SF] |
| `E0617` | an injected local (`inject x : t = v;`) given a value as well [SF] |
| `E0618` | a local with neither type nor value (`inject x;`) [SF] |
| `E0619` | a clause after the `else` of a `when`, or a second `else` (D68) [SF] |
| `E0700` | private field, class, alias or enum used outside its class; a private global, class, alias or enum of a module used outside the global code of that module (a private function or constructor: `W1022`). Reported for names in expressions by the body stages and for type names by stage 3 (`type_access.sharp`) [EF, stage 3] |
| `E0701` | protected member used outside its class, its subclasses and its module; something protected in a module used from another module [EF, stage 3] |
| `E0702` | `local` declaration used outside its file [EF, stage 3] |
| `E0703` | instance field used without an object [EF] |
| `E0704` | instance function called without an object [CO] |
| `E0710` | (not used in this build: assignment to a `const` is the warning `W1021`. The number is kept for the day it becomes an error) |
| `E0711` | the target cannot be assigned to (a call, a literal, `self`, a type, an enum value ...) [CO] |
| `E0712` | compound assignment not available for this type [CO] |
| `E0713` | a setter cannot be used where `operator=` is needed [CO] |
| `E0714` | (not used: a property without a setter is assigned directly, 7.10) |
| `E0720` | `null` given to a non-nullable type [EF] |
| `E0721` | a value that may be null given to a non-nullable type [EF] |
| `E0722` | `.member` or `.call()` on a value that may be null [EF, CO] |
| `E0723` | `!!` on a type that is never null [EF] |
| `E0724` | `?:` fallback does not fit [EF] |
| `E0725` | `x?.member` where the result would be a plain number [EF] |
| `E0730` | (not used in this build: base to derived is the warning `W1017`. Kept for the day it becomes an error) |
| `E0800` | a lambda parameter without a type and nothing to take it from [LI] |
| `E0801` | a lambda has another number of parameters than expected [LI] |
| `E0802` | a lambda parameter's type differs from the expected one [LI] |
| `E0803` | a lambda's result type differs / its returns disagree [LI] |
| `E0804` | an instance function used as a value [LI] |
| `E0805` | a generic function used as a value [LI]; or named by an asm operand: it has no address (7.21) [LR] |
| `E0808` | a lambda where no function value can stand [LI] |
| `E0809` | `it` used where the lambda has no single parameter [LI] |
| `E0900` | a named component declared twice [LI] |
| `E0901` | two definitions of one component that provide the same type, or have the same name [LI] |
| `E0902` | a `get` with nothing that says what is wanted [LI] |
| `E0903` | no definition answers a `get` [LI] |
| `E0904` | the definition of the name asked for provides another type [LI] |
| `E0905` | a definition whose expression has no value [LI] |
| `E0906` | `inject`: no definition provides the type [LI] |
| `E0907` | (not used: a lambda definition is typed like any lambda, `E0800` - D77) |
| `E0908` | definitions that need each other in a circle [LI] |
| `E0909` | a `get` / `inject` names a component that does not exist [LI] |
| `E0920` | `asm`: `[name]` is not a local, field or function [SF] |
| `E0921` | `asm`: a label defined twice [SF] |
| `E0922` | `asm`: a label that is not defined [SF] |
| `E0923` | `asm`: `$type` / `$type[member]` not found [SF] |
| `E0924` | `asm` in a lambda: `[name]` is a local of the function around the lambda [LI] |
| `E0925` | `asm`: `[name]` is a local that is not in the stack frame - a lambda captures it, or it is `localized` [SF] |
| `E0926` | `asm`: a number wider than 32 bits, the widest operand of any instruction (`movi ebx, 5000000000`; the C++ compiler cuts it to its low 32 bits without a word). The limit is the signed range, so `0xFFFFFFFF` is refused too. For `movi` the help gives the line to write: the number in a local, read with `loadl <register>, [big]` (not `movi <register>, [big]`, which is the local's place in the frame) [SF] [completeness fixes] |
| `E0930` | two `main` functions [WP] |
| `E0940` | an enum value that is not a numeric constant [LI] |
| `E0950` | `obfuscate`: unknown option [LI] |
| `E0951` | `obfuscate`: a name that is no module, class, field or function; a function without its parameters, or with parameters no overload has [LI] |
| `E0960` | (not used: a field nothing gives a value is the warning `W1032`, D82. The number is kept) |
| `E0961` | `static_init` or `tls_init` of `platform.kernel#platform` has code in it [WP] |
| `E0962` | the program has a `main`, and a module or class the VM needs is not part of it (D89) [WP]. When the whole standard library is missing (no module `std`), it is reported right after the declarations are collected, ALONE, and the compile stops: every name of the library would otherwise be one more error (`println`, `string`, imports of `std.io`) [completeness fixes] |
| `E0990` | internal: the analysis met a node kind it has no rule for (8.C). Never expected; it means a stage is incomplete [EF, SF] |
| `E0103` | (parser) the code nests deeper than the parser follows: more than 256 statements and expressions open inside one another (`PARSER_NESTING_LIMIT`, frontend/syntax/tree/parse_state.sharp; counted in `parse_statement` and `parse_expression`), or - with the VM's default stack - deeper than the stack holds (about 120 parentheses in the compiler the C++ compiler built, fewer in the compiler built by itself: doc/codegen.md 11.8). Reported at the token reached, the rest of the file is not read; when it was the stack, a note says how to start the compiler with more (`sharp -istack 1mb`) |
| `E0992` | a body nests deeper than the analysis follows: more than 150 statements inside one another (`ANALYSIS_NESTING_LIMIT`, backend/compiler/context/reporting.sharp; counted in `analyse_statement`), or deeper than a pass over it can follow on the VM's stack (with the default stack: about 110 `if` blocks one inside the other, or 100 calls inside each other's arguments). A limit of the compiler, reported at the function, never a crash; when it was the stack, a note says how to start the compiler with more |
| `E0991` | internal: an expression the analysis left with the error type although nothing was reported about it; the first of each file is reported, so such a program never ends "analysis complete: no errors" (10.8) [CP] |
| `W1001` | one name for two different kinds of thing in one place [GM] |
| `W1002` | unreachable statement (not for the `return` the C++ compiler demands after an endless loop, 7.13) [SF] |
| `W1003` | redundant cast: to the type the value has, or from `t` to `t?` [EF] |
| `W1004` | redundant `!!` [EF] |
| `W1005` | unnecessary `?.` / `!!.` [EF] |
| `W1006` | `?:` on a value that is never null (not after `p?.x` with a number `x`: `W1027` says it, D100) [EF] |
| `W1007` | `static` where it is implied [GM] |
| `W1008` | a module imported twice in one import list [GM, reported by stage 2] |
| `W1009` | `local` together with an access word [GM] |
| `W1010` | `!` on a literal [CO] |
| `W1011` | an overloaded function name used as a value: the first was taken [LI] |
| `W1012` | a `static localized` field read through an object [EF] |
| `W1013` | several components answer a `get` / `inject` that names none: the first is taken [LI] |
| `W1014` | `get<t>("name", component)`: the type is not needed [LI] |
| `W1015` | a global `main` that is none of the four forms: it will not be run (D81) [WP] |
| `W1016` | `obfuscate` (not `-keep_inclusive` / `-inclusive`) lists a generic class without type arguments: nothing has that name [LI] |
| `W1017` | a base class value given where a class below it is wanted, without `as` (D6) [EF] |
| `W1018` | an expression statement, or the start expression / step of a `for`, with no effect [SF] |
| `W1019` | a `catch` that an earlier `catch` already covers [SF] |
| `W1020` | the same `when` value twice [SF] |
| `W1021` | a `const` assigned after its declaration (D7) [CO] |
| `W1022` | a private function or constructor used outside its class (D8); also a `local` / `private` / `protected` function of a module used where a variable declared so would be refused (D92) [CO, CP] |
| `W1023` | `x := <whole number>` where the number does not fit the `int` it becomes [EF] |
| `W1024` | a `-> base(...)` argument reads a field of the object before it has its value [LI] |
| `W1025` | an operator function with a number of parameters the operator can never be used with [GM] |
| `W1026` | an item that may be null written into a new array (`{ sure, maybe }`, `new node[] { sure, maybe }`): accepted as in the C++ compiler, the array is a `node[]` [EF] |
| `W1027` | `p?.x` read as a plain number: there is no "number or null", so it reads like `p!!.x` and the program throws when `p` is null (D54); as the value of a `?:` it says that the `?:` never runs, and no `W1006` follows (D100) [CO] |
| `W1028` | a `bool?` used as the condition of a statement: it is read like `flag!!` and the program throws when it is null (D66) [SF] |
| `W1029` | a function value passed as an argument whose parameters fit and whose result differs from the parameter's function type: accepted as in the C++ compiler, which compares only the parameters there (D69) [LI] |
| `W1030` | two values of one enum have the same number (D73) [LI] |
| `W1031` | field values that read each other in a circle, or a value that reads its own field: the first to run reads a start value (D70, D79) [LI] |
| `W1032` | a non-nullable object field nothing gives a value (D82); off unless -wextra [WP] |
| `W1033` | `new` of a class that still has a function without a body (D84) [WP, reported where the object is made] |
| `W1034` | a local variable never used after its declaration; off unless -wextra [WP] |
| `W1035` | a private field or function nothing uses; off unless -wextra [WP] |
| `W1036` | a local variable with the name of a field of its class: the field is hidden [WP] |
| `W1037` | a decimal the compiler knows (a whole number too big for 64 bits is one) stored in a whole-number type that cannot hold it (D97) [CP]. The headline names the value stored ("this number is stored in `_uint64` as 9223372036854775807"), not "does not fit": `18446744073709551615` FITS a `_uint64` - the problem is that written in digits it is a decimal. For a `_uint64` the help gives the hexadecimal bits, worked out from the digits (`0xFFFFFFFFFFFFFFFF`), and says the first Sharp compiler reads those differently (codegen.md G43) [completeness fixes] |
| `W1038` | a decimal literal too big for 64-bit floating point: it becomes infinity (D97) [CP] |
| `W1039` | a mistake in generic code nothing instantiates, found in its stand-in instance; the message is the error's own, whose code is named in a note (4.1, D99) [CP] |
| `W1040` | `public` / `private` / `protected` / `local` written on an alias: it has no effect; an alias in a class is always private, one in a module always public (7.24, D102) [C2] |
| `W1041` | a function with the name and parameters of a function WITH a body in a base class (an instance function below an instance function, a `static` one below a `static` one): it only hides that one, it does not replace it at run time (7.4.3, D44) [C2] |
| `E0333` | two `init(x)` conversion blocks of one class with the same parameter types (7.15.3) [LR] |
| `E0346` | a class lists an interface and declares one of its functions again WITHOUT a body (7.4.3) [LR] |
| `E0952` | a generic function listed in an `obfuscate` block: it has no one function to keep or rename (7.25) [LR] |
| `W1042` | an asm operand names an overloaded function: the address is of the first declared (7.21) [LR] |
| `W1043` | a whole number the compiler knows stored in a narrower whole-number type that cannot hold it (`b : _int8 = 200;`): it is cut (7.5.2) [LR] |
| `W1044` | `x / 0` with a constant whole-number zero: the result is infinity or not a number; `1.0 / 0.0` (a decimal zero) is how infinity is written on purpose and is quiet (7.8.1) [LR] |
| `W1045` | an assignment written as the whole condition of `if` / `while` / `for` / `do` (`if(x = 5)`); parentheses of its own silence it (7.12.1) [LR] |

### 9.3 How diagnostics are written

The bar is in `frontend/diagnostics/diagnostic.sharp` ("the rule for anyone
adding an error") and the brief: every diagnostic has a code, a message that
names the things involved in backticks, a label under the exact token(s),
and - wherever it helps - a related place ("declared here"), a note that
states the RULE, and a help that says how to fix it, with the corrected text
when it can be given.

**The sink.** Body stages do not add to a bare list. As built it is three
functions in `backend/compiler/context/reporting.sharp` (`problem_at`,
`problem_over`, `report`) over the session's `problems` and `reported`
(stage 6): `report(context, d)` drops a diagnostic whose code, file and range it has already
taken (generic code analysed once per instance, a field value analysed on
demand and again by the driver) and counts errors and warnings separately.
Warnings do not stop the build and do not prevent `analysis complete: no
errors` (`print_diagnostics` already counts only errors; `gates.sh` counts
lines that start with `error`, so warnings on the library do not fail the
gates). In a `tests/backend/fail` file the `// expect:` line lists warning
codes like error codes, in printed order; a `pass` file must print no ERROR
and may print warnings. Stage 5 is the first to report warnings and makes
`tests/backend.sh` follow this if it does not already.

Warnings on the library are information for the owner, not noise to be
hidden: `-analysis-stats` prints the number of each `W` code, and each stage
writes the counts for the library into its section of this document (the
`W1017`, `W1021` and `W1022` counts are exactly "how much code would break
if this became an error").

**One mistake, one message:** the error type (6.2). A stage that finds itself
writing "if the operand is not the error type" in many places should route
through `expect_value` / `convert`, which do it once.

**Fixed message shapes** (the stages fill these in; examples of the standard):

```
error[E0400]: there is no variable, field or function named `totl`
  --> shop.sharp:14:12
   |
14 |     return totl * 2;
   |            ^^^^ not declared in this function, in class `cart`, in module `shop` or in anything it imports
   |
   = help: did you mean `total`?                                  closest_names over everything visible
   = help: `totl` is a field of `order`; write `an_order.totl`    it exists, but elsewhere
   = help: `helpers#totl` exists; add `import (helpers)`          it exists, the import is missing
   = help: `this` is written `self` in Sharp                      another language's word
```

```
error[E0501]: no `add` takes (string, _int32)
  --> shop.sharp:20:10
   |
20 |     cart.add("pen", 3);
   |          ^^^ called with (string, _int32)
   |
   = note: add(item: product)                 takes 1 argument, 2 were given
   = note: add(item: product, count: decimal) argument 1 is `string` but `product` is expected
   = help: make a product first: cart.add(new product("pen"), 3)
```

```
error[E0722]: `owner` may be null
  --> pets.sharp:9:17
   |
 9 |     return owner.name;
   |                 ^ `owner` is a `person?`
   |
   = note: a `?` type has to be checked before a member is used
   = help: `owner?.name` gives null when there is no owner; `owner!!.name` insists there is one
```

Rules for particular kinds:

| kind | must contain |
|---|---|
| any name not found (variable, member, function, type argument, label, component definition) | `closest_names(name, candidates, 3)` + `did_you_mean`; candidates are everything that could legally stand there |
| found but not usable | which rule hides it and the way through: private ("declared `private` in `account`" + the declaration as related place), not imported (the `import` line to add), instance member in static code ("call it on an object, or make it `static`"), a member of an outer class (`outer.name`), declared further down (`E0407`) |
| type mismatch (`E0410`) | both types; what the target is ("parameter `count` of `add`", "field `size`", "the result of `area`"); and the nearest fix: `as t` for an unrelated-looking class that is really the same family, `.value` for a box, `.ordinal` for an enum value, `!!` / `?:` for a nullable, `"" + n` for a number to text, `new c(x)` |
| no overload (`E0501`) | every candidate with ONE reason each (`call_candidate.reason`), nearest miss first, at most 6, then "and n more" |
| ambiguous (`E0503`) | the tied candidates and what to write to choose (a cast on the argument, `module#f`) |
| operator (`E0511`) | the operand types; when the class has the operator for another right-hand type, say which; when the other operand order would work, say so |
| habits from other languages | `name_in_sharp` in `type_names.sharp` is the model: `this` -> `self`, `super` -> `base`, `&&=`, `x instanceof t` -> `x is t`, `(t) x` -> `x as t`, `x.length` on an array -> `sizeof(x)`, `var x = 1` -> `x := 1`, `function` / `fn` -> `def`, `nullptr` / `None` / `nil` as a value -> `null`. BUILT (10.11): the syntax habits in the package frontend/syntax/parser/habits (reported once, then parsed as the Sharp form), keywords with a slip in frontend/syntax/parser/habits/misspelt_keywords.sharp, `elif`, `switch`, keyword typos in call form, `len`, `math.sqrt` and `equals` in backend/compiler/names/other_language_habits.sharp |
| in generic code | when the type argument matters: the note "while checking `box<_int64>` (T is `_int64`)" |

Helpers the stages share (stage 6 creates them in `backend/types/program/node_helpers.sharp`
and `backend/compiler/names/not_found.sharp` and `no_member.sharp`): `error_at_node`, `error_at_range(first_node,
last_node)` for a whole expression, `place_of_node`, `place_of_declaration(d,
label)`, `describe(type)`, `describe_declaration(d)` ("field `x` of `point`").

---

## 10. The stages still to build

Each stage fills its section in when it is done: what was built (files), the
objects and fields it added, where it differs from this plan and why, the
result of the gates, the timing, and an honest "not done / known gaps" list.
A stage may refine the design of its own part; when it changes something
another stage relies on (sections 4 and 6), it changes this document in the
same step and says so in its section.

Common to all: `bash ~/sharp_work/tools/gates.sh` must end in `GATES: PASS`;
the library and the 175 suite cases give 0 errors; every rule enforced has a
line in a `tests/backend/fail/*.sharp` file and every accepted form a line in
a `tests/backend/pass/*.sharp` file; section 8 rows are ticked.

### 10.0 What "the library gives 0 errors" proves at each stage

The stages are built in order and each must pass the gates before the next
exists. The gates stay the same; what they PROVE grows, because a stage's
rules only meet real values once the stages after it have removed the
`bound_pending` nodes those values were hiding behind. This table is the
honest reading of a green gate at each point.

| after stage | the driver reaches | 0 errors on the library and the suite proves | it does NOT yet prove | `-analysis-stats` |
|---|---|---|---|---|
| 5 Generics and members | declarations only | every type in a declaration resolves; every generic instance NAMED IN A DECLARATION can be made (argument counts, constraints, nested instances, base classes under the environment); modifiers, accessors, constructor names, interfaces and the `mod` line follow the rules; `find_member` answers | anything about a body. Instances first named inside a body do not exist yet | instances made; members made (lazily) |
| 6 Expression foundations | every body and every field value, through the minimal walker | every NAME in the library is found (this is the strong part: `E0400`, `E0402`, `E0403`, `E0405`, `E0407` have no false positive on any name the library writes), every type written in a body resolves and its instances can be made, scopes and shadowing accept the library, access and static rules on fields accept it | the conversion and null rules, mostly: a value that contains a call, an operator or a lambda is `bound_pending` with the error type, and the error type converts to everything. Only values built purely from names, literals, members, casts and arrays are really checked | pending by kind: calls, operators, assignments, templates, lambdas, function names, `get`. Expected: well over half of all expression roots |
| 7 Calls and operators | the same | every call in the library binds to a function, every operator and assignment has a meaning; the conversion table and the null rules of stage 6 now see real values, so THIS is the stage at which a mistake in them shows as a false error | values that depend on a lambda or on a function name used as a value; statement rules | pending: lambdas, function names, `get`, calls waiting for a lambda. No call, operator, assignment or template |
| 8 Statements and flow | every body as statements | every statement form is accepted; `return` values convert; `E0605` has no false positive (and accepts the D12 cases the C++ compiler rejects); loops, `finally`, `when`, `foreach`, `asm` rules accept the library | bodies of lambdas | the same pending kinds as after 7; every function has a `bound_body` |
| 9 Lambdas and initializers | lambdas too; the definitions of components | all of section 7 except 7.15.4 and 7.23 | whole-program rules | 0 pending |
| 10 Whole-program checks | the finished program | everything | - | 0 pending; 0 unhandled node kinds |

Three rules follow from the table.

1. **A false error is fixed where the rule lives, by the stage that
   uncovered it.** Stage 7 will uncover mistakes in stage 6's conversion code
   (it is the first to feed it real values); it fixes them there, keeps the
   gates green, and lists them in its section. This is expected work, not a
   sign that stage 6 failed.
2. **The pending count only goes down.** Each stage writes the count for the
   library into its section. A later stage whose count for a kind it does not
   own is HIGHER than the stage before has broken something.
3. **A stage's own tests cannot lean on later stages.** The `pass` and `fail`
   files of stage 6 use only what stage 6 analyses (declarations with values
   made of names, literals, members, casts, arrays); a call in such a file
   would be pending and the test would pass for the wrong reason. Each stage
   also RE-RUNS the earlier stages' `fail` files - they must still give
   exactly their codes - and may add lines to them that needed its features.

Time: the brief's budget is about a second per stage on the library. Stage 5
and the body pass are the two that can exceed it; the measures already in the
design are members on first use (4.1), written types shared by all instances
(6.1), shared type and conversion objects (6.1, 6.5), a conversion cache per
class and overload lookup by name (7.6). A stage over budget profiles
(`-timings`, and a counter per analysis function behind `-analysis-stats`) and
fixes the cause; it does not skip bodies (decision 3, section 11).

### 10.1 Generics and members

**status: done, and reviewed** - built as described in section 3.5 (the steps,
the files, the objects, what later stages call). This section records where
it differs from the plan, what the independent review changed, what is
missing, and the numbers.

Scope: section 4.1, 6.1, 7.1, 7.4.3, 7.15.1, 7.17 to 7.19 (declaration side),
7.24, 7.26; inventory A1-A12, A100-A107, B1-B38.

**Built** (new files; `backend/` in front of each):
`types/type_system/resolved_type.sharp`, `types/type_system/type_environment.sharp`,
`postprocessor/generics/resolve_written_type.sharp`, `postprocessor/generics/type_use_messages.sharp`,
`types/program/well_known.sharp`;
`postprocessor/members/members.sharp` (the entry), `member_types.sharp`,
`generic_classes.sharp`, `instance_duplicates.sharp`, `generic_functions.sharp`,
`class_bases.sharp`, `override_families.sharp`, `override_family_messages.sharp`,
`member_lookup.sharp`, `modifiers.sharp`, `default_access.sharp`,
`constructors.sharp`, `accessors.sharp`, `interface_members.sharp`,
`extension_functions.sharp`, `function_forms.sharp`, `operator_functions.sharp`,
`name_conflicts.sharp`, `enum_values.sharp`, `components.sharp`,
`file_rules.sharp`, `import_groups.sharp`, `mutations.sharp` (the `mutate`
code, moved out of `type_names.sharp`).
Changed: the objects in `types/` (new fields), `reports/declaration_printer.sharp`
(the listing), `postprocessor/imports/imports.sharp` (groups), `type_names.sharp`
(`g#name`, extension functions see their class), `base_classes.sharp` and
`interfaces.sharp` (a type parameter as base / interface; the same generic
interface with other arguments), `overrides.sharp` (no `E0340` for an
extension function; remembers that `E0341` was reported; a generic function
is no promise; the certain match is preferred to the possible one),
`main.sharp` (the stage and its `members` time),
`frontend/diagnostics/render.sharp` (`count_errors`), `tests/backend.sh`
(warnings in `// expect:`). The frontend (lexer, parser) was not touched.

Tests: `tests/backend/pass/generics.sharp`, `pass/members.sharp`;
`fail/generics.sharp` (32 diagnostics), `fail/members.sharp` (42),
`fail/mutate_and_imports.sharp` (7), `fail/module_line.sharp` (1);
`fail/class_checks.sharp` (stage 4) had one line changed (`base T` is no
longer an error; `E0320` is shown with an alias for a number).

**Differs from the plan** (each also where the thing is described):

- *The work list.* `sharp_program.generic_instances` keeps every instance and
  is never emptied; a pass remembers how far it got (`instances_completed`).
  The plan's `pending_instances` was a list that is drained. (4.1 is updated.)
- *Real base classes* (`class_bases.sharp`, not in the plan). After stage 4
  the base of `class names base list<string>` was the blueprint `list`. For
  every real class `base_class` and `interfaces` now point at REAL classes
  (`list<string>`). That is what makes member lookup and the override check
  exact, and it is why `find_member` needs no type parameter bookkeeping at
  all. Blueprints keep the stage 4 links.
- *Override families* (`override_families.sharp`). The plan said "run
  `link_overrides_of` again for instances with reporting off". Instead the
  links of every real class are recomputed by comparing RESOLVED parameter
  types, which replaces the lenient "a `T` further up matches anything" of
  stage 4 by exact substitution. Stage 4 still does the reporting for classes
  as written (`E0341`-`E0344`); stage 5 adds only what exactness shows: an
  `override` that fits a `T` but not the type `T` stands for (`E0342`), a
  required function that only looked defined (`E0341`), a result that is
  another type with the real types (`E0343`). All are in `fail/generics.sharp`;
  the C++ compiler reports all three.
- *Where a check lives.* The plan's `type_arguments.sharp` does not exist:
  `E0350`, `E0353`-`E0357`, `E0368` are found inside `resolve_written_type.sharp`
  (their messages: `type_use_messages.sharp`) and `E0351`, `E0352` in
  `generic_classes.sharp`, next to the code that finds
  them. `E0380`-`E0383`, `E0393`, `W1008` are reported by stages 3 and 2,
  because a later stage could not undo what those stages do with a wrong
  `mutate` or import.
- *One type object per type.* `resolved_type` is interned further than 6.1
  asked: also `t[]`, `t?`, `t[]??` exist once per plain type (`with_marks`),
  so comparing two non-function types is a pointer test. The fields of the
  type table are `error_type`, `nil_type`, `decimal_type` (`nil` and `decimal`
  are keywords).
- *Open types.* `RT_TYPE_PARAMETER` is used for a single `T` (`parameter`
  set) and for "something that depends on a type parameter" (`list<T>` in a
  blueprint: `class_of` is the blueprint, `parameter` null). A blueprint's
  members are resolved like everybody's, which is what checks them; their
  open types are never used for anything else.
- *An alias has no `target_type`.* An alias inside a generic class stands
  for a different type in each instance, so its target is resolved where the
  alias is used.
- *`instance.name`* is the display text `box<string>`; the canonical text
  (`main#box<std#string>`, what 4.1 calls the full name) is `class_key(class)`.
  `full_name()` of an instance gives `main#box<string>`.
- *Extension functions for one instance* (`def box<string>.first()`) are
  supported: the C++ backend resolves the type before the dot with its type
  arguments, so the function belongs to that clone only.
- *Enum numbers.* `has_number` / `number` on the value (what the declaration
  fixes) instead of `constant`, which needs stage 6's `constant_value`. A
  value written with an expression that is not a whole-number literal is left
  to stage 9.
- *Not built here:* `leniency_severity` / `access_rules.sharp`. None of the
  three leniency warnings (`W1017`, `W1021`, `W1022`) can arise from a
  declaration; the first stage that reports one creates the function.
- *New:* `W1025`, `E0382`; `E0390` also covers a file with no `mod` line at
  all (the C++ compiler reports the same sentence for both); the modifiers of
  LOCAL variables are checked here too (`E0360` for anything but `excuse` /
  `const`), because the check is one loop over every modifier list.
- *Stage 5 is skipped when stages 1 to 4 reported an error.*

**What the independent review changed.** Every rule of the stage was probed
with small wrong and right programs against both compilers (about 180
one-mistake files, in the job folder `tmp/gmr`). Found and fixed:

| found | was | now |
|---|---|---|
| `class c : i<int>, i<string>` (the same generic interface, other arguments) | `E0325` "listed twice" - a false error; the C++ compiler accepts it and asks for both functions | two interfaces; `E0325` only for the same arguments, also when that happens in one instance only |
| `class mixin<T> base T`, `class tagged<T> : T` | `E0320` / `E0324` - false errors; the C++ compiler supports both and checks them per clone | supported; checked per instance (`E0320`-`E0324`) |
| `put(a: A)` + `put(b: B)`, two constructors, in `either<int, int>` | accepted | `E0331` / `E0332` for the instance (`instance_duplicates.sharp`) |
| an override whose RESULT only differs with the real types | accepted | `E0343` |
| `box<shape?>` for `T base shape` | accepted | `E0351` (the C++ rule: exact) |
| a class inside a generic class named from outside (`outer.inner`, `outer<int>.inner`, or through `base outer<int>`) | silently became the error type (the first report's known gap) | `E0368` |
| a function with a body, or a class, inside an interface | accepted (and a pass test used it) | `E0379` |
| `get { } get { }` on one field | the second silently replaced the first | `E0369` |
| `native def f() { ... }` | accepted | `E0365` |
| `def operator+(...)` at the top of a file | accepted | `E0366` |
| `enum e { }` | accepted | `E0367` |
| `mutate box<int> { }` | silently changed every `box<...>` | `E0383` |
| `def f<T>(x: T);` in a class / `override def f<T>` | `E0341` / `E0342` in words that did not fit | `E0345`; a body-less generic function is no longer "required" of the classes below |
| `alias box as b;` then `b<int>` | two errors for one mistake | one (`E0354` at the alias) |
| a type of a real class that stayed open | silent error type, only counted | `E0396` (internal; never seen) |
| `find_member` on a function repeated lower down without `override` | both returned | the nearest hides the same-parameter function above (7.4.3) |
| messages about an instance | stood in the generic class only | also show where the instance is used (`first_use`); long instance names are cut; `E0351` no longer suggests changing a library class |
| readability | `resolve_written_type.sharp` 490 lines, `type_names.sharp` 620, 33 functions without a comment | messages split out (`type_use_messages.sharp`, `override_family_messages.sharp`), `mutations.sharp` split out, every function commented |

**Rules checked against the C++ compiler** (one mistake per file, probes in
the job folders `tmp/gm/p` and `tmp/gmr`): `E0350`, `E0351` (also arrays and
nullable arguments), `E0353` (it says "could not resolve symbol"), `E0355` in
an instance (`list<_int64>` is refused by the
C++ compiler exactly as here: `list.find` returns `T?`), `E0356`, `E0357`,
`E0358`, `E0359`, `E0362` and the order it accepts (`public const static`
passes in both), `E0363`, `E0370`, `E0371`, `E0372`, `E0375`, `E0377`, exact
`E0341` / `E0342` / `E0343`, `E0325` / `E0331` / `E0332` in an instance,
`E0320`-`E0323` for `base T`, `E0368`. Accepted by the C++ compiler and
refused here: D30 (`static` on a constructor), D34, D35, D36, D39, D40, D43.
Accepted by both although odd (left alone): `override` on a function outside
any class; `interface j base a_class`; a `private` or `static` function
without a body; two parameters with one name (`E0600` since the completeness stage, 10.8);
`class a<T base T>`.

**Not done / known gaps**

- **For the owner: `E0352` (D35).** `class e<T> { deeper : e<e<T>>?; }` used
  as `e<int>` compiles with the C++ compiler, which makes generic clones only
  as far as something uses them. Here every instance gets its members, each
  names the next, and the chain is cut at 32 levels with an error. Nothing in
  the library or the suite does this. The alternative - members of an
  instance only when something uses it - would contradict decision 2 of
  section 11 ("every body of every instance is analysed").
- (fixed in 10.10) `def box<X>.f() : X` - an extension function that renames
  the type parameter of the class it extends - is accepted: `X` is the
  class's `T` (names/renamed_type_parameters.sharp).
- Stage 5 does not run when stages 1 to 4 reported an error, so a program
  with a stage 4 mistake gets its stage 5 messages only after that is fixed
  (`class x : cmp<int, int> { }` is told about the missing function first and
  about the two type arguments afterwards).
- The constructor without parameters is not added to interfaces (the C++
  compiler adds one to everything; an interface has no objects).
- `W1008` is found within one import list (as in the C++ compiler), not
  across two `import` lines.
- `W1007` / the modifier rules are not applied to an alias declared in a
  block (statement stage).
- Stage 4's `E0344` (`override` missing) is not re-asked with resolved types
  (the C++ compiler accepts that case: a function of the same name and real
  parameters without `override` two levels down).
- Enum values: only what a declaration fixes is numbered. `= "text"`,
  `= another_value` (`E0940`) and two values with one number are for the
  stage that analyses enum values; a component declared twice (`E0900`) and
  `inject` without a provider (`E0906`) came with stage 9 (10.5).
- Types written inside BODIES are not resolved yet (stage 6:
  `sharp_file.body_types`), so an instance that only a body names does not
  exist after this stage. That is the plan (10.0), not a gap of the stage, but
  it is why the library shows only about 50 classes made from generic classes
  here.

**Gates and timing** (full `gates.sh`, after the review):

```
library: 0 error(s), reached the end: yes
time: parsing 98 ms, declarations 153 ms, type names 161 ms, class checks 110 ms, members 321 ms
suite cases: 175 run, 0 failing
frontend tests: 13 passed, 0 failed
backend tests: 8 passed, 0 failed
GATES: PASS
```

`members` is the whole stage on the standard library alone (it varies by
about a third from run to run, like the other stages).

**A runtime fault that makes the gates flaky** (not in this compiler; found
while running them). When six compilers run at once - which is how
`gates.sh` runs the suite cases - about one compile in 150 dies in the VM
with `(SIGSEGV) found`, once with "Unhandled exception on thread
coroutine-worker" and a hang. Every stack captured shows the main thread in
stage 1 (`pre_process` -> `declare_function` -> `scope.add` -> `hash_map.put`,
allocating) and a worker thread at the last line of `fiber.fiber_main`: the
parse coroutines have delivered their results and the first collections of
the backend run. The same case always passes when run again or alone (a
failing case was run 12 times alone: 12 passes), and which case fails changes
from run to run. Measured: 10 crashes in 8 runs of the 175 cases; the full
gates passed 3 times and failed twice for this reason alone. (The review saw
it too: its first full run failed with one `SIGSEGV`, in `literals_big`,
before any code was changed.) It is not memory
(140 MB per compiler of 2.9 GB). Giving the fibers 20 ms to end before stage
1 did not change it, so that workaround was removed again. A gate that fails
with one `SIGSEGV` case should be run again; the fix belongs in
`lib/runtime`.

**FIXED (2026-11-14, the "Runtime crash" stage of the compiler stream).** The
cause was in the VM's JIT, not in the collector and not in this compiler. When
a fiber is continued after a context switch, the VM walks up its call stack and
gives every compiled function its machine frame back. It entered every
function that was compiled at that moment - also one the interpreter had been
running when it made its call and that another thread had compiled since (the
parse coroutines all run the same functions, so the tenth call of one of them
compiles it while earlier coroutines are waiting inside it at the lock in
`parse_all`). The callee, still interpreted, then ran in a nested interpreter
loop that, once the callee returned, carried on with the rest of the fiber, and
the outer loop went on past the last instruction of `fiber_main` - the worker
"at the last line of fiber_main" in every stack. The main thread's crash in
stage 1 was only a consequence: the worker's crash handler calls `exit()`,
which tears down the VM under the thread that is still running.
How it was found: a runtime built with the address sanitizer reports a
heap-buffer-overflow "0 bytes after" `fiber_main`'s 36 instructions in about
one bootstrap run in five (with `taskset -c 0-3`, five compilers at once);
instrumenting `main_vm_loop` showed a nested loop (depth 2) finishing the
fiber. The fix is one rule, `continues_in_machine_code` in
`lib/runtime/virtual_machine.cpp`: a function on the walk is continued in
machine code only when the frame of the call it waits in says it made that
call from machine code (`frame.native`). Proof: the sanitizer build failed 9
of 50, 12 of 60 and 6 of 30 bootstrap runs before (five at once on four cores),
0 of 100 after. The release build is far less sensitive - it reads a few bytes
past the code and usually gets away with it: before the fix it crashed 1 time
in 195 runs at six at once on four cores (`taskset -c 0-3`) and 0 times in 600
on six cores; after the fix 0 crashes in 1500 runs on four cores and 0 in 1500
on six cores. The suite case
`tests/suite/cases/lib_coroutines_resume_after_compile.sharp` builds the exact
shape (the sanitizer build of the old runtime failed it every run). The C++
compiler's own suite passes with the new runtime in all four configurations.
The independent check of this stage repeated the proof with sanitizer builds
of the UNCHANGED sources (the repository's runtime, and the same with only
`virtual_machine.cpp` put back to the version before the fix): before, 10 of
60 bootstrap runs stopped with the same heap-buffer-overflow at
`main_vm_loop`; after, 0 of 120, and no other sanitizer report. The release build
ran another 1500 bootstrap runs, six at once on four cores, without a crash. It also added
`tests/suite/cases/lib_coroutines_resume_shapes.sharp`, which puts the other
mixes on the stack of the waiting coroutine (two machine frames, seven frames
of one recursive function, the function on top, a hot loop after the wait, an
exception after the wait, a machine frame below an interpreted one): the old
runtime fails it under the sanitizer every run, the new one passes it with the
JIT, with `-nojit` and compiled with `-O`.
The suite has 177 cases with these two (it had 175).
`gates.sh` now runs the bootstrap on `~/sharp_work/sharp_dev/sharp` (the
runtime built from this repository) and a VM crash is a failure again,
reported as `VM CRASH on <case>`.

**Warning counts on the library:** `W1001` 19 (13 of them: a function
`iterator()` in a class of a module that also has the interface `iterator`;
the rest are `path`, `error`, `hash`, `day_of_week`, `coroutine_scope`).
`W1007`, `W1008`, `W1009`, `W1025`: 0. The suite cases add none of their own
that the library does not have.

**C++ compiler bugs found** (lib/grammar untouched):

- `stable class fixed { }  mutate fixed { n : int; }` at the top of a file
  compiles: "cannot mutate stable class" is only reached for a `mutate`
  written inside a class body (D36).
- `class box<T> { item : T; }  a : box?;` compiles: a variable whose type is
  a blueprint (D34).
- `class c { static c() { } }` compiles (D30, known from the design review).
- `class c { native def f() { } }` stops with "internal runtime_old error
  could not resolve function `f`" instead of a message about the body (D41).
- `def operator+(o: int) : int { return o; }` at the top of a file stops with
  "internal runtime_old error : unknown ast type: 16" (D41).
- `class box<T> { }  mutate box<int> { extra : int; }` compiles, and `extra`
  is then not found on any instance (D40).
- `alias int as n;  alias string as n;` in one module compiles (D43).
- `class a { def f<T>(x: T) : int; }  class b base a { }` compiles: a
  body-less generic function is never asked for (D39).

### 10.2 Expression foundations

**status: done and reviewed** (the review's changes are listed under "What
the review changed" below). The driver runs over every body of the library,
of the 175 suite cases and of the bootstrap compiler's own source with 0
errors.

#### What the stage does

`analyse_bodies` (`backend/compiler/driver/driver.sharp`) visits every
module's globals and global functions, every class that is not a blueprint,
and - as a work list that grows while it runs - every instance of a generic
class and of a generic function. For each field value and each body of a
function, constructor, init block, getter and setter it builds the typed
tree and stores it on the declaration (`sharp_field.bound_initializer`,
`sharp_function.bound_body`, `.locals`). `main.sharp` calls it after
`resolve_members`, only when the earlier stages reported no error, and times
it as `bodies`.

| piece | where | what it is |
|---|---|---|
| the driver and its work list | `compiler/driver/driver.sharp`, `class_bodies.sharp` | each body exactly once (`sharp_class.bodies_analysed`, `sharp_function.body_state`, `sharp_field.value_state`); blueprints, generic functions as written and instances that break their constraint are skipped |
| one function body | `driver/function_body.sharp` | context, parameters as the first locals, `-> base(...)` arguments, the body, the result type of `def f() := value` |
| one field value | `driver/field_value.sharp` | the value converted to the field's type; for `x := value` the type by the table of 7.3, also given to its getter and setter |
| types on demand | `driver/lazy_types.sharp` | `type_of_field`, `result_of_function`; a circle is `E0408` |
| session, context, scopes | `compiler/context/` | `analysis_session` (problems, counters, shared objects), `analysis_context` (6.6), `local_scope` (the scope chain; maps made on first declaration) |
| types written in bodies | `context/body_types.sharp` | `written_type_at` (once per tree node, in `sharp_file.body_types`), `resolve_type_at` (under the context's environment; makes instances); local aliases first |
| the diagnostics sink | `context/reporting.sharp` | `problem_at` / `problem_over` / `report`: the same code at the same place is reported once (generic code is analysed per instance); a `W` code is a warning; a problem inside an instance says which instance and where it is used |
| name lookup | `compiler/names/` | `look_up_name` (7.4.1), `find_class_member`, `check_access` (7.9; `check_access_to_global` for what a module declares), the messages for a plain name that is not found (`not_found.sharp`) and for a member / a name of a module that is not found (`no_member.sharp`); answers per class and per file are remembered (`remembered_name`) |
| access of type names | `postprocessor/names/type_access.sharp` (stage 3) | `check_type_access`: `private` / `protected` / `local` for every type written where a type is expected, in declarations and in bodies |
| conversions | `compiler/conversions/` | `find_conversion` (the table of 7.5.1, a pure question), `conversion_by_construction` (7.5.4, constructors and `init(x)`), `find_cast` (7.5.5), `as_plain_number` (7.5.6), `convert` (wraps or reports), `inferred_type_of` (7.3); `conversion_messages.sharp` (`E0410`, `E0720`, `E0721`) and `mismatch_explanations.sharp` (the rule in the way and the text to write instead) |
| expressions | `compiler/expressions/` and its families (`literals`, `primary`, `primary/members`, `operators`, `collections`) | `analyse_expression` by node kind; `analyse_value`, `analyse_converted`, `analyse_condition`, `analyse_whole_number` (reports `E0415` itself); what a later stage builds becomes `bound_pending` with its analysable parts below it |
| statements | `compiler/statements/` | the minimal walk: blocks and scopes, local variables and aliases, expression statements, `return`; everything else a `bound_unfinished_statement` holding its analysed expressions and walked statements (replaced by stage 8: 10.4) |
| the typed tree | `backend/types/typed_tree/` | 6.4 as built (below) |
| `-analysis`, `-analysis-stats` | `backend/reports/` | the typed tree of the LAST file on the command line; counts, pending by kind, warnings by code |

#### The typed tree as built (differences from the table in 6.4)

- It lives in `backend/types/typed_tree`, not `backend/compiler/bound`: the
  declarations own their trees, and `types` must not import the compiler.
- Field names: `on` for the object a field / call is read from (`object` is a
  keyword), `how` for the `conversion` of a `bound_conversion` / `bound_cast`.
- `bound_pending` has `what` (the syntax node kind, which the statistics
  count by) and `parts` (the operands, arguments, callee that ARE analysed).
- `bound_functions` (new): a function NAME with every candidate and the
  object it is called on; the callee of a pending call. Stage 7 turns it into
  the chosen function.
- `bound_error.inner`: what was analysed before the mistake showed.
- `bound_self.is_implied`: the programmer wrote `x`, not `self->x`.
- `bound_string_template` has `pieces` (text and expressions, analysed) and
  `appends_bound = 0`; stage 7 adds the `string.operator+` calls.
- `bound_safe_navigation.enclosing` links nested navigations (`a?.b?.c`).
- statements: `bound_block`, `bound_local_declaration`,
  `bound_expression_statement`, `bound_return`, `bound_empty`, and
  `bound_unfinished_statement` (temporary, for stage 8).
- `conversion` is in `typed_tree/conversion.sharp`; it has
  `only_nullable_in_the_way`, and `WHY_NULL`, `WHY_NEEDS_CAST`,
  `WHY_FUNCTION_SHAPE`, `WHY_NOT_CASTABLE` beside the reasons of 6.5.
  `CONV_LITERAL_TEXT` is not a kind of its own: a text literal becoming a
  `string` is a `CONV_CONSTRUCT` like any other.

#### Rules found by probing the C++ compiler in this stage (section 7 is corrected)

| rule | probe |
|---|---|
| a statement's condition takes a plain number or a `bool` object ONLY; `if(an_int)`, `while(a_double)` are errors. `? :`, an index and an array size take any number object | `if(i)` with `i : int` is rejected, `c ? 1 : 2` with `c : long` and `a[i]` are accepted (`compile_cond_expression` passes `"std#bool"`) |
| the plain name of an enum value resolves anywhere its enum is visible | `e : color = red;` in a global function compiles |
| `{ a, b }` takes its type from the FIRST item whatever is expected: `d : decimal[] = { 1, 2 };` is an error | "cannot assign field `b: decimal[]` type `_int32[]`" |
| `base` alone is a value (the object) | `x : a = base;` compiles |
| `point.x` for an instance field is an error also inside `point`'s own functions | "cannot access instance field from static context" |
| a class and a function of one name: in front of `.` the class is meant | `build.is_windows()` in a class that has a function `build` (library, `std/io/path.sharp`) |
| `_int64[] as object[]` is refused; `x as (a)(b)` is refused; `c as _int64` for an enum is refused | probes p9, p3, p8 |
| after a `?.`, later links of the chain that may be null need no mark | section 7.11; the library relies on it |
| a whole-number literal that needs more than 64 bits is a `decimal` (so `E0424` is not used) | `double.max_value > 1000000000000000000000` in the suite (`lib_math_primitives.sharp`) |
| a local alias (`alias t as name;` in a block) is accepted | probe; stage 3 reported it as an unknown type before this stage taught `find_in_enclosing_scopes` about blocks |
| (review) `private` on a global / a module-level class: only the GLOBAL code of its module; `protected`: the whole module, no other | a class reading a private global of its own module: "cannot access private field `main#__srt_global.own_private`"; a global function in another file of the module: accepted; another module reading a protected global: refused |
| (review) a `private` / `local` class or alias is refused in TYPE position too | `a : vault.plate? = null;`, `x : secret_class? = null;` from another file |
| (review) `"abc" as string` is refused | "cannot cast `string_literal: "abc"` to `std#string`" |
| (review) `sizeof(null)` is refused; an empty `{ }` is refused also where an array type is expected | "cannot get sizeof from expression of type `null`"; "could not determine the type of the array" for `a : _int64[] = { };` |
| (review) an array item that may be null is accepted (`{ sure, maybe }`, `new node[] { sure, maybe }`); `null` written out as an item is not | compiles / "cannot assign array item of type `unknown`" |
| (review) a class declared inside a class cannot be reached through an OBJECT (`v.plate.depth`); an enum, an enum value and an alias can | "cannot access inner class through field `v`" |
| (review) the items of a `t[]??` read as `t` in a declaration (`s : string = nullable_items[0];` compiles): the bootstrap keeps D25 and reports `E0721` | probe b1 |

#### Error and warning codes reported by this stage

`E0303` (module in `m#name`), `E0400`, `E0402`, `E0403`, `E0404`, `E0405`,
`E0406`, `E0407`, `E0408`, `E0409`, `E0410`, `E0411`, `E0412`, `E0413`,
`E0414`, `E0415`, `E0416`, `E0417`, `E0418`, `E0420`, `E0421`, `E0422`,
`E0600`, `E0700`, `E0701`, `E0702`, `E0703`, `E0720`, `E0721`,
`E0722`, `E0723`, `E0724`, `E0725`, `E0990`; `W1003`, `W1004`, `W1005`,
`W1006`, `W1012`, `W1017`, `W1023`, `W1026`. Meanings: section 9.2. Every
one except `E0409`, `E0701`, `E0702` and `E0990` has a line in
`tests/backend/fail/names.sharp`, `conversions.sharp`,
`null_and_operators.sharp` or `type_access.sharp` (the type-position checks
of stage 3); the accepted forms are in `tests/backend/pass/expressions.sharp`
(which the C++ compiler accepts too).

#### What the review changed

An independent pass over the finished stage: every rule in scope was probed
again with small wrong programs through both compilers (the probes are in
the job's scratch folder, `s6r/`), every message was read as rendered, and
the code was read file by file.

False negatives closed (the C++ compiler refuses, the bootstrap accepted):

- `private` / `protected` on what a MODULE declares was not checked at all
  (7.9). Now `check_access_to_global`.
- A `private`, `protected` or `local` class or alias named in TYPE position
  (`x : vault.plate;`, `new secret()`, a field's type) was never checked:
  only names in expressions were. New `postprocessor/names/type_access.sharp`,
  called from `resolve_type_name` - stage 3, because that stage visits every
  type name of the program exactly once.
- `"abc" as string`, `sizeof(null)`, an empty `{ }` given to an array
  variable, a class reached through an object (`v.plate`).

False positives removed (the C++ compiler accepts and gives a meaning):

- an array item that may be null was `E0721`; it is the warning `W1026` now.

Messages:

- `maybe.nothng` gave two errors (`E0722` and `E0402`); the member is looked
  up first now, so it is `E0402` alone.
- `x := x;` / `n : _int64 = n + 1;` said "there is no variable named `x`"
  and suggested other names; it is `E0407` "used in its own declaration".
- `E0400` finds a static member of ANY class with the name
  (`processor_count` -> "`platform_info.processor_count` exists ... add
  `import (std.system)`"), and never ends without a help.
- `E0402` with no close name lists what the class has (`color` has: `red`,
  `green`); an enum is called an enum.
- `E0410` always carries the text to write (`"" + n`, `{ "text" }`,
  `numbers[0]`, `new decimal[] { ... }`, `class rock : walker`); a text
  literal given to a `string[]` no longer talks about arrays of numbers.
- `E0415` says what the index IS and how to get a number from it
  (`.ordinal`, `!!.value`); `E0411` no longer suggests `null != null` or
  `sure != null` for a value that is never null; `E0721` puts a compound
  expression in parentheses in its `!!` help and has its own text for a
  `?:` whose fallback is nullable; `E0420`, `E0724`, `E0422`, `E0405` for
  `n.value` got a help each.
- `W1012` is reported (it was listed as a gap); `W1003` also for `x as t?`;
  `W1005` also for `point?.count`.

Structure: `not_found.sharp` and `member_expression.sharp` had grown past
one subject each and were split (`no_member.sharp`; the package
`expressions/primary/members` with `member_receiver.sharp` and
`member_messages.sharp`); `explain_mismatch` is its own file. Every function
of the stage has its comment. In `-analysis`, a member read from something
not typed yet is printed as "not known <- follows from the part below"
instead of "error <- reported as a mistake" (nothing was reported).

#### Where it differs from this plan, and why

- Packages instead of flat folders (the owner's instruction); the typed tree
  under `types`. `backend/dependency` became `backend/compiler/names`.
- The sink is three functions in `context/reporting.sharp`, not a class in
  `frontend/diagnostics`; `leniency_severity` is "a `W` code is a warning" in
  `report`.
- String templates are analysed here (pieces and type); only their appends
  wait for stage 7. A pending CALL already has its callee looked up, so an
  unknown function name is `E0400` now; `x.f()` on a nullable `x` is `E0722`
  now.
- A later array item that does not fit is `E0410` (through `convert`, with
  the purpose "an item of this array"); `E0418` is only "cannot tell the type".
- Name lookup looks for type parameters LAST (step 4), not first: a local
  named like a type parameter is not something programs do, and it saves a
  tree walk per name.
- No conversion cache per class yet (7.6): nothing asks often enough before
  overload resolution exists. Name lookups are remembered instead.
- Constant folding is only what literals and conversions give (a converted
  number, `sizeof("abc")`, an `is` known while compiling); operators are
  stage 7's.
- Statement objects exist already (four real ones and the unfinished
  holder), so `-analysis` can print a whole function.

#### Fixes to earlier stages

- `inherits_from` (stage 5, `inheritance/class_bases.sharp`) recursed without
  end when asked whether a class has an interface it does not have: from an
  interface it climbed to `_object_` and walked `_object_`'s interfaces
  again. It now stops at an interface's top.
- `find_in_enclosing_scopes` (stage 3) knows local aliases (above).

#### Not done / known gaps

- Everything "pending" (counts below): calls, operators, assignment, `new`,
  dictionaries, lambdas, `get`, function names as values, `o[i]` on objects,
  template appends, base constructor calls. Their operands ARE analysed;
  lambda bodies and `asm` blocks are not looked into at all.
- Because pending nodes have the error type, the conversion and null rules
  have met few real values (7,600 conversions on the library). Stage 7 is
  where a mistake in them shows (10.0).
- A conversion through a class with several equally good one-parameter
  constructors has `conversion.function == null` (270 on the library); stage
  7 picks or reports `E0503`.
- `n : int;` / a field without a value: `default_constructor` is not set
  (stage 8 / 9). `return` values are not checked against the result type.
- The variable of `foreach` over an OBJECT has the error type (its item type
  comes from `get_elements()`, a call).
- A local alias used INSIDE type arguments (`box<my_alias>`) resolves to the
  error type silently; used directly it works.
- Names inside a LAMBDA body are not looked up at all yet (the body is not
  walked until the lambda stage), so a misspelt name there is not reported.
- An instance of a generic FUNCTION is only made by a call (`gen<int>(x)`),
  and calls are pending: the bodies of generic functions are not analysed
  until stage 7 binds calls.
- The access check of a type name asks "which class is this written in?" by
  the base CHAIN only for `protected` (a protected class of another module
  named from a class that reaches its owner only through an interface is
  refused); and it treats the body of an extension
  function or `mutate` block as a use inside that class.
- The C++ warnings "unusual access of alias / static function through
  field" (`v.money`, `v.make` through an object) have no counterpart; both
  are accepted silently.
- `E0409` and `E0990` have no test (no program is known that reaches them);
  `E0701` and `E0702` need a second module / file, which one test file
  cannot have (both were checked by hand with two-file probes in the review).
- Time: `bodies` is about 1.0 s on the library (2.3 s on the bootstrap's own
  185,000 expressions: about 12 microseconds an expression), at the edge of
  the budget. The measured costs are allocation of tree nodes and hashing of
  names; remembering lookups took 20% off. The next candidate is one name
  table per function instead of a hash map per scope. The review did not
  make it faster.

#### Gates and timing

```
library: 0 error(s), reached the end: yes
suite cases: 175 run, 0 failing
frontend tests: 13 passed, 0 failed
backend tests: 13 passed, 0 failed
GATES: PASS
time: parsing 93 ms, declarations 136 ms, type names 159 ms, class checks 124 ms, members 327 ms, bodies 987 ms
```

(after the review. The first full run of the final tree failed with one
`SIGSEGV` of the VM in the suite case `variables`, in stage 1 - the known
run-time fault described in 10.1; the case passed 22 times out of 22 when run
again alone, and the full gates passed when run again. The bootstrap
compiler's own source, 185,000 expressions, also gives 0 errors: `bodies`
2.3 s.)

#### Pending counts on the library (`-analysis-stats`)

274 classes (61 generic instances), 10,884 function bodies, 657 field
values; 107,573 expressions, of which 35,426 pending (32%):

| kind | count | built by |
|---|---|---|
| binary | 15,688 | 7 |
| call | 6,795 | 7 |
| new | 5,638 | 7 |
| assign | 4,980 | 7 |
| postfix (`++ --`) | 1,487 | 7 |
| unary | 455 | 7 |
| base_call | 249 | 9 |
| string_template (appends) | 119 | 7 |
| lambda | 8 | 9 |
| identifier (a function name as a value) | 7 | 9 |
| index on an object | (counted under its own kind when met; 0 typed targets yet) | 7 |

Unfinished statements: 6,445 (if 3,610, for 845, throw 722, lock 655, while
340, asm 139, break 39, continue 39, foreach 29, try 21, when 4, do_while 2).

#### Warning counts on the library

Stage 6 reports none on the library (`W1001`: 19, from stage 5). On the
suite: `W1003` once (`casts.sharp`), `W1005` once
(`compound_assign_methods.sharp`). `W1017` is 0 so far - expected to move
when stage 7 gives calls their types.

#### C++ compiler bugs found

- The type of a field declared `name := new list<x>()` is resolved in the
  file that first USES the field: a file that reads `prog.module_list`
  without importing the module of `sharp_module` gets "could not resolve
  symbol `sharp_module`", reported at an unrelated line of that file.
- `base.name` (a dot instead of `->`) is accepted and means `base` alone:
  `s : string = base.name;` fails with "cannot assign ... type `circle`".
- `p?.x` with `x` a number is accepted (already listed, D24).

### 10.3 Calls and operators

**status: done, and reviewed** (see "What the review changed" below). Every
call, operator, assignment, `new`, `-> base(...)`, string template and
dictionary literal of the library, the 175 suite cases and the backend tests
is bound; the library gives 0 errors. What is still pending after this stage
is only what section 10.5 builds: function names used as values (7) and
lambdas (8).

#### What the stage does

| piece | where | what it is |
|---|---|---|
| overload resolution | `compiler/overloads/` | `resolve_overload(context, functions, arguments, type_arguments, receiver)`: the ranked candidate search of 7.6. First a quick pass looks for a candidate that takes every argument as it is (then no construction is tried for the others: the library calls `number.operator==` with twenty overloads thousands of times); then step 2 (applicable, constructions allowed), 3 (first exact), 4 (only one), 5 (plain-number rule), 6 (ranks), 7a (number grades), 7b (nearer class). `call_argument`, `call_candidate`, `overload_result` are the data (6.8) |
| generic function calls | `overloads/type_inference.sharp` | every generic candidate is replaced by its INSTANCE for the call (`instantiate_function`, stage 5): with the type arguments written, or worked out by unification of the parameters' WRITTEN types with the argument types (`T`, `T?`, `T[]`, `list<T>`, `(T)(R)`). `E0515`-`E0518`. The instance goes on `pending_functions`; the driver's work list analyses its body |
| messages | `overloads/overload_messages.sharp`, `type_argument_messages.sharp` | `E0501` lists every candidate with one reason each, nearest miss first, at most six; with one candidate it explains the mismatch like `E0410`. `E0503` lists the tied candidates and what to write to choose: a cast, or `new string("x")` where a cast cannot build the object. `E0515`-`E0518` are reported only when EVERY candidate failed on its type arguments; each says what to write |
| base constructor calls | `expressions/calls/base_constructor_call.sharp` | a constructor's `-> base(args)`: overload resolution among the constructors of the class directly above; a `bound_call` with dispatch `CALL_CONSTRUCTOR` kept in `sharp_function.bound_base_call` (`E0509`, `E0503`). Added by the review; the plan had it in stage 9 |
| calls | `expressions/calls/` | `analyse_call` by what is called: a plain name (`call_by_name.sharp`, the order of 7.7.1: local function value, the class's own name, a function-valued field, the class's functions, global function values and functions module by module, a class made without `new`); a member, a type, `base`, `module#name` (`member_calls.sharp`); any other expression as a function value (`value_calls.sharp`, `E0504`, `E0505`); `new t(args)` and `t(args)` (`new_expression.sharp`, `E0506`, `E0507`). `bind_call.sharp` turns the winner into a `bound_call`: arguments converted, `dispatch` (7.7.3), `W1022`, `E0704` |
| operators | `expressions/arithmetic/` | `analyse_binary` by the left operand (7.8.1): numbers (`number_operators.sharp`, result types with D13), operator functions of classes (`operator_calls.sharp`), `==` / `!=` on references and function values (`equality.sharp`), `&&` `\|\|` (`logical_expression.sharp`), `-a` `!a` (`unary_expression.sharp`), constant folding with the C++ literal types (`constant_folding.sharp`), the messages (`operator_messages.sharp`) |
| assignment | `expressions/assignment/` | the target is analysed and classified (`assignment_targets.sharp`: storage, property, object index, nothing); `=` into storage, through a setter, through `operator=` (or a construction when the target may hold no object); `o[i] = v` as `put(i, v)` or `operator=` on `o[i]` (`index_store.sharp`); compound assignment on numbers (with `store`, the conversion back), on properties (getter, operator, setter) and through `operator+=`; `++` / `--` on numbers, properties and through `operator++`; `const` (`W1021`) |
| `o[i]` on an object | `expressions/collections/index_expression.sharp` | `operator[]` by overload resolution: a `bound_call` `AS_INDEX` (`E0414`, `E0501`) |
| string templates | `expressions/literals/string_template.sharp` | the first piece as a `string`, each further non-empty piece appended by the `string.operator+` overload resolution picks (`appends`, `AS_TEXT_APPEND`); `E0423` |
| dictionaries | `expressions/collections/dictionary_expression.sharp` | `pair<k, v>` and `dictionary<k, v>` instances, their constructors, every key and value converted (`E0419`) |
| constructions | `conversions/class_conversion.sharp` | a conversion through a constructor follows the order of overload resolution: constructors before `init(x)`, the first that takes the value as it is, then rank and number grade. The 270 "undecided" constructions of stage 6 are 0; a tie that is left is `E0503` where the conversion is applied (`apply_conversion`, `convert.sharp`) |
| `-analysis` | `reports/analysis_printer.sharp` | prints every new node: the function a call chose, with its parameter types and dispatch; how an operator is worked out; properties; template appends |

#### The typed tree as built (differences from 6.4)

- `bound_binary.operation`, `bound_unary.operation`, `bound_assign.operation`
  hold the token kind (`operator` is a keyword). The new classes are in
  `typed_tree/bound_arithmetic.sharp` (binary, logical, unary, increment,
  assign) and `bound_calls.sharp` (value call, new).
- `bound_string_template` keeps `pieces` and has `start` and `appends`
  (`appends_bound` is gone). `AS_TEXT_APPEND` is a new `written_as`.
- `bound_dictionary.pair_constructor` is null for `{ as <k, v> : }` (no pairs).
- On a property, `bound_increment` / `bound_assign` have the property's FIELD
  as `target` (a `bound_field` with its object) and name the getter and setter.

#### Rules found by probing the C++ compiler in this stage (section 7 is corrected)

| rule | probe |
|---|---|
| the result of `def f() := value;` has a table of its own (7.3): a literal stays plain, a variable keeps its type, any other plain number becomes its number class | `def fc() := w * 2;` is `std#int`, `def fb() := w;` is `_int64`, `def fd() := sizeof(data);` is `std#int` |
| `def f() := g();` with `g` returning nothing returns nothing | `thread.sleep` in the library |
| two literals fold to a literal; comparisons, `/`, `**` and arithmetic with a decimal literal to a DECIMAL literal (7.8.1) | `x := 1 < 2;` is a `std#double`; `"" + (1 < 2)` prints `1.0000000000000000` |
| `null == x` is an error | "unqualified use of operator `==` with type `null`" |
| a generic function without its type arguments is not found | `gen(5)`: "could not resolve symbol `gen(_int32)`" (D53) |
| function types: the receiving side's parameters and result may have a `?` the given function's lack | `self->main = main;` with `(object[]?)(int)` and `(object[])(int)` in `std/io/thread.sharp` |
| `nul("x")` with `nul(string)` and `nul(string?)` is ambiguous (both build a string); `nul(new string("x"))` takes the first | the pass test |
| inside `either<L, R>`, `either<R, L>.left(...)` names the OTHER instance | `std/util/either.sharp` |
| `p?.x` with `x` a number is given to a number parameter in the suite | `nullability.sharp` (D54, `W1027`) |

#### Error and warning codes reported by this stage

`E0400` (a called name that exists nowhere), `E0402`, `E0414`, `E0419`, `E0423`,
`E0427`, `E0500`, `E0501`, `E0503`, `E0504`, `E0505`, `E0506`, `E0507`, `E0508`,
`E0509`, `E0511`, `E0512`, `E0513`, `E0514`, `E0515`, `E0516`, `E0517`, `E0518`,
`E0704`, `E0711`, `E0712`, `E0713`; `W1010`, `W1021`, `W1022`, `W1027` (new).
Every one has a line in `tests/backend/fail/calls.sharp`, `operators.sharp` or
`null_and_operators.sharp`. Every line of the first two
was compiled ALONE with the C++ compiler (the job's `s7/perline.sh`): it
rejects each, except where section 5 says the bootstrap is stricter
(`new printable()` D26, `cash - cash` D9, `a % 0` D55, `5++` D56) and the
warnings it does not have (`W1021`, `W1022`). The accepted forms are
`tests/backend/pass/calls_and_operators.sharp` (the C++ compiler accepts it
too) and `pass/calls_wider.sharp` (D3, D53: what only the bootstrap accepts).
The probe rows of 7.6 are lines of these two, and `-analysis` shows the
winners the table gives: `lit(_int32)`, `lit(_int64)`, `mix(_int32)`,
`ch(_int8)`, `nul(string)`, `two(animal)`, `of(decimal, decimal, decimal)`,
`dd(_int64)`, `obj(animal)`; `three(an_animal)` is `E0503`.

#### Fixes to earlier stages

- `look_up_name` (stage 6): the class's own name followed by type arguments
  (`either<R, L>` inside `either<L, R>`) is looked up with those arguments.
- `find_cast` (stage 6): an array of function values can be cast to and from
  `object` / `object[]` (`copy(...) as T[]` in `list<(int)(string)>`).
- `close_safe_navigation` (stage 6): D54.
- `inferred_result_of` (new, `conversions/inferred_types.sharp`) and
  `analyse_expression_body` (`driver/function_body.sharp`): the table of
  `def f() := value;`.
- `source_text_of`: a call or index quoted in a message keeps its closing
  `)` / `]` (`add(1, 2)`, not `add(1, 2`).
- `function_types_fit` (`conversions/implicit_conversion.sharp`): the rule
  above for function types.

#### What the review changed

An independent pass over the finished stage: the gates were run again, every
rule in scope was probed with about 250 small wrong lines and three files of
correct code through BOTH compilers (the job's scratch folder `r7/`: `a1` -
`a5` wrong, `b1` - `b3` right, `c1` run for its output; each wrong line
compiled alone with the C++ compiler), every new message was read as
rendered, and the code was read file by file.

False negatives closed (the C++ compiler refuses, the bootstrap accepted
silently):

- `-> base(args)` was only analysed, never bound: `dog() -> base(1, 2)` with
  no such base constructor compiled. Now `base_constructor_call.sharp`,
  `E0509` (249 calls in the library, all bound).
- `print = null;` - the name of a FUNCTION on the left of `=` - said nothing
  (a function name has the "not known yet" type, which the target check took
  for "already reported"). Now `E0711`, with what to declare instead.
- `(count ?: 1) = 2;` assigned to `count`: for a value that is never null
  `?:` hands back the value itself, and that was taken for the place. A target
  must now also be WRITTEN as a place (7.10).
- `p.point(1, 2)` was `E0402` "no member named `point`"; it is `E0508` now.

False positives removed (the C++ compiler accepts and gives a meaning):

- `title() = "x";` / `a.get_name() = "z";` was `E0711`. An object that a call
  gave back is changed through its class's `operator=` when one takes the
  value (7.10) - run with the C++ compiler: the name changes.

Decided the other way, on purpose: `maybe?.age = 5;` stays `E0711` (D58), and
`make() = other;` with no `operator=` for the value stays `E0711` (D59).

Messages:

- `maybe?.age = 5;` gave `W1027` AND `E0711`; the warning is not given for
  the left side of an assignment.
- A call of a generic function with the wrong NUMBER OF ARGUMENTS (`identity(1, 2)`)
  said `E0517` "the type arguments cannot be worked out"; it is `E0501` with
  "takes 1 argument, 2 were given". `E0517` names the type parameter nothing
  says anything about; `E0515` / `E0516` label the type arguments, not "called
  with ()", and each of the four has a help with the text to write.
- `E0506` for `new dog("rex")` when only the BASE class has that constructor
  adds: "constructors are not inherited ... Add it: `dog(n: string) -> base(n) { }`".
- `E0503` suggested `"x" as string` and `'c' as int` - casts that are
  themselves errors; it suggests `new string("x")` / `new int('c')` where an
  object would be built. For `nul(string)` / `nul(string?)` it no longer
  prints the same text twice.
- A function name passed where a number is expected was called "a lambda";
  a text literal was "`_int8[]`" in one line and "`_int8[] (text)`" in the
  next - both are "a function" / "a text literal" throughout now.
- `n += "x"` suggested `"" + n + "x"` and `n += an_object` suggested
  `an_object += n` (helps meant for `a + b`); `n ** text` suggested putting
  the text first; `null == null` suggested writing `null == null`;
  `n + null` had no explanation. Each has its own text.
- `any++` on an `object` suggested `(any as <type>) ++ ...`.
- `E0711` explains each kind of non-place (a function, a type, a `?.` chain,
  what `operator[]` gave back) and, for a plain `=`, says that comparing is `==`.
- `E0712` on an object says what to write: `a = a + b` when the class has
  that `operator+`, else the `operator+=` to declare (for a class of the
  program's own module).
- A number or text given where a FUNCTION VALUE is wanted (`each(items, 5)`)
  said "`(_int64)()` has no constructor that takes one"; text given where a
  number is wanted said "an object does not become a number by itself".
- `E0509` is placed over the whole `-> base(...)`, not on the `->`.

Structure: `type_argument_messages.sharp` and `base_constructor_call.sharp`
are new files of their packages (both front-door headers list them);
`assign_operator_call` was split into choosing the `operator=` and calling
it, so that the returned-object form shares it. The quick pass of
`choose_overload` keeps what it found, so a candidate that is applicable
without constructions is not asked a second time, and a candidate with the
wrong number of parameters costs nothing in the quick pass.

#### Not done / known gaps

- Function names used as values and lambdas are pending (stage 9). `x := a.speak;`
  (the name of an INSTANCE function as a value) is therefore not yet refused;
  the C++ compiler says "cannot get address of non-static function". A call
  whose candidates differ only in what they want from a lambda argument stays
  a pending call (none in the library).
- A constructor that writes no `-> base(...)` has no `bound_base_call`: the
  implied call of the base constructor without parameters, and the
  constructors the compiler adds, are stage 9's (10.5). `W1024` (a
  `-> base(...)` argument that reads a field of the object) is stage 9's too.
- (done by stage 9, 10.5) `get<t>()` (components) stays a pending node until the whole-program stage
  (10.6): its type alone could be given now, but whether a component provides
  it cannot.
- `sharp_class.conversion_cache` (7.6, "Cost") is not built: `-analysis-stats`
  counts 66,000 conversion questions for 107,000 expressions on the library,
  too few for a cache to matter.
- An assignment through `?.` is `E0711` by decision (D58), not bound.
- Type arguments worked out from the arguments: the first argument that says
  something about `T` decides it; no common type of several is looked for. A
  text literal makes `T` an `_int8[]`, not a `string`.
- Time: `bodies` is 1.7 - 1.8 s on the library (1.0 s after stage 6). The
  stage has no line of its own in `time:` because stages 6 to 9 are one walk
  over the bodies (section 2). The 35,000 expressions that were pending after
  stage 6 - with the error type, which stops every member lookup and
  conversion after it - are now analysed for real: 107,000 expressions,
  43,000 name lookups and 66,000 conversion questions in 1.8 s is about 17
  microseconds an expression on the VM. No single hot spot was found: keeping
  the quick pass's results (above) changed the time by less than the run-to-run
  noise of about 0.1 s.
- The VM's intermittent SIGSEGV (lib/runtime) hits one or two of the 175
  suite cases in most full gates runs, a different case each time
  (`boxed_numbers`, `lib_collections_bit_set`, `lib_coroutines_jit_fiber_frames`,
  `lib_util_holders` so far); each passes when run again alone.

#### Gates and timing

```
library: 0 error(s), reached the end: yes
suite cases: 175 run, 0 failing
frontend tests: 13 passed, 0 failed
backend tests: 17 passed, 0 failed
GATES: PASS
time: parsing 102 ms, declarations 179 ms, type names 139 ms, class checks 101 ms, members 323 ms, bodies 1828 ms
```

#### Pending counts on the library (`-analysis-stats`)

107,573 expressions, 15 pending: `identifier` (a function name as a value) 7,
`lambda` 8. Constructions where several constructors fit: 0 (270 after
stage 6). Unfinished statements (stage 8): 6,445.

#### Warning counts on the library

`W1022` 22 (the private setter of `number.value` assigned from `atomic`, and
`thread.set_finished` called from `vm`), `W1005` 4, `W1001` 19 (stage 5).
`W1017` 0 and `W1021` 0: no library code relies on an implicit downcast or
assigns a `const`.

#### C++ compiler bugs found

- `three(an_animal)` with `three(c: cat)` and `three(d: dog)` - an ambiguous
  call - makes the C++ compiler crash (segmentation fault) instead of
  reporting "ambiguous".
- `new plain()[0]` does not parse in the C++ compiler ("unexpected symbol
  `[`"); with a variable it does.
- Found by the review: `nonnull_in(null)` with `def nonnull_in(a: animal)` -
  `null` given to a parameter that cannot be null - crashes the C++ compiler
  (segmentation fault) instead of reporting it. `f()` and `g(1)` on function
  values `f : (_int64)(_int64)`, `g : ()(nil)` - a wrong number of arguments -
  report the error and then abort ("Aborted").
- `maybe?.age = 5;` is compiled as `maybe!!.age = 5` (D58).
- `def animal.shout() : string { return name + "!"; }` - an extension function
  that reads a field of its class by its plain name - is "internal error:
  unknown ast type: 16" followed by errors about `std.io#file.name`.

### 10.4 Statements and flow

**status: done.** Every statement of every function, constructor, init
block, getter and setter body in the library, the 175 suite cases and the
bootstrap's own source is a typed-tree object (the stage-6 walker and its
`bound_unfinished_statement` are gone); the flow analysis runs over all of
them. The library gives 0 errors. What is left unanalysed is only inside
lambdas (stage 9).

Scope: 7.3 (locals), 7.12, 7.13, 7.21; inventory A60-A71, A130-A133,
B150-B171 (ticked).

#### What the stage does

| piece | where | what it is |
|---|---|---|
| the statement tree | `types/typed_tree/statements/` (a package of its own) | one class per statement (6.4, rewritten as built): block, local declaration, expression statement, empty; if, when and its clauses; the four loops on a shared `bound_loop`; return, break, continue, throw with `leaves`; try and its catches, lock; asm with its instructions and operands. The `BS_` kinds and `statement_kind_name` are in its front door |
| the dispatcher | `compiler/statements/statements.sharp` | `analyse_statement(context, node, into)` by node kind; `analyse_block` (a scope); `analyse_part` - the body of an `if`, a loop, a `catch`: a block, or one statement in a scope of its own; `E0990` for a statement kind without a rule |
| locals | `statements/local_declaration.sharp`, `local_alias.sharp` | as stage 6 built them, plus: the start object of `n : int;` / `s : string;` (`default_constructor`), `inject` on a local (`is_injected`, `E0617`, `E0618`); a local alias declared twice is `E0600` |
| expression statements | `statements/expression_statement.sharp` | `W1018` when the expression changes nothing (a call, `new`, an assignment, `++` / `--`, or one of those in a part that runs, does): `total + 1;`, `count == 0;`, a function name alone - each with the statement meant |
| branches | `statements/branches/` | `if` (conditions by `analyse_condition`, 7.12.1); `when`: the subject in a hidden local, each value bound as "subject == value" by the rules of `==` (numbers, `operator==`, the same object, function values) and made a condition (`E0616`); without a subject each value is a condition (`E0411`); `W1020` for the same constant or enum value twice |
| loops | `statements/loops/` | while, do-while (the condition after the body's scope), for (the start part's variables for the whole loop, the step after the body), foreach (`iterable.sharp`: an array, or a class that is `loopable<t>` through any base or interface - the bound call of its `get_elements()`, `AS_ITERATE`; `E0609`. The variable: the item type, or a written type the items convert to, `E0610`; hidden locals for the array and the position); `is_endless` for a missing or literal-true condition |
| jumps | `statements/jumps/` | the stack of jump frames on the context (`context/jump_frames.sharp`): `break` / `continue` find their loop (`E0602`, `E0603`, with the `when` and lambda cases explained), every jump collects the locks and try-with-finally it passes (`leaves`), and leaving a finally block is `E0604`. `return` against `expected_result` (`E0606`, `E0607`, conversion `E0410` / `E0721`); `return nothing();` in a function that returns nothing is accepted, as in the C++ compiler |
| exceptions, lock | `statements/exceptions/` | `throw` (`E0612`); `try` sets the frames (`FRAME_GUARDED` around the body and catches when there is a finally, `FRAME_FINALLY` around it); each catch (`E0613`, `E0614`, `W1019` for a class an earlier clause already takes; `catch(t)` without a name); `lock` (`E0615`, a hidden local for the object) |
| asm | `statements/assembly/` | labels collected first (`E0921`, `E0922` with did-you-mean), operands resolved: registers, numbers, text, labels, `[name]` by plain-name lookup (`E0920`), `$type` / `$type[member]` (`E0923`) |
| the flow analysis | `compiler/flow/` | `check_flow(session)`, a pass of its own after the bodies (its own entry, `flow`, on the `time:` line), over `session.analysed_functions`: `flow_of` sets `completes` on every statement (the table of 7.13), marks loops left by a reachable `break` or repeated by a reachable `continue`, gives `W1002` once per block; `E0605` on the closing brace with the path that reaches it (`path_through`: an `if` without `else`, a `when` without `else`, a loop that can end, a `catch` that ends ...) |
| the driver | `driver/function_body.sharp` | sets `expected_result` (a getter's is its field's type, worked out on demand), calls `analyse_statement`, records every analysed body for the flow pass |
| `-analysis`, `-analysis-stats` | `reports/statement_printer.sharp`, `analysis_stats.sharp` | every statement with what it means: where a jump goes and what it passes, endless loops, hidden locals, catches, asm operands, "(does not complete)"; the count of statements by kind |

#### The typed tree as built (differences from the plan's 6.4)

6.4 is rewritten to the classes as built. Against the first table:

- The statements are a package of their own (`typed_tree/statements`, seven
  files); `typed_tree` alone would have had thirteen.
- `bound_block` has no `scope` (scopes are the compiler's; `types` may not
  import it, and a generator needs only the slots in `locals`).
- The loops share `bound_loop` (`body`, `is_endless`, and `left_by_break` /
  `continued`, which the flow analysis needs to know where a reachable jump
  goes). `bound_foreach` has `item_type`, `item_conversion` (the plan's
  `element_conversion`) and the two hidden locals.
- `bound_lock.locked` (`object` is a keyword) and `held`; `bound_catch.caught`.
- `broken` on return / break / continue (one mistake, one message in the flow).
- `bound_asm` names its labels, and an operand is a `bound_asm_operand` with
  a kind.
- `local_scope.statement` / `in_finally` and `analysis_context.returned` were
  not built: the jump frames replace the first two, and no function body
  infers its result from `return`s.

#### Rules found by probing the C++ compiler in this stage (section 7 is corrected)

Probes, one mistake per file (`perline.sh` in the job's `sf` folder), and
whole files compiled and run:

| rule | probe |
|---|---|
| `return nothing();` in a function that returns nothing is accepted | `def f3() { return nothing(); }` compiles |
| `break` / `continue` outside every loop crash the C++ compiler | segmentation fault (D61) |
| the variable of a `for`, a `foreach`, a `catch` collides with a local still in scope | "local field `a` is already defined in function" for each |
| a second local alias of one name in a function | "alias `s` is already defined in function" |
| a `when` value that cannot be compared is reported through `==` | `"a"` against a number, `5` against an object, a call of nothing |
| a text-literal value against a `string` subject compares the text | `when(s) { "a" -> ... }` compiles |
| a catch of an array, a number, a class that is not throwable; the same class twice | four different messages, one code each here |
| a clause after one for a base class, the same `when` value twice, an expression statement that does nothing | accepted silently (`W1019`, `W1020`, `W1018`) |
| an array held in a variable cannot be locked; `null` cannot | D62; "non-lockable object of type `null`" |
| `foreach` over `object`, a number, a class that is not loopable | "foreach expression ... must be an array type" |
| an asm label twice, a jump to a label never defined | accepted (D63) |
| `$_int64` in asm; `$plain[nowhere]` | "invalid asm type: _int64, expected class"; "attempt to get address of a member" |
| the old "not all code paths" check | refuses `{ { return 1; } }`, `while(true) { }`, `do { } while(true);`, `for(;;) { }`, and accepts `if` / `else`, `try` / `catch`, `when` / `else` whose every branch returns (D12) |
| `for(i = 0; ...)` with an existing variable | does not PARSE in the C++ compiler; the bootstrap parser reads it, so it is analysed here (a parser difference, `pass/flow_wider.sharp`) |
| a local declared without a value and read | accepted, no check: no definite-assignment analysis of locals in the C++ compiler |

#### Error and warning codes reported by this stage

`E0406` (a `when` subject without a value), `E0410`, `E0411` (conditions),
`E0600`, `E0602`, `E0603`, `E0604`, `E0605`, `E0606`, `E0607`, `E0609`,
`E0610`, `E0612`, `E0613`, `E0614`, `E0615`, `E0616`, `E0617` (new),
`E0618`, `E0619` (added by the review), `E0920`, `E0921`, `E0922`, `E0923`,
`E0990`; `W1002`, `W1018`, `W1019`, `W1020`, `W1028` (added by the review).
`E0409` is retired (its one use is `E0618`). Every code but
`E0990` (no program reaches it) has a line in `tests/backend/fail/statements.sharp`
or `fail/flow.sharp`; each line was compiled ALONE with the C++ compiler,
which rejects it except where the file's comments say (the warnings, D61,
D63). The accepted forms are `pass/statements.sharp` (the C++ compiler
compiles and runs it) and `pass/flow_wider.sharp` (D12, D62 and the `for`
start expression: what only the bootstrap accepts).

#### Fixes to earlier stages

- `statements/local_declaration.sharp` (stage 6): `E0409` became `E0618` (the
  code the plan gave it); `localized`, written directly under the variable, is
  read; `report_if_already_local` is also asked for the variables of `for`,
  `foreach` and `catch`.
- `context/analysis_context.sharp`: `expected_result`, `jumps`,
  `enter_frame` / `leave_frame`, `hidden_local`. `analysis_session`:
  `analysed_functions`; the counters of statements by kind replace the
  "unfinished" ones.
- `typed_tree/bound_node.sharp`: the `BS_` kinds moved to the statements
  package; `bound_statements.sharp` and `compiler/statements/unfinished_statement.sharp`
  are deleted.
- `reports/analysis_printer.sharp`: the statements moved to
  `statement_printer.sharp`.

#### Where it differs from the C++ compiler

D12 (every path returns, by a flow analysis), D61 (`break` / `continue`
outside a loop), D62 (locking an array in a variable), D63 (asm labels), D64
(more unreachable code is found), D65 (a loop inside a finally block), D66
(a `bool?` condition), D67 (an array thrown), D68 (the `else` of a `when`
must be last). And the warnings the C++ compiler does not
have: `W1018`, `W1019`, `W1020`.

#### Not done / known gaps

- Lambda bodies are still not walked (stage 9). They will use this package as
  it is; what they need on top is the inferred result of a lambda (its
  `return`s) and `expected_result` for a lambda.
- No definite-assignment analysis of LOCALS: `s : shape; s.area();` compiles
  and throws when it runs, as in the C++ compiler. (The fields' version is
  7.15.4, stage 10.) A warning would be the next step.
- Constructors and init blocks get their bodies analysed and flow-checked
  here; the implied base call, the constructors the compiler adds and the
  order of a class's initialisation are stage 9's (10.5).
- What `try` / `finally` needs at run time (the exception being passed on,
  the address to return to after a finally) is not made a hidden local: the
  code generator can derive it from the tree (6.4).
- A FUNCTION NAME or a LAMBDA written where a statement needs a value of a
  certain kind - `if(refresh) { }`, `throw refresh;`, `foreach(x in refresh)`,
  `x : _int64 = refresh;` - is not reported yet: until stage 9 such a value is
  a `bound_pending` with the error type, which passes every check silently.
  Nothing has to change here: once stage 9 gives the value its function type
  where `as_value` makes the pending node today, the checks of this stage
  (`E0411`, `E0612`, `E0609`, `E0615`, `E0410`) see it. Stage 9 must test
  exactly these five lines (they are in `tests/backend/pass/flow_wider.sharp`
  only as a comment, because today they are silently accepted).
- `return; 5;` gets `W1002` (never reached) and `W1018` (does nothing): two
  messages for one dead statement. They come from two passes; merging them
  needs the flow pass to know what the statements stage already said.
- A `when` whose values are of another enum than the subject
  (`when(colour) { size.small -> ... }`), or objects of an unrelated class,
  is accepted as in the C++ compiler ("the same object?" is always a legal
  question). A warning would be the next step.
- `[name]` in asm naming an overloaded function takes the first of that name,
  as the C++ compiler does (its `[name+n]` offsets do not parse in the
  bootstrap).
- `foreach` over a NULLABLE loopable object calls its `get_elements()`
  without the null rule of `.` (`E0722`): looping over a nullable is allowed
  (7.11), and a null throws when it runs, as for an array.
- `E0990` has no test: no program reaches it.

#### The review of this stage (an independent pass over the finished work)

What the reviewer probed, found and changed. About 130 small wrong programs,
one mistake per line, each line compiled alone with the C++ compiler and all
of them with the bootstrap (the probes are in the job's `sf` folder, `rv_*`).

| found | what was done |
|---|---|
| `when(n) { else -> ... 1 -> ... }` and two `else` clauses were accepted silently (the C++ PARSER refuses both) | `E0619` in `branches/when_statement.sharp` (D68) |
| `if(flag)` with `flag : bool?` was `E0411`; the C++ compiler accepts it and the program runs | accepted, read as `flag!!`, with `W1028` (D66; `analyse_condition` in `expressions/values.sharp`) |
| the step of a `for` (and a start expression) that changes nothing was not reported | `W1018` for both (`loops/for_statement.sharp`); the warning now says "this step does nothing" |
| `E0606` in a setter said "`size` returns nothing"; in a constructor and an init block it named them like functions | "the setter of `size`", "the constructor `maker`", "an `init` block" (`result_owner_text`) |
| `E0605` for a getter put the backticks around the whole phrase ("`the getter of size`") | "the getter of `size`" |
| `E0612` / `E0615` called a text literal "an array (`_int8[]`)" | "a text literal" (`described_value`) |
| `E0609` had no help for `null` and other values; the help for `object` used a made-up variable name; the `loopable` example lacked `override` | a help in every case |
| `E0920` and `E0923` had no help when no similar name exists; `E0923` did not say what `$type[name]` means | a note with the rule, and a help with what to write, in each case |
| every new file imported a broad set of modules (98 imports that nothing in the file used) | removed, for every file of `compiler/statements`, `compiler/flow`, `typed_tree/statements` and `reports/statement_printer.sharp`; checked by building, and by the bootstrap compiling its own source (which enforces imports): 0 errors |
| a loop inside a `finally`, an array thrown, `lock("text")`, `catch(e: oops?)`, a class with a base whose constructors all take arguments and no `-> base(...)` | probed: D65, D67 written down; the others are accepted by both compilers (the last one because the C++ compiler gives every class a constructor without parameters - stage 9 binds the implied base call, 10.5) |

The tests grew with the review: `fail/statements.sharp` 63 codes (37 before),
`fail/flow.sharp` 32 (18 before), `pass/flow_wider.sharp` D65. Every marked
line was again compiled alone with the C++ compiler.

#### Gates and timing

```
library: 0 error(s), reached the end: yes
suite cases: 175 run, 0 failing
frontend tests: 13 passed, 0 failed
backend tests: 21 passed, 0 failed
GATES: PASS
time: parsing 95 ms, declarations 141 ms, type names 126 ms, class checks 60 ms, members 410 ms, bodies 2037 ms, flow 23 ms
```

The flow pass costs 20 - 40 ms on the library; the statements add nothing
measurable to `bodies` (1.67 - 2.1 s across runs on a machine whose timings
vary by a quarter from run to run; 1.78 s before this stage). On the
bootstrap's own source (the extra oracle of 10.2): 0 errors, 56,191
statements, `flow` 66 ms.

#### Pending counts on the library (`-analysis-stats`)

107,733 expressions, 15 pending (function names as values 7, lambdas 8 -
unchanged). Unfinished statements: none (6,445 before). 31,058 statements:
block 6,874, local variable 3,458, expression 7,338, return 6,915, if 3,610,
while 340, do-while 2, for 845, foreach 29, break 39, continue 39, throw 722,
try 21, lock 655, when 4, asm 139, empty 28.

#### Warning counts on the library

`W1002` 3 (dead `return`s after an `if` / `try` whose every branch returns:
`reflect/field.sharp`, `util/uuid.sharp`, `util/version.sharp`; the twelve
`return`s after endless loops that the C++ compiler demands are exempt, 7.13);
`W1018`, `W1019`, `W1020` 0. Unchanged from stage 7: `W1022` 22, `W1005` 4,
`W1001` 19. In the suite `W1002` is given in six cases: code after a jump that
the case puts there on purpose (`finally.sharp`, `finally_fixes.sharp`,
`loops.sharp`, `lib_coroutines_core.sharp`) and dead `return`s after a `when`
whose every clause returns (`enums.sharp`, `switch_when.sharp`); `W1020` once,
in `switch_when.sharp`, which tests exactly that.

#### C++ compiler bugs found

- `break;` or `continue;` outside every loop crashes the C++ compiler
  (segmentation fault) instead of reporting it (D61).
- `lock(items) { }` with `items` an array in a variable is refused as
  "non-lockable" (D62).
- A label defined twice in an asm block, or jumped to and never defined, is
  accepted silently (D63).
- `"items " + ((thing as holder).items.size().value as _int64)` - a cast inside
  parentheses, read further and cast again, as the right side of `+` - crashes
  the C++ compiler (segmentation fault in `compile_paren_expression`).
  Reproduction (whole file):
  ```
  mod app;
  class holder { items := new list<string>(); }
  def describe(thing: object) : string {
      return "items " + ((thing as holder).items.size().value as _int64);
  }
  def main() { print(describe(new holder())); }
  ```
  Met in `reports/statement_printer.sharp`; worked around with a local.

### 10.5 Lambdas and initializers

**status: done** (the plan as it was before the stage follows the "as built"
part, unchanged, for the record)

#### What the stage does

Two packages and a self-check:

- `backend/compiler/lambdas` - lambdas and function values, inside the body
  walk. `analyse_expression` sends an `N_LAMBDA` to `analyse_lambda` with the
  type expected at its place; `as_value_for` sends a function NAME used as a
  value to `function_value_of` with the type expected. A lambda is read
  (`lambda_form`), its type completed from the expected function type
  (`lambda_signature_of`: E0800 - E0803), made a `sharp_function` of its own
  and its body analysed in a context whose first scope continues into the
  scope it is written in (`build_lambda`). Captures are recorded by name
  lookup itself: `find_local` counts the function boundaries it crosses, and
  `expression_for_found` hands a use with `depth > 0` to `note_local_capture`
  (`context/closures.sharp`); `self`, an implied `self` and `base` in a lambda
  go through `note_self_capture`. In a CALL a lambda whose types are not all
  written waits: `argument_at` records its written form on the
  `call_argument`, overload resolution compares that form with each candidate
  (`fits_lambda`), and `argument_for` analyses it against the chosen
  parameter. Function names passed as arguments wait the same way.
- `backend/compiler/initializers` - how things are set up. While bodies are
  analysed: the implied `-> base()` of every constructor and `init(x)` that
  writes none and of every constructor the compiler added (`implied_base_call`,
  `complete_added_constructor`), W1024 on written base calls, the numbers of
  enum values (`analyse_enum_values`, called by the driver per enum). After the
  bodies, a pass of its own (`order_initializers`, `initializers` on the
  `time:` line): the default objects of fields, and the initialisation lists of
  every analysed class and of every module, field values in reading order.
- `backend/compiler/components` and `backend/compiler/obfuscation` - built by
  the review of this stage; see "The review of this stage" below.
- `backend/reports/unknown_types.sharp` (since 10.8: `backend/compiler/program/completeness.sharp`, run on every build and reporting E0991) - the self-check: after the bodies
  every typed tree is walked and the expressions with the error type in files
  with no reported error are counted. Printed by `-timings` and
  `-analysis-stats`; the gates require `unknown: 0` for the library
  (`~/sharp_work/tools/gates.sh`, after the `time:` line).

Also: a `const` field whose value is a constant carries it where it is read
(`constant_of_field` in `expressions/primary/field_access.sharp`), so enum
values and folding can use it; `-analysis` prints lambdas with their
parameters, captured variables, closure records and bodies, function values,
and the initialisation lists of every class and the globals' order;
`-analysis-stats` counts lambdas and function values.

#### Objects and fields added

| object | field | meaning |
|---|---|---|
| `bound_function_reference` (`BX_FUNCTION_REFERENCE`) | `function` | a function named as a value |
| `bound_lambda` (`BX_LAMBDA`) | `function`, `record` | the lambda as a function; the record its value carries |
| `closure_record` (`types/declarations/closure_record.sharp`) | `function`, `field`, `variables`, `captures_self`, `parent` | 4.3 |
| `bound_base` | `depth` | as `bound_self.depth` |
| `sharp_function` | `is_lambda`, `enclosing`, `enclosing_field`, `takes_record`, `closure`, `lambdas` | 4.3, 6.10 |
| `sharp_field` | `default_constructor`, `closure`, `lambdas` | 6.10 |
| `sharp_enum_value` | `bound_value`, `constant` | 7.16 |
| `sharp_class` | `instance_initializers`, `static_initializers`, `fiber_initializers` | 7.15.2 |
| `sharp_module` | `global_initializers`, `fiber_initializers` | the globals, in reading order; the `localized` ones |
| `sharp_program` | `lambdas` | every lambda |
| `call_argument` | `lambda_parameter_types`, `lambda_result`, `named` | the written form of a waiting lambda; the functions a waiting name may be |
| `analysis_context` | `inferring_result`, `returns` | a lambda whose result comes from its `return`s |
| `conversion` | kind `CONV_RESULT_IGNORED` | D69 |
| typed tree | `tree_parts.sharp` (`parts_of`, `add_parts`), `statements/statement_parts.sharp` | the parts of any expression / statement, for passes that visit a whole tree |

`overload_result.waits_for_lambda` and `pending_call` are gone: no call
waits for a later stage any more. (The objects of the component system and
of `obfuscate` blocks, added by the review: 6.9.)

#### Rules found by probing the C++ compiler in this stage

| probe | C++ compiler | here |
|---|---|---|
| `f := d.bark;`, `f := bark;` in its class | "cannot get address of non-static function" | `E0804` |
| `h := generic_one;` | "could not resolve symbol" | `E0805` |
| `g : (decimal)(decimal) = twice;` with two `twice` | a warning "ambiguous function reference" although the type decides | the overload of that type, no warning; `W1011` only with no type expected |
| `{ x -> return x; }` with no expected type; `{ -> n += 1; }` with no result | "not a fully qualified lambda expression" | `E0800`; the second is accepted (D72) |
| `b : (decimal)(decimal) = { x, y -> ... }`, a parameter or result of another type | "cannot assign field ... as types do not match" | `E0801`, `E0802`, `E0803` |
| `apply(gives_text, 1)`, `new thread("w", worker)` (results differ) | accepted (parameters only) | `W1029` (D69) |
| `k : (decimal)(decimal) = gives_text;` (result differs) | refused | `E0410` |
| `run { break; }` / `run { continue; }` in a loop | accepted, then a code-generator crash / accepted | `E0602` / `E0603` |
| a lambda as an operand of `+` | "unqualified use of operator `+`" | `E0808` |
| `enum c { a = "text" }`, `= shade.light` | "expected numeric type for enum field" | `E0940` |
| `enum s { a = 1, b = 1 }`; `= limit * 2` with a global | accepted | `W1030`; accepted (evaluated at start) |
| globals `a := b + 1; b := 5;` | compiles; `nullptr_exception` at start | reading order (D70) |

#### Error and warning codes reported by this stage

`E0800`, `E0801`, `E0802`, `E0803`, `E0804`, `E0805`, `E0808`, `E0809`,
`E0940`; `W1011`, `W1024`, `W1029` (new), `W1030` (new), `W1031` (new).
`E0806`, `E0807` stay free. By the review: `E0900` - `E0906`, `E0908`,
`E0909` (new), `E0924` (new), `E0950`, `E0951`; `W1013`, `W1014`, `W1016`.
`E0907` is not used (D77).

#### Not done / known gaps

- Captures through `asm`, and reads of fields inside called functions, do
  not count for the reading order of field values (D70 counts direct reads).
- The written-result filter: when several overloads differ only in the
  RESULT of the function type they want from a lambda, the first wins even
  if the lambda's written result fits a later one better (the result of an
  argument is not compared, D69).
- A lambda in an instance field's value that uses `self` gets a record of
  the field (`sharp_field.closure`); whether a code generator wants that
  record made with the object's fields or per lambda is left to it.
- The self-check counts silently-unknown expressions only for files with no
  reported error; a file with an error may hide a gap.
- Components: a request is answered by the EXACT type only (not by a
  definition of a class below it); a `get()` argument that several overloads
  could take goes to the first that fits; component names are global, not
  per module, and a `get` / `inject` sees every component whatever its file
  imports (as in the C++ compiler); a lambda written in a definition is in
  `sharp_program.lambdas` but in no function's or field's `lambdas` list (its
  place is `type_definition.value`).
- A definition that was reported as wrong and has no type keeps the requests
  it might have answered quiet (one mistake, one message): they are reported
  in the next build, once the definition is right.
- `obfuscate`: a function of a generic class AS WRITTEN (`list.add(T)`) is
  matched by its number of parameters only; `obfuscate` blocks written inside
  a class are looked up like those at the top of a file (the class around
  them does not shorten the names).
- An `asm` block in a function whose local a lambda captures may name that
  local (`[name]`); the variable then lives in the closure record
  (`is_captured`), which the code generator must take into account.

#### The review of this stage (an independent pass over the finished work)

The review ran the gates, probed lambdas, closures, function values, field
values, enum numbers, getters / setters and `mutate` bodies with small wrong
and right programs against both compilers (the records of nested lambdas,
`self`, loop variables, base-call arguments, generic classes and field
values were read off `-analysis`), and found the lambda and initializer work
sound. What it changed:

| what | before | now |
|---|---|---|
| components, `get`, `inject` | left for stage 10: `get<t>()` was a silent pending node, an `inject` was never resolved, nothing in 7.20 was checked | `backend/compiler/components`: 7.20 in full (`E0900` - `E0906`, `E0908`, `E0909`, `W1013`, `W1014`), `bound_get_component`, `injected_from`, injected fields in the initialisation lists |
| `obfuscate` blocks | not looked at | `backend/compiler/obfuscation`: 7.25 (`E0950`, `E0951`, `W1016`), `sharp_program.obfuscation_rules`. The library's own blocks (`platform/dependencies.sharp`) resolve without a message |
| `bound_pending`, `expressions/pending.sharp`, the pending counters of `-analysis-stats` | "an expression a later stage builds" | no expression is pending: the file and the counters are removed; the node remains as the placeholder of a waiting argument. `-analysis-stats` prints `components:` instead |
| the `unknown:` line | "... (0 get<t>() for a later stage)" | no such count: a `get` has a type or a message. Definitions are walked too |
| field values in a circle, `W1031` | the values of a circle ran in the order the depth-first search left them - the LAST member first - although this document said "the written order" (and the C++ compiler runs them as written: a program with a circle would have computed other numbers); the message said "part of a circle"; a value reading its OWN field was not noticed; a lambda's body counted as a read | a circle is placed as one unit, its members in the written order (`place_with_circles`); the message lists the circle (`x` reads `y`, `y` reads `x`), stands at the member that runs first and points at the others; reading itself is reported; a lambda in a value reads nothing yet |
| `E0605` in a lambda | "not every path through `lambda@26:10` returns a value" | "... through this lambda ..." |
| `E0505` for a lambda called where it is written | the message quoted the lambda's source text, cut off | "this lambda takes ..." |
| `E0809` outside a lambda | no help | a help |
| `asm` in a lambda naming a local of the function around it | accepted, with no capture recorded (a listed gap) | `E0924` (D78) |
| `types/declarations/sharp_function.sharp` | also held the component classes | they are `sharp_component.sharp`, with `component_request` |
| tests | - | `pass/components.sharp`, `fail/components.sharp`, `fail/obfuscate.sharp`; `fail/lambdas.sharp` + `E0924`, `fail/initializers.sharp` + the self-read `W1031`; `pass/members.sharp` (stage 5) gained an unnamed `point` definition - its `inject(app) origin : point;` had only a NAMED one, which an `inject` cannot find |

How the component analysis is put together (the files are listed in the
front door, `components/components.sharp`):

- a definition is analysed ON DEMAND, once (`provides_of`), by whoever needs
  its type first, and at the latest by `analyse_definitions`, which
  `analyse_bodies` calls before its work list of generic instances (a
  definition can name `new list<int>()`);
- a request (`component_request`) is searched (`search_definitions`: only
  the search) and then settled (`settle_request`: the answer, or one
  message). A request that finds nothing while a definition in reach is still
  being analysed waits on `session.unsettled_requests`; when every definition
  is known it is either answered by a definition of its own chain - a circle,
  `E0908` - or it is an ordinary `E0903`;
- `check_components` (the `components` entry of the `time:` line) does what
  needs the whole program: `E0900`, `E0901`, the waiting requests, and the
  injected fields of every class and module. It runs before
  `order_initializers`, so an injected field is in the lists.

#### The plan before the stage (kept for the record)

Scope: 4.3, 7.14, 7.15, 7.16, 7.17 (bodies); inventory A80-A89, A140-A144,
B87, B110, B111, B121, B122, B145, B180-B190.

To build: `lambda_expression`, `function_reference`, `dependency/closures.sharp`;
`enum_compiler`, `constructor_compiler`, `init_compiler`, `accessor_compiler`;
`instance_initializers` / `static_initializers` of every class; the calls
stage 7 left pending because a lambda had to decide. A written `-> base(args)`
is already bound by stage 7 (`sharp_function.bound_base_call`, `E0509`); left
for this stage are the IMPLIED base call of a constructor that writes none,
the constructors the compiler adds, and `W1024`. `field_compiler` here is
only what is left of field values after stage 6: a field whose value is a
lambda.

A lambda's body is analysed with the statements of stage 8 as they are
(`analyse_statement`, `backend/compiler/statements`), in a context of its own:
an empty jump stack (so `break` inside a lambda inside a loop is `E0602`, 7.12.2)
and its own `expected_result` - the written result type, or, while the result
is inferred from the lambda's `return`s, null (`analyse_return` then only
checks the names in the value; stage 9 adds the inference and the check of
every `return` against the result found). The flow pass (`check_flow`) walks
`session.analysed_functions`: a lambda's function must be added to it.

Done when: `-analysis-stats` prints 0 pending nodes other than `get` for the
library and the suite; every use of a variable from an enclosing function has
`depth > 0` and its variable `is_captured`; every class has its two
initialisation lists.

#### Fixes to earlier stages

- `expressions/values.sharp`: `as_value_for` (a function name used as a value
  becomes a function value, with the type expected); `analyse_value` passes
  the type expected on. `pending.sharp` covers only `get<t>()`.
- `expressions/calls/*`: arguments wait as described above (`argument_at`,
  `argument_for`); `pending_call` and `overload_result.waits_for_lambda` are
  removed; `overloads/ranking.sharp` no longer stops when a lambda "decides"
  (D71), `applicability.sharp` compares a waiting lambda's written parameter
  types and a function name's overloads, and lets a function value whose
  parameters fit through with `CONV_RESULT_IGNORED` (D69).
- `statements/jumps/return_statement.sharp`: result inference for lambdas;
  messages name "this lambda"; its own E0606 help for a lambda.
- `statements/exceptions/lock_statement.sharp`: a function value can be
  locked (D74; the stage 8 plan had E0615).
- `statements/expression_statement.sharp`: `has_effect` for the pending
  `get`; W1018 names a function value.
- `flow/reachability.sharp`: the result expression of a lambda (`is_result`)
  is a `return`.
- `names/not_found.sharp`: `it` outside a lambda with an `it` is E0809.
- `primary/found_names.sharp`, `primary/self_expression.sharp`: captures
  (`note_local_capture`, `note_self_capture`); an implied `self` is made
  only when one of the functions found needs it.
- `primary/field_access.sharp`: a `const` field carries its constant value.
- `conversions/implicit_conversion.sharp`: two equal function types are an
  identity conversion (was "to nullable").
- `conversions/convert.sharp`, `conversion_messages.sharp`: W1029.
- `driver/function_body.sharp`, `class_bodies.sharp`: implied base calls,
  added constructors, enum numbers.
- messages that said "`lambda@3:9` is static" name the function around the
  lambda (`static_owner_name`, `context/closures.sharp`).
- `tests/backend/pass/flow_wider.sharp`: the five lines the stage 8 note
  listed are reported now (four moved to `fail/statements.sharp`; `lock` of a
  function value is accepted, D74).

#### Tests

`tests/backend/pass/lambdas.sharp` (the C++ compiler compiles it),
`pass/lambdas_wider.sharp` (D10, D11, D27, D72), `pass/initializers.sharp`
(D70, enum numbers, default objects, added constructors);
`fail/lambdas.sharp` (24 codes), `fail/initializers.sharp` (5 codes);
`fail/statements.sharp` gained four lines. Every marked line of
`fail/lambdas.sharp` was compiled alone with the C++ compiler: it rejects
each one except the lines its comments mark (`break` / `continue` in a
lambda, W1029, the bootstrap-only inferred results).

#### C++ compiler bugs found

1. A statement inside a lambda that is written in a `return` is reported
   "unreachable": `def make_adder(amount: decimal) : (decimal)(decimal) {
   kept := new int(amount); return { x: decimal -> (decimal) return x +
   kept.value; }; }` warns S607 at the inner `return`.
2. Function types of ARGUMENTS are compared by their parameters only:
   `apply(gives_text, 1)` with `gives_text` returning a `string` where
   `(decimal)(decimal)` is expected compiles, and `apply` then reads a
   string as a number (D69 keeps it, with W1029, because the library and the
   suite rely on the same rule for results that are ignored).
3. Global field values run in the written order, so `a := b + 1; b := 5;`
   compiles and stops with `nullptr_exception` when the program starts (D70).
4. `g : (decimal)(decimal) = twice;` warns "ambiguous function reference"
   although the declared type picks one overload.
5. (review) A `get` / `inject` that names no component finds a definition
   only in the component declared LAST: with `component app { single { new
   database("main") } }` followed by `component { single { new logger() } }`,
   `d := get<database>();` is "No defined type found of `app#database`" while
   `get<logger>()` compiles. `get_type_definition(manager, type, ast)` in
   `dependency/component_manager.cpp` assigns its result in the condition of
   every round of its loop and returns the last one (D75).
6. (review) A FIELD listed in an `obfuscate` block crashes the compiler
   (segmentation fault): `class point { x : decimal; }` with `obfuscate -keep
   { point.x }`, or a global `total : decimal = 0;` with `obfuscate -keep {
   total }`. A class, a function with its parameters and a module in quotes
   are fine (D80).

#### Gates and timing

```
library: 0 error(s), reached the end: yes
time: parsing 114 ms, declarations 139 ms, type names 131 ms, class checks 58 ms, members 404 ms, bodies 1900 ms, flow 24 ms, components 8 ms, initializers 2 ms
unknown: 0 expression(s) without a type and without a message
suite cases: 175 run, 0 failing
frontend tests: 13 passed, 0 failed
backend tests: 29 passed, 0 failed
GATES: PASS
```

(The block above is the run at the end of the review. The implementer's own
measurements follow.) `initializers` is 3 - 8 ms. `bodies` (stages 6 - 9) measured 4.0 - 4.4 s
on the library in this session; at the start of the session, before any of
this stage's code, the same machine gave 2.9 - 3.3 s, and every OTHER stage
was slower by the same quarter at the end (parsing 143 -> 169 ms, members
677 -> 867 ms), so most of the difference is the machine, not the stage:
the library has 8 lambdas and 7 function names used as values. It remains
over the one-second guide, as stage 7 reported. The bootstrap also compiles
its own source with itself: 0 errors, `unknown: 0`.

The gates script (`~/sharp_work/tools/gates.sh`, outside the repository)
gained two things in this stage: the `unknown:` line of the library must
say 0; and a suite case whose output is ONLY the VM's SIGSEGV while the
files are parsed (lib/runtime, not the bootstrap; it happened to 2 - 18 of
the 175 cases per run in this session, with six compilers at once in a WSL
of 2.9 GB) is compiled once more, alone, before it counts as failing. Every
case that crashed so passed when compiled alone. (Since the runtime fix of
2026-11-14, see the end of 10.1, a crash counts as failing again; the case is
still compiled once more alone so the report says whether it comes back.)

Over the suite (each case with the library, run again by the review):
`unknown: 0` in every case; 1786 lambdas and 1422 function values analysed
in all; `W1029` once per case (the library's `fiber_main`) and 17 times in
`threads.sharp`.

The review's passes: `components` (`check_components` and
`check_obfuscate_blocks`) is 8 - 31 ms on the library (31 when the bootstrap's own source is compiled along), nearly all of it the
library's two `obfuscate` blocks (about 50 names); the library has no
component. `bodies` was 1.7 - 3.9 s over the review's runs on the same code,
with the load of the machine (it is still over the one-second guide, as stage 7 reported); the review added nothing to it for a program
without components (one comparison per expression).

#### Warnings on the library

No expression is pending (the counter is gone). Warnings: `W1001` 19, `W1022`
22, `W1005` 4, `W1002` 3, `W1029` 1 (`vm.start_fiber(..., fiber_main)`).

### 10.6 Whole-program checks

**status: done.** `check_program(session)` in
`backend/compiler/program/program.sharp`, a pass of its own after the
initializers, with its own entry on the `time:` line (`program checks`). It
returns the `analysed_program` (section 12), the object code generation
starts from.

#### What the stage does

In this order:

| step | file | does | codes |
|---|---|---|---|
| `collect_what_was_analysed` | `program.sharp` | every function with a typed body (lambdas included), every real class whose members were analysed, every module - into the `analysed_program` | - |
| `find_main_function` | `main_function.sharp` | the global `main` of one of the four forms (7.23); its form (`MAIN_ARGS_INT` ...); `sharp_program.main_function` | `E0930`, `W1015` |
| `check_platform_functions` | `platform_functions.sharp` | `static_init` / `tls_init` of `platform.kernel#platform` are empty; records `srt_init` (where the VM starts) and the platform class | `E0961` |
| `check_required_declarations` | `required_declarations.sharp` | the modules and classes the VM itself needs (7.26): missing ones are one `E0962` at `main`; those present become `analysed_program.required_classes` | `E0962` |
| `record_usage` | `usage/usage.sharp`, `usage/references.sharp` | walks every typed tree ONCE and keeps on its owner what it names (`sharp_declaration.uses`: functions called, constructors and classes made, fields read or written, enum values, lambdas, functions named as values, classes tested or cast to, the value of a component definition a `get` / `inject` receives); marks `used_anywhere` (also on the member of the blueprint an instance member was copied from) | - |
| `reach_from_roots` | `usage/reachability.sharp` | the graph search from `srt_init` and `main`, then from the classes the runtime needs, over the uses ("Reachability, as built", below): `reached` on every declaration the program can come to, `analysed_program.reached_functions` / `reached_classes` / `reached_fields` in the order found | - |
| `check_required_values` | `required_values.sharp`, `first_uses.sharp` | a non-nullable object field nothing gives a value (7.15.4); every constructor and init block is walked once for all the fields of its class | `W1032` (-wextra) |
| `report_unused` | `usage/unused_declarations.sharp` | unused locals, unused private members, a local named like a field of its class | `W1034`, `W1035` (-wextra), `W1036` |
| `count_problems` | `program.sharp` | errors and warnings of the whole build, on the analysed program; `main.sharp` counts again after the warning switches (-werror turns warnings into errors), and that count is the one code generation asks | - |

Two checks are made where the code is bound, because that is where the place
to point at is known: `E0510` (`base->f()` where `f` has no body, and a call
of a `static` function of a class that has none,
`expressions/calls/bind_call.sharp`) and `W1033` (`new` of a class that still
has a function without a body, `expressions/calls/new_expression.sharp`,
answered once per class: `sharp_class.unkept_promise`).

`main.sharp` then applies the warning switches to the finished list of
diagnostics and prints it within the error limit
(`warning_options.sharp`: `apply_warning_settings`, `print_within_limit`).
-w, -w<kind> (the C++ compiler's nine kinds, `-wdep` added to `main.sharp`
and `settings.sharp`, where it was missing), -werror, -errlmt and the new
-wextra all work now; they were parsed and stored before, and nothing read
them.

`-analysis` prints, besides the typed trees: the parameter types in the
heading of every function (`constructor app#holder.holder(_int64) : nil`:
two overloads are two headings; a static init block is called one), every
local of a function with its slot (`local #2 i : _int64`, with "parameter", "made by the compiler",
"const", "localized", "captured by a lambda"), `[reached from main]` after
the heading of a function the program reaches, and the analysed program in a
few lines (`reports/program_printer.sharp`; also printed by -analysis-stats):

```
program: entry point none (a library: no executable can be built)
  start-up: platform.kernel#platform.srt_init
  to generate: 11090 functions (8 lambdas), 274 classes, 18 modules
  reached from main: 412 functions, 139 classes, 418 fields
  needed by the runtime: 29 classes, 13 of them reached for that reason alone
  reported: 0 error(s), 49 warning(s)
```

`-declarations` is unchanged and still correct (run on the library and on
the tests at the end of the stage).

#### Objects and fields added

| object | field | meaning |
|---|---|---|
| `analysed_program` (new, `types/program/analysed_program.sharp`) | `prog`, `main_function`, `main_form`, `startup_function`, `platform_class`, `required_classes`, `functions`, `classes`, `modules`, `lambdas`, `reached_functions`, `reached_classes`, `reached_fields`, `reached_for_runtime`, `errors`, `warnings` | section 12 |
| `first_uses` (new, `compiler/program/first_uses.sharp`) | `fields`, `states` | the fields of a class that need a value and, beside each, what a piece of code does with it first (`FIRST_NOTHING` / `FIRST_WRITE` / `FIRST_READ`) |
| `sharp_declaration` | `reached`, `used_anywhere`, `uses`, `usage_stamp` | 6.10 |
| `sharp_module` | `reached` | its global values run at start |
| `sharp_class` | `promise_checked`, `unkept_promise` | W1033 |
| `sharp_program` | `main_function` | the entry point |
| `analysis_session` | `extra_warnings`; counters `unused_locals`, `unused_private_members`, `hidden_fields`, `fields_without_value` | -wextra; what -analysis-stats prints on the line "off by default (-wextra)" |
| `warning_settings` (new, `compiler/program/warning_options.sharp`) | `enabled`, `kinds`, `as_errors`, `extra`, `error_limit` | the switches as the backend applies them |
| `name_table<v>` (new, `types/declarations/name_table.sharp`) | | the hash table of names (Speed, below) |

#### Rules (section 7 is corrected)

- 7.23 `main`, as found in `check_main_method_signature` and checked: one
  entry point (`E0930`, with the first shown), the four forms; any other
  global `main` is `W1015` (D81).
- 7.15.4 a field must have a value: the C++ rule, as a warning (D82).
- 7.26 `static_init` / `tls_init` stay empty (`E0961`).
- 7.26 the modules and classes the VM needs are part of a program that has
  a `main` (`E0962`, D89).
- 7.26 a `static` function of a class without a body cannot be called
  (`E0510`, D88): the C++ compiler builds it and the VM stops with
  "attempting to call non-native static delegate function" (checked).
- what is reached from `main`: "Reachability, as built" below.
- `base->f()` on a function without a body is `E0510` (D83); `new` of a class
  that still has one is `W1033` (D84). Both checked with the C++ compiler:
  the first compiles and overflows the stack, the second compiles and runs.

**Reachability (`dependency_runner.cpp`), as built.** A function reaches
what its body and base call use, its class, every function that overrides
it, and, for an accessor, its field. A field reaches its value's uses, its
class, its getter and setter. A class reaches its base class, its
interfaces, its initialisation lists and, for an enum, its values. Anything
reaches the global values of its module. Roots: `srt_init`, `main`, and the
classes the runtime needs (`required_classes`), each with every field it and
the classes above it have - native code writes them.

The C++ compiler's `mark` is coarser: reaching anything also marks its FILE,
and a file marks everything any code in it depends on; that is how the
classes the VM needs come to be "used" there without anything naming them.
The bootstrap follows only real uses, so 13 of those 29 classes are reached
by neither `srt_init` nor a program's `main` and are roots in their own
right (`reached_for_runtime`, printed in the summary).

#### What else the scope asked for, and where it stands

- ACCESS - private / protected / local on every use of a field, function,
  constructor, class, alias: built by stages 3, 6 and 7 (7.9:
  `names/access_rules.sharp`, `postprocessor/names/type_access.sharp`,
  `calls/bind_call.sharp` `check_function_access`). The messages say where
  the member lives and how to reach it. Nothing was left for this stage.
- `const` (W1021, stage 7), static against instance (E0703, E0704, stages 6
  and 7), `native` / `stable` / `ext` / `excuse` / `override` / `localized`
  (stage 5 and 7.26): built earlier. `excuse` now also has its meaning in
  W1032.
- Instantiating what cannot be instantiated: `E0507` (interface, enum, `ext`,
  a number - stage 7); a class with functions without a body: `W1033` (new).
  Calling a function without a body non-virtually: `E0510` (new) - through
  `base->`, and a `static` one through its class.
- Interface completeness for generic instances: stage 5 (`E0341` per instance,
  `link_families`).
- The C++ compiler's warnings: every `create_new_warning` site has a `W` code
  (8.B); the switches work (above).

#### Error and warning codes reported by this stage

`E0510`, `E0930`, `E0961`, `E0962`, `W1015`, `W1032`, `W1033`, `W1034`,
`W1035`, `W1036`. `E0960` is retired (D82). Next free: `E0963`, `W1037`.

#### Where it differs from the C++ compiler

D81 - D89 (section 5).

#### Speed

The `time:` line of the library at the start of this stage and at its end
(the same machine, one run each; runs vary by about a fifth with the load of
the machine):

```
before: parsing 110 ms, declarations 156 ms, type names 138 ms, class checks 58 ms, members 432 ms, bodies 2078 ms, flow 27 ms, components 8 ms, initializers 2 ms
after:  parsing 94 ms, declarations 51 ms, type names 76 ms, class checks 36 ms, members 247 ms, bodies 1165 ms, flow 34 ms, components 5 ms, initializers 5 ms, program checks 188 ms
```

About 3.0 s became 1.9 s in all, the new stage included. Found by timing
the parts of the body pass (temporary timers, removed again), three changes,
none of which changes what is reported:

1. **Names are looked up in a `name_table`, not in the library's
   `hash_map<string, ...>`** (`types/declarations/name_table.sharp`). The
   library works out `string.hash()` with decimal arithmetic and an `int`
   object per character and compares keys with `string.operator==`: 6.5
   microseconds per question on the VM (measured with 20,000 lookups of
   200 names); the table, which keeps a plain-number hash (djb2) of each name
   and compares text only when the numbers agree, takes about a third of
   that. Used for the names of every module and class (`scope`), the
   remembered answers of name lookup (`remembered_members`,
   `remembered_globals`), `sharp_program.modules` and `files_by_path`. A
   block of local variables (`local_scope`) is now a short list with the hash
   of each name beside it, and a lookup works the hash out once and compares
   numbers on the way out through the blocks. This is what took
   declarations, type names and members down by half.
2. **The reason a candidate of a call does not fit is put into words only
   when a message needs it** (`overloads/call_candidate.sharp`,
   `reason_text`). Overload resolution used to write "argument 1 is `int`
   but `string` is expected" for every losing candidate of every call -
   `string.operator+` has twenty overloads - and string building on the VM
   is slow. Overload resolution went from about 420 ms to 230 ms.
3. **`resolve_written_type` keeps its last answer for a type that depends
   on a type parameter** (`sharp_type.last_environment`, `last_resolved`):
   the members of one generic instance are analysed in a row, and working
   the type out again built the text of the instance key and looked it up
   each time. Type resolution in `new` expressions went from about 190 ms to
   40 ms.

What remains in `bodies` (about 1.1 s): plain names (43,000, about 7
microseconds each, mostly the class-member and module lookups that now go
through `name_table`), overload resolution (about 230 ms) and the
constructors chosen for `new` (5,600). Interning every identifier to a
number once per syntax node, so that tables are keyed by numbers, is the
next step; it touches every symbol table and was not cheap enough to be
safe in this stage. The new pass itself is 160 - 190 ms on the library
(260 ms on the bootstrap's own source, after the field names of a class
were put in one table per class for W1036 - a member search per local had
made it 800 ms there): nearly all of it the walk over every typed tree that
records the uses (the self-check of stage 9 walks the same trees a second
time; the two walks could be one).

**Reading the `time:` line.** One stage of a run is often 200 - 800 ms
slower than in the run before, and not the same stage each time (the review
saw `flow` at 30 ms and at 367 ms, `program checks` at 190 ms and at 460 ms,
`bodies` at 900 ms and at 2600 ms, on the same build). Timing the usage walk
per thousand functions showed why: the walk is steady at about 10 ms per
thousand, with ONE gap of 310 ms - a pause of the VM's garbage collector,
which lands in whichever stage is running when the heap fills. Compare the
sum, or the smallest of several runs, not one line.

The review made one more change of this kind: `W1032` walked every
constructor once PER FIELD that needs a value (a class with twenty such
fields walked its constructor twenty times, making two lists per statement);
`first_uses.sharp` walks it once for all of them. The check went from the
largest part of the pass on the bootstrap's own source to a few
milliseconds; what is reported is unchanged (the same 37 fields in the
library, the same messages in the tests).

#### Warnings on the library

On by default: `W1001` 19 (plus one in the bootstrap's own source), `W1022`
22, `W1005` 4, `W1002` 3, `W1029` 1 - unchanged. This stage adds none to the
library: `W1015`, `W1033`, `W1036` and `E0510` do not fire there. Off by
default, counted with -analysis-stats ("off by default (-wextra)"): 3 unused
locals (`W1034`), 2 unused private members (`W1035`), 37 fields never given
a value (`W1032`, 22 places: a field of a generic class counts once per
instance and is reported once). Each fires in the library, so each is off
unless -wextra is given. (Before the review the first two counts were 66 and
107: 63 were variables of a `catch`, and 104 were private members of generic
classes of which the library alone makes no instance - neither is a finding;
see the review below.)

#### Tests

- `tests/backend/pass/program.sharp` (`// options: -wextra`): the four forms
  of `main` (one used), a `main` inside a class, every way a field gets a
  value (declared, every constructor, an `init` block, a `static init`
  block, nullable, `excuse`, a number class, `string`), `base->` to a
  function with a body, `_` locals, a `foreach` variable that only counts, a
  `catch` variable that is not used, a local only an `asm` instruction
  names, a generic class nothing makes an instance of (its private members
  are not "unused"), a local named like a private field of the class above
  and like an instance field inside a static function (neither is hidden).
  The C++ compiler accepts it.
- `tests/backend/fail/program.sharp`: `W1033`, `E0510` (a static function
  without a body called), `E0510` (`base->`), `W1015`, `E0930`, `W1036`.
- `tests/backend/fail/unused.sharp` (`// options: -wextra`): three `W1032`,
  `W1034`, two `W1035`.
- `tests/backend.sh`: a test may name options on its second line
  (`// options: -wextra`).

#### Not done / known gaps

- `E0961` has no test file: the library's own `platform.kernel#platform` is
  compiled with every test, and a second class of that name cannot be
  declared beside it. Checked by hand on a copy of the library whose
  `tls_init` holds a `println` (the job's scratch script `e0961.sh`): one
  `E0961`, at `tls_init`.
- `E0962` has no test file either, for the same reason (every test is
  compiled with the library, which has all of them). Checked by hand:
  `mod app; def main() { }` compiled alone gives one `E0962` at `main`
  naming the five missing modules.
- The C++ compiler also refuses a required module or class that an
  `obfuscate` block renamed ("may be obfuscated"). Not checked: what
  obfuscation does to a name is decided when the executable is written.
- A static function without a body that is only NAMED as a value
  (`f := tool.promise;`) and called through the value is not `E0510`.
- W1032 follows the C++ rule "the first use in written order", not real
  definite assignment: `if(x) { f = a; } read(f);` counts as assigned.
  A warning for "assigned on some paths only" would need the flow analysis
  per field; not built.
- W1032 counts a field assigned only through another object
  (`other.f = x;`) or through a function the constructor calls as NOT
  assigned (the C++ rule counts the first; neither counts the second).
- Shadowing other than a local hiding a field (W1036) is not warned about: a
  local hiding a local is E0600 already (stage 8); a parameter named like a
  field is the common `self->name = name;` form and is not warned about.
- Reachability: a function called only through a function VALUE stored in a
  field is reached through the function reference where it is taken, which
  is right; a FUNCTION called by the VM by name (`fiber_main`, platform hooks
  other than `srt_init`) is not a root unless something else names it - the
  classes the VM needs are roots (with their fields), their functions are
  not. The code generator must add its own roots there.
- No "unused import" or "unused parameter" warning.
- The node-by-node "could code be generated from its own fields" table of
  4.2 is in section 12.3; two answers are "the generator decides" (enum
  values without a number, the object of a `single` definition).
- Speed: `bodies` is still about 1.1 s, a little over the one-second guide
  (see Speed).
- E0510 is reported where the call is bound, so in the BODY stages, not in
  this pass; W1033 likewise.
- 8.C promised "a test that walks a file containing every construct". There
  is no single such file: the pass files of all the stages together hold
  every construct (and the library, the suite and the bootstrap's own source
  are walked by the `unknown:` self-check), but no one file was written.

#### The review of this stage (an independent pass over the finished work)

The review ran the gates, then probed every rule in the stage's scope with
small wrong and right programs against both compilers - access (`private`,
`protected`, `local` on fields, functions, constructors, classes, enums,
aliases), `const`, static against instance, the modifiers, what cannot be
made with `new`, the forms of `main`, unreachable code, the warning
switches, and the new warnings with -wextra - and read `-analysis` for what
is reached. Access, `const`, static / instance and the modifiers were found
as 7.9, 7.10 and 7.24 describe them. (Two things to know when probing: the
C++ compiler does not compile a function nothing calls, so it is silent
about mistakes in dead code; and the bootstrap does not analyse bodies when
a declaration or type name is wrong, so one probe shows one layer.) What the
review changed:

| what | before | now |
|---|---|---|
| a `static` function of a class without a body, called (`tool.promise();`) | accepted; the C++ compiler builds it and the VM stops with "attempting to call non-native static delegate function" | `E0510` (D88) |
| the modules and classes the VM needs (`validate_required_dependencies` in the C++ finalizer) | not looked at; nothing made the VM's own classes roots | `required_declarations.sharp`: `E0962` for a program with a `main` that lacks one; `analysed_program.required_classes`, roots of the reachability walk with all their fields (13 of the 29 were reached by nothing else) |
| `def main(args: string[]) : int?` | `W1015` "will not be run" - but the C++ compiler runs it (checked) | an entry point |
| `def main() := e;` | silently an entry point, undocumented; the C++ compiler does not take it | still an entry point, written down as D87 |
| `W1035` unused private member | given for the private members of a generic class nothing makes an instance of - whose bodies are never analysed, so nothing is known (104 of the 107 in the library); for `srt_init`, which the VM calls; for a `private def main()` | not given for any of those |
| `W1034` unused local | given for the variable of a `catch` (63 of the 66 in the library) and for a local only an `asm` instruction names | not given for either |
| `W1036` a local hiding a field | also for a private field of a class above (which the code could not name) and for an instance field inside a `static` function | only when the field could have been meant |
| `W1032` | "is never given a value" also when the constructor assigns it but reads it first; every constructor walked once per field | "is read before it is given a value" for that case; one walk per constructor (`first_uses.sharp`) |
| `analysed_program.errors` | counted before the warning switches: with -werror it was 0 for a build that printed errors | counted again by `main.sharp` on the list the switches leave |
| `-analysis` | a function heading had no parameters (`holder()` for both overloads); `init` and `static init` blocks looked alike | parameter types in every heading; "static init block" |
| `reports/analysis_printer.sharp` (523 lines) | the walk over the declarations, every expression node and the words for them in one file | the expressions are `expression_printer.sharp` |
| the feature inventory (section 8) | 63 rows of stage 7 (calls and operators) built and tested but never ticked | ticked after checking each code against the `// expect:` lines of the tests and the features against the pass files; `B54` and `B80` stay open with their reason, `B211` and `B212` are new |

#### Gates and timing

The final full run of `gates.sh`, after the review:

```
library: 0 error(s), reached the end: yes
time: parsing 93 ms, declarations 46 ms, type names 73 ms, class checks 26 ms, members 224 ms, bodies 896 ms, flow 246 ms, components 6 ms, initializers 4 ms, program checks 145 ms
unknown: 0 expression(s) without a type and without a message
suite cases: 175 run, 0 failing
frontend tests: 13 passed, 0 failed
backend tests: 32 passed, 0 failed
GATES: PASS
```

Over the 175 suite cases this stage adds no warning: `W1015`, `W1033`,
`W1036`, `E0510` and `E0962` fire in none of them. (`flow` at 246 ms in the
run above is a pause of the garbage collector, not the flow pass: see
Speed.) The bootstrap compiles its own source with itself: 0 errors,
`unknown: 0` (program checks 260 ms there in a run without a pause).
-w, -wmain, -werror and -errlmt 1 were run by hand on a probe with two
errors and two warnings and do what 10.6 says.

#### C++ compiler bugs found

1. The initialization validator never fires (D82): `class holder { item :
   node; holder(x: _int64) { println(item.n); item = new node(); } }` with
   `def main() { h := new holder(1); }` compiles, and stops with
   `nullptr_exception` at `item.n`; a global `g : node;` read in `main` the
   same.
2. `base->speak()` where `speak` has no body in the base class compiles and
   recurses until `stack_overflow_exception` (D83).
3. A field of a number class without a value is null at run time, although
   7.3 / `field_compiler.cpp` give it a default object: `class node { n :
   int; }` and `k := new node(); if(k.n == null) { println("null"); }`
   prints `null`, and `k.n.value` throws `nullptr_exception` (checked). The
   bootstrap records the default object (`sharp_field.default_constructor`);
   the C++ compiler's `preInitScheme` is only injected for a field the
   finalizer marks `used`.
4. (found by the review) A call of a `static` function without a body
   compiles: `class thing { static def stat(); }` and `def main() {
   thing.stat(); }` builds, and the VM stops with `runtime_exception
   (attempting to call non-native static delegate function)` (D88).
5. (found by the review) `def main() := println("x");` is not taken as the
   entry point: "main method might not be executed", and no executable is
   written - `check_main_method_signature` runs before the result of an
   expression function is known (D87).

### 10.7 Review of the plan before building (design critic)

**status: done** - the document only; no compiler code was written and the
gates were not run.

The first draft was checked against the C++ sources, the library, the suite
and about thirty probe programs (in the job's scratch folder, not in the
repo). What changed, so that a reader of the first draft knows:

Rules that were wrong and are corrected:

| was | is | where |
|---|---|---|
| base to derived without `as` is an error | accepted with `W1017`; it also applies to ARGUMENTS and so is a rank in overload resolution | D6, 7.5.1, 7.6 |
| `const` and private functions are errors, with a "fallback" | warnings `W1021`, `W1022`, decided; private CONSTRUCTORS are unchecked in the C++ compiler too | D7, D8, 7.9, 7.10 |
| a literal takes any number type at rank 0 | a literal is an `_int32` / `_int64` / `decimal` / `_int8` like any value; the old rule made `lit(5)` ambiguous where the C++ compiler picks `lit(_int32)` | 7.2, 7.5.2 |
| all modules' global functions ranked together | nearer module hides the name, as in the C++ compiler | D4, 7.6 |
| ranking replaces the C++ steps | the C++ steps (exact, only one, plain-number rule) come first, ranking only where they give up | 7.6 |
| comparisons give `_int8` (D14) | `_int32` for number comparisons and `&&` `\|\|`, `_int8` for `!`, null tests, `is` - it decides which `string.operator+` runs | D14, 7.8.1 |
| integer arithmetic is the wider operand type | `_int64` only when an operand is 64 bits wide, else `_int32` as before; with the evidence of what the C++ rule breaks | D13 |
| enums count 0, 1, 2 | a value without `=` has no promised number (the C++ source says so on purpose); `= e` may be any number expression | D15, 7.16 |
| `x[i]`, operators, `foreach`, `lock`, `throw`, calling a function value need `!!` on a nullable | allowed; only `.member` / `.call()` and giving a `t?` to a `t` are errors | 7.11 |
| assigning a property without a setter is `E0714` | it stores directly (the suite has a case for it) | 7.10, 7.17 |
| `o[i] = v` without a fitting `put` is an error | `operator=` on what `operator[]` returns | 7.8.3 |
| a lambda parameter may not have the name of an outer local | it may | 7.4.2 |
| `{ 1, 2.5 }` widens to the later item | the first item decides; so does the first branch of `? :` | 7.7.5, 7.8.4 |
| `null` needs a `?` type | `object` takes null, and nullable values, without it | 7.5.1 |
| `x?.n` on a number member is `E0355` | its own error `E0725`, with what the C++ compiler does shown | D24 |
| `as` does not remove `?` | the result is the type as written | 7.5.5 |
| own name of a generic class inside it: to be confirmed | means this instance (wider than the C++ compiler, which makes it unusable) | 7.19 |
| `-> base(...)` arguments cannot use fields | they can; they read start values | 7.15.2 |

Design changes:

| was | is | why |
|---|---|---|
| `bound_local.through`: set when the use is bound | `depth` on the use, `is_captured` on the variable | a lambda further down can capture a variable after its earlier uses were bound; the old field would have been wrong for those |
| `dispatch` set by stage 10 over finished trees | set when the call is bound, from declarations; narrowing is the code generator's optimisation through `overridden_by` | nothing in a finished tree is rewritten; stage 10 needs no list of all calls |
| property reads, `o[i]`, `o[i] = v`, `operator=` as flags on field / index / assign nodes | `bound_call` with `written_as` | one way to call a function in the tree |
| `is_safe` flags on members and calls | `bound_safe_navigation` + `bound_receiver` | "the whole chain gives null" needs to know where the chain ends |
| `when` tests "subject == value" with the subject repeated | a hidden local holds the subject | evaluated once |
| types written in bodies: not designed | `written_type_at`, `sharp_file.body_types` beside the tree | generic copy-per-use shares them; the tree is not touched |
| instance members made eagerly | `ensure_members` on first use | cost |
| field values analysed in stage 9 | stage 6 | they are expressions; nothing about them waits |
| no order of initialisation in the objects | `instance_initializers`, `static_initializers` on the class | the code generator would have had to rebuild the rule of 7.15.2 |
| analysing only used members of instances as a fallback | never; with the measurement that allows it | decision 3 |

Added: section 5's rule for "stricter" and the note on how to probe the C++
compiler; D24-D33; 7.26; inventory 8.A2 (40 rows) and 8.C (every node kind);
codes `E0379`, `E0518`, `E0725`, `E0808`, `E0990`, `W1021`-`W1024`; 10.0.
Retired: `E0710`, `E0714`, `E0730` (numbers kept, not used).

C++ compiler behaviour found on the way that is a bug rather than a rule
(none of it was changed; `lib/grammar` is not touched):

1. An error whose TEXT was already printed is dropped, wherever it is. Two
   lines with the same mistake show one error. Reproduce: two functions each
   containing `i : item? = null; a := i.weight;`.
2. Integer arithmetic is labelled `_int32`: with `big : _int64 = 5000000000;
   one : _int64 = 1;`, `println("" + (big + one));` prints `705032705`.
3. `c.n += 2;` where `n` has a getter and no setter: "code-generator fatal
   error: attempting to set temporary data from register".
4. `for(...) { run({ -> break; }); }`: "code-generator fatal error: attempting
   to get data from uninitialized label".
5. `if(p?.x == 0)` with `p` null and `x` a number field: compiles, raises
   `nullptr_exception`.
6. `private` is not checked for functions and constructors; `const` is not
   checked at all; `new` of an interface is accepted.
7. `f : ()()? = null;` is "types do not match".
8. Under heavy load one `-eager` compile of a suite case
   (`logic_precedence.sharp`) reported errors inside library files that two
   reruns of the same command did not: something in the compiler depends on
   thread timing.

---

### 10.8 Completeness: gaps closed after comparing with the C++ compiler

**Status: done, with the gaps listed below.** Independent critics compared the
finished backend with the C++ compiler and listed 23 places where the
bootstrap said nothing, said too much, or said something unhelpful. Each was
reproduced, checked against the C++ compiler, and fixed in the stage of the
backend where its rule lives (so this "stage" is a set of changes spread over
the packages, plus one pass of its own). Several symptoms had one cause and
were fixed there; the table says which.

#### What changed, by cause

| gap | cause | fix (where) |
|---|---|---|
| a definition `factory { get<holder>() }` asks for itself; nothing reported, the build said "no errors" | the definition's type was the error type because its own request WAITED (it was in progress), and a definition with the error type counts as "reported as wrong", which keeps requests quiet | `type_definition.unknown_by_waiting` (types/declarations/sharp_component.sharp) marks such a definition; the search (components/definition_search.sharp) lists it apart instead of as broken; a request that meets one waits and is decided at the end: `E0903`, with the definition shown and a note that it provides nothing by itself (components/requests.sharp, request_messages.sharp). Also with a NAMED definition of the type elsewhere (the near-miss help then shows it) |
| silently unknown expressions in a user program ended "analysis complete: no errors" | the self-check only counted, and only with `-timings` | the self-check moved to `compiler/program/completeness.sharp` (`check_completeness`), runs on every build, and reports `E0991` at the first such expression of each file. Its three known causes were fixed: `f;` (a function name as a statement, W1018) became a statement of nothing (or of the object it was named on) instead of an untyped node (statements/expression_statement.sharp); a local alias INSIDE type arguments (`list<s>` after `alias string as s;`) is now followed (context/body_types.sharp `give_local_alias_its_target`: the stand-in alias the type-names stage made gets the type its `alias` statement writes); the components case above |
| two parameters with one name accepted, the second won | parameters were declared without the check locals get | `E0600` at the second (driver/function_body.sharp `report_if_parameter_twice`); D90 |
| `local` / `private` / `protected` functions of a module usable from anywhere | access of functions was only checked for functions of a class | `warn_global_function_access` (calls/bind_call.sharp), also for a function named as a value (lambdas/function_references.sharp): `W1022`, as for class functions (D92) |
| `_int64[] == _int32[]`, `k[] == string[]` accepted | 7.8.1 only asked "both arrays?" | `arrays_can_be_the_same` / `report_arrays_never_the_same` (arithmetic/equality.sharp): `E0511` when no array can be both (D91) |
| `account.total` / `x.size` / `s.length` where a number is wanted: messages without "call it" | the help for a function value meeting a non-function target did not exist; E0804 always offered a lambda | `E0410` gets the note and help "call it: `account.total()`" (conversions/mismatch_explanations.sharp); `E0804` offers the lambda only where a function type is expected, else "call it: `x.size()`" (lambdas/function_references.sharp) |
| helps dropped the start of a chain: `m?.n` for `x.m.n`, `(o as <type>)` for `x.o`, `b.ordinal` for `e.b` | a member's value is bound at the NAME node of the member, and `source_text_of` printed that node | `source_text_of` (moved from conversions to `context/tree_reading.sharp`, where every package can use it) prints the whole member chain when given the name of a member - one fix for every message |
| `r * r.value` as the fix for an `int` object | the help appended `.value` to the text without parentheses | `as_operand_text` (conversions/conversion_messages.sharp) parenthesises anything that is not a name, chain, call, index or literal; used for `.value`, `.ordinal`, `!!`, `as t`, `[0]`. A second help says where the `int` came from: "declare `r` as a plain number where it is made: `r : _int64 = ...` (`r := 0` makes an `int` object)" (`add_inferred_box_help`) |
| an undeclared name reported at every use; `for(count; ...)` reported four times with irrelevant helps | one E0400 per place | `session.unknown_names`: E0400 once per name and function; later uses become places of that message (at most three). At a plain `=` (a statement or a `for` start) the help is the declaration: "declare it here: `total := 0;`". `for(count; ...)` gets one message at the start part: "write `for(count := 0; ...)`" (names/not_found.sharp; the habits of other languages are in names/other_language_habits.sharp) |
| `{ f(), f() }`, `c ? f() : f()` with `f` returning nothing: two E0406 | each part reported "no value" | `analyse_value_after` (expressions/values.sharp): the second part is quiet when the first had no value (array_literal_expression.sharp, ternary_expression.sharp) |
| `true++`: "`++` cannot be used on a number" | `true` was sent to E0514 | it is a number that is not a place: `E0711`, as `5++` (assignment/increment_expression.sharp) |
| `base->f()` without a base class; `f(alpha = 1)`; `if("x")`; `brake;`; an enum value's members; `catch(e: string)`; `item()` in a candidate list; "is a `item`" | messages without the help the reader needs | `add_base_without_base_class`, `add_statement_keyword` (+ `same_letters`), named arguments (`call_of_named_argument`, `report_named_argument`: "Sharp has no named arguments", the call with values in order, did-you-mean among the parameters), an enum value lists `_enum_`'s members (names/no_member.sharp, not_found.sharp); text conditions say "text in quotes" with a help (values.sharp); a library class is never suggested to be edited (`sharp_module.is_library`, catch_clause.sharp, throw_statement.sharp); the constructor the compiler added is left out of a candidate list when others exist (`without_added_constructors`, overload_messages.sharp); articles (`a_type_word`, applicability.sharp); `get<svc>("main")` with no named definitions says to ask by type (request_messages.sharp) |
| a lambda that does not fit: E0801 and then E0606 for its body | the expected result was still given to a lambda already reported as not fitting | `lambda_signature_of`: after a mismatch the result is worked out from the lambda's own returns; the no-arrow form with several values gets a hidden `it` of the error type (lambdas/lambda_signature.sharp) |
| a lambda written in place that does not fit the ONE function called: E0501 with "wrap it in a function" | the overload message was used | `report_lambda_against_parameter` (calls/call_by_name.sharp): with one candidate and a lambda argument, the lambda's own rule reports - `E0801` (count) or `E0802` at the wrong parameter, "write `x: decimal`" (`report_written_lambda_against`, lambdas/lambda_signature.sharp, for a lambda already typed) |
| `f := twice<int>;`: "a value is missing here"; `x := (_int64)(_int64);`: "found the type `_int64`" | parser messages | the parser reads `name<types>` as a name with type arguments also when `;`, `)` or `,` follows (`generic_arguments_follow`); the backend then gives `E0805` with "`{ x: int -> twice<int>(x) }`". A function type where a value is expected is named whole (`function_type_end`, parse_expressions.sharp). Frontend tests pass |
| a decimal too big for 64 bits stored in `_int64`; `1e400` | silent | `W1037` (conversions/convert.sharp `warn_if_number_does_not_fit`), `W1038` (literal_expression.sharp) (D97) |
| `class m : sized<string> { override def size_of(item: _int64) }`: E0341 and E0342 | the missing check did not know the function was ATTEMPTED | `not_attempted` (inheritance/overrides.sharp) and `has_override_named` (override_families.sharp): an `override` of that name means E0342 alone |
| `o as fn[]` accepted; `list<fn>.to_array()` | `object` to an array of function values was "tested at run time" | written directly: `E0412` with a note and help (explicit_conversion.sharp, cast_expression.sharp). In an instance body, where the written type names a type parameter: accepted (`is_instance_cast_to_function_array`, D93) |
| one declaration error hid every body error of the program | `main.sharp` skipped stages 5-9 after any earlier error | `analyse_bodies_after_declaration_errors` (main.sharp): the body stages run and their body messages are kept (D94) |

#### What the independent review changed

A second agent went through the 23 gaps again with new wrong programs, read
every new message as printed, and changed this:

| found | fix (where) |
|---|---|
| gap 4 was open: unused generic code was never analysed | the stand-in pass of 4.1 (`driver/unused_blueprints.sharp`, `check_flow_from` in flow.sharp, three lines in main.sharp): `W1039`. It found a real mistake in the standard library (`atomic_ref.write`) |
| after a declaration error, body messages with an `E03xx` / `E09xx` code were dropped wholesale - `new crate<int, int>()` in a body (`E0350`) and `get()` without a type (`E0902`) vanished | `analyse_bodies_after_declaration_errors` drops only repeats: the same code at the same place (`place_and_code`) |
| follow-ons of a declaration error: `d.only()` on a class declared twice, `b.inherited()` / `base->r()` on a class whose base was not found gave `E0402`; a class with a refused base got `E0341` | `sharp_class.members_uncertain`, set where the declaration error is reported (declare.sharp, base_classes.sharp, interfaces.sharp), asked by `members_are_uncertain` (names/no_member.sharp; also by `report_unknown_name` for a plain name inside such a class) and `report_missing_exactly` |
| two parameters with one name: a lambda's got the message for locals ("write `a = ...;`"); a function without a body (interface, native) was not checked | `report_if_already_local` has the parameter wording for two parameters of one list; `analyse_function_body` checks the list of a body-less function too |
| `maybe?.x ?: -1`: `W1027` and `W1006` (the rest of gap 17) | `W1027` alone, with the `?:`'s own fallback in the fix (`fallback_of_elvis_around`, safe_navigation.sharp; `is_number_through_safe_navigation`, elvis_expression.sharp). The help of `W1027` now writes the member (`maybe!!.x`, not `maybe!!.`) |
| `E0805` help for `pair<int, string>` was `{ x: int, string -> pair<int, string>(x) }`; for `box<int>.make<string>` it dropped `box<int>.` | `lambda_calling_generic` (lambdas/function_references.sharp): the function's own parameters, each with the written type argument for its `T`, and the receiver |
| `take(twice<int>)`: "has no overload of type ..." | the candidate's reason says it is a generic function and what to pass (`fits_function_name`, overloads/applicability.sharp) |
| `if(is_ready)` with a function name: no help | `E0411` says "call it: `is_ready()`" (expressions/values.sharp) |
| the help "declare `b` as a plain number" on `a < b` (a `bool` because of both sides) | only when the value has the type of that local (`add_inferred_box_help`) |
| `s.x` on a `decimal`: "the class `double` has members: `new double(s).x`" although `double` has no `x` | only when the number class has that member (`box_class_has`, member_messages.sharp) |
| `object[] == _int64[]` accepted | `E0511`: an `object[]` holds objects, never plain numbers (`arrays_can_be_the_same`) |
| `W1037` for a `_uint64` said "at most 9223372036854775807" | says how such a number is written (hexadecimal) |
| an unknown name used in a lambda of the same function was reported a second time; `for(i = 0; ...)` got "did you mean `f` or `g`?" | the key of `unknown_names` is the enclosing function of a lambda; no guesses for a one-letter name when the declaration is already offered (names/not_found.sharp) |
| `pick(nope).length()` with two `pick`: after `E0400` for `nope`, the first overload's result type was taken and `.length()` judged by it | an overload chosen through an unknown argument, when another one fits and returns another type, is a quiet tie with the error type (`is_a_guess`, overloads/overloads.sharp) |
| the completeness pass took about 600 ms on the library | it looked every function's file name up in a hash map (hashing the path each time); `files_with_errors.has` answers at once when no file has an error: about 130 ms |
| a cast to a type that is not known (`as T[]` in a stand-in) got `W1003` "does nothing" | not for the error type (cast_expression.sharp) |

#### Error and warning codes added

`E0991` (internal), `W1037`, `W1038`, `W1039`. Codes given a wider meaning:
`E0600` (two parameters), `W1022` (functions of a module), `E0511` (arrays
that can never be the same), `E0400` (once per name and function).

#### Differences from the C++ compiler recorded

D90 - D100 in section 5. 7.7.5 and B87 now say what the bootstrap does with an
untyped lambda in `new t[] { }` (D95); 7.20 points at D96 for text
definitions; 7.8.1 and 7.9 have the new rows.

#### Tests

`tests/backend/fail/completeness.sharp` (31 codes, one per line, each line
one message; it also holds unused generic code - wrong: three `W1039`; right,
using its `T` in every way: nothing),
`tests/backend/fail/after_declaration_errors.sharp` (stage 1 to 4 errors
followed by the body errors, and the uses of a class declared twice and of a
class without its base, which stay quiet: E0302 E0301 E0301 E0341 E0342 E0402
E0506 E0350 E0605), `tests/backend/pass/completeness.sharp` (a local alias in type
arguments, related arrays compared, a list of lambdas with `to_array`,
`twice<_int64>(4)` and `<` as a comparison, a whole number past 64 bits kept
as a decimal). Changed: `fail/lambdas.sharp` (two E0501 are now the lambda
rule's E0801), `fail/class_checks.sharp` (`wrong_parameters` now gives E0342 alone, one E0341 fewer), `fail/type_access.sharp` (its cast line cast an unrelated class,
which the bodies now report: the line casts an `object` instead). The
`W1022` for module functions needs two files and was checked by hand.

#### Not done / known gaps

- **Unused generic code** (gap 4) is checked through stand-ins (4.1). Not
  reached: a generic function inside a class that is nested in a generic
  class; and a mistake that only shows with real types (anything through
  `T`). A function whose declared result is `T` is not asked for "every path
  returns" in the stand-in (the flow analysis skips unknown result types).
- **A function whose body contradicts its declared result type** (gap 17)
  still lets the declared type reach its callers, and its own E0410 comes
  after theirs (the callers are analysed first). Reporting it first needs the
  body analysed on demand before the first call is bound; left as it is.
- Gap 19: a user's own generic class that casts `o as T[]` with `T` a function
  type is accepted like the library's (D93); `def give() : fn[]? { return null; }`
  is accepted here and refused by the C++ compiler (a C++ bug: null fits a
  nullable array).
- Gap 23 (7), frontend: `account(...) -> self(id) { }` and `${~x}` keep their
  parser messages.
- After declaration errors, the passes after the bodies (flow and unused
  generics are run; components, initializers, program checks are not) say
  nothing: `E0605` is shown, `W1032` and `E0903` are not. (FIXED in 10.10:
  they all run now, only the completeness self-check waits.) Should the body
  stages throw on a broken program, their messages are dropped silently and
  the declaration errors stand alone.
- `println(twice<int>)` / `println(give)` (a function name given to an
  overloaded function that takes no function): `E0501` with the candidate
  list, no "call it" help.

### 10.9 Completeness, second round

**Status: done, with the gaps listed below.** A second round of critics
compared the bootstrap with the C++ compiler again and listed fifteen
places where it said nothing, said too much, or pointed the wrong way. Each
was reproduced, checked against the C++ compiler, and fixed where its rule
lives. "[C2]" in the code table marks what this round added.

#### What changed, by cause

| gap | cause | fix (where) |
|---|---|---|
| an alias in a class with no word in front was usable from outside (`holder.text` as a global's type, a parameter, a local, `new holder.text[1]`) | `check_type_access` (stage 3) ran before stage 5 added the implied `private` | the flags of an alias are set where it is DECLARED (preprocessor/declare.sharp `alias_flags`): in a class private, in a module public. E0700 has its own text for an alias: "an alias declared in a class is always private", help "write `string` here" (names/type_access.sharp) |
| `private alias` in a module refused from another module, `public alias` in a class accepted from outside: the opposite of the C++ compiler | the written words were taken at their word; the C++ parser drops them (grammar.md 2.1) | the words are ignored, as there (D102), and get `W1040`, which says what the alias really is (checks/modifiers.sharp `report_alias_access_word`) |
| `a[0] = "s"` on a `list<int>`: E0711 "cannot be assigned to", help "give `list<int>` a function put" - which it has | `store_through_index` reported the missing store whenever no `put` FITTED | a `put` that takes the index and refuses only the value means the value is the mistake: `E0410` on the value, naming `list<int>.put` and its parameter (assignment/index_store.sharp `put_refusing_only_the_value`). The "give it a put" help is never given for a library class |
| `when(n) { }`, `when(n) { else -> { } }`, `when { }` accepted | the parser read any number of clauses | `E0100` "this `when` has no clauses" / "has only an `else`" (parse_statements.sharp `report_when_without_clauses`) |
| `class c { ; }`, `def f() { };`, `n : _int64 = 0;;` in a class accepted | a stray `;` was skipped everywhere | in a class it is `E0100` "a `;` cannot stand alone in a class", with "a member that ends with `}` needs no `;`" after a block (parse_declarations.sharp `report_semicolon_in_class`) |
| `import (one) as q;` then `thing`: help "add it to the import list: import ( one )"; `r#thing`: no did-you-mean `q` | the messages knew only plain imports and module names | the help says the module is imported as the group `q`: write `q#thing` (type_names.sharp, names/not_found.sharp, `sharp_file.group_importing`); E0303 looks among the file's groups for did-you-mean and lists them in a note (`module_and_group_names`, `add_group_list_note`) |
| `take("5")` for `n: int`, `a["k"]` on a `list<int>`, `s.equals("y")`: help "use one of its items: `"5"[0]`" | a text literal is an array, so the array help fired when the target was a class | `explain_text_for_object` (conversions/mismatch_explanations.sharp): for a number class "write the number without the quotes: `5`" (or "give a number here"), for any other class "`new string("y")`" |
| foreach over a `hash_map`, `m["a"]` on one: helps to edit the library class | the library rule (`is_library`) was applied only to catch and throw | foreach names what the class hands over: "walk what it hands over (`keys()`, `values()`, `entries()`): `foreach(x in m.keys())`" (loops/iterable.sharp `add_walkable_parts`); `m["a"]` names `m.at("a")`, and on the left of `=` `m.put("a", <value>)` (collections/index_expression.sharp `add_library_item_access`) |
| a 200-term `a + a + ...` and a 100-link `x.next().next()...` crashed the compiler (stack overflow) | the analysis recursed once per operand / link | the left spine of an operator chain is walked in a loop (arithmetic.sharp `analyse_binary`, `operation_with_left`); a chain of links is analysed from its first link up, each handed to the next through `analysis_context.ready_node` (expressions.sharp `analyse_chain_from_front`, `front_link_of`). Tested at 1500 terms and 1000 links |
| 130 nested `if` blocks crashed in the flow pass | `flow_of` had one big frame per level | it dispatches to one small function per statement kind (flow/reachability.sharp); 130 levels now pass. An `else if` ladder is not nesting for any pass: see the review table below. What is still too deep for a pass is `E0992` at the function (`report_nested_too_deeply`, context/reporting.sharp; caught in driver/function_body.sharp and flow/flow.sharp), and that function's typed body is dropped so no later pass walks it |
| 150 nested parentheses: "the file could not be read: " with no reason | the parser's stack overflow was caught by the file reader | the parser catches it itself: `E0103` "the code here is nested too deeply for this compiler" at the token it had reached (parse_declarations.sharp `report_nested_too_deeply`) |
| `class job_failed : exception`: E0324, then E0509 (with the wrong help "remove `-> base(...)`"), E0612, E0613 | the class kept `_object_` as its base, so everything about exceptions failed after the first message | error recovery by repair: after E0324 for a class listed after `:`, the class is given it as its base when it writes none and no circle results (inheritance/interfaces.sharp `can_take_as_base`); a note says so. Two such classes give two errors (D104) |
| W1014's help `x := get("main", runtime)`, a form the C++ compiler refuses | the help was the same everywhere | as the value of a `:=` declaration the help moves the type to the variable: `a : settings = get("main", runtime);` (components/get_expression.sharp `declaration_inferring`); the wider acceptance of the `:=` form is D101 |
| D44 said "same", but the C++ compiler calls `new leaf().plain("x")` ambiguous when `leaf.plain` redeclares `base_node.plain` | only the nearest function was ever a candidate, silently | probed: in Sharp only a function WITHOUT a body is replaced at run time; one with a body is hidden, not overridden (`b : base_node = a_leaf; b.plain(...)` runs the base's, and `override` on it is "overrides nothing" in both compilers). D44 is now "wider", and the redeclaration gets `W1041` with the two ways out (inheritance/hidden_functions.sharp). Functions without parameters (`to_string()`) cannot be called ambiguously and get no warning. The E0342 help for `override` on such a function no longer says only "remove `override`" (overrides.sharp) |
| E0501 for a `visitor<double>` given where a `visitor<string>` is wanted: "list it on the class: `class evaluator : visitor`" | the help used the blueprint's name and did not look at what the class already lists | the help writes `visitor<string>`; when the class lists the same interface with other arguments the note says "`evaluator` has the interface `visitor<double>`, not `visitor<string>`" and the help says to pass something that is one (`other_instance_listed`, mismatch_explanations.sharp) |
| every build printed about 50 warnings about the library (about 700 lines) | warnings were printed wherever they were located | a warning located in a library module is left out unless `-wlib`, and one line counts them: "(51 warning(s) in the standard library not shown; -wlib shows them)" (program/warning_options.sharp `is_in_library`, `library_warnings_line`). Errors in the library are always shown. A side effect: `-werror` no longer turns the library's warnings into errors |
| `ages : _int64 = { ... }` and then seven uses: one E0410 and seven E0405 | each use was judged by the declared type | `local_variable.value_refused`: a local whose written type refused its value is read as the error type (6.2), so every use is quiet - members, `foreach`, an index, an assignment, an argument (statements/local_declaration.sharp `written_type_refused`, primary/found_names.sharp) |
| `current := node;` (a `node?`), then `current.value` and `current.after`: two E0722 | one message per use | `local_variable.null_reported`: the first E0722 on a local says later uses are not reported again and shows the fix at the declaration (members/member_receiver.sharp) |

#### What the independent review changed

A second agent ran the fifteen gaps again with new wrong programs through
both compilers, read every new message as printed, and changed this:

| found | fix (where) |
|---|---|
| a 300-term `a == 1 \|\| a == 2 \|\| ...` (and `&&`) still ended in `E0992`: only the arithmetic operators were walked in a loop | `analyse_binary` walks the left spine of EVERY two-operand operator; `&&` / `\|\|` get their sides from it (`logical_operation`, arithmetic/logical_expression.sharp) |
| a ladder of 300 `else if` ended in `E0992` (the C++ compiler compiles it): an `else if` is an `if` in the else part of an `if`, one level of recursion each | followed in a loop where it is built (`analyse_if`, branches/if_statement.sharp), in the flow analysis (`flow_of_if`, flow/reachability.sharp) and in the path of E0605 (`path_through`, flow/missing_return.sharp). 1000 pass |
| a table `a == 0 ? 0 : a == 1 ? 2 : ...` of 200 ended in `E0992` | `analyse_ternary` follows the chain in a loop: conditions and first branches down, types back up (`ternary_start`, `ternary_of`, operators/ternary_expression.sharp) |
| a 1000-term sum as the value of a global or a field, or in a constructor, CRASHED the compiler (stack overflow outside any `try`): three passes after the bodies walked the typed tree by recursion | `all_expressions_in` (types/typed_tree/tree_parts.sharp) lists a tree in evaluation order with a work stack; `collect_reads` (initializers/initialization_order.sharp) and `check_base_arguments` (initializers/base_calls.sharp) loop over it; `note_first_uses_in_statement` / `_in_expression` (program/first_uses.sharp) keep their own stacks |
| `W1041` was not given for two `static` functions, although the C++ compiler calls `leaf.make("x")` ambiguous exactly as for instance functions (checked; also with a `private` one above) | `defined_above_with_same_parameters` compares static with static and instance with instance; the static case has its own label, note and help (inheritance/hidden_functions.sharp) |
| a wrongly declared local was quiet only about missing MEMBERS: `foreach(a in ages)`, `ages[0]`, `ages = "text"`, `take(ages)` each gave another error; and a local whose value was merely an unknown name was made quiet as well | the local is read as the error type, and only when its type refused a value that was itself fine (6.2; `written_type_refused`). `is_local_declared_wrongly` is gone |
| a text literal for a class that is neither a number class nor something a `string` is (`keep("k")` for `item: k`, `w : list<int> = "x"`): help "write `new string("k")`", which does not fit either | the `new string(...)` help only where a `string` object converts to the wanted type (`_object_`, an interface of string); otherwise the note "text is never a `k`" (`explain_text_for_object`) |
| `takeb("true")` for a `bool`, `taked("1.5")` for a `double`, `takec("c")` for a `char`: "give a number here; for ONE character ... `'a'`" | "write it without the quotes: `true`" / "`1.5`"; one character gets its own spelling: `'c'` (`is_number_text`) |
| `show(l)` with a `list<int>` where an interface is wanted: "list it on the class: `class list : visitor<string>`" - editing the library again | not for a class of the library (`explain_unrelated_classes`) |
| E0700 for an alias of a generic class: "write `list<T>` here", outside the class where no `T` exists | "`list<T>` (with the types given to `box`)" (names/type_access.sharp) |
| `type_names.sharp` had grown to 578 lines | the messages (E0301, E0303 - E0305 and their helpers) are `names/type_name_messages.sharp`, with a header of their own |

Checked and left as built: the alias rules against nested and derived
classes (the C++ compiler refuses both as well); `;` after an inner class,
an `init` block, in an interface; the import-group helps for functions and
globals (`q#helper`), `r#helper()` in a value; `class a : b` with `class b : a`
(no circle is built), a class with its own `base`, an enum after `:`.

#### Error and warning codes added

`E0103` (parser: nested too deeply), `E0992` (a body nested too deeply for
a later pass), `W1040` (a word on an alias), `W1041` (a function that only
hides one above). `W1040` is of the kind `access` (`-waccess`), `W1041` of
the kind `ambig` (`-wambig`). New option: `-wlib`.

#### Differences from the C++ compiler recorded

D44 rewritten (now "wider"); D101 - D104 new.

#### Tests

- `tests/parse/fail/empty_parts.sharp` (the three `when` forms, the three
  `;` in a class), `tests/parse/fail/nested_too_deeply.sharp` (400
  parentheses: `E0103`).
- `tests/backend/fail/aliases.sharp` (four `E0700` of the implied private),
  `fail/alias_words.sharp` (three `W1040`), `fail/recovery.sharp` (two
  `E0324` and nothing after them; `W1041` for an instance and for a static
  function, none for an overload), `fail/second_look.sharp` (the
  index store, the text literals, the library classes, one message per
  wrongly declared or nullable local, the visitor), `fail/nested_too_deeply.sharp`
  (200 nested `if`: `E0992`).
- `tests/backend/pass/long_chains.sharp` (generated): a 400-term sum - in a
  function, a global, a field value and a constructor -, 400 conditions
  joined with `||` and with `&&`, a 300-call chain, a table of 250 `? :`, a
  ladder of 300 `else if`, and 100 nested `if` blocks. The C++ compiler
  compiles and runs it.

#### Not done / known gaps

- Deep NESTING is limited, not removed: about 130 nested `if` blocks, 120
  nested parentheses, 100 calls inside each other's arguments
  (`f(f(f(...)))`), and 120 of the right-nested forms `a = b = c = ...` and
  `a ?: b ?: c ...` are the most the VM's stack allows (the C++ compiler takes
  300 of each). Removing the limit needs the statement passes and the parser
  made iterative (an explicit work stack), or a bigger stack, which only the
  runtime's `-istack` option gives; the bootstrap cannot set it for itself.
  Chains - operators, calls and members, `? :`, `else if` - have no limit.
  Re-checked on 2026-11-14 (runtime stage of the compiler stream), with the
  JIT and with `-nojit`, at 150, 400, 2000 and 10000 levels of nested
  parentheses, `-(...)`, `f(f(...))`, nested `if` blocks and `a = b = ...`:
  every one ends in one `E0103` (or `E0992` for 150 nested `if`s), never in a
  crash or a hang; a 10000-term `a + a + ...` compiles. The limit holds; it is
  still a limit.
- The reports asked for by an option (`-analysis`: the statement and
  expression printers in backend/reports) still walk by recursion: on a very
  long chain they can run out of stack. A normal build does not use them.
- `W1041` is not given in generic classes (their override families are
  linked per instance; a warning per instance would repeat), nor for
  functions without parameters.
- The one-message rule `null_reported` covers locals and parameters only.
  (`value_refused` covers fields too since 10.11: the field's value is
  analysed the first time the field is read.)
- `s.equals("y")` gets "`new string("y")`"; the more natural `s == "y"` is
  not offered (the message does not know the call is `equals`).
- `a := get("main", runtime);` is accepted here and refused by the C++
  compiler (D101): recorded as wider, not made an error.
- After `class sq : shape` (E0324, repaired to `base shape`) the functions
  `shape` declares without a body are not asked of `sq` (E0341 is held back
  for a class whose base was assumed); they are once the `base` is written.
- Library warnings that are hidden are still counted by `-analysis-stats`;
  library errors are never hidden.

### 10.10 Leftovers: rules (compiler stream)

**Status: done, with the gaps listed below.** The analysis build ended with
a list of findings nobody had fixed (the round-3 critics and the "not done"
lists of every stage). This stage took every one of them that is about what
the compiler ACCEPTS, REJECTS or REPORTS MORE THAN ONCE, reproduced it,
checked what the C++ compiler does (by compiling and, where the meaning was
in question, by running what it builds), and fixed it in the stage it
belongs to. Findings that are only about the wording of a message are left
to the next stage; the editor's are the IDE stream's.

#### What changed, by cause

| finding | was | now (where) |
|---|---|---|
| asm `$std.io#__srt_global[_srt_thread_start]` (the library's thread entry point) | the operand became the number 0, the function was not reached from main, a misspelt name was silent | the globals of the module are looked up: a global function (its address; now reached from main) or variable (its slot); `E0923` for a name the module does not declare, with did-you-mean, and for `$m#__srt_global` alone (statements/assembly/asm_globals.sharp) |
| asm `[h]` / `$c[h]` naming an overloaded function | the first one, silently | the first one, with `W1042` naming it (asm_globals.sharp `function_address`) |
| asm `[g]` / `$m#__srt_global[g]` naming a GENERIC function | its address, silently - of a function that does not exist until it is called with types (the C++ compiler: "attempt to get address ... none were found") | `E0805`: it has no address (asm_globals.sharp `report_address_of_generic`) [verifier] |
| asm `$__srt_global[name]` with no module | `E0301` "no type named `__srt_global`"; the C++ compiler accepts it | the globals of the module the code is in. And `x : app#__srt_global?` outside asm, which was silently the error type, is `E0304` (names/type_names.sharp `is_asm_operand`) [verifier] |
| `w : item?` given where an `item` is wanted, with a constructor `item(owner: object)` | the nullable value was wrapped in a NEW `item` built by that constructor: the null rule bypassed and a different object passed | a value that already is the class (or below it) is never built into a new one: `E0501` / `E0721` with the `w!!` help, as the C++ compiler (conversions/class_conversion.sharp) |
| `v : c = 5;` with both `init(x: _int64)` and `c(x: _int64)` | the constructor; the C++-built program runs the init block (checked by running) | a declaration with a value prefers a fitting `init(x)` (`with_init_block_preferred`; D5, 7.5.4). `-analysis` names what a construction runs |
| two `init(x)` blocks with the same parameter types | accepted; a use got `E0503` "more than one constructor" | `E0333` at the second (checks/duplicates.sharp `check_init_blocks`); the second is `declared_twice` and no longer a choice, so the use is quiet. The same flag is set by `E0332` |
| `class g<T> { init(x: T) { } init(y: _int64) { } }` used as `g<_int64>` | accepted (the C++ compiler: "function `init<g<_int64>>` is already defined") | `E0333` for the instance, as `E0331` / `E0332` are (generics/instance_duplicates.sharp `check_instance_init_blocks`); two blocks that take the very same type are never "a tie" for `E0503` [verifier] |
| `class shape : drawable { def draw(n: _int64); }` - the interface function declared again without a body | accepted: the promise silently became an abstract function nobody had to define | `E0346` (inheritance/overrides.sharp), and no `E0341` on top. A body-less function of a base CLASS may still be declared again below |
| `mutate i { def f() { ... } }` on an interface | `E0379` "cannot have a body" - a false error on a program the C++ compiler accepts and runs | accepted (members/interface_members.sharp): the function belongs to the interface, like an extension function (D105) |
| `obfuscate { twice<int>(x: int) }` with a generic `twice` | accepted silently | `E0952` (obfuscation/obfuscated_names.sharp); the C++ compiler: "generic functions are not supported." |
| `const c : _int64 = 1, d = 2;` | `const` for `c` only | for every name of the statement, as for fields and globals and in the C++ compiler (statements/local_declaration.sharp, `shared_flags`) |
| `b : _int8 = 200;`, `c : _uint8 = 256;`, `a : _int8 = 127 + 1;` | silent (the value is cut) | `W1043`, with the value really stored; a cast (`300 as _int8`) is quiet (conversions/convert.sharp) |
| `if(x = 5)`, `while(x = 0)` | silent in both compilers | `W1045` "did you mean `==`?"; `if((x = next()))` is quiet (expressions/values.sharp `analyse_condition`) |
| `x / 0` with a constant whole-number zero | silent; the VM gives infinity (`-9223372036854775808` stored in an `_int64`, checked by running) | `W1044` (arithmetic/constant_folding.sharp); `1.0 / 0.0` and `0.0 / 0.0`, the library's way of writing infinity and NaN, are quiet. `x % 0` stays `E0427`. The warned division is not folded, so `n : _int64 = 4 / 0;` is `W1044` alone (it was followed by `W1037` "does not fit" about the infinity); `x /= 0` is `W1044` and `x %= 0` is `E0427` too (assignment/compound_assignment.sharp) [verifier] |
| `colour.red++`, `colour.red += 1` | `E0511` / `E0712` with a help to declare an operator in the enum | `E0711`, as for `colour.red = x` (assignment/increment_expression.sharp, compound_assignment.sharp) |
| `f := tool.promise;` with `static def promise();` | accepted; the C++-built program dies at the call (SIGSEGV, checked) | `E0510`, as for a static call of it (lambdas/function_references.sharp) |
| a misspelt base class: `class oops base exeption` | `E0301` plus `E0509` at `-> base(...)`, `E0612` / `E0613` at every throw and catch, `E0351` at every bound | `E0301` alone: a class whose bases are uncertain (`sharp_class.inherits_uncertainly()`) is not judged as a `throwable`, against a bound, or by its base's constructors |
| a misspelt import: `import ( std.utils )` | `E0303` plus one `E0301` per use of every type the import was meant to bring | `E0303` alone: a type name that is not found, but is declared in a module the file does not import, is quiet when one of the modules the misspelt import most likely meant - the ones its `E0303` offers as "did you mean" - declares it (`sharp_file.meant_imports`, `brought_by_a_misspelt_import`). The same holds for a VALUE name (`range.of(1, 2)`, `sqrt(2.0)`: `E0400` held back in names/not_found.sharp). A name no such module declares is still reported [the verifier narrowed the rule from "any module the file does not import" and added the value names] |
| `new hash_map<string, (decimal)(decimal)>()` (a type argument a bound refuses) | `E0351`, then `E0405` inside the library's `entry<...>`, which the refused instance had made | `E0351` alone: an instance made while resolving a refused instance is marked `inside_refused_instance` and its bodies are not analysed (generics/resolve_written_type.sharp) |
| a function that returns values but declares no result type | `E0606`, then `E0406` at every caller (5 - 10 messages) | `E0606` alone (with its help `def total(...) : _int64`); a call of such a function is quiet (conversions/conversion_messages.sharp, looked up in the callee's tree once) |
| `def point.twice()` clashing with `point.twice()` | `E0331`, then `E0421` / `E0400` from the body analysed as a module function | `E0331` alone: the refused function leaves the module's names and its body is not analysed until it is renamed (members/extension_functions.sharp `refuse_extension`) |
| `def box<X>.f() : X` - an extension function with its own name for the class's `T` | `E0301` for every `X`, `E0400` for every field in the body, `E0402` at every call | accepted: `X` is box's `T` (names/renamed_type_parameters.sharp, the last lookup rule of a type name) [verifier] |
| `def pair<B, A>.f()`, `def pair<string, B>.f()`, `def box<list<T>>.f()` | `E0396` "internal: could not work out what `B?` is", then `E0400` in the body | `E0384` alone, with the form to write (members/extension_functions.sharp) [verifier] |
| `wrap<list<T>>` making ever deeper instances | `E0352` twice (once per written type in the chain) | once per chain (generics/generic_classes.sharp `first_report_of_endless_nesting`) |
| `class h { alias _int64 as x; class x { } }` | `E0302` and `W1001` for the same pair | `E0302` alone (checks/name_conflicts.sharp) |
| `component a { }` written twice | `E0900` and `E0901` for each definition of the second block | `E0900` alone; definitions inside one block are still compared (components/components.sharp) |
| `return; 5;` | `W1002` and `W1018` for the one dead statement | `W1002` alone: the flow pass takes back the `W1018` it explains (context/reporting.sharp `withdraw_earlier`) |
| after declaration errors | flow and unused generics ran; components, obfuscate blocks, initializers and the program checks did not: `E0903`, `E0930`, `W1032` waited for the next build | they run too (main.sharp `analyse_bodies_after_declaration_errors`); only the completeness self-check waits for a sound program. Should one of these later passes fail on the broken program (an exception), only the later passes are given up: what the bodies found is kept [verifier] |
| `def main() { }` written twice | `E0331` and, now that the program checks run after declaration errors, `E0930` "a second entry point" for the same function | `E0331` alone (`declared_twice` is set by `E0331` as well; program/main_function.sharp) [verifier] |
| the order of the messages | per stage: module functions before methods, declarations before bodies (an error at line 150 before one at line 84) | in the order of the program text (frontend/diagnostics/message_order.sharp `in_source_order`, a stable merge sort; D106): the files in the order they were given on the command line - the same on every run, which "the file whose message was found first" was not, files being parsed in parallel - and by place inside a file. Twelve fail tests had their expected order rewritten; each was checked to hold exactly the same codes before (tmp script `reorder.sh`), and the verifier ran every fail test through the compiler as it was before this stage: the same codes in each |

Checked and found already right (no change): duplicate parameters of a
function WITHOUT a body are `E0600` (driver/function_body.sharp); a local
alias inside type arguments (`new box<s>()`) is resolved; enum values
`= "text"` / `= other` are `E0940` (initializers); `E0900` / `E0906` are in
the components pass; stage 5 runs after declaration errors (in
`analyse_bodies_after_declaration_errors`), so `class x : cmp<int, int, int>`
gets `E0341` and `E0350` in one build.

#### Error and warning codes added

`E0333`, `E0346`, `E0384`, `E0952`, `W1042` (kind ambig), `W1043` (kind cast),
`W1044`, `W1045` (kind general). `E0923`, `E0510` and `E0805` gained a case
each. Next free: `E0334`, `E0347`, `E0385`, `E0953`, `W1046`.

#### Objects and fields added

- `sharp_function.declared_twice` - a second function / constructor / init(x)
  with the same parameters, reported; left out of construction choices, and
  not a second `main`.
- `sharp_function.returns_unwritten_value` - -1 not asked; 1 when declared
  without a result type but its body returns a value.
- `sharp_class.inside_refused_instance` - made while resolving an instance a
  bound refused; its bodies are not analysed.
- `sharp_class.inherits_uncertainly()` - the class or one above it has
  `members_uncertain`.
- `sharp_type_parameter.endless_nesting_reported` - E0352 was given for the
  chain of instances of its class or function.
- `sharp_file.missing_imports` - the module names of its import list that
  name no module; `sharp_file.meant_imports` - the modules those names most
  likely meant (the did-you-mean of `E0303`);
  `sharp_file.brought_by_a_misspelt_import(name, types_only)`.

#### Tests

`tests/backend/fail/rules_declarations.sharp` (the declaration rules and the
follow-ons that must stay quiet), `fail/rules_bodies.sharp` (the body rules
and warnings), `pass/rules.sharp` (the correct forms next to each rule). Each
new rule was checked against the C++ compiler: it refuses `E0333`, `E0346`,
`E0952`, the `__srt_global` names and the nullable argument; the warnings
are about programs it accepts.

#### Gates and timing

Full gates: library 0 errors; 177 suite cases, 0 failing, 0 VM crashes;
frontend 15 passed; backend 44 passed (the three new files included);
`GATES: PASS`. The bootstrap compiled over its own source: 0 errors. On the
library with `-wlib` none of the new warnings fires. No new pass, so no new
entry on the `time:` line; the line is unchanged within noise (bodies 856 -
890 ms).

#### Not done / known gaps

- `x := colour.red; x++;` (a local of an enum type) is still `E0511` "has no
  operator++" with a help to declare one in the enum: correct to refuse
  (the C++ compiler does too), the help belongs to the message stage.
- `new c().f()` for a function `mutate` added to an interface `i` that `c`
  implements is accepted (the C++ compiler refuses it; D105, wider).
- A field initialiser that converts through `init(x)`: the C++-built program
  printed nothing for `class h { f : c = 8; }` (neither the init block nor
  the constructor ran) - a C++ bug, not followed; the bootstrap runs the init
  block there as for locals and globals.
- The quiet rule after a misspelt import holds for the modules the `E0303`
  suggests. An import that resembles no module of the program (a file left
  out of the build) quiets nothing: every name it would have brought is
  reported.
- `mutate i { def g(); }` - a function WITHOUT a body added to an interface -
  is then asked of the classes that list `i` (`override def g()` in them is
  accepted). The C++ compiler does not see it as part of the interface
  ("overrides nothing"). Left as it is: the bootstrap's reading is the
  consistent one.
- `return; f(); 5;` - `W1002` at the first dead statement, and still `W1018`
  for the `5;` after it (another statement).
- `h : _int8 = 0xff;` gets `W1043` (255 does not fit; stored as -1) although
  the eight bits written are exactly the eight bits stored. Kept: the value
  read back is not the one written. `0xff as _int8` is the quiet form.
- `e : _uint64 = 0 - 1;` is quiet (64-bit types are outside `W1043`).
- A rejected extension function's body is not analysed at all, so a second
  mistake inside it shows only after the rename.
- Everything about message WORDING from section A of the leftovers (R3-6,
  R3-14 - R3-17, R3-19, R3-24, R3-25, R3-30 - R3-37) and the `E0352` wording
  of R3-29 are the next stage's. [done in 10.11, as is the `x := colour.red;
  x++;` help above]

### 10.11 Leftovers: messages (compiler stream)

**Status: done, with the gaps listed below.** The stage before (10.10)
fixed what the compiler accepts and refuses. This one took every leftover
that is about the MESSAGES: one mistake setting off many, helps that
propose a fix that does not build, habits of other languages that section
9.3 promised and nobody built, did-you-mean that offers the wrong names,
and messages that say what is wrong but not how to fix it. The bar is the
owner's: what is wrong, why, and the text to paste. And, new with this
stage, **a help's text is tested by pasting it in and compiling the
result** (tests/helps.sh, below).

#### Habits of other languages: the parser translates and repairs

Most of these are syntax errors, so they are the parser's. Before, the
parser said what any parser says - "expected `)`", "expected a value, found
`=`" - and often a second message after it. Now the package
`frontend/syntax/parser/habits` (one file per family of habits; it was one
file of 667 lines until the verifying pass split it) recognises each habit
where the parser meets it, reports it once as `E0100` with the Sharp spelling, and then
REPAIRS the parse: it reads on as if the Sharp form had been written, so
the rest of the statement and of the file parse normally. Textbook name:
error productions, or phrase-level recovery (dragon book 4.1.4).

| written | message and the help's text |
|---|---|
| `if(a == 1 and b == 2)`, `or` | "`and` is written `&&` in Sharp" - `a == 1 && b == 2` |
| `if(not done)` | "`not` is written `!`" - `!done` (`not(x)` is left alone: it may be a call) |
| `x === 1`, `!==` | "`===` is written `==`" |
| `ok &&= check();`, `\|\|=` | "Sharp has no `&&=`" - `ok = ok && check()` |
| `o instanceof string` | "`instanceof` is written `is`" - `o is string` |
| `(string) o`, `(_int64) x` | "a cast is written `value as type`" - `o as string`. `(f)(x)` (a call) and `(a) - b` are not taken for casts |
| `x = nil;` | "`nil` is not a value ... written `null`" |
| `new nil[3]` | "expected a type, found `nil`" - for any object `object` |
| `function f() { }`, `fn`, `func`, `fun`, `void f()` | "a function is declared with `def`" - `def f()` |
| `int add(int a, int b) { }` | ... "and its result type comes after the parameters" - `def add(a: int, b: int) : int` (C parameters turned round) |
| `class b extends a`, `implements i` | `class b base a`, `class b : i` |
| `class b base a base c` | "a class has one base class" - `class b base a : c` |
| `b() -> super(1)` | "the base class's constructor is called with `base`" - `-> base(1)` |
| `def f() : string := s;` | "with a result type, a one-expression body is written with `=`" - `def f() : string = s;` |
| `x : int = 0; get { ... }` in a function | "the local variable `x` cannot have a getter" (or setter) - a function, or a field of a class |
| `interface i { ; }` | "a `;` cannot stand alone in an interface" (it said "in a class") |
| `for(x in items)` | "a loop over the items of something is written `foreach`" - `foreach(x in items)` |
| `foreach(i in 0..10)`, `for(i in 0..10)`, `..=`, `..<` | "Sharp has no range `0..10`" - `for(i := 0; i <= 10; i++)` (`<` for `..<`; the note gives Rust's reading of a plain `..`). Was two "expected the name of a member" with no help |

**Keywords with a slip** (`habits/misspelt_keywords.sharp`, added by the
verifying pass). To the tokenizer `retrun` is a name, so `retrun total;` was
"expected `;`" - or was taken for a C declaration and got the help "write:
`total : retrun;`"; `dfe helper() { }` got "write: `def helper() : dfe`",
`fro(i := 0; i < 3; i++)` two "expected" messages, `calss point { }` "a
declaration is written name first". Now, where a statement or declaration
starts with a name and has a SHAPE no correct line has, the parser looks for
the keyword one slip away (one typing mistake by the did-you-mean rule, or
the same letters in another order), reports "`retrun` is not a word of
Sharp: did you mean the keyword `return`?" with the help "write `return`",
turns the token into that keyword and reads the line as it was meant:

| shape (never correct Sharp) | keywords tried | example |
|---|---|---|
| a name, then a value on the same line, at the start of a statement | `return`, `throw` | `retrun total;` `thorw new oops();` |
| a `;` directly between the parentheses of a "call" | `for` | `fro(i := 0; i < 3; i++)` |
| `name in` right after the `(` | `foreach`, `for` | `foreahc(x in items)` |
| a statement right after a call's `)`, on its line | `while`, `if`, `for`, `foreach` | `whlie(x < 3) x++;` `iff(x) retrun 7;` |
| after an `if`'s body: a name, then `if` | `else` | `esle if(b) { }` |
| a name, a name, then `{`, `base`, `:` or `<` | `class`, `interface`, `enum`, `mutate`, `component` | `calss point { }` |
| a name, a name, then `(`, `.` or `<` | `def` | `dfe helper() { }` |
| a name, a type, then `as` | `alias` | `alais _int64 as id;` |
| top of a file: a name and a name, or a name and `(` | `mod`, `import` | `improt (std);` |

Correct code is never touched: both the shape and the closeness must hold.
In a declaration two names in a row can also be C's `type name`, so there a
three-letter keyword (`def`, `mod`) is only taken for the same letters or
another case (`dfe`, `Def`), and `dec total;` keeps its C-habit message. A
slip whose line DOES parse is not the parser's: `whlie(x) { }` and `esle {
}` are calls with a trailing lambda, `brek;` a name alone - correct Sharp
when such a function or variable exists - so the name stage translates
those when the name is not found (table below). Cost: for a statement that
starts `name(`, one look along its tokens to the `)`; the text of the name
is only compared once the shape is already wrong.

A habit's message is reported once the whole expression is read (the help
shows both sides of the `and`), which can put it after a message further
right: the parser now sorts its messages by place before handing them on
(parse_declarations.sharp `parse_source`).

The parser's precedence-climbing loop recurses once per link of a
`? :` chain, so the habit checks were written to add no local to
`parse_binary` or `parse_unary`: a first version added three, and the
250-link chain of tests/backend/pass/long_chains.sharp ran out of stack.

Habits whose wrong form PARSES are translated where the name is not found
(backend/compiler/names/other_language_habits.sharp):

| written | help |
|---|---|
| `elif(x == 2) { }`, `elsif`, `elseif` | `else if(x == 2) { ... }` |
| `switch(x) { }` | the note about `when`, and `when(x) { 1 -> { ... } else -> { ... } }` |
| `whlie(x) { }`, `iff(...)`, `fro(...)` - a keyword with a slip, in call form | "did you mean the keyword `while`?" `while(x) { ... }` |
| `len(items)` | `sizeof(items)` for an array, `items.size()` for a list or a text |
| `math.sqrt(x)` with no `math` anywhere | "`math` is the last part of the module `std.math`": `import (std.math)` and `sqrt(x)`, or `std.math#sqrt(x)`; no did-you-mean (it offered `path`) |
| `Math.sqrt(x)` (Java's spelling) | the same, with "written `math` (names are case-sensitive)"; it was "declare it above this line: `Math := <value>;`" |
| `retrun(x);` - `return` with a slip, in call form, as a statement | "did you mean the keyword `return`? write `return x;`" (it was "declare the function `def retrun(...)`") |
| `if(a) { } esle { }` - a name close to `else` and a block (a call with a trailing lambda) | "did you mean the keyword `else`? write `else { ... }`" (it was "declare the function `def esle(...)`") [verifying pass] |
| `println("total", n)` - Python's print of several values | the note that `println` takes one value, and `println("total ${n}")` (a text literal goes in as its text) |
| `s.equals("y")` | Java's comparison: `s == "y"` (before the narrower `new string("y")`) |

#### One mistake, one message

| mistake | was | now |
|---|---|---|
| an operator declared with the left operand as a parameter too (`def operator+(a: v, b: v)`) | `W1025`, then `E0511` at every use - each with the note "`v` has `operator+` for `v`", which contradicted it | `W1025` and ONE `E0511` "`v.operator+` is declared with 2 parameter(s), so `+` cannot use it", with the declaration as a related place, every later use as "used here too", and the declaration to write (`def operator+(b: v) : v`). The same for unary operators and `++` (arithmetic/operator_messages.sharp `reported_as_misdeclared`) |
| `area(widht = 2, height = 3)` | an `E0400` per named argument | one per call; the other names are places of it |
| `x && y` with `and` / `not` / C casts / `nil` | "expected `)`" and then "a declaration is written name first" | one `E0100` (above) |

| a FIELD (or global) declared with a value its type refuses: `ages : _int64 = { 1, 2 };`, then `ages[0]`, `ages.put(..)` in the functions of the class | `E0410` at the value and one `E0414` / `E0405` per use (the first two passes of this stage wrote "found already right"; it was not) | one `E0410`. The cause was ORDER: function bodies are analysed before field values, so the uses were judged before the refusal was known. Now the first READ of a field analyses its value on the spot (`field_value_refused` in driver/lazy_types.sharp - the mechanism `name := value` fields already used), and a field whose written type refused its value is read as the error type (primary/field_access.sharp), exactly like a local. `written_type_refused` moved to conversions/convert.sharp, where both use it [verifying pass] |
| `println(a, b)`, `println(total)` with a function `total` | the six nearest of the fourteen `println`s, each with why it does not fit, and the helpful line last | the note and the help only: when a call is understood as Python's print of several values, or as a function named where its result was meant, the list of candidates is left out (it stays when there is one candidate) [verifying pass] |

Checked and found already right: a misspelt base class, a misspelt import,
a function returning values with no result type and a refused type argument
were made quiet by 10.10.

#### Helps that proposed what does not build

| message | was | now |
|---|---|---|
| `E0366` (an operator outside a class) | `def _int64.operator+(...)` - an extension function, which the C++ compiler cannot compile, of a number, which nothing can be added to | for a class: the operator in the class, or `mutate money { def operator+(other: money) : money { ... } }`; for a number: the note that its operators are built in, and the form for a class |
| `E0379` (a body in an interface) | "add it from outside: `def i.f(...)`" (an extension function) | "move the function out of the interface into a `mutate` of it: `mutate i { def f() { ... } }`" - the form the C++ compiler accepts and runs (10.10) |
| `E0381` (`mutate` of a stable class), `E0383` | `def h.name() { ... }` | a function that takes one: `def name(value: h) { ... }` |
| `E0903` / `E0906` for an interface | `single { new clock(...) }` - `new` of an interface is itself an error | "no definition provides a `clock`; this one provides a `fixed_clock`" (exact types only) and `new fixed_clock(1000) as clock`; with no definition near: `single { new <class>(...) as clock }` |
| `E0412` for `(c - 32) as _int8` | `c - 32.value` (reads `c - (32.value)`), labelled on `as` | labelled on the value; `(c - 32).value` when `value` already is the type asked for, else `n := c - 32;` then `n.value as T`. (`(c - 32).value as _int8` makes the C++ compiler dump core, see below.) Every help that puts `.value`, `.ordinal`, `!!` or `?.` after an expression now puts an operation in parentheses (`as_operand_text`) |
| `W1027` with a call as receiver | `r.find("ZZ") != null ? r.find("ZZ")!!.price : -1` - the lookup twice | `found := r.find("ZZ");` and then `found != null ? found!!.price : -1` |
| `E0606` (a function returning values without a result type) | `def total(...) : _int32` | `def total(n: _int64) : _int64` - the real parameters, and a literal's type as a declared variable would get it |
| `E0511` on two strings `<`, on text literals | "compare texts with `==`" (`==` does not order); "`def operator<` in `string`" (edit the library) | `s.compare_to(t) < 0` / `new string("a").compare_to("b") < 0`; a library class is never offered for editing |
| `E0511` `chars == word` (`_int8[]`, `string`) | note only | "put the `string` first": `word == chars` |
| `E0423` for an `object?` in a template | "+ takes numbers, text and objects" (it does not take `object`) | the note says objects OF A CLASS; for `object` the help is a cast into a variable of the class |
| `E0355` inside generic code (`T?` with T a number) | helps to edit the `T?` - in the library too | the type argument to change at the use: `list<double>`, `pick<double>`; for a generic FUNCTION the call that made the instance is a related place and the note "found while checking `pick<decimal>`" is added (overloads/type_inference.sharp `point_at_the_call`) |
| `E0352` from a generic function | "a class refers to itself ... move the wrapped use out of the generic class" | worded about the function: "`wrap` calls itself with its type parameter wrapped once more" |
| the group help of `E0301` (`import (std.util) as u;` then `new range(1, 5)`) | `u#range` (the `new`, the arguments, any type arguments dropped) | `new u#range(1, 5)` |
| `E0504` `counts.size()` on a field | note only | "`x.counts.size` is a field: read it without `( )`" |
| the dictionary note of `E0419` | `{ as <k, v> : ... }`, a form the parser refuses, and shown even when `as` was written | `{ as <string, int> "a" : 1 }`; with `as` written: "the values of this dictionary are the type written after `as`" |
| `W1014`, `W1041` | "the C++ compiler refuses ..." | "the first Sharp compiler refuses ..." |
| `E0410` a number where a `string` is wanted (`s : string = n + 1;`) | `"" + n + 1` - which is the text of `n` followed by `1` | `"" + (n + 1)`: an operation goes in parentheses (`as_operand_text`) [second pass] |
| `E0405` `n.to_string()` on a plain number | `new long(n).to_string` - the call's `( )` dropped | `new long(n).to_string()`; the same for `(o as <class>).f(...)` (`call_after`) [second pass] |
| `E0405` `a.first()` on an `_int64[]` | "to use an item write `a[index].first`" - an `_int64` has no members either | the item form only when the items are of a class that has that member [second pass] |

| a function NAMED where its result was meant, anywhere but a declaration: `println(total)`, `total + 1`, `"n: " + total`, `2 * total`, `take(scaled)` | `E0501` / `E0511` with notes only ("a function value can only be called ...") | the note "`total` names the function; only calling it gives its result" and "call it: `total()`" (`scaled(...)` when the function takes parameters) - `call_meant_by` / `add_call_it` in conversions/mismatch_explanations.sharp, used by the operator messages and by overloads/overload_messages.sharp `add_call_of_named_function` (not when a candidate takes a function at that place) [verifying pass] |

#### Helps added where there was none

By the verifying pass: `E0501` / `E0506` with ONE candidate and the wrong
number of arguments ("give a value for `b` as well"; "`add` takes 2: remove
`3`" - `add_count_fix`); `E0351` for a function type as a type argument
(`hash_map<string, (decimal)(decimal)>`: the class to keep the function in,
`class action { run : (decimal)(decimal); action(f: (decimal)(decimal)) {
run = f; } }`, and `action` as the type argument).

`E0324` for a number or an enum after `:` (keep it in a field); `E0414` for
`a[0][0]` (one dimension: `a[row * width + column]`) and for a number
variable (declare an array); `E0511` / `E0712` on an enum value (`x =
colour.green;`, or `x.ordinal`); `E0511` `-s` / `s++` on a string
(`s.to_int()`); `E0712` on an array (a loop over the items, or a `list`).

#### Did-you-mean

- A short name (four letters or fewer) must start with the same letter, or
  with its first two letters swapped (`frontend/diagnostics/similar_names.sharp`
  `starts_unrelated`): `math` is no longer "close" to `path`.
- `E0402` with no close name lists the members that START like the name
  written first, the closest of them first: `to_decimal` gets `to_double()`,
  `to_int()` ... (`no_member.sharp` `written_prefix`), where it listed
  `data`, `replace()` ...
- Text literals are no longer called arrays: `_int8[]` is "text as
  characters (`_int8[]`, what a text literal is)" in every message that uses
  `described_type`, and the keys of `{ "a" : 1 }` are "text literals".

#### Library warnings

Checked, not changed: a user program prints none of the 51 warnings the
standard library has, only the line `(51 warning(s) in the standard library
not shown; -wlib shows them)`; `-wlib` prints exactly 51.

#### The help tests: tests/helps.sh

Each file in `tests/helps/` is a program with one mistake and, in its first
lines, the help it must get and where that help's text goes:

```
// expect: E0100
// help: x == 1 && x == 2
// replace: x == 1 and x == 2
```

(`// before: <text>` puts the help's text in front of that text, `// dots:`
says what stands for a `...` in it, `// remove:` takes text away for a help
that says "move".) The script compiles the program and checks the code and
the help line; pastes the text; compiles the result with this compiler (no
error at all) AND with the C++ compiler (it must build). 58 programs cover
every help of this stage that has concrete text (44 from the first pass,
seven added by the second: the range, `for(x in ...)`, `retrun(x);`,
`println` of several values, `Math.sqrt`, `n.to_string()` and `"" + (n + 1)`;
seven by the verifying pass: the keyword slips `retrun x;`, `fro(...)`,
`calss`, `esle { }`, a function named as an argument and as an operand, and
a function type as a type argument). The `// expect:` code must be the code
of ONE of the messages printed, not necessarily the first (a warning may
come before it). Run by gates.sh after the
backend tests. Writing it found five helps that did not build - the cast
(C++ crash), the interface `mutate` (it said "add" where "move" was meant),
`std.math#sqrt` returning a `double`, the group help dropping `<...>`, and
a C function head with a number type not being recognised - all fixed.

#### Tests

`tests/parse/fail/habits_repaired.sharp` (nineteen habits, one `E0100` each,
and five correct lines that look like habits), `tests/parse/fail/
misspelt_keywords.sharp` (sixteen slips, one `E0100` each, and a function of
correct code whose names are close to keywords), `tests/backend/fail/
messages.sharp` (one message per mistake for the backend's cases; the
verifying pass added `esle { }`, a field and a global with a refused value
used five times, and a function named in three places),
`tests/helps/*.sharp` (58) with `tests/helps.sh`. `fail/completeness.sharp`
expects one `E0400` fewer: `area(width = 2, heigth = 3)` is now one message
for the call (the first pass changed the rule and left that test's expect
line behind, so the full gates failed; the second pass found it).

#### Gates and timing

No new pass: nothing on the `time:` line. Full gates after the verifying
pass: library 0 errors; 177 suite cases, 0 failing, 0 VM crashes; frontend
17 passed; backend 45 passed; help tests 58 passed (one C++ compiler crash
retried, see below); `GATES: PASS`. The bootstrap compiled over its own
source: 0 errors. Reading a field now analyses its value first; that moves
work inside `bodies`, it adds none (`bodies` 1.1 - 1.4 s on the library, as
before). The parse of the library is unchanged in time (the
habit checks run only where the parser is about to report an error, or on
a name followed by another value - which correct code never has; the one
exception is the range check, which looks over the tokens between `in` and
the `)` of every `foreach` once).

#### Not done / known gaps

- A habit's help is built from the source text as written: in a statement
  with two habits (`x === 1 or not y`) the help for `or` still contains the
  `===` and the `not`.
- `E0423` for an `object` in a template has no concrete fix: what class the
  object is cannot be known.
- The local accessor's helps (`def x_value() : _int64 { ... }`) are not in
  the help tests: they mean moving code out of the function, which the test
  format cannot express.
- `x : int = 0; get { }` is caught only right after the local's
  declaration; a `get { }` further down is still "expected `(`".
- `elif` / `switch` are recognised only in call form with parentheses
  (`switch x { }` is still a parse error without a translation).
- Keyword slips: only the keywords and shapes of the table above. `retrun;`
  / `brek;` alone and the call forms are the name stage's, so they are
  reported only when the file has no syntax error. Not covered: a slip in a
  modifier (`statc def f()`), in `try` / `catch` / `finally` / `do` / `when`
  without a statement after the `)`, in `new`, `as`, `is`, `in`, `base`;
  `const z = 3;` is still "expected `;`". After `improt std.io;` is read as
  `import`, the missing parentheses are a second (real) message.
- A body that contradicts its declared result is three messages: `def name() : _int64 {
  return "x"; }` called as `s : string = name();` twice gives `E0410` at the
  `return` and at each caller. Each is a true statement about its own line
  and they are printed in source order; which line is the mistake is not
  known, so none is dropped.
- A misspelt TYPE (`strng`) written five times is five `E0301`, each with
  the did-you-mean: five places to correct.
- After `area(widht = 2, height = 3)` only the first name gets the
  did-you-mean among the parameters.
- Habits not translated: `x++` written `x += 1` is fine; `print` in Python 2
  form, `console.log`, `String.format` and `printf` are plain unknown names
  (with whatever did-you-mean finds).
- The deep-nesting limit is unchanged (10.9).

#### C++ compiler bugs found

- `s : string = "abc"; c := s.at(0); b : _int8 = (c - 32).value as _int8;`
  makes the C++ compiler dump core (`(c - 32).value` alone, and
  `(c.value - 32) as _int8`, compile and run). Likely the same family as the
  paren-expression crash of 10.4.
- Two C++ compiles run at the same time in one folder spoil each other's
  build (library errors such as "could not resolve symbol `t`" in
  loopable.sharp); each compiles when run alone. tests/helps.sh gives every
  compile a folder of its own.
- The C++ compiler now and then dies with a segmentation fault when four of
  it run at once on a loaded machine: one full gates run of the verifying
  pass failed on `helps/field_called.sharp` that way (no error message; the
  same file then compiled six times out of six alone). tests/helps.sh
  therefore runs the C++ compiler a second time when it died on a signal
  WITHOUT reporting an error, and names the file in a `note:` line of its
  summary. A refusal (any error message) is never retried.

A trap of the language, not a bug: `string` has an `operator=` that
changes the object itself, so `b := a; b = "x";` changes `a` too (checked
with the C++-built program). The bootstrap's own code must never assign a
text to a string variable that was given another string variable's
object - W1027's help first did, and printed `found := found;`.

## 11. Decisions on what the first draft left open

The first draft ended with ten open questions "for the owner", each with a
default. They are decided here, by two rules: what the C++ compiler does to a
program that runs is the language; where it has no behaviour worth the name,
the textbook structure is used. The owner can overturn any of them; each says
what would have to change.

1. **Base to derived without `as` (D6): accepted, with the warning `W1017`.**
   The C++ compiler accepts it in declarations AND in arguments, so it is
   part of how existing calls resolve. The typed tree marks the conversion as
   needing a run-time check. To make it an error in 2.0: `leniency_severity`,
   one line; the count of `W1017` on the library says how much code that
   touches.
2. **`const`, private functions and private constructors (D7, D8): warnings
   `W1021` and `W1022`.** The C++ compiler enforces none of them, so existing
   code may break them unknowingly; a warning shows where without rejecting a
   program that runs. Private FIELDS and classes stay errors, as they are in
   the C++ compiler. To make them errors: the same one line each.
3. **Every body of every generic instance is analysed. No "only what is
   used".** Measured, not assumed: the C++ compiler in `-eager` mode (which
   checks every library body for every instance) accepts the library and all
   175 suite cases with 0 errors, so full analysis is what the existing code
   already satisfies. (One case showed errors in one run under load and none
   in two reruns; see 10.7, bug 8.) Skipping unused members would hide
   mistakes and make "fully analysed" untrue for code generation. If the
   body pass is too slow, it is made faster (10.0); if it still cannot meet
   the budget, the stage reports the number and the owner decides - the stage
   does not quietly skip.
4. **Enum numbering (D15): the C++ rule, written down.** A value has a fixed
   number only when the program gives one (`= n`) or it follows a value that
   has one. All others are "different from each other", nothing more; the
   C++ source says this is on purpose. Analysis records which is which; the
   code generator numbers the rest. The code generator is free to keep the
   numbers distinct across enums, as the C++ compiler happens to; analysis
   depends on neither.
5. **Null checks the compiler remembers ("smart casts"): not in this
   build.** The rule is the C++ compiler's, which is narrower than the first
   draft said (7.11): only `.member` on a `t?` and giving a `t?` to a `t` are
   errors; indexing, operators, `foreach`, `lock`, `throw` on a value that may
   be null are left to the run time. Tightening those, and remembering
   `if(x != null)`, belong together and to 2.0; reachability (stage 8) is the
   analysis they would build on.
6. **Types of number results (D13, D14).** Truth values stay exactly as the
   C++ compiler has them (`_int32` for comparisons and logic, `_int8` for
   `!`, null tests and `is`), because the type picks the overload and
   `"" + (a < b)` must keep printing `1`. Integer arithmetic stays `_int32`
   EXCEPT when an operand is a 64-bit integer; then it is 64-bit. That one
   change is on purpose: the C++ rule sends an `_int64` sum to
   `string.operator+(_int32)` and prints the wrong number. It changes which
   overload runs only for programs whose result was already wrong.
7. **No `main` (D18): not an analysis error.** The library alone must analyse
   cleanly. Stage 10 still finds `main`, checks its signature and duplicates,
   and records it (`sharp_program.main_function`); building an executable
   without one is for code generation to refuse.
8. **The `foreach` variable over a `t[]??`: a `t?`** (D25). The C++ source
   means it (`for_each_statement.cpp:85`) and loses it on the way; nothing in
   the library or the suite declares such an array, so no existing program
   changes. Over a plain `t[]` the variable is a `t`, as before, although
   items of an object array can be null - that is kept.
9. **Modifier order (`E0362`): kept.** The C++ compiler rejects another
   order, so it is a rule of the language; the message gives the corrected
   order ready to paste. Relaxing it later accepts more and breaks nothing.
10. **Outer-class members from a nested class: not visible by plain name,
    static ones included** (checked). Kept: the C++ compiler rejects it.
    Allowing it later accepts more and breaks nothing; the "not found"
    message already names `outer.name`.
11. **The details the first draft marked "to be confirmed"** were probed and
    are now stated in section 7: a box does not cast to a number (`E0412`);
    `as` gives the type as written, `?` removed; `?.` on a number member
    (D24); `-> base(...)` arguments may read fields and get start values; an
    enum's number is read as `.ordinal` (a `decimal`); constructors take only
    access modifiers (D30); an interface declares functions only (D29); the
    own name of a generic class means the instance; warnings do not fail the
    gates.

Still for the owner, because they are choices about language 2.0 and not
about this build: whether `W1017`, `W1021`, `W1022` become errors; smart
casts; relaxing 9 and 10. None of them blocks a stage.

---

## 12. Handing over to code generation: how to walk the analysed program

This section is for whoever writes the code generator. Analysis has decided
everything that can be decided without knowing the bytecode; what is left is
how to lay it out. Everything below is reached from ONE object, the
`analysed_program` that `check_program` returns
(`backend/types/program/analysed_program.sharp`); `main.sharp` holds it in
`analysed` after the `program checks` stage.

**Generate only when `analysed.errors == 0`.** The count is made after the
warning switches, so a warning that -werror turned into an error counts. A tree that contains an error
holds `bound_error` nodes (type: the error type) where the mistake was; there
is nothing to generate for them. With no error, the self-check of stage 9
guarantees that no expression of any tree has the error type.

### 12.1 Where to start, and what to emit

| field of `analysed_program` | what to do with it |
|---|---|
| `main_function`, `main_form` | the entry point. Null: the program has no `main` - refuse to build an executable ("could not locate main method"). `main_form` (`MAIN_ARGS_INT`, `MAIN_ARGS`, `MAIN_NO_ARGS`, `MAIN_NO_ARGS_INT`) says which function value of `platform_class` to fill in: `main`, `main2`, `main3`, `main4` respectively (the fields `private static excuse main : (string[])(int)` ... of `platform.kernel#platform`) |
| `startup_function` | `platform.kernel#platform.srt_init`: where the VM starts. It calls `static_init()`, then `tls_init()`, then the `main` function value |
| `platform_class` | its functions `static_init` and `tls_init` are EMPTY in the library (E0961 guards that); the generator writes their bodies: see 12.4 |
| `required_classes` | the classes the VM makes objects of itself (`nullptr_exception`, `string`, `thread`, the reflection classes ...; 7.26). Emit every one, with EVERY field of it and of the classes above it, whether or not the program names it: they are already in `reached_classes` / `reached_fields`. With a `main` and one of them missing the build has stopped with `E0962`. The C++ compiler's second half - refusing one an `obfuscate` block renamed - is yours to do when names are written |
| `functions` | every function with a typed body: functions, constructors, `init` blocks, getters, setters, lambdas, and the instances of generic functions. Each once. Generate a body for each (or only for the reached ones) |
| `classes` | every real class with analysed members: the classes as written that are not generic, and every generic INSTANCE (`list<string>`). Never a blueprint (`list<T>`): there is nothing to generate for one. Each needs its object layout and its method table |
| `modules` | every module: its global variables (`sharp_module.fields`) are the program's static storage; `global_initializers` / `fiber_initializers` say how they are set up |
| `lambdas` | the lambdas alone (each is also in `functions`) |
| `reached_functions`, `reached_classes`, `reached_fields` | what the program can come to from `srt_init`, `main` and the `required_classes` (`sharp_declaration.reached` is the same, as a flag). The rest may be left out - this is the C++ compiler's "used". A function the VM calls BY NAME that nothing in the program names is not reached: add such roots yourself |

### 12.2 One function

`sharp_function`:

| field | meaning |
|---|---|
| `kind` | `D_FUNCTION`, `D_CONSTRUCTOR`, `D_INIT` (`init { }`, `static init { }`, `init(x)`), `D_GETTER`, `D_SETTER`; `is_lambda == 1` for a lambda (a `D_FUNCTION`) |
| `owner` | its class (null: a global function); `has(FLAG_STATIC)`: no `self` |
| `parameters`, `result_type` | resolved types; `result_type.is_nil()`: returns nothing |
| `locals` | EVERY local variable, parameters first and in order, then each local in the order declared, the compiler's hidden ones included. `local_variable.index` is its position in this list: use it as the slot number. Skip a variable whose `function` is not this function: it is a variable of the function around a lambda, captured (below) |
| `local_variable.is_captured` | a lambda inside uses it: it does not live in a slot but in this function's CLOSURE RECORD (`closure`, a `closure_record` with `variables`) - allocate the record when the function starts, and read and write the variable there |
| `local_variable.is_hidden` | made by the compiler: the subject of a `when`, the array and position of a `foreach`, the object of a `lock`. An ordinary slot |
| `bound_body` | the typed tree: a `bound_block`; for `def f() := e;` and `: t = e;` one `bound_return` (or a `bound_expression_statement` when the function returns nothing) |
| `bound_base_call` | constructors and `init(x)`: the base constructor to run first - written (`-> base(args)`) or implied (the base's constructor without parameters). A `bound_call` with dispatch `CALL_CONSTRUCTOR` and no object; null only where there is nothing above (`_object_` itself) |
| `is_synthesized` | a constructor the compiler added (no parameters, empty body) |
| `takes_record`, `enclosing` | a lambda that uses a captured variable or `self` takes its closure record as a hidden last parameter; `enclosing` is the function it is written in |
| `overrides`, `overridden_by`, `family_root` | the virtual family: every function with the same `family_root` shares one slot of the method table |
| `uses` | every declaration its tree names, once: the dependency list (the C++ compiler's `dependencies`) |
| `reached` | the program can come to it |

What runs for `new c(args)` (7.15.2): the constructor's `bound_base_call`,
then, once per class before its first use, the class's static part, then
`c.instance_initializers` in order (each a field with `bound_initializer`, or
a start object `default_constructor`, or `injected_from`; or an `init` block
to call), then the constructor's `bound_body`. A field without any of those
starts as 0 / null.

### 12.3 The typed tree, node by node

Every node has `kind` (`BX_...` for an expression, `BS_...` for a
statement), `unit` and `node` (where it is written: for line numbers and
debug information). Every expression also has `type` (a `resolved_type`),
`constant` (when not null, the value is known: emit it instead of the
computation) and `literal`. Every implicit conversion is a node of its own
(`bound_conversion`), so the type of each node is exactly what it gives.

The question of 4.2 - could code be generated for this node from its own
fields and nothing else? - answered for every class:

| node | fields to read | answer |
|---|---|---|
| `bound_literal`, `bound_null` | `constant`, `type` | yes |
| `bound_local` | `variable` (`index` = slot), `depth` (how many lambdas lie between the use and the function the variable belongs to: follow the closure record's `parent` that many times when the variable `is_captured`) | yes |
| `bound_field` | `field`, `on` (the object; null for a static field or a global, whose storage is `field.owner` / `field.module`) | yes |
| `bound_enum_value` | `value` (a static field of its enum, set up in the enum's `static_initializers`) | yes |
| `bound_self`, `bound_base` | `depth` (inside a lambda: through the closure record, `captures_self`) | yes |
| `bound_type_name` | only ever the `on` of a static member, or the operand of `is` / `as` / `sizeof`: nothing to emit | yes |
| `bound_call` | `function`, `on` (null: static), `arguments` (already converted to the parameter types), `dispatch` (`CALL_STATIC`, `CALL_DIRECT`, `CALL_VIRTUAL` - through the method table; narrow it with `overridden_by` if you like -, `CALL_BASE` - the base's own body, `CALL_CONSTRUCTOR`), `written_as` (an operator, a getter, a setter, `o[i]`, `put`, `operator=`, a foreach's `get_elements`: for debug information only - the call is a call) | yes |
| `bound_value_call` | `callee` (a function value), `arguments` | yes |
| `bound_new` | `class_of`, `constructor`, `arguments` | yes |
| `bound_conversion`, `bound_cast` | `operand`, `how.kind` (`CONV_IDENTITY`, `CONV_NUMERIC`, `CONV_NULL`, `CONV_TO_NULLABLE`, `CONV_UPCAST`, `CONV_TO_OBJECT`, `CONV_DOWNCAST` with `how.checked == 1`: test at run time, `CONV_UNBOX` with `how.field` the `value` field of the number class, `CONV_CONSTRUCT` (a number into an `int`, a text into a `string` ...) with `how.function` the constructor or `init(x)` and `how.inner` what to do to the value first, `CONV_RESULT_IGNORED`), `how.from`, `how.to` | yes |
| `bound_is` | `operand`, `tested` | yes |
| `bound_not_null` | `operand`: nothing to emit - the value is passed on as it is. The C++ compiler emits no test for `!!` (probed: `y := x!!;` with a null `x` raises nothing); the `nullptr_exception` comes later, from whatever USES the value (codegen.md 6.5, Q2) | yes |
| `bound_safe_navigation`, `bound_receiver` | `receiver` is evaluated once; when it is null the WHOLE chain gives null (`type` is nullable); `rest` is the chain, in which `bound_receiver` stands for the receiver's value | yes |
| `bound_elvis`, `bound_ternary` | `value` / `fallback`; `condition` / `when_true` / `when_false` | yes |
| `bound_sizeof` | `operand` (an array; or `constant` for a text literal) | yes |
| `bound_index` | `array`, `index` (an `o[i]` on an object is a `bound_call` of `operator[]`) | yes |
| `bound_new_array`, `bound_array_literal` | `element`, `size` / `items` (converted) | yes |
| `bound_string_template` | `start` (the first piece, a `string`), `appends` (each a `bound_call` that adds the next piece) | yes |
| `bound_dictionary` | `pair_class`, `pair_constructor`, `dictionary_class`, `dictionary_constructor`, `keys`, `values` | yes |
| `bound_binary`, `bound_logical`, `bound_unary` | `operation` (the token kind), `operands` (`OPERANDS_INTEGER`, `OPERANDS_DECIMAL`, `OPERANDS_REFERENCE` for `==` on objects), `left` / `right` / `operand`; `&&` / `||` short-circuit | yes |
| `bound_increment`, `bound_assign` | `target`, `value`, `operation` (0 for `=`), `operands`, `store` (the result back to the target's type), `getter` / `setter` (a property) | yes |
| `bound_function_reference` | `function` (static: a function value without a record) | yes |
| `bound_lambda` | `function` (its body is that function's `bound_body`), `record` (the closure record the new function value carries; null when it captures nothing) | yes |
| `bound_get_component` | `definition`: evaluate `definition.value`; for a `single` definition (`is_factory == 0`) only the first time - the generator gives each `single` definition one storage place of its own | yes, with that one choice left to the generator |
| `bound_error`, `bound_pending` | never in a program without errors | - |
| `bound_block`, `bound_expression_statement`, `bound_empty` | `statements`; `expression` (its value is dropped) | yes |
| `bound_local_declaration` | `variable`, `value` (converted; null: none), `default_constructor` (`n : int;` starts as `int()`), `injected_from` | yes |
| `bound_if`, `bound_while`, `bound_do_while`, `bound_for` | `condition` (a plain number: 0 is false), the parts; `bound_loop.is_endless`, `left_by_break`, `continued` | yes |
| `bound_foreach` | `collection` (an array, or a `bound_call` of `get_elements()`), `array_variable`, `index_variable` (hidden slots), `variable`, `item_conversion` | yes |
| `bound_when`, `bound_when_clause` | `subject_variable` (hidden: the subject, evaluated once), `clauses` (`tests`: finished conditions, already "subject == value"), `else_part` | yes |
| `bound_return`, `bound_break`, `bound_continue` | `value`; `loop` (the target); `leaves`: the `lock` and `try ... finally` statements the jump passes on its way out, innermost first - release the lock / run the finally for each | yes |
| `bound_throw` | `value` (a `throwable`) | yes |
| `bound_try`, `bound_catch` | `body`, `catches` (`caught` class, `variable` or null), `finally_part`. How a `finally` is entered (normally, by a jump, by an exception) is the generator's to lay out | yes |
| `bound_lock` | `locked`, `held` (hidden slot), `body` | yes |
| `bound_asm` | `instructions` with `operands` resolved (`variable`, `field`, `function`, `class_of`, `label`) | yes |
| any statement | `completes`: execution can go on after it (stage 8). A function returning nothing whose body `completes` needs its `return` at the end | yes |

### 12.4 Setting up the program

- **`platform.static_init`** (written by the generator): for every module in
  `modules` order, its `global_initializers` in order; and for every class
  its `static_initializers` - the enum values (each made with
  `well_known.enum_constructor`, `std#_enum_(name, ordinal)`; the number is
  `sharp_enum_value.constant` when known, else `bound_value` worked out at
  start, else - neither - any number different from the enum's others), the
  static field values in reading order, then the `static init { }` blocks.
  Each list is already in an order in which a value comes after the values
  it reads (D70); a field in a circle is in the written order (W1031).
- **`platform.tls_init`**: every `fiber_initializers` list (the `localized`
  values), once per fiber.
- **Objects**: `instance_initializers` as above (12.2).
- **Components**: an injected field or local (`injected_from`) receives the
  value of that definition, made as for `bound_get_component`.

### 12.4.1 A fix found by code generation (codegen.md 13.1)

`base_class_of` (postprocessor/names/type_names.sharp), the stage-3 lookup
of the class a class's written `base` names, used to overwrite the base of
a GENERIC INSTANCE that resolve_bases (inheritance/class_bases.sharp) had
already worked out. For the library's `number<_int8, bool>` (through
`class bool base number<_int8, bool>` and `number<T, NT> base
base_number<NT>`) the base became the blueprint `base_number<NT>`, and the
implied base call of every constructor of the instance named the BLUEPRINT's
constructor (`-analysis` printed `call base_number() (constructor)` where the
other instances say `call base_number<int>()`). Nothing in analysis noticed;
code generation does, because it must call a real function. The lookup now
leaves an instance whose bases are resolved alone. No message and no test
result changed.

### 12.4.2 A second fix found by code generation (codegen.md 13.2)

`analyse_is` (compiler/expressions/operators/is_expression.sharp) gave a
plain number tested against a number type the constant 1 only when the two
types were the same (`n is _int64` for an `_int64` n), and left `n is
decimal` to be tested when the program runs. A number in a register carries
no type, so there is nothing to test: the C++ compiler makes every such test
the constant 1 (an explicit match from number to number; probed: `n is
decimal`, `n is _int32`, `b is _int64` with `b : _int8` all print 1), and the
library relies on it - `number<T>.parse` asks `number is decimal` for every
number class. The rule is now "a plain number `is` any number type: 1". No
message and no test result changed.

### 12.4.3 A third: `is` with a plain number on one side, and against `object` (codegen.md 13.2)

Found by the verifier of the stage "Expressions and calls". `analyse_is` left
three kinds of test to the generator that no instruction of the VM can make:
a plain number against a class or `object` (`n is string`), an object against
a plain number type (`thing is _int64`), and anything against `object` /
`object[]`. The C++ compiler was probed: the first two print 0; the third
prints 1 whatever the value holds. So:

- exactly one side a plain number: the constant 0 (a number is never an
  object, an object never a plain number);
- against `object` / `object[]`: the constant 1 when the value cannot be null
  (`cannot_be_null`: not `t?`, not `null`, not of type `object`, which may hold
  null without the `?`: D33); otherwise there is no constant and the
  generator tests "is there an object" - null gives 0, where the C++-built
  program gives 1 (codegen.md section 10, G29, which is also where the same
  difference for `maybe_dog is animal` is recorded).

No message changed.

### 12.5 What is left to the generator

Slots beyond the order of `locals`, registers, labels, the constant pool and
the method tables; the numbers of enum values the program does not fix; one
storage place per `single` definition; whether a field value's closure
record is made per object or per lambda; narrowing `CALL_VIRTUAL` to a direct
call. Nothing else should need a question back to analysis - if it does, the
missing fact belongs in the objects, and this section should say where.
