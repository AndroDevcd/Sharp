/*
 * binary-trees - the Benchmarks Game.  Makes and throws away a great many
 * complete binary trees, one node an object: a test of the garbage collector.
 *
 *      node binarytrees.js <depth>
 */
'use strict';

class Node {
    constructor(left, right) {
        this.left = left;
        this.right = right;
    }
}

function make(depth) {
    if (depth > 0) {
        const left = make(depth - 1);
        const right = make(depth - 1);
        return new Node(left, right);
    }
    return new Node(null, null);
}

/* how many nodes the tree has */
function check(n) {
    return n.left === null ? 1 : 1 + check(n.left) + check(n.right);
}

const n = process.argv.length > 2 ? parseInt(process.argv[2], 10) : 10;
const minDepth = 4;
const maxDepth = n > minDepth + 2 ? n : minDepth + 2;
const stretchDepth = maxDepth + 1;

console.log("stretch tree of depth " + stretchDepth + "\t check: " + check(make(stretchDepth)));

const longLived = make(maxDepth);

for (let depth = minDepth; depth <= maxDepth; depth += 2) {
    const iterations = 1 << (maxDepth - depth + minDepth);
    let total = 0;
    for (let i = 0; i < iterations; i++) {
        total += check(make(depth));
    }
    console.log(iterations + "\t trees of depth " + depth + "\t check: " + total);
}

console.log("long lived tree of depth " + maxDepth + "\t check: " + check(longLived));
