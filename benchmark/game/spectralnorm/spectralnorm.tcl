# spectral-norm - the Benchmarks Game.  The largest singular value of the
# infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
# found with ten steps of the power method.
#
#      tclsh spectralnorm.tcl <n>

# i and j are whole numbers, so the division by 2 is a whole number division
proc a {i j} {
    return [expr {1.0 / (($i + $j) * ($i + $j + 1) / 2 + $i + 1)}]
}

# A * in
proc times {n in} {
    set out {}
    for {set i 0} {$i < $n} {incr i} {
        set sum 0.0
        for {set j 0} {$j < $n} {incr j} {
            set sum [expr {$sum + [a $i $j] * [lindex $in $j]}]
        }
        lappend out $sum
    }
    return $out
}

# A transposed * in
proc times_transposed {n in} {
    set out {}
    for {set i 0} {$i < $n} {incr i} {
        set sum 0.0
        for {set j 0} {$j < $n} {incr j} {
            set sum [expr {$sum + [a $j $i] * [lindex $in $j]}]
        }
        lappend out $sum
    }
    return $out
}

proc main {argv} {
    set n [expr {[llength $argv] > 0 ? int([lindex $argv 0]) : 100}]
    set u [lrepeat $n 1.0]
    set v {}

    for {set i 0} {$i < 10} {incr i} {
        set v [times_transposed $n [times $n $u]]      ;# v = At A u
        set u [times_transposed $n [times $n $v]]      ;# u = At A v
    }

    set vbv 0.0
    set vv 0.0
    for {set i 0} {$i < $n} {incr i} {
        set ui [lindex $u $i]
        set vi [lindex $v $i]
        set vbv [expr {$vbv + $ui * $vi}]
        set vv [expr {$vv + $vi * $vi}]
    }
    puts [format "%0.9f" [expr {sqrt($vbv / $vv)}]]
}

main $argv
