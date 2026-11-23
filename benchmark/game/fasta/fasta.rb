# fasta - the Benchmarks Game.  Writes three DNA sequences in FASTA format,
# 60 letters a line: one by repeating a fixed text, two by drawing letters
# with given probabilities from a simple random number generator.
#
#      ruby fasta.rb <n>  > sequences.txt

LINE = 60
IM = 139968
IA = 3877
IC = 29573

$last = 42

# the next random number, from 0 up to (not including) 1
def next_random
  $last = ($last * IA + IC) % IM
  $last.to_f / IM
end

ALU =
  "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" \
  "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" \
  "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" \
  "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" \
  "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" \
  "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" \
  "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"

IUB_LETTERS = "acgtBDHKMNRSVWY"
IUB_CHANCES = [0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02]

HUMAN_LETTERS = "acgt"
HUMAN_CHANCES = [0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008]

# `count` letters of `text`, over and over
def repeat(header, text, count)
  length = text.length
  at = 0

  $stdout.write(header)
  while count > 0
    width = count < LINE ? count : LINE
    line = String.new
    i = 0
    while i < width
      line << text[at]
      at += 1
      at = 0 if at == length
      i += 1
    end
    line << "\n"
    $stdout.write(line)
    count -= width
  end
end

# `count` letters, each drawn by its chance
def draw(header, letters, chances, count)
  kinds = chances.length

  # each chance becomes the sum of the chances up to it
  sum = 0.0
  chances = chances.map { |chance| sum += chance }

  $stdout.write(header)
  while count > 0
    width = count < LINE ? count : LINE
    line = String.new
    i = 0
    while i < width
      r = next_random
      k = 0
      k += 1 while k < kinds - 1 && chances[k] < r
      line << letters[k]
      i += 1
    end
    line << "\n"
    $stdout.write(line)
    count -= width
  end
end

n = ARGV.empty? ? 1000 : ARGV[0].to_i

repeat(">ONE Homo sapiens alu\n", ALU, n * 2)
draw(">TWO IUB ambiguity codes\n", IUB_LETTERS, IUB_CHANCES, n * 3)
draw(">THREE Homo sapiens frequency\n", HUMAN_LETTERS, HUMAN_CHANCES, n * 5)
