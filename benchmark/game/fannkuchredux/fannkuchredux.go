// fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
// first k numbers, where k is the first number, until the first is 1. Prints
// a checksum of the flip counts and the most flips any permutation needed.
//
//	fannkuchredux <n>
package main

import (
	"fmt"
	"os"
	"strconv"
)

func main() {
	n := 7
	if len(os.Args) > 1 {
		n, _ = strconv.Atoi(os.Args[1])
	}
	var perm, perm1, count [32]int
	maxFlips, checksum, permutations, r := 0, 0, 0, n

	for i := 0; i < n; i++ {
		perm1[i] = i
	}

	for {
		for r != 1 {
			count[r-1] = r
			r--
		}

		for i := 0; i < n; i++ {
			perm[i] = perm1[i]
		}
		flips := 0
		for k := perm[0]; k != 0; k = perm[0] {
			for i, j := 0, k; i < j; i, j = i+1, j-1 {
				perm[i], perm[j] = perm[j], perm[i]
			}
			flips++
		}
		if flips > maxFlips {
			maxFlips = flips
		}
		if permutations%2 == 0 {
			checksum += flips
		} else {
			checksum -= flips
		}

		// the next permutation
		for {
			if r == n {
				fmt.Printf("%d\nPfannkuchen(%d) = %d\n", checksum, n, maxFlips)
				return
			}
			first := perm1[0]
			for i := 0; i < r; i++ {
				perm1[i] = perm1[i+1]
			}
			perm1[r] = first
			count[r]--
			if count[r] > 0 {
				break
			}
			r++
		}
		permutations++
	}
}
