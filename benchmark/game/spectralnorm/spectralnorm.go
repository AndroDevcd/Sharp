// spectral-norm - the Benchmarks Game.  The largest singular value of the
// infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
// found with ten steps of the power method.
//
//	spectralnorm <n>
package main

import (
	"fmt"
	"math"
	"os"
	"strconv"
)

func a(i, j int) float64 {
	return 1.0 / float64((i+j)*(i+j+1)/2+i+1)
}

// out = A * in
func times(n int, in, out []float64) {
	for i := 0; i < n; i++ {
		sum := 0.0
		for j := 0; j < n; j++ {
			sum += a(i, j) * in[j]
		}
		out[i] = sum
	}
}

// out = A transposed * in
func timesTransposed(n int, in, out []float64) {
	for i := 0; i < n; i++ {
		sum := 0.0
		for j := 0; j < n; j++ {
			sum += a(j, i) * in[j]
		}
		out[i] = sum
	}
}

func main() {
	n := 100
	if len(os.Args) > 1 {
		n, _ = strconv.Atoi(os.Args[1])
	}
	u := make([]float64, n)
	v := make([]float64, n)
	t := make([]float64, n)

	for i := 0; i < n; i++ {
		u[i] = 1
	}
	for i := 0; i < 10; i++ {
		times(n, u, t)
		timesTransposed(n, t, v) // v = At A u
		times(n, v, t)
		timesTransposed(n, t, u) // u = At A v
	}

	vbv, vv := 0.0, 0.0
	for i := 0; i < n; i++ {
		vbv += u[i] * v[i]
		vv += v[i] * v[i]
	}
	fmt.Printf("%0.9f\n", math.Sqrt(vbv/vv))
}
