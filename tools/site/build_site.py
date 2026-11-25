#!/usr/bin/env python3
"""
build_site.py  -  makes the Sharp website: a folder of plain files for GitHub Pages.

    python3 tools/site/build_site.py [<folder>]         (default: build/site)

WHAT IT MAKES

    index.html  gui.html  projects.html  install.html   the pages written by hand: pages/*.html
                                                        with the header and the footer put round them
    benchmarks.html                                     doc/benchmarks.html as it is
    std/index.html                                      the standard library: every module
    std/<module>.html                                   one page a module, READ FROM THE LIBRARY ITSELF
    std/search.js                                       every name, for the search box
    std/api.txt                                         every documented name on a line of its own:
                                                        compare two of these to see what changed
    site.css  site.js  shots/  logo.png

THE LIBRARY PAGES are made from lib/support/<version>/std: every .sharp file is read for its
`mod` line, its classes, interfaces and enums, their functions and fields, the functions and
constants that stand alone, and the /** ... */ comment above each. What is private, local or
protected is left out. Nothing is compiled: a file that does not compile is still read.

THE FOLDER is emptied first (everything but .git and CNAME), so a page whose module is gone
does not stay behind. To be safe it must be empty, hold a site this script made, or be a git
checkout (the gh-pages branch).
"""
import html
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
GITHUB = "https://github.com/AndroDevcd/Sharp"

# the top bar: address, words, and the pages that light it up besides its own
NAV = [("index.html", "Home", ()), ("guide/index.html", "Guide", ("guide/", "projects.html")),
       ("std/index.html", "Library", ("std/",)), ("gui.html", "GUI", ()),
       ("debugger.html", "Debugger", ("sdb.html",)), ("performance.html", "Performance", ("benchmarks.html",)),
       ("install.html", "Download", ())]

# the guide: GETTING_STARTED.md, a page for each of its numbered parts ("## 4. A tour of the language")
GUIDE_SOURCE = "GETTING_STARTED.md"
SDB_GUIDE = "lib/debugger/doc/sdb.md"

HAND_WRITTEN = [
    # file, the name in the browser's tab, one sentence for search engines
    ("index.html", "The Sharp programming language",
     "Sharp: dependency injection, mocks, tests and GUI apps, all built in."),
    ("gui.html", "GUI development - Sharp",
     "Desktop apps with std.guikit: widgets, charts and 3D scenes that come with Sharp."),
    ("projects.html", "Projects - Sharp",
     "A Sharp project: a folder, a settings.json and one command to build it."),
    ("install.html", "Download - Sharp",
     "Download and install Sharp on Windows, Linux or a Raspberry Pi."),
    ("debugger.html", "sdb, the debugger - Sharp",
     "sdb, the Sharp debugger: breakpoints, variables, threads and fibers in the terminal."),
    ("performance.html", "Performance - Sharp",
     "How fast Sharp is: the Benchmarks Game against other languages, and against Sharp in 2020."),
]


# ------------------------------------------------------------------------------------------ small things

def esc(text):
    return html.escape(text, quote=True)


def read(path):
    with open(path, encoding="utf-8", errors="replace") as file:
        return file.read()


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as file:
        file.write(text)


def newest(names):
    """the name with the highest version number in it"""
    def numbers(name):
        return [int(n) for n in re.findall(r"\d+", name)]
    return sorted(names, key=numbers)[-1]


def branch():
    try:
        found = subprocess.run(["git", "-C", ROOT, "rev-parse", "--abbrev-ref", "HEAD"],
                               capture_output=True, text=True).stdout.strip()
        return found or "master"
    except OSError:
        return "master"


# ------------------------------------------------------------------------------------------ a page

def page(file, title, description, body, facts):
    """`body` with the head, the header and the footer round it. `file` is where it goes, from the site's root."""
    root = "../" * file.count("/")
    links = []
    for target, words, also in NAV:
        here = target == file or any(file.startswith(start) for start in also)
        links.append('<a href="%s%s"%s>%s</a>' % (root, target, ' aria-current="page"' if here else "", words))
    links.append('<a href="%s">GitHub</a>' % GITHUB)
    body = body.replace("{root}", root).replace("{version}", facts["version"]) \
               .replace("{github}", GITHUB).replace("{branch}", facts["branch"])
    return """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>%s</title>
<meta name="description" content="%s">
<link rel="icon" href="%slogo.png">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;500;600;700;800&family=IBM+Plex+Mono:wght@400;500;600&display=swap">
<link rel="stylesheet" href="%ssite.css">
</head>
<body>
<header class="top">
  <div class="wrap">
    <a class="brand" href="%sindex.html"><span class="mark">#</span>Sharp</a>
    <span class="version">%s</span>
    <nav class="main" aria-label="Site">
      %s
    </nav>
  </div>
</header>

%s
<footer><div class="wrap"><span>The Sharp programming language: compiler, VM, standard library and documentation.</span><a href="%s">github.com/AndroDevcd/Sharp</a></div></footer>
<script src="%ssite.js"></script>
</body>
</html>
""" % (esc(title), esc(description), root, root, root, facts["version"], "\n      ".join(links), body, GITHUB, root)


# ------------------------------------------------------------------------------------------ pages from Markdown

def inline(text, facts):
    """one line of Markdown: `code`, **bold**, [words](address)"""
    def link(found):
        words_, address = found.group(1), found.group(2)
        if not re.match(r"^(https?:|#)", address):         # a file of the repository
            address = "%s/blob/%s/%s" % (GITHUB, facts["branch"], address.lstrip("./"))
        return '<a href="%s">%s</a>' % (address, words_)
    text = esc(text)
    text = re.sub(r"`([^`]+)`", r"<code>\1</code>", text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    return re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", link, text)


def markdown(text, facts):
    """The Markdown the repository's guides are written in: headings, paragraphs, lists, tables, fenced code."""
    out, lines, i = [], text.split("\n"), 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("```"):
            code = []
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                code.append(lines[i])
                i += 1
            i += 1
            body = "\n".join(code)
            sharp = re.search(r"^\s*(mod |def |class |import |extern )", body, re.M) is not None
            out.append('<div class="code"><pre%s>%s</pre></div>' % (' class="sharp"' if sharp else "", esc(body)))
        elif re.match(r"^#{2,4} ", line):
            level = len(line.split(" ")[0])
            words_ = line[level + 1:].strip()
            name = re.sub(r"[^a-z0-9]+", "-", words_.lower()).strip("-")
            out.append('<h%d id="%s">%s</h%d>' % (level, name, inline(words_, facts), level))
            i += 1
        elif line.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                cells = [c.strip() for c in lines[i].replace("\\|", "&#124;").strip().strip("|").split("|")]
                if not all(re.match(r"^:?-+:?$", c) for c in cells):
                    tag = "th" if not rows else "td"
                    rows.append("<tr>%s</tr>" % "".join("<%s>%s</%s>" % (tag, inline(c, facts).replace("&amp;#124;", "|"), tag)
                                                        for c in cells))
                i += 1
            out.append('<div class="scroll"><table class="cmds">%s</table></div>' % "".join(rows))
        elif re.match(r"^\s*(-|\d+\.) ", line):
            items = []
            while i < len(lines) and (re.match(r"^\s*(-|\d+\.) ", lines[i]) or (lines[i].startswith("  ") and lines[i].strip())):
                if re.match(r"^\s*(-|\d+\.) ", lines[i]):
                    items.append(re.sub(r"^\s*(-|\d+\.) ", "", lines[i]))
                else:
                    items[-1] += " " + lines[i].strip()
                i += 1
            out.append('<ul class="plainlist">%s</ul>' % "".join("<li>%s</li>" % inline(item, facts) for item in items))
        elif line.strip() == "" or line.startswith("# ") or line.startswith("---"):
            i += 1
        else:
            para = []
            while i < len(lines) and lines[i].strip() and not re.match(r"^(```|#{1,4} |\||\s*(-|\d+\.) )", lines[i]):
                para.append(lines[i].strip())
                i += 1
            out.append("<p>%s</p>" % inline(" ".join(para), facts))
    return "\n".join(out)


def guide_pages(facts):
    """GETTING_STARTED.md as the guide: [(file, title, html)], a page for each numbered part."""
    text = read(os.path.join(ROOT, GUIDE_SOURCE))
    parts = re.split(r"^## (\d+)\. (.+)$", text, flags=re.M)
    chapters = []
    for at in range(1, len(parts), 3):
        title = parts[at + 1].strip()
        file = "index.html" if not chapters else re.sub(r"[^a-z0-9]+", "-", title.lower()).strip("-") + ".html"
        chapters.append((file, title, parts[at + 2]))

    pages = []
    for n, (file, title, body) in enumerate(chapters):
        items = []
        for other, name, _ in chapters:
            items.append('<li%s><a href="%s">%s</a></li>' % (' class="on"' if other == file else "", other, esc(name)))
        items.append('<li><a href="../projects.html">Projects</a></li>')
        items.append('<li><a href="../gui.html">GUI development</a></li>')
        items.append('<li><a href="../debugger.html">Debugging with sdb</a></li>')
        pager = []
        if n > 0:
            pager.append('<a href="%s">&larr; %s</a>' % (chapters[n - 1][0], esc(chapters[n - 1][1])))
        else:
            pager.append("<span></span>")
        if n + 1 < len(chapters):
            pager.append('<a href="%s">%s &rarr;</a>' % (chapters[n + 1][0], esc(chapters[n + 1][1])))
        html_ = """<div class="lib guide"><div class="wrap">
    <aside>
      <p class="eyebrow">Guide</p>
      <ul class="mods">
        %s
      </ul>
    </aside>
    <main>
      <p class="crumbs">Guide / %d of %d</p>
      <h1>%s</h1>
%s
      <div class="pager">%s</div>
    </main>
</div></div>
""" % ("\n        ".join(items), n + 1, len(chapters), esc(title), markdown(body, facts), "".join(pager))
        pages.append(("guide/" + file, title, html_))
    return pages


def sdb_guide(facts):
    """the debugger's own guide, whole, on one page"""
    return """<div class="lib guide"><div class="wrap one">
    <main>
      <p class="crumbs"><a href="debugger.html">Debugger</a> / The sdb guide</p>
      <h1>sdb, the Sharp debugger</h1>
%s
    </main>
</div></div>
""" % markdown(read(os.path.join(ROOT, SDB_GUIDE)), facts)


# ------------------------------------------------------------------------------------------ reading the library

class Code:
    """Takes strings and comments out of source lines, so that braces can be counted; remembers /** */ comments."""

    def __init__(self):
        self.in_comment = False     # inside /* ... */ that began on an earlier line
        self.doc = None             # the lines of the /** */ comment being read
        self.finished_doc = None    # the last /** */ comment that ended, until something takes it

    def clean(self, line):
        out = []
        i, n = 0, len(line)
        while i < n:
            if self.in_comment:
                end = line.find("*/", i)
                if end < 0:
                    if self.doc is not None:
                        self.doc.append(line[i:])
                    return "".join(out)
                if self.doc is not None:
                    self.doc.append(line[i:end])
                    self.finished_doc = self.doc
                    self.doc = None
                self.in_comment = False
                i = end + 2
                continue
            two = line[i:i + 2]
            if two == "//":
                break
            if two == "/*":
                self.in_comment = True
                if line[i:i + 3] == "/**" and line[i:i + 4] != "/**/":
                    self.doc = []
                    i += 3
                else:
                    i += 2
                continue
            c = line[i]
            if c == '"' or c == "'":
                i += 1
                while i < n and line[i] != c:
                    i += 2 if line[i] == "\\" else 1
                i += 1
                out.append('""')
                continue
            out.append(c)
            i += 1
        return "".join(out)


def doc_blocks(lines):
    """A /** */ comment as blocks: ('text', words) and ('code', lines). Leading ` * ` is taken off every line."""
    if not lines:
        return []
    cleaned = []
    for line in "\n".join(lines).split("\n"):
        line = line.rstrip()
        line = re.sub(r"^\s*\*? ?", "", line, count=1)
        cleaned.append(line)
    blocks, current = [], []

    def close():
        if current:
            if all(l.startswith("    ") for l in current):
                indent = min(len(l) - len(l.lstrip()) for l in current)
                blocks.append(("code", [l[indent:] for l in current]))
            else:
                blocks.append(("text", " ".join(l.strip() for l in current)))
            current.clear()

    for line in cleaned:
        if line.strip() == "":
            close()
        else:
            current.append(line)
    close()
    # what an editor's file template left behind says nothing about the code
    kept = []
    for kind, value in blocks:
        if kind == "text":
            if value.startswith("Created by ") or value.startswith("@class") or value.startswith("@author"):
                continue
            if value.startswith("Description:"):
                value = value[len("Description:"):].strip()
        kept.append((kind, value))
    return kept


def words(text):
    """escaped, with `code` marked"""
    return re.sub(r"`([^`]+)`", r"<code>\1</code>", esc(text))


def doc_html(blocks):
    out = []
    for kind, value in blocks:
        if kind == "code":
            out.append('<div class="code"><pre class="sharp">%s</pre></div>' % esc("\n".join(value)))
        else:
            out.append("<p>%s</p>" % words(value))
    return "\n".join(out)


def summary(blocks):
    """the first sentence of the first paragraph"""
    for kind, value in blocks:
        if kind == "text":
            first = re.split(r"(?<=[.!?])\s", value, maxsplit=1)[0]
            return first if len(first) <= 180 else first[:177] + "..."
    return ""


HIDDEN = ("private", "local", "protected")
TYPE = re.compile(r"^((?:\w+\s+)*?)(class|interface|enum|component)\s+(\w+)\s*(<[^{;]*?>)?\s*([^{;]*)")
FUNCTION = re.compile(r"^((?:\w+\s+)*?)def\s+(.*)$")
FIELD = re.compile(r"^((?:\w+\s+)*?)(\w+)\s*(:=|:)\s*([^;]*);")
CONST = re.compile(r"^((?:\w+\s+)*?)const\s+(\w+)\s*:\s*([^=;]+?)\s*=\s*([^;]*);")


def signature(cleaned, at):
    """The declaration that starts on line `at`, up to its `{` or `;` - it may run over several lines."""
    text, depth = "", 0
    for line in cleaned[at:at + 12]:
        for c in line.strip() + " ":
            if c in "([":
                depth += 1
            elif c in ")]":
                depth -= 1
            elif (c == "{" or c == ";") and depth <= 0:
                return re.sub(r"\s+", " ", text).strip()
            text += c
    return re.sub(r"\s+", " ", text).strip()


def read_file(path, shown_path, modules):
    raw = read(path).split("\n")
    code = Code()
    cleaned, docs_before = [], []
    for line in raw:
        cleaned.append(code.clean(line))
        docs_before.append(code.finished_doc)       # the comment that had ended when this line was read
        if cleaned[-1].strip() != "" and code.doc is None and not code.in_comment:
            code.finished_doc = None                # code came after it: it belongs to nothing further down
    # (a declaration takes the comment that ended on a line above it; docs_before[i] still holds
    #  it for the first line of code after the comment, because it is cleared after that line)

    module = None
    depth = 0
    current = None          # the class, interface or enum being read
    for i, line in enumerate(cleaned):
        text = line.strip()
        before = depth
        depth += line.count("{") - line.count("}")
        if text == "":
            continue
        doc = doc_blocks(docs_before[i])
        note = ""
        if "//" in raw[i]:
            note = raw[i].split("//", 1)[1].strip()

        if before == 0:
            current = None
            found = re.match(r"^mod\s+([\w.]+)\s*;", text)
            if found:
                module = modules.setdefault(found.group(1), {"types": [], "functions": [], "constants": []})
                continue
            if module is None:
                continue
            found = TYPE.match(text)
            if found:
                modifiers, kind, name, generic, rest = found.groups()
                current = {"kind": kind, "name": name, "generic": generic or "", "parents": rest.strip(),
                           "doc": doc, "file": shown_path, "functions": [], "fields": [],
                           "hidden": any(w in modifiers.split() for w in HIDDEN)}
                if not current["hidden"]:
                    module["types"].append(current)
                continue
            found = CONST.match(text)
            if found and not any(w in found.group(1).split() for w in HIDDEN):
                module["constants"].append({"name": found.group(2), "type": found.group(3).strip(),
                                            "value": found.group(4).strip(), "note": note or summary(doc)})
                continue
            found = FUNCTION.match(text)
            if found and not any(w in found.group(1).split() for w in HIDDEN):
                module["functions"].append({"sig": signature(cleaned, i), "doc": doc, "file": shown_path})
            continue

        if before == 1 and current is not None and not current["hidden"]:
            found = FUNCTION.match(text)
            if found:
                if not any(w in found.group(1).split() for w in HIDDEN):
                    current["functions"].append({"sig": signature(cleaned, i), "doc": doc})
                continue
            # the constructor: the type's own name and an opening bracket
            if re.match(r"^(?:\w+\s+)*%s\s*\(" % re.escape(current["name"]), text):
                if not any(w in text.split("(")[0].split() for w in HIDDEN):
                    current["functions"].append({"sig": signature(cleaned, i), "doc": doc, "maker": True})
                continue
            if current["kind"] == "enum":
                continue
            found = FIELD.match(text)
            if found and not any(w in found.group(1).split() for w in HIDDEN):
                declared = found.group(4).strip()
                kind = declared.split("=")[0].strip() if found.group(3) == ":" else ""
                current["fields"].append({"name": found.group(2), "type": kind, "note": note or summary(doc)})


def read_library(folder):
    modules = {}
    for where, _, files in sorted(os.walk(folder)):
        for name in sorted(files):
            if name.endswith(".sharp"):
                path = os.path.join(where, name)
                read_file(path, os.path.relpath(path, ROOT).replace("\\", "/"), modules)
    for module in modules.values():
        module["types"].sort(key=lambda t: t["name"])
        module["functions"].sort(key=lambda f: function_name(f["sig"]))
        module["constants"].sort(key=lambda c: c["name"])
    return dict(sorted(modules.items()))


# ------------------------------------------------------------------------------------------ the library's pages

def function_name(sig):
    # (a function added to another type is written `def list<T>.sum()`: its name here is list<T>.sum)
    found = re.match(r"^(?:\w+\s+)*?def\s+((?:\w+(?:<[^>(]*>)?\.)?(?:operator\s*[^\s(]+|\w+))", sig) \
        or re.match(r"^(?:\w+\s+)*?(\w+)\s*\(", sig)
    return found.group(1) if found else sig


def sig_html(sig):
    """the signature with its name marked: `def <b>add</b>(element: T)`"""
    name = function_name(sig)
    at = sig.find(name)
    return esc(sig[:at]) + "<b>" + esc(name) + "</b>" + esc(sig[at + len(name):])


def module_file(name):
    return name + ".html"


def top_of(name):
    """the topmost module a module belongs to: std.guikit.core.state -> std.guikit"""
    return ".".join(name.split(".")[:2])


def inside(modules, top):
    """the modules below a topmost one"""
    return [name for name in modules if name != top and top_of(name) == top]


def module_card(name, module, more):
    names = ", ".join(t["name"] for t in module["types"][:6])
    if len(module["types"]) > 6:
        names += ", ..."
    functions = len(module["functions"]) + sum(len(t["functions"]) for t in module["types"])
    return ('<a class="modcard" href="%s"><b>%s</b><span>%d types, %d functions%s</span><span class="names">%s</span></a>'
            % (module_file(name), esc(name), len(module["types"]), functions, more, esc(names)))


def sidebar(modules, here):
    """The topmost modules; below the one being read, the modules inside it."""
    items = []
    for name in modules:
        if name != top_of(name) and (here is None or top_of(name) != top_of(here)):
            continue
        classes = []
        if name == here:
            classes.append("on")
        if name.count(".") > 1:
            classes.append("sub")
        items.append('<li%s><a href="%s">%s</a></li>' % (' class="%s"' % " ".join(classes) if classes else "",
                                                           module_file(name), esc(name)))
    return """<aside>
      <label class="eyebrow" for="lib-search">Standard library</label>
      <input id="lib-search" type="search" placeholder="Search classes and functions" autocomplete="off">
      <ul class="found" id="lib-found" hidden></ul>
      <ul class="mods">
        %s
      </ul>
    </aside>""" % "\n        ".join(items)


def function_html(function, anchor):
    doc = doc_html(function["doc"])
    return '<div class="fn" id="%s"><div class="sig">%s</div>%s</div>' % (esc(anchor), sig_html(function["sig"]), doc)


def anchors():
    """names for the places on a page, each used once"""
    used = {}

    def make(name):
        name = re.sub(r"[^\w.]+", "_", name)
        used[name] = used.get(name, 0) + 1
        return name if used[name] == 1 else "%s-%d" % (name, used[name])
    return make


def module_page(name, module, modules, facts, index, api):
    anchor = anchors()
    top = top_of(name)
    above = ""
    if name != top and top in modules:
        above = ' / <a href="%s">%s</a>' % (module_file(top), esc(top))
    parts = ['<p class="crumbs"><a href="index.html">Standard library</a>%s / %s</p>' % (above, esc(name)),
             "<h1>%s <small>module</small></h1>" % esc(name)]
    below = inside(modules, name) if name == top else []
    if below:
        parts.append('<h2 class="inside">Modules inside</h2>\n<div class="modgrid">\n%s\n</div>'
                     % "\n".join(module_card(sub, modules[sub], "") for sub in below))

    contents = []
    for t in module["types"]:
        contents.append('<a href="#%s">%s</a>' % (esc(t["name"]), esc(t["name"])))
    if module["functions"]:
        contents.append('<a href="#functions">functions</a>')
    if module["constants"]:
        contents.append('<a href="#constants">constants</a>')
    parts.append('<p class="contents">%s</p>' % " ".join(contents))

    for t in module["types"]:
        make = anchor(t["name"])
        title = "%s %s%s" % (t["kind"], t["name"], t["generic"])
        parts.append('<section class="type" id="%s">' % esc(make))
        parts.append('<h2><span class="kind">%s</span> %s%s</h2>' % (t["kind"], esc(t["name"]), esc(t["generic"])))
        meta = []
        if t["parents"]:
            meta.append("<code>%s</code>" % esc(t["parents"]))
        meta.append('<a href="%s/blob/%s/%s">source</a>' % (GITHUB, facts["branch"], t["file"]))
        parts.append('<p class="meta">%s</p>' % " &middot; ".join(meta))
        parts.append(doc_html(t["doc"]))
        index.append((t["name"], name, "%s#%s" % (module_file(name), make), summary(t["doc"])))
        api.append("%s  %s" % (name, title))

        if t["fields"]:
            rows = []
            for f in t["fields"]:
                rows.append("<tr><td><code>%s</code></td><td><code>%s</code></td><td>%s</td></tr>"
                            % (esc(f["name"]), esc(f["type"]), words(f["note"])))
                api.append("%s  %s.%s : %s" % (name, t["name"], f["name"], f["type"]))
            parts.append('<h3>Fields</h3><div class="scroll"><table class="fields">%s</table></div>' % "".join(rows))
        if t["functions"]:
            parts.append("<h3>Functions</h3><div>")
            for f in sorted(t["functions"], key=lambda f: (not f.get("maker", False), function_name(f["sig"]))):
                made = anchor("%s.%s" % (t["name"], function_name(f["sig"])))
                parts.append(function_html(f, made))
                index.append(("%s.%s" % (t["name"], function_name(f["sig"])), name,
                              "%s#%s" % (module_file(name), made), summary(f["doc"])))
                api.append("%s  %s.%s" % (name, t["name"], f["sig"]))
            parts.append("</div>")
        parts.append("</section>")

    if module["functions"]:
        parts.append('<section class="type" id="functions"><h2>Functions</h2><div>')
        for f in module["functions"]:
            made = anchor(function_name(f["sig"]))
            parts.append(function_html(f, made))
            index.append((function_name(f["sig"]), name, "%s#%s" % (module_file(name), made), summary(f["doc"])))
            api.append("%s  %s" % (name, f["sig"]))
        parts.append("</div></section>")

    if module["constants"]:
        rows = []
        for c in module["constants"]:
            rows.append("<tr><td><code>%s</code></td><td><code>%s</code></td><td><code>%s</code></td><td>%s</td></tr>"
                        % (esc(c["name"]), esc(c["type"]), esc(c["value"]), words(c["note"])))
            index.append((c["name"], name, "%s#constants" % module_file(name), c["note"]))
            api.append("%s  const %s : %s = %s" % (name, c["name"], c["type"], c["value"]))
        parts.append('<section class="type" id="constants"><h2>Constants</h2>'
                     '<div class="scroll"><table class="fields">%s</table></div></section>' % "".join(rows))

    body = '<div class="lib"><div class="wrap">\n    %s\n    <main>\n%s\n    </main>\n</div></div>\n<script src="search.js"></script>\n' \
           % (sidebar(modules, name), "\n".join(parts))
    return page("std/" + module_file(name), "%s - Sharp standard library" % name,
                "The %s module of the Sharp standard library." % name, body, facts)


def library_index(modules, facts, counts):
    # only the topmost modules: what is inside one is on its own page
    cards = []
    for name, module in modules.items():
        if name != top_of(name):
            continue
        below = len(inside(modules, name))
        more = "" if below == 0 else ", %d module%s inside" % (below, "" if below == 1 else "s")
        cards.append(module_card(name, module, more))
    main = """<p class="crumbs">Standard library</p>
      <h1>The standard library</h1>
      <p class="summary">Everything that comes with Sharp %s: %d modules, %d types and %d functions. These pages are made from the library's own source and comments.</p>
      <div class="modgrid">
        %s
      </div>""" % (facts["version"], len(modules), counts["types"], counts["functions"], "\n        ".join(cards))
    body = '<div class="lib"><div class="wrap">\n    %s\n    <main>\n      %s\n    </main>\n</div></div>\n<script src="search.js"></script>\n' \
           % (sidebar(modules, None), main)
    return page("std/index.html", "Standard library - Sharp",
                "The Sharp standard library: every module, class and function.", body, facts)


def js_text(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("<", "\\x3c") + '"'


# ------------------------------------------------------------------------------------------ the whole site

def empty(folder):
    """Empty the folder the site goes in - only when it is safe to (see the top of this file)."""
    os.makedirs(folder, exist_ok=True)
    there = os.listdir(folder)
    if there and ".git" not in there and ".sharp-site" not in there:
        sys.exit("%s is not empty and is not a site this script made: nothing was changed" % folder)
    for name in there:
        if name in (".git", "CNAME"):
            continue
        path = os.path.join(folder, name)
        if os.path.isdir(path) and not os.path.islink(path):
            shutil.rmtree(path)
        else:
            os.remove(path)


def main():
    out = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(ROOT, "build", "site")
    library = os.path.join(ROOT, "lib", "support", newest(os.listdir(os.path.join(ROOT, "lib", "support"))), "std")
    release = newest([n for n in os.listdir(os.path.join(ROOT, "releases")) if n.endswith(".zip")])
    facts = {"version": re.search(r"(\d+(?:\.\d+)+)\.zip$", release).group(1), "branch": branch()}

    empty(out)
    write(os.path.join(out, ".sharp-site"), "made by tools/site/build_site.py\n")
    write(os.path.join(out, ".nojekyll"), "")           # GitHub Pages: serve the files as they are

    for file, title, description in HAND_WRITTEN:
        write(os.path.join(out, file), page(file, title, description, read(os.path.join(HERE, "pages", file)), facts))
    guide = guide_pages(facts)
    for file, title, body in guide:
        write(os.path.join(out, file), page(file, "%s - Sharp guide" % title, "The Sharp guide: %s." % title, body, facts))
    write(os.path.join(out, "sdb.html"), page("sdb.html", "The sdb guide - Sharp",
                                              "Everything sdb, the Sharp debugger, can do.", sdb_guide(facts), facts))
    for name in ("site.css", "site.js"):
        shutil.copy(os.path.join(HERE, name), os.path.join(out, name))
    shutil.copytree(os.path.join(HERE, "shots"), os.path.join(out, "shots"))
    shutil.copy(os.path.join(ROOT, "doc", "logo.png"), os.path.join(out, "logo.png"))
    shutil.copy(os.path.join(ROOT, "doc", "benchmarks.html"), os.path.join(out, "benchmarks.html"))

    modules = read_library(library)
    modules = {name: m for name, m in modules.items() if m["types"] or m["functions"] or m["constants"]}
    index, api = [], []
    for name, module in modules.items():
        write(os.path.join(out, "std", module_file(name)), module_page(name, module, modules, facts, index, api))
    counts = {"types": sum(len(m["types"]) for m in modules.values()),
              "functions": sum(len(m["functions"]) + sum(len(t["functions"]) for t in m["types"]) for m in modules.values())}
    write(os.path.join(out, "std", "index.html"), library_index(modules, facts, counts))
    write(os.path.join(out, "std", "search.js"), "var SHARP_INDEX = [\n%s\n];\n" % ",\n".join(
        "[%s,%s,%s,%s]" % tuple(js_text(part) for part in row) for row in index))
    write(os.path.join(out, "std", "api.txt"), "\n".join(sorted(api)) + "\n")

    print("site: %s" % out)
    print("guide: %d pages from %s" % (len(guide), GUIDE_SOURCE))
    print("Sharp %s, branch %s: %d modules, %d types, %d functions, %d names to search"
          % (facts["version"], facts["branch"], len(modules), counts["types"], counts["functions"], len(index)))


if __name__ == "__main__":
    main()
