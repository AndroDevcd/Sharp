#!/usr/bin/env python3
"""
from_mockup.py  -  cuts a design mock-up (one HTML file with every page in it) into this folder's
source files: site.css, site.js and pages/*.html. Run it when the look was changed in the mock-up:

    python3 tools/site/from_mockup.py <mock-up.html>

The mock-up keeps each page in a <div id="view-NAME">; links between pages are written #NAME there
and become real addresses here. extra.css (the styles only the generated pages need) is put at the
end of site.css. Pictures embedded in the mock-up are pointed back at the files in shots/.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# the mock-up's name for a page -> its file
FILES = {"home": "index.html", "gui": "gui.html", "projects": "projects.html", "install": "install.html",
         "debugger": "debugger.html", "speed": "performance.html"}
LINKS = dict(FILES, guide="guide/index.html", library="std/index.html")
SHOTS = [("png", "counter.png"), ("jpeg", "charts.jpg"), ("jpeg", "sharpcraft.jpg")]

# what the mock-up could only pretend: where a link really goes, and code that was corrected after it was drawn
FIXES = [
    ('about 160 times faster than Sharp was in 2020', 'up to 160 times faster than Sharp was in 2020'),
    ('<a class="btn" href="{root}debugger.html">Read the full sdb guide</a>',
     '<a class="btn" href="{root}sdb.html">Read the full sdb guide</a>'),
    ('<a href="{root}install.html">Get the plugin</a>', '<a href="{github}/tree/{branch}/tools/clion-plugin">Get the plugin</a>'),
    ('<a class="btn" href="{root}performance.html">Open the interactive chart</a>',
     '<a class="btn" href="{root}benchmarks.html">Open the interactive chart</a>'),
    # (until a release is published on GitHub Releases, the zip in the repository is the latest)
    ('href="https://github.com/AndroDevcd/Sharp/releases/latest"', 'href="{github}/raw/{branch}/releases/sharp-{version}.zip"'),
    ('<span>One zip for Windows and 64-bit Linux, from GitHub Releases.</span>', '<span>One zip for Windows and 64-bit Linux.</span>'),
    ('<a href="https://github.com/AndroDevcd/Sharp/releases">All releases</a>', '<a href="{github}/tree/{branch}/releases">All releases</a>'),
    ('Download <code>sharp-{version}.zip</code> from the releases folder, unpack', 'Download the zip, unpack'),
    ('''    cube(1, 1, 1).at(0, 0.5, 0).color(0x6AA84FFF);

    eye := camera();
    eye.at(0, 2, 6);
    on_frame { f: frame -> eye.facing(f.time * 20, 10); };''',
     '''    box := cube(1, 1, 1).at(0, 0.5, 0).color(0x6AA84FFF);
    camera().at(0, 2, 4).look_at(0, 0.5, 0);

    on_frame { f: frame -> box.turn(f.time * 40); };'''),
]


def main():
    text = open(sys.argv[1], encoding="utf-8").read()
    for kind, name in SHOTS:
        text = re.sub(r'src="data:image/%s;base64,[^"]+"' % kind, 'src="{root}shots/%s"' % name, text, count=1)

    css = re.search(r"<style>\n(.*?)</style>", text, re.S).group(1)
    css = re.sub(r"/\* ---- the note that this is a mock-up \*/\n.*?\n\n", "", css, flags=re.S)
    css = css.replace("\nbody { background:", "\nbody { margin: 0; background:")
    css += "\n" + open(os.path.join(HERE, "extra.css"), encoding="utf-8").read()
    write("site.css", css)

    js = re.search(r"<script>\n(.*?)</script>", text, re.S).group(1)
    js = js[:js.index("// three pages on the hash")].rstrip() + "\n\n"
    write("site.js", js + open(os.path.join(HERE, "extra.js"), encoding="utf-8").read())

    for name, file in FILES.items():
        found = re.search(r'(<div id="view-%s"[^>]*>)\n(.*?)\n</div>\n\n<(?:!--|footer)' % name, text, re.S)
        look = re.search(r'class="([^"]+)"', found.group(1))
        body = found.group(2)
        for target, address in LINKS.items():
            body = body.replace('href="#%s"' % target, 'href="{root}%s"' % address)
        body = body.replace("0.5.11", "{version}")
        for old, new in FIXES:
            body = body.replace(old, new)
        if look:
            body = '<div class="%s">\n%s\n</div>' % (look.group(1), body)
        write("pages/" + file, body + "\n")
        print(file, len(body))


def write(name, text):
    with open(os.path.join(HERE, name), "w", encoding="utf-8", newline="\n") as file:
        file.write(text)


if __name__ == "__main__":
    main()
