// spectral-norm - the Benchmarks Game.  The largest singular value of the
// infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
// found with ten steps of the power method.
//
//      spectralnorm <n>

fn a(i: i32, j: i32) -> f64 {
    1.0 / ((i + j) * (i + j + 1) / 2 + i + 1) as f64
}

// out = A * input
fn times(n: usize, input: &[f64], out: &mut [f64]) {
    for i in 0..n {
        let mut sum = 0.0;
        for j in 0..n {
            sum += a(i as i32, j as i32) * input[j];
        }
        out[i] = sum;
    }
}

// out = A transposed * input
fn times_transposed(n: usize, input: &[f64], out: &mut [f64]) {
    for i in 0..n {
        let mut sum = 0.0;
        for j in 0..n {
            sum += a(j as i32, i as i32) * input[j];
        }
        out[i] = sum;
    }
}

fn main() {
    let n: usize = std::env::args().nth(1).and_then(|a| a.parse().ok()).unwrap_or(100);
    let mut u = vec![1.0; n];
    let mut v = vec![0.0; n];
    let mut t = vec![0.0; n];

    for _ in 0..10 {
        times(n, &u, &mut t);
        times_transposed(n, &t, &mut v); // v = At A u
        times(n, &v, &mut t);
        times_transposed(n, &t, &mut u); // u = At A v
    }

    let (mut vbv, mut vv) = (0.0, 0.0);
    for i in 0..n {
        vbv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    println!("{:.9}", (vbv / vv).sqrt());
}
