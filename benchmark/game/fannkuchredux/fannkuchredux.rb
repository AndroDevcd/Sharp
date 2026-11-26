# fannkuch-redux - the Benchmarks Game.  Every permutation of 1..n: flip the
# first k numbers, where k is the first number, until the first is 1. Prints
# a checksum of the flip counts and the most flips any permutation needed.
#
#      ruby fannkuchredux.rb <n>

def fannkuch(n)
  perm = Array.new(n, 0)
  perm1 = Array.new(n) { |i| i }
  count = Array.new(n, 0)
  max_flips = 0
  checksum = 0
  permutations = 0
  r = n

  while true
    while r != 1
      count[r - 1] = r
      r -= 1
    end

    i = 0
    while i < n
      perm[i] = perm1[i]
      i += 1
    end
    flips = 0
    while (k = perm[0]) != 0
      i = 0
      j = k
      while i < j
        t = perm[i]
        perm[i] = perm[j]
        perm[j] = t
        i += 1
        j -= 1
      end
      flips += 1
    end
    max_flips = flips if flips > max_flips
    checksum += permutations % 2 == 0 ? flips : -flips

    # the next permutation
    while true
      if r == n
        print "#{checksum}\nPfannkuchen(#{n}) = #{max_flips}\n"
        return
      end
      first = perm1[0]
      i = 0
      while i < r
        perm1[i] = perm1[i + 1]
        i += 1
      end
      perm1[r] = first
      count[r] -= 1
      break if count[r] > 0
      r += 1
    end
    permutations += 1
  end
end

fannkuch(ARGV.empty? ? 7 : ARGV[0].to_i)
