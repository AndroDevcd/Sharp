//
// Created by BNunnally on 9/14/2020.
//

#ifndef SHARP_SERIALIZATION_H
#define SHARP_SERIALIZATION_H

#include "../../runtime/memory/sharp_object.h"

#define BUFFER_ALLOC_CHUNK_SIZE_STANDARD (128 * 128)
#define BUFFER_ALLOC_CHUNK_SIZE_LARGE (512 * 512)
#define BUFFER_ALLOC_CACHE_LIMIT (2048 * 2048)
#define SERIALIZE_START (0x3b)
#define SERIALIZE_END (0x3d)
#define BEGIN_OBJECT (0x3a)
#define CLASS_NAME_BEGIN (0x1a)
#define CLASS_NAME_END (0xb)
#define REFERENCE_OBJECT (0xf)
#define DATA_BEGIN (0xe)
#define DATA_END (0x2e)
#define ITEM_START (0x002)
#define ITEM_END (0x1c)
#define NULL_OBJECT (0x008)
#define OBJECT_ID_START (0x7F)

#define push_data(bufferSize, data) \
    if((bufferPos + 1) >= bufferSize) { \
        alloc_buffer(); \
    } \
    serializeBuffer[++bufferPos] = (data);

#define push_int32(bufferSize, data) \
    if((bufferPos + 4) >= bufferSize) { \
        alloc_buffer(); \
    } \
    serializeBuffer[++bufferPos] = GET_i32w(data); \
    serializeBuffer[++bufferPos] = GET_i32x(data); \
    serializeBuffer[++bufferPos] = GET_i32y(data); \
    serializeBuffer[++bufferPos] = GET_i32z(data);

#define formatted_buffer(pos) \
    ((char) deserializeBuffer[(pos)])

#define expect_data(bufferSize, data) \
    if((bufferPos + 1) >= bufferSize) { \
        throw vm_exception("invalid format: unexpected end of deserialization buffer");\
    } else if(formatted_buffer(++bufferPos) != data) { \
        throw vm_exception("unexpected data found in deserialization buffer");\
    }

#define read_int32(bufferSize, out) \
    if((bufferPos + 4) >= bufferSize) { \
        throw vm_exception("invalid format: unexpected end of deserialization buffer");\
    } \
    (out) = SET_i32(formatted_buffer(bufferPos+1), formatted_buffer(bufferPos+2), \
                formatted_buffer(bufferPos+3), formatted_buffer(bufferPos+4)); \
    bufferPos += 4;

#define read_data \
    formatted_buffer(++bufferPos)

void serialize(object *from, object *to);
void deserialize(object *from, object *to);

#endif //SHARP_SERIALIZATION_H
