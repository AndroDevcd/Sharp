//
// Created by BNunnally on 9/14/2020.
// Optimized by Claude on 3/26/2025
// Further performance optimizations by Claude on 4/4/2025
//

#ifndef SHARP_SERIALIZATION_H
#define SHARP_SERIALIZATION_H

#include "../../runtime/memory/sharp_object.h"

#define BUFFER_ALLOC_CHUNK_SIZE_STANDARD KB_TO_BYTES(24)
#define BUFFER_ALLOC_CHUNK_SIZE_LARGE MB_TO_BYTES(2)
#define SERIALIZE_START (0x3b)
#define SERIALIZE_END (0x3d)
#define REFERENCE_OBJECT (0xf)
#define DATA_BEGIN (0xe)
#define DATA_END (0x2e)
#define CLASS_SECTION_BEGIN (0x002)
#define CLASS_SECTION_END (0x1c)
#define NULL_OBJECT (0x008)
#define OBJECT_ID_START (0x0)
#define STANDARD_OBJECT (0x009)
#define NUMERIC_OBJECT (0x3a)
#define CLASS_OBJECT (0x1a)
#define CLASS_ARRAY_OBJECT (0x1b)

struct serialize_buffer_t
{
    char *buf = nullptr;
    int32_t size = 0;
    int32_t pos = -1;
};

struct deserialize_buffer_t
{
    long double *buf = nullptr;
    int32_t size = 0;
    int32_t pos = -1;
    sharp_object **references = nullptr;
};

struct serialized_classes_t
{
    sharp_class **serialized = nullptr;
    int32_t size = 0;
    int32_t count = 0;
};

// Main serialization functions
void serialize(object *from, object *to);
void deserialize(object *from, object *to);

// Helper functions for buffer management
void init_serialization();
void reset_serialization_state();
void clean_serialization_resources();

// For debugging and profiling
void print_serialization_stats();

#endif //SHARP_SERIALIZATION_H
