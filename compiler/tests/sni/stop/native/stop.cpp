//
// The C++ side of the library "stop" (tests/sni/stop/main.sharp).
//
#include "stop_sni.h"
#include <atomic>
#include <chrono>
#include <thread>

static std::atomic<int> loops_started(0);

// the polite loop: asks the VM now and then whether its thread was asked to stop
int64_t stop_spin_politely(sni_env* env) {
    loops_started++;
    int64_t turns = 0;
    while(!env->should_stop()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        turns++;
    }
    return turns;
}

// the stubborn loop: never asks. The VM waits 2 seconds for it, then stops waiting.
void stop_spin_stubbornly(sni_env* env) {
    loops_started++;
    for(;;) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

int32_t stop_started(sni_env* env) { return loops_started.load(); }
