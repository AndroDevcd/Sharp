# Hello, Window — the smallest Sharp GUI program

```sharp
import ( gui )

def main(args: string[]) {
    w := gui.window("Sharp GUI - hello", 640, 400);
    while(w.is_open()) {
        e := w.next_event();
        while(e.kind != EVENT_NONE) {
            if(e.kind == EVENT_QUIT) { w.close(); }
            e = w.next_event();
        }
        w.clear(gui.rgb(24, 28, 40));
        w.show();
    }
    println("window closed. bye.");
}
```

That is the whole program. Notice what is **not** here: no `native` block, no
`load_library`, no `settings.json`, no mention of SDL2. The GUI is a library that
ships with Sharp — you just `import gui` and build from `gui.`. A program that
never imports it carries none of it.

```
your program  ->  import gui  ->  the sharpgui native backend  ->  SDL2  ->  a window
```

## Install SDL2 (once, per machine)

The GUI's native backend draws through SDL2, so that one system package must be
present:

| Platform              | Command                                  |
|-----------------------|------------------------------------------|
| Raspberry Pi / Debian | `sudo apt install libsdl2-dev`           |
| macOS (Homebrew)      | `brew install sdl2`                      |
| Windows (MSYS2)       | `pacman -S mingw-w64-ucrt-x86_64-SDL2`   |

## Build and run

```sh
# from examples/gui/hello/src
sharpc main.sharp -o hello     # the installed `sharpc` already knows the gui library
sharp hello
```

A 640×400 window titled **"Sharp GUI - hello"** appears, filled a calm dark blue.
Click the X and the program prints `window closed. bye.` and ends.

The first `gui.window(...)` quietly loads the `sharpgui` native backend that was
installed beside the `sharp` runtime — you never load it yourself.

## Where this sits in the plan

This is **M1** — the base the rest is built on:

- **M2** — drawing on the window: boxes, lines, circles, pictures, text, and mouse/keyboard input. Done: see `examples/gui/shapes`.
- **M3** — the declarative framework: `gui.column()`, `gui.row()`, `gui.button()`, `gui.text()`, `gui.state(...)`, nested with `.add(...)`, rendered onto this window.
- **M5** — games: a `canvas` escape hatch, sprites, audio, gamepad, over this same surface.

## The event codes

`window.next_event()` returns an `event`; its `kind` is one of the `EVENT_*`
constants. Those constants (`lib/support/0.4.0/gui/event.sharp`) and the `EVT_*`
enum in `lib/support/0.4.0/gui/native/sharpgui.cpp` are the **same numbers** and
must stay in step.
