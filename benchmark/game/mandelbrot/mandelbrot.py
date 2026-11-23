# mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
# [-1, 1] as an n x n black and white picture, written to the standard output
# as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
# whole byte.
#
#      python3 mandelbrot.py <n>  > picture.pbm

import sys


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    row_bytes = (n + 7) // 8
    row = bytearray(row_bytes)
    write = sys.stdout.buffer.write

    write(b"P4\n%d %d\n" % (n, n))
    for y in range(n):
        ci = 2.0 * y / n - 1.0
        bits = 0
        at = 0
        for x in range(n):
            cr = 2.0 * x / n - 1.5
            zr = zi = tr = ti = 0.0
            i = 0
            while i < 50 and tr + ti <= 4.0:
                zi = 2.0 * zr * zi + ci
                zr = tr - ti + cr
                tr = zr * zr
                ti = zi * zi
                i += 1
            bits = bits * 2 + (1 if tr + ti <= 4.0 else 0)
            if x % 8 == 7:
                row[at] = bits
                at += 1
                bits = 0
        if n % 8 != 0:
            # the last byte of the row: the bits that are there, moved to its top
            for _ in range(n % 8, 8):
                bits = bits * 2
            row[at] = bits
            at += 1
        write(row)


main()
