# The standard library looked over, 21 November 2026

The time code was reworked, and the rest of `lib/support/0.4.0/std` (without `guikit`) was read
for things that exist twice, things that are wrong, and things nothing uses. This note says what
was changed and what was found and left for a decision.

**The decisions were taken the same day.** What follows under "Found, not changed" is the list
as it was found; what was done about each line is in `library_cleanup_2026-11-21.md`.

## Time: what changed

The class `time` of the module `std` (`std/time.sharp`) is gone. Everything about time is in
the module `std.time`.

| Before | Now |
|---|---|
| `time.measure(milliseconds, { -> work(); })` | `measure_time { work(); }` gives a `duration`; `measure_millis { work(); }` gives the number |
| `time.measureMills(...)` | `measure_millis { ... }` |
| `time.mills_time()`, `time.nano_time()` | `utc_mills()`, `nano_time()` (they were there already), or `system_clock.instance.millis()` / `.micros()` / `.nanos()` |
| `time.micro_time()` | `system_clock.instance.micros()` |
| `time.sleep(ms)`, `time.sleep(unit, n)` | `thread.sleep(ms)`, `thread.sleep_micros(n)` |
| `time.nano_to_micro / _mills / _secs / _unit` | `duration.of_nanos(n).to_micros()` / `.to_millis()` / `.to_seconds()` |
| `time.get_os_time(time_request.tm_hour)` | `system_clock.local_field(system_clock.local_hour)` |
| `time_unit`, `time_request`, `time.wait`, `time.INFINITE` | gone (`wait` did nothing: its instruction was switched off) |
| `thread.suspend_for(ms)` | gone: it set a flag, called the `wait` that did nothing, and cleared the flag; nothing called it |

```
import ( std.time )

took := measure_time { sort(items); };
println("sorting took " + took.to_string());        // "sorting took 1.25s"
```

`measure_time` is in `std/time/clocks/measure.sharp`. With a clock as its first argument
(`measure_time(clock, block)`) a test gets exact numbers from a `manual_clock`.

A program that used the old class has to change to the right-hand column.

## Fixed

| Where | What was wrong |
|---|---|
| `println(numbers)` for `_int16[]`, `_int32[]`, `_int64[]` and the unsigned arrays | each printing function called itself: the program ended with a stack overflow. A `decimal[]` was printed through a conversion to characters. |
| `"hello".starts_with("")` | gave false |
| `url_codec.parse_query`, so `http_exchange.query(...)` in the server | a `?` inside a value (`next=/a?b=1`) was taken for the start of the query, and the pairs before it were lost |
| `url.resolve("page?u=http://x")` | was read as a whole address and refused |
| `list.drop_last(-1)` | read past the end of the list |
| `list.join_to_string(", ", "[", "]")` of an empty list | gave `[null]` |
| `synced_list.hash()` | went over the empty slots of its storage too (a null there ended the program), and its counter never moved |
| `file.set_last_modified` | refused times after the year 2038 as "negative" |
| `file_system.set_read_only` | lost the answer of its first step |
| `atomic_ref.write` | called an unlock function that does not exist (the class could not be used) |
| `tcp_socket.read_line`, `subprocess.read_line` | left out every `\r` of a line, not only the one before `\n` |
| `environment.remove` on Windows | handed over `null!!` where a text is asked for; it now passes C's NULL as what it is |

Each has a test in the suite except the printing (seen by running it), `synced_list.hash`,
`set_read_only`, `set_last_modified` and `atomic_ref`.

## Found, not changed: the same thing twice

These need a decision about which name stays, because programs use both.

| What | Where | Suggested |
|---|---|---|
| filter, map, take, sorted_by ... | the new functions of `list` (`collection_functions.sharp`) and the static classes `collections<T>`, `transforms<T, R>`, `lists<T>` in `std/collections` | keep the list functions; the static classes are used only by their own tests. They also differ: `average_of` of nothing is 0 in one and an exception in the other; `take(-1)` likewise. |
| a stable merge sort | six copies: `list`, `arrays`, `var_arrays`, `collections`, twice in `collection_functions` | one for objects, one for numbers |
| `hash_table`, `synced_list` | copies of `hash_map` and `list` with a lock around each function, and already behind them (no `sort`, no `entries`) | a locked wrapper around the plain class |
| files | `file_system.sharp` and `files/file_natives.sharp` hold the same VM calls; `file` has an old and a new name for most things (`get_name` / `name`, `delete` / `remove`, `length` / `size`, `read` / `read_text`) | keep `file_natives` and the new names; the compiler still uses some old ones |
| path pieces | separator in 5 places, "is this path absolute" in 3 that do not agree, extension and parent in 2 | `path` |
| number to text | `io_strings.number_to_string`, `math/number_text`, `number_format.plain`, and the VM's own (`"" + x`) | the VM's: the others lose digits for very small and very large numbers |
| whole number to text, number to hex, text to number | 4 to 5 places each (`io_strings`, `number_format`, `number_parser`, `primitives/number_extensions`, `hashing`, `crypto`) | `io_strings` for writing, `number_parser` for reading (it checks the range; `int.parse_or_null("99999999999")` does not) |
| a growing text buffer | `text_buffer` and `char_buffer` | `char_buffer` |
| lines of a text | `io_strings.split_lines`, `string.lines`, `scanner.next_line`, `string_reader.read_line`; `"a\n"` is one line in the first and two in the second | one rule |
| character questions | three sets: on `char` (`is_letter`, `upper_case`), in `primatives.sharp` (`is_alpha`, `to_upper`), the class `character` | one set of names |
| CRC-32 | `util/hashing` (bit by bit) and `compress/gzip` (by a table) | the table |
| square root, power, logarithm | `math/math.sharp` and `math/native_math.sharp` call the same VM instructions under two sets of names | one forwards to the other |
| the environment | `environment`, `process.environment`, `platform_info.environment_or` | `environment` |
| running a program | `process` (through a shell and temporary files) and `subprocess` (pipes) | `subprocess`; `process` on top of it |
| exceptions that mean the same | `unsupported_operation` / `invalid_operation`, `not_found` / `no_such_element` | one each |

## Found, not changed: probably wrong

| Where | What |
|---|---|
| `thread_group.remove` | locks itself and then calls `find`, which locks again; the VM's lock does not count, so the inner one ends the outer early |
| `thread` `==` | compares a handle that is -1 for every thread not started yet, so in its group a new thread takes the place of another one that has not started |
| `read_line()` of the console | stops at `\r` and leaves the `\n`: with input that has `\r\n` line ends, every second line read is empty. Not changed because a key press of Enter gives `\r` alone, and waiting for a `\n` there would hang. |
| `hash_map.set_threshold(int)` | a load of 0.75 becomes 0; nothing calls it |
| `new hash_map(0)` | divides by zero at the first `put` |
| `lazy` | says it remembers a null result and throws for one |
| `string_builder + ""` | adds the word "null" (an empty text in quotes arrives as null) |
| `"abc".substring(3, 3)` | throws; `substring(3)` does not |
| `io_strings.shell_quote` on Windows | an argument that ends in `\` comes out as `"C:\dir\"`, which a program reads as an open quote |
| `process` | its temporary files stay when writing the input fails |
| `file_stream` | a file that is never closed is never given back |
| `directory_listing` | starts one shell per folder, so walking a tree starts a process for each folder |

## Found, not changed: nothing uses it

`critical_section`, `reentrant_lock`, the ten `invert` functions of `memory.sharp`, `sizeOf`,
`key_waiting`, `strtol`, `file_system.list_tree_files`, `file.file_attrs` / `last_accessed` /
`get_full_path`, `build.is_linux`, `string_builder.expand`, `hash_map.set_threshold`,
`list.linear_sort` (one example), `collections/generic_functions.sharp` (a demonstration).

## Found, not changed: names

- The file `std/primatives.sharp` beside the folder `std/primitives`.
- Words run together or written with capitals in a library of words with underscores:
  `indexof`, `sizeOf`, `initialCapacity`, `destStart`, `startPos`, `recursiveLocks`.
- Is it empty: `empty()`, `is_empty()`, `is_not_empty()`, `not_empty()`; some answer `bool`,
  some a number.
- `hash_map` and `linked_hash_map` answer differently (`bool` / number, `size` a field / a
  function, entries as `pair` / `map_entry`).
