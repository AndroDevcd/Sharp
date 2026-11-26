// fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
// first k numbers, where k is the first number, until the first is 1. Prints
// a checksum of the flip counts and the most flips any permutation needed.
//
//      fannkuchredux <n>

fn main() {
    let n: usize = std::env::args().nth(1).and_then(|a| a.parse().ok()).unwrap_or(7);
    let mut perm = [0usize; 32];
    let mut perm1 = [0usize; 32];
    let mut count = [0usize; 32];
    let (mut max_flips, mut checksum, mut permutations) = (0, 0, 0);
    let mut r = n;

    for i in 0..n {
        perm1[i] = i;
    }

    loop {
        while r != 1 {
            count[r - 1] = r;
            r -= 1;
        }

        for i in 0..n {
            perm[i] = perm1[i];
        }
        let mut flips = 0;
        loop {
            let k = perm[0];
            if k == 0 {
                break;
            }
            let (mut i, mut j) = (0, k);
            while i < j {
                let t = perm[i];
                perm[i] = perm[j];
                perm[j] = t;
                i += 1;
                j -= 1;
            }
            flips += 1;
        }
        if flips > max_flips {
            max_flips = flips;
        }
        checksum += if permutations % 2 == 0 { flips } else { -flips };

        // the next permutation
        loop {
            if r == n {
                println!("{}\nPfannkuchen({}) = {}", checksum, n, max_flips);
                return;
            }
            let first = perm1[0];
            for i in 0..r {
                perm1[i] = perm1[i + 1];
            }
            perm1[r] = first;
            count[r] -= 1;
            if count[r] > 0 {
                break;
            }
            r += 1;
        }
        permutations += 1;
    }
}
