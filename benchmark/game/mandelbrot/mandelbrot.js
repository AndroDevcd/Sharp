/*
 * mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
 * [-1, 1] as an n x n black and white picture, written to the standard output
 * as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
 * whole byte.
 *
 *      node mandelbrot.js <n>  > picture.pbm
 */
'use strict';

const fs = require('fs');

/* writes the whole buffer to the standard output before it returns */
function write(buffer) {
    let at = 0;
    while (at < buffer.length) {
        try {
            at += fs.writeSync(1, buffer, at, buffer.length - at);
        } catch (error) {
            if (error.code !== 'EAGAIN') throw error;   /* a full pipe: try again */
        }
    }
}

const n = process.argv.length > 2 ? parseInt(process.argv[2], 10) : 200;
const rowBytes = (n + 7) >> 3;
const row = Buffer.alloc(rowBytes);

write(Buffer.from("P4\n" + n + " " + n + "\n"));
for (let y = 0; y < n; y++) {
    const ci = 2.0 * y / n - 1.0;
    let bits = 0, at = 0;
    for (let x = 0; x < n; x++) {
        const cr = 2.0 * x / n - 1.5;
        let zr = 0, zi = 0, tr = 0, ti = 0;
        for (let i = 0; i < 50 && tr + ti <= 4.0; i++) {
            zi = 2.0 * zr * zi + ci;
            zr = tr - ti + cr;
            tr = zr * zr;
            ti = zi * zi;
        }
        bits = bits * 2 + (tr + ti <= 4.0 ? 1 : 0);
        if (x % 8 === 7) { row[at++] = bits; bits = 0; }
    }
    if (n % 8 !== 0) {
        /* the last byte of the row: the bits that are there, moved to its top */
        for (let k = n % 8; k < 8; k++) bits = bits * 2;
        row[at++] = bits;
    }
    write(row);
}
