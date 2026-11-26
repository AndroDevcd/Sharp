<?php
/*
 * fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
 * 60 letters a line: one by repeating a fixed text, two by drawing letters
 * with given probabilities from a simple random number generator.
 *
 *      php fasta.php <n>  > sequences.txt
 */

const LINE = 60;
const IM = 139968;
const IA = 3877;
const IC = 29573;

$last = 42;

/* the next random number, from 0 up to (not including) 1 */
function next_random() {
    global $last;
    $last = ($last * IA + IC) % IM;
    return (float) $last / IM;
}

$alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" .
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" .
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" .
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" .
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" .
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" .
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

$iub_letters = "acgtBDHKMNRSVWY";
$iub_chances = [0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02];

$human_letters = "acgt";
$human_chances = [0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008];

/* `count` letters of `text`, over and over */
function repeat($header, $text, $count) {
    $length = strlen($text);
    $at = 0;

    echo $header;
    while ($count > 0) {
        $width = $count < LINE ? $count : LINE;
        $line = '';
        for ($i = 0; $i < $width; $i++) {
            $line .= $text[$at];
            $at++;
            if ($at == $length) $at = 0;
        }
        echo $line, "\n";
        $count -= $width;
    }
}

/* `count` letters, each drawn by its chance */
function draw($header, $letters, $chances, $count) {
    $kinds = count($chances);

    /* each chance becomes the sum of the chances up to it */
    $sum = 0.0;
    for ($i = 0; $i < $kinds; $i++) { $sum += $chances[$i]; $chances[$i] = $sum; }

    echo $header;
    while ($count > 0) {
        $width = $count < LINE ? $count : LINE;
        $line = '';
        for ($i = 0; $i < $width; $i++) {
            $r = next_random();
            $k = 0;
            while ($k < $kinds - 1 && $chances[$k] < $r) $k++;
            $line .= $letters[$k];
        }
        echo $line, "\n";
        $count -= $width;
    }
}

$n = $argc > 1 ? (int) $argv[1] : 1000;

repeat(">ONE Homo sapiens alu\n", $alu, $n * 2);
draw(">TWO IUB ambiguity codes\n", $iub_letters, $iub_chances, $n * 3);
draw(">THREE Homo sapiens frequency\n", $human_letters, $human_chances, $n * 5);
