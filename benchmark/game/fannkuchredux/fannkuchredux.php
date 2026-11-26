<?php
/*
 * fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
 * first k numbers, where k is the first number, until the first is 1. Prints
 * a checksum of the flip counts and the most flips any permutation needed.
 *
 *      php fannkuchredux.php <n>
 */

function fannkuch($n) {
    $perm = array_fill(0, $n, 0);
    $perm1 = [];
    $count = array_fill(0, $n, 0);
    $max_flips = 0; $checksum = 0; $permutations = 0; $r = $n;

    for ($i = 0; $i < $n; $i++) $perm1[$i] = $i;

    for (;;) {
        while ($r != 1) { $count[$r - 1] = $r; $r--; }

        for ($i = 0; $i < $n; $i++) $perm[$i] = $perm1[$i];
        $flips = 0;
        while (($k = $perm[0]) != 0) {
            for ($i = 0, $j = $k; $i < $j; $i++, $j--) {
                $t = $perm[$i]; $perm[$i] = $perm[$j]; $perm[$j] = $t;
            }
            $flips++;
        }
        if ($flips > $max_flips) $max_flips = $flips;
        $checksum += $permutations % 2 == 0 ? $flips : -$flips;

        /* the next permutation */
        for (;;) {
            if ($r == $n) {
                printf("%d\nPfannkuchen(%d) = %d\n", $checksum, $n, $max_flips);
                return;
            }
            $first = $perm1[0];
            for ($i = 0; $i < $r; $i++) $perm1[$i] = $perm1[$i + 1];
            $perm1[$r] = $first;
            $count[$r]--;
            if ($count[$r] > 0) break;
            $r++;
        }
        $permutations++;
    }
}

fannkuch($argc > 1 ? (int) $argv[1] : 7);
