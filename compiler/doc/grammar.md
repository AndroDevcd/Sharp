# Sharp grammar, as the C++ compiler accepts it

This is the specification the self-hosted parser is written from. It records what
`lib/grammar/frontend/tokenizer/tokenizer.cpp` and `lib/grammar/frontend/parser/Parser.cpp`
do today, including the odd parts. Nothing is tidied.

Sources and how to read the citations:

- `tokenizer.cpp`, `token.h`, `tokentype.h`: the lexer.
- `Parser.cpp`, `Parser.h`, `Ast.h`: the parser. Line numbers are those of `Parser.cpp` at the time of writing (4,799 lines).
- `doc/grammars.txt` (removed from the repository in November 2026; it is in the git history): the author's old draft. Rule names are taken from it wherever its nonterminal still describes something real (section 7 compares the two).
- Claims marked "(compiled)" were checked by compiling a small file with the current `sharpc`; "(ran)" means the program was also executed.
- `UNCERTAIN:` marks anything not fully established.

Parser conventions that matter when reading the C++:

- `_current` is the last token consumed. `peek(1)` is the next token. Every `expect(...)`/`expectIdentifier(...)` first advances, then checks.
- A statement or declaration parser is entered with `_current` ON its first token (the keyword). Many of them do `_current--; expect(branch, "while")` to re-read it.
- `Token == "text"` is false for string and character literals whatever their text (`token.h`, `operator==`). So `")"` or `'{'` as a literal never matches punctuation.
- Keywords are ordinary `IDENTIFIER` tokens. The parser tells them apart by text (`isKeyword`, line 4531).
- `errors->enterProtectedMode()` starts a speculative section. `errors->pass()` DISCARDS the errors collected in it; `errors->fail()` REPORTS them. The names read backwards.

---

## 1. Lexical grammar

The tokenizer works on bytes and uses C `isalpha` / `isdigit` / `isalnum`. Source for the whole section: `tokenizer.cpp`, `tokenizer::parse`, lines 110-355, unless another function is named.

### 1.1 Whitespace and comments

```ebnf
whitespace = " " | "\t" | "\b" | "\v" | "\r" | "\f" | "\n" ;
<!-- tokenizer.cpp: parse, lines 131-144 -->
line_comment = "//" { any_char_except_newline } ( "\n" | end_of_file ) ;
<!-- tokenizer.cpp: parse, lines 178-189 -->
block_comment = "/*" { block_comment | any_char } "*/" ;
<!-- tokenizer.cpp: parse, lines 190-215 -->
```

- Block comments nest: every `/*` inside one must be closed by its own `*/`. An unclosed block comment is the error "expected `*/` at end of block comment".
- Comments produce no token. There is no documentation-comment token.
- Any character not listed in this section (for example `~`, `` ` ``, `\` outside a literal) is the error "unexpected symbol" (compiled).

### 1.2 Identifiers and keywords

```ebnf
identifier_token = ( letter | "_" ) { letter | digit | "_" } ;
<!-- tokenizer.cpp: parseIdentifier, line 838 -->
identifier = identifier_token ;   (* that is not a keyword *)
<!-- Parser.cpp: expectIdentifier, line 4567; isKeyword, line 4531 -->
```

- An identifier must contain at least one letter. `_` and `_1` are the error "expected at least 1 letter in identifier" (compiled).
- Letters are ASCII only.

Keywords (`isKeyword`, line 4531). None of them can be used as a name anywhere, not even as a member name after `.` (`def get(...)` is "expected identifier", compiled):

```
mod import class interface enum mutate alias obfuscate component single factory inject
def operator init get set
public private protected static local const ext stable native excuse override localized
if else while do for foreach when try catch finally throw return break continue goto lock asm
new self base null nil true false sizeof as is in
object decimal _int8 _int16 _int32 _int64 _uint8 _uint16 _uint32 _uint64
```

Notes:

- `goto` is reserved but has no statement. `nil` is only valid as a return type.
- `bool` is a keyword: the built-in type of `true` and `false`, like `_int64` or `decimal`.
- `int`, `string`, `var`, `boolean`, `long`, `char` and so on are NOT keywords. They are library classes and aliases.
- `keep`, `keep_inclusive`, `inclusive` (obfuscate options) and `it` (implicit lambda parameter) are not keywords. `switch`, `case`, `default` are not keywords either (the predicates `isSwitchStatement` / `isSwitchDeclarator` exist but nothing calls them).
- The native types are the last line of the list (`isNativeType`, line 4245).
- The assembly mnemonics and register names (section 2.7) are only special inside `asm { }`.

### 1.3 Number literals

Decimal integers and decimals are the same token kind (`INTEGER_LITERAL`); hex is `HEX_LITERAL`. There is no sign in the token: `-5` is `-` then `5`.

```ebnf
number_literal = digit { digit | "_" } [ "." digit { digit } ] [ exponent ] ;
<!-- tokenizer.cpp: parseNumber, lines 533-629 -->
exponent = ( "e" | "E" ) [ "+" | "-" ] digit { digit } ;
<!-- tokenizer.cpp: parseNumber, lines 579-614 -->
hex_literal = "0x" [ any_char ] { hex_digit | "_" } ;
<!-- tokenizer.cpp: parseNumber, lines 508-532 -->
hex_digit = digit | "a".."h" | "A".."H" ;
<!-- tokenizer.cpp: ishexnum macro, lines 59-61 -->
```

Exactly what the code accepts (the EBNF above is an approximation, these rules are the truth):

- Underscores: allowed after at least one digit, any number of them, also several in a row and at the end (`1_000`, `1__0`, `1_` all compile). Not allowed right after the `.`, and not anywhere after the `e`.
- `.` belongs to the number only when a digit follows it. `5.foo` is `5` `.` `foo` (and the parser then rejects `.` after a number literal). `.5` is `.` `5`. A second `.` followed by a digit (`1.2.3`) is the error "double decimal" (compiled).
- `e` / `E`: the next character must be `+`, `-` or a digit, otherwise "expected `+`, `-`, or digit" (`12e;`, compiled). So a number may not be directly followed by a name that starts with `e`.
- Exponent sign trap: once an `e` has been seen, the FIRST `+` or `-` met anywhere later in the digits is swallowed into the literal. `1e2-1` is ONE token and evaluates to 100, not 99 (ran). `1e-2-1` is `1e-2` `-` `1` because the sign slot is already used.
- No suffixes. `10L` is `10` followed by the identifier `L` (compiled: "expected `;`"). `0X1F` with a capital X is `0` followed by the identifier `X1F` (compiled).
- Hex: only lower-case `0x`. The digit test accepts the letters a-h, so `0xFG` is one hex token (compiled). The character right after `0x` is never checked (the loop tests a stale `current`), so it is taken into the token whatever it is: `0x_1` compiles. `0x` alone is also a token.
- Digits directly followed by letters split into two tokens: `123abc` is `123` `abc`.

### 1.4 Character literals

```ebnf
character_literal = "'" ( plain_char | "\" char_escape ) "'" ;
<!-- tokenizer.cpp: parseChar, line 632 -->
char_escape = letter | "0" | "\" | '"' | "'" ;
<!-- tokenizer.cpp: parseChar, lines 674-685 -->
```

- Exactly one character or one escape. `''` and `'ab'` are errors (compiled).
- Escape values (`from_escaped_string`, line 856): `\n \t \b \v \r \f \0 \a \\` have their usual meaning. Any other accepted escape yields the character itself: `\'` is `'`, `\"` is `"`, `\q` is `q` (compiled). There are no hex or unicode escapes.

### 1.5 String literals and interpolation

```ebnf
string_literal = '"' { string_char | "\" string_escape | interpolation } '"' ;
<!-- tokenizer.cpp: parseString, line 711 -->
string_escape = letter | "\" | '"' | "'" ;
<!-- tokenizer.cpp: parseString, lines 809-818 -->
interpolation = "$$" | "$" identifier_token | "${" tokens "}" ;
<!-- tokenizer.cpp: parseString, lines 750-807; parse, lines 225-252 -->
```

- A string ends at the first unescaped `"` and may not contain a newline ("expected `\"` before end of line").
- `\0` is NOT accepted in a string (only in a character literal): "text preceding `\` must be alpha" (compiled). Escape values are those of 1.4.
- `$` is special only before `$`, `{`, a letter or `_`. Before anything else it is that character (`"$5"`).
- `$$` is one `$`.
- Trap: when a string contains any backslash escape or `$$`, every other lone `$` in it is DROPPED. `"tab\t$5"` prints `tab	5` (ran). (`from_escaped_string`, lines 897-902.)

Interpolation is done entirely in the tokenizer. The parser never sees it; it sees a flat run of tokens joined by `+`:

| Source | Tokens produced |
|---|---|
| `"a $x b"` | `"a "` `+` `x` `+` `" b"` |
| `"$x"` | `""` `+` `x` |
| `y + "$x"` | `y` `+` `x` (no `""` is added when the previous token is `+`) |
| `"a ${e1 e2} b"` | `"a "` `+` `(` e1 e2 `)` `+` `" b"` |
| `"${e}"` | `""` `+` `(` e `)` |
| `"$a$b"` | `""` `+` `a` `+` `b` |

Consequences:

- The name after `$` is one identifier token: letters, digits, `_`. `"$a.b"` is `a` followed by the text `.b` (ran).
- There are no parentheses around the whole string. A postfix after the closing quote binds to the last piece: `"n=$n".size()` is `"n=" + n.size()` (compiled: "could not resolve symbol `size()`"). Operators tighter than `+` on either side also split it.
- `${ }` tracks nested `{ }` with one counter, so a block or lambda inside is fine, and a plain string inside is fine (`"v ${ "in" + n } w"`, ran). A second `${` inside a string that is itself inside `${ }` resets the counter: nested interpolation is broken.
- If `}` is immediately followed by `"` the string ends there; otherwise `)` `+` are emitted and the rest is scanned as a string.
- Interpolation is on for every source file (the flag is only off for the JSON reader).

### 1.6 Operators and punctuation

Longest match, as coded in the switch (lines 225-343). Every token:

| Token | Name | Token | Name | Token | Name |
|---|---|---|---|---|---|
| `(` | LEFTPAREN | `)` | RIGHTPAREN | `{` | LEFTCURLY |
| `}` | RIGHTCURLY | `[` | LEFTBRACE | `]` | RIGHTBRACE |
| `,` | COMMA | `.` | DOT | `;` | SEMICOLON |
| `:` | COLON | `:=` | INFER | `#` | HASH |
| `@` | AT | `$` | DOLLAR | `?` | QUESMK |
| `?:` | ELVIS | `?.` | SAFEDOT | `!` | NOT |
| `!!` | DOUBLEBANG | `!!.` | FORCEDOT | `!=` | NOTEQ |
| `=` | EQUALS | `==` | EQEQ | `->` | PTR |
| `+` | PLUS | `++` | _INC | `+=` | PLUSEQ |
| `-` | MINUS | `--` | _DEC | `-=` | MINUSEQ |
| `*` | MULT | `**` | EXPONENT | `*=` | MULTEQ |
| `/` | _DIV | `/=` | DIVEQ | `%` | _MOD |
| `%=` | MODEQ | `<` | LESSTHAN | `<=` | _LTE |
| `<<` | SHL | `>` | GREATERTHAN | `>=` | _GTE |
| `>>` | SHR | `&` | AND | `&&` | ANDAND |
| `&=` | ANDEQ | `\|` | OR | `\|\|` | OROR |
| `\|=` | OREQ | `^` | XOR | `^=` | XOREQ |

- Not tokens: `<<=`, `>>=`, `**=`, `&&=`, `||=`, `~`, `...`, `::`, `=>`.
- Order of the tests matters: `+=` before `++`; `-=`, then `->`, then `--`; `*=` before `**`; `&=` before `&&`; `|=` before `||`; `<<` before `<=`; `>>` before `>=`; `!!` (then `!!.`) before `!=`; `?:` before `?.`.
- `?:` is one token only when the two characters touch. `a ? : b` and `a ?` newline `: b` are a broken ternary (compiled: "expected expression").
- `a!!.b` is `a` `!!.` `b`. `a !! .b` is three tokens and does not parse (compiled).
- `>>` is always one token; the parser splits it when it closes two generic lists (section 4.4). `>=` is never split: `x : list<int>= v;` fails (compiled), `x : list<list<int>>= v;` works because `>>` is matched first.
- `$` and `@` only have meaning inside `asm { }` (`@` also in `base@type`). `#` is the module separator.
- Token kinds `NUMBER`, `LETTER`, `UNDERSCORE`, `DUBQUOTE`, `SINGQUOTE`, `NEWLINE` exist in `tokentype.h` and are never produced.

---

## 2. Syntactic grammar

EBNF: `rule = ... ;`, `|` alternatives, `[ ]` optional, `{ }` zero or more, terminals in quotes. `identifier` always means a non-keyword identifier.

### 2.1 Compilation unit

```ebnf
compilation_unit = { top_level_decl } ;
<!-- Parser.cpp: parse, line 40 -->
top_level_decl = module_decl
               | import_decl
               | [ access_types ] ( class_decl | interface_decl | enum_decl | method_decl | variable_decl )
               | [ access_types ] alias_decl
               | [ access_types ] mutate_decl
               | [ access_types ] component_decl
               | [ access_types ] obfuscate_decl
               | ";" ;
<!-- Parser.cpp: parse, lines 47-129 -->
module_decl = "mod" identifier { "." identifier } ";" ;
<!-- Parser.cpp: parseModuleDecl, line 4602 -->
import_decl = "import" "(" import_item { "," import_item } ")" [ "as" identifier ] ;
<!-- Parser.cpp: parseImportDecl, line 4692 -->
import_item = identifier { "." identifier } [ "." "*" ] ;
<!-- Parser.cpp: parseImportItem, line 4662 -->
access_types = access_type { access_type } ;
<!-- Parser.cpp: parseAccessTypes, line 4559 -->
access_type = "public" | "private" | "protected" | "static" | "local" | "const"
            | "ext" | "stable" | "native" | "excuse" | "override" ;
<!-- Parser.cpp: isAccessDecl, line 4222 -->
```

- The parser does not require `mod` to come first or to appear once, and imports may appear anywhere at the top level. A second `mod` is a compiler error later ("file module cannot be declared more than once", compiled).
- `import` has no semicolon. `import std.io;` is a syntax error (compiled). A `;` after `import (...)` is accepted as the "unnecessary semicolon" warning.
- `init` is not a top-level declaration (compiled: error).
- The order of the tests is: module, import, class, mutate, def, alias, component, interface, obfuscate, enum, variable, `;`. Anything else is the error "expected class, enum, or import declaration" followed by `parseAll` (section 5.9).
- Access types: the parser accepts ANY sequence of the eleven words, in any order, repeated, in front of every item. What happens to them:

| In front of | Parser | Later check (backend `parse_access_flags`, `astparser/ast_parser.cpp`) |
|---|---|---|
| `mod`, `import`, `;` | error ILLEGAL_ACCESS_DECLARATION | |
| `mutate` | error "access types not allowed here" | |
| `alias` | silently discarded (`parseAliasDeclaration` clears the list; `public alias a as b;` compiles) | the backend would allow public private protected local static, but never sees any |
| `component`, `obfuscate` | silently ignored | |
| `class`, `interface` | stored | public private protected local stable ext |
| `enum` | stored | public private protected local stable ext |
| field (class or global variable) | stored | public private protected static excuse local const |
| local variable | stored | excuse const |
| `def`, operator, delegate, constructor | stored | public private protected override local static native |
| `init` | stored | static |
| `get` / `set` | stored | public private protected local static, and they must equal the field's own |
| any other statement | error ILLEGAL_ACCESS_DECLARATION | |

- The backend also requires this order: (`public` \| `private` \| `protected`), `override`, `excuse`, `local`, `static`, `const`, `stable`, `ext`, `native`; rejects a word repeated twice in a row; and allows `local` only at global scope. `private static local const ext stable native excuse override public class A {}` passes the parser and fails there (compiled).

### 2.2 Classes, interfaces, enums, mutate

```ebnf
class_decl = "class" identifier [ "<" generic_identifier_list ">" ]
             [ "base" reference_identifier ] [ ":" reference_identifier_list ] class_block ;
<!-- Parser.cpp: parseClassDecl, line 3892 -->
generic_identifier_list = generic_identifier { "," generic_identifier } ;
<!-- Parser.cpp: parseGenericIdentifierList, line 4616 -->
generic_identifier = identifier [ "base" utype ] ;
<!-- Parser.cpp: parseGenericIdentifierList, lines 4619-4636 -->
reference_identifier = reference_pointer ;
<!-- Parser.cpp: parseReferencePointer, line 4099 (the draft's name for a reference_pointer used as a type name) -->
reference_identifier_list = reference_identifier { "," reference_identifier } ;
<!-- Parser.cpp: parseReferencePointerList, line 4640 -->
class_block = "{" { class_member } "}" ;
<!-- Parser.cpp: parseClassBlock, line 3928 -->
class_member = [ access_types ] ( class_decl | interface_decl | enum_decl | variable_decl
                                | init_decl | method_decl | constructor_decl )
             | [ access_types ] alias_decl
             | mutate_decl
             | obfuscate_decl ;
<!-- Parser.cpp: parseClassBlock, lines 3933-4047 -->
mutate_decl = "mutate" reference_identifier [ "base" reference_identifier ]
              [ ":" reference_identifier_list ] class_block ;
<!-- Parser.cpp: parseMutateDecl, line 3865 -->
interface_decl = "interface" identifier [ "<" generic_identifier_list ">" ]
                 [ "base" reference_identifier ] interface_block ;
<!-- Parser.cpp: parseInterfaceDecl, line 190 -->
interface_block = "{" { interface_member } "}" ;
<!-- Parser.cpp: parseInterfaceBlock, line 1694 -->
interface_member = [ access_types ] ( interface_decl | delegate_decl ) | [ access_types ] alias_decl ;
<!-- Parser.cpp: parseInterfaceBlock, lines 1699-1815 -->
enum_decl = "enum" identifier "{" enum_identifier_list "}" ;
<!-- Parser.cpp: parseEnumDecl, line 1902; parseEnumBlock, line 1929 -->
enum_identifier_list = enum_identifier { "," enum_identifier } ;
<!-- Parser.cpp: parseEnumBlock, lines 1934-1941 -->
enum_identifier = identifier [ "=" expression ] ;
<!-- Parser.cpp: parseEnumIdentifier, line 1946 -->
```

- A class with `<` after its name becomes `ast_generic_class_decl` (interface: `ast_generic_interface_decl`).
- `base` takes a reference pointer, which must start with a non-keyword identifier: `class a base object {}` is "expected identifier" (compiled). `T base object` in a generic parameter is fine, because that position is a `utype`.
- Interfaces have no `: list`.
- Order of the tests in a class block: module (error), class, mutate, interface, obfuscate, stray `get`/`set` (error), import (error), variable, alias, enum, init, def, constructor. A variable is recognised before anything that follows it, by the lookahead in section 4.1.
- A constructor is any non-keyword identifier followed by `(` (`isConstructorDecl`, line 4365). The parser does not compare the name with the class name.
- `component` is not a class member. `;` inside a class block is an error (compiled), unlike at the top level and inside function bodies.
- An enum needs at least one item and takes no trailing comma (compiled). The value is a full expression.
- In an interface block the parser parses a full `method_decl` and then reports "unexpected method declaration" unless it turned out to be the `;` form (`ast_delegate_decl`). Variables, constructors, `init`, `mutate`, components are parsed and reported as errors.
- Stray braces: see section 5.1.

### 2.3 Functions, constructors, init

```ebnf
method_decl = "def" method_name utype_arg_list method_body ;
<!-- Parser.cpp: parseMethodDecl, line 241 -->
method_name = identifier "<" generic_identifier_list ">"
            | reference_pointer
            | identifier ;
<!-- Parser.cpp: parseMethodDecl, lines 247-260; genericFunctionDeclFollows, line 225 -->
method_body = ":" method_return_type ( "=" expression ";" | ";" | block )
            | infer_expr ";"
            | ";"
            | block ;
<!-- Parser.cpp: parseMethodDecl, lines 262-290 -->
infer_expr = ":=" expression ;
<!-- Parser.cpp: parseMethodDecl, lines 278-281; parseVariableDecl, lines 2140-2142 -->
method_return_type = "nil" | utype ;
<!-- Parser.cpp: parseMethodReturnType, line 3317 -->
delegate_decl = "def" method_name utype_arg_list [ ":" method_return_type ] ";" ;
<!-- Parser.cpp: parseMethodDecl, lines 270-272 and 282-284 (ast_delegate_decl) -->
operator_decl = "def" reference_pointer utype_arg_list method_body ;
<!-- Parser.cpp: parseMethodDecl, lines 254-257 (ast_operator_decl) -->
utype_arg_list = "(" [ utype_arg { "," utype_arg } ] ")" ;
<!-- Parser.cpp: parseUtypeArgList, line 3370 -->
utype_arg = identifier ":" utype ;
<!-- Parser.cpp: parseUtypeArg, line 3272 -->
constructor_decl = identifier utype_arg_list [ base_class_constructor ] block ;
<!-- Parser.cpp: parseConstructor, line 1966 -->
base_class_constructor = "->" "base" value_list ;
<!-- Parser.cpp: parseBaseClassConstructor, line 1959 -->
init_decl = "init" [ utype_arg_list [ base_class_constructor ] ] block ;
<!-- Parser.cpp: parseInitDecl, line 1912 -->
```

- `delegate_decl` and `operator_decl` are not separate parse paths. They are `method_decl` whose AST type was changed: the `;` body gives `ast_delegate_decl`; a name that is not "identifier directly followed by `(`" gives `ast_operator_decl`. That second case covers `def operator+(...)` and also dotted names such as `def some_class.name(...)` (an extension function).
  UNCERTAIN: the dotted form at the top level compiles to "internal error: unknown ast type: 16" (compiled). I did not find a working example in the tests or the library, only the comment at line 222 that mentions `def list<T>.something(`.
- Body forms: `def f() { }`, `def f(): t { }`, `def f(): t = expr;`, `def f() := expr;`, `def f();`, `def f(): t;`. There is no `def f() = expr;` (compiled: "expected `{`").
- A `def` body must start with `{` (error "expected `{`" otherwise, compiled). Constructor and `init` bodies are a plain `block`, which also accepts one statement without braces: `box(v: int) self->v = v;` and `init v = 1;` compile.
- Parameters always have a name and a type. No default values, no trailing comma, no varargs (compiled).
- `native def f(x: decimal): decimal;` is the `;` form with the `native` access type. A `;`-form function outside a class parses and is rejected later ("delegate function missing enclosing class", compiled).
- `init(args) { }` becomes `ast_init_func_decl`. The comment says it must have exactly one parameter; the parser does not check.

### 2.4 Variables, properties, aliases

```ebnf
variable_decl = [ storage_locality ] variable_first { "," variable_next } ";" [ getter_setter ] ;
<!-- Parser.cpp: parseVariableDecl, line 2111 -->
storage_locality = "localized" ;
<!-- Parser.cpp: isStorageType, line 4269 -->
variable_first = [ inject_request ] identifier [ ":" utype [ "=" expression ] | infer_expr ] ;
<!-- Parser.cpp: parseVariableDecl, lines 2125-2147 -->
variable_next = [ inject_request ] identifier [ "=" expression ] ;
<!-- Parser.cpp: parseVariableDecl, lines 2148-2159 (recursion > 1) -->
inject_request = "inject" [ "(" identifier ")" ] ;
<!-- Parser.cpp: parseInjectRequest, line 2090 -->
getter_setter = getter [ setter ] | setter [ getter ] ;
<!-- Parser.cpp: parseVariableDecl, lines 2161-2195; parseGetterSetterAccessTypes, line 2101 -->
getter = [ access_types ] "get" block ;
<!-- Parser.cpp: parseGetter, line 2070 -->
setter = [ access_types ] "set" identifier "->" block ;
<!-- Parser.cpp: parseSetter, line 2079 -->
alias_decl = "alias" utype "as" identifier ";" ;
<!-- Parser.cpp: parseAliasDeclaration, line 393 -->
```

- The same `variable_decl` is used for global variables, fields, local variables and the first part of a `for`. It is entered only on the lookahead of section 4.1.
- Forms: `x : t;`  `x : t = e;`  `x := e;`  `x := 1, y = 2;` (later names take only `= e`; they share the first one's type or infer from their own value).
- A bare `x;` matches `variable_first` but can only be reached through `for (x; ...)` or after `inject` (the entry lookahead needs `:` or `:=` otherwise). `for (i; i < 3; i++)` parses and declares a new local `i` (compiled: "already defined").
- `get` and `set` blocks belong to the declaration before them and come AFTER its `;`. Either order, one of each. Their block may be a single statement (`get return w;` compiles).
- The getter/setter lookahead runs after EVERY variable declaration, also local ones (section 5.5).
- `alias` is also a statement (local alias).

### 2.5 Dependency injection and obfuscation

```ebnf
component_decl = "component" [ identifier ] "{" component_type_list "}" ;
<!-- Parser.cpp: parseComponentDeclaration, line 468 -->
component_type_list = { component_type } ;
<!-- Parser.cpp: parseComponentTypeList, line 408 -->
component_type = single_type | factory_type ;
<!-- Parser.cpp: parseTypeDefinition, line 420 -->
single_type = "single" [ component_name ] "{" expression "}" ;
<!-- Parser.cpp: parseSingleTypeDefinition, line 427 -->
factory_type = "factory" [ component_name ] "{" expression "}" ;
<!-- Parser.cpp: parseFactoryTypeDefinition, line 440 -->
component_name = "(" string_literal ")" ;
<!-- Parser.cpp: parseComponentName, line 453 -->
obfuscate_decl = "obfuscate" [ "-" obfuscate_option ] obfuscate_block ;
<!-- Parser.cpp: parseObfuscateDecl, line 140 -->
obfuscate_option = "keep" | "keep_inclusive" | "inclusive" ;
<!-- Parser.cpp: isObfuscationOption, line 134 -->
obfuscate_block = "{" [ obfuscate_element { "," obfuscate_element } ] "}" ;
<!-- Parser.cpp: parseObfuscateBlock, line 169 -->
obfuscate_element = string_literal | utype [ utype_arg_list_opt ] ;
<!-- Parser.cpp: parseObfuscateElement, line 154 -->
```

- Use sites: `inject name : type;` / `inject(component) name : type;` (a `variable_decl`) and the `get` expression (section 2.9). All of `component app { single { new A() } factory("named") { new A() } }`, `inject a : A;`, `inject(app) b : A;`, `get<A>()`, `get("named", app)` parse (compiled).
- The body of `single` / `factory` is ONE expression, no `;`.
- Any token other than `single` in the list is parsed as a factory (and fails on `expect("factory")` if it is not one).
- A string element of `obfuscate` names a module (`obfuscate -keep { "std", "std.io" }`); a `utype` names a class, field or, with a parameter list, a function.
- No component, inject or `get<>` use exists in `tests/suite/cases` or `stdlib`; only `obfuscate` does (`platform/dependencies.sharp`).

### 2.6 Types

```ebnf
utype = type_identifier [ "[" "]" ] [ "?" ] [ "?" ] ;
<!-- Parser.cpp: parseUtype, line 2032 -->
utype_naked = type_identifier ;
<!-- Parser.cpp: parseUtypeNaked, line 2057 -->
type_identifier = native_type | function_ptr | reference_pointer ;
<!-- Parser.cpp: parseTypeIdentifier, line 2007 -->
native_type = "decimal" | "object" | "_int8" | "_int16" | "_int32" | "_int64"
            | "_uint8" | "_uint16" | "_uint32" | "_uint64" ;
<!-- Parser.cpp: isNativeType, line 4245 -->
function_ptr = utype_arg_list_opt "(" [ "nil" | utype ] ")" ;
<!-- Parser.cpp: parseFunctionPtr, line 1983 -->
utype_arg_list_opt = "(" [ utype_arg_opt { "," utype_arg_opt } ] ")" ;
<!-- Parser.cpp: parseUtypeArgListOpt, line 3347 -->
utype_arg_opt = identifier ":" utype | utype ;
<!-- Parser.cpp: parseUtypeArgOpt, line 3304 -->
utype_list = utype { "," utype } ;
<!-- Parser.cpp: parseUtypeList, line 4651 -->
reference_pointer = "operator" override_operator
                  | [ dot_notation_format "#" ] reference_item { dot reference_item }
                    [ dot "operator" override_operator ] ;
<!-- Parser.cpp: parseReferencePointer, line 4099 -->
dot_notation_format = identifier { dot identifier } ;
<!-- Parser.cpp: parseReferencePointer, lines 4110-4141 (ast_module_reference) -->
reference_item = identifier [ "<" utype_list ">" ] ;
<!-- Parser.cpp: parseReferencePointer, lines 4143-4210; genericCloseFollows, line 4076; consumeGenericClose, line 4086 -->
dot = "." | "?." | "!!." ;
<!-- Parser.cpp: parseReferencePointer, lines 4114-4116 and 4170-4172 -->
override_operator = assign_expr | "++" | "--" | "*" | "/" | "%" | "-" | "+" | "==" | "!="
                  | ">>" | "<<" | "<" | ">" | "<=" | ">=" | "&&" | "||" | "!" | "**"
                  | "&" | "|" | "^" | "[" "]" ;
<!-- Parser.cpp: isOverrideOperator, line 2285; expectOverrideOperator, line 4582 -->
assign_expr = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" ;
<!-- Parser.cpp: isAssignExprSymbol, line 2260 -->
```

Notes:

- Suffix order is fixed: array brackets first, then the question marks. `string[]?` is a nullable array. `string[]??` is a nullable array with nullable items (backend `parse_utype`). `string?[]` does not parse (compiled). `decimal??` parses; the backend rejects two `?` on a non-array.
- `[` is taken as the array suffix only when `]` is the very next token.
- Function types: `(decimal)(decimal)`, `()()`, `()(nil)`, `(a: decimal, b: decimal)(decimal)`, `(string[])(int)`. An empty second pair and `nil` both mean "returns nothing". The usual suffixes apply after it: `(decimal)(decimal)[]`.
- `reference_pointer`, exactly as coded: an identifier, then `dot identifier` repeated; then an optional `# identifier` (everything before the `#` becomes the module name, and `?.` / `!!.` before it is an error); then an optional generic list on the last name; then `dot identifier [generic list]` repeated. After any `dot`, the word `operator` plus an operator ends the reference. In effect every name after the module part may carry a generic list.
- Examples: `string`, `std#string`, `std.collections#first_or<int>`, `pair2<T, U>`, `holder.inner.on`, `a?.b`, `a!!.b`, `operator+`, `operator[]`.
- The reference pointer is used for types AND for names in expressions; `?.` and `!!.` are therefore accepted syntactically inside a type.
- `utype_naked` (no `[]`, no `?`) is used after `new` and for the name in a call chain.

### 2.7 Statements

```ebnf
block = "{" { statement | block } "}" | statement ;
<!-- Parser.cpp: parseBlock, line 293 -->
lambda_block = { statement | block } "}" ;
<!-- Parser.cpp: parseLambdaBlock, line 344 -->
statement = return_statement | if_statement | for_statement | alias_decl
          | [ access_types ] variable_decl
          | foreach_statement | while_statement | do_while_statement | throw_statement
          | break_statement | continue_statement | lock_statement | trycatch_statement
          | when_statement | asm_statement | ";" | expression_statement ;
<!-- Parser.cpp: parseStatement, line 1500 -->
expression_statement = expression [ ";" ] ;
<!-- Parser.cpp: parseStatement, lines 1659-1689 -->
return_statement = "return" [ expression ] ";" ;
<!-- Parser.cpp: parseReturnStatement, line 382 -->
if_statement = "if" "(" expression ")" block
               { "else" "if" "(" expression ")" block } [ "else" block ] ;
<!-- Parser.cpp: parseIfStatement, line 480 -->
for_statement = "for" "(" ( variable_decl | ";" ) [ expression ] ";" [ expression ] ")" block
              | "for" "(" ")" block
              | "for" block ;
<!-- Parser.cpp: parseForStatement, line 524 -->
foreach_statement = "foreach" "(" ( utype_arg | identifier ) "in" expression ")" block ;
<!-- Parser.cpp: parseForEachStatement, line 1461 -->
while_statement = "while" "(" expression ")" block ;
<!-- Parser.cpp: parseWhileStatement, line 578 -->
do_while_statement = "do" block "while" "(" expression ")" ";" ;
<!-- Parser.cpp: parseDoWhileStatement, line 591 -->
throw_statement = "throw" expression ";" ;
<!-- Parser.cpp: parseThrowStatement, line 605 -->
break_statement = "break" ";" ;
<!-- Parser.cpp: parseBreakStatement, line 1384 -->
continue_statement = "continue" ";" ;
<!-- Parser.cpp: parseContinueStatement, line 1392 -->
lock_statement = "lock" "(" expression ")" block ;
<!-- Parser.cpp: parseLockStatement, line 1400 -->
trycatch_statement = "try" block ( catch_clause { catch_clause } [ finally_block ] | finally_block ) ;
<!-- Parser.cpp: parseTryCatchStatement, line 1447; parseCatchChain, line 1423 -->
catch_clause = "catch" "(" utype_arg_opt ")" block ;
<!-- Parser.cpp: parseCatchClause, line 1411 -->
finally_block = "finally" block ;
<!-- Parser.cpp: parseFinallyBlock, line 1437 -->
when_statement = "when" [ "(" expression ")" ] when_block ;
<!-- Parser.cpp: parseWhenStatement, line 623 -->
when_block = "{" when_clause { when_clause } [ when_else_clause ] "}" ;
<!-- Parser.cpp: parseWhenBlock, line 1346 -->
when_clause = expression { "," expression } "->" block ;
<!-- Parser.cpp: parseWhenBlock, lines 1359-1370 -->
when_else_clause = "else" "->" block ;
<!-- Parser.cpp: parseWhenBlock, lines 1352-1358 -->
asm_statement = "asm" asm_block ;
<!-- Parser.cpp: parseAsmStatement, line 637 -->
asm_block = "{" asm_instruction { asm_instruction } "}" ;
<!-- Parser.cpp: parseAsmBlock, line 1329 -->
asm_instruction = mnemonic [ asm_operand { "," asm_operand } ] | "." identifier ":" ;
<!-- Parser.cpp: parseAssemblyInstruction, line 645; iaAssemblyInstruction, line 4370 -->
asm_operand = asm_register | asm_literal | literal ;
<!-- Parser.cpp: parseAssemblyInstruction, lines 645-1327 (which operand kinds, per mnemonic) -->
asm_register = [ "%" ] ( "adx" | "cx" | "cmt" | "ebx" | "ecx" | "ecf" | "edf" | "ehf" | "bmr" | "egx" ) ;
<!-- Parser.cpp: parseRegister, line 2950 (register names are matched case-insensitively) -->
asm_literal = [ "-" ] literal
            | "@" identifier
            | asm_member_item
            | "$" utype [ asm_member_item ] ;
<!-- Parser.cpp: parseAsmLiteral, line 2870 -->
asm_member_item = "[" identifier "]" ;
<!-- Parser.cpp: parseAsmLiteral, lines 2910-2932 -->
```

Blocks:

- `block` has two forms. With `{` it is a list of statements and nested blocks. Without `{` it is exactly one statement. The braceless form is accepted for the bodies of `if`, `else`, `for`, `foreach`, `while`, `do`, `lock`, `try`, `catch`, `finally`, `when` clauses, `get`, `set`, constructors and `init` (all compiled). Only `def` insists on `{`.
- A nested `{ ... }` at statement position is always a nested block. An expression statement can therefore not start with `{`.
- After each statement the parser checks whether the next token is `.` and reports "unexpected symbol `.`".
- `;` alone is an empty statement with the warning "unnecessary semicolon".

Statement selection is by the first token, in this order: optional access types, `return`, `if`, `for`, `alias`, variable declaration (lookahead of section 4.1), `foreach`, `while`, `do`, `throw`, `break`, `continue`, `lock`, `try`, `when`, `asm`, `;`, otherwise an expression statement. There is no `goto`, no label, no `switch`, no local `def` or `class` (`goto x;` is "not a statement", compiled).

Individual statements:

- `expression_statement`: any expression is accepted. The `;` is required except when the statement's last token is the `}` of a trailing lambda (`launch { work(); }`); then it is optional (section 4.7).
- `if`: `else if` is recognised as the two tokens together and becomes an `ast_elseif_statement` of the same `if`. After a plain `else` the chain ends. A dangling `else` goes to the nearest `if`.
- `for`: three shapes. `for (init; cond; step)`: `init` is a `variable_decl` including its own `;` (so `for (i := 0; ...)`, `for (i : decimal = 0, j = 5; ...)`), or just `;`. An assignment such as `for (i = 0; ...)` is a syntax error (compiled). No access types and no expression are possible in `init`. `cond` and `step` are optional expressions; there is no comma list in `step`. `for () block` and `for block` produce `ast_for_style_2_statement` ("for {}").
  UNCERTAIN: `for { break; }` parses, but the current compiler answers "attempt to call `break` outside of an enclosing loop" and then crashes (compiled). I did not look into what style 2 is meant to do.
- `foreach`: `foreach (x in e)` or `foreach (x: t in e)`.
- `try`: at least one `catch` or a `finally`, otherwise "expected `catch` or `finally`". The catch parameter is `name: type` or a type alone (`catch (exception)`).
- `when`: with a subject, `when (x) { 1, 2 -> ...; else -> ...; }`; without, each clause expression is a condition. At least one non-else clause is required (`when (a) { else -> ...; }` is "expected when clause", compiled). `else` must be last: parsing of clauses stops after it (compiled). The clause body is a `block`, so one statement or `{ }`.
- `asm`: at least one instruction. The list continues while the next token's text is a mnemonic or `.`; newlines mean nothing. `.name:` defines a label, `@name` refers to one, `[name]` is a member or local, `$type` a class, `$type[member]` a member of a class. The 157 mnemonics and the operand kinds each takes are the `if` chain of `parseAssemblyInstruction` (lines 645-1327) and the list in `iaAssemblyInstruction` (line 4370, misspelled "ia"); they are not repeated here. Example: `stdlib/platform/kernel/platform.sharp`, line 43.

### 2.8 Expressions: top level

The expression parser is not a clean precedence climber. These rules describe which token sequences `parseExpression` accepts; section 3 gives the grouping.

```ebnf
expression = lead_operand [ assign_expr expression ] { binary_operator right_operand }
             [ "?" expression ":" expression ]
           | non_null_operand "?:" expression
           | vector_array
           | dictionary ;
<!-- Parser.cpp: parseExpression, line 2415 -->
lead_operand = "-" prefix_operand
             | ( "++" | "--" ) prefix_operand
             | non_null_operand ;
<!-- Parser.cpp: parseExpression, lines 2420-2458 -->
prefix_operand = "-" prefix_operand
               | ( "++" | "--" ) prefix_operand
               | primary_expr
               | vector_array
               | dictionary ;
<!-- Parser.cpp: parseExpression called with ignoreBinary = true, lines 2424, 2436, 3413 -->
non_null_operand = primary_expr [ "!!" { array_expression [ "." dot_notation_call_expr ] } ] ;
<!-- Parser.cpp: parseNonNullSuffix, line 2842; unary, line 2813 -->
right_operand = non_null_operand | "-" expression ;
<!-- Parser.cpp: unary, line 2813; the MINUS branch in binaryLevel/equality/.../exponent, e.g. lines 2584-2590 -->
binary_operator = "||" | "&&" | "|" | "^" | "&" | "==" | "!=" | "<" | ">" | "<=" | ">="
                | "<<" | ">>" | "+" | "-" | "*" | "/" | "%" | "**" ;
<!-- Parser.cpp: binaryLevel, line 2562; equality 2600; comparason 2635; shift 2670; addition 2705; multiplication 2742; exponent 2777 -->
vector_array = "{" [ expression_list ] "}" ;
<!-- Parser.cpp: parseVectorArray, line 2301 -->
expression_list = expression { "," expression } ;
<!-- Parser.cpp: parseVectorArray, lines 2307-2314; parseExpressionList, lines 3014-3022 -->
value_list = "(" [ expression_list ] ")" ;
<!-- Parser.cpp: parseExpressionList, line 3004 -->
dictionary = "{" [ dict_type ] ( ":" | dict_element { "," dict_element } ) "}" { dictionary_suffix } ;
<!-- Parser.cpp: parseDictExpression, line 2342 -->
dict_type = "as" "<" utype "," utype ">" ;
<!-- Parser.cpp: parseDictionaryType, line 2320 -->
dict_element = expression ":" expression ;
<!-- Parser.cpp: parseDictElement, line 2334 -->
dictionary_suffix = "++" | "--"
                  | "[" expression "]" [ "." dot_notation_call_expr | value_list ]
                  | "." dot_notation_call_expr ;
<!-- Parser.cpp: parseDictExpression, lines 2366-2412 -->
```

What the shape of `expression` means:

- Assignment and elvis are only looked for directly after the FIRST operand of an expression. `a = b`, `a.b[i] += c`, `a ?: b` work. `a + b = c` and `a + b ?: c` stop before the `=` / `?:` and fail in the caller (compiled).
- The right side of `=`, `?:` and both arms of `? :` are full expressions (so all three group to the right).
- Elvis needs a plain `non_null_operand` on its left. It is not tried after a prefix `-`, `++`, `--`.
- A `vector_array` or `dictionary` literal is only tried when no primary expression could be parsed at the START of an expression, and the expression ends with it: `{1, 2}.size()` and `a == {1, 2}` are syntax errors (compiled). A dictionary has its own small suffix loop; a vector array has none.
- `{}` is an empty vector array (rejected later: no element type). `{ : }` and `{ as <k, v> : }` are empty dictionaries.
- After a binary operator the right operand is a `non_null_operand`: no prefix `++`/`--` (`a + ++b` is an error, compiled), no literal `{...}`, no assignment. The one prefix allowed there is `-`, and it takes the ENTIRE rest of the expression as its operand (section 5.2).
- The binary loops report "expected expression" when an operator is directly followed by another token from `isExprSymbol` (line 2268) other than `-`, unless that token is a string or character literal.

### 2.9 Expressions: primary

```ebnf
primary_expr = "!" prefix_operand
             | primary_head { primary_suffix } [ cast_suffix ] ;
<!-- Parser.cpp: parsePrimaryExpr, line 3405 -->
primary_head = literal
             | self_expr
             | base_expr
             | get_component
             | dot_notation_call_expr
             | new_expr
             | lambda_function
             | sizeof_expr
             | paren_expr
             | "null" ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3417-3758, tried in this order -->
literal = string_literal | character_literal | number_literal | hex_literal | boolean_literal ;
<!-- Parser.cpp: parseLiteral, line 2978 -->
boolean_literal = "true" | "false" ;
<!-- Parser.cpp: parseLiteral, line 2984 -->
self_expr = "self" [ "->" dot_notation_call_expr ] ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3450-3477 -->
base_expr = "base" [ "@" utype ] [ "->" dot_notation_call_expr ] ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3479-3505; parseBaseClassUtype, line 2996 -->
get_component = "get" [ "<" utype ">" ] "(" [ string_literal [ "," identifier ] | identifier ] ")" ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3508-3540 -->
dot_notation_call_expr = [ dot ] utype_naked [ value_list [ trailing_lambda ] | trailing_lambda ]
                         { call_suffix } [ dot_notation_call_expr ] ;
<!-- Parser.cpp: parseDotNotCallExpr, line 3138 -->
call_suffix = value_list
            | "++" | "--"
            | array_expression [ "." dot_notation_call_expr | value_list ] ;
<!-- Parser.cpp: parseDotNotCallExpr, lines 3173-3212 -->
array_expression = "[" array_items "]" ;
<!-- Parser.cpp: parseArrayItems, line 4678 (ast_arry_e / ast_array_index_items) -->
array_items = expression { "," expression } ;
<!-- Parser.cpp: parseArrayItems, lines 4682-4687 -->
new_expr = "new" utype_naked ( new_array_size | "[" "]" vector_array | value_list ) ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3601-3636 -->
new_array_size = "[" expression "]" ;
<!-- Parser.cpp: parseArrayExpression, line 3237 -->
lambda_function = "{" [ lambda_arg_list ] "->" [ lambda_return_type ] lambda_block ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3640-3693 -->
lambda_arg_list = lambda_arg { "," lambda_arg } ;
<!-- Parser.cpp: parseLambdaArgList, line 3391 -->
lambda_arg = identifier [ ":" utype ] ;
<!-- Parser.cpp: parseLambdaArg, line 3287 -->
lambda_return_type = "(" ( "nil" | utype ) ")" ;
<!-- Parser.cpp: parseLambdaReturnType, line 3331 -->
trailing_lambda = "{" [ lambda_arg_list ] "->" [ lambda_return_type ] lambda_block
                | "{" expression "}"
                | "{" lambda_block ;
<!-- Parser.cpp: parseTrailingLambda, line 3072; trailingLambdaFollows, line 3038 -->
sizeof_expr = "sizeof" "(" expression ")" ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3695-3718 -->
paren_expr = "(" expression ")" ;
<!-- Parser.cpp: parsePrimaryExpr, lines 3720-3744 -->
primary_suffix = "++" | "--"
               | "." dot_notation_call_expr
               | array_expression ;
<!-- Parser.cpp: parsePrimaryExpr, labels checkInc 3842, dotNot 3571, arrayExpr 3760 -->
cast_suffix = ( "as" | "is" ) utype ;
<!-- Parser.cpp: parsePrimaryExpr, labels asExpr 3803, isExpr 3822 -->
```

`dot_notation_call_expr` is the workhorse: a name, a call, a member chain.

- The name is a `utype_naked`: normally a `reference_pointer`, so the whole dotted path `a.b.c`, with `?.`, `!!.`, generic arguments and `module#name`, is consumed as ONE reference before any call is looked at. Because it is a type, a native type name (`_int8`) and even a function type (`(decimal)(decimal)`) are also accepted as a primary expression (compiled; rejected later as "must evaluate to a value").
- Then, optionally, the call: `name(args)`, `name(args) { lambda }`, or `name { lambda }`.
- Then the suffix loop: another call `f()()`, `++`/`--`, an index `[i]` (which may be followed by one `.chain` or by a call of the element, `fns[0](4)`), repeated.
- Then, optionally, a continuation `.name...` / `?.name...` / `!!.name...`, which is a nested `dot_notation_call_expr` and ends the current one.
- The leading `dot` of the rule is what a continuation starts with. Nothing stops it from appearing at the very start of an expression (section 5.1).
- A trailing lambda is only possible directly after the name or after the first argument list. `mk()() { }` and `a[0](x) { }` do not take one (compiled).
- An index takes a comma list (`c[0, 1]` parses; the backend says "Multi-dimensional arrays are not supported", compiled).

Which suffixes may follow which head (anything else in that position is either reported as "unexpected symbol" or left for the caller):

| Head | `++` `--` | `.` chain | `[index]` | `as` / `is` |
|---|---|---|---|---|
| string literal | yes | yes (`"abc".size()`) | error | yes |
| other literal | yes (`5++` parses) | error | error | yes |
| `self`, `base`, `base@t` (without `->`) | yes | yes | yes | yes |
| `self->...`, `base->...` | part of the chain | part of the chain | part of the chain | no for `self->` (returns at once), yes for `base->` |
| `dot_notation_call_expr` | yes | yes | yes | yes |
| `new ...` | yes | yes | error | yes |
| `sizeof(...)` | yes | error | error | yes |
| `( ... )` | yes | yes | yes | yes |
| `get(...)`, lambda, `null` | no | no | no | no |

- After a `primary_suffix` any other may follow, with one exception: a bare `[index]` reached through the `arrayExpr` label (that is, an index directly on `self`, `base` or `( )`) is not followed by `as` / `is` unless a `.chain` comes in between.
- `as` and `is` end the primary. They do not chain: `a as x as y` is an error (compiled); write `(a as x) as y`. Nothing may follow them except a binary operator.
- `!!` is not a `primary_suffix`. It is applied one level up (`non_null_operand`), so `s!! as string` is an error (compiled); `s!!.name` uses the single token `!!.` and is part of the reference.
- `new`: `new t(args)`, `new t[size]`, `new t[] { a, b }`. No trailing lambda, no `new t { field = value }`.
- Lambda: `{ -> body }`, `{ a, b -> body }`, `{ a: decimal -> (decimal) return a + 1; }`. The return type, when given, is written in parentheses right after the arrow. The body is statements up to the closing `}`.
- Trailing lambda: the three forms of the rule, tried in that order: explicit (with an arrow); implicit single expression with no `;` before the `}` (it becomes `return expr;` with the marker token `@implicit`); implicit statement list. An implicit lambda takes no parameter, or one called `it` if the body uses that name (decided by the compiler).
- `get`: `get<t>()`, `get<t>("name")`, `get("name", component)`, `get(component)`.
- `base@type->f()` selects which base. It parses; no example exists in the tests or library.
- `!`: the operand is a `prefix_operand`, so `!a.b()` is `!(a.b())`, `!a as t` is `!(a as t)`, and `!a == b` is `(!a) == b`.

---

## 3. Precedence and associativity

Lowest first. "Lead only" means the operator is recognised only directly after the first operand of an expression (it is not part of the binary chain).

| # | Operators | Assoc. | Parsed by | Notes |
|---|---|---|---|---|
| 1 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` | right | `parseExpression`, label `assignExpr` (2497) | Lead only. Left side: a `lead_operand`. Right side: a full expression. |
| 2 | `?:` (elvis) | right | `parseExpression` (2489) | Lead only. Left side: a `non_null_operand`. Right side: a full expression, so `a ?: b + c` is `a ?: (b + c)`. |
| 3 | `? :` (ternary) | right | `parseExpression` (2522) | Condition: everything parsed so far (the binary chain). Both arms: full expressions. `a = b ? c : d` is `a = (b ? c : d)`. |
| 4 | `\|\|` | left | `binaryLevel` level 0 (2562) | |
| 5 | `&&` | left | `binaryLevel` level 1 | |
| 6 | `\|` | left | `binaryLevel` level 2 | |
| 7 | `^` | left | `binaryLevel` level 3 | |
| 8 | `&` | left | `binaryLevel` level 4 | |
| 9 | `==` `!=` | left | `equality` (2600) | |
| 10 | `<` `>` `<=` `>=` | left | `comparason` (2635) | |
| 11 | `<<` `>>` | left | `shift` (2670) | |
| 12 | `+` `-` | left | `addition` (2705) | |
| 13 | `*` `/` `%` | left | `multiplication` (2742) | |
| 14 | `**` | LEFT | `exponent` (2777) | `2 ** 3 ** 2` is 64 (ran). |
| 15 | prefix `-` `++` `--` | right | `parseExpression` head (2420, 2431) | Only at the start of an expression. Operand is a `prefix_operand`. Binds tighter than every binary operator: `-a ** 2` is `(-a) ** 2` (ran). After a binary operator `-` behaves differently, see 5.2. |
| 16 | `!` | right | `parsePrimaryExpr` (3409) | Allowed wherever a primary is. Operand is a `prefix_operand`. |
| 17 | postfix `!!`, then `[i]` and `.chain` on the result | left | `parseNonNullSuffix` (2842), from `parseExpression` and `unary` | Applies to the whole primary before it, including a leading `!`: `!a!!` is `(!a)!!`. |
| 18 | `as` `is` | none | `parsePrimaryExpr`, labels `asExpr` (3803), `isExpr` (3822) | Postfix on a primary; not chainable. `a + b as t` is `a + (b as t)`. |
| 19 | postfix `++` `--`; index `[ ]`; call `( )`; trailing lambda `{ }`; `.` `?.` `!!.`; `->` after `self` / `base`; generic arguments `< >`; `#` | left | `parsePrimaryExpr` labels, `parseDotNotCallExpr` (3138), `parseReferencePointer` (4099) | Dotted names are consumed whole by `parseReferencePointer` before any call or index. |
| 20 | literals, `self`, `base`, `null`, names, `new`, `sizeof( )`, `( )`, lambda `{ a -> }`, `get( )` | | `parsePrimaryExpr` (3405) | |
| - | `{ a, b }` array literal, `{ k : v }` dictionary literal | | `parseExpression` (2460), `parseVectorArray`, `parseDictExpression` | Only as a whole expression (or the operand of a lead prefix). Never an operand of a binary operator. |

How the binary chain actually runs: `parseExpression` parses the first operand itself and then calls `binary()`. `binary()` descends to `unary()`, which fails immediately because the next token is an operator; each level's `while` loop then picks the already-parsed operand out of the AST (`ast->sub_asts.last()`) as its left side. The result is ordinary precedence climbing. Operands evaluate right to left at run time; that is a property of the code generator, not of this grammar.

Unary plus does not exist. `!!x` (double negation) does not parse because `!!` is one token (compiled); write `!(!x)`.

---

## 4. How the parser decides

### 4.1 Declaration or expression statement

`parse` (104), `parseStatement` (1542), `parseClassBlock` (3990) use the same test, on up to three tokens, no backtracking:

- `identifier` followed by `:` or `:=`, or
- `localized` `identifier` followed by `:` or `:=`, or
- `inject`, or `localized` `inject`.

`identifier` here is `isVariableDecl`: an identifier token that is not a keyword. Everything else that reaches the end of `parseStatement` is an expression statement. In a class block the next candidates are tried instead; a constructor is `identifier` `(` (`isConstructorDecl`, 4365).

### 4.2 `{` at the start of a statement

`parseBlock` (314) and `parseLambdaBlock` (357) test for `{` before calling `parseStatement`: it is always a nested block.

### 4.3 `{` at the start of an expression: lambda, array literal or dictionary

1. `parsePrimaryExpr` (3640) looks one token past the `{`. `->` means a lambda with no parameters.
2. Otherwise it parses a `lambda_arg_list` speculatively (protected mode, position saved in `old`). If `->` follows, it is a lambda. If not, the position is restored and the errors are dropped.
3. `parsePrimaryExpr` then fails, and `parseExpression` (2460) takes over. One token past the `{`: `:` or `as` means a dictionary.
4. Otherwise it parses ONE expression speculatively. If `:` follows, it is a dictionary; if not, a vector array. In both cases the position is restored and the literal is parsed again from the `{`.

Consequences: the first element of every array or dictionary literal is parsed twice (nested literals: twice per level). `{ a ? b : c }` is a one-element array, because the speculative expression eats the ternary's colon. `{ a : b -> ... }` is a lambda, `{ a : b }` a dictionary.

### 4.4 Generic `<` or less-than

`parseReferencePointer` (4143, 4184): after a name, on `<`, it parses a `utype_list` speculatively and then asks `genericCloseFollows` (4076) whether a closing `>` comes next. Yes: restore and parse the generic list for real. No: restore; the `<` is left for the binary operator code.

- So `name < types >` is a generic argument list whenever it CAN be one. `g(a < b, c > d)` is a syntax error (compiled) and `a < b > (c)` is a call of `a<b>` (compiled). `a < b && c > d` is fine because `&&` follows the "type" `b`.
- `>>` closes two lists: the inner list takes the first half and leaves the token in place (`halfConsumedShr`), the outer list consumes it (`consumeGenericClose`, 4086). A `>>` is accepted as a close only when the list is nested inside another generic list of a reference pointer (`genericDepth > 0`) or the token is already half used. `x < y >> 2` stays a comparison.
- The `<` `>` of `get<t>` and of `dict_type` are plain `expect` calls and do not count as depth: `{ as <string, list<decimal>> : }` fails with "expected `>`" (compiled), while `get<list<list<decimal>>>()` parses, because there the inner two lists pair up and the last `>` is a separate token (compiled).
- Declarations: after `class name` / `interface name` a `<` always starts the parameter list. After `def`, `genericFunctionDeclFollows` (225) scans ahead up to 64 tokens: `identifier` `<` ... `>` `(`, giving up at the first `(`, `{`, `;` or end of file.

### 4.5 `(`: function type or parenthesised expression (and casts)

There is no C-style cast. The cast is `expr as type`; the test is `expr is type`.

Every primary that starts with `(` is first tried as a NAME, because `dot_notation_call_expr` is tried before `paren_expr` and its name is a type. `parseTypeIdentifier` (2007) runs `parseFunctionPtr` (1983) speculatively: a parenthesised list of types that parses AND is directly followed by another `(` is a function type.

- `(a + b)`, `(a)`, `(a).b`: the type list fails or no `(` follows, the attempt is undone, and `paren_expr` is used.
- `(f)(x)`: taken as the function TYPE `(f)(x)`. `(f)(3)` is the error "expected identifier" (compiled): once the second `(` is seen, `parseFunctionPtr` returns true whatever happens inside it, and `parseTypeIdentifier` then reports the collected errors. A parenthesised expression cannot be called. Assign it to a variable first.
- In a type position (`x : (decimal)(decimal)`, `as ()()`) the same code is what makes function types work.

### 4.6 Calls, indexes and chains: `a[i](x)`, `f()()`, `a.b().c`

All inside `parseDotNotCallExpr`, by one token of lookahead in its loop (3173):

- `(`: a further call; a new `ast_dot_fn_e` node.
- `[`: an index (`ast_arry_e`). After it, `.` starts a nested chain attached to the index node (speculative: undone if no name follows), and `(` makes a separate call node, so `fns[0](4)` indexes first and calls the element.
- `++` / `--`: postfix node.
- `.`, `?.`, `!!.`: a nested `dot_notation_call_expr` (speculative), then the function returns.

`a.b.c(x)` is not a chain of three accesses: `a.b.c` is one `reference_pointer` and the call applies to it.

### 4.7 `{` after a name or a call: trailing lambda

`parseDotNotCallExpr` (3154, 3159): a `{` directly after the argument list, or directly after the name when `trailingLambdaFollows` (3038: the previous token is an identifier token or `>`), is a trailing lambda. There is no further check and no backtracking on that decision.

Inside (`parseTrailingLambda`, 3072), up to three attempts: `->` right after the `{`; else a speculative `lambda_arg_list` followed by `->`; else a speculative single expression followed by `}`; else a statement list.

`parseStatement` (1683) remembers the `}` of the last trailing lambda in `trailingLambdaEnd`. If the expression statement ends exactly on that token and no `;` follows, none is required.

Trap: a call followed by a block on the next line is one statement. `f(1)` newline `{ ... }` passes the block to `f` (compiled: "could not resolve symbol `f(_int32, @anonymous_fun...)`").

### 4.8 `new t[`

`parseArrayExpression` (3237), speculative: `[` expression `]` is an array size. `[` `]` followed by `{` is undone and parsed as `[` `]` `vector_array`. `[` `]` followed by anything else is the error "expected expression after '['".

### 4.9 Smaller decisions (plain lookahead, no backtracking unless said)

| Where | Looks at | Decides |
|---|---|---|
| `parseIfStatement` (492) | `else` then `if` | else-if clause or final else |
| `parseForStatement` (531-540) | `(` present; then `)`; then `;` | the three `for` shapes; empty init |
| `parseForEachStatement` (1468) | second token is `:` | typed or untyped loop variable |
| `parseUtypeArgOpt` (3307), `parseLambdaArg` (3290) | `identifier` `:` | named or unnamed parameter; typed or untyped lambda parameter |
| `parseMethodDecl` (254) | token after the name is not `(`, or the name is `operator` | operator / extension declaration |
| `parseMethodDecl` (263-290) | `:`, `=`, `:=`, `;`, `{` | which body form |
| `parseWhenStatement` (628), `parseWhenBlock` (1352) | `(`; `else` | subject or not; else clause |
| `parseVariableDecl` (2164-2194) | speculative: skips access types, then `get` / `set` | whether accessors follow; restores the position if not |
| `parseUtype` (2037) | `[` then `]` | array suffix (otherwise the `[` belongs to the caller, e.g. `new t[5]`) |
| `parseObfuscateDecl` (143) | `-` then an option word | option present |
| `parseExpression` (2445 and others) | `isExprSymbol(peek(1))` | whether the expression continues after an operand |
| `parsePrimaryExpr` (3419) | speculative `parseLiteral` | literal or not |
| `parseBlock` (297) | `{` | braced block or single statement |

---

## 5. Oddities and traps

### 5.1 Things that parse and should not

- Stray braces in class and interface bodies are counted, not rejected: `class A { { } v : decimal; }` compiles (`parseClassBlock`, 4021-4040; `parseInterfaceBlock`, 1786-1805).
- An expression may begin with a dot: `x := .a;` and `c := ?.a;` compile (the optional leading `dot` of `parseDotNotCallExpr`).
- A primary may be an index with nothing in front: `x := [1, 2];` parses (fall-through into the `arrayExpr` label; rejected later). Likewise `x := as _int8;` and `is t` parse with an empty operand (fall-through into `asExpr` / `isExpr`; compiled).
- Postfix `++` on anything: `5++`, `true++`, `sizeof(a)++`, `a++ ++` all parse (compiled).
- A native type or a function type as a value: `x := _int8;` (section 2.9).
- `-{ 1, 2 }` parses (compiled).
- `c[0, 1]` parses (section 2.9). `decimal??` parses (section 2.6).
- All eleven access types in front of anything, in any order (section 2.1). Access types in front of `alias`, `component`, `obfuscate` vanish without a message.
- `for (i; ...)` declares a variable (section 2.4).
- `mod` twice, `import` after declarations: accepted by the parser.
- Hex literals with `g`/`h`, and any one character after `0x` (section 1.3).
- `?.` and `!!.` inside type names.

### 5.2 Unary minus after a binary operator swallows the rest

In every binary level (`binaryLevel`, `equality`, `comparason`, `shift`, `addition`, `multiplication`, `exponent`), when the token after the operator is `-`, the code calls the full `parseExpression` for what follows the minus and returns. So the negation applies to everything to its right:

- `a * -b + c` is `a * -(b + c)`: with 2, 3, 4 it gives -14, not -2 (ran).
- `a == -1 || c` is `a == -(1 || c)`: with a = 1, c = 3 the `if` took the else branch (ran).
- `a + -b = 5` is `a + -(b = 5)`: it assigned 5 to `b` (ran).

A minus at the very start of an expression does not do this (`-a + b` is `(-a) + b`).

### 5.3 Prefix operators have narrow reach

- `-`, `++`, `--` are only recognised at the start of an expression (and `-` after a binary operator, 5.2). `a + ++b` is an error (compiled); `a + (++b)` works.
- A prefix operand that is itself prefixed, or a prefix expression followed by `!!` or `?:`, fails with "expected expression": `!-a + 1`, `-a!!` (both compiled). The inner call runs with `ignoreBinary`, jumps to `assignExpr`, skips `binary()` and reports failure.
- The ternary test at line 2522 is not guarded by `ignoreBinary`, so in that same nested situation a `?` is taken by the INNER call: `!-a ? b : c` would be `!(-a ? b : c)`. Derived from the code, not compiled.

### 5.4 Speculative parsing leaks

- `errors->fail()` writes the collected errors straight to the real list even when an outer speculative section is still open and later calls `pass()`. Errors committed by an inner section survive an outer rollback. Combined with the error filter (`shouldReport`: no two errors on the same position, none on the line after the last one, none whose text is contained in the last one), what gets reported after a failed speculation is hard to predict.
- When the speculative `.chain` after a primary fails (label `dotNot`, 3571), control does not return: it falls through into the tests below it (`new`, `{`, `sizeof`, `(`, `null`, `arrayExpr`, ...), with the head already parsed. `parsePrimaryExpr` is one function with eight labels and `goto` between them; a faithful port must keep that fall-through or decide deliberately to drop it.
- `parseExpression` has a third parameter `ignoreInlineLambda`. The code that used it is commented out (3542-3569); the flag does nothing.
- `parseExpression` (2444) tests `branch->hasSubAst(ast_get_component)`. `hasSubAst` is not recursive and the node is one level deeper, so the test is always false.
- `thread_local bool inIf` (479) is set and cleared by `parseIfStatement` and read nowhere.

### 5.5 `get` / `set` after any variable declaration

After the `;` of every `variable_decl`, also a local one and the one inside `for (...)`, the parser looks for `[access_types] get` or `set`. A statement that starts with the `get` expression right after a local declaration is therefore read as a getter: `x := 5;` newline `get { ... }` gives "local field `x` cannot have getter" (compiled).

The access-type list is not cleared between a getter and the setter that follows it unless the getter's block contained a statement, so a setter after `private get { }` can inherit `private`.

### 5.6 Parser state that changes what is accepted

| State | Set by | Effect |
|---|---|---|
| `access_types` (list) | `parseAccessTypes`; cleared at many points | Attached to the next declaration that calls `addAccessTypes`. |
| `recursion` | `parseVariableDecl`; forced to 0 by `RETAIN_RECURSION` around getter, setter, lambda bodies and `for` | `recursion == 1` means "first variable of the declaration": only then are access types, `localized`, `: type`, `:=`, the `;` and the accessors parsed. |
| `genericDepth`, `halfConsumedShr` | `parseReferencePointer`, `consumeGenericClose` | Whether `>>` may close generic lists (4.4). |
| `trailingLambdaEnd` | `parseTrailingLambda` | Whether an expression statement may omit `;` (4.7). |
| `panic` | error-limit check `CHECK_ERRLMT` | Stops parsing when the error count passes `options.max_errors`. |
| tokenizer `dynamicString`, `brackets` | `parseString` | Inside `${ }` (1.5). |

### 5.7 Token-level traps

- `1e2-1` is one number (1.3). `"...\n...$5"` loses its `$` (1.5). `"n=$n".size()` calls `size` on `n` (1.5).
- `x : list<int>= v` fails on `>=` (1.6).
- `?:` must be written without a space; `? :` is an incomplete ternary.
- Keywords cannot be member names: no method may be called `get`, `set`, `in`, `is`, `as`, `base`, `init`, `local`, `single`, `factory`, `alias`, `mod`, `lock`, `when`, `stable`, `ext`, `native`, `inject`, `component`.
- `self.x` and `self->x` both parse; so do `base.x` and `base->x`. After `self->` the parser returns immediately, so `self->x as t` leaves the `as` unparsed, while `self.x as t` works.

### 5.8 Parses, then rejected or broken later (not parser errors)

- A second `mod`; a `;`-form `def` outside a class; `{}`; `c[0, 1]`; `decimal??`; a misordered or disallowed access type; a `get` block on a local; a name used as a value that is a type.
- `obfuscate -keep { A, "main", A.v }` inside a class body parses and then crashes the compiler (segmentation fault, compiled). At the top level the same form is used by the library and works.
- `for { break; }` (section 2.7) and the dotted `def a.b()` at the top level (section 2.3).

### 5.9 Error recovery that accepts extra input

- `parseAll` (1828) is the catch-all used after "expected class, enum, or import declaration" at the top level, "expected method, class, or variable declaration" in a class and "expected delegate declaration" in an interface. It parses ANY declaration and, failing that, any statement, into the tree. The file already has an error by then, so nothing new is accepted without a message, but the tree contains nodes in places the grammar above does not allow (statements at the top level, `init` at the top level, `get`/`set` blocks alone).
- In class and interface blocks, `mod`, `import`, a stray `get`/`set`, and (interfaces) variables, `init`, constructors, `mutate`, components are reported AND parsed.
- `expect` never skips: on a mismatch it has still advanced one token. `expectIdentifier` likewise. Recovery therefore often resynchronises by accident one token later.
- `parseBlock` without braces reports a stray `}` and gives it back.
- `parseMethodDecl` reports "expected `{`" and then still parses a single statement as the body.
- A failed expression statement is "not a statement"; the parser advances one token and continues.
- The `step` part of `for` is skipped when the next token is `)` or `:`; the `:` is a leftover of an older syntax.

### 5.10 Dead code

`parseLabelDecl` (1482) and `parseGotoStatement` (614) are never called; a commented-out block in `parseStatement` (1546-1563) shows where labels were tried. `isGotoStatement`, `isSwitchStatement`, `isSwitchDeclarator` have no callers; `isStaticDecl` is declared and not defined. `ast_label_decl`, `ast_goto_statement` remain in `Ast.h`.

---

## 6. Coverage check

### 6.1 Parse functions and the rule that covers each

| Function (line) | Rule |
|---|---|
| `parse` (40) | `compilation_unit`, `top_level_decl` |
| `parseObfuscateDecl` (140), `isObfuscationOption` (134) | `obfuscate_decl`, `obfuscate_option` |
| `parseObfuscateElement` (154) | `obfuscate_element` |
| `parseObfuscateBlock` (169) | `obfuscate_block` |
| `parseInterfaceDecl` (190) | `interface_decl` |
| `genericFunctionDeclFollows` (225) | `method_name`, section 4.4 |
| `parseMethodDecl` (241) | `method_decl`, `method_name`, `method_body`, `delegate_decl`, `operator_decl` |
| `parseBlock` (293) | `block` |
| `parseLambdaBlock` (344) | `lambda_block` |
| `parseReturnStatement` (382) | `return_statement` |
| `parseAliasDeclaration` (393) | `alias_decl` |
| `parseComponentTypeList` (408) | `component_type_list` |
| `parseTypeDefinition` (420) | `component_type` |
| `parseSingleTypeDefinition` (427) | `single_type` |
| `parseFactoryTypeDefinition` (440) | `factory_type` |
| `parseComponentName` (453) | `component_name` |
| `parseComponentDeclaration` (468) | `component_decl` |
| `parseIfStatement` (480) | `if_statement` |
| `parseForStatement` (524) | `for_statement` |
| `parseWhileStatement` (578) | `while_statement` |
| `parseDoWhileStatement` (591) | `do_while_statement` |
| `parseThrowStatement` (605) | `throw_statement` |
| `parseGotoStatement` (614) | none: never called (5.10) |
| `parseWhenStatement` (623) | `when_statement` |
| `parseAsmStatement` (637) | `asm_statement` |
| `parseAssemblyInstruction` (645) | `asm_instruction`, `asm_operand` (mnemonic table not transcribed) |
| `parseAsmBlock` (1329) | `asm_block` |
| `parseWhenBlock` (1346) | `when_block`, `when_clause`, `when_else_clause` |
| `parseBreakStatement` (1384) | `break_statement` |
| `parseContinueStatement` (1392) | `continue_statement` |
| `parseLockStatement` (1400) | `lock_statement` |
| `parseCatchClause` (1411) | `catch_clause` |
| `parseCatchChain` (1423) | `trycatch_statement` |
| `parseFinallyBlock` (1437) | `finally_block` |
| `parseTryCatchStatement` (1447) | `trycatch_statement` |
| `parseForEachStatement` (1461) | `foreach_statement` |
| `parseLabelDecl` (1482) | none: never called (5.10) |
| `parseStatement` (1500) | `statement`, `expression_statement` |
| `parseInterfaceBlock` (1694) | `interface_block`, `interface_member` |
| `parseAll` (1828) | none: error recovery only (5.9) |
| `parseEnumDecl` (1902) | `enum_decl` |
| `parseInitDecl` (1912) | `init_decl` |
| `parseEnumBlock` (1929) | `enum_identifier_list` |
| `parseEnumIdentifier` (1946) | `enum_identifier` |
| `parseBaseClassConstructor` (1959) | `base_class_constructor` |
| `parseConstructor` (1966) | `constructor_decl` |
| `parseFunctionPtr` (1983) | `function_ptr` |
| `parseTypeIdentifier` (2007) | `type_identifier`, `native_type` |
| `parseUtype` (2032) | `utype` |
| `parseUtypeNaked` (2057) | `utype_naked` |
| `parseGetter` (2070) | `getter` |
| `parseSetter` (2079) | `setter` |
| `parseInjectRequest` (2090) | `inject_request` |
| `parseGetterSetterAccessTypes` (2101) | `getter_setter` (lookahead) |
| `parseVariableDecl` (2111) | `variable_decl`, `variable_first`, `variable_next`, `storage_locality`, `infer_expr` |
| `parseVectorArray` (2301) | `vector_array` |
| `parseDictionaryType` (2320) | `dict_type` |
| `parseDictElement` (2334) | `dict_element` |
| `parseDictExpression` (2342) | `dictionary`, `dictionary_suffix` |
| `parseExpression` (2415) | `expression`, `lead_operand`, `prefix_operand` |
| `binary` (2541), `binaryLevel` (2562), `equality` (2600), `comparason` (2635), `shift` (2670), `addition` (2705), `multiplication` (2742), `exponent` (2777) | `binary_operator`, `right_operand`, section 3 |
| `unary` (2813) | `right_operand`, `non_null_operand` |
| `parseNonNullSuffix` (2842) | `non_null_operand` |
| `parseAsmLiteral` (2870) | `asm_literal`, `asm_member_item` |
| `parseRegister` (2950) | `asm_register` |
| `parseLiteral` (2978) | `literal`, `boolean_literal` |
| `parseBaseClassUtype` (2996) | `base_expr` |
| `parseExpressionList` (3004) | `value_list`, `expression_list` |
| `trailingLambdaFollows` (3038), `parseTrailingLambda` (3072) | `trailing_lambda` |
| `parseDotNotCallExpr` (3138) | `dot_notation_call_expr`, `call_suffix` |
| `parseArrayExpression` (3237) | `new_array_size` |
| `parseUtypeArg` (3272) | `utype_arg` |
| `parseLambdaArg` (3287) | `lambda_arg` |
| `parseUtypeArgOpt` (3304) | `utype_arg_opt` |
| `parseMethodReturnType` (3317) | `method_return_type` |
| `parseLambdaReturnType` (3331) | `lambda_return_type` |
| `parseUtypeArgListOpt` (3347) | `utype_arg_list_opt` |
| `parseUtypeArgList` (3370) | `utype_arg_list` |
| `parseLambdaArgList` (3391) | `lambda_arg_list` |
| `parsePrimaryExpr` (3405) | `primary_expr`, `primary_head`, `primary_suffix`, `cast_suffix`, `self_expr`, `base_expr`, `get_component`, `new_expr`, `lambda_function`, `sizeof_expr`, `paren_expr` |
| `parseMutateDecl` (3865) | `mutate_decl` |
| `parseClassDecl` (3892) | `class_decl` |
| `parseClassBlock` (3928) | `class_block`, `class_member` |
| `addAccessTypes` (4055), `parseAccessTypes` (4559), `isAccessDecl` (4222) | `access_types`, `access_type` |
| `genericCloseFollows` (4076), `consumeGenericClose` (4086) | `reference_item`, section 4.4 |
| `parseReferencePointer` (4099) | `reference_pointer`, `reference_identifier`, `dot_notation_format`, `reference_item`, `dot` |
| `expectIdentifier` (4567) | `identifier` |
| `expectOverrideOperator` (4582), `isOverrideOperator` (2285) | `override_operator` |
| `isAssignExprSymbol` (2260) | `assign_expr` |
| `parseModuleDecl` (4602) | `module_decl` |
| `parseGenericIdentifierList` (4616) | `generic_identifier_list`, `generic_identifier` |
| `parseReferencePointerList` (4640) | `reference_identifier_list` |
| `parseUtypeList` (4651) | `utype_list` |
| `parseImportItem` (4662) | `import_item` |
| `parseArrayItems` (4678) | `array_expression`, `array_items` |
| `parseImportDecl` (4692) | `import_decl` |
| `exportLines`, `importData`, `exportData`, `getBranch`, `expect`, `match`, `peek`, `free`, `astAt`, the `is...` predicates | helpers, no rule |

Naming note: the draft's `array-expression` is the index suffix `[items]`, and that is what `array_expression` means here (AST: `ast_arry_e`). The C++ function called `parseArrayExpression` parses something else, the `[size]` after `new t`; it is `new_array_size` here.

### 6.2 Not determined with certainty

1. `UNCERTAIN:` extension functions `def type.name(...)`. The parser builds `ast_operator_decl` for them and the backend has `process_extension_class`, but my top-level test produced "internal error: unknown ast type: 16" and I found no working example. Checked: `parseMethodDecl`, grep of tests and library, one compile. Not checked: the same declaration inside a class or a `mutate` block.
2. `UNCERTAIN:` `for block` / `for () block` (`ast_for_style_2_statement`). Parses; `for { break; }` then failed and crashed the compiler. Intended meaning not established.
3. `UNCERTAIN:` `base@type`. Read from the code (`parseBaseClassUtype`) and the backend reference (`base_expression.cpp`); not compiled, no example exists.
4. `UNCERTAIN:` the `asm` operand forms `@label + n`, `[name + n]`. The code that reads the number tests a stale token pointer (`t`, lines 2898 and 2920) and looks as if it always reports "expected literal of type (integer, hex)". Not compiled. The rule above therefore lists the forms without an offset.
5. `UNCERTAIN:` `!-a ? b : c` grouping (5.3) and the leak of errors out of nested speculative sections (5.4): derived by reading, not demonstrated by a test.
6. `UNCERTAIN:` component / inject / `get` forms were compiled once each (they parse) but have no test in the suite; their later meaning was not examined.
7. `UNCERTAIN:` the per-declaration access-type table (2.1) is read from the backend call sites of `parse_access_flags`; only the class row was confirmed by compiling. `unstable` and `global` exist as backend flags and cannot be written in source.
8. The exact error messages produced after a failed speculation depend on `ErrorManager::shouldReport`, which filters by position and by the previous message. Which inputs are ACCEPTED does not depend on it; which message a rejected input gets sometimes does.
9. Tokenizer: behaviour on non-ASCII bytes depends on the C locale of `isalpha`; not tested.

---

## 7. Compared with doc/grammars.txt

The draft's names are kept above in snake_case. This section lists where the draft and the C++ parser agree and differ. The parser wins everywhere.

### 7.1 Still right as written

- `enum-identifier`, `enum-identifier-list`.
- `Utype-list`, `generic-identifier-list` (`identifier [base utype]`).
- `typeidentifier` (native type, reference, or function pointer) and `utype-naked`.
- `uTypeArg` (`identifier : utype`), `uTypeArgOpt` (`[identifier :] utype`), and the two list rules (apart from the parentheses being required, not optional).
- `value-list` (parenthesised) and `expression-list` (bare).
- `lambda-return-type`: `( utype )`, now also `( nil )`.
- `single-type`, `factory-type`, `component-type`, `component-type-list`, `inject_request`, component declaration (without the final `;`).
- Getter (`get block`) and setter (`set identifier -> block`).
- Module declaration; mutate declaration; class and generic class declarations.
- Constructor declaration (plus the new `-> base(...)`); init declaration (same addition).
- While, lock, do-while, throw statements; `catchClause`, `finallyBlock`.
- `self`, `self -> dot-notation-call-expr`, `base -> dot-notation-call-expr`, `sizeof ( expression )`, `null`, `( expression )` as primaries.
- The empty dictionary `{ [dict-type] : }`.
- Boolean literal; the single-line comment.

### 7.2 Changed (old form -> current form)

- `nativetype`: `var`, `object` -> `decimal`, `object`, `_int8` ... `_uint64`. `var` is now a library name.
- `identifier`: "first character is a letter" -> letter or `_`, with at least one letter somewhere.
- `number-literal`: digits, underscores, one fraction -> also exponents and hex (1.3).
- `string-literal`: `{name}` interpolation -> `$name`, `${expr}`, `$$`; more escapes.
- `character-literal`: `'\<letter>'` -> also `\0`, `\\`, `\'`, `\"`.
- `storagelocality`: `local`, `thread_local` -> `localized` only. `local` is now an access type.
- `accesstype`: protected, private, public, static, const -> those plus `local`, `ext`, `stable`, `native`, `excuse`, `override`.
- `refrence-pointer`: `[module#]name<...>(.name<...>)` with generic arguments being reference identifiers -> generic arguments are a `utype-list`; the dots may be `?.` or `!!.`; `operator <op>` may end it.
- `function-ptr`: `( Utype-list ) ( [utype] )` -> the first list is a `uTypeArg-list-opt` (names allowed) and the result may be `nil`.
- `uType`: `typeidentifier [[]] [?]` -> up to two `?` (`t[]??`).
- `VectorArray`: at least one expression -> may be empty; no trailing comma (the draft's example has one).
- `assign-expr`: listed `>>=`, `<<=` as "not added" -> still not added.
- `override-operator`: assignment operators, `* / % + - ++ -- ==` -> also `!= >> << < > <= >= && || ! ** & | ^ []`.
- `array-expression`: `expression [ items ] [dot-notation-call-expr] expression` -> the suffix `[ items ]`, optionally followed by a chain or a call.
- `dict-type`: `as ( utype , utype )` -> `as < utype , utype >`.
- `dot-notation-call-expr`: `[.] utype-naked [value-list [chain | array-expression]]` -> the leading dot may also be `?.` / `!!.`; a trailing lambda may follow or replace the value list; then a loop of calls, `++`/`--`, indexes, and a chain.
- Lambda: `{ [name [: utype]]* -> <lambda-return-type> block }` -> same, parameters separated by commas, return type optional.
- Dictionary: elements `[expr : expr]*` -> comma separated, at least one, or `:` alone.
- `new`: `new uType [value-list | VectorArray]` ("TODO: add array creation") -> `new utype-naked ( value-list | [ expr ] | [ ] VectorArray )`.
- `expression`: one flat list of alternatives with a different order of levels (`& ^ | && ||` on one level, no `**`, unary `+`) -> sections 2.8 and 3.
- Import: `import a.b.*;` -> `import ( a.b, c.* ) [as name]`, no semicolon.
- Interface declarations: with `: list` -> without it.
- Enum declaration: list optional -> at least one item.
- Method declaration: `[: uType] [block | infer-expr]` -> also `: type = expr;` and the `;` form; generic functions `def name<T>(...)`.
- Delegate: `def delegate::name(...)` -> a `def` that ends in `;`.
- Operator declaration: `def operator <op> ( ... )` -> the name is any reference pointer that is not a plain `name (`.
- Variable declaration: `[accesstype] [storagelocality] [inject] name [: uType] [infer | = value] (; [getter-setter] | , variable-decl ;)` -> same idea; later names in the comma list take only `= value`; accessors come after the `;` in either order.
- Alias declaration: no terminator -> ends with `;`.
- `block`: `{ statements* }` -> also a single statement without braces.
- If: `if (e) block [else [if] block]*` -> `else if` chains, then at most one `else`.
- For: `for ( [uTypeArg]; [e]; e )` -> the first part is a variable declaration; all three parts optional; `for ()` and `for` alone exist.
- Foreach: `foreach ( uTypeArg : expression )` -> `foreach ( name [: utype] in expression )`.
- Try: `try block [catchClause [finallyBlock]]` -> any number of catch clauses, optional finally, at least one of the two.
- Return: `return value ;` -> the value is optional.
- `asm-decl`: `asm ( "..." ) ;` ("not added") -> `asm { instructions }`.
- `statement`: see the list in 2.7.

### 7.3 In the draft, gone now

- `var` as a native type; `thread_local`.
- `field-initlization`, `qualifying-field-initlization-list`, and with them `new uType { expression-list }` and `new uType { field = value }`.
- `[.] uType . class` (`ast_utype_class_e`).
- The arrow lambda without braces: `name -> expression` (the code is commented out).
- `free ( expression )`, `malloc ( uType , expression )`.
- The C-style cast `( uType ) expression`.
- Unary `+`.
- `switch`, `switch-block`, `case-declarator`, `default-declarator`.
- `goto identifier ;` and the label `identifier : statement` (functions exist, never called).
- `def delegate::`.
- "Method Invocation statement" as its own rule (it is an expression statement).
- `refrence-identifier` as something distinct from `refrence-pointer` (kept here as an alias).

### 7.4 Exists now, missing from the draft

- Lexical: block comments (nesting); hex and exponent numbers; `$` interpolation; the tokens `:=` `?:` `?.` `!!` `!!.` `**` `->` `#` `@`.
- Declarations: `obfuscate`; generic functions; `: type = expr;` bodies; `-> base(...)` on constructors and `init`; `native`, `override`, `excuse`, `ext`, `stable`, `local` access types; `localized`; `import ( ... ) as name`; top-level functions and variables.
- Types: nullable `?` and `??`; `nil` results; named parameters in function types.
- Statements: `when` (with and without subject); `asm { }` with its instruction grammar; the empty statement `;`; expression statements that end in a trailing lambda without `;`.
- Expressions: elvis `?:`; `!!`; `?.` and `!!.` chains; `as` and `is`; `**`; trailing lambdas and the implicit `it` form; `get<t>(...)`; `base@type`; method calls on string literals; calling the result of a call or of an index (`f()()`, `a[i](x)`); `new t[n]` and `new t[] { }`.
- Every decision procedure of section 4 and every trap of section 5.
