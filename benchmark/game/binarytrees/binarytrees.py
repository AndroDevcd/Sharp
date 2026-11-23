# binary-trees - the Benchmarks Game.  Makes and throws away a great many
# complete binary trees, one node an object: a test of the garbage collector.
#
#      python3 binarytrees.py <depth>

import sys


class Node:
    __slots__ = ("left", "right")

    def __init__(self, left, right):
        self.left = left
        self.right = right


def make(depth):
    if depth > 0:
        left = make(depth - 1)
        right = make(depth - 1)
        return Node(left, right)
    return Node(None, None)


def check(n):
    """how many nodes the tree has"""
    if n.left is None:
        return 1
    return 1 + check(n.left) + check(n.right)


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    min_depth = 4
    max_depth = n if n > min_depth + 2 else min_depth + 2
    stretch_depth = max_depth + 1

    print("stretch tree of depth %d\t check: %d" % (stretch_depth, check(make(stretch_depth))))

    long_lived = make(max_depth)

    for depth in range(min_depth, max_depth + 1, 2):
        iterations = 1 << (max_depth - depth + min_depth)
        total = 0
        for _ in range(iterations):
            total += check(make(depth))
        print("%d\t trees of depth %d\t check: %d" % (iterations, depth, total))

    print("long lived tree of depth %d\t check: %d" % (max_depth, check(long_lived)))


main()
