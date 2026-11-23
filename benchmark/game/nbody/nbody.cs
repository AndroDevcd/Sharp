/*
 * n-body - the Benchmarks Game.  The sun and the four big planets, moved
 * `steps` times by a simple symplectic integrator; prints the energy of the
 * system before and after.
 *
 *      mono nbody.exe <steps>
 */
using System;
using System.Globalization;

static class NBody {
    const double PI = 3.141592653589793;
    const double SolarMass = 4 * PI * PI;
    const double DaysPerYear = 365.24;
    const int Bodies = 5;

    static double[] x  = { 0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01 };
    static double[] y  = { 0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01 };
    static double[] z  = { 0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01 };
    static double[] vx = { 0, 1.66007664274403694e-03 * DaysPerYear, -2.76742510726862411e-03 * DaysPerYear, 2.96460137564761618e-03 * DaysPerYear, 2.68067772490389322e-03 * DaysPerYear };
    static double[] vy = { 0, 7.69901118419740425e-03 * DaysPerYear, 4.99852801234917238e-03 * DaysPerYear, 2.37847173959480950e-03 * DaysPerYear, 1.62824170038242295e-03 * DaysPerYear };
    static double[] vz = { 0, -6.90460016972063023e-05 * DaysPerYear, 2.30417297573763929e-05 * DaysPerYear, -2.96589568540237556e-05 * DaysPerYear, -9.51592254519715870e-05 * DaysPerYear };
    static double[] mass = { SolarMass, 9.54791938424326609e-04 * SolarMass, 2.85885980666130812e-04 * SolarMass, 4.36624404335156298e-05 * SolarMass, 5.15138902046611451e-05 * SolarMass };

    static void OffsetMomentum() {
        double px = 0, py = 0, pz = 0;
        for (int i = 0; i < Bodies; i++) {
            px += vx[i] * mass[i];
            py += vy[i] * mass[i];
            pz += vz[i] * mass[i];
        }
        vx[0] = -px / SolarMass;
        vy[0] = -py / SolarMass;
        vz[0] = -pz / SolarMass;
    }

    static double Energy() {
        double e = 0;
        for (int i = 0; i < Bodies; i++) {
            e += 0.5 * mass[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
            for (int j = i + 1; j < Bodies; j++) {
                double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
                e -= (mass[i] * mass[j]) / Math.Sqrt(dx * dx + dy * dy + dz * dz);
            }
        }
        return e;
    }

    static void Advance(double dt) {
        for (int i = 0; i < Bodies; i++) {
            for (int j = i + 1; j < Bodies; j++) {
                double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
                double distance2 = dx * dx + dy * dy + dz * dz;
                double mag = dt / (distance2 * Math.Sqrt(distance2));
                vx[i] -= dx * mass[j] * mag;
                vy[i] -= dy * mass[j] * mag;
                vz[i] -= dz * mass[j] * mag;
                vx[j] += dx * mass[i] * mag;
                vy[j] += dy * mass[i] * mag;
                vz[j] += dz * mass[i] * mag;
            }
        }
        for (int i = 0; i < Bodies; i++) {
            x[i] += dt * vx[i];
            y[i] += dt * vy[i];
            z[i] += dt * vz[i];
        }
    }

    static void Main(string[] args) {
        int steps = args.Length > 0 ? int.Parse(args[0]) : 1000;

        OffsetMomentum();
        Console.Write(Energy().ToString("F9", CultureInfo.InvariantCulture) + "\n");
        for (int i = 0; i < steps; i++) Advance(0.01);
        Console.Write(Energy().ToString("F9", CultureInfo.InvariantCulture) + "\n");
    }
}
