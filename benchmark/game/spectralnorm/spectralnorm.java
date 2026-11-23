/*
 * spectral-norm - the Benchmarks Game.  The largest singular value of the
 * infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
 * found with ten steps of the power method.
 *
 *      java spectralnorm <n>
 */
public class spectralnorm {
    static double a(int i, int j) {
        return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1);
    }

    /* out = A * in */
    static void times(int n, double[] in, double[] out) {
        for (int i = 0; i < n; i++) {
            double sum = 0;
            for (int j = 0; j < n; j++) sum += a(i, j) * in[j];
            out[i] = sum;
        }
    }

    /* out = A transposed * in */
    static void timesTransposed(int n, double[] in, double[] out) {
        for (int i = 0; i < n; i++) {
            double sum = 0;
            for (int j = 0; j < n; j++) sum += a(j, i) * in[j];
            out[i] = sum;
        }
    }

    public static void main(String[] args) {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 100;
        double[] u = new double[n];
        double[] v = new double[n];
        double[] t = new double[n];

        for (int i = 0; i < n; i++) u[i] = 1;
        for (int i = 0; i < 10; i++) {
            times(n, u, t); timesTransposed(n, t, v);       /* v = At A u */
            times(n, v, t); timesTransposed(n, t, u);       /* u = At A v */
        }

        double vbv = 0, vv = 0;
        for (int i = 0; i < n; i++) {
            vbv += u[i] * v[i];
            vv += v[i] * v[i];
        }
        System.out.printf("%.9f\n", Math.sqrt(vbv / vv));
    }
}
