/*
 * spectral-norm - the Benchmarks Game.  The largest singular value of the
 * infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
 * found with ten steps of the power method.
 *
 *      mono spectralnorm.exe <n>
 */
using System;
using System.Globalization;

static class SpectralNorm {
    static double A(int i, int j) {
        return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1);
    }

    /* output = A * input */
    static void Times(int n, double[] input, double[] output) {
        for (int i = 0; i < n; i++) {
            double sum = 0;
            for (int j = 0; j < n; j++) sum += A(i, j) * input[j];
            output[i] = sum;
        }
    }

    /* output = A transposed * input */
    static void TimesTransposed(int n, double[] input, double[] output) {
        for (int i = 0; i < n; i++) {
            double sum = 0;
            for (int j = 0; j < n; j++) sum += A(j, i) * input[j];
            output[i] = sum;
        }
    }

    static void Main(string[] args) {
        int n = args.Length > 0 ? int.Parse(args[0]) : 100;
        double[] u = new double[n];
        double[] v = new double[n];
        double[] t = new double[n];

        for (int i = 0; i < n; i++) u[i] = 1;
        for (int i = 0; i < 10; i++) {
            Times(n, u, t); TimesTransposed(n, t, v);       /* v = At A u */
            Times(n, v, t); TimesTransposed(n, t, u);       /* u = At A v */
        }

        double vbv = 0, vv = 0;
        for (int i = 0; i < n; i++) {
            vbv += u[i] * v[i];
            vv += v[i] * v[i];
        }
        Console.Write(Math.Sqrt(vbv / vv).ToString("F9", CultureInfo.InvariantCulture) + "\n");
    }
}
