//
// Created by bknun on 4/23/2023.
//

#ifndef SHARP_NOTIFICATION_HELPER_H
#define SHARP_NOTIFICATION_HELPER_H

#include <condition_variable>
#include "../../../../stdimports.h"

struct notification {
    std::condition_variable cv;
    std::mutex cv_m;
    bool (*cond)();
};

CXX11_INLINE void init_struct(notification *n);

void wait_for_notification(notification *n);
void send_notification(notification *n);

#endif //SHARP_NOTIFICATION_HELPER_H
