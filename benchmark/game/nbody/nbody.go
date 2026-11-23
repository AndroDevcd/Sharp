// n-body - the Benchmarks Game.  The sun and the four big planets, moved
// `steps` times by a simple symplectic integrator; prints the energy of the
// system before and after.
//
//	nbody <steps>
package main

import (
	"fmt"
	"math"
	"os"
	"strconv"
)

const bodies = 5

// Variables, not constants: Go works constants out with unlimited precision,
// and the numbers below must be multiplied as 64-bit doubles like in C.
var (
	pi          = 3.141592653589793
	solarMass   = 4 * pi * pi
	daysPerYear = 365.24
)

var x = [bodies]float64{0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01}
var y = [bodies]float64{0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01}
var z = [bodies]float64{0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01}
var vx = [bodies]float64{0, 1.66007664274403694e-03 * daysPerYear, -2.76742510726862411e-03 * daysPerYear, 2.96460137564761618e-03 * daysPerYear, 2.68067772490389322e-03 * daysPerYear}
var vy = [bodies]float64{0, 7.69901118419740425e-03 * daysPerYear, 4.99852801234917238e-03 * daysPerYear, 2.37847173959480950e-03 * daysPerYear, 1.62824170038242295e-03 * daysPerYear}
var vz = [bodies]float64{0, -6.90460016972063023e-05 * daysPerYear, 2.30417297573763929e-05 * daysPerYear, -2.96589568540237556e-05 * daysPerYear, -9.51592254519715870e-05 * daysPerYear}
var mass = [bodies]float64{solarMass, 9.54791938424326609e-04 * solarMass, 2.85885980666130812e-04 * solarMass, 4.36624404335156298e-05 * solarMass, 5.15138902046611451e-05 * solarMass}

func offsetMomentum() {
	px, py, pz := 0.0, 0.0, 0.0
	for i := 0; i < bodies; i++ {
		px += vx[i] * mass[i]
		py += vy[i] * mass[i]
		pz += vz[i] * mass[i]
	}
	vx[0] = -px / solarMass
	vy[0] = -py / solarMass
	vz[0] = -pz / solarMass
}

func energy() float64 {
	e := 0.0
	for i := 0; i < bodies; i++ {
		e += 0.5 * mass[i] * (vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i])
		for j := i + 1; j < bodies; j++ {
			dx, dy, dz := x[i]-x[j], y[i]-y[j], z[i]-z[j]
			e -= (mass[i] * mass[j]) / math.Sqrt(dx*dx+dy*dy+dz*dz)
		}
	}
	return e
}

func advance(dt float64) {
	for i := 0; i < bodies; i++ {
		for j := i + 1; j < bodies; j++ {
			dx, dy, dz := x[i]-x[j], y[i]-y[j], z[i]-z[j]
			distance2 := dx*dx + dy*dy + dz*dz
			mag := dt / (distance2 * math.Sqrt(distance2))
			vx[i] -= dx * mass[j] * mag
			vy[i] -= dy * mass[j] * mag
			vz[i] -= dz * mass[j] * mag
			vx[j] += dx * mass[i] * mag
			vy[j] += dy * mass[i] * mag
			vz[j] += dz * mass[i] * mag
		}
	}
	for i := 0; i < bodies; i++ {
		x[i] += dt * vx[i]
		y[i] += dt * vy[i]
		z[i] += dt * vz[i]
	}
}

func main() {
	steps := 1000
	if len(os.Args) > 1 {
		steps, _ = strconv.Atoi(os.Args[1])
	}

	offsetMomentum()
	fmt.Printf("%.9f\n", energy())
	for i := 0; i < steps; i++ {
		advance(0.01)
	}
	fmt.Printf("%.9f\n", energy())
}
