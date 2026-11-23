# n-body - the Benchmarks Game.  The sun and the four big planets, moved
# `steps` times by a simple symplectic integrator; prints the energy of the
# system before and after.
#
#      tclsh nbody.tcl <steps>

set PI 3.141592653589793
set SOLAR_MASS [expr {4 * $PI * $PI}]
set DAYS_PER_YEAR 365.24
set BODIES 5

proc setup {} {
    global x y z vx vy vz mass SOLAR_MASS DAYS_PER_YEAR
    set x  [list 0.0 4.84143144246472090e+00 8.34336671824457987e+00 1.28943695621391310e+01 1.53796971148509165e+01]
    set y  [list 0.0 -1.16032004402742839e+00 4.12479856412430479e+00 -1.51111514016986312e+01 -2.59193146099879641e+01]
    set z  [list 0.0 -1.03622044471123109e-01 -4.03523417114321381e-01 -2.23307578892655734e-01 1.79258772950371181e-01]
    set vx [list 0.0 \
        [expr {1.66007664274403694e-03 * $DAYS_PER_YEAR}] [expr {-2.76742510726862411e-03 * $DAYS_PER_YEAR}] \
        [expr {2.96460137564761618e-03 * $DAYS_PER_YEAR}] [expr {2.68067772490389322e-03 * $DAYS_PER_YEAR}]]
    set vy [list 0.0 \
        [expr {7.69901118419740425e-03 * $DAYS_PER_YEAR}] [expr {4.99852801234917238e-03 * $DAYS_PER_YEAR}] \
        [expr {2.37847173959480950e-03 * $DAYS_PER_YEAR}] [expr {1.62824170038242295e-03 * $DAYS_PER_YEAR}]]
    set vz [list 0.0 \
        [expr {-6.90460016972063023e-05 * $DAYS_PER_YEAR}] [expr {2.30417297573763929e-05 * $DAYS_PER_YEAR}] \
        [expr {-2.96589568540237556e-05 * $DAYS_PER_YEAR}] [expr {-9.51592254519715870e-05 * $DAYS_PER_YEAR}]]
    set mass [list $SOLAR_MASS \
        [expr {9.54791938424326609e-04 * $SOLAR_MASS}] [expr {2.85885980666130812e-04 * $SOLAR_MASS}] \
        [expr {4.36624404335156298e-05 * $SOLAR_MASS}] [expr {5.15138902046611451e-05 * $SOLAR_MASS}]]
}

proc offset_momentum {} {
    global vx vy vz mass SOLAR_MASS BODIES
    set px 0.0
    set py 0.0
    set pz 0.0
    for {set i 0} {$i < $BODIES} {incr i} {
        set m [lindex $mass $i]
        set px [expr {$px + [lindex $vx $i] * $m}]
        set py [expr {$py + [lindex $vy $i] * $m}]
        set pz [expr {$pz + [lindex $vz $i] * $m}]
    }
    lset vx 0 [expr {-$px / $SOLAR_MASS}]
    lset vy 0 [expr {-$py / $SOLAR_MASS}]
    lset vz 0 [expr {-$pz / $SOLAR_MASS}]
}

proc energy {} {
    global x y z vx vy vz mass BODIES
    set e 0.0
    for {set i 0} {$i < $BODIES} {incr i} {
        set vxi [lindex $vx $i]
        set vyi [lindex $vy $i]
        set vzi [lindex $vz $i]
        set e [expr {$e + 0.5 * [lindex $mass $i] * ($vxi * $vxi + $vyi * $vyi + $vzi * $vzi)}]
        for {set j [expr {$i + 1}]} {$j < $BODIES} {incr j} {
            set dx [expr {[lindex $x $i] - [lindex $x $j]}]
            set dy [expr {[lindex $y $i] - [lindex $y $j]}]
            set dz [expr {[lindex $z $i] - [lindex $z $j]}]
            set e [expr {$e - ([lindex $mass $i] * [lindex $mass $j]) / sqrt($dx * $dx + $dy * $dy + $dz * $dz)}]
        }
    }
    return $e
}

proc advance {dt} {
    global x y z vx vy vz mass BODIES
    for {set i 0} {$i < $BODIES} {incr i} {
        for {set j [expr {$i + 1}]} {$j < $BODIES} {incr j} {
            set dx [expr {[lindex $x $i] - [lindex $x $j]}]
            set dy [expr {[lindex $y $i] - [lindex $y $j]}]
            set dz [expr {[lindex $z $i] - [lindex $z $j]}]
            set distance2 [expr {$dx * $dx + $dy * $dy + $dz * $dz}]
            set mag [expr {$dt / ($distance2 * sqrt($distance2))}]
            set mi [lindex $mass $i]
            set mj [lindex $mass $j]
            lset vx $i [expr {[lindex $vx $i] - $dx * $mj * $mag}]
            lset vy $i [expr {[lindex $vy $i] - $dy * $mj * $mag}]
            lset vz $i [expr {[lindex $vz $i] - $dz * $mj * $mag}]
            lset vx $j [expr {[lindex $vx $j] + $dx * $mi * $mag}]
            lset vy $j [expr {[lindex $vy $j] + $dy * $mi * $mag}]
            lset vz $j [expr {[lindex $vz $j] + $dz * $mi * $mag}]
        }
    }
    for {set i 0} {$i < $BODIES} {incr i} {
        lset x $i [expr {[lindex $x $i] + $dt * [lindex $vx $i]}]
        lset y $i [expr {[lindex $y $i] + $dt * [lindex $vy $i]}]
        lset z $i [expr {[lindex $z $i] + $dt * [lindex $vz $i]}]
    }
}

proc main {argv} {
    set steps [expr {[llength $argv] > 0 ? int([lindex $argv 0]) : 1000}]

    setup
    offset_momentum
    puts [format "%.9f" [energy]]
    for {set i 0} {$i < $steps} {incr i} { advance 0.01 }
    puts [format "%.9f" [energy]]
}

main $argv
