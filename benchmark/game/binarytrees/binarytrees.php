<?php
/*
 * binary-trees - the Benchmarks Game.  Makes and throws away a great many
 * complete binary trees, one node an object: a test of the memory manager.
 *
 *      php binarytrees.php <depth>
 */

class Node {
    public $left = null;
    public $right = null;
}

function make($depth) {
    $n = new Node();
    if ($depth > 0) {
        $n->left = make($depth - 1);
        $n->right = make($depth - 1);
    }
    return $n;
}

/* how many nodes the tree has */
function check($n) {
    return $n->left === null ? 1 : 1 + check($n->left) + check($n->right);
}

$n = $argc > 1 ? (int) $argv[1] : 10;
$min_depth = 4;
$max_depth = $n > $min_depth + 2 ? $n : $min_depth + 2;
$stretch_depth = $max_depth + 1;

$stretch = make($stretch_depth);
printf("stretch tree of depth %d\t check: %d\n", $stretch_depth, check($stretch));
unset($stretch);

$long_lived = make($max_depth);

for ($depth = $min_depth; $depth <= $max_depth; $depth += 2) {
    $iterations = 1 << ($max_depth - $depth + $min_depth);
    $total = 0;
    for ($i = 0; $i < $iterations; $i++) {
        $tree = make($depth);
        $total += check($tree);
        unset($tree);
    }
    printf("%d\t trees of depth %d\t check: %d\n", $iterations, $depth, $total);
}

printf("long lived tree of depth %d\t check: %d\n", $max_depth, check($long_lived));
