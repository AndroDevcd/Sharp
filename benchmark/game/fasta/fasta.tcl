# fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
# 60 letters a line: one by repeating a fixed text, two by drawing letters
# with given probabilities from a simple random number generator.
#
#      tclsh fasta.tcl <n>  > sequences.txt

set LINE 60
set IM 139968
set IA 3877
set IC 29573

set last 42

# the next random number, from 0 up to (not including) 1
proc next_random {} {
    global last IM IA IC
    set last [expr {($last * $IA + $IC) % $IM}]
    return [expr {double($last) / $IM}]
}

set alu [string cat \
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" \
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" \
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" \
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" \
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" \
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" \
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"]

set iub_letters "acgtBDHKMNRSVWY"
set iub_chances {0.27 0.12 0.12 0.27 0.02 0.02 0.02 0.02 0.02 0.02 0.02 0.02 0.02 0.02 0.02}

set human_letters "acgt"
set human_chances {0.3029549426680 0.1979883004921 0.1975473066391 0.3015094502008}

# `count` letters of `text`, over and over
proc repeat {header text count} {
    global LINE
    set length [string length $text]
    set at 0

    puts $header
    while {$count > 0} {
        set width [expr {$count < $LINE ? $count : $LINE}]
        set line ""
        for {set i 0} {$i < $width} {incr i} {
            append line [string index $text $at]
            incr at
            if {$at == $length} { set at 0 }
        }
        puts $line
        set count [expr {$count - $width}]
    }
}

# `count` letters, each drawn by its chance
proc draw {header letters chances count} {
    global LINE
    set kinds [llength $chances]

    # each chance becomes the sum of the chances up to it
    set sum 0.0
    for {set i 0} {$i < $kinds} {incr i} {
        set sum [expr {$sum + [lindex $chances $i]}]
        lset chances $i $sum
    }

    puts $header
    while {$count > 0} {
        set width [expr {$count < $LINE ? $count : $LINE}]
        set line ""
        for {set i 0} {$i < $width} {incr i} {
            set r [next_random]
            set k 0
            while {$k < $kinds - 1 && [lindex $chances $k] < $r} { incr k }
            append line [string index $letters $k]
        }
        puts $line
        set count [expr {$count - $width}]
    }
}

proc main {argv} {
    global alu iub_letters iub_chances human_letters human_chances
    set n [expr {[llength $argv] > 0 ? int([lindex $argv 0]) : 1000}]

    repeat ">ONE Homo sapiens alu" $alu [expr {$n * 2}]
    draw ">TWO IUB ambiguity codes" $iub_letters $iub_chances [expr {$n * 3}]
    draw ">THREE Homo sapiens frequency" $human_letters $human_chances [expr {$n * 5}]
}

main $argv
