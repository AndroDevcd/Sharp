-- mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
-- [-1, 1] as an n x n black and white picture, written to the standard output
-- as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
-- whole byte.
--
--      lua mandelbrot.lua <n>  > picture.pbm        (Lua 5.4 or LuaJIT)

local char, concat, write = string.char, table.concat, io.write

local n = tonumber(arg and arg[1]) or 200
local row = {}

write(string.format("P4\n%d %d\n", n, n))
for y = 0, n - 1 do
    local ci = 2.0 * y / n - 1.0
    local bits, at = 0, 0
    for x = 0, n - 1 do
        local cr = 2.0 * x / n - 1.5
        local zr, zi, tr, ti = 0.0, 0.0, 0.0, 0.0
        local i = 0
        while i < 50 and tr + ti <= 4.0 do
            zi = 2.0 * zr * zi + ci
            zr = tr - ti + cr
            tr = zr * zr
            ti = zi * zi
            i = i + 1
        end
        bits = bits * 2 + (tr + ti <= 4.0 and 1 or 0)
        if x % 8 == 7 then
            at = at + 1
            row[at] = char(bits)
            bits = 0
        end
    end
    if n % 8 ~= 0 then
        -- the last byte of the row: the bits that are there, moved to its top
        for _ = n % 8, 7 do bits = bits * 2 end
        at = at + 1
        row[at] = char(bits)
    end
    write(concat(row, "", 1, at))
end
