/*
 * spectral-norm - the Benchmarks Game.  The largest singular value of the
 * infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
 * found with ten steps of the power method.
 *
 *      node spectralnorm.js <n>
 */
'use strict';

function a(i, j) {
    return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1);
}

/* out = A * in */
function times(n, input, out) {
    for (let i = 0; i < n; i++) {
        let sum = 0;
        for (let j = 0; j < n; j++) sum += a(i, j) * input[j];
        out[i] = sum;
    }
}

/* out = A transposed * in */
function timesTransposed(n, input, out) {
    for (let i = 0; i < n; i++) {
        let sum = 0;
        for (let j = 0; j < n; j++) sum += a(j, i) * input[j];
        out[i] = sum;
    }
}

const n = process.argv.length > 2 ? parseInt(process.argv[2], 10) : 100;
const u = new Float64Array(n);
const v = new Float64Array(n);
const t = new Float64Array(n);

for (let i = 0; i < n; i++) u[i] = 1;
for (let i = 0; i < 10; i++) {
    times(n, u, t); timesTransposed(n, t, v);       /* v = At A u */
    times(n, v, t); timesTransposed(n, t, u);       /* u = At A v */
}

let vbv = 0, vv = 0;
for (let i = 0; i < n; i++) {
    vbv += u[i] * v[i];
    vv += v[i] * v[i];
}
console.log(Math.sqrt(vbv / vv).toFixed(9));
