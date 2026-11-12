//
// Created by bknun on 4/23/2023.
//

#include "notification_helper.h"
#include "../../memory/tracing_collector.h"

void wait_for_notification(notification *n) {
    // Declared before the lock so it ends AFTER the lock is released: leaving a
    // safe region can wait for the garbage collector, and nothing may be held then.
    gc_safe_region waiting;
    std::unique_lock<std::mutex> lk(n->cv_m);
    while ( !n->cond() ) n->cv.wait(lk);
}

void send_notification(notification *n) {
    std::unique_lock<std::mutex> lk(n->cv_m);
    n->cv.notify_one();
}

void init_struct(notification *n) {
    new (&n->cv) std::condition_variable();
    new (&n->cv_m) std::mutex();
    n->cond = nullptr;
}