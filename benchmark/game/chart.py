#!/usr/bin/env python3
"""
Turns results.json (run.sh writes it) into

    results.md      a table per test, and the summary
    chart.html      "How many times slower?": the box plot of the Benchmarks Game site

        python3 chart.py results/results.json results

BEFORE AND AFTER. When the folder also holds a `before.json` - Sharp's rows of
an earlier run, in the shape of results.json - those are drawn as languages of
their own, "Sharp -O (before)", so a change to Sharp shows as two boxes. Only
tests run on the same input in both are taken.

HOW MANY TIMES SLOWER. For each test the fastest program is 1; every other
program's time is divided by that one's. A language then has one such number
per test, and its box shows them: the line in the middle is the median, the
box holds the middle half, the whiskers reach the smallest and the largest.
The languages stand in the order of their geometric mean, and the scale is
logarithmic (each step up is a multiplication).
"""
import html
import json
import math
import sys

LEAST_SECONDS = 0.01        # a time below what `time` can measure counts as this


def quantile(sorted_values, q):
    """the value at fraction q of the list, between two entries when it falls there"""
    if len(sorted_values) == 1:
        return sorted_values[0]
    at = q * (len(sorted_values) - 1)
    low = int(math.floor(at))
    high = min(low + 1, len(sorted_values) - 1)
    return sorted_values[low] + (sorted_values[high] - sorted_values[low]) * (at - low)


def summarize(data):
    """per language: its ratios to the fastest program of each test, and what the box needs"""
    names = {l["id"]: l["name"] for l in data["languages"]}
    tests = []
    for r in data["results"]:
        if r["test"] not in tests:
            tests.append(r["test"])

    fastest = {}
    for r in data["results"]:
        if r["status"] == "ok":
            seconds = max(r["seconds"], LEAST_SECONDS)
            fastest[r["test"]] = min(fastest.get(r["test"], seconds), seconds)

    languages = []
    for lang in names:
        points, missing = [], []
        for r in data["results"]:
            if r["language"] != lang:
                continue
            if r["status"] == "ok":
                seconds = max(r["seconds"], LEAST_SECONDS)
                points.append({"test": r["test"], "seconds": r["seconds"], "memory": r["memory_mb"],
                               "ratio": seconds / fastest[r["test"]]})
            else:
                missing.append({"test": r["test"], "status": r["status"]})
        if not points:
            continue
        ratios = sorted(p["ratio"] for p in points)
        languages.append({
            "id": lang, "name": names[lang], "points": points, "missing": missing,
            "low": ratios[0], "q1": quantile(ratios, 0.25), "median": quantile(ratios, 0.5),
            "q3": quantile(ratios, 0.75), "high": ratios[-1],
            "mean": math.exp(sum(math.log(x) for x in ratios) / len(ratios)),
        })
    languages.sort(key=lambda l: l["mean"])
    return tests, languages


def add_before(data, before):
    """Sharp's results of an earlier run join `data` as languages of their own, named like `Sharp -O (before)`"""
    sizes = {r["test"]: r["n"] for r in data["results"]}
    for language in before["languages"]:
        if not language["id"].startswith("sharp"):
            continue
        rows = [dict(r, language=language["id"] + "_before") for r in before["results"]
                if r["language"] == language["id"] and sizes.get(r["test"]) == r["n"]]
        if rows:
            data["languages"].append({"id": language["id"] + "_before", "name": language["name"] + " (before)",
                                      "version": language["version"]})
            data["results"] += rows


def merge(kept_path, new_path):
    """the languages of the run in `new_path` replace their rows in the results at `kept_path`; the others stay"""
    with open(kept_path) as f:
        kept = json.load(f)
    with open(new_path) as f:
        new = json.load(f)
    if kept["size"] != new["size"]:
        print("the results there are of size %s, this run of size %s: not put together" % (kept["size"], new["size"]))
        sys.exit(1)
    again = {l["id"] for l in new["languages"]}
    tests = {r["test"] for r in new["results"]}
    # (a language run again keeps its old rows only for tests this run did not do)
    rows = [r for r in kept["results"] if r["language"] not in again or r["test"] not in tests]
    order = [l["id"] for l in kept["languages"]] + [l["id"] for l in new["languages"] if l["id"] not in
                                                   {k["id"] for k in kept["languages"]}]
    by_id = {l["id"]: l for l in kept["languages"]}
    by_id.update({l["id"]: l for l in new["languages"]})
    kept["languages"] = [by_id[i] for i in order]
    kept["results"] = rows + new["results"]
    kept["updated"] = "%s: %s run again" % (new["date"], ", ".join(sorted(again)))
    with open(kept_path, "w") as f:
        json.dump(kept, f, indent=2)
        f.write("\n")


def kind_of(language):
    """which colour a language's box wears: sharp, before (Sharp's earlier results) or other"""
    if language["id"].endswith("_before"):
        return "before"
    return "sharp" if language["id"].startswith("sharp") else "other"


def times(x):
    return ("%.1f" if x < 10 else "%.0f") % x


def write_markdown(data, tests, languages, path):
    names = {l["id"]: l["name"] for l in data["languages"]}
    out = ["# The Benchmarks Game: Sharp and the others", "",
           "%s. %s. Sizes: %s; the best of %d run(s); the whole run is measured, start to exit." % (
               data["date"], data["machine"], data["size"], data["runs"]), "",
           "## How many times slower than the fastest program", "",
           "| language | geometric mean | median | best | worst | tests |", "|---|---|---|---|---|---|"]
    for l in languages:
        out.append("| %s | %s x | %s x | %s x | %s x | %d of %d |" % (
            l["name"], times(l["mean"]), times(l["median"]), times(l["low"]), times(l["high"]),
            len(l["points"]), len(tests)))
    for test in tests:
        rows = [r for r in data["results"] if r["test"] == test]
        done = sorted((r for r in rows if r["status"] == "ok"), key=lambda r: r["seconds"])
        if not rows:
            continue
        out += ["", "## %s %d" % (test, rows[0]["n"]), "", "| language | seconds | times the fastest | memory |", "|---|---|---|---|"]
        best = max(done[0]["seconds"], LEAST_SECONDS) if done else 1
        for r in done:
            out.append("| %s | %.2f | %s x | %d MB |" % (names[r["language"]], r["seconds"],
                                                         times(max(r["seconds"], LEAST_SECONDS) / best), r["memory_mb"]))
        for r in rows:
            if r["status"] != "ok":
                out.append("| %s | %s | | |" % (names[r["language"]], r["status"]))
    out += ["", "## Versions", ""]
    for l in data["languages"]:
        out.append("- %s: %s" % (l["name"], l["version"]))
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")


def write_chart(data, tests, languages, path):
    width, height = 60 + 68 * len(languages) + 20, 460
    left, top, bottom = 60, 20, 92
    plot_height = height - top - bottom
    highest = max(l["high"] for l in languages)
    ticks = [t for t in (1, 2, 3, 5, 10, 20, 30, 50, 100, 200, 300, 500, 1000, 2000, 3000, 5000) if t <= highest * 1.6]
    top_value = ticks[-1] if ticks[-1] >= highest else highest * 1.1

    def y(value):
        return top + plot_height * (1 - math.log(value) / math.log(top_value))

    svg = ['<svg viewBox="0 0 %d %d" width="%d" role="img" aria-label="How many times slower each language is than the fastest program, per test">' % (width, height, width)]
    for t in ticks:
        svg.append('<line class="grid" x1="%d" x2="%d" y1="%.1f" y2="%.1f"/>' % (left, width - 10, y(t), y(t)))
        svg.append('<text class="axis" x="%d" y="%.1f" text-anchor="end">%s×</text>' % (left - 8, y(t) + 4, t))
    for i, l in enumerate(languages):
        cx = left + 68 * i + 40
        kind = kind_of(l)
        svg.append('<g class="%s">' % kind)
        svg.append('<line class="whisker" x1="%d" x2="%d" y1="%.1f" y2="%.1f"/>' % (cx, cx, y(l["high"]), y(l["low"])))
        for end in (l["high"], l["low"]):
            svg.append('<line class="whisker" x1="%d" x2="%d" y1="%.1f" y2="%.1f"/>' % (cx - 8, cx + 8, y(end), y(end)))
        box_top, box_bottom = y(l["q3"]), y(l["q1"])
        svg.append('<rect class="box" x="%d" y="%.1f" width="36" height="%.1f" rx="3"/>' % (cx - 18, box_top, max(box_bottom - box_top, 2)))
        svg.append('<line class="median" x1="%d" x2="%d" y1="%.1f" y2="%.1f"/>' % (cx - 18, cx + 18, y(l["median"]), y(l["median"])))
        for p in l["points"]:
            tip = "%s · %s: %.2f s, %s× the fastest, %d MB" % (l["name"], p["test"], p["seconds"], times(p["ratio"]), p["memory"])
            svg.append('<circle class="dot" cx="%d" cy="%.1f" r="4" data-tip="%s"/>' % (cx, y(p["ratio"]), html.escape(tip, quote=True)))
        svg.append('</g>')
        words = l["name"].replace(" (", "|(").split("|")
        for line, word in enumerate(words):
            svg.append('<text class="name%s" x="%d" y="%d" text-anchor="middle">%s</text>' % (
                " strong" if kind != "other" else "", cx, height - bottom + 20 + 14 * line, html.escape(word)))
        svg.append('<text class="axis" x="%d" y="%d" text-anchor="middle">%s×</text>' % (cx, height - bottom + 52, times(l["mean"])))
        if l["missing"]:
            svg.append('<text class="axis" x="%d" y="%d" text-anchor="middle">%d of %d</text>' % (cx, height - bottom + 66, len(l["points"]), len(tests)))
    svg.append('</svg>')

    rows = []
    for l in languages:
        cells = []
        for test in tests:
            point = next((p for p in l["points"] if p["test"] == test), None)
            gone = next((m for m in l["missing"] if m["test"] == test), None)
            cells.append("<td>%s</td>" % ("%.2f s<small>%s×</small>" % (point["seconds"], times(point["ratio"])) if point
                                          else html.escape(gone["status"]) if gone else ""))
        rows.append('<tr%s><th>%s</th><td>%s×</td>%s</tr>' % (
            ' class="%s"' % kind_of(l) if kind_of(l) != "other" else "", html.escape(l["name"]), times(l["mean"]), "".join(cells)))
    sizes = {r["test"]: r["n"] for r in data["results"]}
    head = "".join("<th>%s<small>%s</small></th>" % (t, sizes[t]) for t in tests)

    legend = '<span><i style="background: var(--sharp)"></i>Sharp</span>'
    if any(kind_of(l) == "before" for l in languages):
        legend += '<span><i style="background: var(--before)"></i>Sharp before the change</span>'
    legend += '<span><i style="background: var(--other)"></i>the other languages</span>'
    page = PAGE.replace("@LEGEND@", legend).replace("@SVG@", "\n".join(svg)).replace("@ROWS@", "\n".join(rows)).replace("@HEAD@", head)
    page = page.replace("@WHEN@", html.escape("%s · %s · sizes: %s · best of %d run(s)" % (data["date"], data["machine"], data["size"], data["runs"])))
    with open(path, "w") as f:
        f.write(page)


PAGE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sharp in the Benchmarks Game</title>
<style>
  :root { --page: #f9f9f7; --surface: #fcfcfb; --ink: #0b0b0b; --ink2: #52514e; --muted: #898781; --grid: #e1e0d9; --axis: #c3c2b7;
          --other: #2a78d6; --sharp: #eb6834; --before: #1baf7a; }
  @media (prefers-color-scheme: dark) {
    :root { --page: #0d0d0d; --surface: #1a1a19; --ink: #f0efec; --ink2: #c3c2b7; --grid: #2c2c2a; --axis: #383835;
            --other: #3987e5; --sharp: #d95926; --before: #199e70; }
  }
  body { margin: 0; padding: 24px 16px 48px; background: var(--page); color: var(--ink); font: 15px/1.5 system-ui, "Segoe UI", sans-serif; }
  main { max-width: 1180px; margin: 0 auto; }
  h1 { font-size: 22px; margin: 0 0 4px; }
  p { color: var(--ink2); margin: 0 0 12px; max-width: 78ch; }
  .card { background: var(--surface); border: 1px solid var(--grid); border-radius: 10px; padding: 16px; overflow-x: auto; position: relative; }
  .legend { display: flex; gap: 18px; color: var(--ink2); font-size: 13px; margin-bottom: 4px; }
  .legend i { display: inline-block; width: 12px; height: 12px; border-radius: 3px; margin-right: 6px; vertical-align: -1px; }
  svg { display: block; max-width: none; }
  .grid { stroke: var(--grid); stroke-width: 1; }
  .axis { fill: var(--muted); font-size: 12px; }
  .name { fill: var(--ink2); font-size: 12.5px; }
  .name.strong { fill: var(--ink); font-weight: 600; }
  .other { color: var(--other); } .sharp { color: var(--sharp); } .before { color: var(--before); }
  .whisker { stroke: currentColor; stroke-width: 2; }
  .box { fill: currentColor; fill-opacity: .22; stroke: currentColor; stroke-width: 2; }
  .median { stroke: currentColor; stroke-width: 3; }
  .dot { fill: currentColor; stroke: var(--surface); stroke-width: 2; cursor: default; }
  .dot:hover { r: 6; }
  #tip { position: fixed; pointer-events: none; background: var(--ink); color: var(--surface); padding: 6px 9px; border-radius: 6px; font-size: 12.5px; display: none; z-index: 5; }
  table { border-collapse: collapse; width: 100%; font-size: 13.5px; font-variant-numeric: tabular-nums; }
  th, td { text-align: right; padding: 6px 10px; border-bottom: 1px solid var(--grid); white-space: nowrap; }
  th:first-child { text-align: left; }
  thead th { color: var(--ink2); font-weight: 500; }
  small { display: block; color: var(--muted); font-size: 11.5px; font-weight: 400; }
  tr.sharp th { box-shadow: inset 3px 0 0 var(--sharp); }
  tr.before th { box-shadow: inset 3px 0 0 var(--before); }
  h2 { font-size: 16px; margin: 28px 0 8px; }
</style>
</head>
<body>
<main>
  <h1>How many times slower?</h1>
  <p>Six programs of the Benchmarks Game, written the same plain single-threaded way in every language. For each test the
     fastest program is 1×. A box holds the middle half of a language's six results, the line in it is the median, the whiskers
     reach its best and worst test, and every dot is one test. The number under a name is the geometric mean. The scale is logarithmic.</p>
  <p>@WHEN@</p>
  <div class="card">
    <div class="legend">@LEGEND@</div>
    @SVG@
  </div>
  <h2>The measurements</h2>
  <div class="card">
    <table>
      <thead><tr><th>language</th><th>geometric mean</th>@HEAD@</tr></thead>
      <tbody>
      @ROWS@
      </tbody>
    </table>
  </div>
</main>
<div id="tip"></div>
<script>
  const tip = document.getElementById('tip');
  document.querySelectorAll('.dot').forEach(dot => {
    dot.addEventListener('mousemove', e => {
      tip.textContent = dot.dataset.tip;
      tip.style.display = 'block';
      tip.style.left = Math.min(e.clientX + 14, window.innerWidth - tip.offsetWidth - 8) + 'px';
      tip.style.top = (e.clientY - 34) + 'px';
    });
    dot.addEventListener('mouseleave', () => { tip.style.display = 'none'; });
  });
</script>
</body>
</html>
"""


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--merge":
        merge(sys.argv[2], sys.argv[3])
        return
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(2)
    with open(sys.argv[1]) as f:
        data = json.load(f)
    try:
        with open(sys.argv[2] + "/before.json") as f:
            add_before(data, json.load(f))
    except FileNotFoundError:
        pass
    tests, languages = summarize(data)
    if not languages:
        print("no program ran to the end: nothing to draw")
        sys.exit(1)
    write_markdown(data, tests, languages, sys.argv[2] + "/results.md")
    write_chart(data, tests, languages, sys.argv[2] + "/chart.html")
    print("\nhow many times slower than the fastest program (geometric mean of the tests):")
    for l in languages:
        print("  %-20s %7s x   %d of %d tests" % (l["name"], times(l["mean"]), len(l["points"]), len(tests)))


main()
