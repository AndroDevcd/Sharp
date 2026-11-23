# fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
# 60 letters a line: one by repeating a fixed text, two by drawing letters
# with given probabilities from a simple random number generator.
#
#      perl fasta.pl <n>  > sequences.txt
use strict;
use warnings;

use constant LINE => 60;
use constant IM => 139968;
use constant IA => 3877;
use constant IC => 29573;

my $last = 42;

# the next random number, from 0 up to (not including) 1
sub next_random {
    $last = ($last * IA + IC) % IM;
    return $last / IM;
}

my $alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" .
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" .
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" .
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" .
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" .
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" .
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

my $iub_letters = "acgtBDHKMNRSVWY";
my @iub_chances = (0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02);

my $human_letters = "acgt";
my @human_chances = (0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008);

# `count` letters of `text`, over and over
sub repeat {
    my ($header, $text, $count) = @_;
    my $length = length($text);
    my $at = 0;

    print $header;
    while ($count > 0) {
        my $width = $count < LINE ? $count : LINE;
        my $line = '';
        for (my $i = 0; $i < $width; $i++) {
            $line .= substr($text, $at, 1);
            $at++;
            $at = 0 if $at == $length;
        }
        print $line, "\n";
        $count -= $width;
    }
}

# `count` letters, each drawn by its chance
sub draw {
    my ($header, $letters, $chances, $count) = @_;
    my $kinds = scalar @$chances;

    # each chance becomes the sum of the chances up to it
    my $sum = 0;
    for (my $i = 0; $i < $kinds; $i++) { $sum += $chances->[$i]; $chances->[$i] = $sum; }

    print $header;
    while ($count > 0) {
        my $width = $count < LINE ? $count : LINE;
        my $line = '';
        for (my $i = 0; $i < $width; $i++) {
            my $r = next_random();
            my $k = 0;
            $k++ while $k < $kinds - 1 && $chances->[$k] < $r;
            $line .= substr($letters, $k, 1);
        }
        print $line, "\n";
        $count -= $width;
    }
}

my $n = @ARGV ? int($ARGV[0]) : 1000;

repeat(">ONE Homo sapiens alu\n", $alu, $n * 2);
draw(">TWO IUB ambiguity codes\n", $iub_letters, \@iub_chances, $n * 3);
draw(">THREE Homo sapiens frequency\n", $human_letters, \@human_chances, $n * 5);
