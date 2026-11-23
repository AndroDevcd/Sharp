// binary-trees - the Benchmarks Game.  Makes and throws away a great many
// complete binary trees, one node an allocation: a test of the allocator (or
// the garbage collector).
//
//      binarytrees <depth>

struct Node {
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

fn make(depth: i32) -> Box<Node> {
    if depth > 0 {
        Box::new(Node { left: Some(make(depth - 1)), right: Some(make(depth - 1)) })
    } else {
        Box::new(Node { left: None, right: None })
    }
}

// how many nodes the tree has
fn check(n: &Node) -> i32 {
    match (&n.left, &n.right) {
        (Some(left), Some(right)) => 1 + check(left) + check(right),
        _ => 1,
    }
}

fn main() {
    let n: i32 = std::env::args().nth(1).and_then(|a| a.parse().ok()).unwrap_or(10);
    let min_depth = 4;
    let max_depth = if n > min_depth + 2 { n } else { min_depth + 2 };
    let stretch_depth = max_depth + 1;

    let stretch = make(stretch_depth);
    println!("stretch tree of depth {}\t check: {}", stretch_depth, check(&stretch));
    drop(stretch);

    let long_lived = make(max_depth);

    let mut depth = min_depth;
    while depth <= max_depth {
        let iterations = 1 << (max_depth - depth + min_depth);
        let mut total = 0;
        for _ in 0..iterations {
            let tree = make(depth);
            total += check(&tree);
            // the tree is freed here, node by node
        }
        println!("{}\t trees of depth {}\t check: {}", iterations, depth, total);
        depth += 2;
    }

    println!("long lived tree of depth {}\t check: {}", max_depth, check(&long_lived));
}
