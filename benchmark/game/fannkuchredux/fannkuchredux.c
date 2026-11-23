/*
 * fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
 * first k numbers, where k is the first number, until the first is 1. Prints
 * a checksum of the flip counts and the most flips any permutation needed.
 *
 *      fannkuchredux <n>
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 7;
    int perm[32], perm1[32], count[32];
    int max_flips = 0, checksum = 0, permutations = 0, r = n;

    for (int i = 0; i < n; i++) perm1[i] = i;

    for (;;) {
        while (r != 1) { count[r - 1] = r; r--; }

        for (int i = 0; i < n; i++) perm[i] = perm1[i];
        int flips = 0, k;
        while ((k = perm[0]) != 0) {
            for (int i = 0, j = k; i < j; i++, j--) {
                int t = perm[i]; perm[i] = perm[j]; perm[j] = t;
            }
            flips++;
        }
        if (flips > max_flips) max_flips = flips;
        checksum += permutations % 2 == 0 ? flips : -flips;

        /* the next permutation */
        for (;;) {
            if (r == n) {
                printf("%d\nPfannkuchen(%d) = %d\n", checksum, n, max_flips);
                return 0;
            }
            int first = perm1[0];
            for (int i = 0; i < r; i++) perm1[i] = perm1[i + 1];
            perm1[r] = first;
            count[r]--;
            if (count[r] > 0) break;
            r++;
        }
        permutations++;
    }
}
