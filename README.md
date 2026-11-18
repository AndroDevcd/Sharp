<div align="center">

<img src="https://github.com/AndroDevcd/Sharp/blob/master/doc/logo.png?raw=true" alt="Sharp Logo" width="210" height="142">

# The Sharp Programming Language

**Dependency injection, mocks, tests and GUI apps. Built in, not bolted on.**

[Getting Started](GETTING_STARTED.md) · [Examples](examples) · [Install](#install) · [Build from source](build-scripts/README.md) · [CLion plugin](tools/clion-plugin)

</div>

---

- **Dependency injection is a keyword.** `inject` what you need. No framework.
- **Mocks are written by the compiler.** `mock<mailer>()` and you have one.
- **Tests are just a program.** `std.testing` ships with the language.
- **A GUI app in 30 lines.** A declarative toolkit, in the standard library.
- **Coroutines and channels.** Thousands of tasks on a few threads.
- **Modern by default.** Null safety, generics, closures, type inference.

## Dependency injection is a keyword

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

## Mocks are written by the compiler

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

## Put them together

Swap anything a class injects, from inside a test.

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

## A GUI app in 30 lines

Change the state and the screen follows.

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

More: [to-do list](examples/gui/todo/src/main.sharp) · [task manager](examples/gui/tasks/src/main.sharp) · [all GUI examples](examples/gui)

## Coroutines

```javascript
mod app;

import ( std.io.coroutines )

def fetch_price(item: string) : int {
    delay(200);                             // a slow network call
    return item.size().value * 10;
}

def main() {
    run_blocking {
        apples := async<int>.start { fetch_price("apples") };
        pears := async<int>.start { fetch_price("pears") };

        // both ran at once: 200 ms, not 400
        println("total: ${apples.await().value + pears.await().value}");
    };
}
```

## Install

Download the newest `sharp-<version>.zip` from [`releases`](releases) and unpack it.

| | |
|---|---|
| **Windows** | double-click `windows\win-install.cmd` |
| **Linux (64-bit x86)** | `bash linux/unix-install.sh` |
| **Raspberry Pi, other Linux** | `git clone https://github.com/AndroDevcd/Sharp.git && cd Sharp && bash build-scripts/linux/install_from_source.sh` |

Open a new terminal, then:

```
sharpc hello.sharp -o hello
sharp hello
```

## Learn more

- [**Getting Started**](GETTING_STARTED.md) - first program, classes, generics, null safety, tests, a window
- [**Examples**](examples) - sample programs, [GUI apps](examples/gui), [tests and mocks](examples/testing)
- [**Language by example**](tests/suite/cases) - one small commented program per feature
- [**Building Sharp**](build-scripts/README.md) - build it, test it, make a release
- [**CLion / IntelliJ plugin**](tools/clion-plugin)
