/*
 * fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
 * 60 letters a line: one by repeating a fixed text, two by drawing letters
 * with given probabilities from a simple random number generator.
 *
 *      java fasta <n>  > sequences.txt
 */
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class fasta {
    static final int LINE = 60;
    static final int IM = 139968;
    static final int IA = 3877;
    static final int IC = 29573;

    static int last = 42;

    /* the next random number, from 0 up to (not including) 1 */
    static double nextRandom() {
        last = (last * IA + IC) % IM;
        return (double) last / IM;
    }

    static final String ALU =
        "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
        + "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
        + "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
        + "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
        + "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
        + "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
        + "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

    static final String IUB_LETTERS = "acgtBDHKMNRSVWY";
    static double[] iubChances = { 0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02 };

    static final String HUMAN_LETTERS = "acgt";
    static double[] humanChances = { 0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008 };

    static byte[] bytes(String text) {
        return text.getBytes(StandardCharsets.US_ASCII);
    }

    /* `count` letters of `text`, over and over */
    static void repeat(OutputStream out, String header, byte[] text, int count) throws IOException {
        int length = text.length, at = 0;
        byte[] line = new byte[LINE + 1];

        out.write(bytes(header));
        while (count > 0) {
            int width = count < LINE ? count : LINE;
            for (int i = 0; i < width; i++) {
                line[i] = text[at];
                at++;
                if (at == length) at = 0;
            }
            line[width] = '\n';
            out.write(line, 0, width + 1);
            count -= width;
        }
    }

    /* `count` letters, each drawn by its chance */
    static void draw(OutputStream out, String header, byte[] letters, double[] chances, int kinds, int count) throws IOException {
        byte[] line = new byte[LINE + 1];

        /* each chance becomes the sum of the chances up to it */
        double sum = 0;
        for (int i = 0; i < kinds; i++) { sum += chances[i]; chances[i] = sum; }

        out.write(bytes(header));
        while (count > 0) {
            int width = count < LINE ? count : LINE;
            for (int i = 0; i < width; i++) {
                double r = nextRandom();
                int k = 0;
                while (k < kinds - 1 && chances[k] < r) k++;
                line[i] = letters[k];
            }
            line[width] = '\n';
            out.write(line, 0, width + 1);
            count -= width;
        }
    }

    public static void main(String[] args) throws IOException {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 1000;
        OutputStream out = new BufferedOutputStream(System.out);

        repeat(out, ">ONE Homo sapiens alu\n", bytes(ALU), n * 2);
        draw(out, ">TWO IUB ambiguity codes\n", bytes(IUB_LETTERS), iubChances, 15, n * 3);
        draw(out, ">THREE Homo sapiens frequency\n", bytes(HUMAN_LETTERS), humanChances, 4, n * 5);
        out.flush();
    }
}
