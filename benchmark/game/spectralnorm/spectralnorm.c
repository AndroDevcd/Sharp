/*
 * spectral-norm - the Benchmarks Game.  The largest singular value of the
 * infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
 * found with ten steps of the power method.
 *
 *      spectralnorm <n>
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static double a(int i, int j) {
    return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1);
}

/* out = A * in */
static void times(int n, const double *in, double *out) {
    for (int i = 0; i < n; i++) {
        double sum = 0;
        for (int j = 0; j < n; j++) sum += a(i, j) * in[j];
        out[i] = sum;
    }
}

/* out = A transposed * in */
static void times_transposed(int n, const double *in, double *out) {
    for (int i = 0; i < n; i++) {
        double sum = 0;
        for (int j = 0; j < n; j++) sum += a(j, i) * in[j];
        out[i] = sum;
    }
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 100;
    double *u = malloc(n * sizeof(double));
    double *v = malloc(n * sizeof(double));
    double *t = malloc(n * sizeof(double));

    for (int i = 0; i < n; i++) u[i] = 1;
    for (int i = 0; i < 10; i++) {
        times(n, u, t); times_transposed(n, t, v);      /* v = At A u */
        times(n, v, t); times_transposed(n, t, u);      /* u = At A v */
    }

    double vbv = 0, vv = 0;
    for (int i = 0; i < n; i++) {
        vbv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    printf("%0.9f\n", sqrt(vbv / vv));
    return 0;
}
