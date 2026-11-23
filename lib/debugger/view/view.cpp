//
// See view.h.
//
// HOW THE SCREEN IS DRAWN. Every drawing makes the whole screen anew as rows of
// text, each exactly as wide as the terminal (compose), and then writes only
// the rows that differ from what is on the terminal already (draw). So nothing
// flickers, and no part of the code has to know what another part left there.
//
// WHO DRAWS. The prompt's thread, after every key and every command; and the
// thread that reads what the program prints, when something arrives. One lock
// (screen_mut) is held for a whole drawing and for every change to what is
// drawn.
//
// WHAT THE RIGHT-HAND BOXES SHOW is worked out by the prompt's thread while the
// program is held (refresh_boxes) and kept as plain rows of text: the drawing
// itself never reads the program's memory, so the output thread may draw while
// the program runs.
//

#include "view.h"

#ifdef _WIN32

// (the full view is not built for Windows yet: sdb uses the plain view there)
bool view_can_be_full(string &why) { why = "the full view is not there on Windows yet"; return false; }
void view_begin(debug_session *session) {}
bool view_is_full() { return false; }
void view_say(const string &line) {}
bool view_read_command(string &line) { return false; }
void view_program_stopped(const string &why) {}
void view_refresh() {}
void view_program_runs() {}
void view_end(const string &lastWords) {}

#else

#include "../commands/commands.h"
#include "../values/values.h"
#include "../../runtime/multitasking/fiber/fiber.h"
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <csignal>
#include <deque>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

#define NARROWEST 100           // the fewest columns the boxes fit in
#define LOWEST 20               // ... and the fewest lines
#define LINES_KEPT 5000         // how many lines of each left-hand box are kept to scroll back to
#define SOURCE_LINES_KEPT 200   // how many source lines around the current one are kept for the source box

static bool full = false;
static int terminal = -1;                           // the terminal itself: a copy kept when the output streams became a pipe
static termios as_found;                            // the terminal's settings before sdb changed them
static std::recursive_mutex &screen_mut = *new std::recursive_mutex();
static debug_session *session = nullptr;

// ---- what is drawn
static std::deque<string> output_lines;             // what the program printed, a line each
static string output_unfinished;                    // ... and its last line, when no line end has come yet
static std::deque<string> talk_lines;               // what was typed to sdb and what it answered
static int scrolled_back = 0;                       // how many lines the sdb box is scrolled back (PageUp)
static string typed;                                // the command being typed
static std::vector<string> given;                   // the commands given before (Up / Down)
static size_t given_at = 0;
static string last_line;                            // the screen's last line: why the program stopped
static bool running = false;
static bool over = false;                           // the program has ended: Enter leaves sdb

static string source_title;
static std::vector<string> source_rows;             // the source around the current line ...
static int source_current = -1;                     // ... and which of them the current line is (-1: none)
static std::vector<string> local_rows;
static std::vector<string> call_rows;

static std::vector<string> on_screen;               // the rows the terminal shows now
static std::atomic<bool> resized(false);
static bool colours = true;

bool view_is_full() {
    return full;
}

// ---------------------------------------------------------------------------------
// The terminal
// ---------------------------------------------------------------------------------

static void to_terminal(const string &text) {
    size_t done = 0;
    while(done < text.size()) {
        ssize_t wrote = write(terminal, text.data() + done, text.size() - done);
        if(wrote <= 0) return;
        done += (size_t)wrote;
    }
}

static void terminal_size(int &columns, int &lines) {
    winsize size;
    columns = 0;
    lines = 0;
    if(ioctl(terminal >= 0 ? terminal : STDOUT_FILENO, TIOCGWINSZ, &size) == 0) {
        columns = size.ws_col;
        lines = size.ws_row;
    }
}

/** keys come one at a time and are not shown by the terminal: sdb reads and shows them itself */
static void keys_to_sdb() {
    termios raw = as_found;
    raw.c_lflag &= ~(ICANON | ECHO);        // (ISIG stays: Ctrl+C is still the pause)
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

/** the keyboard as the program expects it: a line at a time, shown as typed */
static void keys_to_the_program() {
    tcsetattr(STDIN_FILENO, TCSANOW, &as_found);
}

static void size_changed(int) {
    resized = true;
}

/** (also run when the process ends by any road) */
static void give_the_terminal_back() {
    if(!full) return;
    full = false;
    keys_to_the_program();
    to_terminal("\x1b[0m\x1b[?25h\x1b[?1049l");     // plain text, the cursor shown, the first screen again
}

bool view_can_be_full(string &why) {
    if(!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        why = "sdb is not on a terminal";
        return false;
    }
    const char *kind = getenv("TERM");
    if(kind == nullptr || string(kind) == "dumb") {
        why = "this terminal cannot be drawn on";
        return false;
    }

    int columns, lines;
    terminal_size(columns, lines);
    if(columns < NARROWEST || lines < LOWEST) {
        stringstream words;
        words << "the terminal is " << columns << " by " << lines << ", and the boxes need " << NARROWEST << " by " << LOWEST;
        why = words.str();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------
// Text that fits
// ---------------------------------------------------------------------------------

/**
 * `text` as exactly `width` places on the screen: what does not print is left
 * out (a tab is four blanks; an escape sequence the program sent - colours -
 * is dropped whole), what is too long is cut, what is too short gets blanks.
 * (A character counts as one place: wide ones, as in Chinese, come out wrong.)
 */
static string fitted(const string &text, int width) {
    string made;
    int places = 0;

    for(size_t i = 0; i < text.size() && places < width; i++) {
        unsigned char c = (unsigned char)text[i];
        if(c == 0x1b) {
            // an escape sequence: ESC [ ... a letter
            if(i + 1 < text.size() && text[i + 1] == '[') {
                i += 2;
                while(i < text.size() && !((text[i] >= '@' && text[i] <= '~'))) i++;
            }
            continue;
        }
        if(c == '\t') {
            for(int blank = 0; blank < 4 && places < width; blank++) { made += ' '; places++; }
            continue;
        }
        if(c < 32 || c == 127) continue;

        made += (char)c;
        if((c & 0xC0) != 0x80) places++;            // (the first byte of a character; the others follow it)
        // the rest of this character's bytes belong to the same place
        while(i + 1 < text.size() && ((unsigned char)text[i + 1] & 0xC0) == 0x80) made += text[++i];
    }
    if(places < width) made += string(width - places, ' ');
    return made;
}

/** how many places a text takes */
static int places_of(const string &text) {
    int places = 0;
    for(unsigned char c : text) {
        if((c & 0xC0) != 0x80) places++;
    }
    return places;
}

/** `─` so many times */
static string dashes(int count) {
    string made;
    for(int i = 0; i < count; i++) made += "─";
    return made;
}

/** the top or a dividing line of a box, with a title in it: `┌ output ─────┐` */
static string edge(const string &left, const string &title, const string &right, int width) {
    string shown = title.empty() ? "" : " " + title + " ";
    if(places_of(shown) > width - 2) shown = fitted(shown, width - 2);
    return left + shown + dashes(width - 2 - places_of(shown)) + right;
}

/** one line inside a box: `│ text      │` */
static string inside(const string &text, int width, bool strong = false) {
    string cell = fitted(text, width - 3);
    if(strong && colours) cell = "\x1b[1m" + cell + "\x1b[0m";
    return "│ " + cell + "│";
}

// ---------------------------------------------------------------------------------
// The screen
// ---------------------------------------------------------------------------------

/** a line cut into pieces of at most `width` places, so that all of it can be read in a box */
static void wrap_into(std::deque<string> &rows, const string &line, int width) {
    if(places_of(line) <= width) {
        rows.push_back(line);
        return;
    }
    string piece;
    int places = 0;
    for(size_t i = 0; i < line.size(); i++) {
        unsigned char c = (unsigned char)line[i];
        if((c & 0xC0) != 0x80 && places == width) {
            rows.push_back(piece);
            piece = "    ";                             // (what follows belongs to the line above)
            places = 4;
        }
        piece += (char)c;
        if((c & 0xC0) != 0x80) places++;
    }
    rows.push_back(piece);
}

/** the last lines of a box as rows that fit it: long lines take several (only as many lines as can be in view are cut up) */
static std::deque<string> rows_of(const std::deque<string> &lines, int width, int most) {
    std::deque<string> rows;
    size_t from = lines.size() > (size_t)most ? lines.size() - most : 0;
    for(size_t i = from; i < lines.size(); i++) wrap_into(rows, lines[i], width);
    return rows;
}

/** the last `count` lines of a box, with `lines` at their end shown `back` lines further up */
static std::vector<string> window_of(const std::deque<string> &lines, int count, int back) {
    std::vector<string> shown;
    int last = std::max(0, (int)lines.size() - back);
    int first = std::max(0, last - count);
    if(last - first < count) last = std::min((int)lines.size(), first + count);     // (scrolled back to the top)
    for(int i = first; i < last; i++) shown.push_back(lines[i]);
    while((int)shown.size() < count) shown.push_back("");      // fewer lines than room: they stand at the top
    return shown;
}

/** every row of the screen, each `columns` places wide (screen_mut held) */
static std::vector<string> compose(int columns, int lines) {
    std::vector<string> rows;
    int left = columns * 45 / 100;
    int right = columns - left;
    int body = lines - 1;                           // (the last line is the one below the boxes)

    // ---- the left side: the program's output above, sdb below
    int output_inside = std::max(3, (body - 3) * 40 / 100);
    int talk_inside = body - 3 - output_inside;

    std::deque<string> printed = output_lines;
    if(!output_unfinished.empty()) printed.push_back(output_unfinished);

    std::deque<string> said = talk_lines;
    if(over) said.push_back("(press Enter to leave sdb)");
    else if(running) said.push_back("(the program is running: Ctrl+C stops it; what you type goes to it)");
    else said.push_back("(sdb) " + typed + (scrolled_back == 0 ? "█" : ""));

    // (a line longer than its box goes on in the next row)
    printed = rows_of(printed, left - 3, output_inside);
    said = rows_of(said, left - 3, talk_inside + scrolled_back);

    std::vector<string> leftRows;
    leftRows.push_back(edge("┌", "output", "┐", left));
    for(const string &line : window_of(printed, output_inside, 0)) leftRows.push_back(inside(line, left));
    leftRows.push_back(edge("├", scrolled_back > 0 ? "sdb (scrolled back: PageDown comes forward)" : "sdb", "┤", left));
    for(const string &line : window_of(said, talk_inside, scrolled_back)) leftRows.push_back(inside(line, left));
    leftRows.push_back(edge("└", "", "┘", left));

    // ---- the right side: the source, the variables, the calls
    int available = body - 4;
    int source_inside = available * 50 / 100;
    int locals_inside = available * 25 / 100;
    int calls_inside = available - source_inside - locals_inside;

    std::vector<string> rightRows;
    rightRows.push_back(edge("┌", source_title.empty() ? "source" : source_title, "┐", right));
    // the current line in the middle of its box, when there are more lines than room
    int first = 0;
    if((int)source_rows.size() > source_inside && source_current >= 0) {
        first = source_current - source_inside / 2;
        first = std::max(0, std::min(first, (int)source_rows.size() - source_inside));
    }
    for(int i = 0; i < source_inside; i++) {
        int at = first + i;
        rightRows.push_back(inside(at < (int)source_rows.size() ? source_rows[at] : "", right, at == source_current));
    }
    rightRows.push_back(edge("├", "locals", "┤", right));
    for(int i = 0; i < locals_inside; i++) rightRows.push_back(inside(i < (int)local_rows.size() ? local_rows[i] : "", right));
    rightRows.push_back(edge("├", "calls", "┤", right));
    for(int i = 0; i < calls_inside; i++) rightRows.push_back(inside(i < (int)call_rows.size() ? call_rows[i] : "", right));
    rightRows.push_back(edge("└", "", "┘", right));

    for(int i = 0; i < body; i++) rows.push_back(leftRows[i] + rightRows[i]);
    rows.push_back(fitted(" " + (running ? string("running") : last_line), columns));
    return rows;
}

/** bring the terminal in line with what is to be shown (takes screen_mut) */
static void draw() {
    if(!full) return;
    std::lock_guard<std::recursive_mutex> lock(screen_mut);

    int columns, lines;
    terminal_size(columns, lines);
    if(resized.exchange(false)) on_screen.clear();

    if(columns < NARROWEST || lines < LOWEST) {
        on_screen.clear();
        to_terminal("\x1b[2J\x1b[1;1Hthe terminal is too small for sdb's boxes: make it larger");
        return;
    }

    std::vector<string> rows = compose(columns, lines);
    if(on_screen.size() != rows.size()) {
        on_screen.assign(rows.size(), "");
        to_terminal("\x1b[2J");
    }

    string changes = "\x1b[?25l";                   // (the cursor is hidden: the prompt draws its own)
    for(size_t i = 0; i < rows.size(); i++) {
        if(rows[i] == on_screen[i]) continue;
        stringstream place;
        place << "\x1b[" << i + 1 << ";1H";
        changes += place.str() + rows[i];
        on_screen[i] = rows[i];
    }
    // while the program runs the terminal shows what is typed itself: on the last line, after the word there
    stringstream rest;
    rest << "\x1b[" << lines << ";" << 10 << "H";
    if(running) changes += rest.str() + "\x1b[?25h";
    to_terminal(changes);
}

static void keep(std::deque<string> &lines, const string &line) {
    lines.push_back(line);
    if(lines.size() > LINES_KEPT) lines.pop_front();
}

// ---------------------------------------------------------------------------------
// What the program prints
// ---------------------------------------------------------------------------------

/** the thread that reads the pipe the program's output goes into, for as long as the process lives */
static void read_the_output(int from) {
    // SDB_OUTPUT_COPY=<file>: everything the program printed, written to a file as well - the boxes
    // go when sdb ends, and with them what a program that crashed said last
    FILE *copy = getenv("SDB_OUTPUT_COPY") != nullptr ? fopen(getenv("SDB_OUTPUT_COPY"), "w") : nullptr;

    char bytes[8192];
    for(;;) {
        ssize_t count = read(from, bytes, sizeof(bytes));
        if(count <= 0) return;
        if(copy != nullptr) {
            fwrite(bytes, 1, (size_t)count, copy);
            fflush(copy);
        }
        {
            std::lock_guard<std::recursive_mutex> lock(screen_mut);
            for(ssize_t i = 0; i < count; i++) {
                if(bytes[i] == '\n') {
                    keep(output_lines, output_unfinished);
                    output_unfinished.clear();
                } else if(bytes[i] != '\r') {
                    output_unfinished += bytes[i];
                }
            }
        }
        draw();
    }
}

// ---------------------------------------------------------------------------------
// The right-hand boxes, from the held program
// ---------------------------------------------------------------------------------

/** work out what the three boxes show (the prompt's thread, while the program is held) */
static void refresh_boxes() {
    std::string title;
    std::vector<string> source, locals, calls;
    int current = -1;

    frame_view frame;
    if(session != nullptr && session->look != nullptr
       && (frame_at(session->look, session->frame, frame) || frame_at(session->look, 0, frame))) {
        sharp_function *function = frame.place.function;
        Int file = function->sourceFile;
        Int line = line_of(function, frame.place.pc);
        title = shown_file(file) + " ─ " + name_of(function);

        // the lines of the file that hold a breakpoint
        std::vector<Int> marked;
        for(breakpoint &point : sdb_breakpoints()) {
            for(code_place &place : point.places) {
                if(point.on && place.function->sourceFile == file) marked.push_back(line_of(place.function, place.pc));
            }
        }

        Int count = source_line_count(file);
        if(count == 0) {
            source.push_back("(the program does not carry its source text: build it with -dbg)");
        } else {
            Int from = std::max((Int)1, line - SOURCE_LINES_KEPT / 2);
            Int to = std::min(count, from + SOURCE_LINES_KEPT);
            for(Int at = from; at <= to; at++) {
                bool stops = std::find(marked.begin(), marked.end(), at) != marked.end();
                stringstream row;
                row << (at == line ? "→" : stops ? "●" : " ") << std::setw(5) << at << "  " << source_line(file, at);
                if(at == line) current = (int)source.size();
                source.push_back(row.str());
            }
        }

        std::vector<const local_data*> seen = locals_in_view(frame);
        size_t widest = 0;
        for(const local_data *local : seen) widest = std::max(widest, written_name(*local).size());
        for(const local_data *local : seen) {
            held_value value;
            string problem;
            string name = written_name(*local);
            string row = name + string(widest - name.size(), ' ') + " = ";
            if(is_being_declared(frame, *local)) row += "(not set yet)";
            else if(value_of_local(frame, *local, value, problem)) row += text_of(value);
            else row += "(" + problem + ")";
            locals.push_back(row);
        }
        if(seen.empty() && function->localCount == 0) locals.push_back("(no names: build with -dbg and without -O)");

        std::vector<code_place> stack = call_stack_of(session->look);
        for(size_t i = 0; i < stack.size(); i++) {
            stringstream row;
            row << ((Int)i == frame.index ? "* " : "  ") << i << "  " << describe_place(stack[i]);
            calls.push_back(row.str());
        }
    }

    std::lock_guard<std::recursive_mutex> lock(screen_mut);
    source_title = title;
    source_rows = source;
    source_current = current;
    local_rows = locals;
    call_rows = calls;
}

// ---------------------------------------------------------------------------------
// The view's life
// ---------------------------------------------------------------------------------

void view_begin(debug_session *theSession) {
    session = theSession;
    colours = getenv("NO_COLOR") == nullptr;
    tcgetattr(STDIN_FILENO, &as_found);

    // What the process prints goes into a pipe from now on, and a thread reads it into the output
    // box; sdb keeps a copy of the terminal to draw on. (Without a buffer in between: a line the
    // program printed must be in the box when the program stops.)
    cout << flush;
    fflush(stdout);
    terminal = dup(STDOUT_FILENO);
    int ends[2];
    if(terminal < 0 || pipe(ends) != 0) return;
    dup2(ends[1], STDOUT_FILENO);
    dup2(ends[1], STDERR_FILENO);
    close(ends[1]);
    setvbuf(stdout, nullptr, _IOLBF, 0);
    cout.setf(std::ios::unitbuf);
    std::thread(read_the_output, ends[0]).detach();

    full = true;
    atexit(give_the_terminal_back);
    std::signal(SIGWINCH, size_changed);
    to_terminal("\x1b[?1049h\x1b[2J");              // the terminal's second screen, empty
    keys_to_sdb();
    draw();
}

void view_say(const string &line) {
    std::lock_guard<std::recursive_mutex> lock(screen_mut);
    keep(talk_lines, line);
    scrolled_back = 0;
}

void view_program_stopped(const string &why) {
    keys_to_sdb();
    refresh_boxes();
    {
        std::lock_guard<std::recursive_mutex> lock(screen_mut);
        running = false;
        last_line = why;
    }
    draw();
}

void view_refresh() {
    refresh_boxes();
    draw();
}

void view_program_runs() {
    {
        std::lock_guard<std::recursive_mutex> lock(screen_mut);
        running = true;
        scrolled_back = 0;
    }
    keys_to_the_program();
    draw();
}

/** the next key: a character, or one of the names below; 0 when a tenth of a second passed without one */
enum { KEY_NONE = 0, KEY_UP = 1000, KEY_DOWN, KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_END_OF_INPUT };

static int next_key() {
    pollfd waiting = { STDIN_FILENO, POLLIN, 0 };
    if(poll(&waiting, 1, 100) <= 0) return KEY_NONE;

    unsigned char c;
    if(read(STDIN_FILENO, &c, 1) != 1) return KEY_END_OF_INPUT;
    if(c != 0x1b) return c;

    // an escape sequence: the keys with arrows on them. What follows comes at once or not at all.
    string rest;
    while(rest.size() < 8) {
        pollfd more = { STDIN_FILENO, POLLIN, 0 };
        if(poll(&more, 1, 20) <= 0 || read(STDIN_FILENO, &c, 1) != 1) break;
        rest += (char)c;
        if(rest.size() > 1 && c >= '@' && c <= '~') break;
    }
    if(rest == "[A") return KEY_UP;
    if(rest == "[B") return KEY_DOWN;
    if(rest == "[5~") return KEY_PAGE_UP;
    if(rest == "[6~") return KEY_PAGE_DOWN;
    return KEY_NONE;
}

bool view_read_command(string &line) {
    {
        std::lock_guard<std::recursive_mutex> lock(screen_mut);
        typed.clear();
        given_at = given.size();
    }
    draw();

    for(;;) {
        int key = next_key();
        if(resized.load()) draw();
        if(key == KEY_NONE) continue;

        std::lock_guard<std::recursive_mutex> lock(screen_mut);
        if(key == KEY_END_OF_INPUT || (key == 4 && typed.empty())) {
            return false;
        } else if(key == '\r' || key == '\n') {
            line = typed;
            keep(talk_lines, "(sdb) " + typed);
            if(!typed.empty() && (given.empty() || given.back() != typed)) given.push_back(typed);
            typed.clear();
            scrolled_back = 0;
            draw();
            return true;
        } else if(key == 127 || key == 8) {
            // one character: its last byte and the bytes that only continue it
            while(!typed.empty() && ((unsigned char)typed.back() & 0xC0) == 0x80) typed.pop_back();
            if(!typed.empty()) typed.pop_back();
        } else if(key == KEY_UP) {
            if(given_at > 0) typed = given[--given_at];
        } else if(key == KEY_DOWN) {
            if(given_at + 1 < given.size()) typed = given[++given_at];
            else { given_at = given.size(); typed.clear(); }
        } else if(key == KEY_PAGE_UP) {
            scrolled_back = std::min((int)talk_lines.size(), scrolled_back + 5);
        } else if(key == KEY_PAGE_DOWN) {
            scrolled_back = std::max(0, scrolled_back - 5);
        } else if(key == 12) {
            on_screen.clear();                      // Ctrl+L: everything again
        } else if(key >= 32 && key < 256) {
            typed += (char)key;
        }
        draw();
    }
}

void view_end(const string &lastWords) {
    if(!full) return;

    if(!lastWords.empty()) {
        keys_to_sdb();
        {
            std::lock_guard<std::recursive_mutex> lock(screen_mut);
            running = false;
            over = true;
            last_line = lastWords;
            keep(talk_lines, lastWords);
            // (the program is gone: there is no line it is on, no variables, no calls)
            source_title = "the program has ended";
            source_rows.clear();
            source_current = -1;
            local_rows.clear();
            call_rows.clear();
        }
        draw();
        for(;;) {
            int key = next_key();
            if(resized.load()) draw();
            if(key == '\r' || key == '\n' || key == KEY_END_OF_INPUT || key == 4 || key == 'q') break;
        }
    }

    // the terminal as it was - and on it, what would otherwise be gone with the boxes: the last of
    // what the program printed, and the last words
    std::deque<string> printed;
    {
        std::lock_guard<std::recursive_mutex> lock(screen_mut);
        printed = output_lines;
        if(!output_unfinished.empty()) printed.push_back(output_unfinished);
    }
    give_the_terminal_back();
    size_t from = printed.size() > 30 ? printed.size() - 30 : 0;
    for(size_t i = from; i < printed.size(); i++) to_terminal(printed[i] + "\n");
    if(!lastWords.empty()) to_terminal(lastWords + "\n");
}

#endif
