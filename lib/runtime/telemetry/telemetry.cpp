//
// Sharp VM Telemetry - data collection and report writer.
// See telemetry.h for the overview and how to add metrics.
//

#include "telemetry.h"

#ifdef SHARP_TELEMETRY

#include "../virtual_machine.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../types/sharp_function.h"
#include "../jit/jit_compiler.h"
#include "../main.h"
#include <vector>
#include <map>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

thread_local telemetry_thread* tm_self = nullptr;

// ---------------------------------------------------------------------------------
// Global state (only touched on slow paths, guarded by tm_mutex)
// ---------------------------------------------------------------------------------
static std::mutex tm_mutex;
static std::vector<telemetry_thread*> tm_threads;        // every thread that ever reported, never freed
static unsigned long long tm_vm_start = 0;
static bool tm_report_written = false;

struct tm_jit_function {
    bool compiled = false;
    int blockedBy = -1;                 // opcode that stopped compilation, -1 = none
    unsigned long long compileNs = 0;
    size_t codeBytes = 0;
};
static std::map<sharp_function*, tm_jit_function> tm_jit_functions;

// Opcode names for the report, indexed by opcode value (lib/core/opcode/opcode.h).
// If you add an opcode there, add its name here too - unknown ones print as numbers.
static const char* tm_opcode_names[] = {
    "NOP", "INT", "MOVI", "RET", "HLT", "NEWARRAY", "CAST", "MOV8", "MOV16", "MOV32", "MOV64", "MOVU8",
    "MOVU16", "MOVU32", "MOVU64", "RSTORE", "ADD", "SUB", "MUL", "DIV", "MOD", "IADD", "ISUB", "IMUL",
    "IDIV", "IMOD", "POP", "INC", "DEC", "MOVR", "BRH", "IFE", "IFNE", "LT", "GT", "LTE", "GTE", "MOVL",
    "MOVSL", "SIZEOF", "PUT", "PUTC", "CHECKLEN", "JMP", "LOADPC", "PUSHOBJ", "DEL", "CALL", "NEWCLASS",
    "MOVN", "SLEEP", "TEST", "TNE", "LOCK", "ULOCK", "MOVG", "MOVND", "NEWOBJARRAY", "NOT", "SKIP",
    "LOADVAL", "SHL", "SHR", "SKPE", "SKNE", "AND", "UAND", "OR", "XOR", "THROW", "CHECKNULL",
    "RETURNOBJ", "NEWCLASSARRAY", "NEWSTRING", "ADDL", "SUBL", "MULL", "DIVL", "MODL", "IADDL", "ISUBL",
    "IMULL", "IDIVL", "IMODL", "LOADL", "IALOAD", "POPOBJ", "SMOVR", "ANDL", "ORL", "XORL", "RMOV",
    "SMOV", "RETURNVAL", "ISTORE", "SMOVR_2", "ISTOREL", "POPL", "PUSHNULL", "IPUSHL", "PUSHL", "ITEST",
    "INVOKE_DELEGATE", "GET", "ISADD", "JE", "JNE", "IPOPL", "CMP", "CALLD", "VARCAST", "TLS_MOVL",
    "DUP", "POPOBJ_2", "SWAP", "LDC", "SMOVR_3", "NEG", "EXP", "IS", "MOV_ABS", "LOAD_ABS", "ILOAD",
    "SMOVR_4", "IMOV"
};

static std::string opcode_name(int op) {
    if(op >= 0 && op < (int)(sizeof(tm_opcode_names) / sizeof(tm_opcode_names[0])))
        return tm_opcode_names[op];
    return "op#" + std::to_string(op);
}

static const char* tm_mode_names[] = {
#define X(id, name, desc) name,
    TM_MODE_LIST(X)
#undef X
};
static const char* tm_mode_descriptions[] = {
#define X(id, name, desc) desc,
    TM_MODE_LIST(X)
#undef X
};
static const char* tm_counter_names[] = {
#define X(id, name) name,
    TM_COUNTER_LIST(X)
#undef X
};

// ---------------------------------------------------------------------------------
// Thread registration
// ---------------------------------------------------------------------------------
telemetry_thread* telemetry_register_thread() {
    auto* t = (telemetry_thread*) calloc(1, sizeof(telemetry_thread));
    t->vmThreadId = -1;
    t->startNs = t->modeStartNs = telemetry_now();
    t->mode = tm_mode_other;

    std::lock_guard<std::mutex> guard(tm_mutex);
    snprintf(t->name, sizeof(t->name), "helper thread #%zu", tm_threads.size());
    tm_threads.push_back(t);
    tm_self = t;
    return t;
}

void telemetry_name_thread(const char* name) {
    telemetry_thread* t = telemetry_self();
    snprintf(t->name, sizeof(t->name), "%s", name);
}

void telemetry_thread_begin(sharp_thread* thread) {
    telemetry_thread* t = telemetry_self();
    t->vmThreadId = thread->id;
    snprintf(t->name, sizeof(t->name), "%s", thread->name.empty() ? "(unnamed)" : thread->name.c_str());
}

void telemetry_thread_end() {
    telemetry_thread* t = telemetry_self();
    telemetry_switch_mode(tm_mode_other);     // flush the time of the current mode
    t->endNs = telemetry_now();
    t->finished = true;
}

void telemetry_context_switch_wait(unsigned long long ns) {
    telemetry_thread* t = telemetry_self();
    t->switchWaitNs += ns;
    if(ns > t->switchWaitMaxNs) t->switchWaitMaxNs = ns;
}

// ---------------------------------------------------------------------------------
// JIT events
// ---------------------------------------------------------------------------------
void telemetry_jit_compiled(sharp_function* fn, unsigned long long ns, size_t codeBytes) {
    std::lock_guard<std::mutex> guard(tm_mutex);
    auto &entry = tm_jit_functions[fn];
    entry.compiled = true;
    entry.compileNs = ns;
    entry.codeBytes = codeBytes;
}

void telemetry_jit_blocked(sharp_function* fn, int opcode) {
    std::lock_guard<std::mutex> guard(tm_mutex);
    auto &entry = tm_jit_functions[fn];
    entry.compiled = false;
    entry.blockedBy = opcode;
}

void telemetry_jit_exit(int jitReturnCode) {
    switch(jitReturnCode) {
        case JIT_OK:             TELEMETRY_COUNT(tm_jit_exit_ok) break;
        case JIT_EXCEPTION:      TELEMETRY_COUNT(tm_jit_exit_exception) break;
        case JIT_CONTEXT_SWITCH: TELEMETRY_COUNT(tm_jit_exit_ctx_switch) break;
        case JIT_KILL:           TELEMETRY_COUNT(tm_jit_exit_kill) break;
        default: break;
    }
}

// ---------------------------------------------------------------------------------
// Report
// ---------------------------------------------------------------------------------
static double ms(unsigned long long ns) { return (double)ns / 1000000.0; }
static double pct(unsigned long long part, unsigned long long whole) {
    return whole == 0 ? 0.0 : ((double)part * 100.0) / (double)whole;
}

static void rule(FILE* f, const char* title) {
    fprintf(f, "\n================================================================================\n");
    fprintf(f, " %s\n", title);
    fprintf(f, "================================================================================\n");
}

void telemetry_write_report() {
    std::lock_guard<std::mutex> guard(tm_mutex);
    if(tm_report_written) return;
    tm_report_written = true;

    const char* path = getenv("SHARP_TELEMETRY_FILE");
    if(path == nullptr || *path == 0) path = "sharp_telemetry.txt";
    FILE* f = fopen(path, "w");
    if(f == nullptr) {
        fprintf(stderr, "telemetry: could not write %s\n", path);
        return;
    }

    unsigned long long now = telemetry_now();
    unsigned long long wall = now - tm_vm_start;

    // ---- snapshot every thread (threads that are still running are read as-is) and build totals
    std::vector<telemetry_thread> threads;
    telemetry_thread total;
    memset(&total, 0, sizeof(total));
    for(telemetry_thread* live : tm_threads) {
        telemetry_thread t = *live;
        if(!t.finished) {
            t.modeNs[t.mode] += now - t.modeStartNs;
            t.endNs = now;
        }
        for(int i = 0; i < tm_mode_count; i++) { total.modeNs[i] += t.modeNs[i]; total.modeEnters[i] += t.modeEnters[i]; }
        for(int i = 0; i < tm_counter_count; i++) total.counters[i] += t.counters[i];
        for(int i = 0; i < 256; i++) total.opcodes[i] += t.opcodes[i];
        total.switchWaitNs += t.switchWaitNs;
        if(t.switchWaitMaxNs > total.switchWaitMaxNs) total.switchWaitMaxNs = t.switchWaitMaxNs;
        threads.push_back(t);
    }
    unsigned long long threadTime = 0, instructions = 0;
    for(int i = 0; i < tm_mode_count; i++) threadTime += total.modeNs[i];
    for(int i = 0; i < 256; i++) instructions += total.opcodes[i];

    fprintf(f, "Sharp VM telemetry report\n");
    fprintf(f, "application : %s\n", vm.mf.application.c_str());
    fprintf(f, "jit         : %s\n", c_options.jit ? "enabled" : "disabled (-nojit or unsupported host)");
    fprintf(f, "wall time   : %.3f ms\n", ms(wall));
    fprintf(f, "threads     : %zu (sum of all thread time: %.3f ms)\n", threads.size(), ms(threadTime));

    // ------------------------------------------------------------------ where the time went
    rule(f, "WHERE THE TIME WENT (all threads, exclusive time)");
    fprintf(f, "%-28s %14s %8s %12s   %s\n", "mode", "time (ms)", "share", "entered", "what it means");
    for(int i = 0; i < tm_mode_count; i++) {
        fprintf(f, "%-28s %14.3f %7.2f%% %12llu   %s\n", tm_mode_names[i], ms(total.modeNs[i]),
                pct(total.modeNs[i], threadTime), total.modeEnters[i], tm_mode_descriptions[i]);
    }
    unsigned long long execNs = total.modeNs[tm_mode_interpreter] + total.modeNs[tm_mode_jit];
    fprintf(f, "\nbytecode execution split: interpreter %.2f%%  |  jit %.2f%%\n",
            pct(total.modeNs[tm_mode_interpreter], execNs), pct(total.modeNs[tm_mode_jit], execNs));
    if(instructions > 0) {
        fprintf(f, "interpreter speed: %llu instructions in %.3f ms = %.2f ns per instruction (%.1f million instructions/sec)\n",
                instructions, ms(total.modeNs[tm_mode_interpreter]),
                (double)total.modeNs[tm_mode_interpreter] / (double)instructions,
                total.modeNs[tm_mode_interpreter] == 0 ? 0.0
                    : ((double)instructions * 1000.0) / (double)total.modeNs[tm_mode_interpreter]);
    }

    // ------------------------------------------------------------------ threads
    rule(f, "THREADS");
    for(telemetry_thread &t : threads) {
        unsigned long long life = t.endNs - t.startNs;
        fprintf(f, "\n[%s]  vm thread id: %lld  lifetime: %.3f ms  %s\n", t.name, t.vmThreadId, ms(life),
                t.finished ? "(exited)" : "(still running at exit)");
        for(int i = 0; i < tm_mode_count; i++) {
            if(t.modeNs[i] == 0 && t.modeEnters[i] == 0) continue;
            fprintf(f, "    %-28s %14.3f ms %7.2f%%   entered %llu times\n", tm_mode_names[i], ms(t.modeNs[i]),
                    pct(t.modeNs[i], life), t.modeEnters[i]);
        }
        if(t.counters[tm_context_switches] > 0) {
            fprintf(f, "    context switches: %llu, avg wait for next fiber %.3f ms, worst %.3f ms\n",
                    t.counters[tm_context_switches],
                    ms(t.switchWaitNs) / (double)t.counters[tm_context_switches], ms(t.switchWaitMaxNs));
        }
    }

    // ------------------------------------------------------------------ multitasking
    rule(f, "MULTITASKING");
    fprintf(f, "threads created            : %llu\n", total.counters[tm_threads_created]);
    fprintf(f, "fibers created             : %llu\n", total.counters[tm_fibers_created]);
    fprintf(f, "context switches           : %llu\n", total.counters[tm_context_switches]);
    if(total.counters[tm_context_switches] > 0) {
        fprintf(f, "  avg time parked per switch: %.3f ms (worst %.3f ms) - time between giving up a fiber and running the next one\n",
                ms(total.switchWaitNs) / (double)total.counters[tm_context_switches], ms(total.switchWaitMaxNs));
    }
    fprintf(f, "suspends                   : %llu (total time suspended %.3f ms)\n",
            total.counters[tm_suspends], ms(total.modeNs[tm_mode_suspended]));
    fprintf(f, "object locks               : %llu acquired, %llu contended\n",
            total.counters[tm_lock_acquired], total.counters[tm_lock_contended]);

    // ------------------------------------------------------------------ counters
    rule(f, "COUNTERS (all threads)");
    for(int i = 0; i < tm_counter_count; i++)
        fprintf(f, "%-62s %14llu\n", tm_counter_names[i], total.counters[i]);

    // ------------------------------------------------------------------ opcodes
    rule(f, "HOTTEST INTERPRETER OPCODES");
    fprintf(f, "(only instructions run by the interpreter are counted; jit compiled code is not)\n\n");
    std::vector<std::pair<unsigned long long, int>> ops;
    for(int i = 0; i < 256; i++) if(total.opcodes[i]) ops.push_back({total.opcodes[i], i});
    std::sort(ops.rbegin(), ops.rend());
    fprintf(f, "%-18s %16s %8s %10s\n", "opcode", "executed", "share", "cumulative");
    double cumulative = 0;
    for(size_t i = 0; i < ops.size() && i < 40; i++) {
        cumulative += pct(ops[i].first, instructions);
        fprintf(f, "%-18s %16llu %7.2f%% %9.2f%%\n", opcode_name(ops[i].second).c_str(), ops[i].first,
                pct(ops[i].first, instructions), cumulative);
    }

    // ------------------------------------------------------------------ jit
    rule(f, "JIT");
    size_t compiled = 0, blocked = 0, codeBytes = 0;
    unsigned long long compileNs = 0;
    std::map<int, std::pair<unsigned long long, unsigned long long>> blockers; // opcode -> (functions, calls)
    for(auto &it : tm_jit_functions) {
        if(it.second.compiled) { compiled++; codeBytes += it.second.codeBytes; compileNs += it.second.compileNs; }
        else if(it.second.blockedBy >= 0) {
            blocked++;
            blockers[it.second.blockedBy].first++;
            blockers[it.second.blockedBy].second += it.first->callCount;
        }
    }
    fprintf(f, "functions compiled         : %zu (%zu bytes of machine code, %.3f ms spent compiling)\n",
            compiled, codeBytes, ms(compileNs));
    fprintf(f, "functions that could not be compiled: %zu\n", blocked);
    fprintf(f, "calls that ran compiled code: %llu of %llu total calls (%.2f%%)\n",
            total.counters[tm_calls_into_jit], total.counters[tm_calls_total],
            pct(total.counters[tm_calls_into_jit], total.counters[tm_calls_total]));
    fprintf(f, "nested interpreter loops    : %llu (a compiled function called a function and it ran in the interpreter)\n",
            total.counters[tm_calls_nested_interp]);

    if(!blockers.empty()) {
        fprintf(f, "\nUNIMPLEMENTED OPCODES THAT STOPPED A HOT FUNCTION FROM BEING COMPILED\n");
        fprintf(f, "(the first unsupported opcode in each function; sorted by how many calls it is costing.\n");
        fprintf(f, " implementing the top ones in the jit unlocks the most functions)\n\n");
        std::vector<std::pair<unsigned long long, int>> order;
        for(auto &b : blockers) order.push_back({b.second.second, b.first});
        std::sort(order.rbegin(), order.rend());
        fprintf(f, "%-18s %12s %16s\n", "opcode", "functions", "calls affected");
        for(auto &o : order)
            fprintf(f, "%-18s %12llu %16llu\n", opcode_name(o.second).c_str(), blockers[o.second].first, o.first);
    }

    // ------------------------------------------------------------------ functions
    rule(f, "HOTTEST FUNCTIONS (by call count)");
    std::vector<sharp_function*> fns;
    for(uInt i = 0; i < vm.mf.methods; i++)
        if(vm.methods[i].callCount > 0) fns.push_back(&vm.methods[i]);
    std::sort(fns.begin(), fns.end(), [](sharp_function* a, sharp_function* b) { return a->callCount > b->callCount; });
    fprintf(f, "%14s %8s  %-26s %s\n", "calls", "words", "jit status", "function");
    for(size_t i = 0; i < fns.size() && i < 50; i++) {
        sharp_function* fn = fns[i];
        std::string status;
        auto it = tm_jit_functions.find(fn);
        if(fn->nativeFunc) status = "native";
        else if(it != tm_jit_functions.end() && it->second.compiled) status = "COMPILED";
        else if(it != tm_jit_functions.end() && it->second.blockedBy >= 0) status = "blocked by " + opcode_name(it->second.blockedBy);
        else if(!c_options.jit) status = "jit disabled";
        else status = "not attempted";
        fprintf(f, "%14u %8d  %-26s %s\n", fn->callCount, fn->bytecodeSize, status.c_str(), fn->fullName.c_str());
    }

    // ------------------------------------------------------------------ memory
    rule(f, "MEMORY / GC");
    fprintf(f, "objects allocated          : %llu\n", total.counters[tm_objects_created]);
    fprintf(f, "reference assignments      : %llu (each one takes the global copy_object lock)\n", total.counters[tm_object_copies]);
    fprintf(f, "gc collections             : %llu, total %.3f ms\n", total.counters[tm_gc_cycles], ms(total.modeNs[tm_mode_gc]));
    fprintf(f, "objects freed by the gc    : %llu\n", total.counters[tm_gc_objects_freed]);
    fprintf(f, "fiber stack grows          : %llu, call-frame grows: %llu\n",
            total.counters[tm_stack_grows], total.counters[tm_frame_grows]);

    fclose(f);
    fprintf(stderr, "telemetry: report written to %s\n", path);
}

void telemetry_init() {
    tm_vm_start = telemetry_now();
    // atexit also covers the paths where the vm leaves through exit() instead of
    // returning from main (fatal errors, the signal handler, System.exit).
    atexit(telemetry_write_report);
}

#endif // SHARP_TELEMETRY
