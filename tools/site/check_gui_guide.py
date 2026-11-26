#!/usr/bin/env python3
"""
THE CODE OF THE GUI GUIDE, COMPILED AND RUN.

    python3 tools/site/check_gui_guide.py <folder>

doc/gui_guide.md shows code. This writes that code out as programs, into <folder>:

    guide_window.sharp      the first program of the guide (the one with gui.run): to be COMPILED
    guide_tests.sharp       every class and function the guide shows, and the tests it shows at its
                            end: to be compiled and RUN - the tests click through the guide's own screens

Then, with the Sharp being worked on:

    bash test.sh try <folder>/guide_tests.sharp         must end "all N tests passed"
    bash test.sh try <folder>/guide_window.sharp        opens a window: compiling it is the check

A block of the guide is taken when it begins with `class`, `def`, `component` or a /** comment and does not
start a window; the block that begins `mod main;` and holds `ui_test(` is the tests.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "guide_check")
    os.makedirs(out, exist_ok=True)
    text = open(os.path.join(ROOT, "doc", "gui_guide.md"), encoding="utf-8").read().replace("\r\n", "\n")
    blocks = re.findall(r"^```javascript\n(.*?)^```", text, flags=re.M | re.S)

    window, parts, tests = None, [], None
    for block in blocks:
        first = block.lstrip().split("\n")[0]
        if "gui.run(" in block:
            window = window or block
        elif first.startswith("mod main;") and "ui_test(" in block:
            tests = block[block.index("def main("):]
        elif re.match(r"^(class |def |interface |component |/\*\*)", first):
            parts.append(block)

    if window is None or tests is None:
        sys.exit("the guide has no program with gui.run, or no tests: nothing was written")
    open(os.path.join(out, "guide_window.sharp"), "w", encoding="utf-8", newline="\n").write(window)
    whole = "mod main;\n\nimport (\n    std,\n    std.di,\n    std.io.coroutines,\n    std.testing,\n    std.guikit.*,\n    std.guikit.testing\n)\n\n" \
            + "\n".join(parts) + "\n" + tests
    open(os.path.join(out, "guide_tests.sharp"), "w", encoding="utf-8", newline="\n").write(whole)
    print("%d blocks of code in the guide: 1 window program, %d classes and functions, the tests" % (len(blocks), len(parts)))
    print("written: %s" % out)


if __name__ == "__main__":
    main()
