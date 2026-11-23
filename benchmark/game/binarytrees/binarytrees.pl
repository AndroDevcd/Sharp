# binary-trees - the Benchmarks Game.  Makes and throws away a great many
# complete binary trees, one node an array: a test of the memory manager.
# A node is a reference to [left, right]; a leaf is [undef, undef].
#
#      perl binarytrees.pl <depth>
use strict;
use warnings;

sub make {
    my ($depth) = @_;
    if ($depth > 0) {
        return [make($depth - 1), make($depth - 1)];
    }
    return [undef, undef];
}

# how many nodes the tree has
sub check {
    my ($n) = @_;
    return defined $n->[0] ? 1 + check($n->[0]) + check($n->[1]) : 1;
}

my $n = @ARGV ? int($ARGV[0]) : 10;
my $min_depth = 4;
my $max_depth = $n > $min_depth + 2 ? $n : $min_depth + 2;
my $stretch_depth = $max_depth + 1;

my $stretch = make($stretch_depth);
printf("stretch tree of depth %d\t check: %d\n", $stretch_depth, check($stretch));
undef $stretch;

my $long_lived = make($max_depth);

for (my $depth = $min_depth; $depth <= $max_depth; $depth += 2) {
    my $iterations = 1 << ($max_depth - $depth + $min_depth);
    my $total = 0;
    for (my $i = 0; $i < $iterations; $i++) {
        my $tree = make($depth);
        $total += check($tree);
    }
    printf("%d\t trees of depth %d\t check: %d\n", $iterations, $depth, $total);
}

printf("long lived tree of depth %d\t check: %d\n", $max_depth, check($long_lived));
