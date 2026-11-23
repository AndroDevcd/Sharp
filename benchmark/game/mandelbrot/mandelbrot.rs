// mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
// [-1, 1] as an n x n black and white picture, written to the standard output
// as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
// whole byte.
//
//      mandelbrot <n>  > picture.pbm

use std::io::{self, BufWriter, Write};

fn main() -> io::Result<()> {
    let n: usize = std::env::args().nth(1).and_then(|a| a.parse().ok()).unwrap_or(200);
    let row_bytes = (n + 7) / 8;
    let mut row = vec![0u8; row_bytes];
    let stdout = io::stdout();
    let mut out = BufWriter::new(stdout.lock());

    write!(out, "P4\n{} {}\n", n, n)?;
    for y in 0..n {
        let ci = 2.0 * y as f64 / n as f64 - 1.0;
        let mut bits = 0;
        let mut at = 0;
        for x in 0..n {
            let cr = 2.0 * x as f64 / n as f64 - 1.5;
            let (mut zr, mut zi, mut tr, mut ti) = (0.0, 0.0, 0.0, 0.0);
            let mut i = 0;
            while i < 50 && tr + ti <= 4.0 {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
                i += 1;
            }
            bits = bits * 2 + if tr + ti <= 4.0 { 1 } else { 0 };
            if x % 8 == 7 {
                row[at] = bits as u8;
                at += 1;
                bits = 0;
            }
        }
        if n % 8 != 0 {
            // the last byte of the row: the bits that are there, moved to its top
            for _ in n % 8..8 {
                bits = bits * 2;
            }
            row[at] = bits as u8;
        }
        out.write_all(&row)?;
    }
    out.flush()
}
