// n-body - the Benchmarks Game.  The sun and the four big planets, moved
// `steps` times by a simple symplectic integrator; prints the energy of the
// system before and after.
//
//      nbody <steps>

const PI: f64 = 3.141592653589793;
const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS_PER_YEAR: f64 = 365.24;
const BODIES: usize = 5;

struct System {
    x: [f64; BODIES],
    y: [f64; BODIES],
    z: [f64; BODIES],
    vx: [f64; BODIES],
    vy: [f64; BODIES],
    vz: [f64; BODIES],
    mass: [f64; BODIES],
}

fn offset_momentum(s: &mut System) {
    let (mut px, mut py, mut pz) = (0.0, 0.0, 0.0);
    for i in 0..BODIES {
        px += s.vx[i] * s.mass[i];
        py += s.vy[i] * s.mass[i];
        pz += s.vz[i] * s.mass[i];
    }
    s.vx[0] = -px / SOLAR_MASS;
    s.vy[0] = -py / SOLAR_MASS;
    s.vz[0] = -pz / SOLAR_MASS;
}

fn energy(s: &System) -> f64 {
    let mut e = 0.0;
    for i in 0..BODIES {
        e += 0.5 * s.mass[i] * (s.vx[i] * s.vx[i] + s.vy[i] * s.vy[i] + s.vz[i] * s.vz[i]);
        for j in i + 1..BODIES {
            let dx = s.x[i] - s.x[j];
            let dy = s.y[i] - s.y[j];
            let dz = s.z[i] - s.z[j];
            e -= (s.mass[i] * s.mass[j]) / (dx * dx + dy * dy + dz * dz).sqrt();
        }
    }
    e
}

fn advance(s: &mut System, dt: f64) {
    for i in 0..BODIES {
        for j in i + 1..BODIES {
            let dx = s.x[i] - s.x[j];
            let dy = s.y[i] - s.y[j];
            let dz = s.z[i] - s.z[j];
            let distance2 = dx * dx + dy * dy + dz * dz;
            let mag = dt / (distance2 * distance2.sqrt());
            s.vx[i] -= dx * s.mass[j] * mag;
            s.vy[i] -= dy * s.mass[j] * mag;
            s.vz[i] -= dz * s.mass[j] * mag;
            s.vx[j] += dx * s.mass[i] * mag;
            s.vy[j] += dy * s.mass[i] * mag;
            s.vz[j] += dz * s.mass[i] * mag;
        }
    }
    for i in 0..BODIES {
        s.x[i] += dt * s.vx[i];
        s.y[i] += dt * s.vy[i];
        s.z[i] += dt * s.vz[i];
    }
}

fn main() {
    let steps: i32 = std::env::args().nth(1).and_then(|a| a.parse().ok()).unwrap_or(1000);

    let mut s = System {
        x: [0.0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01],
        y: [0.0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01],
        z: [0.0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01],
        vx: [0.0, 1.66007664274403694e-03 * DAYS_PER_YEAR, -2.76742510726862411e-03 * DAYS_PER_YEAR, 2.96460137564761618e-03 * DAYS_PER_YEAR, 2.68067772490389322e-03 * DAYS_PER_YEAR],
        vy: [0.0, 7.69901118419740425e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR],
        vz: [0.0, -6.90460016972063023e-05 * DAYS_PER_YEAR, 2.30417297573763929e-05 * DAYS_PER_YEAR, -2.96589568540237556e-05 * DAYS_PER_YEAR, -9.51592254519715870e-05 * DAYS_PER_YEAR],
        mass: [SOLAR_MASS, 9.54791938424326609e-04 * SOLAR_MASS, 2.85885980666130812e-04 * SOLAR_MASS, 4.36624404335156298e-05 * SOLAR_MASS, 5.15138902046611451e-05 * SOLAR_MASS],
    };

    offset_momentum(&mut s);
    println!("{:.9}", energy(&s));
    for _ in 0..steps {
        advance(&mut s, 0.01);
    }
    println!("{:.9}", energy(&s));
}
