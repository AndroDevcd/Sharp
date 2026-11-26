/*
 * mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
 * [-1, 1] as an n x n black and white picture, written to the standard output
 * as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
 * whole byte.
 *
 *      java mandelbrot <n>  > picture.pbm
 */
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class mandelbrot {
    public static void main(String[] args) throws IOException {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 200;
        int rowBytes = (n + 7) / 8;
        byte[] row = new byte[rowBytes];
        OutputStream out = new BufferedOutputStream(System.out);

        out.write(("P4\n" + n + " " + n + "\n").getBytes(StandardCharsets.US_ASCII));
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
                if (x % 8 == 7) { row[at++] = (byte) bits; bits = 0; }
            }
            if (n % 8 != 0) {
                /* the last byte of the row: the bits that are there, moved to its top */
                for (int k = n % 8; k < 8; k++) bits = bits * 2;
                row[at++] = (byte) bits;
            }
            out.write(row, 0, rowBytes);
        }
        out.flush();
    }
}
