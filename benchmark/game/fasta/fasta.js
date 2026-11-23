/*
 * fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
 * 60 letters a line: one by repeating a fixed text, two by drawing letters
 * with given probabilities from a simple random number generator.
 *
 *      node fasta.js <n>  > sequences.txt
 */
'use strict';

const fs = require('fs');

const LINE = 60;
const IM = 139968;
const IA = 3877;
const IC = 29573;

/* The output is gathered in one block that is written when it is full, so
   the program holds 64 KB of text however much it writes. */
const block = Buffer.alloc(64 * 1024);
let blockSize = 0;

function flush() {
    let at = 0;
    while (at < blockSize) {
        try {
            at += fs.writeSync(1, block, at, blockSize - at);
        } catch (error) {
            if (error.code !== 'EAGAIN') throw error;   /* a full pipe: try again */
        }
    }
    blockSize = 0;
}

/* the first `length` bytes of `bytes` */
function write(bytes, length) {
    if (blockSize + length > block.length) flush();
    bytes.copy(block, blockSize, 0, length);
    blockSize += length;
}

let last = 42;

/* the next random number, from 0 up to (not including) 1 */
function nextRandom() {
    last = (last * IA + IC) % IM;
    return last / IM;
}

const alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" +
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" +
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" +
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" +
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" +
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" +
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

const iubLetters = "acgtBDHKMNRSVWY";
const iubChances = new Float64Array([0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02]);

const humanLetters = "acgt";
const humanChances = new Float64Array([0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008]);

const NEWLINE = 10;

/* `count` letters of `text`, over and over */
function repeat(header, text, count) {
    const letters = Buffer.from(text);
    const length = letters.length;
    const line = Buffer.alloc(LINE + 1);
    let at = 0;

    const head = Buffer.from(header);
    write(head, head.length);
    while (count > 0) {
        const width = count < LINE ? count : LINE;
        for (let i = 0; i < width; i++) {
            line[i] = letters[at];
            at++;
            if (at === length) at = 0;
        }
        line[width] = NEWLINE;
        write(line, width + 1);
        count -= width;
    }
}

/* `count` letters, each drawn by its chance */
function draw(header, text, chances, kinds, count) {
    const letters = Buffer.from(text);
    const line = Buffer.alloc(LINE + 1);

    /* each chance becomes the sum of the chances up to it */
    let sum = 0;
    for (let i = 0; i < kinds; i++) { sum += chances[i]; chances[i] = sum; }

    const head = Buffer.from(header);
    write(head, head.length);
    while (count > 0) {
        const width = count < LINE ? count : LINE;
        for (let i = 0; i < width; i++) {
            const r = nextRandom();
            let k = 0;
            while (k < kinds - 1 && chances[k] < r) k++;
            line[i] = letters[k];
        }
        line[width] = NEWLINE;
        write(line, width + 1);
        count -= width;
    }
}

const n = process.argv.length > 2 ? parseInt(process.argv[2], 10) : 1000;

repeat(">ONE Homo sapiens alu\n", alu, n * 2);
draw(">TWO IUB ambiguity codes\n", iubLetters, iubChances, 15, n * 3);
draw(">THREE Homo sapiens frequency\n", humanLetters, humanChances, 4, n * 5);
flush();
