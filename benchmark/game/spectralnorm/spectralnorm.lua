-- spectral-norm - the Benchmarks Game.  The largest singular value of the
-- infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
-- found with ten steps of the power method.
--
--      lua spectralnorm.lua <n>        (Lua 5.4 or LuaJIT)

-- i and j count from 0, as in the formula
local function a(i, j)
    return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1)
end

-- out = A * inp
local function times(n, inp, out)
    for i = 1, n do
        local sum = 0.0
        for j = 1, n do sum = sum + a(i - 1, j - 1) * inp[j] end
        out[i] = sum
    end
end

-- out = A transposed * inp
local function times_transposed(n, inp, out)
    for i = 1, n do
        local sum = 0.0
        for j = 1, n do sum = sum + a(j - 1, i - 1) * inp[j] end
        out[i] = sum
    end
end

local n = tonumber(arg and arg[1]) or 100
local u, v, t = {}, {}, {}

for i = 1, n do u[i], v[i], t[i] = 1.0, 0.0, 0.0 end
for _ = 1, 10 do
    times(n, u, t) times_transposed(n, t, v)        -- v = At A u
    times(n, v, t) times_transposed(n, t, u)        -- u = At A v
end

local vbv, vv = 0.0, 0.0
for i = 1, n do
    vbv = vbv + u[i] * v[i]
    vv = vv + v[i] * v[i]
end
io.write(string.format("%0.9f\n", math.sqrt(vbv / vv)))
