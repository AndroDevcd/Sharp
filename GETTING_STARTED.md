# Getting Started with Sharp

From nothing to a running program, a passing test and a window.

1. [Install](#1-install)
2. [Your first program](#2-your-first-program)
3. [A program with a class](#3-a-program-with-a-class)
4. [A tour of the language](#4-a-tour-of-the-language)
5. [Your first test](#5-your-first-test)
6. [Your first window](#6-your-first-window)
7. [Where to go next](#7-where-to-go-next)

## 1. Install

Download the newest `sharp-<version>.zip` from the [`releases`](releases) folder and unpack
the WHOLE archive (the installers need the folders next to them). `START-HERE.txt` inside
says the same as below.

**Windows** - open the `windows` folder and double-click `win-install.cmd` (or run it in a terminal).

**Linux (64-bit x86)** - in a terminal, inside the unpacked folder:

```
bash linux/unix-install.sh
```

**Raspberry Pi, or any other Linux** - the VM is built on the machine itself:

```
git clone https://github.com/AndroDevcd/Sharp.git
cd Sharp
bash build-scripts/linux/install_from_source.sh
```

The installer needs no administrator rights. It copies Sharp into your user folder
(Windows: `%LOCALAPPDATA%\Sharp`, Linux: `~/.sharp`), adds it to your PATH, and then
compiles and runs a small program to show that everything works.

**Open a NEW terminal afterwards** - a terminal that was already open does not see the new
PATH. (A terminal inside an IDE sees it once the IDE itself is restarted.)

To remove Sharp again: `win-uninstall.cmd` on Windows, `sh unix-uninstall.sh` on Linux.

## 2. Your first program

Put this in a file called `hello.sharp`:

```javascript
mod app;

def main() {
    name := "world";
    println("hello, $name!");
}
```

Compile it, then run it:

```
sharpc hello.sharp -o hello
sharp hello
```

```
hello, world!
```

There are two commands:

| | |
|---|---|
| `sharpc file.sharp -o name` | the compiler: writes the program `name`. The standard library is always included. Add `-O` to make the program faster; `sharpc --h` lists every option. |
| `sharp name` | runs a compiled program |

Every file starts with `mod <name>;` (the module it belongs to), and a program starts at `main`.

## 3. A program with a class

```javascript
mod app;

class basket {
    private items : list<string> = new list<string>();

    def add(item: string) { items.add(item); }
    def count() : _int64 { return items.size(); }

    def describe() : string {
        line := "";
        foreach(item in items) {
            if(line != "") { line = line + ", "; }
            line = line + item;
        }
        return line;
    }
}

def main(args: string[]) {
    b := basket();
    b.add("milk");
    b.add("bread");
    foreach(extra in args) {            
        b.add(extra);
    }

    println("${b.count()} items: ${b.describe()}");
}
```

```
sharpc basket.sharp -o basket
sharp basket eggs
```

```
3 items: milk, bread, eggs
```

What is new here:

- `name : type = value` declares a field or variable; `name := value` works the type out.
- `def name(parameter: type) : result_type { ... }` is a function.
- `list<string>` comes from the standard library; nothing has to be imported for it.
- `main(args: string[])` receives what was typed after the program's name.

## 4. A tour of the language

Interfaces, generics, null safety, `when`, and blocks passed to functions, in one program:

```javascript
mod app;

interface shape {
    def name() : string;
    def area() : decimal;
}

class circle : shape {
    radius : decimal;
    circle(radius: decimal) { self->radius = radius; }

    override def name() : string { return "circle"; }
    override def area() : decimal { return 3.14159 * radius * radius; }
}

class rectangle : shape {
    width : decimal;
    height : decimal;
    rectangle(width: decimal, height: decimal) {
        self->width = width;
        self->height = height;
    }

    override def name() : string { return "rectangle"; }
    override def area() : decimal { return width * height; }
}

// a generic function: one definition, any type
def largest<T>(items: list<T>, score: (T)(decimal)) : T {
    best := items.at(0);
    foreach(item in items) {
        if(score(item) > score(best)) { best = item; }
    }
    return best;
}

// `shape?` may be null, and the compiler makes you deal with that
def find(shapes: list<shape>, name: string) : shape? {
    foreach(s in shapes) {
        if(s.name() == name) { return s; }
    }
    return null;
}

def corners(s: shape) : decimal {
    when(s.name()) {
        "circle" -> { return 0; }
        "rectangle", "square" -> { return 4; }
    }
    return 3;
}

def main() {
    shapes := new list<shape>();
    shapes.add(circle(2));                  // `new` is optional
    shapes.add(rectangle(3, 5));

    foreach(s in shapes) {
        println("${s.name()}: area ${s.area()}, ${corners(s)} corners");
    }

    // a block after a call is its last argument, and `it` is its parameter
    big := largest<shape>(shapes) { it.area() };
    println("largest: ${big.name()}");

    missing := find(shapes, "triangle");
    println("triangle: " + (missing?.name() ?: "not found"));
}
```

```
circle: area 12.56636, 0 corners
rectangle: area 15, 4 corners
largest: rectangle
triangle: not found
```

What that used:

| | |
|---|---|
| `x := value` | the type is worked out for you |
| `"$name"`, `"${a + b}"` | values inside text |
| `class circle : shape` | a class that implements an interface (`class dog base animal` inherits) |
| `override def` | the class's own version of a function the interface declares |
| `def largest<T>(...)` | generic functions, next to generic classes like `list<T>` |
| `shape?`, `x?.y`, `x ?: y`, `x!!` | types that may be null, and the three ways to handle one |
| `when(value) { ... }` | a multi-way branch, with no fall through |
| `f(a) { it.area() }` | a trailing block; `{ a, b -> ... }` names its parameters |
| `(T)(decimal)` | the type of a function that takes a `T` and gives a `decimal` |

## 5. Your first test

Tests are an ordinary program that uses `std.testing`:

```javascript
mod app;

import (
    std.testing
)

def add(a: decimal, b: decimal) : decimal { return a + b; }

def main() {
    tests("adding") {
        test("two and two is four") {
            expect(add(2, 2)).should_be(4);
        };
        test("adding zero changes nothing") {
            expect(add(7, 0)).should_be(7);
        };
    };

    finish_tests();
}
```

```
adding
  ok    two and two is four
  ok    adding zero changes nothing

all 2 tests passed
```

The program's exit code is 0 only when every test passed. Mocks (`mock<an_interface>()`)
and stand-ins for injected objects are shown in the [README](README.md) and in
[`examples/testing`](examples/testing).

## 6. Your first window

```javascript
mod app;

import (
    std.guikit.*
)

class counter base composable {
    count := state(0);

    override def build() : element {
        return column(modifier().fill()) {
            text("count: " + count.value).size(32);
            row {
                button("-") { count.value -= 1; }.outline();
                button("Reset") { count.value = 0; }.plain();
                button("+") { count.value += 1; };
            }.gap(8);
        }.center().gap(16);
    }
}

def main() {
    gui.run("Counter", 360, 240, counter());
}
```

```
sharpc counter.sharp -o counter
sharp counter
```

`build()` says what the screen looks like for the state as it is now; the buttons only
change the state, and the screen follows.

The GUI draws through SDL2:

- **Windows** - nothing to install: SDL2 is inside the `sharpgui.dll` that came with Sharp.
- **Raspberry Pi / Debian / Ubuntu** - `sudo apt install libsdl2-dev`

## 7. Your first 3D scene

A scene is a widget, written the way a screen is: the block says what is in it.

```javascript
mod app;

import (
    std.guikit.*
)

def main() {
    gui.run("Yard", 900, 600, { ->
        return scene {
            camera().orbit();                                   // drag to look round, wheel to zoom
            sun().from(0.6, 0.7, 0.4).shadows();
            ground(30).texture(made_texture("grass", 1), 10);

            cube(1).at(0, 0.5, 0).color(0xD63031FF).spin(40);
            sphere(0.5).at(2, 0.5, 0).color(0xC0C8D0FF).shiny(0.9);
            add(tree(7)).at(0 - 3, 0, 0 - 2);
            group {                                             // a street lamp: moves as one thing
                cylinder(0.06, 2.4).at(0, 1.2, 0).color(0x2D3436FF);
                sphere(0.18).at(0, 2.5, 0).color(0xFFE9A0FF).glow(1);
                lamp().at(0, 2.5, 0).reach(7);
            }.at(3, 0, 2);
        };
    });
}
```

- **Things** are made by words - `cube`, `sphere`, `cylinder`, `cone`, `ground`, `tree`,
  `pine`, `model("file.gltf")` - and what a thing is like is chained onto it: `.at(x, y, z)`,
  `.turn(degrees)`, `.scale(s)`, `.color(0xRRGGBBAA)`, `.texture(path)`, `.shiny(...)`,
  `.spin(...)`, `.click { ... }`.
- **The space**: x to the right, y up, z towards you; one unit is a metre.
- **Colours are numbers**: `0xD63031FF` is red, green, blue and alpha, two hex digits each.
  One kept in a variable needs its type written: `red : _int64 = 0xD63031FF;`.
- **Your own models** come from `.obj`, `.gltf` and `.glb` files; your own shapes from
  `mesh` (corners and triangles). `terrain` makes land from a height function and
  `scatter(thing, count)` strews copies over it.
- **Your own code** runs before every frame in `on_frame { f: frame -> ... }` (the keys, the
  time passed), and ordinary widgets go over the scene with `hud { ... }`.

[`examples/gui/lantern_hunt`](examples/gui/lantern_hunt) is a complete small game built
from these; `stdlib/std/guikit/words3d.sharp` lists every word.

Scenes are drawn with OpenGL, which every PC has; on a Raspberry Pi it is OpenGL ES 2.

## 8. Calling a C library

A Sharp program can call the functions of any C library directly. List them in an `extern`
block, each without a body and under its C name:

```
mod app;

import ( std.c )

extern "c" {
    def strlen(text: string) : c_size;
    def qsort(items: _int32[], count: c_size, width: c_size, compare: (pointer, pointer)(c_int));
}

def main() {
    println("strlen says " + strlen("hello, C"));

    numbers := new _int32[] { 42, 7, 19, 3 };
    qsort(numbers, 4, 4, { a: pointer, b: pointer ->
        return c_memory.view(a, 4).int32_at(0) - c_memory.view(b, 4).int32_at(0);
    });
    println("sorted by C: " + numbers[0] + " " + numbers[1] + " " + numbers[2] + " " + numbers[3]);
}
```

```
strlen says 8
sorted by C: 3 7 19 42
```

- **`extern "c"`** is C's own library, the functions every C program has. Any other name is a
  library file: `extern "SDL2" { ... }` opens `SDL2.dll` on Windows and `libSDL2.so` on Linux.
  `c_library("sdl", "path/or/name")` says which file a name is, and so does `"c_libraries"`
  in a project's `settings.json`.
- **The types** are C's by their C names: `c_int`, `c_float`, `c_double`, `c_size`, `pointer`
  (an address) and so on. A `string` is handed over as a C string, an array of numbers as
  the address of its items.
- **A C struct** is a class based on `c_struct` whose fields are the members. Written as a
  parameter it travels by value; written with a `?` it is a pointer, and what C writes
  through it is in your object after the call.
- **A function** can be handed to C, as `qsort` is handed the comparison above.
- **When something is wrong** - the library is not found, the function is not in it - the
  message lists what was looked for. A crash inside C names the C function, how it was
  declared and what it was handed.

[`compiler/doc/extern.md`](compiler/doc/extern.md) has all of it.

## 9. Where to go next

- The [README](README.md) shows what sets Sharp apart: dependency injection in the
  language, mocks the compiler writes, the GUI toolkit and coroutines.
- [`examples`](examples) has sample programs; [`examples/gui`](examples/gui) goes from a
  to-do list to a full task manager, and to a 3D game.
- [`tests/suite/cases`](tests/suite/cases) has one small, commented program per language
  feature and library - the quickest way to see how something is written.
- [`tools/clion-plugin`](tools/clion-plugin) is an editor plugin for CLion and IntelliJ IDEA.
- [`build-scripts/README.md`](build-scripts/README.md) explains how to build Sharp itself,
  run its tests and make a release.

#### If the compiler misbehaves

```
sharpc --unoptimized file.sharp -o name
```

uses a second copy of the same compiler that was built without the optimizer. If that one
works where the normal one does not, you have found a bug worth reporting.
