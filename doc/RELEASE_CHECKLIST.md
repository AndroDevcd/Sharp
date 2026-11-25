# What is left before a release on the main branch

Written 2026-11-24, with everything on `runtime-3.0.0_r8` uncommitted and Sharp 0.5.11 installed.
Tick a line off by deleting it.

## To do with Claude

**Finish what is in flight**

1. Commit the work on `runtime-3.0.0_r8`: whole-number division, the mock fix, the GUI additions
   (`gui.keep_mouse`, `gui.app_name`, `gui.app_icon`, `gui.quit`), `tools/site`, the `sharp-site`
   skill, the build-script fix.
2. Run the rest of the full tests: compiler, library, debugger, tools, programs, and the Windows
   half. (Done 2026-11-25, Linux: `bash test.sh vm full linux` - the suite four ways, kind checks,
   collector stress, calls to C++, ARM. All passed.)
3. Audit `examples/`, `benchmark/`, Project Dragonfly and Sharpcraft for divisions of two whole
   numbers that relied on a fraction (`pow(27, 1 / 3)` is now `pow(27, 0)`).
4. (Done 2026-11-25: ARM64 and ARM32 under qemu, 207 of 207 cases each, with whole-number
   division and the new serialization among them. A real Raspberry Pi is still untested.)
5. Commit Project Dragonfly's channel-bar fix (uncommitted in that repository).

**Added 2026-11-25: work since this list was written, all uncommitted**

- The 3D library for Abyss: pieces (`lump`, `tube`, `blade`), `flock`, `motes`, `underwater`, `bloom`,
  sound, `endless` land, `steeper`, finer cuts, copies drawn in one call. Commit with item 1.
- Serialization rewritten (`lib/Modules/std.io/serialization.cpp`): the format changed, so data
  serialized by 0.5.11 is refused. Say so in the release notes. The VM's number moves for it too.
- Run the Windows suite on the new serialization and the new 3D drawing (Linux and ARM have passed).
- Look into `nano_time()`: it once gave a negative difference in WSL.
- The docs and the site do not know any of the new 3D words yet (item 8, and the site's library pages).

**Bug sweep on what the site advertises**

6. Injection, testing, GUI and C calls, the way mocks were swept.
7. Decide on the warning gap: a parameter that hides a field is not flagged, a local is (W1036).

**Documents and the site**

8. `GETTING_STARTED.md` (the site's guide is built from it): the division rule and the new GUI calls.
9. `README.md`: the site's address, the "six years of Sharp" numbers if wanted.
10. The site: the real logo in place of the `#` mark; look at the projects, download and
    performance pages, dark mode and phone width (never looked at).

**The release**

11. `release.sh <version> install`. The language number moves, and the VM's too (it has a new
    instruction, WDIV).
12. Merge `runtime-3.0.0_r8` into `master` (they diverged long ago: check how first).
13. Rebuild the site from `master`, so "source" links and the download button leave the working branch.

## Only Braxton can do

1. Choose a licence (there is no licence file).
2. Pick the version number: 0.5.12, 0.6.0 or a 1.0 beta.
3. Publish on GitHub Releases (the newest there is v0.2.9 from 2020); then the site's download
   button is pointed at it.
4. Check GitHub Pages is on: Settings > Pages, source `gh-pages`.
5. Play-test Sharpcraft by hand: mouse look, Escape, the Windows key, the pause buttons.
6. Decide whether Escape always lets a kept mouse go, in the library itself.
7. Test on a real Raspberry Pi: the installer, the GUI, sdb, Dragonfly's channel bars.
8. Mac: needs someone with a Mac to run the build.
9. Play-test Abyss by hand (added 2026-11-25): swimming, the mouse, Escape and Resume, the menu,
   the tour, the tunnel, a long swim out of the home waters, the sound.

## Parked on purpose

- Tests on every push (CI). - VS Code support. - `list<decimal>` (generics over plain numbers). - A package manager.

## The true blockers

The rest of the full test run (Windows above all), the licence, the merge to `master`, a published GitHub release.
