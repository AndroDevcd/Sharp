//
// Created by bnunnally on 8/17/21.
//

#ifndef SHARP_TASK_CONTROLLER_H
#define SHARP_TASK_CONTROLLER_H

#include "../fiber.h"
#include "scheduler.h"

#define NO_DELAY 0

class Method;

bool is_bound(fiber*, Thread*);
void kill_bound_fibers(Thread*);
int kill_task(fiber*);
int kill_task(uInt);
fiber *locate_task(uInt);
Int suspend_task(uInt);
Int unsuspend_task(uInt);
void set_task_wakeable(fiber*, bool);
void set_task_state(Thread*, fiber*, fiber_state, uInt);
Int bind_task(fiber*, Thread*);
Int get_state(fiber*);
Thread* get_attached_thread(fiber*);
Thread* get_bound_thread(fiber*);
void set_attached_thread(fiber*, Thread*);
void delay_task(uInt, bool);
bool start_task(Thread*, fiber*);
Int bound_task_count(Thread*);

fiber* create_task(string&, Method*);


#endif //SHARP_TASK_CONTROLLER_H
