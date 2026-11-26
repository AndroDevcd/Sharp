#!/usr/bin/env python3
"""optimizer_fuzz.py <seed> <out.sharp>: one random Sharp program that stresses the optimizer.

Used by tests/optimizer_fuzz.sh (read its header first). The same seed always gives the same
program. A program prints with show() / show_decimal() of tests/codegen/print.sharp and always
ends: every loop has a constant bound, nothing recurses.

What a program is made of, and the optimizer pass each part is aimed at (doc/codegen.md 7.2):

  constants mixed into every expression          constant propagation, folding, immediates
  `v1 = v0;` and values used after they change   copy propagation, copy coalescing
  loops with constant bounds, break, continue    loop unrolling, loop rotation, jump cleanup
  try / catch / finally around calls that        the rules about `try`: a handler sees every
    raise for some arguments                       slot as it was before the instruction that raised
  small helpers h0 .. h5                         inlining (also a helper with a local set on one
                                                   path only: the copies share their slots)
  objects `o`, `q` used through their fields     box elimination (`q = o;` makes one escape)
  narrow integers, decimals                      narrowing and folding exactly as the VM does it

Things left out ON PURPOSE, because the C++ compiler - which builds the same program for
comparison - gets them wrong or means something else, so they would only make noise:
  `<` and `<=`      written as `>` / `>=` (a `<` after a parenthesis can be read as a generic list);
  literals wider than 32 bits (codegen.md section 10, G3).

Environment: NO_TRY=1 writes no `try` statement (see optimizer_fuzz.sh).
"""
import os
import random
import sys

R = random.Random(int(sys.argv[1]))
NO_TRY = os.environ.get("NO_TRY") == "1"

INTS = ["v0", "v1", "v2", "v3"]                                     # the _int64 locals of a function
NARROW = [("n0", "_int8"), ("n1", "_uint16"), ("n2", "_int32"), ("n3", "_uint8")]
DECS = ["d0", "d1"]                                                 # its decimal locals
LITS = [0, 1, 2, 3, 5, 7, 10, 63, 64, 100, 255, 256, 1000, 65535, 65536, 2147483647, 2147483646, 123456789]
COMPARISONS = [">", ">=", "==", "!="]
HELPERS = 6

# while a helper is written: it sees only its parameters and calls only the helpers before it
in_helper = False
helpers_callable = HELPERS
names_made = [0]


def unique():
    """a number for a name: Sharp wants every local of a function to have its own"""
    names_made[0] += 1
    return names_made[0]


def int_leaf(counters):
    if in_helper:
        return str(R.choice(LITS)) if R.random() < 0.3 else R.choice(["a", "b"])
    c = R.random()
    if c < 0.35:
        n = R.choice(LITS)
        return str(n) if R.random() < 0.8 else "(0 - %d)" % n
    if c < 0.8:
        return R.choice(INTS + ["p0", "p1"] + counters)
    if c < 0.9:
        return R.choice([name for name, _ in NARROW])
    return R.choice(["o.v", "o.s", "q.v"])


def int_expr(depth, counters):
    if depth <= 0 or R.random() < 0.2:
        return int_leaf(counters)
    c = R.random()
    a = int_expr(depth - 1, counters)
    b = int_expr(depth - 1, counters)
    if c < 0.45:
        return "(%s %s %s)" % (a, R.choice(["+", "-", "*", "&", "|", "^", "+", "-"]), b)
    if c < 0.52:
        return "(%s %s (%s & %d))" % (a, R.choice(["<<", ">>"]), b, R.choice([7, 31, 63]))
    if c < 0.60:
        return "(%s %s ((%s & %d) + 1))" % (a, R.choice(["/", "%"]), b, R.choice([3, 15, 255]))    # never by 0
    if c < 0.72:
        return "(%s %s %s)" % (a, R.choice(COMPARISONS), b)
    if c < 0.77:
        return "(!%s)" % a
    if c < 0.84:
        return "(%s ? %s : %s)" % (condition(depth - 1, counters), a, b)
    if c < 0.93:
        return "h%d(%s, %s)" % (R.randrange(helpers_callable), a, b)
    if c < 0.97:
        return "((%s) as %s)" % (a, R.choice(["_int8", "_int16", "_int32", "_uint8", "_uint16", "_uint32"]))
    return "(%s %s %s)" % (dec_expr(1, counters), R.choice(COMPARISONS), dec_expr(1, counters))


def condition(depth, counters):
    c = R.random()
    if depth <= 0 or c < 0.5:
        return "(%s %s %s)" % (int_expr(1, counters), R.choice(COMPARISONS), int_expr(1, counters))
    if c < 0.7:
        return "(%s && %s)" % (condition(depth - 1, counters), condition(depth - 1, counters))
    if c < 0.9:
        return "(%s || %s)" % (condition(depth - 1, counters), condition(depth - 1, counters))
    return "(!%s)" % condition(depth - 1, counters)


def dec_expr(depth, counters):
    if in_helper:
        return R.choice(["0.5", "1.5", "a", "b"])
    if depth <= 0 or R.random() < 0.3:
        c = R.random()
        if c < 0.4:
            return R.choice(["0.5", "1.5", "0.1", "2.0", "3.25", "100.0", "0.0", "1e3"])
        if c < 0.8:
            return R.choice(DECS + ["p2", "o.d"])
        return R.choice(INTS)
    a = dec_expr(depth - 1, counters)
    b = dec_expr(depth - 1, counters)
    return "(%s %s %s)" % (a, R.choice(["+", "-", "*", "+", "-"]), b)


def statements(count, depth, counters, in_loop, indent):
    lines = []
    for _ in range(count):
        lines.extend(statement(depth, counters, in_loop, indent))
    return lines


def statement(depth, counters, in_loop, indent):
    c = R.random()
    pad = "    " * indent
    if c < 0.30 or depth <= 0:                  # a whole number local changes
        target = R.choice(INTS)
        c2 = R.random()
        if c2 < 0.6:
            return ["%s%s = %s;" % (pad, target, int_expr(3, counters))]
        if c2 < 0.7:
            return ["%s%s%s;" % (pad, target, R.choice(["++", "--"]))]
        if c2 < 0.85:
            return ["%s%s = %s;" % (pad, target, R.choice(INTS))]
        return ["%s%s %s %s;" % (pad, target, R.choice(["+=", "-=", "*="]), int_expr(2, counters))]
    if c < 0.36:                                # a narrow local
        name, kind = R.choice(NARROW)
        return ["%s%s = (%s) as %s;" % (pad, name, int_expr(2, counters), kind)]
    if c < 0.44:                                # a decimal local, kept in a range so it never becomes infinite
        target = R.choice(DECS)
        return ["%s%s = %s;" % (pad, target, dec_expr(2, counters)),
                "%sif(%s > 1e6 || %s < 0 - 1e6) { %s = 0.25; }" % (pad, target, target, target)]
    if c < 0.50:                                # a decimal cut to a whole number
        return ["%s%s = (%s) as _int64;" % (pad, R.choice(INTS), dec_expr(2, counters))]
    if c < 0.58:                                # the objects
        c2 = R.random()
        if c2 < 0.3:
            return ["%so.v = %s;" % (pad, int_expr(2, counters))]
        if c2 < 0.5:
            return ["%so.s = (%s) as _int16;" % (pad, int_expr(2, counters))]
        if c2 < 0.6:
            return ["%so.d = %s;" % (pad, R.choice(DECS))]
        if c2 < 0.8:
            return ["%sq.v = %s;" % (pad, int_expr(2, counters))]
        if c2 < 0.9:
            return ["%so = new box();" % pad] if R.random() < 0.5 else ["%sq = o;" % pad]
        return ["%sshared.v = shared.v + %s;" % (pad, int_expr(1, counters))]
    if c < 0.64:
        return ["%sshow(%s);" % (pad, int_expr(2, counters))]
    if c < 0.76:                                # if / else
        lines = ["%sif%s {" % (pad, condition(2, counters))]
        lines += statements(R.randint(1, 3), depth - 1, counters, in_loop, indent + 1)
        if R.random() < 0.5:
            lines.append("%s} else {" % pad)
            lines += statements(R.randint(1, 3), depth - 1, counters, in_loop, indent + 1)
        lines.append("%s}" % pad)
        return lines
    if c < 0.86:                                # a loop with a constant bound
        counter = "c%d" % unique()
        bound = R.randint(0, 6)
        shape = R.random()
        inside = counters + [counter]
        if shape < 0.6:
            lines = ["%sfor(%s : _int64 = 0; %s < %d; %s++) {" % (pad, counter, counter, bound, counter)]
        elif shape < 0.8:
            lines = ["%sfor(%s : _int64 = %d; %s > 0 && %s; %s--) {"
                     % (pad, counter, bound, counter, condition(1, counters), counter)]
        else:
            lines = ["%s%s : _int64 = 0;" % (pad, counter), "%swhile(%s < %d) {" % (pad, counter, bound),
                     "%s    %s++;" % (pad, counter)]
        lines += statements(R.randint(1, 4), depth - 1, inside, True, indent + 1)
        if R.random() < 0.4:
            lines.append("%s    if%s { %s; }" % (pad, condition(1, inside), R.choice(["break", "continue"])))
            lines += statements(R.randint(1, 2), depth - 1, inside, True, indent + 1)
        lines.append("%s}" % pad)
        return lines
    if c < 0.96 and not NO_TRY:                 # try: a call that raises for some arguments in its middle
        lines = ["%stry {" % pad]
        lines += statements(R.randint(1, 3), depth - 1, counters, in_loop, indent + 1)
        lines.append("%s    %s = %s(%s);" % (pad, R.choice(INTS), R.choice(["thrower", "sometimes"]), int_expr(1, counters)))
        lines += statements(R.randint(0, 3), depth - 1, counters, in_loop, indent + 1)
        if R.random() < 0.25:
            lines.append("%s    %s = table[%s & 7];" % (pad, R.choice(INTS), int_expr(1, counters)))    # 6 elements
        has_finally = R.random() < 0.35
        if (not has_finally) or R.random() < 0.7:
            lines.append("%s} catch(e%d: exception) {" % (pad, unique()))
            lines += statements(R.randint(1, 3), depth - 1, counters, in_loop, indent + 1)
        if has_finally:
            lines.append("%s} finally {" % pad)
            lines += statements(R.randint(1, 2), 0, counters, in_loop, indent + 1)
        lines.append("%s}" % pad)
        return lines
    if in_loop and R.random() < 0.3:
        return ["%sif%s { %s; }" % (pad, condition(1, counters), R.choice(["break", "continue"]))]
    return ["%sif%s { return %s; }" % (pad, condition(2, counters), int_expr(1, counters))]


def helper(number):
    """a small function of two numbers: what the inliner copies into its callers"""
    global in_helper, helpers_callable
    if number == 0:
        return "def h0(a: _int64, b: _int64) : _int64 { return a + b; }"
    in_helper = True
    helpers_callable = number
    shape = R.random()
    if shape < 0.5:
        body = "return %s;" % int_expr(2, [])
    elif shape < 0.8:
        body = "if%s { return %s; } return %s;" % (condition(1, []), int_expr(1, []), int_expr(2, []))
    else:
        # a local set on one path only: it must be 0 on the other, in every copy of the body
        body = "t : _int64; if%s { t = %s; } a = a + t; return a ^ %s;" % (condition(1, []), int_expr(1, []), int_expr(1, []))
    in_helper = False
    helpers_callable = HELPERS
    return "def h%d(a: _int64, b: _int64) : _int64 { %s }" % (number, body)


def function(number):
    lines = ["def f%d(p0: _int64, p1: _int64, p2: decimal) : _int64 {" % number]
    for name in INTS:
        lines.append("    %s : _int64 = %s;" % (name, R.choice(["0", "1", "p0", "p1", str(R.choice(LITS))])))
    for name, kind in NARROW:
        lines.append("    %s : %s = 0;" % (name, kind))
    for name in DECS:
        lines.append("    %s : decimal = %s;" % (name, R.choice(["0", "0.5", "p2"])))
    lines += ["    o := new box();", "    q := new box();", "    shared := keeper.shared;", "    table := keeper.table;"]
    lines += statements(R.randint(6, 12), 3, [], False, 1)
    # everything the function worked on is printed: a wrong value anywhere shows
    for name in INTS + [name for name, _ in NARROW]:
        lines.append("    show(%s);" % name)
    for name in DECS:
        lines.append("    show_decimal(%s);" % name)
    lines += ["    show(o.v + o.s + q.v + shared.v);", "    return v0 ^ v1 ^ v2 ^ v3;", "}", ""]
    return lines


out = ["mod main;", "",
       "class box { v : _int64; d : decimal; n : box?; s : _int16; }",
       "class keeper { static shared : box = new box(); static table : _int64[] = new _int64[6]; }", "",
       "def thrower(a: _int64) : _int64 { if(a % 3 == 1) { throw new exception(\"thrown\"); } return a + 1; }",
       "def sometimes(a: _int64) : _int64 { if((a & 5) == 4) { return thrower(a); } return a * 2; }"]
for number in range(HELPERS):
    out.append(helper(number))
out.append("")
FUNCTIONS = 3
for number in range(FUNCTIONS):
    out += function(number)
out.append("def main() {")
out.append("    for(i : _int64 = 0; i < 6; i++) { keeper.table[i] = i * 7 + 1; }")
for number in range(FUNCTIONS):
    for _ in range(4):
        out.append("    try { show(f%d(%d, %d, %s)); } catch(e%d: exception) { show(0 - 424242); }"
                   % (number, R.choice([0, 1, 2, 3, 5, 9, 100, -1, -7]), R.choice([0, 1, 2, 4, 8, 77, -3]),
                      R.choice(["0.5", "2.0", "0.0", "7.25"]), unique()))
out.append("    show(keeper.shared.v);")
out.append("}")
with open(sys.argv[2], "w") as written:
    written.write("\n".join(out) + "\n")
