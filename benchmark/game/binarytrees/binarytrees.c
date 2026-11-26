/*
 * binary-trees - the Benchmarks Game.  Makes and throws away a great many
 * complete binary trees, one node an allocation: a test of the allocator (or
 * the garbage collector).
 *
 *      binarytrees <depth>
 */
#include <stdio.h>
#include <stdlib.h>

struct node { struct node *left, *right; };

static struct node *make(int depth) {
    struct node *n = malloc(sizeof(struct node));
    if (depth > 0) {
        n->left = make(depth - 1);
        n->right = make(depth - 1);
    } else {
        n->left = n->right = NULL;
    }
    return n;
}

/* how many nodes the tree has */
static int check(const struct node *n) {
    return n->left == NULL ? 1 : 1 + check(n->left) + check(n->right);
}

static void discard(struct node *n) {
    if (n->left != NULL) { discard(n->left); discard(n->right); }
    free(n);
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 10;
    int min_depth = 4;
    int max_depth = n > min_depth + 2 ? n : min_depth + 2;
    int stretch_depth = max_depth + 1;

    struct node *stretch = make(stretch_depth);
    printf("stretch tree of depth %d\t check: %d\n", stretch_depth, check(stretch));
    discard(stretch);

    struct node *long_lived = make(max_depth);

    for (int depth = min_depth; depth <= max_depth; depth += 2) {
        int iterations = 1 << (max_depth - depth + min_depth);
        int total = 0;
        for (int i = 0; i < iterations; i++) {
            struct node *tree = make(depth);
            total += check(tree);
            discard(tree);
        }
        printf("%d\t trees of depth %d\t check: %d\n", iterations, depth, total);
    }

    printf("long lived tree of depth %d\t check: %d\n", max_depth, check(long_lived));
    return 0;
}
