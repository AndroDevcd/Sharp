// binary-trees - the Benchmarks Game.  Makes and throws away a great many
// complete binary trees, one node an allocation: a test of the allocator (or
// the garbage collector).
//
//	binarytrees <depth>
package main

import (
	"fmt"
	"os"
	"strconv"
)

type node struct {
	left, right *node
}

func makeTree(depth int) *node {
	n := new(node)
	if depth > 0 {
		n.left = makeTree(depth - 1)
		n.right = makeTree(depth - 1)
	}
	return n
}

// how many nodes the tree has
func check(n *node) int {
	if n.left == nil {
		return 1
	}
	return 1 + check(n.left) + check(n.right)
}

func main() {
	n := 10
	if len(os.Args) > 1 {
		n, _ = strconv.Atoi(os.Args[1])
	}
	minDepth := 4
	maxDepth := minDepth + 2
	if n > maxDepth {
		maxDepth = n
	}
	stretchDepth := maxDepth + 1

	stretch := makeTree(stretchDepth)
	fmt.Printf("stretch tree of depth %d\t check: %d\n", stretchDepth, check(stretch))
	stretch = nil

	longLived := makeTree(maxDepth)

	for depth := minDepth; depth <= maxDepth; depth += 2 {
		iterations := 1 << (maxDepth - depth + minDepth)
		total := 0
		for i := 0; i < iterations; i++ {
			tree := makeTree(depth)
			total += check(tree)
		}
		fmt.Printf("%d\t trees of depth %d\t check: %d\n", iterations, depth, total)
	}

	fmt.Printf("long lived tree of depth %d\t check: %d\n", maxDepth, check(longLived))
}
