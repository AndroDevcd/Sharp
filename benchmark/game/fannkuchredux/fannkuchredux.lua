-- fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
-- first k numbers, where k is the first number, until the first is 1. Prints
-- a checksum of the flip counts and the most flips any permutation needed.
--
--      lua fannkuchredux.lua <n>        (Lua 5.4 or LuaJIT)

-- The tables count from 1 as Lua tables do; the numbers in a permutation are
-- 0..n-1 as in the C program, so the first k+1 numbers are flipped until the
-- first is 0.
local function fannkuch(n)
    local perm, perm1, count = {}, {}, {}
    local max_flips, checksum, permutations, r = 0, 0, 0, n

    for i = 1, 32 do perm[i], perm1[i], count[i] = 0, 0, 0 end
    for i = 1, n do perm1[i] = i - 1 end

    while true do
        while r ~= 1 do count[r] = r; r = r - 1 end

        for i = 1, n do perm[i] = perm1[i] end
        local flips = 0
        local k = perm[1]
        while k ~= 0 do
            local i, j = 1, k + 1
            while i < j do
                local t = perm[i]; perm[i] = perm[j]; perm[j] = t
                i = i + 1
                j = j - 1
            end
            flips = flips + 1
            k = perm[1]
        end
        if flips > max_flips then max_flips = flips end
        if permutations % 2 == 0 then checksum = checksum + flips else checksum = checksum - flips end

        -- the next permutation
        while true do
            if r == n then
                io.write(string.format("%d\nPfannkuchen(%d) = %d\n", checksum, n, max_flips))
                return
            end
            local first = perm1[1]
            for i = 1, r do perm1[i] = perm1[i + 1] end
            perm1[r + 1] = first
            count[r + 1] = count[r + 1] - 1
            if count[r + 1] > 0 then break end
            r = r + 1
        end
        permutations = permutations + 1
    end
end

fannkuch(tonumber(arg and arg[1]) or 7)
