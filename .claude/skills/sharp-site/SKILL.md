---
name: sharp-site
description: Rebuild the Sharp website (GitHub Pages, branch gh-pages) from the repository and say what changed against the site that is live - above all the standard library's documentation, which is generated from lib/support. Use when the user asks to "update the site", "update the docs", "rebuild the website", "refresh the library documentation", "is the site up to date", or after a release or a library change they want on the site.
---

# Sharp site: rebuild, compare with what is live, publish on the user's word

The site is plain files on the `gh-pages` branch. Nothing on that branch is written by hand:
everything comes out of `tools/site/build_site.py`, run on the branch being worked on.

| what | where |
|---|---|
| the generator | `tools/site/build_site.py` (its top says what it makes) |
| pages written by hand | `tools/site/pages/*.html` (home, gui, projects, install) |
| the look, the scripts, the screenshots | `tools/site/site.css`, `site.js`, `shots/` |
| the library's pages | generated from `lib/support/<version>/std` - never edited |
| the built site | `build/site`, a git worktree of `gh-pages` |

## Ground rules

- Never push and never commit to `gh-pages` unless the user asks in that turn: a push changes the live site.
- Never edit files in `build/site` by hand: change `tools/site` or the library's comments and build again.
- A wrong or thin page in the library section is fixed in the library's `/** */` comment, not on the site.
- Code shown on a hand-written page must have been compiled and run as shown. Say so when it was not.
- Report faithfully: what changed, what was not looked at.

## Steps

### 1. The worktree

`build/site` must be a checkout of `gh-pages` at what is live:

```
git fetch origin gh-pages
git worktree list                                  # is build/site there?
git worktree add build/site gh-pages               # only when it is not (remove a plain build/site folder first)
git -C build/site status --short                   # must be clean before step 2 ...
git -C build/site merge --ff-only origin/gh-pages  # ... and at what is live
```

If `build/site` has changes nobody committed, they are an earlier build: say so, and ask before
throwing them away.

### 2. Build

In WSL (the generator needs Python 3 and git):

```
MSYS_NO_PATHCONV=1 wsl bash -lc 'cd "/mnt/c/Users/Braxton Nunnally/Documents/Github/Sharp" && python3 tools/site/build_site.py'
```

It must end with `Sharp <version>, branch <branch>: N modules, N types, N functions`. The version is
the newest zip in `releases/`; the "source" links point at the branch it was run on.

### 3. What changed against the live site

```
git -C build/site status --short                   # pages added, changed, gone
git -C build/site diff --stat
git -C build/site diff -- std/api.txt              # THE LIBRARY: one line for every documented name
```

`std/api.txt` is the list to read: a `+` line is a name that is new or changed, a `-` line one that
is gone or was changed. Tell the user, grouped by module:

- what was added, removed and changed in the library (names, not a dump of the diff);
- which hand-written pages changed;
- anything that looks wrong: a module that vanished, a count that fell sharply, a page that is empty.

When nothing changed, say that the site is up to date and stop.

### 4. Look at it

Open the pages that changed in a browser before publishing (`build/site/index.html` works from the
disk). With Edge: `msedge --headless --screenshot=<file> --window-size=1280,1500 file:///...`.

### 5. Publish - only when the user says so

```
git -C build/site add -A
git -C build/site commit -m "Site for Sharp <version>: <what changed, in a line>"
git -C build/site push origin gh-pages
```

The live site follows a minute or two after the push (the repository's Settings > Pages must
publish from the `gh-pages` branch). Tell the user the address and what was published.
