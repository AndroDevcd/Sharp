//
// Created by bknun on 4/22/2023.
//

#ifndef SHARP_IDLE_SCHEDULER_H
#define SHARP_IDLE_SCHEDULER_H

#define MAIN_POOL_SIZE_LIMIT (1000)

struct sched_task;
struct sharp_thread;

void idle_handler_startup();
bool is_task_idle(sched_task *task);
void post_idle_task(sched_task *task);
void kill_bound_idle_tasks(sharp_thread* thread);

#endif //SHARP_IDLE_SCHEDULER_H
