# binary-trees - the Benchmarks Game.  Makes and throws away a great many
# complete binary trees, one node a list: a test of the memory manager.
# A node is a list of two, {left right}; a leaf is a pair of empty lists.
#
#      tclsh binarytrees.tcl <depth>

proc make {depth} {
    if {$depth > 0} {
        incr depth -1
        return [list [make $depth] [make $depth]]
    }
    return [list {} {}]
}

# how many nodes the tree has
proc check {n} {
    set left [lindex $n 0]
    if {[llength $left] == 0} { return 1 }
    return [expr {1 + [check $left] + [check [lindex $n 1]]}]
}

proc main {argv} {
    set n [expr {[llength $argv] > 0 ? int([lindex $argv 0]) : 10}]
    set min_depth 4
    set max_depth [expr {$n > $min_depth + 2 ? $n : $min_depth + 2}]
    set stretch_depth [expr {$max_depth + 1}]

    set stretch [make $stretch_depth]
    puts "stretch tree of depth $stretch_depth\t check: [check $stretch]"
    unset stretch

    set long_lived [make $max_depth]

    for {set depth $min_depth} {$depth <= $max_depth} {incr depth 2} {
        set iterations [expr {1 << ($max_depth - $depth + $min_depth)}]
        set total 0
        for {set i 0} {$i < $iterations} {incr i} {
            set tree [make $depth]
            incr total [check $tree]
            unset tree
        }
        puts "$iterations\t trees of depth $depth\t check: $total"
    }

    puts "long lived tree of depth $max_depth\t check: [check $long_lived]"
}

main $argv
