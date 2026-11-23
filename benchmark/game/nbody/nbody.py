# n-body - the Benchmarks Game.  The sun and the four big planets, moved
# `steps` times by a simple symplectic integrator; prints the energy of the
# system before and after.
#
#      python3 nbody.py <steps>

import sys
from math import sqrt

PI = 3.141592653589793
SOLAR_MASS = 4 * PI * PI
DAYS_PER_YEAR = 365.24
BODIES = 5

x = [0.0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01]
y = [0.0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01]
z = [0.0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01]
vx = [0.0, 1.66007664274403694e-03 * DAYS_PER_YEAR, -2.76742510726862411e-03 * DAYS_PER_YEAR, 2.96460137564761618e-03 * DAYS_PER_YEAR, 2.68067772490389322e-03 * DAYS_PER_YEAR]
vy = [0.0, 7.69901118419740425e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR]
vz = [0.0, -6.90460016972063023e-05 * DAYS_PER_YEAR, 2.30417297573763929e-05 * DAYS_PER_YEAR, -2.96589568540237556e-05 * DAYS_PER_YEAR, -9.51592254519715870e-05 * DAYS_PER_YEAR]
mass = [SOLAR_MASS, 9.54791938424326609e-04 * SOLAR_MASS, 2.85885980666130812e-04 * SOLAR_MASS, 4.36624404335156298e-05 * SOLAR_MASS, 5.15138902046611451e-05 * SOLAR_MASS]


def offset_momentum():
    px = py = pz = 0.0
    for i in range(BODIES):
        px += vx[i] * mass[i]
        py += vy[i] * mass[i]
        pz += vz[i] * mass[i]
    vx[0] = -px / SOLAR_MASS
    vy[0] = -py / SOLAR_MASS
    vz[0] = -pz / SOLAR_MASS


def energy():
    e = 0.0
    for i in range(BODIES):
        e += 0.5 * mass[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i])
        for j in range(i + 1, BODIES):
            dx = x[i] - x[j]
            dy = y[i] - y[j]
            dz = z[i] - z[j]
            e -= (mass[i] * mass[j]) / sqrt(dx * dx + dy * dy + dz * dz)
    return e


def advance(dt):
    for i in range(BODIES):
        for j in range(i + 1, BODIES):
            dx = x[i] - x[j]
            dy = y[i] - y[j]
            dz = z[i] - z[j]
            distance2 = dx * dx + dy * dy + dz * dz
            mag = dt / (distance2 * sqrt(distance2))
            vx[i] -= dx * mass[j] * mag
            vy[i] -= dy * mass[j] * mag
            vz[i] -= dz * mass[j] * mag
            vx[j] += dx * mass[i] * mag
            vy[j] += dy * mass[i] * mag
            vz[j] += dz * mass[i] * mag
    for i in range(BODIES):
        x[i] += dt * vx[i]
        y[i] += dt * vy[i]
        z[i] += dt * vz[i]


def main():
    steps = int(sys.argv[1]) if len(sys.argv) > 1 else 1000

    offset_momentum()
    print("%.9f" % energy())
    for _ in range(steps):
        advance(0.01)
    print("%.9f" % energy())


main()
