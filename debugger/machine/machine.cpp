//
// See machine.h.
//
// TWO THREADS. The one run_machine() is called on waits for the program to
// stop, says so, waits for a request that lets it go, and lets it go. The other
// reads the editor's requests, all the time, and answers them. What the two
// share (is the program stopped, the request that lets it go) is under state_mut.
//

#include "machine.h"
#include "../control/control.h"
#include "../control/threads.h"
#include "../values/values.h"
#include "../prompt/prompt.h"
#include "../settings/json.h"
#include "../../runtime/multitasking/thread/sharp_thread.h"
#include "../../runtime/multitasking/fiber/fiber.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_handle;
#define NO_SOCKET INVALID_SOCKET
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_handle;
#define NO_SOCKET (-1)
#endif

#define MOST_PARTS_AT_ONCE 100          // how many elements of an array one "children" request gives at most

// ---------------------------------------------------------------------------------
// The channel: lines over one connection
// ---------------------------------------------------------------------------------

static socket_handle channel = NO_SOCKET;
static std::mutex &send_mut = *new std::mutex();        // a line is sent whole: two threads send

bool machine_connect(int port) {
#ifdef _WIN32
    WSADATA data;
    if(WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
#endif
    channel = socket(AF_INET, SOCK_STREAM, 0);
    if(channel == NO_SOCKET) return false;

    sockaddr_in where;
    memset(&where, 0, sizeof(where));
    where.sin_family = AF_INET;
    where.sin_port = htons((unsigned short)port);
    where.sin_addr.s_addr = htonl(INADDR_LOOPBACK);     // this computer only
    if(connect(channel, (sockaddr*)&where, sizeof(where)) != 0) return false;

    int at_once = 1;                                    // a line goes out when it is written, not a moment later
    setsockopt(channel, IPPROTO_TCP, TCP_NODELAY, (const char*)&at_once, sizeof(at_once));
    return true;
}

static void send_line(const string &line) {
    std::lock_guard<std::mutex> lock(send_mut);
    string whole = line + "\n";
    size_t done = 0;
    while(done < whole.size()) {
        int sent = (int)send(channel, whole.data() + done, (int)(whole.size() - done), 0);
        if(sent <= 0) return;
        done += (size_t)sent;
    }
}

/** the next line the editor sent; false when the connection is gone */
static bool read_line(string &line) {
    static string waiting;                              // what came after the last line's end
    for(;;) {
        size_t end = waiting.find('\n');
        if(end != string::npos) {
            line = waiting.substr(0, end);
            if(!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
            waiting.erase(0, end + 1);
            return true;
        }
        char bytes[4096];
        int count = (int)recv(channel, bytes, sizeof(bytes), 0);
        if(count <= 0) return false;
        waiting.append(bytes, (size_t)count);
    }
}

// ---------------------------------------------------------------------------------
// Writing JSON: `{"name": value, ...}` built a member at a time
// ---------------------------------------------------------------------------------

struct json_object {
    string text = "{";

    json_object& member(const string &name) {
        if(text.size() > 1) text += ", ";
        text += json_text(name) + ": ";
        return *this;
    }
    json_object& add(const string &name, const string &value) { member(name); text += json_text(value); return *this; }
    json_object& add(const string &name, const char *value) { return add(name, string(value)); }
    json_object& add(const string &name, Int value) { member(name); text += std::to_string(value); return *this; }
    json_object& add(const string &name, bool value) { member(name); text += value ? "true" : "false"; return *this; }
    /** a member whose value is JSON already (a list, an object) */
    json_object& add_json(const string &name, const string &json) { member(name); text += json; return *this; }
    string done() const { return text + "}"; }
};

/** `[a, b, c]` from texts that are JSON already */
static string json_list(const std::vector<string> &items) {
    string text = "[";
    for(size_t i = 0; i < items.size(); i++) text += (i > 0 ? ", " : "") + items[i];
    return text + "]";
}

void machine_say(const string &line) {
    send_line(json_object().add("event", "say").add("text", line).done());
}

// ---------------------------------------------------------------------------------
// What the two threads share
// ---------------------------------------------------------------------------------

static std::mutex &state_mut = *new std::mutex();
static std::condition_variable &state_changed = *new std::condition_variable();
static bool stopped = false;                // the program is held and may be looked at
static stop_report stop;                    // ... where
static bool let_go = false;                 // a request said: run on
static step_kind step_wanted = STEP_NONE;   // ... how far
static fiber *step_task = nullptr;          // ... which fiber (null: the one that stopped)

// ---------------------------------------------------------------------------------
// Answers
// ---------------------------------------------------------------------------------

static string refused(Int id, const string &why) {
    return json_object().add("id", id).add("ok", false).add("why", why).done();
}

static json_object accepted(Int id) {
    json_object answer;
    answer.add("id", id).add("ok", true);
    return answer;
}

/** the fiber a request names by its id; null when there is none like it */
static fiber* fiber_of(const json_value &request) {
    Int id = json_number_of(request, "fiber", -1);
    for(fiber_row &row : fibers_now()) {
        if(row.id == id) return row.task;
    }
    return nullptr;
}

/** the frame a request names (fiber, frame); false, with the answer to send, when it is not there */
static bool frame_of(const json_value &request, Int id, frame_view &frame, string &answer) {
    fiber *task = fiber_of(request);
    if(task == nullptr || !frame_at(task, json_number_of(request, "frame", 0), frame)) {
        answer = refused(id, "there is no such fiber or frame");
        return false;
    }
    return true;
}

/** one variable as the editor gets it: its name, what it holds, its kind, whether it can be opened, the path to ask with */
static string variable_json(const string &name, const string &path, const held_value &value) {
    return json_object().add("name", name).add("value", text_of(value)).add("kind", kind_of(value))
            .add("parts", has_parts(value)).add("path", path).done();
}

static string answer_break(Int id, const json_value &request) {
    string file = json_text_of(request, "file");
    Int line = json_number_of(request, "line", -1);

    std::vector<Int> files = files_named(file);
    if(files.size() != 1) return refused(id, files.empty() ? "the program has no source file " + file : "more than one source file is named " + file);

    Int nextLine = -1;
    std::vector<code_place> places = places_at_line(files[0], line, nextLine);
    if(places.empty()) {
        json_object answer;
        answer.add("id", id).add("ok", false).add("why", "the line holds no code");
        if(nextLine != -1) answer.add("next", nextLine);
        return answer.done();
    }

    stringstream written;
    written << file << ":" << line;
    int number = sdb_add_breakpoint(places, written.str());
    return accepted(id).add("number", (Int)number).add("line", line).done();
}

static string answer_threads(Int id) {
    std::vector<string> rows;
    for(thread_row &row : threads_now()) {
        if(row.own) continue;               // the VM's own threads run no Sharp code
        rows.push_back(json_object().add("id", row.id).add("name", row.name).add("state", row.state)
                .add("fiber", row.fiberId).add("stopped", row.task != nullptr && row.task == stop.task).done());
    }
    return accepted(id).add_json("threads", json_list(rows)).done();
}

static string answer_frames(Int id, const json_value &request) {
    fiber *task = fiber_of(request);
    if(task == nullptr) return refused(id, "there is no such fiber");

    std::vector<string> rows;
    std::vector<code_place> calls = call_stack_of(task);
    for(size_t i = 0; i < calls.size(); i++) {
        sharp_function *function = calls[i].function;
        rows.push_back(json_object().add("index", (Int)i).add("function", name_of(function))
                .add("file", written_file(function->sourceFile)).add("line", line_of(function, calls[i].pc)).done());
    }
    return accepted(id).add_json("frames", json_list(rows)).done();
}

static string answer_variables(Int id, const json_value &request) {
    frame_view frame;
    string answer;
    if(!frame_of(request, id, frame, answer)) return answer;

    std::vector<string> rows;
    for(const local_data *local : locals_in_view(frame)) {
        held_value value;
        string problem;
        string name = written_name(*local);
        if(is_being_declared(frame, *local)) {
            rows.push_back(json_object().add("name", name).add("value", "not set yet").add("kind", "").add("parts", false).add("path", name).done());
        } else if(value_of_local(frame, *local, value, problem)) {
            rows.push_back(variable_json(name, name, value));
        }
    }
    return accepted(id).add_json("variables", json_list(rows)).done();
}

static string answer_children(Int id, const json_value &request) {
    frame_view frame;
    string answer;
    if(!frame_of(request, id, frame, answer)) return answer;

    string path = json_text_of(request, "path");
    held_value value;
    string problem;
    if(!value_of_path(frame, path, value, problem)) return refused(id, problem);

    Int total = 0;
    Int from = json_number_of(request, "from", 0);
    Int count = std::min((Int)MOST_PARTS_AT_ONCE, json_number_of(request, "count", MOST_PARTS_AT_ONCE));
    std::vector<string> rows;
    for(value_part &part : parts_between(value, from, count, total)) {
        rows.push_back(variable_json(part.shown, path + part.name, part.value));
    }
    return accepted(id).add_json("variables", json_list(rows)).add("total", total).done();
}

static string answer_set(Int id, const json_value &request) {
    frame_view frame;
    string answer;
    if(!frame_of(request, id, frame, answer)) return answer;

    held_value value;
    string problem;
    if(!value_of_path(frame, json_text_of(request, "path"), value, problem)) return refused(id, problem);
    if(!set_value(value, json_text_of(request, "value"), problem)) return refused(id, problem);
    return accepted(id).add("value", text_of(value)).add("kind", kind_of(value)).add("parts", has_parts(value)).done();
}

/** a request that lets the program go: note how far, and wake the other thread */
static string answer_go(Int id, const json_value &request, step_kind step) {
    if(step != STEP_NONE && !stop.atInstruction) return refused(id, "no thread was running Sharp code when the program stopped: there is nothing to step");
    let_go = true;
    step_wanted = step;
    step_task = step == STEP_NONE ? nullptr : fiber_of(request);
    stopped = false;
    state_changed.notify_all();
    return accepted(id).done();
}

/** one request, answered (state_mut held) */
static string answer_to(const json_value &request) {
    Int id = json_number_of(request, "id", 0);
    string what = json_text_of(request, "do");

    // ---- at any time
    if(what == "break") return answer_break(id, request);
    if(what == "clear") {
        if(!sdb_clear_breakpoint((int)json_number_of(request, "number", 0))) return refused(id, "there is no such breakpoint");
        return accepted(id).done();
    }
    if(what == "pause") {
        if(stopped) return refused(id, "the program is stopped already");
        sdb_pause();
        return accepted(id).done();
    }
    if(what == "quit") {
        send_line(accepted(id).done());
        _exit(0);
    }

    // ---- only while the program is stopped
    if(!stopped) return refused(id, "the program is running");
    if(what == "start" || what == "continue") return answer_go(id, request, STEP_NONE);
    if(what == "next") return answer_go(id, request, STEP_NEXT);
    if(what == "into") return answer_go(id, request, STEP_INTO);
    if(what == "out") return answer_go(id, request, STEP_OUT);
    if(what == "threads") return answer_threads(id);
    if(what == "frames") return answer_frames(id, request);
    if(what == "variables") return answer_variables(id, request);
    if(what == "children") return answer_children(id, request);
    if(what == "set") return answer_set(id, request);
    return refused(id, "sdb does not know `" + what + "`");
}

/** the thread that reads the editor's requests for as long as the connection lasts */
static void read_requests() {
    string line;
    while(read_line(line)) {
        if(line.empty()) continue;

        json_value request;
        string problem;
        if(!read_json_object(line, request, problem)) {
            send_line(refused(0, "not a request: " + problem));
            continue;
        }
        std::lock_guard<std::mutex> lock(state_mut);
        send_line(answer_to(request));
    }
    // the editor is gone: there is nobody to debug for
    _exit(0);
}

// ---------------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------------

static string stopped_event(const stop_report &report) {
    const char *reason = report.reason == STOP_AT_BREAKPOINT ? "breakpoint"
                       : report.reason == STOP_AFTER_STEP ? "step"
                       : report.reason == STOP_PAUSED ? "pause" : "start";

    json_object event;
    event.add("event", "stopped").add("reason", reason).add("number", (Int)report.breakpoint)
         .add("thread", (Int)report.thread->id).add("fiber", (Int)report.task->id).add("can_step", report.atInstruction);

    std::vector<code_place> calls = call_stack_of(report.task);
    if(!calls.empty()) {
        sharp_function *function = calls[0].function;
        event.add("function", name_of(function)).add("file", written_file(function->sourceFile))
             .add("line", line_of(function, calls[0].pc));
    }
    return event.done();
}

void run_machine() {
    std::thread(read_requests).detach();

    stop_report report;
    while(sdb_wait_for_stop(report)) {
        if(report.reason == STOP_LOADED) {
            prepare_the_program();
            send_line(json_object().add("event", "ready").done());
        } else {
            send_line(stopped_event(report));
        }

        step_kind step;
        fiber *task;
        {
            std::unique_lock<std::mutex> lock(state_mut);
            stop = report;
            stopped = true;
            let_go = false;
            state_changed.wait(lock, [] { return let_go; });
            step = step_wanted;
            task = step_task != nullptr ? step_task : report.task;
        }
        send_line(json_object().add("event", "running").done());
        sdb_resume(step, task);
    }

    send_line(json_object().add("event", "ended").add("code", (Int)sdb_exit_code()).done());
}
