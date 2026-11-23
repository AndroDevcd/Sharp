/*
 * mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
 * [-1, 1] as an n x n black and white picture, written to the standard output
 * as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
 * whole byte.
 *
 *      mandelbrot <n>  > picture.pbm
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 200;
    int row_bytes = (n + 7) / 8;
    unsigned char *row = malloc(row_bytes);

    printf("P4\n%d %d\n", n, n);
    for (int y = 0; y < n; y++) {
        double ci = 2.0 * y / n - 1.0;
        int bits = 0, at = 0;
        for (int x = 0; x < n; x++) {
            double cr = 2.0 * x / n - 1.5;
            double zr = 0, zi = 0, tr = 0, ti = 0;
            for (int i = 0; i < 50 && tr + ti <= 4.0; i++) {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
            }
            bits = bits * 2 + (tr + ti <= 4.0 ? 1 : 0);
            if (x % 8 == 7) { row[at++] = (unsigned char) bits; bits = 0; }
        }
        if (n % 8 != 0) {
            /* the last byte of the row: the bits that are there, moved to its top */
            for (int k = n % 8; k < 8; k++) bits = bits * 2;
            row[at++] = (unsigned char) bits;
        }
        fwrite(row, 1, row_bytes, stdout);
    }
    return 0;
}
