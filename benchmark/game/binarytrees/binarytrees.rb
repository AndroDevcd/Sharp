# binary-trees - the Benchmarks Game.  Makes and throws away a great many
# complete binary trees, one node an object: a test of the garbage collector.
# A node is a two-element array, [left, right]; a leaf is [nil, nil].
#
#      ruby binarytrees.rb <depth>

def make(depth)
  if depth > 0
    [make(depth - 1), make(depth - 1)]
  else
    [nil, nil]
  end
end

# how many nodes the tree has
def check(node)
  node[0].nil? ? 1 : 1 + check(node[0]) + check(node[1])
end

n = ARGV.empty? ? 10 : ARGV[0].to_i
min_depth = 4
max_depth = n > min_depth + 2 ? n : min_depth + 2
stretch_depth = max_depth + 1

stretch = make(stretch_depth)
print "stretch tree of depth #{stretch_depth}\t check: #{check(stretch)}\n"
stretch = nil

long_lived = make(max_depth)

depth = min_depth
while depth <= max_depth
  iterations = 1 << (max_depth - depth + min_depth)
  total = 0
  iterations.times do
    tree = make(depth)
    total += check(tree)
  end
  print "#{iterations}\t trees of depth #{depth}\t check: #{total}\n"
  depth += 2
end

print "long lived tree of depth #{max_depth}\t check: #{check(long_lived)}\n"
