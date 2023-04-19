//
// Created by bknun on 9/21/2022.
//

#ifndef SHARP_HANDSHAKE_H
#define SHARP_HANDSHAKE_H

#include <atomic>
#include "../../../../stdimports.h"
#include "../../../util/KeyPair.h"

enum handshake_code {
    HANDSHAKE_NEW_TASK = 0x000
};

/**
 * Sngle event handshake between 2 threads
 *
 */
struct handshake {
    recursive_mutex mut;
    bool listening;
    atomic<bool> accepted;
    atomic<void *> data;
    atomic<Int> code;
};

void init_struct(handshake *hs);

KeyPair<Int, void*> listen(handshake *hs, uInt usTime = -1);
bool try_handshake(handshake *hs, void *data, Int code, uInt usTime = -1);

#endif //SHARP_HANDSHAKE_H
