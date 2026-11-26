/*
 * fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
 * 60 letters a line: one by repeating a fixed text, two by drawing letters
 * with given probabilities from a simple random number generator.
 *
 *      fasta <n>  > sequences.txt
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE 60
#define IM 139968
#define IA 3877
#define IC 29573

static int last = 42;

/* the next random number, from 0 up to (not including) 1 */
static double next_random(void) {
    last = (last * IA + IC) % IM;
    return (double) last / IM;
}

static const char *alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

static const char iub_letters[] = "acgtBDHKMNRSVWY";
static double iub_chances[] = { 0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02 };

static const char human_letters[] = "acgt";
static double human_chances[] = { 0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008 };

/* `count` letters of `text`, over and over */
static void repeat(const char *header, const char *text, int count) {
    int length = (int) strlen(text), at = 0;
    char line[LINE + 1];

    fputs(header, stdout);
    while (count > 0) {
        int width = count < LINE ? count : LINE;
        for (int i = 0; i < width; i++) {
            line[i] = text[at];
            at++;
            if (at == length) at = 0;
        }
        line[width] = '\n';
        fwrite(line, 1, width + 1, stdout);
        count -= width;
    }
}

/* `count` letters, each drawn by its chance */
static void draw(const char *header, const char *letters, double *chances, int kinds, int count) {
    char line[LINE + 1];

    /* each chance becomes the sum of the chances up to it */
    double sum = 0;
    for (int i = 0; i < kinds; i++) { sum += chances[i]; chances[i] = sum; }

    fputs(header, stdout);
    while (count > 0) {
        int width = count < LINE ? count : LINE;
        for (int i = 0; i < width; i++) {
            double r = next_random();
            int k = 0;
            while (k < kinds - 1 && chances[k] < r) k++;
            line[i] = letters[k];
        }
        line[width] = '\n';
        fwrite(line, 1, width + 1, stdout);
        count -= width;
    }
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 1000;

    repeat(">ONE Homo sapiens alu\n", alu, n * 2);
    draw(">TWO IUB ambiguity codes\n", iub_letters, iub_chances, 15, n * 3);
    draw(">THREE Homo sapiens frequency\n", human_letters, human_chances, 4, n * 5);
    return 0;
}
