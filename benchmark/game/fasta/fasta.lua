-- fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
-- 60 letters a line: one by repeating a fixed text, two by drawing letters
-- with given probabilities from a simple random number generator.
--
--      lua fasta.lua <n>  > sequences.txt        (Lua 5.4 or LuaJIT)

local concat, write = table.concat, io.write

local LINE = 60
local IM = 139968
local IA = 3877
local IC = 29573

local last = 42

-- the next random number, from 0 up to (not including) 1
local function next_random()
    last = (last * IA + IC) % IM
    return last / IM
end

local alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" ..
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" ..
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" ..
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" ..
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" ..
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" ..
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"

local iub_letters = "acgtBDHKMNRSVWY"
local iub_chances = { 0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02 }

local human_letters = "acgt"
local human_chances = { 0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008 }

-- the letters of a text, one a table entry
local function split(text)
    local letters = {}
    for i = 1, #text do letters[i] = text:sub(i, i) end
    return letters
end

-- `count` letters of `text`, over and over
local function repeat_text(header, text, count)
    local letters = split(text)
    local length, at = #letters, 1
    local line = {}

    write(header)
    while count > 0 do
        local width = count < LINE and count or LINE
        for i = 1, width do
            line[i] = letters[at]
            at = at + 1
            if at > length then at = 1 end
        end
        line[width + 1] = "\n"
        write(concat(line, "", 1, width + 1))
        count = count - width
    end
end

-- `count` letters, each drawn by its chance
local function draw(header, text, chances, kinds, count)
    local letters = split(text)
    local line = {}

    -- each chance becomes the sum of the chances up to it
    local sum = 0.0
    for i = 1, kinds do sum = sum + chances[i]; chances[i] = sum end

    write(header)
    while count > 0 do
        local width = count < LINE and count or LINE
        for i = 1, width do
            local r = next_random()
            local k = 1
            while k < kinds and chances[k] < r do k = k + 1 end
            line[i] = letters[k]
        end
        line[width + 1] = "\n"
        write(concat(line, "", 1, width + 1))
        count = count - width
    end
end

local n = tonumber(arg and arg[1]) or 1000

repeat_text(">ONE Homo sapiens alu\n", alu, n * 2)
draw(">TWO IUB ambiguity codes\n", iub_letters, iub_chances, 15, n * 3)
draw(">THREE Homo sapiens frequency\n", human_letters, human_chances, 4, n * 5)
