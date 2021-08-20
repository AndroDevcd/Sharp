//
// Created by BNunnally on 4/29/2020.
//

#ifndef SHARP_THREADSTATES_H
#define SHARP_THREADSTATES_H

enum ThreadState {
    THREAD_CREATED      =0x000,
    THREAD_STARTED      =0x001,
    THREAD_RUNNING      =0x002,
    THREAD_SUSPENDED    =0x003,
    THREAD_KILLED       =0x004
};

enum ThreadPriority {
    THREAD_PRIORITY_LOW = 0x0001,
    THREAD_PRIORITY_NORM = 0X0004,
    THREAD_PRIORITY_HIGH = 0X0006
};

enum ThreadProcessingResult {
    RESULT_OK                      = 0x000,
    RESULT_ILL_THREAD_START        = 0x001,
    RESULT_THREAD_RUNNING          = 0x002,
    RESULT_THREAD_TERMINATED       = 0x003,
    RESULT_INVALID_STACK_SIZE      = 0x004,
    RESULT_THREAD_NOT_STARTED      = 0x005,
    RESULT_ILL_THREAD_JOIN         = 0x006,
    RESULT_THREAD_JOIN_FAILED      = 0x007,
    RESULT_ILL_THREAD_INTERRUPT    = 0x008,
    RESULT_THREAD_INTERRUPT_FAILED = 0x009,
    RESULT_ILL_THREAD_DESTROY      = 0x00a,
    RESULT_THREAD_CREATE_FAILED    = 0x00b,
    RESULT_NO_THREAD_ID            = 0x00c,
    RESULT_ILL_PRIORITY_SET        = 0x00e,
    RESULT_ILL_THREAD_SUSPEND      = 0x00f,
    RESULT_MAX_SPIN_GIVEUP         = 0x0ff
};

/**
 * Thread signals for events being handled in the threadding system
 *
 * Signals are send and read from at specific times and it makes
 * for a much faster processing to better handle signals as they arise
 */
enum tsig_t {
    tsig_empty = 0x000,
    tsig_except = 0x001,
    tsig_suspend = 0x002,
    tsig_kill = 0x004,
    tsig_context_switch = 0x008
};

#endif //SHARP_THREADSTATES_H
