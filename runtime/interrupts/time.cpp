//
// System interrupts for the clock and for pausing.
//
// Moved verbatim from the old switch in virtual_machine.cpp (OP_OS_TIME,
// OP_NAN_TIME, OP_DELAY). See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"

//
// os_time(which): a wall-clock time from the operating system (the `which`
// selects which clock). Whole units, returned in BMR.
//
void op_os_time(Int interrupt) {
    registers[BMR].i = Clock::__os_time((int)registers[EBX].i);
}

//
// nano_time(): a high-resolution timestamp in nanoseconds, returned in BMR.
// Used for measuring elapsed time, not for telling the date.
//
void op_nano_time(Int interrupt) {
    registers[BMR].i = Clock::realTimeInNSecs();
}

//
// delay(ms): pause this fiber for a number of milliseconds (EBX). The scheduler
// is free to run other fibers while it waits.
//
void op_delay(Int interrupt) {
    delay_task(registers[EBX].i);
}
