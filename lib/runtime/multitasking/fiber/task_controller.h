//
// Created by bnunnally on 8/17/21.
//

#ifndef SHARP_TASK_CONTROLLER_H
#define SHARP_TASK_CONTROLLER_H

#include "fiber.h"
#include "../scheduler/scheduler.h"

#define NO_DELAY 0

class sharp_function;

bool is_bound(fiber*, sharp_thread*);
int kill_task(fiber*);
int kill_task(uInt);
Int suspend_task(uInt);
Int unsuspend_task(uInt);
void set_task_wakeable(fiber*, bool);
void set_task_state(sharp_thread*, fiber*, fiber_state, uInt);
Int bind_task(fiber*, sharp_thread*);
Int get_state(fiber*);
sharp_thread* get_attached_thread(fiber*);
sharp_thread* get_bound_thread(fiber*);
void set_attached_thread(fiber*, sharp_thread*);
void delay_task(uInt);

fiber* create_task(string&, sharp_function*);


#endif //SHARP_TASK_CONTROLLER_H
