<?php
/*
 * spectral-norm - the Benchmarks Game.  The largest singular value of the
 * infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
 * found with ten steps of the power method.
 *
 *      php spectralnorm.php <n>
 */

function a($i, $j) {
    return 1.0 / (intdiv(($i + $j) * ($i + $j + 1), 2) + $i + 1);
}

/* A * in */
function times($n, $in) {
    $out = [];
    for ($i = 0; $i < $n; $i++) {
        $sum = 0.0;
        for ($j = 0; $j < $n; $j++) $sum += a($i, $j) * $in[$j];
        $out[$i] = $sum;
    }
    return $out;
}

/* A transposed * in */
function times_transposed($n, $in) {
    $out = [];
    for ($i = 0; $i < $n; $i++) {
        $sum = 0.0;
        for ($j = 0; $j < $n; $j++) $sum += a($j, $i) * $in[$j];
        $out[$i] = $sum;
    }
    return $out;
}

$n = $argc > 1 ? (int) $argv[1] : 100;
$u = array_fill(0, $n, 1.0);
$v = [];

for ($i = 0; $i < 10; $i++) {
    $v = times_transposed($n, times($n, $u));      /* v = At A u */
    $u = times_transposed($n, times($n, $v));      /* u = At A v */
}

$vbv = 0.0;
$vv = 0.0;
for ($i = 0; $i < $n; $i++) {
    $vbv += $u[$i] * $v[$i];
    $vv += $v[$i] * $v[$i];
}
printf("%0.9f\n", sqrt($vbv / $vv));
