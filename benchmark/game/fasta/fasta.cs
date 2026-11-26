/*
 * fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
 * 60 letters a line: one by repeating a fixed text, two by drawing letters
 * with given probabilities from a simple random number generator.
 *
 *      mono fasta.exe <n>  > sequences.txt
 */
using System;
using System.IO;
using System.Text;

static class Fasta {
    const int Line = 60;
    const int IM = 139968;
    const int IA = 3877;
    const int IC = 29573;

    static int last = 42;

    /* the next random number, from 0 up to (not including) 1 */
    static double NextRandom() {
        last = (last * IA + IC) % IM;
        return (double) last / IM;
    }

    const string Alu =
        "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
        + "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
        + "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
        + "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
        + "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
        + "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
        + "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

    const string IubLetters = "acgtBDHKMNRSVWY";
    static double[] iubChances = { 0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02 };

    const string HumanLetters = "acgt";
    static double[] humanChances = { 0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008 };

    static byte[] Bytes(string text) {
        return Encoding.ASCII.GetBytes(text);
    }

    /* `count` letters of `text`, over and over */
    static void Repeat(Stream output, string header, byte[] text, int count) {
        int length = text.Length, at = 0;
        byte[] line = new byte[Line + 1];

        byte[] head = Bytes(header);
        output.Write(head, 0, head.Length);
        while (count > 0) {
            int width = count < Line ? count : Line;
            for (int i = 0; i < width; i++) {
                line[i] = text[at];
                at++;
                if (at == length) at = 0;
            }
            line[width] = (byte) '\n';
            output.Write(line, 0, width + 1);
            count -= width;
        }
    }

    /* `count` letters, each drawn by its chance */
    static void Draw(Stream output, string header, byte[] letters, double[] chances, int kinds, int count) {
        byte[] line = new byte[Line + 1];

        /* each chance becomes the sum of the chances up to it */
        double sum = 0;
        for (int i = 0; i < kinds; i++) { sum += chances[i]; chances[i] = sum; }

        byte[] head = Bytes(header);
        output.Write(head, 0, head.Length);
        while (count > 0) {
            int width = count < Line ? count : Line;
            for (int i = 0; i < width; i++) {
                double r = NextRandom();
                int k = 0;
                while (k < kinds - 1 && chances[k] < r) k++;
                line[i] = letters[k];
            }
            line[width] = (byte) '\n';
            output.Write(line, 0, width + 1);
            count -= width;
        }
    }

    static void Main(string[] args) {
        int n = args.Length > 0 ? int.Parse(args[0]) : 1000;
        Stream output = new BufferedStream(Console.OpenStandardOutput());

        Repeat(output, ">ONE Homo sapiens alu\n", Bytes(Alu), n * 2);
        Draw(output, ">TWO IUB ambiguity codes\n", Bytes(IubLetters), iubChances, 15, n * 3);
        Draw(output, ">THREE Homo sapiens frequency\n", Bytes(HumanLetters), humanChances, 4, n * 5);
        output.Flush();
    }
}
