#!/bin/bash
# The suite's program, run with the garbage collector collecting far more often than it would:
# after every $EVERY new objects, three minor collections and then a full one (SHARP_GC=stress,
# runtime/memory/tracing_collector.h, TESTING IT). An object the collector does not find -
# a root nobody told it about, a store without a write barrier - is freed while it is in use,
# and under stress that shows within moments instead of once in a thousand runs.
# A PART OF test.sh (test 18 of build-scripts/test.sh).   (Linux / WSL)
#
#     internal/gc_stress.sh <folder> ...
#         the folders test 8 built the suite into: out/suite_runner/plain, out/suite_runner/O
#         (each holds the program, `suite`)
#
#     EVERY=<n>       collect after every n new objects (default 40)
#     VM_OPTIONS=     options for the VM (-nojit)
#
# The program must still pass: exit code 0 and `all N tests passed` (compiler/tests/suite_program.sh;
# a run that dies without its summary is run again one top-level group at a time, which names the
# group). It runs in the suite's folder, one run at a time, and is killed after 3600 s. Prints a
# line for each folder, the failed tests below it; exits 1 when one did not pass.
# What each run printed: <folder>/gc_stress.txt (gc_stress-nojit.txt with -nojit).
source "$(dirname "$0")/common.sh"
source "$REPO/compiler/tests/suite_program.sh"
EVERY=${EVERY:-40}
[ -n "$1" ] || fail "gc_stress.sh <folder> ..."

ok=1
for folder in "$@"; do
    [ -f "$folder/suite" ] || { echo "collector stress, $(basename "$folder"): no program at $folder/suite - run test 8 first"; ok=0; continue; }
    log="$folder/gc_stress$(echo $VM_OPTIONS | tr -d ' ').txt"; rm -f "${log%.txt}".*
    suite_way "collector stress, $(basename "$folder") (a collection every $EVERY objects)" "$WORK/suite" "$log" 3600 \
        env SHARP_GC=stress=$EVERY prlimit --core=0 "$VM" $VM_OPTIONS "$folder/suite" || ok=0
done
[ $ok = 1 ]
