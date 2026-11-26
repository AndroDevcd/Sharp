// fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
// 60 letters a line: one by repeating a fixed text, two by drawing letters
// with given probabilities from a simple random number generator.
//
//      fasta <n>  > sequences.txt

use std::io::{self, BufWriter, Write};

const LINE: usize = 60;
const IM: i32 = 139968;
const IA: i32 = 3877;
const IC: i32 = 29573;

struct Random {
    last: i32,
}

impl Random {
    // the next random number, from 0 up to (not including) 1
    fn next(&mut self) -> f64 {
        self.last = (self.last * IA + IC) % IM;
        self.last as f64 / IM as f64
    }
}

const ALU: &[u8] = b"GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG\
                     GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA\
                     CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT\
                     ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA\
                     GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG\
                     AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC\
                     AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

// `count` letters of `text`, over and over
fn repeat(out: &mut impl Write, header: &str, text: &[u8], mut count: usize) -> io::Result<()> {
    let length = text.len();
    let mut at = 0;
    let mut line = [0u8; LINE + 1];

    out.write_all(header.as_bytes())?;
    while count > 0 {
        let width = if count < LINE { count } else { LINE };
        for i in 0..width {
            line[i] = text[at];
            at += 1;
            if at == length {
                at = 0;
            }
        }
        line[width] = b'\n';
        out.write_all(&line[..width + 1])?;
        count -= width;
    }
    Ok(())
}

// `count` letters, each drawn by its chance
fn draw(
    out: &mut impl Write,
    random: &mut Random,
    header: &str,
    letters: &[u8],
    chances: &mut [f64],
    mut count: usize,
) -> io::Result<()> {
    let kinds = letters.len();
    let mut line = [0u8; LINE + 1];

    // each chance becomes the sum of the chances up to it
    let mut sum = 0.0;
    for i in 0..kinds {
        sum += chances[i];
        chances[i] = sum;
    }

    out.write_all(header.as_bytes())?;
    while count > 0 {
        let width = if count < LINE { count } else { LINE };
        for i in 0..width {
            let r = random.next();
            let mut k = 0;
            while k < kinds - 1 && chances[k] < r {
                k += 1;
            }
            line[i] = letters[k];
        }
        line[width] = b'\n';
        out.write_all(&line[..width + 1])?;
        count -= width;
    }
    Ok(())
}

fn main() -> io::Result<()> {
    let n: usize = std::env::args().nth(1).and_then(|a| a.parse().ok()).unwrap_or(1000);
    let stdout = io::stdout();
    let mut out = BufWriter::new(stdout.lock());
    let mut random = Random { last: 42 };

    let mut iub_chances = [0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02];
    let mut human_chances = [0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008];

    repeat(&mut out, ">ONE Homo sapiens alu\n", ALU, n * 2)?;
    draw(&mut out, &mut random, ">TWO IUB ambiguity codes\n", b"acgtBDHKMNRSVWY", &mut iub_chances, n * 3)?;
    draw(&mut out, &mut random, ">THREE Homo sapiens frequency\n", b"acgt", &mut human_chances, n * 5)?;
    out.flush()
}
