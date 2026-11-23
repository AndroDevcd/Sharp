/*
 * fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
 * first k numbers, where k is the first number, until the first is 1. Prints
 * a checksum of the flip counts and the most flips any permutation needed.
 *
 *      node fannkuchredux.js <n>
 */
'use strict';

function fannkuch(n) {
    const perm = new Int32Array(32), perm1 = new Int32Array(32), count = new Int32Array(32);
    let maxFlips = 0, checksum = 0, permutations = 0, r = n;

    for (let i = 0; i < n; i++) perm1[i] = i;

    for (;;) {
        while (r !== 1) { count[r - 1] = r; r--; }

        for (let i = 0; i < n; i++) perm[i] = perm1[i];
        let flips = 0, k;
        while ((k = perm[0]) !== 0) {
            for (let i = 0, j = k; i < j; i++, j--) {
                const t = perm[i]; perm[i] = perm[j]; perm[j] = t;
            }
            flips++;
        }
        if (flips > maxFlips) maxFlips = flips;
        checksum += permutations % 2 === 0 ? flips : -flips;

        /* the next permutation */
        for (;;) {
            if (r === n) {
                console.log(checksum + "\nPfannkuchen(" + n + ") = " + maxFlips);
                return;
            }
            const first = perm1[0];
            for (let i = 0; i < r; i++) perm1[i] = perm1[i + 1];
            perm1[r] = first;
            count[r]--;
            if (count[r] > 0) break;
            r++;
        }
        permutations++;
    }
}

fannkuch(process.argv.length > 2 ? parseInt(process.argv[2], 10) : 7);
