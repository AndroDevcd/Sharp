/*
 * binary-trees - the Benchmarks Game.  Makes and throws away a great many
 * complete binary trees, one node an allocation: a test of the allocator (or
 * the garbage collector).
 *
 *      java binarytrees <depth>
 */
public class binarytrees {
    static class Node {
        Node left, right;
    }

    static Node make(int depth) {
        Node n = new Node();
        if (depth > 0) {
            n.left = make(depth - 1);
            n.right = make(depth - 1);
        }
        return n;
    }

    /* how many nodes the tree has */
    static int check(Node n) {
        return n.left == null ? 1 : 1 + check(n.left) + check(n.right);
    }

    public static void main(String[] args) {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 10;
        int minDepth = 4;
        int maxDepth = n > minDepth + 2 ? n : minDepth + 2;
        int stretchDepth = maxDepth + 1;

        Node stretch = make(stretchDepth);
        System.out.print("stretch tree of depth " + stretchDepth + "\t check: " + check(stretch) + "\n");
        stretch = null;

        Node longLived = make(maxDepth);

        for (int depth = minDepth; depth <= maxDepth; depth += 2) {
            int iterations = 1 << (maxDepth - depth + minDepth);
            int total = 0;
            for (int i = 0; i < iterations; i++) {
                Node tree = make(depth);
                total += check(tree);
            }
            System.out.print(iterations + "\t trees of depth " + depth + "\t check: " + total + "\n");
        }

        System.out.print("long lived tree of depth " + maxDepth + "\t check: " + check(longLived) + "\n");
    }
}
