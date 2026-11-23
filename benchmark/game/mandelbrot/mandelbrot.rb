# mandelbrot - the Benchmarks Game.  The Mandelbrot set on [-1.5, 0.5] x
# [-1, 1] as an n x n black and white picture, written to the standard output
# as a portable bitmap (PBM, "P4"): one bit a point, each row filled up to a
# whole byte.
#
#      ruby mandelbrot.rb <n>  > picture.pbm

def mandelbrot(n)
  $stdout.binmode
  $stdout.write("P4\n#{n} #{n}\n")

  y = 0
  while y < n
    ci = 2.0 * y / n - 1.0
    row = []
    bits = 0
    x = 0
    while x < n
      cr = 2.0 * x / n - 1.5
      zr = zi = tr = ti = 0.0
      i = 0
      while i < 50 && tr + ti <= 4.0
        zi = 2.0 * zr * zi + ci
        zr = tr - ti + cr
        tr = zr * zr
        ti = zi * zi
        i += 1
      end
      bits = bits * 2 + (tr + ti <= 4.0 ? 1 : 0)
      if x % 8 == 7
        row << bits
        bits = 0
      end
      x += 1
    end
    if n % 8 != 0
      # the last byte of the row: the bits that are there, moved to its top
      k = n % 8
      while k < 8
        bits = bits * 2
        k += 1
      end
      row << bits
    end
    $stdout.write(row.pack('C*'))
    y += 1
  end
end

mandelbrot(ARGV.empty? ? 200 : ARGV[0].to_i)
