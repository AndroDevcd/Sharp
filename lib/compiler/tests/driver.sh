#!/bin/bash
# The driver's options that name FOLDERS and THREADS: `-L <folder>` and `-j <n>` (main.sharp, configuration/,
# source_folders.sharp; doc/codegen.md 8.2 and 13.5). The suite runner, the benchmark script and
# every build tool call the compiler as
#
#       <sharpc> <files> -j 1 -o <exe> -L <library>
#
# so these two must behave as the C++ compiler's do.
#
#   tests/driver.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>
#       LIBRARY=<the standard library folder>  OUT=<a folder to work in>
#       (defaults: ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/stdlib, ~/sharp_work/out/boot/driver)
#
# What is checked:
#   1. a library of its own next to the standard one: files in the folder, in a folder below it
#      and two below it are all found; a trailing `/` on the folder changes nothing; a folder
#      whose NAME ends in .sharp is not taken for a file; a file that is not .sharp is ignored
#   2. a folder whose name starts with a dot is not source (the C++ compiler skips it too): a
#      broken file in `.hidden/` does not stop the compile
#   3. a file named on the command line AND found below -L is compiled once
#   4. a library folder that does not exist is a warning, and the compile goes on
#   5. `-j`: 1 and 4 are accepted; `abc`, `0`, `-3`, `1.5` and a missing number are refused with
#      a message that names the option, and nothing is written
#   6. the same bytes every time: two compiles of one program give the same file
#   7. COMPILERS SIDE BY SIDE: 4 compiles started at the same moment, 3 times over, must each
#      write the file a compile on its own writes. (With file.walk() they read each other's
#      folder listings - a temporary file named after the clock - and failed with "file ...
#      doesnt exist!" or compiled with library files missing; 13.5.)
#
#   8. THE OUTPUT FILE: without `-o` the executable is `out` in the current folder (as the C++
#      compiler names it); with `-c` nothing is written
#   9. THE EXIT CODE: 0 after a compile without errors, 1 for a program with errors, for an
#      option that is refused and for a file that does not exist
#  10. `-run`: the program is started after the compile (the runtime is `sharp` on the PATH) and
#      its output follows the compiler's; without a `sharp` on the PATH that is said
#  11. PROJECTS: `--new-project` makes the folder, a main.sharp and a settings.json that is JSON;
#      a second `--new-project` of the same name is refused; `-P <folder>` writes
#      build/outputs/<output>, with the library given by `-L`; the second build finds every file
#      unchanged (the build cache), `--clean` finds every file changed again, `-cacheoff` does
#      not use the cache; "output", "extern_lib", "release" of settings.json are followed; a
#      setting with the wrong kind of value and an unknown "target_api" are refused; a name that
#      is not a setting is a warning with the nearest setting, and the build goes on; a
#      settings.json that is not JSON stops the build
#  12. `-cache <folder>` keeps the cache in that folder, also without a project
#  13. `-sni <folder>` writes the native interface header there (sni.h), also for a program
#      without native functions
#  14. an option given an empty value, or an option where its value should be, is refused with a
#      message naming the option; `-j 00`, a too large `-errlmt` and an unknown `-target` too
#  15. an empty argument reaches the PROGRAM as "" (the runtime used to die with SIGSEGV)
#  16. `-o` naming a source file is refused and the source kept; a folder named x.sharp is refused;
#      a missing source file gets did-you-mean
#  17. `-errlmt` limits the syntax errors as well
#  18. `-ir` and `-analysis` show the program's last file when `-L` adds the library after it
#  19. `-run` ends with the program's exit code (main's result, 1 after an uncaught exception), and
#      with 1 and the compiler's own message when there is no runtime on the PATH
#  20. an empty settings.json stops the build; a setting given twice, and `-o` overruled by
#      settings.json, are warnings; a project without the library gets ONE error, E0962
#  21. a module that is missing because `-ignoredir` left its file out: the E0303 says which file
#      declares it and that it was left out (left_out_modules.sharp)
#
# Prints one line per failure and a summary; exits 1 when anything failed.
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
OUT=${OUT:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/driver}
WORK="$OUT/work"
rm -rf "${OUT:?}/work"      # what an earlier run left: a stale file must not pass for a new one
mkdir -p "$WORK/src" "$WORK/my lib/sub/deeper" "$WORK/my lib/folder.sharp" "$WORK/my lib/.hidden" "$WORK/a" "$WORK/b"
cd "$WORK" || exit 1

printf 'mod main;\nimport (helper)\ndef main() { println("sum " + (top_value() + helper_value() + deep_value())); }\n' > src/prog.sharp
printf 'mod helper;\ndef top_value() : _int64 { return 100; }\n' > "my lib/top.sharp"
printf 'mod helper;\ndef helper_value() : _int64 { return 20; }\n' > "my lib/sub/helper.sharp"
printf 'mod helper;\ndef deep_value() : _int64 { return 3; }\n' > "my lib/sub/deeper/deep.sharp"
printf 'this is not Sharp\n' > "my lib/notes.txt"
printf 'mod helper;\ndef broken( { this does not parse\n' > "my lib/.hidden/broken.sharp"

passed=0
failed=0
pass() { passed=$((passed+1)); }
fail() { failed=$((failed+1)); echo "FAIL  $1"; shift; for line in "$@"; do echo "        $line"; done; }

# compile <log> <arguments...>: one run of the bootstrap compiler
compile() {
    local log=$1; shift
    timeout -s KILL 300 "$SHARP" "$COMPILER" "$@" > "$log" 2>&1
}

# builds_and_prints <name> <text> <arguments...>: the compile writes <name>.exe and the program prints <text>
builds_and_prints() {
    local name=$1 text=$2; shift 2
    compile "$name.txt" "$@" -o "$name.exe"
    if [ ! -f "$name.exe" ]; then
        fail "$name" "nothing was written:" "$(grep -m1 -A3 "error" "$name.txt" | head -4)"
    elif [ "$("$SHARP" "$name.exe" 2>&1 | head -1)" != "$text" ]; then
        fail "$name" "the program printed \`$("$SHARP" "$name.exe" 2>&1 | head -1)\`, not \`$text\`"
    else
        pass
    fi
}

# refused <name> <text the message must contain> <arguments...>: the compile writes nothing and says why
refused() {
    local name=$1 text=$2; shift 2
    compile "$name.txt" "$@" -o "$name.exe"
    if [ -f "$name.exe" ]; then
        fail "$name" "a file was written; the option should have been refused"
    elif ! grep -q -- "$text" "$name.txt"; then
        fail "$name" "the message does not contain \`$text\`:" "$(head -2 "$name.txt")"
    else
        pass
    fi
}

# ---- 1 - 4: -L
builds_and_prints nested          "sum 123" src/prog.sharp -j 1 -L "$LIBRARY" -L "my lib"
builds_and_prints trailing_slash  "sum 123" src/prog.sharp -j 1 -L "$LIBRARY" -L "my lib/"
builds_and_prints named_twice     "sum 123" src/prog.sharp "my lib/sub/helper.sharp" -j 1 -L "$LIBRARY" -L "my lib"
builds_and_prints folder_twice    "sum 123" src/prog.sharp -j 1 -L "$LIBRARY" -L "my lib" -L "my lib/sub"
builds_and_prints missing_folder  "sum 123" src/prog.sharp -j 1 -L "$LIBRARY" -L "no such folder" -L "my lib"
grep -q "warning: library path: .*no such folder" missing_folder.txt && pass || fail "missing_folder" "no warning about the folder that does not exist"
grep -q "broken.sharp" nested.txt && fail "nested" "a file in a folder whose name starts with a dot was compiled" || pass

# ---- 5: -j
builds_and_prints threads_4       "sum 123" src/prog.sharp -j 4 -L "$LIBRARY" -L "my lib"
refused threads_word     "option \`-j\`" src/prog.sharp -j abc -L "$LIBRARY" -L "my lib"
refused threads_zero     "option \`-j\`" src/prog.sharp -j 0 -L "$LIBRARY" -L "my lib"
refused threads_negative "option \`-j\`" src/prog.sharp -j -3 -L "$LIBRARY" -L "my lib"
refused threads_fraction "option \`-j\`" src/prog.sharp -L "$LIBRARY" -L "my lib" -j 1.5
refused threads_missing  "option \`-j\`" src/prog.sharp -L "$LIBRARY" -L "my lib" -j

# ---- 6: the same bytes every time (the output is named the same: the manifest carries the name)
(cd a && compile ../same_a.txt ../src/prog.sharp -j 1 -L "$LIBRARY" -L "../my lib" -o prog.exe)
(cd b && compile ../same_b.txt ../src/prog.sharp -j 1 -L "$LIBRARY" -L "../my lib" -o prog.exe)
if [ -f a/prog.exe ] && cmp -s a/prog.exe b/prog.exe; then pass; else fail "same_bytes" "two compiles of one program wrote different files (or none)"; fi

# ---- 7: compilers side by side
side_by_side_failures=0
for round in 1 2 3; do
    for k in 1 2 3 4; do
        mkdir -p "side_${round}_$k"
        (cd "side_${round}_$k" && compile log.txt ../src/prog.sharp -j 1 -L "$LIBRARY" -L "../my lib" -o prog.exe) &
    done
    wait
    for k in 1 2 3 4; do
        if ! cmp -s a/prog.exe "side_${round}_$k/prog.exe"; then
            side_by_side_failures=$((side_by_side_failures+1))
            [ $side_by_side_failures -le 3 ] && echo "        side by side, round $round, compile $k: $(grep -m1 "error\|^parsed" "side_${round}_$k/log.txt" | cut -c1-150)"
        fi
    done
done
if [ $side_by_side_failures = 0 ]; then pass; else fail "side_by_side" "$side_by_side_failures of 12 compiles started 4 at a time did not write the file a compile alone writes"; fi

# ---- 8 - 10: the output file, the exit code, -run
printf 'mod main;
def main() { println("ran"); }
' > src/hello.sharp
printf 'mod main;
def main() { count : _int64 = "text"; }
' > src/wrong.sharp
# exits_with <name> <code> <arguments...>
exits_with() {
    local name=$1 code=$2; shift 2
    compile "$name.txt" "$@"
    local got=$?
    if [ $got = "$code" ]; then pass; else fail "$name" "the compiler ended with exit code $got, not $code:" "$(tail -2 "$name.txt")"; fi
}
mkdir -p default_out && (cd default_out && compile ../default_out.txt ../src/hello.sharp -j 1 -L "$LIBRARY")
if [ -f default_out/out ] && [ "$("$SHARP" default_out/out 2>&1 | head -1)" = "ran" ]; then pass; else fail "default_out" "without -o the file \`out\` was not written (or does not run)"; fi
mkdir -p compile_only && (cd compile_only && compile ../compile_only.txt ../src/hello.sharp -c -j 1 -L "$LIBRARY")
if [ -z "$(ls compile_only)" ] && grep -q "^analysis complete" compile_only.txt; then pass; else fail "compile_only" "-c wrote a file, or the analysis did not finish"; fi
exits_with exit_good 0 src/hello.sharp -j 1 -L "$LIBRARY" -o exit_good.exe
exits_with exit_errors 1 src/wrong.sharp -j 1 -L "$LIBRARY" -o exit_errors.exe
[ -f exit_errors.exe ] && fail "exit_errors" "a program with errors was written"
exits_with exit_option 1 src/hello.sharp -no-such-option -L "$LIBRARY" -o exit_option.exe
exits_with exit_missing 1 src/not_there.sharp -L "$LIBRARY" -o exit_missing.exe

mkdir -p bin
printf '#!/bin/bash
exec "%s" "$@"
' "$SHARP" > bin/sharp
chmod +x bin/sharp
PATH="$WORK/bin:$PATH" compile run.txt src/hello.sharp -j 1 -L "$LIBRARY" -o run.exe -run
if grep -q "^ran" run.txt && grep -q "^code generation: " run.txt; then pass; else fail "run" "-run did not start the program:" "$(tail -3 run.txt)"; fi
PATH="/usr/bin:/bin" compile run_no_runtime.txt src/hello.sharp -j 1 -L "$LIBRARY" -o run2.exe -run
if grep -q "could not start the runtime" run_no_runtime.txt; then pass; else fail "run_no_runtime" "no message about the runtime that is not on the PATH:" "$(tail -2 run_no_runtime.txt)"; fi

# ---- 11: projects
compile new_project.txt --new-project demo
if [ -f demo/src/main.sharp ] && [ -d demo/build/outputs ] && grep -q '"output": "demo"' demo/settings.json; then pass; else fail "new_project" "the project folder is not complete:" "$(find demo 2>&1 | head -8)"; fi
exits_with new_project_twice 1 --new-project demo
compile project.txt -P demo -L "$LIBRARY"
if [ -f demo/build/outputs/demo ] && [ "$("$SHARP" demo/build/outputs/demo 2>&1 | head -1)" = "hello, world!" ]; then pass; else fail "project" "build/outputs/demo was not written (or does not run):" "$(tail -2 project.txt)"; fi
compile project_again.txt -P demo -L "$LIBRARY"
if grep -q "^parsed .* [1-9][0-9]* unchanged since the last build, 0 new or changed" project_again.txt; then pass; else fail "project_again" "the second build did not find every file unchanged:" "$(grep "^parsed" project_again.txt)"; fi
compile project_clean.txt -P demo -L "$LIBRARY" --clean
if grep -q "^parsed .* 0 unchanged since the last build" project_clean.txt; then pass; else fail "project_clean" "--clean did not forget the cache:" "$(grep "^parsed" project_clean.txt)"; fi
compile project_cacheoff.txt -P demo -L "$LIBRARY" -cacheoff
if grep -q "^parsed [0-9]* file(s), no errors$" project_cacheoff.txt; then pass; else fail "project_cacheoff" "-cacheoff still used the cache:" "$(grep "^parsed" project_cacheoff.txt)"; fi
printf '{ "name": "demo", "output": "app", "extern_lib": [ "%s" ], "release": true, "ignore_folders": [ "build" ] }
' "$LIBRARY" > demo/settings.json
(cd demo && compile ../project_settings.txt -p)
if [ -f demo/build/outputs/app ] && [ -f demo/build/outputs/app.names ] && [ "$("$SHARP" demo/build/outputs/app 2>&1 | head -1)" = "hello, world!" ]; then pass; else fail "project_settings" "settings.json (output, extern_lib, release) was not followed:" "$(tail -2 project_settings.txt)"; fi
printf '{ "output": 5 }
' > demo/settings.json
exits_with settings_wrong_type 1 -P demo -L "$LIBRARY"
grep -q "settings option: output" settings_wrong_type.txt && pass || fail "settings_wrong_type" "the message does not name the setting:" "$(tail -1 settings_wrong_type.txt)"
printf '{ "target_api": "9.9" }
' > demo/settings.json
exits_with settings_library_version 1 -P demo -L "$LIBRARY"
printf '{ "ouptut": "app", "version": "2.5", "extern_lib": [ "%s" ], "ignore_folders": [ "build" ] }
' "$LIBRARY" > demo/settings.json
compile settings_unknown.txt -P demo
if grep -q 'unknown setting `ouptut` is ignored: did you mean `output`' settings_unknown.txt && ! grep -q 'unknown setting `version`' settings_unknown.txt && [ -f demo/build/outputs/out ]; then pass; else fail "settings_unknown" "a misspelt setting: no warning with the nearest name, or the build did not go on:" "$(grep "warning\|error" settings_unknown.txt | head -2)"; fi
printf '{ "output": "app",
' > demo/settings.json
exits_with settings_not_json 1 -P demo -L "$LIBRARY"
grep -q "settings.json is not JSON" settings_not_json.txt && pass || fail "settings_not_json" "the message does not say the file is not JSON:" "$(head -2 settings_not_json.txt)"

# ---- 12, 13: -cache <folder>, an option that is accepted but not acted on
compile cache_first.txt src/hello.sharp -j 1 -L "$LIBRARY" -cache "$WORK/my cache" -o cache.exe
compile cache_second.txt src/hello.sharp -j 1 -L "$LIBRARY" -cache "$WORK/my cache" -o cache.exe
if [ -f "my cache/files/cache_info.data" ] && grep -q " 0 new or changed" cache_second.txt; then pass; else fail "cache_folder" "-cache <folder> did not keep the cache there:" "$(grep "^parsed" cache_second.txt)"; fi
compile native_dir.txt src/hello.sharp -j 1 -L "$LIBRARY" -sni generated -o native_dir.exe
if [ -f native_dir.exe ] && grep -q "SNI_VERSION" generated/sni.h; then pass; else fail "native_dir" "-sni <folder>: no executable, or no sni.h in the folder"; fi

# ---- 14 - 20: what the completeness critics found (doc/codegen.md 13.8)
# 14. a value-taking option with an empty value, or with an option where its value should be,
#     is refused and names the option (it used to crash the VM: `-o ""`, or swallow `-c`)
refused empty_value      "option \`-o\` was given an empty value" src/hello.sharp -L "$LIBRARY" -o ""
refused option_as_value  "the argument after it, \`-c\`, is an option" src/hello.sharp -L "$LIBRARY" -v -c
refused threads_zeros    "option \`-j\`" src/hello.sharp -L "$LIBRARY" -j 00
refused errlmt_huge      "above 100000" src/hello.sharp -L "$LIBRARY" -errlmt 99999999999999999999
refused target_unknown   "the platforms are \`alpha\`" src/hello.sharp -L "$LIBRARY" -target bad
exits_with empty_argument 1 "" src/hello.sharp -L "$LIBRARY"
# 15. the runtime hands an empty argument to the program as "" (it died with SIGSEGV before main)
printf 'mod main;\ndef main(args: string[]) { foreach(a in args) { print("[" + a + "]"); } println(""); }\n' > src/args.sharp
compile args.txt src/args.sharp -j 1 -L "$LIBRARY" -o args.exe
if [ "$("$SHARP" args.exe a "" b 2>&1 | head -1)" = "[a][][b]" ]; then pass; else fail "empty_program_argument" "the program did not get \`a\`, \`\`, \`b\`:" "$("$SHARP" args.exe a "" b 2>&1 | head -3)"; fi
# 16. -o naming a source file is refused and the source is untouched; a FOLDER named x.sharp is not a source
cp src/hello.sharp src/victim.sharp
exits_with output_is_source 1 src/victim.sharp -L "$LIBRARY" -o src/victim.sharp
grep -q "is one of the source files being compiled" output_is_source.txt && pass || fail "output_is_source" "the message does not say the output is a source:" "$(head -2 output_is_source.txt)"
cmp -s src/hello.sharp src/victim.sharp && pass || fail "output_is_source" "the source file was changed"
mkdir -p folder_source.sharp
refused folder_as_source "is a folder, not a source file" folder_source.sharp -L "$LIBRARY"
refused missing_source_hint "did you mean \`hello.sharp\`" src/helo.sharp -L "$LIBRARY"
# 17. -errlmt holds for syntax errors too
{ echo "mod app;"; for k in $(seq 1 30); do echo "def g$k() { x := 1 }"; done; } > src/many.sharp
compile errlmt_syntax.txt src/many.sharp -L "$LIBRARY" -errlmt 3 -c
if [ "$(grep -c '^error\[' errlmt_syntax.txt)" = 3 ] && grep -q "27 more error(s) not shown" errlmt_syntax.txt; then pass; else fail "errlmt_syntax" "-errlmt 3 did not limit the syntax errors to 3:" "$(grep -c '^error\[' errlmt_syntax.txt) shown"; fi
# 18. -ir and -analysis show the program's last file, not the library's
printf 'mod main;\ndef twice_it(x: _int64) : _int64 { return x * 2; }\ndef main() { println("" + twice_it(4)); }\n' > src/inspect.sharp
compile inspect_ir.txt src/inspect.sharp -L "$LIBRARY" -o inspect.exe -ir
grep -q "^ir of main#__srt_global.twice_it" inspect_ir.txt && pass || fail "ir_with_library" "-ir did not print the program's function"
compile inspect_analysis.txt src/inspect.sharp -L "$LIBRARY" -c -analysis
grep -q "^function main#twice_it" inspect_analysis.txt && pass || fail "analysis_with_library" "-analysis did not print the program's function"
# 19. -run: the exit code is the program's (main's result; 1 after an uncaught exception);
#     no runtime on the PATH is exit code 1 with the compiler's own message only
printf 'mod main;\ndef main() : int { return 3; }\n' > src/three.sharp
printf 'mod main;\ndef main() { throw new exception("boom"); }\n' > src/boom.sharp
PATH="$WORK/bin:$PATH" exits_with run_result 3 src/three.sharp -j 1 -L "$LIBRARY" -o three.exe -run
PATH="$WORK/bin:$PATH" exits_with run_uncaught 1 src/boom.sharp -j 1 -L "$LIBRARY" -o boom.exe -run
PATH="/usr/bin:/bin" exits_with run_no_runtime_code 1 src/hello.sharp -j 1 -L "$LIBRARY" -o run3.exe -run
grep -q "not found" run_no_runtime_code.txt && fail "run_no_runtime_code" "the shell's own message got through" || pass
# 20. projects: an empty settings.json stops the build; a setting given twice and an -o that
#     settings.json overrules are warnings; without the library: one E0962 and nothing else
: > demo/settings.json
exits_with settings_empty 1 -P demo -L "$LIBRARY"
grep -q "is empty, and an empty file is not JSON" settings_empty.txt && pass || fail "settings_empty" "no message about the empty file:" "$(head -2 settings_empty.txt)"
printf '{ "output": "a", "output": "b", "extern_lib": [ "%s" ] }\n' "$LIBRARY" > demo/settings.json
compile settings_twice.txt -P demo -o custom
if grep -q "the setting \`output\` is given more than once" settings_twice.txt && grep -q "\-o custom is overruled by settings.json" settings_twice.txt; then pass; else fail "settings_twice" "no warning for a setting given twice or for the overruled -o:" "$(grep warning settings_twice.txt | head -2)"; fi
printf '{ "output": "demo" }\n' > demo/settings.json
exits_with no_library 1 -P demo
if [ "$(grep -c '^error\[' no_library.txt)" = 1 ] && grep -q "error\[E0962\]" no_library.txt && grep -q "extern_lib" no_library.txt; then pass; else fail "no_library" "not exactly one E0962 naming extern_lib:" "$(grep '^error' no_library.txt | head -4)"; fi
# 21. -ignoredir takes away the only file of a module the program imports: the E0303 names the
#     file and says it was left out; a build that leaves nothing out says nothing of the kind
mkdir -p shop/model shop/app
printf 'mod model;
class person { name : string = "x"; }
' > shop/model/person.sharp
printf 'mod main;
import (model)
def main() { p := new person(); println(p.name); }
' > shop/app/main.sharp
exits_with left_out_module 1 shop/app/main.sharp -L shop/model -L "$LIBRARY" -ignoredir shop/model -c
if grep -q "model/person.sharp\` declares module \`model\`, but it was left out of this build" left_out_module.txt; then pass; else fail "left_out_module" "the E0303 does not say the module's file was left out:" "$(grep -m1 -A8 'E0303' left_out_module.txt | grep 'note' | head -2)"; fi
exits_with not_left_out 0 shop/app/main.sharp -L shop/model -L "$LIBRARY" -c
grep -q "left out of this build" not_left_out.txt && fail "not_left_out" "a build that leaves nothing out talks about files left out" || pass

echo "driver tests: $passed passed, $failed failed   (work folder: $WORK)"
[ "$failed" -eq 0 ]
