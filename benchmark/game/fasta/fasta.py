# fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
# 60 letters a line: one by repeating a fixed text, two by drawing letters
# with given probabilities from a simple random number generator.
#
#      python3 fasta.py <n>  > sequences.txt

import sys

LINE = 60
IM = 139968
IA = 3877
IC = 29573

ALU = (
    b"GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
    b"GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
    b"CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
    b"ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
    b"GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
    b"AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
    b"AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"
)

IUB_LETTERS = b"acgtBDHKMNRSVWY"
IUB_CHANCES = [0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02]

HUMAN_LETTERS = b"acgt"
HUMAN_CHANCES = [0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008]

NEWLINE = 10

# the random number generator remembers its last number
last = 42


def next_random():
    """the next random number, from 0 up to (not including) 1"""
    global last
    last = (last * IA + IC) % IM
    return last / IM


def repeat(write, header, text, count):
    """`count` letters of `text`, over and over"""
    length = len(text)
    at = 0
    line = bytearray(LINE + 1)

    write(header)
    while count > 0:
        width = count if count < LINE else LINE
        for i in range(width):
            line[i] = text[at]
            at += 1
            if at == length:
                at = 0
        line[width] = NEWLINE
        write(line[:width + 1])
        count -= width


def draw(write, header, letters, chances, kinds, count):
    """`count` letters, each drawn by its chance"""
    line = bytearray(LINE + 1)

    # each chance becomes the sum of the chances up to it
    total = 0.0
    for i in range(kinds):
        total += chances[i]
        chances[i] = total

    write(header)
    while count > 0:
        width = count if count < LINE else LINE
        for i in range(width):
            r = next_random()
            k = 0
            while k < kinds - 1 and chances[k] < r:
                k += 1
            line[i] = letters[k]
        line[width] = NEWLINE
        write(line[:width + 1])
        count -= width


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
    write = sys.stdout.buffer.write

    repeat(write, b">ONE Homo sapiens alu\n", ALU, n * 2)
    draw(write, b">TWO IUB ambiguity codes\n", IUB_LETTERS, IUB_CHANCES, 15, n * 3)
    draw(write, b">THREE Homo sapiens frequency\n", HUMAN_LETTERS, HUMAN_CHANCES, 4, n * 5)


main()
