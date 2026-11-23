# mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
# [-1, 1] as an n x n black and white picture, written to the standard output
# as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
# whole byte.
#
#      tclsh mandelbrot.tcl <n>  > picture.pbm

proc mandelbrot {n} {
    fconfigure stdout -translation binary
    puts -nonewline "P4\n$n $n\n"

    for {set y 0} {$y < $n} {incr y} {
        set ci [expr {2.0 * $y / $n - 1.0}]
        set row {}
        set bits 0
        for {set x 0} {$x < $n} {incr x} {
            set cr [expr {2.0 * $x / $n - 1.5}]
            set zr 0.0
            set zi 0.0
            set tr 0.0
            set ti 0.0
            for {set i 0} {$i < 50 && $tr + $ti <= 4.0} {incr i} {
                set zi [expr {2.0 * $zr * $zi + $ci}]
                set zr [expr {$tr - $ti + $cr}]
                set tr [expr {$zr * $zr}]
                set ti [expr {$zi * $zi}]
            }
            set bits [expr {$bits * 2 + ($tr + $ti <= 4.0 ? 1 : 0)}]
            if {$x % 8 == 7} {
                lappend row $bits
                set bits 0
            }
        }
        if {$n % 8 != 0} {
            # the last byte of the row: the bits that are there, moved to its top
            for {set k [expr {$n % 8}]} {$k < 8} {incr k} { set bits [expr {$bits * 2}] }
            lappend row $bits
        }
        puts -nonewline [binary format c* $row]
    }
}

mandelbrot [expr {[llength $argv] > 0 ? int([lindex $argv 0]) : 200}]
