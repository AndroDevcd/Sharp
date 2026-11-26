<?php
/*
 * mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
 * [-1, 1] as an n x n black and white picture, written to the standard output
 * as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
 * whole byte.
 *
 *      php mandelbrot.php <n>  > picture.pbm
 */

function mandelbrot($n) {
    printf("P4\n%d %d\n", $n, $n);
    for ($y = 0; $y < $n; $y++) {
        $ci = 2.0 * $y / $n - 1.0;
        $row = '';
        $bits = 0;
        for ($x = 0; $x < $n; $x++) {
            $cr = 2.0 * $x / $n - 1.5;
            $zr = 0.0; $zi = 0.0; $tr = 0.0; $ti = 0.0;
            for ($i = 0; $i < 50 && $tr + $ti <= 4.0; $i++) {
                $zi = 2.0 * $zr * $zi + $ci;
                $zr = $tr - $ti + $cr;
                $tr = $zr * $zr;
                $ti = $zi * $zi;
            }
            $bits = $bits * 2 + ($tr + $ti <= 4.0 ? 1 : 0);
            if ($x % 8 == 7) { $row .= chr($bits); $bits = 0; }
        }
        if ($n % 8 != 0) {
            /* the last byte of the row: the bits that are there, moved to its top */
            for ($k = $n % 8; $k < 8; $k++) $bits = $bits * 2;
            $row .= chr($bits);
        }
        echo $row;
    }
}

mandelbrot($argc > 1 ? (int) $argv[1] : 200);
