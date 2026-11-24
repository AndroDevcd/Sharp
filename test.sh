#!/bin/bash
# TEST SHARP: the one command for every test of the repository.
#
#     bash test.sh                      everything, the quick version                  about 3 minutes
#     bash test.sh full                 everything, all of it                          more than an hour
#     bash test.sh vm                   one part, quick          (several: bash test.sh compiler library)
#     bash test.sh vm full              one part, all of it
#     bash test.sh changed              the parts whose files changed since the last commit, quick
#     bash test.sh changed full         ... all of each
#     bash test.sh case lib_data_json   one case of the suite (tests/suite/cases), compiled and run
#     bash test.sh try app/main.sharp   any one file - or a project folder - compiled with what is in the
#                                       repository now, and run   (a folder: try shop -tests  builds its tests)
#                                       (what follows the file goes to the program: try a.sharp --port 80)
#     bash test.sh list                 the parts, what each one runs, how long it takes
#     bash test.sh help                 this text
#
# THE PARTS
#     compiler    reading and checking source, the error messages; full: code generation, self-hosting
#     library     the standard library compiles; full: every case of the suite, run four ways
#     vm          the suite run once, calls to C++; full: the suite four ways, the kind checks, collector stress, the ARM JIT under qemu, the Windows VM
#     debugger    sdb driven by its own tests; full: on Windows too
#     tools       the command line, the files the editor reads; full: the CLion plugin's tests
#     programs    the examples and the benchmarks still compile and run
#
# MORE WORDS, anywhere on the line
#     windows     a quick run does the Windows half too (a full run always does)
#     linux       a full run leaves the Windows half out
#     O           with `case`: compile the case with -O
#
# WHERE IT RUNS. From Git Bash on Windows: the Linux half runs inside WSL, the Windows half here.
# From Linux or WSL: the Linux half only, and the Windows half is listed as not run.
# Everything is built first, and only what changed is built again.
#
# THE LAST LINES say how it went, one line for each part:
#     compiler   PASS
#     vm         FAIL   see ...            and the command that runs just that part again
#     tools      not run
#     ALL TESTS: PASS    or    ALL TESTS: FAIL    (the script then exits 1)
#
# What the parts are made of: build-scripts/test.sh (the numbered tests of the Linux half), the
# scripts of build-scripts/windows, lib/debugger/tests, tools/clion-plugin. They can still be
# run alone; this script only chooses them, runs them in order and adds up the result.
REPO=$(cd "$(dirname "$0")" && pwd)
S="$REPO/build-scripts"
LOGS="$REPO/build/test"

PARTS="compiler library vm debugger tools programs"
# the numbered tests of build-scripts/test.sh behind each part (test 1, the compiler builds, always runs)
quick_of() { case $1 in compiler) echo "4 5";; library) echo "2";; vm) echo "8 15";; debugger) echo "16";; tools) echo "12 14";; programs) echo "11";; esac; }
full_of()  { case $1 in compiler) echo "4 5 6 7 13";; library) echo "2 3 8";; vm) echo "8 9 10 15 17 18";; debugger) echo "16";; tools) echo "12 14";; programs) echo "11";; esac; }
# the Windows half of a part (full runs, and quick runs with the word `windows`)
windows_of() { case $1 in vm) echo "the suite on the Windows VM";; debugger) echo "sdb on Windows";; tools) echo "the CLion plugin's tests";; esac; }

help() { sed -n '2,/^# What the parts are made of/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'; }
list() {
    echo "part        quick                                          full adds"
    echo "compiler    frontend, backend                    2 min     helps, code generation, self-hosting   35 min"
    echo "library     the library compiles                 1 min     every case compiles; the suite 4 ways  35 min"
    echo "vm          the suite once, calls to C++         2 min     the suite 4 ways, kind checks          35 min   + Windows: the suite, 10 min"
    echo "debugger    sdb's tests                          1 min                                                     + Windows: sdb's tests, 3 min"
    echo "tools       the command line, the editor's files 1 min                                                     + Windows: the plugin's tests, 2 min"
    echo "programs    examples and benchmarks              1 min"
    echo ""
    echo "Parts run side by side, so a run takes less than the sum.   bash test.sh help   says how to start them."
}

# ---- `try <file> [arguments]`: everything after the file belongs to the program, not to this script
TRY_FILE=""
if [ "$1" = try ]; then
    [ -n "$2" ] || { echo "test.sh try <file.sharp> [the program's arguments]   the file's path from the top of the repository"; exit 2; }
    TRY_FILE=$2; shift 2; TRY_ARGUMENTS=("$@"); set --
fi

# ---- the words of the command line
LEVEL=quick; CHOSEN=""; CHANGED=0; WINDOWS=""; CASES=""; CASE_OPTION=""; MODE=parts
for word in "$@"; do
    case "$word" in
        help|-h|--help) help; exit 0 ;;
        list)     list; exit 0 ;;
        quick)    LEVEL=quick ;;
        full)     LEVEL=full ;;
        changed)  CHANGED=1 ;;
        windows)  WINDOWS=yes ;;
        linux)    WINDOWS=no ;;
        case)     MODE=case ;;
        O|-O)     CASE_OPTION="-O" ;;
        compiler|library|vm|debugger|tools|programs) CHOSEN="$CHOSEN $word" ;;
        *)  if [ $MODE = case ]; then CASES="$CASES ${word%.sharp}"
            else echo "test.sh: unknown word '$word'"; echo ""; help; exit 2; fi ;;
    esac
done
[ -z "$WINDOWS" ] && { [ $LEVEL = full ] && WINDOWS=yes || WINDOWS=no; }

# ---- where this runs: Git Bash (both halves) or Linux / WSL (the Linux half)
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) ON_WINDOWS=1 ;; *) ON_WINDOWS=0 ;; esac
if [ $ON_WINDOWS = 1 ]; then
    IN_WSL=$(MSYS_NO_PATHCONV=1 wsl wslpath -a "$(cygpath -m "$S")" 2>/dev/null | tr -d '\r')
    [ -n "$IN_WSL" ] || { echo "WSL did not answer: the Linux half needs WSL (wsl --install), with g++, rsync and python3 in it"; exit 1; }
fi
# a script of build-scripts/, run on Linux
linux() { local script=$1; shift
    if [ $ON_WINDOWS = 1 ]; then MSYS_NO_PATHCONV=1 wsl bash "$IN_WSL/$script" "$@" < /dev/null
    else bash "$S/$script" "$@"; fi
}
quiet() { grep -v "ld.exe\|^time:\|^unknown"; }
mkdir -p "$LOGS"

# ---- one file, compiled with the compiler and library as they are now, and run
if [ -n "$TRY_FILE" ]; then
    linux internal/try_program.sh "$TRY_FILE" "${TRY_ARGUMENTS[@]}"
    exit $?
fi

# ---- one case of the suite
if [ $MODE = case ]; then
    [ -n "$CASES" ] || { echo "test.sh case <name> ...   the names are the files of tests/suite/cases, without .sharp"; exit 2; }
    for name in $CASES; do [ -f "$REPO/tests/suite/cases/$name.sharp" ] || { echo "no case tests/suite/cases/$name.sharp"; exit 2; }; done
    ok=1
    linux internal/run_case.sh $CASE_OPTION $CASES || ok=0
    if [ $WINDOWS = yes ]; then
        if [ $ON_WINDOWS = 0 ]; then echo "(the Windows VM: not run - start this from Git Bash on Windows)"
        else
            echo "== on the Windows VM"
            bash "$S/windows/build_vm.sh" 2>&1 | quiet | tail -1
            linux internal/hand_over_compiler.sh > /dev/null || { echo "the compiler could not be handed over to Windows"; exit 1; }
            VM="$REPO/cmake-build-windows-vm/sharp.exe"; LIBRARY="$REPO/lib/support/$(ls "$REPO/lib/support" | sort -V | tail -1)"
            for name in $CASES; do
                program="$LOGS/cases/$name"; mkdir -p "$LOGS/cases"; rm -f "$program"
                "$VM" "$LOGS/sharpc" "$REPO/tests/suite/cases/$name.sharp" "$REPO/tests/suite/testing.sharp" -o "$program" $CASE_OPTION -L "$LIBRARY" > "$program.compile.txt" 2>&1
                if [ ! -f "$program" ]; then grep -vE "^(time:|unknown)" "$program.compile.txt" | head -40; echo "$name: DID NOT COMPILE"; ok=0; continue; fi
                ( cd "$LOGS/cases" && timeout 150 "$VM" "$program" 2>&1 | tail -${TAIL:-15} )
                ( cd "$LOGS/cases" && timeout 150 "$VM" "$program" 2>&1 | grep -q -e "RESULT PASS" -e " tests passed" ) || ok=0
            done
        fi
    fi
    [ $ok = 1 ] && echo "CASES: PASS" || { echo "CASES: FAIL"; exit 1; }
    exit 0
fi

# ---- `changed`: the parts whose files changed since the last commit
if [ $CHANGED = 1 ]; then
    files=$(git -C "$REPO" status --porcelain 2>/dev/null | cut -c4- | tr -d '"')
    add() { case " $CHOSEN " in *" $1 "*) ;; *) CHOSEN="$CHOSEN $1" ;; esac; }
    while IFS= read -r file; do
        case "$file" in
            lib/runtime/*|lib/core/*|main.cpp|stdimports.h|CMakeLists.txt) add vm; add debugger ;;      # (sdb is the VM)
            lib/debugger/*)             add debugger ;;
            lib/compiler/backend/ide/*) add compiler; add tools ;;
            lib/compiler/*)             add compiler; add library ;;
            lib/support/*|tests/suite/*) add library ;;
            tools/clion-plugin/*)       add tools ;;
            examples/*|benchmark/*)     add programs ;;
        esac
    done <<< "$files"
    if [ -z "$CHOSEN" ]; then
        echo "nothing that a test covers changed since the last commit (git status).   bash test.sh   runs everything."
        exit 0
    fi
    echo "changed since the last commit:$CHOSEN"
fi
[ -n "$CHOSEN" ] || CHOSEN=" $PARTS"

# ---- the Linux half: the numbered tests the chosen parts need, in one run of build-scripts/test.sh
NUMBERS=""; SUITE_ONCE=yes
for part in $CHOSEN; do
    [ $LEVEL = full ] && numbers=$(full_of $part) || numbers=$(quick_of $part)
    for n in $numbers; do
        case " $NUMBERS " in *" $n "*) ;; *) NUMBERS="$NUMBERS $n" ;; esac
        # (the suite is run once, plainly, for a quick look at the VM; a full part wants it four ways)
        [ $n = 8 ] && [ $LEVEL = full ] && SUITE_ONCE=no
    done
done
NUMBERS=$(echo $NUMBERS | tr ' ' '\n' | sort -n | tr '\n' ' ')
began=$SECONDS
echo "testing:$CHOSEN   ($LEVEL$([ $WINDOWS = yes ] && echo ", with the Windows half"))"
echo ""
echo "======== Linux: build, then tests $NUMBERS"
linux internal/test_parts.sh $SUITE_ONCE $NUMBERS 2>&1 | tee "$LOGS/linux.txt"
linux_said=$(grep -a "^TESTS: " "$LOGS/linux.txt" | tail -1)

# ---- the Windows half
declare -A windows_result
windows_step() {    # windows_step <part> <log name> <the line that says it passed> <command...>
    local part=$1 log="$LOGS/$2.txt" pass=$3; shift 3
    echo ""
    echo "======== Windows: $(windows_of $part)"
    "$@" 2>&1 | quiet | tee "$log" | tail -12
    grep -aq "$pass" "$log" && windows_result[$part]=PASS || windows_result[$part]="FAIL   see $log"
}
if [ $WINDOWS = yes ]; then
    for part in $CHOSEN; do
        [ -n "$(windows_of $part)" ] || continue
        if [ $ON_WINDOWS = 0 ]; then windows_result[$part]="not run (start this from Git Bash on Windows)"; continue; fi
        linux internal/hand_over_compiler.sh > /dev/null
        case $part in
            vm)       bash "$S/windows/build_vm.sh" 2>&1 | quiet | tail -1
                      windows_step vm windows_suite "WINDOWS SUITE: PASS" env COMPILER="$LOGS/sharpc" bash "$S/windows/suite.sh" ;;
            debugger) bash "$S/windows/build_sdb.sh" 2>&1 | quiet | tail -1
                      windows_step debugger windows_sdb "WINDOWS SDB TESTS: PASS" env COMPILER="$LOGS/sharpc" bash "$S/windows/sdb_tests.sh" ;;
            tools)    windows_step tools plugin "^built: " bash "$S/windows/build_plugin.sh" test ;;
        esac
    done
fi

# ---- the result, part by part
echo ""
echo "======== RESULT   ($(( (SECONDS - began) / 60 )) min $(( (SECONDS - began) % 60 )) s)"
all=1
for part in $PARTS; do
    case " $CHOSEN " in *" $part "*) ;; *) printf "%-10s not run\n" $part; continue ;; esac
    [ $LEVEL = full ] && numbers=$(full_of $part) || numbers=$(quick_of $part)
    result=PASS
    case "$linux_said" in
        "TESTS: PASS"*) ;;
        *)  failed=""
            for n in $numbers; do grep -aq "TEST $n FAILED" "$LOGS/linux.txt" && failed="$failed $n"; done
            # (no test named: the build itself failed, and nothing after it ran)
            grep -aq "TEST [0-9]* FAILED" "$LOGS/linux.txt" || failed=" (the build)"
            [ -n "$failed" ] && result="FAIL   Linux, test$failed: see $LOGS/linux.txt" ;;
    esac
    if [ -n "${windows_result[$part]}" ]; then
        case "${windows_result[$part]}" in
            PASS) [ "$result" = PASS ] && result="PASS   (Linux and Windows)" ;;
            FAIL*) [ "$result" = PASS ] && result="FAIL   Windows: ${windows_result[$part]#FAIL   }" || result="$result;  Windows: ${windows_result[$part]#FAIL   }" ;;
            *) result="$result   Windows half: ${windows_result[$part]}" ;;
        esac
    elif [ -n "$(windows_of $part)" ] && [ "$result" = PASS ]; then
        result="PASS   (Linux only)"
    fi
    printf "%-10s %s\n" $part "$result"
    case "$result" in FAIL*) all=0; echo "           again:  bash test.sh $part$([ $LEVEL = full ] && echo " full")     one case of the suite:  bash test.sh case <name>" ;; esac
done
[ $LEVEL = quick ] && echo "(the quick version: bash test.sh full runs all of each part)"
if [ $all = 1 ]; then echo "ALL TESTS: PASS"; else echo "ALL TESTS: FAIL"; exit 1; fi
