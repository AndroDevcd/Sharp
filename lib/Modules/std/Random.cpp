//
// Created by bknun on 7/24/2018.
//

#include "Random.h"
#include "../../runtime/Thread.h"
#include "../../runtime/register.h"

double __crand(int proc) {
    Random *rand = thread_self->rand;
    switch(proc) {
        case 0x000:
            return rand->rand(EBX, ECX);
        case 0x001:
            return rand->randInt((long long)EBX, (long long)ECX);
        case 0x002:
            return rand->reset();
        case 0x003:
            rand->seed((__seed_type)EBX);
            return 0;
        case 0x004:
            return rand->seed();
        case 0x005:
            return rand->rand();
        case 0x006:
            return rand->randInt();
        default:
            throw Exception("unsupported operation");
    }
}
