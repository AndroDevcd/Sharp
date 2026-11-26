//
// Created by bknun on 4/22/2023.
//

#include "idle_scheduler.h"
#include "scheduler.h"
#include "../thread/thread_controller.h"
#include "../thread/sharp_thread.h"
#include "../thread_state.h"
#include "../../error/vm_exception.h"
#include "../../memory/garbage_collector.h"

/* how long the idle thread sleeps between checks of its own signals */
#define IDLE_SLEEP_MICROS 10000

/**
 * The idle thread has no work of its own anymore (see idle_scheduler.h). It
 * sleeps and only looks at its signals so it can be suspended by the garbage
 * collector and shut down with the vm like every other thread.
 */
void idle_main_loop() {
    for(;;) {
        CHECK_STATE
        __usleep(IDLE_SLEEP_MICROS);
    }
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
idle_main(void *pVoid) {
    thread_self = (sharp_thread*)pVoid;
    thread_self->state = THREAD_RUNNING;
    set_thread_priority(thread_self, THREAD_PRIORITY_LOW);

    try {
        idle_main_loop();
    } catch(vm_exception &e){
        /* Should never happen */
        enable_exception_flag(thread_self, true);
    }

    /*
     * Check for uncaught exception in thread before exit
     */
    shutdown_thread(thread_self);

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void idle_handler_startup() {
    start_daemon_thread(
            idle_main,
            create_idle_handler_thread()
    );
}
