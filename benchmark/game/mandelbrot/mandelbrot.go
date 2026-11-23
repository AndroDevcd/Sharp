// mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
// [-1, 1] as an n x n black and white picture, written to the standard output
// as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
// whole byte.
//
//	mandelbrot <n>  > picture.pbm
package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
)

func main() {
	n := 200
	if len(os.Args) > 1 {
		n, _ = strconv.Atoi(os.Args[1])
	}
	rowBytes := (n + 7) / 8
	row := make([]byte, rowBytes)
	out := bufio.NewWriter(os.Stdout)
	defer out.Flush()

	fmt.Fprintf(out, "P4\n%d %d\n", n, n)
	for y := 0; y < n; y++ {
		ci := 2.0*float64(y)/float64(n) - 1.0
		bits, at := 0, 0
		for x := 0; x < n; x++ {
			cr := 2.0*float64(x)/float64(n) - 1.5
			zr, zi, tr, ti := 0.0, 0.0, 0.0, 0.0
			for i := 0; i < 50 && tr+ti <= 4.0; i++ {
				zi = 2.0*zr*zi + ci
				zr = tr - ti + cr
				tr = zr * zr
				ti = zi * zi
			}
			bits = bits * 2
			if tr+ti <= 4.0 {
				bits++
			}
			if x%8 == 7 {
				row[at] = byte(bits)
				at++
				bits = 0
			}
		}
		if n%8 != 0 {
			// the last byte of the row: the bits that are there, moved to its top
			for k := n % 8; k < 8; k++ {
				bits = bits * 2
			}
			row[at] = byte(bits)
		}
		out.Write(row)
	}
}
