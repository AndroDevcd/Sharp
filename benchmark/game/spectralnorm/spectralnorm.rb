# spectral-norm - the Benchmarks Game.  The largest singular value of the
# infinite matrix A, a(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1), cut to n x n,
# found with ten steps of the power method.
#
#      ruby spectralnorm.rb <n>

def a(i, j)
  1.0 / ((i + j) * (i + j + 1) / 2 + i + 1)
end

# A * input
def times(n, input)
  out = Array.new(n, 0.0)
  i = 0
  while i < n
    sum = 0.0
    j = 0
    while j < n
      sum += a(i, j) * input[j]
      j += 1
    end
    out[i] = sum
    i += 1
  end
  out
end

# A transposed * input
def times_transposed(n, input)
  out = Array.new(n, 0.0)
  i = 0
  while i < n
    sum = 0.0
    j = 0
    while j < n
      sum += a(j, i) * input[j]
      j += 1
    end
    out[i] = sum
    i += 1
  end
  out
end

n = ARGV.empty? ? 100 : ARGV[0].to_i
u = Array.new(n, 1.0)
v = nil

10.times do
  v = times_transposed(n, times(n, u))      # v = At A u
  u = times_transposed(n, times(n, v))      # u = At A v
end

vbv = 0.0
vv = 0.0
n.times do |i|
  vbv += u[i] * v[i]
  vv += v[i] * v[i]
end
printf("%0.9f\n", Math.sqrt(vbv / vv))
