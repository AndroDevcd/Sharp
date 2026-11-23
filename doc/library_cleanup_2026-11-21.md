# The standard library cleaned up, 21 November 2026

The audit of the same day (`library_audit_2026-11-21.md`) listed what the library had twice,
what was wrong and what nothing used. This note says what was done about it. It is also the
list to change a program by: the left column is what it was, the right column what it is now.

The rules the library follows after it:

- **One name for one thing.** Where two classes or two functions did the same job, one stayed.
- **A question is answered with a `bool`.** `is_empty()`, `contains(x)`, `put(k, v)` ("was it
  new?") and the comparison operators of the library's classes gave 1 or 0 as a number in some
  places and a bool in others. They all give a bool. Printed, that is `true` / `false`.
- **A count is a plain number.** `size()` of a list, a text, a string_builder gave an `int`
  object (`names.size().value`); it gives the number (`names.size()`).
- **Plain numbers in, plain numbers out** for the functions of `std.math` and `character`.
- **Words with underscores**: `index_of`, not `indexof`.

A program written for 0.5.4 that uses a name of the left column does not compile until it is
changed: the compiler names the place.

## Collections

| Before | Now |
|---|---|
| `collections<T>.filter(items, f)`, `.sorted_by`, `.any` ... (45 static functions) | the functions of a list: `items.filter { ... }`, `items.sorted_by { ... }` (`collection_functions.sharp`). Another collection first becomes a list: `seen.to_list().filter { ... }` |
| `collections<T>.chunked`, `.windowed`, `.flatten` | stay: they give or take lists of lists |
| `transforms<T, R>.map(items, f)`, `.group_by`, `.fold`, `.zip` ... | `items.map<R>() { ... }`, `items.group_by<K>() { ... }` ... |
| `lists<T>.binary_search(l, key, compare)`, `.insert_all`, `.remove_range`, `.swap`, `.count_of`, `.equals`, `.iterator` | `l.binary_search(key, compare)`, `l.insert_all(at, others)`, `l.remove_range(from, to)`, `l.swap(a, b)`, `l.count_of(x)`, `l.equals(other)`, `l.iterator()` |
| `lists<T>.is_sorted(l, compare)`, `.sorted(l, compare)`, `.sort(l, compare)` | `l.is_sorted_with(compare)`, `l.sorted_with(compare)`, `l.sort(compare)` |
| `lists<T>.remove_all(l, x)`, `.remove_if(l, f)`, `.retain_if(l, f)` | `l.remove_every(x)`, `l.remove_all { ... }`, `l.retain_all { ... }` |
| `lists<T>.of(array)`, `.copy_of(l)`, `.add_array(l, array)` | `list_of<T>(array)`, `l.to_list()`, `l.add_all(list_of<T>(array))` |
| `list.indexof(x)`, `list.last_indexof(x)` (an `int` object) | `list.index_of(x)`, `list.last_index_of(x)` (a number) |
| `list.empty()`, `string.empty()`, `string.not_empty()` | `is_empty()`, `is_not_empty()` |
| `list.append_all(other)` | `list.add_all(other)` |
| `list.linear_sort(swap)` | `list.sort(compare)` |
| `list.set_compact(1)` | gone (nothing used it) |
| `list.size().value`, `text.size().value` | `list.size()`, `text.size()` |
| `map.size` (a field of `hash_map`) | `map.size()`, as every other map |
| `hash_map.entries()` gave `pair` (`first`, `second`) | gives `map_entry` (`key`, `value`), as `linked_hash_map` and `tree_map` do; `map_entry` is in the module `std` |
| `hash_map.set_threshold(int)` | `set_threshold(decimal)` (0.75 was turned into 0) |
| - | `hash_map`: `at_or_default`, `put_if_absent`, `put_all`, `contains_value`, `take`, as the other maps have |
| `hash_table`, `synced_list`: copies of `hash_map` and `list` with locks, behind them | the plain class with a lock around every call; they have every function of it (`sort`, `entries`, `put_if_absent` in one step ...) |
| `var_arrays` / `arrays<T>` / `list` / `io_strings` each with a merge sort | one for objects (`list.sort_array`; `arrays<T>.sort` and `io_strings.sort` come there), one for numbers (`var_arrays.sort`), and `var_arrays.order_of(keys)`, the positions in order, for `sorted_by` |

## Text and numbers

| Before | Now |
|---|---|
| `text_buffer` (`std.text`) | `char_buffer` (`std.io`); it took over `build()`, `last()`, `drop_last()`, `ensure_capacity()`; `append_repeat` is `append_repeated` |
| `buffer.to_string()!!` | `buffer.build()` |
| `character.is_digit(c)` in `std.text`, taking a `char`, answering 1 or 0 | in `std`, takes the code of a character (`'7'`, `text.data[i]`, `c.value`), answers `bool`; `to_upper` / `to_lower` / `digit_value` give the code |
| `char.is_digit(c)`, `char.is_alpha(c)`, `char.is_alpha_num(c)`, `char.to_upper(c)`, `char.to_lower(c)` | `character.is_digit(c)`, `.is_letter(c)`, `.is_letter_or_digit(c)`, `.to_upper(c)`, `.to_lower(c)` |
| a `char` object: `c.is_upper_case()`, `c.is_lower_case()`, `c.is_decimal_digit()`, `c.upper_case()`, `c.lower_case()` | `c.is_upper()`, `c.is_lower()`, `c.is_digit()`, `c.to_upper()`, `c.to_lower()`: the names of `character` |
| `io_strings.is_space(ch)`, `.is_digit`, `.is_letter`, `.lower_char`, `.upper_char`, `.hex_value` | `character.is_whitespace(ch)`, `.is_digit`, `.is_letter`, `.to_lower`, `.to_upper`, `.digit_value(ch, 16)` |
| `io_strings.number_to_string(x)`, `number_to_string(x)` of `std.math`, `"" + x`: three results | all three are the text the VM makes: the fewest digits that read back as the same number. `1e21` is `"1e+21"`; the VM writes `NaN`, `Infinity`, `-Infinity` (was `nan`, `infinity`) |
| `hash_work.to_hex(bytes)`, `hashing.to_hex` and `tls` with digits of their own | `hex.encode_bytes(bytes)`, `io_strings.int_to_hex(n, digits)` |
| `"abc".substring(3, 3)` threw | `""` |
| `builder + ""` added the word `null` | adds nothing |

## Files

| Before | Now |
|---|---|
| `f.get_path()` | `f.path` (read it; only the class writes it) |
| `f.get_name()`, `f.get_parent_file()`, `f.get_absolute_path()`, `f.get_full_path()` | `f.name()`, `f.parent()`, `f.absolute_path()` |
| `f.length()` (an `int` object) | `f.size()` |
| `f.create()`, `f.delete()`, `f.list()` | `f.create_new()`, `f.remove()`, `f.list_names()` |
| `f.read()` (null for an empty file), `f.write(text)` (a number) | `f.read_text()`, `f.write_text(text)`: they throw `io_exception` |
| `f.rename(other)` | `f.move_to(other)` |
| `f.mkdir()` gave 0 when it worked | gives `true` |
| `f.set_writable(1)`, `set_readable`, `set_executable`, `set_read_only()`, `set_last_modified(t)` gave 0 (or the time) | take and give `bool` |
| `f.last_modified()` (a `long` object) | a number |
| `f.is_writeable()` | `f.is_writable()` |
| `f.get_total_space()`, `get_free_space()`, `get_usable_space()` | `f.total_space()`, `free_space()`, `usable_space()` |
| `file.create_temp_file(prefix, suffix)` | `temp_files.create_file(prefix, suffix)` |
| `file.separator_char()`, `file_natives.separator()` | `path.separator()` |
| `file_natives.is_rooted(p)`, `file_natives.parent_of(p)` | `path.is_absolute(p)`, `path.parent(p)` (`""` when there is none) |
| the classes `file_system`, `unix_fs`, `win_fs` | gone: `file_natives` holds every call to the VM once |

Inside `file` (and wherever a variable is called `path`) the class `path` is written
`std.io#path`.

## Math

| Before | Now |
|---|---|
| `sqrt(x).value`, `floor(x).value`, `pow(a, b).value` ... (a `double` object in and out) | `sqrt(x)`, `floor(x)`, `pow(a, b)`: plain numbers. For a number object pass its `value` |
| `square_root(x)`, `power(a, b)`, `ln(x)`, `exp_of(x)`, `floor_of(x)`, `ceil_of(x)`, `absolute(x)`, `sin_of`, `cos_of`, `tan_of`, `asin_of` ... | `sqrt`, `pow`, `log`, `exp`, `floor`, `ceil`, `abs`, `sin`, `cos`, `tan`, `asin` ... |
| `atan(y, x)` | `atan2(y, x)` |
| `PI.value`, `NaN.value`, `POSITIVE_INFINITY.value` | `PI`, `NaN`, `POSITIVE_INFINITY`: plain numbers |
| `is_nan(d)`, `is_infinite(d)` of a `double` object | of a plain number |
| CRC-32 in `hashing` (bit by bit) and in `std.compress` (a table); Adler-32 twice | `hashing.crc32`, `hashing.adler32` (the table, the long sums) |

## System

| Before | Now |
|---|---|
| `process.environment(name)`, `platform_info.environment_or(name, other)` | `environment.value(name)`, `environment.value_or(name, other)` |
| `ill_arg_except`, `ill_state_except`, `bounds_except`, `null_except`, `invalid_op_except`, `runtime_except`, `stack_except`, `cast_except`, `memory_except`, `incompatible_except` | the whole names: `illegal_argument_exception`, `out_of_bounds_exception`, `nullptr_exception` ... |
| `unsupported_operation_exception` | `invalid_operation_exception` |
| `not_found_exception` | `no_such_element_exception` |
| `sizeOf(object)` | `memory_size(object)` |
| `std/primatives.sharp` | `std/primitives/numbers.sharp` |

## Answers that became a bool

About 250 functions. By kind:

- every `is_...`, `has_...`, `contains...`, `equals...`, `starts_with...`, `ends_with...`,
  `same...`, `matches...` that gave a number, and `any` / `all` / `none` of a list;
- "did it happen": `put`, `put_if_absent`, `remove` of `linked_hash_map` and `tree_map`, `add` and
  `remove` of `hash_set` and `tree_set`, `remove` of `linked_list` and `priority_queue`;
- `==`, `!=`, `<`, `<=`, `>`, `>=` of `date`, `date_time`, `duration`, `instant`,
  `offset_date_time`, `time_of_day`, `period`, `zone_offset`;
- `key_waiting()`, `number_parser.parse_bool`, `iterator.has_next()`.

What changes in a program: `if(set.add(x) == 1)` is `if(set.add(x))`; a bool added to a text
is `true` / `false`. `expect(x.is_empty()).should_be(1)` still passes.

## Mistakes mended

| Where | What was wrong |
|---|---|
| `thread_group.remove` | locked itself, then called `find`, which locked again and ended the outer lock early |
| `thread` `==` | compared a handle that is -1 for every thread not started: in its group a new thread took the place of another |
| `new hash_map(0)` | divided by zero at the first `put` |
| `read_line()` of the console | with `\r\n` line ends every second line was empty; the `\n` after a `\r` is now part of that line end |
| `io_strings.shell_quote` on Windows | `C:\dir\` came out as `"C:\dir\"`, an open quote; back slashes at the end are written twice |
| `process.run` | its temporary files stayed when a step threw |
| `lazy` | said it remembers a null result; it throws for one, and says so |
| `file.set_last_modified` | answered the time it set, read by callers as "0 is success" |

Each has a test in `tests/suite/cases/lib_cleanup_fixes.sharp` except the console's line ends
(needs a keyboard or a pipe) and the temporary files.

## Found on the way: the optimizer and an array's length

With `-O` five suite cases wrote past the end of a list once `list.expand()` had become one line.
The optimizer took an array's length for something that never changes; `realloc` makes the same
array longer. As long as `expand()` was too big to be put inline, the call hid that. Now

- a length worked out before a call that may change anything is worked out again after it
  (common subexpressions, loop invariants: `optimizer/analysis/expressions.sharp`);
- a bounds check stays where such a call runs between the loop's test and the element, or, when
  the length was taken before the loop, anywhere in the loop (`passes/control/bounds_checks.sharp`).

A loop without calls before its element access loses nothing. `tests/suite/cases/arrays_grown_inline.sharp`
is the program that failed, as any program with such a class would have.

## What the changes cost a program

- `name?.size() ?: 0` does not compile any more: `?.` cannot give a plain number. It is
  `(name ?: "").size()`, or a test for null. (`?.` with a plain number behind it would be a
  change to the language.)
- A function type given as the value type of a `hash_map` is reported twice (E0351): once where
  it is written, once where `hash_map` names itself in `put_all`. The second report is one too many.

## Taken out: nothing used it

`critical_section`, `reentrant_lock`, the enum `buffer_type`, the ten `invert` functions of
`memory.sharp`, `strtol`, `list.set_compact`, `list.linear_sort`, `file.file_attrs`,
`collections/generic_functions.sharp` (a demonstration; its test uses `list_of<T>` and `map<R>`).

## Left as it is, and why

| What | Why |
|---|---|
| `io_strings.split_lines("a\n")` is one line, `"a\n".lines()` is two | on purpose: the first is the lines of a file, the second splits and joins back to the same text (Kotlin has both rules too). Each comment says so. |
| `file_natives.join(dir, name)` beside `path.join` | the first only puts a separator between a directory and a name read from it (a walk of a tree calls it for every entry); `path.join` cleans both parts |
| `process` beside `subprocess` | `process.run` waits and hands over everything; done with pipes it would have to read two streams at once. Its temporary files no longer stay. |
| `directory_listing` starts a shell for each folder | the VM's call does not report folders or hidden names. Listing through C (`opendir`, `FindFirstFile`) is the fix; not done here. |
| `file_stream`: a file that is never closed stays open | the language has no finalizers |
| `number_format.to_radix` beside `to_string_radix` of a number object | not merged |
| `thread`: `get_id()`, `get_name()` ... | the VM starts and ends threads through this class; its names were left |
| `string.at(i)` gives a `char` object | changing it changes every loop over a text |
| `std.guikit` | not looked at (it compiles and its uses of renamed functions were changed) |

## Checked

Linux: `build.sh`, then `test.sh` (the groups run are in the report of the day). Windows: the
suite, each case plain and `-O`, with the JIT and without. ARM (qemu) was not run.
