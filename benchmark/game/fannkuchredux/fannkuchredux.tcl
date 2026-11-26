# fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
# first k numbers, where k is the first number, until the first is 1. Prints
# a checksum of the flip counts and the most flips any permutation needed.
#
#      tclsh fannkuchredux.tcl <n>

proc fannkuch {n} {
    set perm [lrepeat $n 0]
    set perm1 {}
    set count [lrepeat $n 0]
    set max_flips 0
    set checksum 0
    set permutations 0
    set r $n

    for {set i 0} {$i < $n} {incr i} { lappend perm1 $i }

    while {1} {
        while {$r != 1} {
            lset count [expr {$r - 1}] $r
            incr r -1
        }

        for {set i 0} {$i < $n} {incr i} { lset perm $i [lindex $perm1 $i] }
        set flips 0
        while {[set k [lindex $perm 0]] != 0} {
            for {set i 0; set j $k} {$i < $j} {incr i; incr j -1} {
                set t [lindex $perm $i]
                lset perm $i [lindex $perm $j]
                lset perm $j $t
            }
            incr flips
        }
        if {$flips > $max_flips} { set max_flips $flips }
        if {$permutations % 2 == 0} {
            incr checksum $flips
        } else {
            incr checksum [expr {-$flips}]
        }

        # the next permutation
        while {1} {
            if {$r == $n} {
                puts "$checksum\nPfannkuchen($n) = $max_flips"
                return
            }
            set first [lindex $perm1 0]
            for {set i 0} {$i < $r} {incr i} {
                lset perm1 $i [lindex $perm1 [expr {$i + 1}]]
            }
            lset perm1 $r $first
            set left [expr {[lindex $count $r] - 1}]
            lset count $r $left
            if {$left > 0} break
            incr r
        }
        incr permutations
    }
}

fannkuch [expr {[llength $argv] > 0 ? int([lindex $argv 0]) : 7}]
