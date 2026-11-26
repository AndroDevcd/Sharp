# fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
# first k numbers, where k is the first number, until the first is 1. Prints
# a checksum of the flip counts and the most flips any permutation needed.
#
#      perl fannkuchredux.pl <n>
use strict;
use warnings;

sub fannkuch {
    my ($n) = @_;
    my @perm = (0) x $n;
    my @perm1 = (0 .. $n - 1);
    my @count = (0) x $n;
    my ($max_flips, $checksum, $permutations, $r) = (0, 0, 0, $n);

    for (;;) {
        while ($r != 1) { $count[$r - 1] = $r; $r--; }

        for (my $i = 0; $i < $n; $i++) { $perm[$i] = $perm1[$i]; }
        my $flips = 0;
        my $k;
        while (($k = $perm[0]) != 0) {
            for (my ($i, $j) = (0, $k); $i < $j; $i++, $j--) {
                my $t = $perm[$i]; $perm[$i] = $perm[$j]; $perm[$j] = $t;
            }
            $flips++;
        }
        $max_flips = $flips if $flips > $max_flips;
        $checksum += $permutations % 2 == 0 ? $flips : -$flips;

        # the next permutation
        for (;;) {
            if ($r == $n) {
                printf("%d\nPfannkuchen(%d) = %d\n", $checksum, $n, $max_flips);
                return;
            }
            my $first = $perm1[0];
            for (my $i = 0; $i < $r; $i++) { $perm1[$i] = $perm1[$i + 1]; }
            $perm1[$r] = $first;
            $count[$r]--;
            last if $count[$r] > 0;
            $r++;
        }
        $permutations++;
    }
}

fannkuch(@ARGV ? int($ARGV[0]) : 7);
