#!/bin/bash
# THE SUITE'S ONE PROGRAM, for the scripts that build and run it. Not run on its own - they read it
# (`source .../compiler/tests/suite_program.sh`): tests/suite_runner.sh, kindcheck.sh, codegen.sh,
# selfhost.sh, build-scripts/internal/gc_stress.sh, arm_suite.sh, release_compilers.sh,
# build-scripts/windows/suite.sh and the test.sh at the top of the repository.
#
# The suite (tests/suite, tests/suite/README.md) is ONE project: built with
#
#       sharp <compiler> -P <the suite's folder> -j 1 -L <library> [-O]
#
# into <the suite's folder>/build/outputs/suite, and run with the project's folder as the current
# one. It prints a line for every test, the full names of the failed ones under `failed:`, and last
#
#       all N tests passed                      or      K failed, P passed (N tests)
#
# and its exit code is 0 only when everything passed.
#
#     suite_build <project> <copy to> <log> <the compile command...>
#         deletes the project's build/ first (no cache of an earlier build is read), runs the command
#         (the whole compile, -P <project> included), and copies the program to <copy to>, where it is
#         kept for the tests that run it later. When no program was written it says why and returns 1.
#
#     suite_way <label> <project> <log> <seconds> <the run command...>
#         runs the program (the command ends with the program's path) in the project's folder and
#         prints one line, "<label>: PASS   all N tests passed" or "<label>: FAIL ..." with the failed
#         tests below it. A way PASSES when the exit code is 0 AND the summary says all passed.
#         A RUN THAT ENDS WITHOUT A SUMMARY (the VM crashed, or the timeout killed it) has lost
#         everything after the crash, so it is run again once for each top-level group
#         (-test-only suite.language, ...): the report names the group that crashes and still gives
#         the results of the others. Everything a run printed: <log>; a group's: <log>.<group>.txt.
#
#     SUITE_PICK="<group> ..."    run only these groups (-test-only each); the runs again keep to them
#
#     suite_groups <project>                  the top-level groups, as main.sharp calls them
#     suite_without_self_starters <project>   the groups to name to leave out every group whose file
#                                             starts the program once more (run_isolated, program_path(),
#                                             runtime_path()); prints the left-out ones on stderr

SUITE_OUTPUT=suite      # settings.json's "output"

suite_build() {
    local project=$1 copy_to=$2 log=$3; shift 3
    rm -rf "$project/build" "$copy_to"
    timeout -s KILL "${SUITE_BUILD_SECONDS:-1200}" "$@" > "$log" 2>&1 < /dev/null
    if [ ! -f "$project/build/outputs/$SUITE_OUTPUT" ]; then
        echo "the suite did not compile: $(grep -a -c '^error' "$log") error(s)   (what the compiler printed: $log)"
        grep -a -A4 "^error" "$log" | head -15 | cut -c1-200
        grep -a -v '^$' "$log" | tail -2 | cut -c1-200
        return 1
    fi
    mkdir -p "$(dirname "$copy_to")"
    cp "$project/build/outputs/$SUITE_OUTPUT" "$copy_to"
}

# (read from the calls in main's tests("suite") { ... }: `language_tests();` is the group suite.language)
suite_groups() {
    sed -n '/tests("suite")/,/^[[:space:]]*};/p' "$1/main.sharp" | grep -o -E '^[[:space:]]+[A-Za-z0-9_]+_tests\(\);' \
        | tr -d ' \t\r' | sed 's/^\(.*\)_tests();$/suite.\1/'
}

# the last line of a run's report ("" when it has none: the run died before it)
suite_summary() {
    grep -a -E "^(all [0-9]+ tests passed|[0-9]+ failed, [0-9]+ passed|no tests were run)" "$1" | tail -1
}

# the names under `failed:` (at most 20), and the -test-only names nothing answered to
suite_failures() {
    local log=$1 room=${2:-}
    local names; names=$(sed -n '/^failed:/,/^$/p' "$log" | grep -a '^  ')
    echo "$names" | grep -v '^$' | head -20 | sed "s/^/$room/"
    [ "$(echo "$names" | grep -c .)" -gt 20 ] && echo "$room  ... and $(( $(echo "$names" | grep -c .) - 20 )) more (all of them: $log)"
    sed -n '/^-test-only: no such group or test:/,/^$/p' "$log" | grep -v '^$' | sed "s/^/$room/"
}

# how a run without a summary ended
suite_ending() {
    case $1 in
        137) echo "killed after $2 s: it hung, or was too slow" ;;
        *)   echo "exit code $1" ;;
    esac
}

suite_way() {
    local label=$1 project=$2 log=$3 seconds=$4; shift 4
    local picked=() group name code summary one
    for name in $SUITE_PICK; do picked+=(-test-only "$name"); done
    ( cd "$project" && timeout -s KILL "$seconds" "$@" "${picked[@]}" ) > "$log" 2>&1 < /dev/null; code=$?
    summary=$(suite_summary "$log")
    if [ "$code" = 0 ] && [[ "$summary" == "all "* ]]; then echo "$label: PASS   $summary"; return 0; fi
    if [ -n "$summary" ]; then
        echo "$label: FAIL   $summary   (exit code $code; everything it printed: $log)"
        suite_failures "$log" "   "
        return 1
    fi
    echo "$label: FAIL   it ended without a summary ($(suite_ending $code "$seconds")); its last lines: $(grep -a -v '^$' "$log" | tail -3 | cut -c1-160 | tr '\n' '|')"
    echo "   once more, one top-level group at a time:"
    for group in $(suite_groups "$project"); do
        picked=()
        if [ -z "$SUITE_PICK" ]; then picked=(-test-only "$group")
        else for name in $SUITE_PICK; do case "$name" in "$group"|"$group".*) picked+=(-test-only "$name") ;; esac; done; fi
        [ ${#picked[@]} -gt 0 ] || continue
        one="${log%.txt}.$group.txt"
        ( cd "$project" && timeout -s KILL "$seconds" "$@" "${picked[@]}" ) > "$one" 2>&1 < /dev/null; code=$?
        summary=$(suite_summary "$one")
        if [ "$code" = 0 ] && [[ "$summary" == "all "* ]]; then echo "   $group: $summary"
        elif [ -n "$summary" ]; then echo "   $group: FAIL   $summary   (exit code $code: $one)"; suite_failures "$one" "      "
        else echo "   $group: ENDED WITHOUT A SUMMARY ($(suite_ending $code "$seconds")) - the crash is in this group; its last lines: $(grep -a -v '^$' "$one" | tail -3 | cut -c1-160 | tr '\n' '|')"; fi
    done
    return 1
}

# does this file start the program once more?
suite_starts_itself() {
    grep -q -E 'run_isolated\(|program_path\(\)|runtime_path\(\)' "$1"
}

# the fewest groups that cover everything but the files that start the program once more: a folder
# none of whose files does is named whole, else its files and folders one by one
suite_without_self_starters() {
    local project=$1 group
    for group in $(suite_groups "$project"); do suite_cover "$project" "$group"; done
}
suite_cover() {
    local project=$1 group=$2 path entry
    path="$project/$(echo "${group#suite.}" | tr . /)"
    if [ -f "$path.sharp" ]; then
        if suite_starts_itself "$path.sharp"; then echo "left out: $group" >&2; else echo "$group"; fi
        return
    fi
    [ -d "$path" ] || return
    if ! grep -r -q -l -E 'run_isolated\(|program_path\(\)|runtime_path\(\)' --include='*.sharp' "$path"; then echo "$group"; return; fi
    for entry in $(ls "$path" | sort); do
        case "$entry" in
            entry.sharp) suite_starts_itself "$path/entry.sharp" && echo "left out: the tests written in $group's entry.sharp" >&2 ;;
            *.sharp) suite_cover "$project" "$group.${entry%.sharp}" ;;
            *) [ -d "$path/$entry" ] && suite_cover "$project" "$group.$entry" ;;
        esac
    done
}
