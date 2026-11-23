// fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
// 60 letters a line: one by repeating a fixed text, two by drawing letters
// with given probabilities from a simple random number generator.
//
//	fasta <n>  > sequences.txt
package main

import (
	"bufio"
	"os"
	"strconv"
)

const (
	line = 60
	im   = 139968
	ia   = 3877
	ic   = 29573
)

var last = 42

// the next random number, from 0 up to (not including) 1
func nextRandom() float64 {
	last = (last*ia + ic) % im
	return float64(last) / im
}

const alu = "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" +
	"GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" +
	"CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" +
	"ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" +
	"GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" +
	"AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" +
	"AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"

const iubLetters = "acgtBDHKMNRSVWY"

var iubChances = []float64{0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02}

const humanLetters = "acgt"

var humanChances = []float64{0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008}

// `count` letters of `text`, over and over
func repeat(out *bufio.Writer, header, text string, count int) {
	length, at := len(text), 0
	var buf [line + 1]byte

	out.WriteString(header)
	for count > 0 {
		width := line
		if count < line {
			width = count
		}
		for i := 0; i < width; i++ {
			buf[i] = text[at]
			at++
			if at == length {
				at = 0
			}
		}
		buf[width] = '\n'
		out.Write(buf[:width+1])
		count -= width
	}
}

// `count` letters, each drawn by its chance
func draw(out *bufio.Writer, header, letters string, chances []float64, count int) {
	kinds := len(letters)
	var buf [line + 1]byte

	// each chance becomes the sum of the chances up to it
	sum := 0.0
	for i := 0; i < kinds; i++ {
		sum += chances[i]
		chances[i] = sum
	}

	out.WriteString(header)
	for count > 0 {
		width := line
		if count < line {
			width = count
		}
		for i := 0; i < width; i++ {
			r := nextRandom()
			k := 0
			for k < kinds-1 && chances[k] < r {
				k++
			}
			buf[i] = letters[k]
		}
		buf[width] = '\n'
		out.Write(buf[:width+1])
		count -= width
	}
}

func main() {
	n := 1000
	if len(os.Args) > 1 {
		n, _ = strconv.Atoi(os.Args[1])
	}
	out := bufio.NewWriter(os.Stdout)
	defer out.Flush()

	repeat(out, ">ONE Homo sapiens alu\n", alu, n*2)
	draw(out, ">TWO IUB ambiguity codes\n", iubLetters, iubChances, n*3)
	draw(out, ">THREE Homo sapiens frequency\n", humanLetters, humanChances, n*5)
}
