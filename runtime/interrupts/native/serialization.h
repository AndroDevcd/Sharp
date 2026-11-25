//
// Created by BNunnally on 9/14/2020.
// Optimized by Claude on 3/26/2025
// Further performance optimizations by Claude on 4/4/2025
// The format and both directions rewritten for speed by Claude on 11/25/2026 (serialization.cpp says how)
//

#ifndef SHARP_SERIALIZATION_H
#define SHARP_SERIALIZATION_H

#include "../../memory/sharp_object.h"

// serialize(from, to): the object in `from`, and everything it refers to, as bytes: an _int8[] put into `to`.
// deserialize(from, to): the objects those bytes (an _int8[] in `from`) describe, built again: the first one into `to`.
// Both throw a vm_exception when they cannot (and leave null in `to`).
void serialize(object *from, object *to);
void deserialize(object *from, object *to);

#endif //SHARP_SERIALIZATION_H
