#!/usr/bin/env python3
"""
Checks of the files the compiler writes for the editor (`-ide <folder>`).

    check.py <case> <folder written by the compiler> <compiler output text file> <source file> [more]

tests/ide.sh runs the compiler and then this, once per case. The cases (doc/ide.md 5.5):

    shapes        a clean program: every kind of declaration and every kind of use
    errors        a declaration error + body errors + a warning: level `full`, the sound function still described
    syntax        a file that does not parse: diagnostics only, no index
    weird         tabs, CRLF line ends, non-ASCII text and an emoji: positions cut the right text out
    corners       a generic function, `mutate`, a nested class, an operator, static members, a one-line function,
                  a lambda in a field's value, a base call, and the text every kind of local can be seen in
    odd           a lone carriage return, bytes that are not UTF-8, an error with a related place in another file
    bodies_both  bodies_for.txt lists one user file and one library file
    bodies_none   bodies_for.txt is empty
    bodies_auto   no bodies_for.txt: every file that is not a library file
    suite_<name>  a file of the language's own test suite (tests/suite): positions cut the right text out

Every check prints `FAIL <case>: what` and the script exits 1 when any failed.
"""
import bisect
import glob
import json
import os
import re
import sys

failures = []


def check(case, condition, message):
    if not condition:
        failures.append(case + ": " + message)
        print("FAIL " + case + ": " + message)


def load(folder, name):
    with open(os.path.join(folder, name), "r", encoding="utf-8") as handle:
        return json.load(handle)


class Run:
    """everything one run wrote"""

    def __init__(self, folder):
        self.folder = folder
        self.ide = load(folder, "ide.json")
        self.diagnostics = load(folder, "diagnostics.json")
        self.types = load(folder, "types.json") if self.ide["types"] else None
        self.declarations = load(folder, "declarations.json") if self.ide["declarations"] else None
        self.bodies = {}
        for entry_index, entry in enumerate(self.ide["files"]):
            if entry["bodies"]:
                self.bodies[entry_index] = load(folder, entry["bodies"])

    def file_index(self, path):
        """the manifest number of a file: by its whole path, else by its name (a program file before a library file)"""
        for index, entry in enumerate(self.ide["files"]):
            if entry["path"] == path:
                return index
        name = "/" + os.path.basename(path)
        for index, entry in enumerate(self.ide["files"]):
            if entry["path"].endswith(name) and not entry["library"]:
                return index
        return -1


def editor_text(path):
    """the file as the editor holds it, as UTF-16 bytes: line ends are \\n, a byte-order mark is gone"""
    with open(path, "rb") as handle:
        data = handle.read()
    text = data.decode("utf-8", "replace").replace("\r\n", "\n")
    if text.startswith("﻿"):
        text = text[1:]
    return text.encode("utf-16-le")


def cut(units, start, end):
    return units[2 * start:2 * end].decode("utf-16-le", "replace")


def line_and_column(units, offset):
    """1-based line and column of an offset by the convention: a line ends at \\n, a column counts UTF-16 units"""
    before = units[:2 * offset].decode("utf-16-le")
    line = before.count("\n") + 1
    column = len(before.split("\n")[-1].encode("utf-16-le")) // 2 + 1
    return line, column


def common(case, run, output_text, sources):
    """what must hold for every run"""
    ide = run.ide
    check(case, ide["format"] == 1, "the manifest's format is not 1")
    ids = {ide["run"], run.diagnostics["run"]}
    if run.types:
        ids.add(run.types["run"])
    if run.declarations:
        ids.add(run.declarations["run"])
    for body in run.bodies.values():
        ids.add(body["run"])
    check(case, len(ids) == 1, "the files of one run carry different run ids: %s" % sorted(ids))
    leftovers = glob.glob(os.path.join(run.folder, "**", "*.tmp"), recursive=True)
    check(case, not leftovers, "temporary files were left behind: %s" % leftovers)

    paths = [entry["path"] for entry in ide["files"]]
    check(case, paths == sorted(paths, key=lambda p: p.encode("utf-8")), "files are not sorted by path")
    for source in sources:
        check(case, any(path.endswith(os.path.basename(source)) for path in paths), "the manifest does not list " + source)

    # the diagnostics are the ones the compiler printed, with the same codes in the same order
    printed = re.findall(r"^(?:error|warning)\[([EW][0-9]+)\]", output_text, re.M)
    written = [d["code"] for d in run.diagnostics["diagnostics"]]
    check(case, written[:len(printed)] == printed, "printed codes %s, written codes %s" % (printed, written))
    check(case, ide["errors"] == sum(1 for d in run.diagnostics["diagnostics"] if d["severity"] == "error"),
          "the manifest's error count is wrong")
    check(case, ide["warnings"] == sum(1 for d in run.diagnostics["diagnostics"] if d["severity"] == "warning"),
          "the manifest's warning count is wrong")

    check(case, isinstance(ide.get("written_in_ms"), int) and ide["written_in_ms"] >= 0, "written_in_ms is missing")
    check(case, run.diagnostics["truncated"] is False, "the list of diagnostics was cut short")

    # every range of a diagnostic and of its related places lies in its file, and BOTH ends agree with the
    # line and column the convention gives; every part of the message is there, as the right kind of value
    for d in run.diagnostics["diagnostics"]:
        check(case, d["severity"] in ("error", "warning") and d["message"] and isinstance(d["label"], str)
              and isinstance(d["notes"], list) and isinstance(d["helps"], list), "diagnostic %s has a missing part" % d["code"])
        check(case, d["severity"] == ("warning" if d["code"].startswith("W") else "error"),
              "the severity of %s is %s" % (d["code"], d["severity"]))
        check(case, d["message"] in output_text, "the message of %s is not what the compiler printed: %r" % (d["code"], d["message"]))
        places = [(d, "diagnostic " + d["code"])] + [(r, "a related place of " + d["code"]) for r in d["related"]]
        for where, what in places:
            check(case, os.path.exists(where["file"]), what + " names a file that does not exist: " + where["file"])
            if os.path.exists(where["file"]):
                check_place(case, editor_text(where["file"]), where, what)


def check_place(case, units, where, what):
    """a place with both forms: the offsets lie in the file, and the line and column of BOTH ends follow from them"""
    length = len(units) // 2
    inside = 0 <= where["from"] <= where["to"] <= length
    check(case, inside, "%s has a range outside the file: %s..%s" % (what, where["from"], where["to"]))
    if not inside:
        return
    start = line_and_column(units, where["from"])
    check(case, (where["line"], where["column"]) == start,
          "%s says line %d column %d, the convention gives %d:%d" % (what, where["line"], where["column"], start[0], start[1]))
    end = line_and_column(units, where["to"])
    check(case, (where["end_line"], where["end_column"]) == end,
          "%s says it ends at %d:%d, the convention gives %d:%d" % (what, where["end_line"], where["end_column"], end[0], end[1]))


def declaration_index(run):
    """(file, node) -> (name, kind word, at) of every declaration in declarations.json (an operator function has the kind word `operator`)"""
    table = {}

    def add_parameters(owner_file, owner):
        for parameter in owner.get("tparams", []):
            table[(owner_file, parameter["node"])] = (parameter["name"], "tparam", parameter["at"])

    def add_member(owner_file, member, kind):
        file_index = member.get("file", owner_file)
        table[(file_index, member["node"])] = (member["name"], kind, member["at"])
        add_parameters(file_index, member)

    def add_class(record):
        file_index = record["file"]
        table[(file_index, record["node"])] = (record["name"], "class", record["at"])
        add_parameters(file_index, record)
        for value in record.get("values", []):
            add_member(file_index, value, "enum value")
        for field in record["fields"]:
            add_member(file_index, field, "field")
        for function in record["functions"] + record["constructors"]:
            add_member(file_index, function, "operator" if function["operator"] else "function")
        for synonym in record["aliases"]:
            add_member(file_index, synonym, "alias")
        for nested in record["classes"]:
            add_class(nested)

    for module in run.declarations["modules"]:
        for record in module["classes"]:
            add_class(record)
        for field in module["globals"]:
            add_member(-1, field, "field")
        for function in module["functions"]:
            add_member(-1, function, "operator" if function["operator"] else "function")
        for synonym in module["aliases"]:
            add_member(-1, synonym, "alias")
    return table


def class_records(run):
    """every class record of declarations.json, the nested ones included"""
    found = []

    def walk(record):
        found.append(record)
        for nested in record["classes"]:
            walk(nested)

    for module in run.declarations["modules"]:
        for record in module["classes"]:
            walk(record)
    return found


class SourceText:
    """one file as the editor holds it, with the offsets at which its lines start (found once: a file has many declarations)"""

    def __init__(self, path):
        self.units = editor_text(path)
        self.starts = [0]
        position = 0
        for piece in self.units.decode("utf-16-le", "replace").split("\n")[:-1]:
            position += len(piece.encode("utf-16-le")) // 2 + 1
            self.starts.append(position)

    def line_and_column(self, offset):
        line = bisect.bisect_right(self.starts, offset)
        return line, offset - self.starts[line - 1] + 1


def check_declaration_places(case, run, sources):
    """
    Every `at` of a declaration slices to its name and its line and column are right, and every class `body`
    is its `{ ... }` - in EVERY file of the program that is on disk, the standard library included (some
    6,500 members: the library is the largest body of real Sharp there is to check positions on).
    """
    texts = {}

    def text_of(file_index):
        if file_index not in texts:
            path = run.ide["files"][file_index]["path"] if 0 <= file_index < len(run.ide["files"]) else ""
            texts[file_index] = SourceText(path) if os.path.exists(path) else None
        return texts[file_index]

    for (file_index, node), (name, kind, at) in declaration_index(run).items():
        check(case, 0 <= file_index < len(run.ide["files"]), "%s `%s` names file %s, which the manifest does not have" % (kind, name, file_index))
        text = text_of(file_index)
        if text is None:
            continue
        expected = "operator" if kind == "operator" else name       # an operator function is at the word `operator`
        check(case, cut(text.units, at[0], at[1]) == expected,
              "%s `%s` at %s of file %d cuts out %r" % (kind, name, at, file_index, cut(text.units, at[0], at[1])))
        check(case, (at[2], at[3]) == text.line_and_column(at[0]),
              "%s `%s` of file %d: line/column %s, expected %s" % (kind, name, file_index, at[2:], text.line_and_column(at[0])))

    for record in class_records(run):
        text = text_of(record["file"])
        check(case, "body" in record, "class `%s` has no body range" % record["id"])
        if text is None or "body" not in record:
            continue
        start, end = record["body"]
        body = cut(text.units, start, end)
        check(case, record["at"][1] <= start and body[:1] == "{" and body[-1:] == "}",
              "the body of class `%s` is %r ... %r" % (record["id"], body[:12], body[-12:]))

    for source in sources:
        check(case, text_of(run.file_index(source)) is not None, "the sample %s was not found through the manifest" % source)


def alias_names(run):
    """every alias name of the program: another way to write the name of a class"""
    names = set()

    def walk(record):
        for synonym in record["aliases"]:
            names.add(synonym["name"])
        for nested in record["classes"]:
            walk(nested)

    for module in run.declarations["modules"]:
        for synonym in module["aliases"]:
            names.add(synonym["name"])
        for record in module["classes"]:
            walk(record)
    return names


def type_entry(run, type_id):
    return run.types["types"][type_id]


def check_bodies(case, run, source):
    """positions in a bodies file cut the right text out of the source"""
    index = run.file_index(source)
    check(case, index in run.bodies, "no bodies file for " + source)
    if index not in run.bodies:
        return
    body = run.bodies[index]
    units = editor_text(source)
    length = len(units) // 2
    table = declaration_index(run)

    # lines: every line starts where the text has a \n before it
    starts = body["lines"]
    check(case, starts[0] == 0, "lines[0] is not 0")
    for number, offset in enumerate(starts[1:], start=1):
        check(case, cut(units, offset - 1, offset) == "\n", "line %d does not start after a line end (offset %d)" % (number + 1, offset))

    for function in body["functions"]:
        start, end = function["body"]
        check(case, 0 <= start <= end <= length, "function %s has a body range outside the file" % function["name"])
        text = cut(units, start, end)
        # `{ ... }`;  `= value;` and `:= value;` for the one-line forms;  `-> base(...) { ... }` for a constructor
        check(case, text[:1] in ("{", "=", ":") or text[:2] == "->", "the body of %s starts with %r" % (function["name"], text[:10]))
        check(case, text[-1:] in ("}", ";"), "the body of %s ends with %r" % (function["name"], text[-10:]))
        if function["lambda"]:
            check(case, "in" not in function or 0 <= function["in"] < len(body["functions"]), "lambda without a valid `in`")

    for variable in body["locals"]:
        decl_from, decl_to = variable["decl"]
        if variable["kind"] != "lambda_parameter" or variable["name"] != "it":
            check(case, cut(units, decl_from, decl_to) == variable["name"],
                  "local `%s` is declared at %r" % (variable["name"], cut(units, decl_from, decl_to)))
        visible_from, visible_to = variable["visible"]
        check(case, visible_from <= decl_from or variable["kind"].endswith("parameter"), "local `%s` is visible before it is declared" % variable["name"])
        check(case, visible_to <= length and visible_from <= visible_to, "local `%s` has a bad visible range" % variable["name"])
        check(case, variable["kind"] in ("parameter", "local", "foreach", "catch", "lambda_parameter"), "kind of local: %s" % variable["kind"])
        check(case, 0 <= variable["function"] < len(body["functions"]), "local `%s` names a function that is not listed" % variable["name"])
        function = body["functions"][variable["function"]]
        check(case, function["body"][0] <= variable["visible"][1] <= function["body"][1] or variable["kind"] == "lambda_parameter",
              "local `%s` is visible outside its function" % variable["name"])
        # the text a variable can be seen in stops where a block or a statement stops
        check(case, cut(units, visible_to - 1, visible_to) in ("}", ";"),
              "local `%s` is visible up to %r, which ends no block and no statement" % (variable["name"], cut(units, visible_to - 12, visible_to)))
        check(case, -1 <= variable["type"] < len(run.types["types"]), "local `%s` has a type id outside the table" % variable["name"])

    other_names = alias_names(run) | {entry[0] for entry in table.values() if entry[1] == "tparam"}
    kinds_seen = set()
    starts_seen = set()
    for use in body["uses"]:
        start, end, kind, target_file, node, type_id = use
        kinds_seen.add(kind)
        slice_text = cut(units, start, end)
        check(case, 0 <= start < end <= length, "a use has a bad range %s" % use)
        check(case, start not in starts_seen, "two uses start at offset %d" % start)
        starts_seen.add(start)
        check(case, 1 <= kind <= 7, "a use has the unknown kind %s" % kind)
        if kind == 6:
            check(case, target_file == -1 and 0 <= node < len(body["locals"]), "a use names a local that is not listed: %s" % use)
            if not 0 <= node < len(body["locals"]):
                continue
            variable = body["locals"][node]
            check(case, slice_text == variable["name"], "use of local `%s` cuts out %r" % (variable["name"], slice_text))
            check(case, variable["visible"][0] <= start and end <= variable["visible"][1],
                  "local `%s` is used at %d, outside the text it is said to be visible in %s" % (variable["name"], start, variable["visible"]))
        elif kind == 7:
            check(case, slice_text in ("self", "base"), "a self/base use cuts out %r" % slice_text)
        elif (target_file, node) not in table:
            # Everything a use can point at is in declarations.json (the library is written in full), with one
            # exception the design accepts: an alias declared inside a function body is not a declaration there.
            check(case, kind == 1, "a use of kind %d at %r points at (%d, %d), which declarations.json does not have"
                  % (kind, slice_text, target_file, node))
        else:
            name, declared_kind, _ = table[(target_file, node)]
            if kind == 3 and (declared_kind == "operator" or not any(c.isalpha() for c in slice_text)):
                continue            # an operator is a use of the operator token, whatever the function is called
            if kind == 4:
                # A class may be written under another name: `alias illegal_state_exception as ill_state_except`,
                # or, in generic code, as a type parameter: `new NT(...)` in `class number<T, NT>` is described
                # for the FIRST instance of the class (doc/ide.md, limit 5), where NT is a real class.
                check(case, slice_text == name or slice_text in other_names,
                      "constructor use of `%s` cuts out %r" % (name, slice_text))
            else:
                check(case, slice_text == name, "use of %s `%s` cuts out %r" % (declared_kind, name, slice_text))
        if type_id >= 0:
            check(case, type_id < len(run.types["types"]), "a use has a type id outside the table: %s" % use)
    return kinds_seen


def case_shapes(run, output_text, sources):
    case = "shapes"
    source = sources[0]
    common(case, run, output_text, sources)
    check(case, run.ide["level"] == "full", "level is %s" % run.ide["level"])
    check(case, run.ide["errors"] == 0, "the clean program has errors: %s" % [d["message"] for d in run.diagnostics["diagnostics"]])
    check_declaration_places(case, run, sources)
    kinds = check_bodies(case, run, source)
    check(case, kinds == {1, 2, 3, 4, 5, 6, 7}, "kinds of use found: %s" % sorted(kinds or []))

    index = run.file_index("shapes.sharp")
    entry = run.ide["files"][index]
    check(case, entry["module"] == "shapes" and not entry["library"], "the sample's manifest entry is wrong: %s" % entry)
    check(case, "std" in entry["imports"] and entry["imports"][0] == "shapes", "imports are %s" % entry["imports"])
    library = [f for f in run.ide["files"] if f["library"]]
    check(case, len(library) > 100 and all(f["bodies"] is None for f in library), "library files must be marked and have no bodies")

    modules = {m["name"]: m for m in run.declarations["modules"]}
    mod = modules["shapes"]
    classes = {c["name"]: c for c in mod["classes"]}
    check(case, set(classes) >= {"printable", "shape", "circle", "box", "color"}, "classes are %s" % sorted(classes))
    check(case, classes["printable"]["kind"] == "interface" and classes["color"]["kind"] == "enum", "kinds of printable / color")

    # inheritance: circle's base is shape, shape implements printable
    circle = classes["circle"]
    check(case, "base" in circle and type_entry(run, circle["base"]).get("class") == "shapes#shape", "circle's base is not shapes#shape")
    shape = classes["shape"]
    check(case, [type_entry(run, i).get("class") for i in shape["interfaces"]] == ["shapes#printable"], "shape's interfaces are wrong")
    check(case, "area" in [f["name"] for f in circle["functions"]], "circle.area is missing")
    check(case, [c["name"] for c in circle["constructors"]] == ["circle"], "circle's constructor is missing")
    diameter = [f for f in circle["fields"] if f["name"] == "diameter"][0]
    check(case, diameter["getter"] and diameter["setter"], "the property `diameter` has no getter/setter flags")

    # a generic class: one record as written, with its type parameter; its member types keep the T
    box = classes["box"]
    check(case, [p["name"] for p in box["tparams"]] == ["T"], "box has no type parameter T")
    item = [f for f in box["fields"] if f["name"] == "item"][0]
    check(case, type_entry(run, item["type"]).get("param") is True, "box.item must keep its type parameter T")
    check(case, "box<string>" not in [c["name"] for c in mod["classes"]], "an instance of a generic class was written as a declaration")

    # an enum
    values = classes["color"]["values"]
    check(case, [v["name"] for v in values] == ["red", "green", "blue"] and values[1].get("number") == 5, "enum values are wrong: %s" % values)

    # an alias, a global, a global function
    check(case, [a["name"] for a in mod["aliases"]] == ["names"], "alias names is missing")
    check(case, [g["name"] for g in mod["globals"]] == ["limit"], "global limit is missing")
    check(case, "total" in [f["name"] for f in mod["functions"]], "global function total is missing")
    check(case, "shout" not in [f["name"] for f in mod["functions"]], "the extension function is listed in its module")

    # an extension function is a member of its class, written with its own file
    string_class = None
    for module in run.declarations["modules"]:
        for record in module["classes"]:
            if record["id"] == "std#string":
                string_class = record
    check(case, string_class is not None, "class std#string is missing")
    if string_class:
        shouts = [f for f in string_class["functions"] if f["name"] == "shout"]
        check(case, len(shouts) == 1 and shouts[0].get("extension") is True, "string.shout is not written in the class with extension: true")
        if shouts:
            check(case, shouts[0].get("file") == index, "the extension function does not carry its own file")
            check(case, type_entry(run, shouts[0]["extends"]).get("class") == "std#string", "the extension's `extends` is wrong")

    # bodies: instances, lambdas, locals with the types of the analysed program
    body = run.bodies[index]
    functions = body["functions"]
    names = [f["name"] for f in functions]
    check(case, {"total", "main", "area", "shout"} <= set(names), "functions of the bodies file: %s" % names)
    box_functions = [f for f in functions if f.get("class") == "shapes#box"]
    check(case, box_functions and all(f.get("instance", "").startswith("box<") for f in box_functions), "box functions must say which instance they describe")
    lambdas = [f for f in functions if f["lambda"]]
    check(case, len(lambdas) == 1, "expected one lambda, found %d" % len(lambdas))
    if lambdas:
        total_number = names.index("total")
        check(case, lambdas[0].get("in") == total_number, "the lambda is not marked as written in `total`")
        check(case, lambdas[0]["static"] is True, "the lambda in a global function must be static")
    shout = [f for f in functions if f["name"] == "shout"][0]
    check(case, shout["static"] is False and shout.get("class") == "std#string", "string.shout must have a self of class std#string")
    check(case, [f for f in functions if f["name"] == "total"][0]["static"] is True, "a global function is static")

    local_types = {v["name"]: type_entry(run, v["type"]) for v in body["locals"] if v["type"] >= 0 and v["function"] == names.index("total")}
    check(case, local_types["c"].get("class") == "shapes#circle", "local c is not a circle")
    check(case, local_types["b"].get("class") == "shapes#box" and
          [type_entry(run, a).get("class") for a in local_types["b"]["args"]] == ["std#string"],
          "local b is not a box<string>: %s" % local_types["b"])
    check(case, type_entry(run, [v for v in body["locals"] if v["name"] == "t"][0]["type"])["text"] == "box<_int64>", "local t is not a box<_int64>")
    kinds_of_locals = {v["name"]: v["kind"] for v in body["locals"]}
    check(case, kinds_of_locals.get("scale") == "parameter" and kinds_of_locals.get("n") == "foreach" and
          kinds_of_locals.get("x") == "lambda_parameter", "kinds of locals: %s" % kinds_of_locals)

    # a use of a field with get/set blocks is a use of the FIELD (kind 2); a call of an extension resolves there
    table = declaration_index(run)
    uses_of = lambda name, kind: [u for u in body["uses"] if table.get((u[3], u[4]), ("",))[0] == name and u[2] == kind]
    check(case, uses_of("diameter", 2), "no field use for the property `diameter` read by name")
    check(case, uses_of("shout", 3) and uses_of("shout", 3)[0][3] == index, "the call of the extension function does not resolve to it")
    check(case, uses_of("circle", 4), "no constructor use for `new circle`")
    check(case, uses_of("green", 5), "no enum value use")
    check(case, uses_of("string", 1), "no use of the type name `string`")


def case_errors(run, output_text, sources):
    case = "errors"
    common(case, run, output_text, sources)
    check(case, run.ide["level"] == "full", "a program with a declaration error must still be level `full`, it is %s" % run.ide["level"])
    codes = [d["code"] for d in run.diagnostics["diagnostics"]]
    check(case, any(c.startswith("E03") for c in codes), "the declaration error is missing: %s" % codes)
    check(case, any(c.startswith("E04") for c in codes), "the body error is missing: %s" % codes)
    # the warning: written with its own severity, counted apart from the errors, and marking the cast's type
    warnings = [d for d in run.diagnostics["diagnostics"] if d["severity"] == "warning"]
    check(case, [d["code"] for d in warnings] == ["W1003"], "the warning for the cast that does nothing is missing: %s" % codes)
    check(case, run.ide["warnings"] == 1 and run.ide["errors"] == 2, "the manifest counts %d error(s) and %d warning(s)" % (run.ide["errors"], run.ide["warnings"]))
    units = editor_text(sources[0])
    marked = {d["code"]: cut(units, d["from"], d["to"]) for d in run.diagnostics["diagnostics"]}
    check(case, marked == {"E0301": "strng", "E0400": "missing_name", "W1003": "string"}, "the diagnostics mark %s" % marked)
    unknown_type = [d for d in run.diagnostics["diagnostics"] if d["code"] == "E0301"][0]
    check(case, unknown_type["helps"] == ["did you mean `string`?"] and unknown_type["label"].startswith("not declared"),
          "the label and the help of E0301 are %r, %r" % (unknown_type["label"], unknown_type["helps"]))
    check_declaration_places(case, run, sources)
    kinds = check_bodies(case, run, sources[0])
    index = run.file_index(sources[0])
    body = run.bodies[index]
    names = [f["name"] for f in body["functions"]]
    check(case, "sound" in names, "the sound function has no entry: %s" % names)
    sound_locals = [v["name"] for v in body["locals"] if v["function"] == names.index("sound")]
    check(case, sound_locals == ["first", "second", "sum", "book"], "locals of `sound`: %s" % sound_locals)
    # the field whose type was not found has no type; the program still has a declaration of it
    modules = {m["name"]: m for m in run.declarations["modules"]}
    ledger = [c for c in modules["analysis_errors"]["classes"] if c["name"] == "ledger"][0]
    title = [f for f in ledger["fields"] if f["name"] == "title"][0]
    check(case, title.get("type", -1) == -1, "a field of an unknown type must have no type id: %s" % title)
    # the error type is never written as a type
    for variable in body["locals"]:
        if variable["name"] == "total":
            check(case, variable["type"] == -1 or type_entry(run, variable["type"])["text"] != "?", "an unknown type was written as a guess")


def case_syntax(run, output_text, sources):
    case = "syntax"
    common(case, run, output_text, sources)
    check(case, run.ide["level"] == "syntax-errors", "level is %s" % run.ide["level"])
    check(case, run.ide["types"] is None and run.ide["declarations"] is None, "a syntax-error run must have no index")
    present = sorted(os.listdir(run.folder))
    check(case, present == ["diagnostics.json", "ide.json"], "files written: %s" % present)
    diagnostics = run.diagnostics["diagnostics"]
    check(case, len(diagnostics) >= 1 and all(d["code"].startswith("E01") for d in diagnostics), "syntax errors expected: %s" % [d["code"] for d in diagnostics])
    units = editor_text(sources[0])
    first = diagnostics[0]
    line, column = line_and_column(units, first["from"])
    check(case, (first["line"], first["column"]) == (line, column), "line/column of the syntax error")
    check(case, first["line"] == 10, "the syntax error is on line %d, expected 10" % first["line"])
    entry = run.ide["files"][run.file_index("syntax_error.sharp")]
    check(case, entry["module"] is None and entry["bodies"] is None and entry["size"] > 0, "manifest entry at level syntax-errors: %s" % entry)


def case_weird(run, output_text, sources):
    case = "weird"
    source = sources[0]
    common(case, run, output_text, sources)
    check(case, run.ide["level"] == "full", "level is %s" % run.ide["level"])
    check_declaration_places(case, run, sources)
    check_bodies(case, run, source)
    units = editor_text(source)
    errors = [d for d in run.diagnostics["diagnostics"] if d["severity"] == "error"]
    names = [cut(units, d["from"], d["to"]) for d in errors]
    check(case, names == ["undefined_one", "undefined_two"], "the errors mark %s" % names)
    for d in errors:
        # the column counts UTF-16 units: a tab is one, an emoji is two
        line_text = cut(units, 0, len(units) // 2).split("\n")[d["line"] - 1]
        expected_column = len(line_text[:line_text.index(cut(units, d["from"], d["to"]))].encode("utf-16-le")) // 2 + 1
        check(case, d["column"] == expected_column, "column of %s is %d, expected %d" % (cut(units, d["from"], d["to"]), d["column"], expected_column))
    # the use table: the names used on the lines with the emoji are cut out right (check_bodies did the slicing)
    index = run.file_index("weird_text.sharp")
    check(case, run.ide["files"][index]["size"] == os.path.getsize(source), "the manifest's size is not the file's size in bytes")
    check(case, any(u[2] == 6 for u in run.bodies[index]["uses"]), "no use of a local variable in the weird file")


def case_corners(run, output_text, sources):
    """corners.sharp + corners_more.sharp: what shapes.sharp does not reach"""
    case = "corners"
    source, more = sources[0], sources[1]
    common(case, run, output_text, sources)
    check(case, run.ide["level"] == "full" and run.ide["errors"] == 0, "level %s, %d error(s)" % (run.ide["level"], run.ide["errors"]))
    check_declaration_places(case, run, sources)
    check_bodies(case, run, source)
    check_bodies(case, run, more)
    index, more_index = run.file_index(source), run.file_index(more)
    if index not in run.bodies or more_index not in run.bodies:
        return
    units = editor_text(source)
    body = run.bodies[index]
    table = declaration_index(run)

    modules = {m["name"]: m for m in run.declarations["modules"]}
    mod = modules["corners"]
    check(case, sorted(mod["files"]) == sorted([index, more_index]), "the module's files are %s" % mod["files"])
    classes = {c["name"]: c for c in mod["classes"]}
    counter = classes["counter"]

    # a nested class has the id of the class around it in front, and is found inside it
    check(case, [c["id"] for c in counter["classes"]] == ["corners#counter.step"], "nested classes: %s" % [c["id"] for c in counter["classes"]])

    # a member added by `mutate` is written in its class, with the file it is really in
    bonus = [f for f in counter["functions"] if f["name"] == "bonus"]
    check(case, counter["mutated"] is True and len(bonus) == 1 and bonus[0].get("file") == more_index,
          "counter.bonus (added by `mutate` in the other file) is written as %s" % bonus)
    check(case, [f["name"] for f in run.bodies[more_index]["functions"]] == ["bonus"] and
          run.bodies[more_index]["functions"][0].get("class") == "corners#counter",
          "the bodies file of corners_more.sharp lists %s" % run.bodies[more_index]["functions"])

    # an operator function, static members, a one-line function
    plus = [f for f in counter["functions"] if f["name"] == "operator+"]
    check(case, len(plus) == 1 and plus[0]["operator"] is True and plus[0]["sig"] == "operator+(other: counter) : counter",
          "operator+ is written as %s" % plus)
    describe = [f for f in counter["functions"] if f["name"] == "describe"][0]
    check(case, "static" in describe["flags"].split(), "describe's flags are %r" % describe["flags"])
    functions = {f["name"]: f for f in body["functions"]}
    check(case, functions["describe"]["static"] is True and functions["twice"]["static"] is False, "static of describe / twice")
    check(case, cut(units, *functions["twice"]["body"]) == ":= amount * 2;", "the body of the one-line function is %r" % cut(units, *functions["twice"]["body"]))
    twice = [f for f in counter["functions"] if f["name"] == "twice"][0]
    # `def twice() := value`: the result type is the one the compiler worked out for the value, and the signature says it
    check(case, twice.get("result", -1) >= 0 and twice["sig"] == "twice() : " + type_entry(run, twice["result"])["text"],
          "the worked-out result of `twice` is written as %s" % twice)

    # a constructor's text starts at its `-> base(...)`: the parameters are used there
    tally_body = cut(units, *functions["tally"]["body"])
    check(case, tally_body.startswith("-> base()") and tally_body.endswith("}"), "the body of the constructor `tally` is %r" % tally_body[:20])

    # a generic function: one entry, saying which instance its types are of; a call of it points at the declaration
    first_of = functions["first_of"]
    check(case, first_of.get("instance") == "first_of<_int64>", "first_of's instance is %r" % first_of.get("instance"))
    names = [f["name"] for f in body["functions"]]
    items = [v for v in body["locals"] if v["name"] == "items"]
    check(case, len(items) == 1 and items[0]["function"] == names.index("first_of") and type_entry(run, items[0]["type"])["text"] == "_int64[]",
          "the parameter of the generic function is %s" % items)
    uses_of = lambda name, kind: [u for u in body["uses"] if table.get((u[3], u[4]), ("",))[0] == name and u[2] == kind]
    check(case, len(uses_of("first_of", 3)) == 1, "the call of the generic function does not point at its declaration")
    check(case, [f["name"] for f in mod["functions"] if "tparams" in f] == ["first_of"], "the generic function has no `tparams`")
    check(case, uses_of("bonus", 3) and uses_of("bonus", 3)[0][3] == more_index, "the call of the `mutate` member does not point into the other file")
    own_plus = [cut(units, u[0], u[1]) for u in uses_of("operator+", 3) if u[3] == index]      # (string has an operator+ too)
    check(case, own_plus == ["+"], "the use of counter's operator+ is not at the `+` of `a + a`: %s" % own_plus)

    # the class name before the dot of a static member is a use of the class
    counter_names = [u for u in uses_of("counter", 1) if cut(units, u[1], u[1] + 1) == "."]
    after_dot = sorted(cut(units, u[1] + 1, u[1] + 5) for u in counter_names)
    check(case, after_dot == ["desc", "made"], "class names before a dot: %s" % after_dot)

    # a lambda in the value of a field: no function around it, and `self` is the field's object
    lambdas = [f for f in body["functions"] if f["lambda"]]
    check(case, len(lambdas) == 1 and "in" not in lambdas[0] and lambdas[0]["static"] is False and lambdas[0].get("class") == "corners#counter",
          "the lambda in a field's value is %s" % lambdas)

    # an alias of an array type: the array's items are called by their own name
    aliased = [t for t in run.types["types"] if t["text"] == "words"]
    check(case, len(aliased) == 1 and aliased[0].get("array") is True and type_entry(run, aliased[0]["element"])["text"] == "string",
          "the type `words` (an alias of string[]) is %s" % aliased)

    # the text each kind of local can be seen in
    scopes = names.index("scopes")
    seen = {v["name"]: cut(units, *v["visible"]) for v in body["locals"] if v["function"] == scopes}
    kinds = {v["name"]: v["kind"] for v in body["locals"] if v["function"] == scopes}
    check(case, kinds == {"limit": "parameter", "outer": "local", "index": "local", "inner": "local", "problem": "catch",
                          "item": "foreach", "after": "local"}, "kinds of the locals of `scopes`: %s" % kinds)
    whole = cut(units, *functions["scopes"]["body"])
    check(case, seen["limit"] == whole, "a parameter is visible in %r" % seen["limit"][:30])
    check(case, seen["outer"].startswith("outer : _int64") and whole.endswith(seen["outer"]), "`outer` is visible in %r" % seen["outer"][:30])
    check(case, seen["index"] == "index : _int64 = 0; index < limit; index++) outer += index;", "the `for` variable is visible in %r" % seen["index"])
    # `inner` stops at the `}` of the `if`: neither the `{ }` inside the string's `${ }` nor the `{` in its text ends it early
    check(case, seen["inner"].startswith("inner := \"a ${") and seen["inner"].endswith("outer += 1;\n    }") and "try" not in seen["inner"],
          "`inner` is visible in %r" % seen["inner"])
    check(case, seen["problem"].startswith("problem: exception) {") and seen["problem"].endswith("outer = 0;\n    }") and "foreach" not in seen["problem"],
          "the `catch` variable is visible in %r" % seen["problem"])
    check(case, seen["item"] == "item in { 1, 2 }) outer += item;", "the `foreach` variable is visible in %r" % seen["item"])
    check(case, seen["after"].startswith("after : _int64") and whole.endswith(seen["after"]), "`after` is visible in %r" % seen["after"])
    delta = [v for v in body["locals"] if v["name"] == "delta"][0]
    check(case, delta["kind"] == "lambda_parameter" and cut(units, *delta["visible"]) == "{ delta: _int64 -> (_int64) return delta * 2; }",
          "the lambda's parameter is visible in %r" % cut(units, *delta["visible"]))
    pace = [v for v in body["locals"] if v["name"] == "pace"][0]
    check(case, type_entry(run, pace["type"]).get("class") == "corners#counter.step", "`pace` is not a counter.step")


def case_odd(run, output_text, sources):
    """
    Three files in one run: a lone carriage return (the compiler takes it for a space, the editor for a line
    end: the offsets must still agree), bytes that are not UTF-8 (each damaged piece is one unit, as the
    editor shows it), and a class declared twice in two files (an error with a related place in the other file).
    """
    case = "odd"
    common(case, run, output_text, sources)
    check(case, run.ide["level"] == "full", "level is %s" % run.ide["level"])
    check_declaration_places(case, run, sources)
    for source in sources:
        check_bodies(case, run, source)
    marked = {}
    for d in run.diagnostics["diagnostics"]:
        marked[d["code"] + " " + os.path.basename(d["file"])] = cut(editor_text(d["file"]), d["from"], d["to"])
    # (which of the two twins is "the second one" depends on the order the files were parsed in)
    twin_key = [key for key in marked if key.startswith("E0302 twin_")]
    check(case, len(twin_key) == 1 and len(marked) == 3 and marked.get("E0400 lone_cr.sharp") == "missing_cr" and
          marked.get("E0400 bad_bytes.sharp") == "missing_bad" and marked.get(twin_key[0] if twin_key else "") == "twin",
          "the errors mark %s" % marked)
    twice = [d for d in run.diagnostics["diagnostics"] if d["code"] == "E0302"]
    if twice:
        related = twice[0]["related"]
        check(case, len(related) == 1 and os.path.basename(related[0]["file"]) in ("twin_a.sharp", "twin_b.sharp") and
              related[0]["file"] != twice[0]["file"] and
              cut(editor_text(related[0]["file"]), related[0]["from"], related[0]["to"]) == "twin" and related[0]["label"],
              "the related place of the class declared twice is %s" % related)
    # the text in the damaged file that comes AFTER the bad bytes is still cut out right
    bad = run.bodies.get(run.file_index("bad_bytes.sharp"))
    if bad:
        units = editor_text([s for s in sources if s.endswith("bad_bytes.sharp")][0])
        check(case, [cut(units, *v["decl"]) for v in bad["locals"]] == ["text"], "the local after the bad bytes: %s" % bad["locals"])


def case_suite(case, run, output_text, sources):
    """a program of the language's own test suite: every position in it cuts the right text out"""
    common(case, run, output_text, sources)
    check(case, run.ide["level"] == "full", "level is %s" % run.ide["level"])
    check_declaration_places(case, run, sources)
    check_bodies(case, run, sources[0])


def case_bodies(case, run, output_text, sources, expected_names):
    common(case, run, output_text, sources)
    written = sorted(os.path.basename(e["path"]) for e in run.ide["files"] if e["bodies"])
    check(case, written == sorted(expected_names), "bodies written for %s, expected %s" % (written, sorted(expected_names)))
    folder_files = sorted(os.listdir(os.path.join(run.folder, "bodies"))) if os.path.isdir(os.path.join(run.folder, "bodies")) else []
    check(case, len(folder_files) == len(expected_names), "files in bodies/: %s" % folder_files)
    for entry in run.ide["files"]:
        if entry["bodies"]:
            check(case, os.path.exists(os.path.join(run.folder, entry["bodies"])), "the manifest names a missing file " + entry["bodies"])


def main():
    case, folder, output_path, sources = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
    run = Run(folder)
    with open(output_path, "r", encoding="utf-8", errors="replace") as handle:
        output_text = handle.read()
    if case == "shapes":
        case_shapes(run, output_text, sources)
    elif case == "errors":
        case_errors(run, output_text, sources)
    elif case == "syntax":
        case_syntax(run, output_text, sources)
    elif case == "weird":
        case_weird(run, output_text, sources)
    elif case == "corners":
        case_corners(run, output_text, sources)
    elif case == "odd":
        case_odd(run, output_text, sources)
    elif case.startswith("suite_"):
        case_suite(case, run, output_text, sources)
    elif case == "bodies_both":
        # the sample and the first library file of the list were asked for (the script says which, in sources[1])
        case_bodies(case, run, output_text, sources[:1], [os.path.basename(sources[0]), os.path.basename(sources[1])])
    elif case == "bodies_none":
        case_bodies(case, run, output_text, sources[:1], [])
    elif case == "bodies_auto":
        case_bodies(case, run, output_text, sources[:1], [os.path.basename(sources[0])])
    else:
        print("unknown case " + case)
        sys.exit(2)
    sys.exit(1 if failures else 0)


main()
