/*
 * n-body - the Benchmarks Game.  The sun and the four big planets, moved
 * `steps` times by a simple symplectic integrator; prints the energy of the
 * system before and after.
 *
 *      nbody <steps>
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.141592653589793
#define SOLAR_MASS (4 * PI * PI)
#define DAYS_PER_YEAR 365.24
#define BODIES 5

static double x[BODIES]  = { 0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01 };
static double y[BODIES]  = { 0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01 };
static double z[BODIES]  = { 0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01 };
static double vx[BODIES] = { 0, 1.66007664274403694e-03 * DAYS_PER_YEAR, -2.76742510726862411e-03 * DAYS_PER_YEAR, 2.96460137564761618e-03 * DAYS_PER_YEAR, 2.68067772490389322e-03 * DAYS_PER_YEAR };
static double vy[BODIES] = { 0, 7.69901118419740425e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR };
static double vz[BODIES] = { 0, -6.90460016972063023e-05 * DAYS_PER_YEAR, 2.30417297573763929e-05 * DAYS_PER_YEAR, -2.96589568540237556e-05 * DAYS_PER_YEAR, -9.51592254519715870e-05 * DAYS_PER_YEAR };
static double mass[BODIES] = { SOLAR_MASS, 9.54791938424326609e-04 * SOLAR_MASS, 2.85885980666130812e-04 * SOLAR_MASS, 4.36624404335156298e-05 * SOLAR_MASS, 5.15138902046611451e-05 * SOLAR_MASS };

static void offset_momentum(void) {
    double px = 0, py = 0, pz = 0;
    for (int i = 0; i < BODIES; i++) {
        px += vx[i] * mass[i];
        py += vy[i] * mass[i];
        pz += vz[i] * mass[i];
    }
    vx[0] = -px / SOLAR_MASS;
    vy[0] = -py / SOLAR_MASS;
    vz[0] = -pz / SOLAR_MASS;
}

static double energy(void) {
    double e = 0;
    for (int i = 0; i < BODIES; i++) {
        e += 0.5 * mass[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
        for (int j = i + 1; j < BODIES; j++) {
            double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
            e -= (mass[i] * mass[j]) / sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    return e;
}

static void advance(double dt) {
    for (int i = 0; i < BODIES; i++) {
        for (int j = i + 1; j < BODIES; j++) {
            double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
            double distance2 = dx * dx + dy * dy + dz * dz;
            double mag = dt / (distance2 * sqrt(distance2));
            vx[i] -= dx * mass[j] * mag;
            vy[i] -= dy * mass[j] * mag;
            vz[i] -= dz * mass[j] * mag;
            vx[j] += dx * mass[i] * mag;
            vy[j] += dy * mass[i] * mag;
            vz[j] += dz * mass[i] * mag;
        }
    }
    for (int i = 0; i < BODIES; i++) {
        x[i] += dt * vx[i];
        y[i] += dt * vy[i];
        z[i] += dt * vz[i];
    }
}

int main(int argc, char **argv) {
    int steps = argc > 1 ? atoi(argv[1]) : 1000;

    offset_momentum();
    printf("%.9f\n", energy());
    for (int i = 0; i < steps; i++) advance(0.01);
    printf("%.9f\n", energy());
    return 0;
}
