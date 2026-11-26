/*
 * Matrix multiplication - the "matmul" test of github.com/kostya/benchmarks,
 * written the way matmul.sharp is: two n x n matrices of doubles, each one
 * array of n * n numbers, multiplied the plain way after transposing the
 * second. Prints the middle element of the product.
 *
 *      matmul [n]          (default 100; the original runs n = 1500)
 *
 * Measured: building and multiplying. The last line is `time: <seconds>`.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static double *build_matrix(int n, double seed) {
    double t = seed / n / n;
    double *m = malloc(sizeof(double) * n * n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            m[i * n + j] = t * (i - j) * (i + j);
    return m;
}

static double *matmul(const double *a, const double *b, int n) {
    /* c = b transposed */
    double *c = malloc(sizeof(double) * n * n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            c[i * n + j] = b[j * n + i];

    double *d = malloc(sizeof(double) * n * n);
    for (int i = 0; i < n; i++) {
        const double *row = a + i * n;
        for (int j = 0; j < n; j++) {
            const double *col = c + j * n;
            double s = 0;
            for (int k = 0; k < n; k++) s += row[k] * col[k];
            d[i * n + j] = s;
        }
    }
    free(c);
    return d;
}

static double calc(int size) {
    int n = size / 2 * 2;
    double *a = build_matrix(n, 1), *b = build_matrix(n, 2);
    double *d = matmul(a, b, n);
    double result = d[n / 2 * n + n / 2];
    free(a); free(b); free(d);
    return result;
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 100;

    double start = now();
    double result = calc(n);
    double seconds = now() - start;

    printf("%.6f\n", result);
    printf("time: %.3f\n", seconds);
    return 0;
}
