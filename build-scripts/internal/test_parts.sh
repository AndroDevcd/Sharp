#!/bin/bash
# The Linux half of the repository's test.sh: builds the VM and the compiler, then runs the
# numbered tests it is handed.   A PART OF test.sh (the one at the top of the repository).   (Linux / WSL)
#
#     internal/test_parts.sh <yes|no> <test numbers...>
#
# yes: the suite (test 8) is run once, plainly - a quick look. no: four ways, as build-scripts/test.sh does.
HERE=$(cd "$(dirname "$0")/.." && pwd)
suite_once=$1; shift
bash "$HERE/build.sh" || { echo "TESTS: FAIL (the build failed)"; exit 1; }
[ "$suite_once" = yes ] && export WAYS=plain
bash "$HERE/test.sh" only "$@"
