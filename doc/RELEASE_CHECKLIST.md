# What is left before the release on the main branch

Rewritten 2026-11-25, after the repository clean-up. The version to be released is 0.5.12.
Tick a line off by deleting it.

## Done (kept for one release, so it is clear what was decided)

- Licence: Apache License 2.0. `LICENSE` at the top, `doc/NOTICE`, `doc/THIRD_PARTY_LICENSES.txt`;
  every release zip carries the three.
- The old compression code (SCZ, under the LGPL) is out of the VM; `legacy/README.md` says what the
  archive still holds.
- The layout: `runtime/`, `debugger/`, `compiler/`, `stdlib/` at the top; `lib/` is gone.
- Every example and benchmark compiles, and the tests fail when one does not (test 11).
- Whole-number division, the new serialization, UI tests, view-model tests, pictures from links.
- Linux: the full tests passed on 2026-11-25, before the clean-up. ARM64 and ARM32 under qemu too.

## To do with Claude

**Prove the new layout, end to end**

1. The Windows builds: the VM, the debugger, the GUI library.
2. A release run (`release.sh install`): the zip must carry the licence files and a VM without
   the old compression code. The VM changed, so its number moves (4.0.12 -> 4.0.13).
3. The full tests on Linux (`bash test.sh full`), and the Windows suite, which has never had a full run.
4. Rebuild the site: its "source" links point at the old paths until then.
5. Commit and push.

**New in the language, decided 2026-11-25** (after the steps above)

6. Dictionary literals.
7. `--watch`: build again when a file changes.
8. The shorter `for`.
9. `with(x) as c { } else { }` - still to be decided.

**Clean-up left**

10. `tests/suite/known_bugs/README.md`: its status is from 12 November. Check each bug; fixed ones leave.
11. The library: `hash_map`, `hash_table` and `dictionary` side by side; `atomic.sharp`; the C++ random
    generator, to be replaced by the Sharp one.
12. The VM: what is left of the old profiler (`SHARP_PROF_`, in three files).
13. The name lookup in `stdlib/std/net/udp.sharp` has no thread guard yet; the picture cache has no size limit.

**Before it goes out**

14. Audit the examples and benchmarks for divisions of two whole numbers that relied on a fraction.
15. A bug sweep on what the site advertises: injection, testing, GUI, C calls.
16. Decide on the warning gap: a parameter that hides a field is not flagged, a local is (W1036).
17. `GETTING_STARTED.md`: the division rule and the newer GUI calls. `doc/CHANGELIST.md`: read the 0.5.12 entry.
18. The site: the real logo; look at the projects, download and performance pages, dark mode and phone width.
19. Look into `nano_time()`: it once gave a negative difference in WSL.

**The release itself**

20. Merge `runtime-3.0.0_r8` into `master` (they diverged long ago: check how first).
21. Rebuild the site from `master`, so "source" links and the download button leave the working branch.

## Only Braxton can do

1. Publish on GitHub Releases (the newest there is v0.2.9 from 2020); then the site's download button
   is pointed at it, and the zips in `releases/` can leave the repository.
2. Check GitHub Pages is on: Settings > Pages, source `gh-pages`.
3. Play-test Sharpcraft and Abyss by hand.
4. Decide whether Escape always lets a kept mouse go, in the library itself.
5. Test on a real Raspberry Pi: the installer, the GUI, sdb.
6. Mac: needs someone with a Mac to run the build.

## Parked on purpose

- Tests on every push (CI). - VS Code support. - `list<decimal>` (generics over plain numbers). - A package manager.
- Ahead-of-time compiling: the JIT is fast enough.

## The true blockers

The Windows suite, the merge to `master`, a published GitHub release.
