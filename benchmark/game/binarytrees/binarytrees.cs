/*
 * binary-trees - the Benchmarks Game.  Makes and throws away a great many
 * complete binary trees, one node an allocation: a test of the allocator (or
 * the garbage collector).
 *
 *      mono binarytrees.exe <depth>
 */
using System;

class Node {
    public Node Left, Right;
}

static class BinaryTrees {
    static Node Make(int depth) {
        Node n = new Node();
        if (depth > 0) {
            n.Left = Make(depth - 1);
            n.Right = Make(depth - 1);
        }
        return n;
    }

    /* how many nodes the tree has */
    static int Check(Node n) {
        return n.Left == null ? 1 : 1 + Check(n.Left) + Check(n.Right);
    }

    static void Main(string[] args) {
        int n = args.Length > 0 ? int.Parse(args[0]) : 10;
        int minDepth = 4;
        int maxDepth = n > minDepth + 2 ? n : minDepth + 2;
        int stretchDepth = maxDepth + 1;

        Node stretch = Make(stretchDepth);
        Console.Write("stretch tree of depth " + stretchDepth + "\t check: " + Check(stretch) + "\n");
        stretch = null;

        Node longLived = Make(maxDepth);

        for (int depth = minDepth; depth <= maxDepth; depth += 2) {
            int iterations = 1 << (maxDepth - depth + minDepth);
            int total = 0;
            for (int i = 0; i < iterations; i++) {
                Node tree = Make(depth);
                total += Check(tree);
            }
            Console.Write(iterations + "\t trees of depth " + depth + "\t check: " + total + "\n");
        }

        Console.Write("long lived tree of depth " + maxDepth + "\t check: " + Check(longLived) + "\n");
    }
}
