# spectral-norm - the Benchmarks Game.  The largest singular value of the
# infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
# found with ten steps of the power method.
#
#      python3 spectralnorm.py <n>

import sys
from math import sqrt


def a(i, j):
    return 1.0 / ((i + j) * (i + j + 1) // 2 + i + 1)


def times(n, inp, out):
    """out = A * inp"""
    for i in range(n):
        total = 0.0
        for j in range(n):
            total += a(i, j) * inp[j]
        out[i] = total


def times_transposed(n, inp, out):
    """out = A transposed * inp"""
    for i in range(n):
        total = 0.0
        for j in range(n):
            total += a(j, i) * inp[j]
        out[i] = total


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    u = [1.0] * n
    v = [0.0] * n
    t = [0.0] * n

    for _ in range(10):
        times(n, u, t)              # v = At A u
        times_transposed(n, t, v)
        times(n, v, t)              # u = At A v
        times_transposed(n, t, u)

    vbv = vv = 0.0
    for i in range(n):
        vbv += u[i] * v[i]
        vv += v[i] * v[i]
    print("%0.9f" % sqrt(vbv / vv))


main()
