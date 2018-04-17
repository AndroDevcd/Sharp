//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_MUTEX_H
#define SHARP_MUTEX_H

#include <chrono>
#include <thread>
#include "../../stdimports.h"

int MUTEX_INIT(MUTEX *mutex);
int MUTEX_LOCK(MUTEX *mutex);
int MUTEX_UNLOCK(MUTEX *mutex);

#endif //SHARP_MUTEX_H
