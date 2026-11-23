# Reflection and the functions of a list, 21 November 2026

Two additions to the standard library: `std.reflect`, written again from nothing, and the
functions Kotlin's collections have (`filter`, `map`, `sorted_by` ...) as functions of `list`.

## Reflection: `std.reflect`

A program looking at its own classes. The old `std.reflect` (`describe`, `_class_`, `_field_`,
`_function_`, `new_object`, `set_field` ...) is gone; nothing of it is kept.

```
import ( std.reflect )

shape := reflect.of<user>();                    // from a type; no object is made
shape := reflect.of(someone)!!;                 // from an object
shape := reflect.find("app.models#user")!!;     // from a full name

foreach(field in shape.fields()) {
    print(field.name + " : " + field.type.name + "\n");      // "tags : list<string>"
}

someone := reflect.create<user>("ann", new int(31));         // any constructor
age := shape.field("age")!!;
age.number(someone)                             // 31
age.write(someone, 32);
shape.function("greet")!!.call(someone, "bob")  // what someone.greet("bob") gives

view := reflect.on(someone);                    // one object, by names
view.read("home.city")
view.write("age", 33);
view.call("greet", "bob")
view.to_text()                                  // user(name=ann, age=33, home=address(city=oslo))
view.copy()
view.same_as(other)
```

| Class | What it is | The most used |
|---|---|---|
| `reflect` | the front door | `of<t>()`, `of(object)`, `find(name)`, `classes()`, `create<t>(...)`, `on(object)`, `type_of(object)` |
| `class_info` | a class, an interface or an enum | `name`, `full_name`, `module`, `base_class()`, `interfaces()`, `is_a(other)`, `fields()`, `field(name)`, `static_fields()`, `functions()`, `function(name)`, `constructors()`, `create(...)`, `create_array(n)`, `values()`, `type_arguments()` |
| `field_info` | one field | `name`, `type`, `is_static`, `is_private`, `is_constant`, `declared_in()`, `read(target)`, `number(target)`, `text(target)`, `write(target, value)`, `open()`, `is_open()` |
| `function_info` | a function, a constructor or an operator | `name`, `parameters`, `result`, `is_static`, `owner()`, `call(target, ...)`, `call_static(...)`, `call_on(target, values)` |
| `type_info` | what a field, a parameter or a result is | `name`, `full_name`, `kind`, `is_number`, `is_class`, `is_text`, `is_array`, `class_of()`, `arguments()` |
| `object_view` | one object with its class | `read(path)`, `number(path)`, `text(path)`, `write(name, value)`, `call(name, ...)`, `fields { }`, `to_text()`, `copy()`, `same_as(other)` |
| `enum_value` | one value of an enum | `name`, `number`, `value` |

Things to know:

- **`read` and `write`.** `get` and `set` are words of the language.
- **A wrong value is refused where it is given**, with a `reflect_exception` that names the
  field: `` `main#user.home` holds a `address` and was given a `point` ``. The same for a call
  with wrong arguments and for a constructor that does not exist (the message lists the
  constructors there are).
- **Numbers travel as objects.** A value read or handed over as an `object` is an object, so
  a number argument is written `new int(31)`, and a number result is taken out of its number
  class: `(found as long).value` for an `_int64`, `(found as double).value` for a `decimal`.
  `field.number(target)` and `field.write(target, 31)` take and give plain numbers.
- **Private, protected and const are kept** (see Protections below).
- **Enums.** `reflect.of<color>().values()` gives each value with its name and number; json
  reads and writes enums with it.
- **A call runs the function the `function_info` is.** For what an object's own class has
  under a name, ask that class: `reflect.on(thing).call("area")`.
- **At most 7 arguments** in a call by reflection.

### Protections

Reflection lists everything a class has, and does not go around what the class closed:

| What | By default | After `open()` |
|---|---|---|
| a private or protected field | `read`, `number`, `text`, `write` throw a `reflect_exception` | allowed |
| a private or protected function or constructor | `call`, `create` throw | allowed |
| a `const` field | read; `write` throws | `write` still throws |

```
code := shape.field("code")!!;
code.read(box)                      // `main#vault.code` is private: open() it (or its class_info) before reading it
code.open().read(box)               // one field, on purpose
wide := shape.open();               // the whole class: fields, functions, private constructors
reflect.on(box).open().read("code") // one object
```

- **`open()` gives another object and leaves the first closed.** Opening a field in one place
  does not open it for the rest of the program. An open `class_info` is a second object for
  the same class: compare the two by `id`.
- **A view shows what is public.** `to_text()` and `fields { }` of `reflect.on(x)` leave
  private fields out; those of `reflect.on(x).open()` have them. `copy()` and `same_as()` go
  by every field either way, since they hand no private value out.
- **json opens the classes it fills**, so private fields are read and written as before. It
  leaves a `const` field as it is.
- **These checks are the library's.** Code that writes `asm` or calls `reflect_vm` itself is
  not held by them; they stop a program from doing it without saying so.

### What stays in the program

An executable holds only what its text uses. Reflection shows what is there, so code that
works by names says what it needs:

| Written | What stays |
|---|---|
| `reflect.of<t>()`, `reflect.create<t>()` | all of `t` and of the classes above it: fields, functions, constructors |
| `reflects<t>()` | the fields of `t` and its constructor without parameters, and the same for the class of each field (what `json.to<t>` says) |
| the compiler option `-eager` | everything (for `reflect.find` of any class) |

`reflect.of(object)` and `reflect.find(name)` always work and show what stayed.

### Not known to a program, so not here

- The names of parameters (their types are there).
- Whether a type is written with `?`.
- `bool` is `_uint8` in a program: a `bool` field's type says `_uint8`.

These three need the compiler to write more into the executable. The same goes for notes
on declarations (`@json("user_name")`) and for objects made from an interface, which the
draft named as a later stage: neither is built.

### How it is built

- **The VM knows no class of `std.reflect`.** It answers questions in numbers and names
  ("class 12: its base, its flags, how many fields"), and the library builds its objects from
  the answers the first time each is asked for. Nine interrupts, in
  `lib/runtime/interrupts/reflection.cpp`.
- **Nothing is built when a program starts.** The old reflection made an object for every
  class, field and function of every program at start-up, used or not.
- **A call** lays the arguments on the stack as the function expects them and uses the
  instruction that calls a function value (`calld`); the result comes back as an object.
- **The compiler** no longer asks for six `std.reflect` classes in every program, and knows
  `reflects_whole<t>()` by name beside `reflects<t>()`.

## Functions of a list

What Kotlin has as extension functions of its collections, as functions of `list<T>`
(`lib/support/0.4.0/std/collection_functions.sharp`). No import is needed.

```
adults := people.filter { it.age >= 18 };
names  := people.map<string>() { it.name };
oldest := people.max_by { it.age };
by_city := people.group_by<string>() { it.city };
text   := people.filter { it.age >= 18 }
                .sorted_by_text { it.name }
                .join_to_string(", ") { it.name };
total  := prices.fold<int>(new int(0)) { so_far, price -> new int(so_far.value + price.value) };
words  := list_of<string>(line.split(",")).map<string>() { it.trim() }.sorted();
```

A function that makes another type of thing names the type and has `()` before its block:
`map<string>() { ... }`.

| Group | Functions |
|---|---|
| keeping some | `filter`, `filter_not`, `filter_indexed`, `partition`, `distinct`, `distinct_by` |
| a part | `take`, `take_last`, `drop`, `drop_last`, `take_while`, `take_last_while`, `drop_while`, `drop_last_while`, `slice` |
| one element | `first { }`, `last { }`, `first_or_null`, `last_or_null`, `find`, `find_last`, `only`, `only_or_null`, `at_or_null`, `at_or`, `random`, `random_or_null`, `index_of_first`, `index_of_last` |
| questions | `any`, `all`, `none`, `is_not_empty`, `contains_all` |
| numbers | `count`, `sum_of`, `average_of`, `min_of`, `max_of`, `min_by`, `max_by`, `min_by_or_null`, `max_by_or_null`, `min_with_or_null`, `max_with_or_null`; on lists of numbers `sum`, `average`, `min`, `max` |
| order | `sorted_by`, `sorted_by_descending`, `sorted_by_text`, `sorted_by_text_descending`, `sorted_with`, `sorted`, `sorted_descending`, `reversed`, `shuffled`, `is_sorted_by` |
| with others | `plus`, `plus_one`, `minus`, `minus_one`, `intersect`, `union`, `zip`, `to_list` |
| something else | `map`, `map_indexed`, `map_not_null`, `flat_map`, `first_not_null_of`, `fold`, `fold_right`, `running_fold`, `reduce`, `reduce_or_null`, `running_reduce`, `group_by`, `count_by`, `associate_by`, `associate_with`, `join_to_string` |
| each | `for_each`, `for_each_indexed` |
| changing the list | `remove_all { }`, `retain_all { }`, `sort_by`, `sort_by_descending`, `sort_by_text`, `shuffle`, `fill`, `add_every` |
| starting one | `list_of<t>(array)` |

Different from Kotlin, and why:

- **`single` is `only`.** `single` is a word of the language.
- **Sorting by a key has two forms**: `sorted_by { number }` and `sorted_by_text { string }`.
- **`chunked`, `windowed`, `zip_with_next` and `flatten` are not functions of a list.** They
  give a list of lists (or of pairs of the list's own type), and a function of every list that
  does so makes the compiler build `list<list<T>>`, then `list<list<list<T>>>`, without end.
  `chunked`, `windowed` and `flatten` are there as `collections<T>.chunked(items, 3)`.
- **Only `list` has them.** Another collection becomes a list first: `seen.to_list().filter { ... }`.
  As functions of every collection they would also belong to `string`, which is a collection
  of characters, and replace its own `slice` and `find`.
- **Scope functions (`let`, `also`, `apply`, `run`, `take_if`) are not built.** A function of
  every object would see `it` as a plain `object`, without the type that makes them useful.
- **Maps**: `map.entries()` is a list, so `ages.entries().filter { it.second.value > 30 }` works.

Strings already had Kotlin's set (`trim`, `split`, `substring_before`, `pad_start`,
`to_int_or_null` ...: `std/text/string`). Added to them: `if_empty`, `if_blank`,
`filter_chars`, `count_chars`, `any_char`, `all_chars`.

Source of the list of functions: the Kotlin documentation's pages on collection
transformations, filtering, grouping, parts, single elements, ordering, aggregates, write
operations, list and map operations, and strings (kotlinlang.org/docs).
