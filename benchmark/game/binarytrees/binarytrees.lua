-- binary-trees - the Benchmarks Game.  Makes and throws away a great many
-- complete binary trees, one node a table: a test of the garbage collector.
--
--      lua binarytrees.lua <depth>        (Lua 5.4 or LuaJIT)

local function make(depth)
    if depth > 0 then
        local left = make(depth - 1)
        local right = make(depth - 1)
        return { left = left, right = right }
    end
    return {}
end

-- how many nodes the tree has
local function check(n)
    if n.left == nil then return 1 end
    return 1 + check(n.left) + check(n.right)
end

local n = tonumber(arg and arg[1]) or 10
local min_depth = 4
local max_depth = n > min_depth + 2 and n or min_depth + 2
local stretch_depth = max_depth + 1

io.write(string.format("stretch tree of depth %d\t check: %d\n", stretch_depth, check(make(stretch_depth))))

local long_lived = make(max_depth)

for depth = min_depth, max_depth, 2 do
    local iterations = 2 ^ (max_depth - depth + min_depth)
    local total = 0
    for _ = 1, iterations do
        total = total + check(make(depth))
    end
    io.write(string.format("%d\t trees of depth %d\t check: %d\n", iterations, depth, total))
end

io.write(string.format("long lived tree of depth %d\t check: %d\n", max_depth, check(long_lived)))
