# mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
# [-1, 1] as an n x n black and white picture, written to the standard output
# as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
# whole byte.
#
#      perl mandelbrot.pl <n>  > picture.pbm
use strict;
use warnings;

sub mandelbrot {
    my ($n) = @_;

    binmode(STDOUT);
    printf("P4\n%d %d\n", $n, $n);
    for (my $y = 0; $y < $n; $y++) {
        my $ci = 2.0 * $y / $n - 1.0;
        my @row;
        my $bits = 0;
        for (my $x = 0; $x < $n; $x++) {
            my $cr = 2.0 * $x / $n - 1.5;
            my ($zr, $zi, $tr, $ti) = (0, 0, 0, 0);
            for (my $i = 0; $i < 50 && $tr + $ti <= 4.0; $i++) {
                $zi = 2.0 * $zr * $zi + $ci;
                $zr = $tr - $ti + $cr;
                $tr = $zr * $zr;
                $ti = $zi * $zi;
            }
            $bits = $bits * 2 + ($tr + $ti <= 4.0 ? 1 : 0);
            if ($x % 8 == 7) { push @row, $bits; $bits = 0; }
        }
        if ($n % 8 != 0) {
            # the last byte of the row: the bits that are there, moved to its top
            for (my $k = $n % 8; $k < 8; $k++) { $bits = $bits * 2; }
            push @row, $bits;
        }
        print pack('C*', @row);
    }
}

mandelbrot(@ARGV ? int($ARGV[0]) : 200);
