#!/usr/bin/env python3
"""
The compiler carries a copy of the VM's sni.h, so `sharpc -sni <folder>` can
write it next to the generated files without having to find an installed one.

    lib/runtime/sni/sni.h   ->   lib/compiler/backend/finalizer/sni/sni_header.sharp

Run this after every change of sni.h:      python build-scripts/internal/embed_sni_header.py
With --check it only says whether the copy is up to date (exit 1 when not).
"""
import os, sys

here = os.path.dirname(os.path.abspath(__file__))
source = os.path.join(here, "..", "..", "lib", "runtime", "sni", "sni.h")
target = os.path.join(here, "..", "..", "lib", "compiler", "backend", "finalizer", "sni", "sni_header.sharp")

lines = open(source, encoding="utf-8").read().split("\n")
if lines and lines[-1] == "":
    lines.pop()

out = []
out.append("mod backend.finalizer.sni;")
out.append("")
out.append("import (")
out.append("    std.io")
out.append(")")
out.append("")
out.append("/**")
out.append(" * sni.h, THE INTERFACE A NATIVE LIBRARY IS WRITTEN AGAINST - as text.")
out.append(" *")
out.append(" * A COPY of lib/runtime/sni/sni.h, made by build-scripts/internal/embed_sni_header.py.")
out.append(" * Never edit this file: change sni.h and run the script again.")
out.append(" */")
out.append("def sni_header_text() : string {")
out.append("    text := new string_builder(\"\");")
for line in lines:
    assert "$" not in line, "a `$` would start an interpolation in a Sharp string: " + line
    escaped = line.replace("\\", "\\\\").replace("\"", "\\\"")
    out.append("    text.append(\"" + escaped + "\\n\");")
out.append("    return text.build();")
out.append("}")
made = "\n".join(out) + "\n"

if "--check" in sys.argv:
    current = open(target, encoding="utf-8").read() if os.path.exists(target) else ""
    if current.replace("\r\n", "\n") != made:
        print("sni_header.sharp is not up to date with sni.h: run build-scripts/internal/embed_sni_header.py")
        sys.exit(1)
    print("sni_header.sharp is up to date")
else:
    open(target, "w", encoding="utf-8", newline="\n").write(made)
    print("wrote", os.path.normpath(target), "(" + str(len(lines)) + " lines)")
