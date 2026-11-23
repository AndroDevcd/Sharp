# fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
# first k numbers, where k is the first number, until the first is 1. Prints
# a checksum of the flip counts and the most flips any permutation needed.
#
#      python3 fannkuchredux.py <n>

import sys


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 7
    perm = [0] * 32
    perm1 = [0] * 32
    count = [0] * 32
    max_flips = 0
    checksum = 0
    permutations = 0
    r = n

    for i in range(n):
        perm1[i] = i

    while True:
        while r != 1:
            count[r - 1] = r
            r -= 1

        for i in range(n):
            perm[i] = perm1[i]
        flips = 0
        k = perm[0]
        while k != 0:
            i = 0
            j = k
            while i < j:
                t = perm[i]
                perm[i] = perm[j]
                perm[j] = t
                i += 1
                j -= 1
            flips += 1
            k = perm[0]
        if flips > max_flips:
            max_flips = flips
        if permutations % 2 == 0:
            checksum += flips
        else:
            checksum -= flips

        # the next permutation
        while True:
            if r == n:
                print("%d\nPfannkuchen(%d) = %d" % (checksum, n, max_flips))
                return
            first = perm1[0]
            for i in range(r):
                perm1[i] = perm1[i + 1]
            perm1[r] = first
            count[r] -= 1
            if count[r] > 0:
                break
            r += 1
        permutations += 1


main()
