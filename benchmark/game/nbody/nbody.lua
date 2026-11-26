-- n-body - the Benchmarks Game.  The sun and the four big planets, moved
-- `steps` times by a simple symplectic integrator; prints the energy of the
-- system before and after.
--
--      lua nbody.lua <steps>        (Lua 5.4 or LuaJIT)

local sqrt = math.sqrt

local PI = 3.141592653589793
local SOLAR_MASS = 4 * PI * PI
local DAYS_PER_YEAR = 365.24
local BODIES = 5

local x = { 0.0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01 }
local y = { 0.0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01 }
local z = { 0.0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01 }
local vx = { 0.0, 1.66007664274403694e-03 * DAYS_PER_YEAR, -2.76742510726862411e-03 * DAYS_PER_YEAR, 2.96460137564761618e-03 * DAYS_PER_YEAR, 2.68067772490389322e-03 * DAYS_PER_YEAR }
local vy = { 0.0, 7.69901118419740425e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR }
local vz = { 0.0, -6.90460016972063023e-05 * DAYS_PER_YEAR, 2.30417297573763929e-05 * DAYS_PER_YEAR, -2.96589568540237556e-05 * DAYS_PER_YEAR, -9.51592254519715870e-05 * DAYS_PER_YEAR }
local mass = { SOLAR_MASS, 9.54791938424326609e-04 * SOLAR_MASS, 2.85885980666130812e-04 * SOLAR_MASS, 4.36624404335156298e-05 * SOLAR_MASS, 5.15138902046611451e-05 * SOLAR_MASS }

local function offset_momentum()
    local px, py, pz = 0.0, 0.0, 0.0
    for i = 1, BODIES do
        px = px + vx[i] * mass[i]
        py = py + vy[i] * mass[i]
        pz = pz + vz[i] * mass[i]
    end
    vx[1] = -px / SOLAR_MASS
    vy[1] = -py / SOLAR_MASS
    vz[1] = -pz / SOLAR_MASS
end

local function energy()
    local e = 0.0
    for i = 1, BODIES do
        e = e + 0.5 * mass[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i])
        for j = i + 1, BODIES do
            local dx, dy, dz = x[i] - x[j], y[i] - y[j], z[i] - z[j]
            e = e - (mass[i] * mass[j]) / sqrt(dx * dx + dy * dy + dz * dz)
        end
    end
    return e
end

local function advance(dt)
    for i = 1, BODIES do
        for j = i + 1, BODIES do
            local dx, dy, dz = x[i] - x[j], y[i] - y[j], z[i] - z[j]
            local distance2 = dx * dx + dy * dy + dz * dz
            local mag = dt / (distance2 * sqrt(distance2))
            vx[i] = vx[i] - dx * mass[j] * mag
            vy[i] = vy[i] - dy * mass[j] * mag
            vz[i] = vz[i] - dz * mass[j] * mag
            vx[j] = vx[j] + dx * mass[i] * mag
            vy[j] = vy[j] + dy * mass[i] * mag
            vz[j] = vz[j] + dz * mass[i] * mag
        end
    end
    for i = 1, BODIES do
        x[i] = x[i] + dt * vx[i]
        y[i] = y[i] + dt * vy[i]
        z[i] = z[i] + dt * vz[i]
    end
end

local steps = tonumber(arg and arg[1]) or 1000

offset_momentum()
io.write(string.format("%.9f\n", energy()))
for _ = 1, steps do advance(0.01) end
io.write(string.format("%.9f\n", energy()))
