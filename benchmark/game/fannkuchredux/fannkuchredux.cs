/*
 * fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
 * first k numbers, where k is the first number, until the first is 1. Prints
 * a checksum of the flip counts and the most flips any permutation needed.
 *
 *      mono fannkuchredux.exe <n>
 */
using System;

static class FannkuchRedux {
    static void Main(string[] args) {
        int n = args.Length > 0 ? int.Parse(args[0]) : 7;
        int[] perm = new int[32], perm1 = new int[32], count = new int[32];
        int maxFlips = 0, checksum = 0, permutations = 0, r = n;

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
            if (flips > maxFlips) maxFlips = flips;
            checksum += permutations % 2 == 0 ? flips : -flips;

            /* the next permutation */
            for (;;) {
                if (r == n) {
                    Console.Write(checksum + "\nPfannkuchen(" + n + ") = " + maxFlips + "\n");
                    return;
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
}
