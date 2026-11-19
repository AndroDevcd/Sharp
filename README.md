<div align="center">

<img src="https://github.com/AndroDevcd/Sharp/blob/master/doc/logo.png?raw=true" alt="Sharp Logo" width="210" height="142">

# The Sharp Programming Language

**Dependency injection, mocks, tests and GUI apps, all built in.**

[Install](#install) · [Getting Started](GETTING_STARTED.md) · [Examples](examples) · [Documentation](#documentation) · [Building from Source](#building-from-source)

</div>

---

Sharp ships with the tools most projects have to piece together from third-party libraries.
Programs compile with `sharpc` and run on the Sharp VM, on Windows and Linux.

This repository contains the compiler, the VM, the standard library, and the documentation.

## Why Sharp?

- **Dependency injection built-in:** Write `inject`, bind it in a `component`, and you're done. No framework, no container.
- **Mocks without the boilerplate:** `mock<mailer>()` is all it takes. The compiler writes the class.
- **Testing out of the box:** `std.testing` comes with the language, so there's nothing to install or configure.
- **GUI apps in about 30 lines:** `std.guikit` is ready when you are. Change the state and the screen follows.
- **Concurrency that scales:** Coroutines and channels run thousands of tasks on just a few threads.
- **Modern by default:** Null safety, generics, closures and type inference from line one.

## Install

Download the latest `sharp-<version>.zip` from the [`releases`](releases) folder and unpack the whole archive, then run the installer for your system:

| System | Command |
|---|---|
| **Windows** | Double-click `windows\win-install.cmd` |
| **Linux (64-bit x86)** | `bash linux/unix-install.sh` |

On a Raspberry Pi or any other Linux system, build the VM on the machine itself:

```
git clone https://github.com/AndroDevcd/Sharp.git
cd Sharp
bash build-scripts/linux/install_from_source.sh
```

The installer doesn't need administrator rights. Once it finishes, open a new terminal so that `sharp` and `sharpc` are on your `PATH`.

## Hello, world

Save this as `hello.sharp`:

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

## A Tour of Sharp

### Dependency injection built-in

Classes ask for what they need. One `component` decides how it's made.

```javascript
mod app;

interface greeter {
    def greet(name: string) : string;
}

class english : greeter {
    override def greet(name: string) : string { return "Hello, $name!"; }
}

class welcome_desk {
    inject words : greeter;                 // asked for, never built here

    def welcome(name: string) { println(words.greet(name)); }
}

component app {
    single { english() as greeter }         // the one place that says how it is made
}

def main() {
    welcome_desk().welcome("ada");
}
```

### Mocks written for you

Ask for a mock of any interface, then verify exactly how your code used it.

```javascript
mod app;

import ( std.testing )

interface mailer {
    def send(to: string, message: string);
}

def welcome(mail: mailer, name: string) {
    mail.send(name, "Welcome, $name!");
}

def main() {
    tests("signing up") {
        test("a new member gets exactly one welcome") {
            mail := mock<mailer>();             // the compiler writes this class

            welcome(mail, "ada");

            verify { mail.send("ada", any_text()); }.once();
        };
    };
    finish_tests();
}
```

```
signing up
  ok    a new member gets exactly one welcome

all 1 tests passed
```

### Swap any dependency from a test

Replace anything a class injects, without touching the class.

```javascript
mod app;

import ( std.testing, std.di )

interface clock {
    def now() : string;
}

class system_clock : clock {
    override def now() : string { return "the real time"; }
}

class report {
    inject time : clock;
    def title() : string { return "Sales at ${time.now()}"; }
}

component app {
    factory { replaceable<clock>() { return system_clock(); } }
}

def main() {
    tests("a report") {
        test("the title shows the time") {
            fake := mock<clock>();
            given { fake.now(); }.returns("09:30");
            provide<clock>() { return fake; };      // every `inject clock` now gets the mock

            expect(report().title()).should_be("Sales at 09:30");
        };
    };
    finish_tests();
}
```

### A GUI app in 30 lines

Describe the screen once. Change the state and it updates itself.

```javascript
mod app;

import (
    std.guikit.*
)

class counter base composable {
    count := state(0);

    override def build() : element {
        return column(
            modifier()
                .fill()
        ) {
            text(
                "count: " + count.value
            ).size(32);
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

See also the [to-do list](examples/gui/todo/src/main.sharp), the [task manager](examples/gui/tasks/src/main.sharp) and the [other GUI examples](examples/gui).

### Concurrency without the ceremony

Run work at the same time, give up on anything slow, and stream values between coroutines.

```javascript
mod app;

import ( std.io.coroutines, std.io.coroutines.channels )

def fetch_price(item: string) : int {
    delay(200);                             // a slow network call
    return item.size().value * 10;
}

def main() {
    run_blocking {
        // run side by side: 200 ms, not 400
        apples := async<int>.start { fetch_price("apples") };
        pears := async<int>.start { fetch_price("pears") };
        println("total: ${apples.await().value + pears.await().value}");

        // too slow? it gets cancelled
        kiwis := with_timeout_or_null(100) { -> (object) return fetch_price("kiwis"); };
        println(kiwis == null ? "kiwis: timed out" : "kiwis: found");

        // stream values through a channel
        numbers := produce<int>.start { out: send_channel<int> ->
            out.send(1);
            out.send(2);
        };
        numbers.for_each { n: int -> println("received $n"); };
    };
}
```

```
total: 110
kiwis: timed out
received 1
received 2
```

## Documentation

| Resource | What you'll find |
|---|---|
| [Getting Started](GETTING_STARTED.md) | Your first program, classes, generics, null safety, tests and a window |
| [Examples](examples) | Sample programs, [GUI apps](examples/gui), and [tests and mocks](examples/testing) |
| [Language by example](tests/suite/cases) | Small, commented programs that each cover one feature |
| [Building Sharp](build-scripts/README.md) | How to build, test and release Sharp |
| [CLion / IntelliJ plugin](tools/clion-plugin) | Editor support for Sharp |

## Building from Source

Sharp builds on Linux or WSL. Install the prerequisites, then run the build and test scripts:

```
sudo apt install g++ rsync python3 libsdl2-dev
bash build-scripts/build.sh
bash build-scripts/test.sh
```

The compiler is written in Sharp and is built by its own last good build. See [Building Sharp](build-scripts/README.md) for the full guide, including how to make a release.

## Repository Layout

| Component | Language | Location |
|---|---|---|
| Virtual machine | C++ | [`lib/runtime`](lib/runtime), [`lib/core`](lib/core), [`main.cpp`](main.cpp) |
| Compiler | Sharp | [`lib/compiler`](lib/compiler) |
| Standard library | Sharp | [`lib/support/0.4.0`](lib/support/0.4.0) |
| GUI native layer | C++ | [`lib/support/0.4.0/std/guikit/platform/native`](lib/support/0.4.0/std/guikit/platform/native) |
