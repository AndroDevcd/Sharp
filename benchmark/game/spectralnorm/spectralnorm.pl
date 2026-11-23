# spectral-norm - the Benchmarks Game.  The largest singular value of the
# infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
# found with ten steps of the power method.
#
#      perl spectralnorm.pl <n>
use strict;
use warnings;

sub a {
    my ($i, $j) = @_;
    return 1.0 / (int(($i + $j) * ($i + $j + 1) / 2) + $i + 1);
}

# A * in (Perl has a built-in `times`, hence the name)
sub multiply {
    my ($n, $in) = @_;
    my @out;
    for (my $i = 0; $i < $n; $i++) {
        my $sum = 0;
        for (my $j = 0; $j < $n; $j++) { $sum += a($i, $j) * $in->[$j]; }
        $out[$i] = $sum;
    }
    return \@out;
}

# A transposed * in
sub multiply_transposed {
    my ($n, $in) = @_;
    my @out;
    for (my $i = 0; $i < $n; $i++) {
        my $sum = 0;
        for (my $j = 0; $j < $n; $j++) { $sum += a($j, $i) * $in->[$j]; }
        $out[$i] = $sum;
    }
    return \@out;
}

my $n = @ARGV ? int($ARGV[0]) : 100;
my $u = [(1) x $n];
my $v;

for (my $i = 0; $i < 10; $i++) {
    $v = multiply_transposed($n, multiply($n, $u));      # v = At A u
    $u = multiply_transposed($n, multiply($n, $v));      # u = At A v
}

my ($vbv, $vv) = (0, 0);
for (my $i = 0; $i < $n; $i++) {
    $vbv += $u->[$i] * $v->[$i];
    $vv += $v->[$i] * $v->[$i];
}
printf("%0.9f\n", sqrt($vbv / $vv));
