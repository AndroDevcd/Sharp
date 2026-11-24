#!/bin/bash
# The suite's programs, run with the garbage collector collecting far more often than it would:
# after every $EVERY new objects, three minor collections and then a full one (SHARP_GC=stress,
# lib/runtime/memory/tracing_collector.h, TESTING IT). An object the collector does not find -
# a root nobody told it about, a store without a write barrier - is freed while it is in use,
# and under stress that shows within moments instead of once in a thousand runs.
# A PART OF test.sh (test 18 of build-scripts/test.sh).   (Linux / WSL)
#
#     internal/gc_stress.sh <folder of executables> ...
#         the suite runner's work folders: out/suite_runner/plain, out/suite_runner/O
#
#     EVERY=<n>       collect after every n new objects (default 40)
#     VM_OPTIONS=     options for the VM (-nojit)
#
# Every program must still print RESULT PASS. Prints the ones that did not and a summary;
# exits 1 when there was one.
source "$(dirname "$0")/common.sh"
EVERY=${EVERY:-40}
[ -n "$1" ] || fail "gc_stress.sh <folder of executables> ..."
export VM EVERY VM_OPTIONS

stress_one() {
    exe=$1
    # (in its own folder, as the suite runner runs it: the cases write files beside themselves)
    out=$(cd "$(dirname "$exe")" && SHARP_GC=stress=$EVERY timeout -s KILL 600 prlimit --core=0 "$VM" $VM_OPTIONS "$exe" 2>&1)
    if echo "$out" | grep -a -q -e "RESULT PASS" -e " tests passed"; then echo "PASSED $exe"
    else echo "NOT-PASSED $exe: $(echo "$out" | grep -a -v '^$' | tail -2 | tr '\n' '|' | cut -c1-160)"; fi
}
export -f stress_one

ok=1
for folder in "$@"; do
    results=$(find "$folder" -maxdepth 1 -type f ! -name "*.*" | sort | xargs -P ${TEST_JOBS:-4} -I{} bash -c 'stress_one {}')
    echo "$results" | grep '^NOT-PASSED' | sed "s|$folder/||" | head -12
    passed=$(echo "$results" | grep -c '^PASSED')
    not_passed=$(echo "$results" | grep -c '^NOT-PASSED')
    echo "collector stress, $(basename "$folder") (a collection every $EVERY objects): $passed passed, $not_passed did not"
    [ "$not_passed" = 0 ] && [ "$passed" -gt 0 ] || ok=0
done
[ $ok = 1 ]
