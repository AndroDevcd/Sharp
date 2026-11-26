# The GUI guide

How Sharp's GUI library, `std.guikit`, works: windows, screens, layout, widgets, dialogs, view
models, several screens, time, and how to test all of it. It comes with Sharp; there is nothing to install on
Windows, and on Linux or a Raspberry Pi only SDL2 (`sudo apt install libsdl2-dev`).

Every piece of code on this page was compiled and run. The screens shown here are the ones the
tests at the end of the page click through.

## A window

```javascript
mod app;

import (
    std.guikit.*
)

class hello base composable {
    override def build() : element {
        return column(modifier().fill()) {
            text("Hello").size(32);
        }.center();
    }
}

def main() {
    gui.run("Hello", 360, 240, hello());
}
```

```
sharpc hello.sharp -o hello
sharp hello
```

`gui.run` opens the window, shows the screen and runs until the window is closed. A screen is a
class on top of `composable`; its `build()` says what is on it.

## A screen follows its state

```javascript
class counter base composable {
    count := state(0);

    override def build() : element {
        return column(modifier().fill()) {
            text("Count: " + count.value, modifier().tag("total")).size(32);
            row {
                button("-", modifier().tag("less")) { count.value -= 1; }.outline();
                button("+", modifier().tag("more")) { count.value += 1; };
            }.gap(8);
        }.center().gap(16);
    }
}
```

This is the whole idea of the library:

- A **state** is a value the screen depends on: `state(0)` for a number, `state("")` for text.
  Read and change it through `.value`.
- `build()` describes the screen for the state as it is **now**.
- When a state changes, `build()` runs again and the screen follows. A button never changes what
  is on the screen; it only changes a state.

Nothing is shown, hidden or updated by hand. If the words on the screen are wrong, the state is
wrong.

A state that several screens share is made outside them, with `gui.state(0)`.

The window does not redraw while nothing changes: an app that is standing still uses no processor.

## Laying things out

Three containers do most of the work. What is written inside a container's block is in it, in the
order it is written; ordinary `if` and `for` work there.

| Word | Its children |
|---|---|
| `column { }` | one under the other |
| `row { }` | side by side |
| `stack { }` | on top of each other |

```javascript
def profile_row(name: string, role: string) {
    row(modifier().fill_width().padding(12).tag("profile")) {
        column {
            text(name).bold();
            text(role).dim();
        }.gap(2);
        spacer();
        button("Message") { }.outline();
    }.gap(12);
}
```

- `.gap(8)` is the room between the children.
- `spacer()` takes all the room that is left: here it pushes the button to the right.
- `.center()` puts the children in the middle.

More containers: `scroll { }` for content taller than the window, `grid_fit(170) { }` for cards
that take as many columns as fit, `wrap { }` for things that flow onto the next line, `card { }`.

A long list is a `lazy_column`. It only builds the rows that are in sight, so ten thousand rows
cost the same as ten:

```javascript
def inbox(count: _int64) {
    lazy_column(modifier().fill().tag("inbox")) {
        items(count) { i: _int64 ->
            text("mail " + i, modifier().tag("mail_" + i).height(40));
        };
    };
}
```

## The modifier

Every widget takes a modifier, and the modifier is the one place a widget's box is changed. The
settings chain:

```javascript
def price_tag(words: string) {
    text(words, modifier().padding(10, 5).background(rgb(40, 90, 160)).corner(12).tag("price"));
}
```

| For | Words |
|---|---|
| Room | `padding(16)`, `padding(sides, top_bottom)`, `margin(8)` |
| Size | `width(200)`, `height(40)`, `fill()`, `fill_width()`, `grow(1)` (a share of the room left over) |
| Look | `background(colour)`, `corner(8)`, `border(1, colour)`, `shadow(6)`, `opacity(0.5)` |
| The mouse | `click { }`, `long_press { }`, `double_click { }`, `right_click { }`, `tooltip("words")` |
| There or not | `show(0)` takes no room, `visible(0)` keeps its room, `enable(0)` greys it out |
| Tests | `tag("save")`: a name a test finds the widget by |

What a widget **is** has its own words on the widget: `text(...).size(24).bold()`,
`button(...).outline()`, `text_field(...).label("Email")`.

## Widgets

| Word | What it is |
|---|---|
| `text("words")` | words: `.size(24)`, `.bold()`, `.dim()`, `.color(c)` |
| `button("Save") { }` | a button: `.outline()`, `.plain()`, `.danger()` |
| `text_field(state)` | a line to type into: `.label("Name")`, `.hint("...")`, `.password()`, `.on_submit { }` |
| `text_area(state)` | several lines |
| `checkbox("words", state)` | a tick box; the state is 1 or 0 |
| `toggle(state)` | a switch |
| `radio("words", state, 2)` | one of several; it is chosen when the state is its number |
| `slider(state, 0, 100)` | a number chosen by dragging |
| `dropdown(state) { option("A"); option("B"); }` | a list to pick from; the state is the option's number |
| `chip("words")`, `badge(3)` | small labels |
| `progress(0.4)` | how far along something is |
| `image("picture.png")`, `image("https://...")` | a picture, from a file or from a link |
| `divider()` | a thin line |

### Pictures

`image(...)` shows a picture file, or one from a link. A link is fetched in the background, so the
window never waits for it, and it is fetched once: the file is kept in a cache folder and read from
there afterwards, also the next time the program runs.

```
image("assets/logo.png");                                                       // a file, at its own size
image(link, modifier().size(150, 100)).fit(FIT_INSIDE);                         // all of it, as large as fits
image(link, modifier().size(150, 100).corner(14)).fit(FIT_CROP);                // the box filled, the rest cut off
image(link, modifier().size(100, 100).clip(shape.circle())).fit(FIT_CROP);      // a round portrait
image(link, modifier().size(100, 100).rotate(12).shadow(8)).fit(FIT_CROP);      // turned, with a shadow
image(link, modifier().size(100, 100)).on_error("assets/logo.png");             // shown when it cannot be had
```

- `.fit(...)` says how the picture sits in a box of another shape: `FIT_STRETCH` (how it starts),
  `FIT_INSIDE` or `FIT_CROP`.
- `.placeholder("file")` is shown while a link is being fetched, `.on_error("file")` when it cannot
  be fetched or read, and `.fade_in(0.2)` lets it come in over a fifth of a second.
- Shape, rotation, blur, border and shadow are the modifier's, as for every widget.
- `gui.image_cache("folder")` says where fetched pictures are kept. Unsaid, they go to a folder of
  their own in the system's place for caches. `.cache("folder")` on one picture keeps that one
  somewhere else than the rest.

(These lines were run in a window, not by this page's tests: a test without a window reads no
picture and fetches no link. There a picture counts as 100 x 100.)

A form puts them together. The widgets are bound to states, and the rest of the screen reads the
same states:

```javascript
class settings_form base composable {
    name := state("");
    updates := state(1);
    dark := state(0);
    plan := state(1);
    volume := state(0.5);

    override def build() : element {
        return column(modifier().fill().padding(24)) {
            text("Settings").size(24).bold();
            text_field(name, modifier().width(300).tag("name")).label("Your name");
            checkbox("Email me about updates", updates, modifier().tag("updates"));
            row {
                text("Dark mode");
                toggle(dark, modifier().tag("dark"));
            }.gap(12);
            row {
                radio("Monthly", plan, 1, modifier().tag("monthly"));
                radio("Yearly", plan, 2, modifier().tag("yearly"));
            }.gap(16);
            slider(volume, 0, 1, modifier().width(300).tag("volume"));
            // (the button cannot be used until there is a name)
            button("Save", modifier().tag("save").enable(name.value.size() > 0 ? 1 : 0)) {
                gui.toast("Saved for " + name.value);
            };
        }.gap(14);
    }
}
```

## Dialogs, menus, sheets and toasts

A dialog is not opened and closed. It is there while its state is not 0:

```javascript
class delete_button base composable {
    asking := state(0);
    deleted := state(0);

    override def build() : element {
        return column(modifier().fill().padding(24)) {
            text(deleted.value == 1 ? "The file is gone" : "report.txt", modifier().tag("file"));
            button("Delete", modifier().tag("delete")) { asking.value = 1; }.danger();

            dialog(asking) {
                text("Delete report.txt?").bold();
                row {
                    button("Keep it", modifier().tag("keep")) { asking.value = 0; }.outline();
                    button("Delete it", modifier().tag("really")) {
                        asking.value = 0;
                        deleted.value = 1;
                    }.danger();
                }.gap(8);
            };
        }.gap(12);
    }
}
```

- `dialog(state) { }` is a card in the middle of the window.
- `sheet(state) { }` rises from the bottom.
- `menu(state) { menu_item("Copy") { }; }` is a list of choices.

All three float above the page. While one is up, the page behind it cannot be clicked, a click
outside it closes it, and so does Escape.

`gui.toast("Saved")` shows a short message at the bottom of the window for a few seconds.

## View models

A screen that decides things soon becomes hard to read and hard to test. A **view model** is the
logic of a screen, kept apart from what the screen looks like.

```javascript
class basket_model base view_model {
    things := state_list<string>();
    problem := gui.state("");

    def add(name: string) {
        if(name.size() == 0) {
            problem.value = "Say what to add";
            return;
        }
        problem.value = "";
        things.add(name);
    }

    def take_out(index: _int64) {
        things.remove_at(index);
    }
}
```

The model holds the states and the functions that change them. It knows nothing about widgets: no
`text(...)`, no modifier, no colours. (In a model a state is made with `gui.state(...)`; the short
`state(...)` belongs to a screen.)

```javascript
class basket_screen base composable {
    model := model_of<basket_model>();
    typed := state("");

    override def build() : element {
        return column(modifier().fill().padding(24)) {
            text("Basket (" + model.things.size() + ")", modifier().tag("basket_title")).size(24).bold();
            row {
                text_field(typed, modifier().width(240).tag("thing")).hint("What to add");
                button("Add", modifier().tag("add_thing")) {
                    model.add(typed.value);
                    typed.value = "";
                };
            }.gap(8);
            text(model.problem.value, modifier().tag("basket_problem").show(model.problem.value.size() > 0 ? 1 : 0));
            for(i : _int64 = 0; i < model.things.size(); i++) {
                basket_line(model, i);
            }
        }.gap(10);
    }
}

/** one thing in the basket, and a button that takes it out */
def basket_line(model: basket_model, index: _int64) {
    row {
        text(model.things.at(index), modifier().tag("thing_" + index));
        button("Take out", modifier().tag("take_out_" + index)) { model.take_out(index); }.plain();
    }.gap(12);
}
```

The screen is dumb on purpose. It **reads** the model's states and **calls** the model's functions,
and decides nothing itself.

- `model_of<basket_model>()` gives the screen its model: made the first time it is asked for, and
  the same one every time after. A screen is built again and again; its model stays.
- Every part of a screen that asks for the same class gets the same model, so parts of a screen
  share one.
- The model belongs to the screen. When the screen is closed (gone back from), the model is
  dropped; `when_cleared { }` in the model runs then.
- `shared_model_of<...>()` is one model for the whole app, kept until the window closes.
- `state_list<string>()` is a list the screen follows: adding, removing or moving an item builds
  the screen again.
- A model class is made with no arguments. What it needs from outside, the screen hands it once
  (`model.open(it.number)`).

A model must not make the screen wait. Slow work - a file, a server - is started with the model's
`launch`, and the answer comes back with `on_main`, the one place where states may be changed:

```javascript
component app {
    factory { replaceable<coroutine_dispatcher>() { return dispatchers.io(); } }
}

/** what a file or a server would give */
def read_the_notes() : list<string> {
    found := new list<string>();
    found.add("call the bank");
    found.add("water the plants");
    return found;
}

class notes_model base view_model {
    inject background : coroutine_dispatcher;
    notes := state_list<string>();
    busy := gui.state(0);

    def load() {
        busy.value = 1;
        launch(background) {
            found := read_the_notes();          // on another thread: the window stays alive
            on_main {
                notes.replace_all(found);       // back on the main thread: the screen follows
                busy.value = 0;
            };
        };
    }
}
```

The model does not pick its thread itself. It asks for a dispatcher (`inject`), and the app says
which one it gets: here the threads for files and servers, `dispatchers.io()`. Which thread work
runs on matters - one thread may own a database, order may matter - so it is written where it can
be read, and a test can give the model another one (below). These need
`import ( std.di, std.io.coroutines )`.

Work started with the model's `launch` is cancelled when the model is dropped, so an answer that
arrives late cannot write to a screen that is gone.

Because a model is plain logic, it can be tested with no screen at all, and the screen with a few
clicks. Both are in the tests at the end of this page.

## Several screens

An app with more than one screen has a `navigator`, and one small class for each place it can go:
a **route**. A route's fields are what its screen needs.

```javascript
class to_list base route { }

class to_item base route {
    number : _int64;
    to_item(n: _int64) { number = n; }
}

class shop base composable {
    nav : navigator = navigator();

    override def build() : element {
        return column(modifier().fill()) {
            screens(nav, to_list(), modifier().grow(1).fill_width()) {
                screen<to_list>() {
                    column(modifier().fill().padding(16)) {
                        text("The shop").size(24).bold();
                        for(i : _int64 = 1; i <= 3; i++) {
                            item_line(nav, i);
                        }
                    }.gap(8);
                };
                screen<to_item>() {
                    column(modifier().fill()) {
                        top_bar("Item " + it.number, nav);
                        text("This is item " + it.number, modifier().padding(16).tag("item_words"));
                    };
                };
            };
        };
    }
}

/** one line of the list: clicking it opens that item */
def item_line(nav: navigator, n: _int64) {
    text("Item " + n, modifier().fill_width().padding(12).tag("item_" + n).click { nav.go(to_item(n)); });
}
```

- `screens(nav, first_route) { }` lists every screen of the app in one place.
- `nav.go(to_item(7))` goes there. A screen never needs to know another screen, only a route.
- `nav.back()` goes back, and so does Escape. `top_bar(title, nav)` is a title with a back arrow.
- Inside a `screen<...>() { }` block, `it` is the route that was gone to.

For tabs along the bottom, write `tab<to_list>("List") { }` in place of `screen<...>` and put
`nav_bar(nav)` under the screens. Each tab keeps its own way back.

## Time

```javascript
class stopwatch base composable {
    seconds := state(0);

    stopwatch() {
        every(1) { seconds.value += 1; };
    }

    override def build() : element {
        return text("" + seconds.value + " s", modifier().tag("seconds")).size(32);
    }
}
```

- `every(1) { }` in a screen's constructor runs each second while that screen is up.
- `gui.after(2) { }` runs once, two seconds from now; `gui.every(1) { }` runs until the timer it
  gives back is cancelled.
- `animated(0)` is a number that glides: `width.go(240, 0.25)` takes a quarter of a second to get
  there, and the screen follows on the way.

## The look

```javascript
def theme_buttons() {
    row {
        button("Light", modifier().tag("light")) { gui.use_theme(theme.light()); }.outline();
        button("Dark", modifier().tag("dark_theme")) { gui.use_theme(theme.dark()); }.outline();
    }.gap(8);
}
```

`gui.use_theme(...)` changes the whole app on the spot. For a widget of your own,
`gui.current_theme()` has the colours in use: `t.surface`, `t.primary`, `t.text`.

`gui.app_name("My app")` and `gui.app_icon("icon.png")` set what the title bar and the task bar
show (an `.ico`, an `.svg` or a picture file).

## Charts and 3D

Both are part of the same library and go on a screen like any widget:

- `chart { bars(...); line(...); }` draws charts: see [std.guikit.charts](std/std.guikit.charts.html).
- `scene { }` is a 3D scene with a camera, lights, shapes, land and water: see
  [Your first 3D scene](guide/your-first-3d-scene.html) and [std.guikit.three](std/std.guikit.three.html).

## Testing a screen

`std.guikit.testing` runs a screen without opening a window, uses it the way a person does, and
checks what happens. These are the tests of the screens on this page:

```javascript
mod main;

import (
    std,
    std.di,
    std.io.coroutines,
    std.testing,
    std.guikit.*,
    std.guikit.testing
)

def main(args: string[]) {
    tests("the screens of the GUI guide") {
        ui_test("the counter counts") {
            set_content(counter());
            on_tag("total").should_say("Count: 0");
            on_tag("more").click().click();
            on_tag("less").click();
            on_tag("total").should_say("Count: 1");
        };

        ui_test("widgets written right in the test") {
            set_content {
                column {
                    profile_row("Ann", "Designer");
                    price_tag("12.50");
                    theme_buttons();
                };
            };
            on_tag("profile").on_text("Ann").should_be_shown();
            on_tag("profile").on_kind<button>().should_say("Message");
            on_tag("price").should_say("12.50");
            all_kinds<button>().should_have_count(3);
        };

        ui_test("a row far down a long list") {
            set_content { inbox(5000); };
            on_tag("mail_4000").should_not_exist();
            on_tag("inbox").scroll_to(on_tag("mail_4000"));
            on_tag("mail_4000").should_be_shown();
        };

        ui_test("the form: typing, ticking, choosing, saving") {
            set_content(settings_form());
            on_tag("save").should_be_disabled();
            on_tag("name").type("Ann");
            on_tag("save").should_be_enabled();

            on_tag("updates").should_be_on().click().should_be_off();
            on_tag("dark").click().should_be_on();
            on_tag("yearly").click().should_be_selected();
            on_tag("monthly").should_be_off();
            on_tag("volume").should_have_value(0.5);

            on_tag("save").click();
            should_show_toast("Saved for Ann");
            advance(20);
            should_show_no_toast();
        };

        ui_test("deleting asks first") {
            set_content(delete_button());
            on_tag("really").should_not_exist();
            on_tag("delete").click();
            on_text("Delete report.txt?").should_be_shown();
            on_tag("keep").click();
            on_tag("file").should_say("report.txt");

            on_tag("delete").click();
            on_tag("really").click();
            on_tag("file").should_say("The file is gone");
        };

        ui_test("from the list to an item") {
            set_content(shop());
            on_tag("item_words").should_not_exist();
            on_tag("item_2").click();
            on_tag("item_words").should_say("This is item 2");
            back();
            advance(1);                                     // the slide back takes a moment
            on_tag("item_words").should_not_exist();
        };

        test("the basket's model, with no screen at all") {
            basket := new basket_model();
            basket.add("");
            expect(basket.problem.value).should_be("Say what to add");
            expect(basket.things.size()).should_be(0);

            basket.add("milk");
            basket.add("bread");
            expect(basket.problem.value).should_be("");
            expect(basket.things.size()).should_be(2);
            basket.take_out(0);
            expect(basket.things.at(0)).should_be("bread");
        };

        ui_test("the basket's screen follows its model") {
            set_content(basket_screen());
            on_tag("add_thing").click();
            on_tag("basket_problem").should_say("Say what to add");

            on_tag("thing").type("milk");
            on_tag("add_thing").click();
            on_tag("thing").type("bread");
            on_tag("add_thing").click();
            on_tag("basket_title").should_say("Basket (2)");
            on_tag("basket_problem").should_not_exist();
            on_tag("thing").should_say("");

            on_tag("take_out_0").click();
            on_tag("basket_title").should_say("Basket (1)");
            on_tag("thing_0").should_say("bread");
        };

        test("the notes' model: its background work, run when the test says") {
            work := test_dispatcher();
            provide<coroutine_dispatcher>() { return work; };

            model := new notes_model();
            seen := watching(model.busy);
            model.load();
            expect(model.notes.size()).should_be(0);        // the work is held: nothing has run yet

            run_work();
            expect(model.notes.size()).should_be(2);
            seen.should_have_been(0, 1, 0);                 // not busy, busy, not busy again
        };

        ui_test("the stopwatch, without waiting for it") {
            set_content(stopwatch());
            on_tag("seconds").should_say("0 s");
            advance(3.5);
            on_tag("seconds").should_say("3 s");
        };
    };
    finish_tests();
}
```

```
sharpc guide_tests.sharp -o guide_tests
sharp guide_tests
```

### How it works

- `ui_test("...") { }` is one test, inside a `tests(...)` group like any other test.
- `set_content(counter())` puts a screen up. `set_content { ... }` takes widgets written right there.
- No window opens. The screen is built, laid out, clicked and typed into through the same code a
  real window uses, so a click on a disabled button does nothing and a click on something a dialog
  covers fails the test.
- A check that does not hold fails the test there and prints the whole screen as text: every
  widget, its tag, its words, where it is.

### Finding

| Word | Finds |
|---|---|
| `on_tag("save")` | the widget tagged `modifier().tag("save")` |
| `on_text("Save")` | the widget that shows these words |
| `on_kind<button>()` | the widget of this class |
| `all_tags(...)`, `all_text(...)`, `all_kinds<...>()` | every such widget: `.first()`, `.last()`, `.at(2)`, `.count()` |
| `on_tag("row_3").on_text("Delete")` | one inside another |
| `all_kinds<checkbox>().that_are_on()` | of those, the ones that are on, off, enabled, disabled or shown |
| `all_kinds<button>().with_text("Save")` | of those, the ones with these words, or `.with_tag(...)` |

`on_tag`, `on_text` and `on_kind` mean exactly one widget. When there is none, or more than one,
the test fails and says so; it never picks one for you.

Find by tag where you can. The words on a screen change with the wording and the language; a tag
stays.

### Doing

| Word | Does |
|---|---|
| `click()`, `double_click()`, `long_click()`, `right_click()` | the mouse, in the middle of the widget |
| `type("words")`, `replace("words")`, `clear()` | typing into a field |
| `press(KEY_ENTER)`, `press('s', MOD_CTRL)` | a key, alone or with Ctrl, Shift or Alt held |
| `hover()` | the mouse rests on it |
| `scroll_to()` | brings the widget into sight |
| `on_tag("list").scroll_to(on_tag("row_40"))` | scrolls a list until that row is there |
| `scroll_to_end()`, `scroll_to_start()`, `scroll_down(3)`, `scroll_up(3)` | on a scrolling area |
| `swipe_left()`, `swipe_up()`, `drag(dx, dy)` | dragging |
| `back()` | the back key |

### Checking

| Word | Holds when |
|---|---|
| `should_exist()`, `should_not_exist()` | it is on the screen, or not |
| `should_be_shown()`, `should_be_hidden()` | it can be seen, or not (hidden, scrolled away) |
| `should_be_enabled()`, `should_be_disabled()` | it can be used, or is greyed out |
| `should_be_on()`, `should_be_off()`, `should_be_selected()` | a checkbox, a switch, a radio |
| `should_be_focused()` | it has the keyboard |
| `should_say("words")`, `should_contain("part")` | its words |
| `should_have_value(0.5)` | a slider or a progress bar |
| `should_have_count(3)` | on a group |
| `should_show_toast("Saved")`, `should_show_no_toast()` | a toast |
| `should_show_tooltip("words")` | a tooltip, after `hover()` and `advance(1)` |

They chain: `on_tag("agree").click().should_be_on();`

### Time

The clock of a test stands still. A timer does not ring and a toast does not go away until the
test lets time pass, so a test of something that takes a minute takes no time and gives the same
answer on every machine.

| Word | Does |
|---|---|
| `advance(2)` | two seconds pass |
| `wait_until { return on_text("Done").exists(); }` | time passes until the block answers true |
| `idle()` | no time passes; everything waiting to be built again finishes |

### Watching a test

```
sharp guide_tests -watch
```

The same tests run in a real window, slowly enough to follow: the screen comes up, a ring shows
where the test presses, the words appear as it types. Nothing in the tests changes for it.

`hold_on_failure()` at the top of `main` keeps the window of a failed test up, as it was when the
check did not hold, until you close it.

### What something looks like

A test that is not watched paints nothing, so it cannot see a colour. The words that look at
colours work in a watched test only:

```javascript
def check_the_price_tag() {
    on_tag("price").should_have_color_at(4, 4, rgb(40, 90, 160));
}
```

Without `-watch` they stop the test with an error that says so. They never pass quietly.

### A model's background work

A model that starts work on another thread is tested in an ordinary `test(...)` or a `ui_test(...)`;
there is no third kind of test. Three words do it:

| | |
|---|---|
| `test_dispatcher()` | a dispatcher that holds the work it is given; hand it to the model with `provide<coroutine_dispatcher>() { return work; };` |
| `run_work()` | runs that work, in the order it was started, and what `on_main` handed back, until nothing is left |
| `watching(state)` | writes down every value a state takes: `seen.should_have_been(0, 1, 0)`, `seen.last()`, `seen.size()` |

Until `run_work()` nothing the model started has run, so a test can also check what the screen
shows in between ("Loading"). In a `ui_test`, `run_work()` lets the screen follow what changed.

### The rest of testing

The UI words sit on top of `std.testing`, and everything there works in a UI test too:

- Groups can hold groups: `tests("a") { tests("b") { test(...) }; };`. `before_each { }` and
  `after_each { }` of every group around a test run for it.
- `skip_test("why")` leaves a test, counted as skipped.
- `mock<store>()` makes a stand-in for an interface; `given { }`, `verify { }` and
  `verify_order { }.then { }` set it up and check how it was used.

See [std.guikit.testing](std/std.guikit.testing.html) and [std.testing](std/std.testing.html) for
every word.
